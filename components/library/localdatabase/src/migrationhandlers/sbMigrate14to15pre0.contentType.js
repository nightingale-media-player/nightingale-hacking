/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 21;
const TO_VERSION = 22;

function LOG(s) {
  dump("----++++----++++sbLibraryMigration " +
       FROM_VERSION + " to " + TO_VERSION + ": " +
       s +
       "\n----++++----++++\n");
}

function sbLibraryMigration()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Nightingale Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{73b78226-1dd2-11b2-92ba-c4ba81af8fc1}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,
  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try{
      this.startNotificationTimer();
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Run a query that will mark the library as migrated
      query = this.createMigrationQuery(aLibrary);
      query.addQuery(<>
        CREATE INDEX idx_media_items_content_mime_type
          ON media_items(content_mime_type);
        </>);
      query.addQuery(<>
        UPDATE media_items
          SET content_mime_type = "audio"
          WHERE content_mime_type IS NULL
            OR NOT content_mime_type IN ("audio", "video")
        </>);
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
  get progress() 0,
  get total() 0,
  get errorCount() 0,
  getErrorMessages: ArrayConverter.stringEnumerator([]),
  /// addJobProgressListener / removeJobProgressListener not implemented
///// sbIJobProgressCancellable
  get canCancel() false,
  cancel: function sbLibMig21to22_cancel() { throw Cr.NS_ERROR_NOT_IMPLEMENTED }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLibraryMigration
  ]);
}

