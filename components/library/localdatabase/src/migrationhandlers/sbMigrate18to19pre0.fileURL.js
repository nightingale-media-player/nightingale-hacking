/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 24;
const TO_VERSION = 25;

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
}

//-----------------------------------------------------------------------------
//
// sbLocalDatabaseMigration Implementation
//
//   Mac OS/X stores file paths using unicode normalization form D (NFD).
// However, when creating an nsIFile in xulrunner 1.9.0 using
// nsIFile.initWithPath, the nsIFile keeps the path in unicode normalization
// form C (NFC).  When such an nsIFile is converted to a local file URL, the URL
// encoding is NFC.
//   xulrunner 1.9.2 changed nsIFile so that the file path is maintained using
// the OS format, NFD on Mac OS/X.  In addition, in both xulrunner 1.9.0 and
// 1.9.2, enumerating files with nsIFile.directoryEntries produces file paths
// in the OS format, NFD on Mac OS/X.
//   Thus, in Songbird 1.8.0 and earlier, importing a playlist file (e.g., M3U)
// containing local files results in media items with content URLs in NFC format
// since the playlist importers use nsIFile.initWithPath for local files.
// However, importing via directory scan or importing playlists with Songbird
// 1.9.0 and later results in media items with local file content URLs in NFD
// format on Mac OS/X.
//   This migration fixes local file URLs by converting them to nsIFiles and
// back to URLs.  Doing so ensures that they're encoded in the native OS format
// (NFD on Mac OS/X).  The migration could directly do an NFC to NFD conversion,
// but converting them via nsIFile should better ensure that the final result is
// correct in case other conversions are required.
//   Since the content URL is copied to the origin URL when media items are
// copied to a device, the origin URL property is migrated as well as the
// content URL.
//   The album artwork uses nsIFile.initWithPath for local artwork files (e.g.,
// folder.jpg), so the primaryImageURL property needs to be migrated.
//   See http://bugzilla.songbirdnest.com/show_bug.cgi?id=21568 and
//   http://bugzilla.songbirdnest.com/show_bug.cgi?id=21612 for reference.
//
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{d460596d-aa22-4bd4-ae83-e82fb5a70019}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  _errors: [],
  _progress: 0,
  _total: 0,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,

  batchSize: 1000,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try {
      // Get the platform.
      var sysInfo = Cc["@mozilla.org/system-info;1"]
                      .getService(Ci.nsIPropertyBag2);
      var platform = sysInfo.getProperty("name");

      // Get the database GUID and location.
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Convert URL property values on Mac OS/X.
      if (platform == "Darwin") {
        this._convertTopLevelURLPropertyValues(aLibrary, "content_url");
        this._convertNonTopLevelURLPropertyValues(aLibrary,
                                                  SBProperties.originURL);
        this._convertNonTopLevelURLPropertyValues(aLibrary,
                                                  SBProperties.primaryImageURL);
      }

      // Run a query that will mark the library as migrated
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("commit");
      query.setAsyncQuery(false);
      query.execute();

      // Raise a flag indicating that this library will need all
      // sort info to be recomputed.
      // Normally we'd call propertyCache.invalidateSortData(), but
      // at this point in startup the property cache does not exist yet.
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("songbird.propertycache." +
                          this._databaseGUID + ".invalidSortData",
                        true);
      prefs.QueryInterface(Ci.nsIPrefService).savePrefFile(null);
    }
    catch (e) {
      LOG("Exception occured: " + e);
      throw e;
    }
  },


  /**
   * Convert all local file values for the top level URL property specified by
   * aPropertyName in the library specified by aLibrary.
   *
   * \param aLibrary            Library in which to convert property values.
   * \param aPropertyName       Name of property for which to convert values.
   */

  _convertTopLevelURLPropertyValues: function
    sbLibraryMigration__convertTopLevelURLPropertyValues(aLibrary,
                                                         aPropertyName) {
    // Set up the query strings.
    var selectQueryStr = <>SELECT guid, {aPropertyName} FROM media_items
                             WHERE {aPropertyName} LIKE "file:%"</>;
    var updatePreparedQueryStr = <>UPDATE media_items
                                     SET {aPropertyName} = ?
                                     WHERE guid = ?</>;

    // Convert the URL property values.
    this._convertURLPropertyValues(aLibrary,
                                   selectQueryStr,
                                   updatePreparedQueryStr);
  },


  /**
   * Convert all local file values for the non-top level URL property specified
   * by aPropertyName in the library specified by aLibrary.
   *
   * \param aLibrary            Library in which to convert property values.
   * \param aPropertyName       Name of property for which to convert values.
   */

  _convertNonTopLevelURLPropertyValues: function
    sbLibraryMigration__convertNonTopLevelURLPropertyValues(aLibrary,
                                                            aPropertyName) {
    // Get the property ID from the property name.
    var propertyID = this._getPropertyID(aLibrary, aPropertyName);

    // Set up the query strings.
    var selectQueryStr = <>SELECT media_item_id, obj
                             FROM resource_properties
                             WHERE property_id = "{propertyID}" AND
                                   obj LIKE "file:%"</>;
    var updatePreparedQueryStr = <>UPDATE resource_properties
                                     SET obj = ?
                                     WHERE media_item_id = ? AND
                                           property_id = "{propertyID}"</>;

    // Convert the URL property values.
    this._convertURLPropertyValues(aLibrary,
                                   selectQueryStr,
                                   updatePreparedQueryStr);
  },


  /**
   *   Convert all local file URL property values in the library specified by
   * aLibrary using the selection query string specified by aSelectionQueryStr
   * and the update prepared query string specified by aUpdatePreparedQueryStr.
   *   The selection query result should return an ID string (e.g., guid or
   * media item ID) in column 0 and the URL property value in column 1.
   *   The prepared query string should take the converted URL as parameter 0
   * and the ID string as parameter 1.
   *
   * \param aLibrary            Library in which to convert property values.
   * \param aSelectQueryStr     Item selection query string.
   * \param aUpdatePreparedQueryStr
   *                            Property update prepared query string.
   */

  _convertURLPropertyValues: function
    sbLibraryMigration__convertURLPropertyValues(aLibrary,
                                                 aSelectQueryStr,
                                                 aUpdatePreparedQueryStr) {
    // Select the list of URL property values to update.
    var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = aLibrary.databaseLocation;
    query.setDatabaseGUID(aLibrary.databaseGuid);
    query.addQuery(aSelectQueryStr);
    query.setAsyncQuery(false);
    if (query.execute() != 0)
      throw "Media item fetch failed";
    var result = query.getResultObject();

    // Convert property URLs to nsIFiles and back again.
    var preparedStatement;
    var queryCount = 0;
    query = null;
    this._total = result.getRowCount();
    for (this._progress = 0; this._progress < this._total; ++this._progress) {
      // Get the property info.
      var id = result.getRowCell(this._progress, 0);
      var url = result.getRowCell(this._progress, 1);

      // Convert the URL to an nsIFile and back.  Skip property if no converted
      // URL or converted URL is the same as the original.
      var convertedURL = this._convertURL(url);
      if (!convertedURL || (convertedURL == url))
        continue;

      // Set up query to write converted URL back.
      if (!query) {
        query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
        query.databaseLocation = aLibrary.databaseLocation;
        query.setDatabaseGUID(aLibrary.databaseGuid);
        preparedStatement = query.prepareQuery(aUpdatePreparedQueryStr);
      }
      query.addPreparedStatement(preparedStatement);
      query.bindStringParameter(0, convertedURL);
      query.bindStringParameter(1, id);
      queryCount++;

      // Execute query if batch is full.
      if (queryCount >= this.batchSize) {
        query.setAsyncQuery(false);
        if (query.execute() != 0)
          throw "Media item write failed";
        query = null;
        queryCount = 0;
      }
    }

    // Execute any remaining queries.
    if (query && (queryCount > 0)) {
      query.setAsyncQuery(false);
      if (query.execute() != 0)
        throw "Media item write failed";
    }
  },


  /**
   * Convert the local file URL specified by aURL to an nsIFile and back to a
   * URL and return the result.
   *
   * \param aURL                URL to convert.
   *
   * \return                    Converted URL.
   */

  _convertURL: function sbLibraryMigration__convertURL(aURL) {
    // Get the IO service.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);

    // Convert the URL to an nsIFile and back.
    try {
      var uri = ioService.newURI(aURL, null, null);
      var fileURL = uri.QueryInterface(Ci.nsIFileURL);
      var convertedURL = ioService.newFileURI(fileURL.file).spec;
    }
    catch (e) {
      LOG("URL should have been a file URL: " + aURL + ": " + e);
      return null;
    }

    return convertedURL;
  },


  /**
   * Return the property ID for the property with the name specified by
   * aPropertyName within the library specified by aLibrary.
   *
   * \param aLibrary            Library containing property.
   * \param aPropertyName       Name of property.
   *
   * \return                    Property ID.
   */

  _getPropertyID: function sbLibraryMigration__getPropertyID(aLibrary,
                                                             aPropertyName) {
    // Query for the property ID.
    var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = aLibrary.databaseLocation;
    query.setDatabaseGUID(aLibrary.databaseGuid);
    query.addQuery(<>SELECT property_id FROM properties
                       WHERE property_name = "{aPropertyName}"</>);
    query.setAsyncQuery(false);
    if (query.execute() != 0)
      throw "Property ID query failed";

    return query.getResultObject().getRowCell(0, 0);
  },


///// sbIJobProgress
  get status() Ci.sbIJobProgress.STATUS_RUNNING,
  get blocked() false,
  get progress() this._progress,
  get total() this._total,
  get errorCount() this._errors.length,
  getErrorMessages: function sbLibraryMigration_getErrorMessages()
    ArrayConverter.StringEnumerator(this._errors),
  /// addJobProgressListener / removeJobProgressListener not implemented
///// sbIJobProgressCancellable
  get canCancel() false,
  cancel: function sbLibraryMigration_cancel()
    { throw Cr.NS_ERROR_NOT_IMPLEMENTED }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

