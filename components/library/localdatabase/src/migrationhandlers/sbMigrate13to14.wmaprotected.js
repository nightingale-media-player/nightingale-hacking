/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 20;
const TO_VERSION = 21;

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
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{925d55b1-9fed-451b-857b-3cdb08150d8c}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  _errors: [],
  _progress: 0,
  _total: 0,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,
  migrate: function sbLibraryMigration_migrate(aLibrary) {
    if (!("@songbirdnest.com/Songbird/MetadataHandler/WMA;1" in Cc)) {
      // no WMA metadata handler - nothing to do here
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("commit");
      query.setAsyncQuery(false);
      if (query.execute() != 0) {
        throw("Query failed: " + query.getLastError());
      }
      return;
    }
    try{
      this.startNotificationTimer();
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Run a query to figure out the property ID for the new property
      var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                    .createInstance(Ci.sbIDatabaseQuery);
      query.databaseLocation = aLibrary.databaseLocation;
      query.setDatabaseGUID(aLibrary.databaseGuid);

      query.addQuery(<>
        INSERT OR IGNORE INTO properties
          (property_name) VALUES ("{SBProperties.isDRMProtected}");</>);
      query.addQuery(<>
        SELECT property_id FROM properties
          WHERE property_name = "{SBProperties.isDRMProtected}";
        </>);
      query.setAsyncQuery(false);
      if (query.execute() != 0) {
        throw "Failed to create property";
      }
      var propertyId = query.getResultObject().getRowCell(0, 0);

      // Run a query to look for all m4p files and assume they are protected :(
      query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                    .createInstance(Ci.sbIDatabaseQuery);
      query.databaseLocation = aLibrary.databaseLocation;
      query.setDatabaseGUID(aLibrary.databaseGuid);

      query.addQuery(<>
        INSERT OR REPLACE INTO resource_properties(
          media_item_id, property_id, obj, obj_searchable, obj_sortable)
          SELECT media_item_id, "{propertyId}", "1", "1", "1"
            FROM media_items
            WHERE content_url LIKE "%.m4p";
        </>);
      query.setAsyncQuery(false);
      if (query.execute() != 0) {
        throw "Media item fetch failed";
      }

      // Run a query to look for all wma files :(
      query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                    .createInstance(Ci.sbIDatabaseQuery);
      query.databaseLocation = aLibrary.databaseLocation;
      query.setDatabaseGUID(aLibrary.databaseGuid);

      query.addQuery(<>SELECT guid, content_url FROM media_items
                         WHERE content_url LIKE "file://%.wma"
                           COLLATE NOCASE</>);
      query.setAsyncQuery(false);
      if (query.execute() != 0) {
        throw "Media item fetch failed";
      }
      var result = query.getResultObject();

      function addChunk(guids) {
        return String(<>
          INSERT OR REPLACE INTO resource_properties(
            media_item_id, property_id, obj, obj_searchable, obj_sortable)
            SELECT media_item_id, "{propertyId}", "1", "1", "1"
              FROM media_items
              WHERE guid in ("{guids.join('", "')}");
          </>);
      }

      // find the guids of the tracks with DRM
      var handler = Cc["@songbirdnest.com/Songbird/MetadataHandler/WMA;1"]
                      .createInstance(Ci.sbIMetadataHandlerWMA);
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
      this._total = result.getRowCount();
      const CHUNK_SIZE = 1000;
      let guids = [];
      // Run a query that will mark the library as migrated
      query = this.createMigrationQuery(aLibrary);
      for (this._progress = 0; this._progress < this._total; ++this._progress) {
        let spec = result.getRowCell(this._progress, 1);
        try {
          let uri = ioService.newURI();
          if (!(uri instanceof Ci.nsIFileURL)) {
            // HUH? This shouldn't have matched our query...
            this._errors.push(spec);
            continue;
          }
          let file = uri.file;
          if (!file.exists()) {
            continue;
          }
          if (handler.isDRMProtected(file.path)) {
            guids.push(result.getRowCell(this._progress, 0));
          }
        } catch (e) {
          // silently eat all exceptions
          this._errors.push(spec);
        }
        if (guids.length >= CHUNK_SIZE) {
          query.addQuery(addChunk(guids));
          guids = [];
        }
      }

      if (guids.length > 0) {
        query.addQuery(addChunk(guids));
      }
      query.addQuery("commit");
      query.setAsyncQuery(false);
      if (query.execute() != 0) {
        throw("Query failed: " + query.getLastError());
      }
    }
    catch (e) {
      LOG("Exception occured: " + e);
      throw e;
    }
    finally {
      this.stopNotificationTimer();
    }
  },

///// sbIJobProgress
  get status() Ci.sbIJobProgress.STATUS_RUNNING,
  get blocked() false,
  get progress() this._progress,
  get total() this._total,
  get errorCount() this._errors.length,
  getErrorMessages: function sbLibMig20to21_getErrorMessages()
    ArrayConverter.StringEnumerator(this._errors),
  /// addJobProgressListener / removeJobProgressListener not implemented
///// sbIJobProgressCancellable
  get canCancel() false,
  cancel: function sbLibMig20to21_cancel() { throw Cr.NS_ERROR_NOT_IMPLEMENTED }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

