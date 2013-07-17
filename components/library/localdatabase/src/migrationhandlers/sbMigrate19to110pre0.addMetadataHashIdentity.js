/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/GeneratorThread.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");

const FROM_VERSION = 28;
const TO_VERSION = 29;

function LOG(s) {
  dump("----++++----++++sbLibraryMigration " +
       FROM_VERSION + " to " + TO_VERSION + ": " +
       s +
       "\n----++++----++++\n");
}

function sbLibraryMigration()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  this._errors = [];

  /* We handle each contentType individually, and we keep extra information
   * about each in these helper objects */
  this._contentMigrations = {
    "audio": {
      /* The properties used when hashing metadata of an audio mediaitem.
       * The order here is important and should match the order used by
       * sbIdentityService */
      props: [
        SBProperties.trackName,
        SBProperties.artistName,
        SBProperties.albumName,
        SBProperties.genre
      ],

      // flag to indicate that audio files have been migrated
      completed: false,
    },

    "video": {
      /* The properties used when hashing metadata of a video mediaitem.
       * The order here is important and should match the order used by
       * sbIdentityService */
      props: [
        SBProperties.trackName,
        SBProperties.artistName,
        SBProperties.albumName,
        SBProperties.genre
      ],

      // flag to indicate that video files have been migrated
      completed: false,
    },
  }

  /* The separator character to put between each property when concatenated
   * for hashing.  This should match that used by sbIdentityService */
  this.separator = '|';

  /* Initial values for sbIJobProgress attributes, will be updated when the
   * number of hashes that will need to be calculated is known */
  this._progress = 0;
  this._total = 0;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{4eba22e9-d657-4599-a181-b8340852e7a2}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,

  migrate: function sbLibraryMigration_migrate(aLibrary) {

    this._library = aLibrary;

    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = this;

    /* We use a generator thread so that we can update the progress dialog
     * periodically throughout the migration.  That only takes relatively
     * infrequent updates and low CPU so give the migration thread more CPU
     * and a longer period */
    this._thread = new GeneratorThread(this.processItems());
    this._thread.maxPctCPU = 95;
    this._thread.period = 50;
    this._thread.start();

    // Show the progress dialog tethered to this job
    SBJobUtils.showProgressDialog(sip.data, null, 0);
  },

  processItems: function sbLibraryMigration_processItems() {
    try {
      this._databaseGUID = this._library.databaseGuid;
      this._databaseLocation = this._library.databaseLocation;

      // Set some basics for the dialog and check if we should update the dialog
      this._titleText = "Library Migration Helper";
      this._statusText = "Preparing to migrate 1.9 database to 1.10 database...";
      yield this.checkIfShouldUpdateAndYield();

      // first query is for adding our new metadata hash col and index on it
      var query = this.createMigrationQuery(this._library);
      query.addQuery("alter table media_items add column metadata_hash_identity");
      query.addQuery("alter table library_media_item add column metadata_hash_identity");
      query.addQuery("create index idx_media_items_metadata_hash_identity" +
                      "on media_items (metadata_hash_identity)");

      query.addQuery("REINDEX");
      query.addQuery("ANALYZE");
      query.addQuery("COMMIT");

      query.setAsyncQuery(true);
      query.execute();

      /* With hash column added, now we generate a hash for each existing item.
       * The properties used for each contentType could be different so we
       * handle them separately. */
      for (var contentType in this._contentMigrations) {
        yield this.hashExistingMediaItems(contentType);
      }
    }
    catch (e) {
      dump("Exception occured: " + e);
      throw e;
    }
  },

  hashExistingMediaItems: function sbLibraryMigration_hashExistingMediaItems
                                   (contentType) {
    /* Make maps of property_name to property_id for the properties used when
     * hashing audio and video files. The ids are used to get the property
     * values for each mediaitem. */
    var propNames = this._contentMigrations[contentType].props;
    var propMap = this.getPropertyIDs(propNames);

    var idService = Cc["@songbirdnest.com/Songbird/IdentityService;1"]
                      .getService(Ci.sbIIdentityService);

    /* Build a query that will associate each media_item_id to all it's relevant
     * properties in a single row so that we can easily access, concatenate, and
     * then hash those properties.
     * The order of the properties will be the same as in the passed propNames
     * and should match the order used in sbIdentityService.  As content_type is
     * not stored in the resource_properties table but needs to be hashed,
     * it is explicitly added in the initialization of selectSQL */
    let selectSQL = "SELECT media_items.guid, media_items.content_mime_type";
    let fromSQL = "FROM media_items"
    let joinSQL = "";
    let whereSQL = "WHERE content_mime_type = '" + contentType + "'" +
                   " AND media_items.is_list = '0'";

    for (var i = 0; i < propNames.length; i++) {
      // Check if it's time for us to yield, and update the dialog if so
      yield this.checkIfShouldUpdateAndYield();

      var propID = propMap[propNames[i]];
      var resourcePropAlias = 'rp' + i;
      selectSQL += ", " + resourcePropAlias + ".obj"; // rp_.obj
      joinSQL += " LEFT OUTER JOIN resource_properties as " + resourcePropAlias +
                 " ON media_items.media_item_id = " +
                   resourcePropAlias + ".media_item_id" +
                 " AND " + resourcePropAlias + ".property_id = " + propID;
    }

    // When finished the query will look like:
      /* SELECT media_items.guid, media_items.content_mime_type,
       *  rp0.obj, rp1.obj... */
      /* FROM media_items */
      /* LEFT OUTER JOIN resource_properties as rp0
       *  ON media_items.media_item_id = rp0.media_item_id
       *  AND rp0.property_id = <property_id>
       * LEFT OUTER JOIN resource_properties as rp1
       *  ON media_items.media_item_id = rp1.media_item_id
       *  AND rp1.property_id = <property_id> */
      /* WHERE content_mime_type = '<contentType>'"
       *  AND media_items.is_list = '0' */

    let sql = selectSQL + " " +
              fromSQL + " " +
              joinSQL + " " +
              whereSQL;
    var selectPropertiesQuery = this.createMigrationQuery(this._library);
    selectPropertiesQuery.addQuery(sql);

    // execute the query to retrieve the property data
    var retval;
    selectPropertiesQuery.execute(retval);

    /* The updateQuery will push all of the identities to the database.
     * We add an update statement to updateQuery for each mediaitem as the
     * identity for that mediaitem is calculate.
     * All of the update statements are of a similar form, so we use the
     * preparedUpdateStatement below. */
    var updateQuery = this.createMigrationQuery(this._library);
    updateQuery.addQuery("BEGIN");

    var preparedUpdateStatement = updateQuery.prepareQuery
        ("UPDATE media_items SET metadata_hash_identity = ? WHERE guid = ?");

    /* Our last query retrieved the property data for each mediaitem, so we'll
     * walk through that to form our string that will be hashed */
    var propertyResultSet = selectPropertiesQuery.getResultObject();
    var colCount = propertyResultSet.getColumnCount();
    var rowCount = propertyResultSet.getRowCount();

    /* We know how many identities we will need to calculate, so make that the
     * migration total.  The dialog will be updated the next time we are
     * told to yield in the following identity-calculation loop. */
    this._total = rowCount;
    this._statusText = "Migrating " + contentType + " files in 1.9 database to 1.10 database...";

    var idService = Cc["@songbirdnest.com/Songbird/IdentityService;1"]
                      .getService(Ci.sbIIdentityService);
    for(let currentRow = 0; currentRow < rowCount; currentRow++) {
      // Check if it's time for us to yield, and update the dialog if so
      yield this.checkIfShouldUpdateAndYield();

      var guid = propertyResultSet.getRowCell(currentRow, 0);

      var hasHashableMetadata = false;
      var propsToHash = [];

      // Form the string that will be hashed to form the identity
      for (let currentCol = 1; currentCol < colCount; currentCol++) {
        let propVal = propertyResultSet.getRowCell(currentRow, currentCol);

        if (propVal) {
          propsToHash.push(propVal);
          hasHashableMetadata = true;
        }
        else {
          propsToHash.push("");
        }
      }

      /* If there was hashable metadata, calculate the identity now and add it
       * to the update query */
      if (hasHashableMetadata) {
        var stringToHash = propsToHash.join(this.separator);
        var identity = idService.hashString(stringToHash);
        updateQuery.addPreparedStatement(preparedUpdateStatement);
        updateQuery.bindStringParameter(0, identity);
        updateQuery.bindStringParameter(1, guid);
      }

      // This lets the dialog know that another item's identity was calculated
      this._progress++;
    }

    updateQuery.addQuery("commit");
    updateQuery.execute(retval);

    /* We have finished handling this contentType. Mark this type as completed
     * and check if all contentTypes have been completed.  If no contentType
     * still needs to be handled, mark the job as completed successfully and
     * inform the dialog */
    this._contentMigrations[contentType].completed = true;
    for (var contentType in this._contentMigrations) {
      if (!this._contentMigrations[contentType].completed) {
        return;
      }
    }

    this._status = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this.notifyJobProgressListeners();
  },

  /* Takes an array of propertyNames and returns a map of those property_names
   * to their corresponding property_id in the db */
  getPropertyIDs: function sbLibraryMigration_getPropertyIDs(propertyNames) {
    var propMap = {};
    var sql = "SELECT property_name, property_id FROM properties WHERE " +
              "property_name = '";
    sql += propertyNames.join("' OR property_name = '");
    sql += "'";

    // When finished the query will look like:
      /* SELECT property_name, property_id
       * FROM properties
       * WHERE property_name = '<propertyNames[0]>'
       *   OR property_name = '<propetyNames[1]>'... */

    var query = this.createMigrationQuery(this._library);
    query.addQuery(sql);

    var retval;
    query.execute(retval);

    var resultSet = query.getResultObject();

    /* The rows of the result are of the form | property_name | property_id |
     * for each of the property names that we were passed and could find.
     * Fill the propMap with the appropriate
     * property_name => property_id mappings. */
    var propertyIDs = [];
    var rowCount = resultSet.getRowCount();
    for(let currentRow = 0; currentRow < rowCount; ++currentRow) {
      propName = resultSet.getRowCell(currentRow, 0);
      propId = resultSet.getRowCell(currentRow, 1);
      propMap[propName] = propId;
    }
    return propMap;
  },

  /* This utility method checks if it is time to yield, and if it is, we
   * notify the dialog so that it will pick up the updated progress
   * and then we yield so the dialog can update itself. */
  checkIfShouldUpdateAndYield: function
    sbLibraryMigration_checkIfShouldUpdateAndYield() {

    /* GeneratorThread handles controlling when we should yield, but we need to
     * check it to know if that time has come. */
    if (GeneratorThread.shouldYield()) {
      this.notifyJobProgressListeners();
      yield;
    }
  },

 /* We override these methods to report the current state in the
  * progress dialog. */
  get status() {
    return this._status;
  },
  get progress() {
    return this._progress;
  },
  get total() {
    return this._total;
  },
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

