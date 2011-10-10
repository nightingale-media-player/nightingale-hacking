/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 23;
const TO_VERSION = 24;

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
  classDescription: 'Nightingale Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{6f74cb1d-49b5-4475-99ca-63ebb3696a40}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try {
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Get all the smart playlists
      var dbQuery = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
                    .createInstance(Ci.sbIDatabaseQuery);
      dbQuery.databaseLocation = aLibrary.databaseLocation;
      dbQuery.setDatabaseGUID(aLibrary.databaseGuid);
      dbQuery.addQuery(<>
        SELECT guid FROM media_items
          WHERE media_list_type_id = "2" AND is_list = "1";
        </>);
      dbQuery.setAsyncQuery(false);
      if (dbQuery.execute() != 0) {
        throw "Failed to get the smart playlists";
      }

      // Insert all the smart playlist guids to the dirty db.
      // sbSmartMediaListsUpdater will update them later.
      var result = dbQuery.getResultObject();
      var length = result.getRowCount();

      // Run a query that will mark the library as migrated
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("commit");
      var retval;
      query.setAsyncQuery(false);

      if (length == 0) {
        query.execute(retval);
        return;
      }

      // Init the dirty db
      var smartListQuery = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
                           .createInstance(Ci.sbIDatabaseQuery);
      smartListQuery.setAsyncQuery(false);
      smartListQuery.setDatabaseGUID("nightingale");
      smartListQuery.addQuery(
          "CREATE TABLE IF NOT EXISTS smartplupd_update_video_lists " +
          "(listguid TEXT UNIQUE NOT NULL)");

      var prepStatement = smartListQuery.prepareQuery(
          "INSERT OR REPLACE INTO smartplupd_update_video_lists VALUES (?)");
      var guid;
      for (var i = 0; i < length; ++i) {
        guid = result.getRowCell(i, 0);
        smartListQuery.addPreparedStatement(prepStatement);
        smartListQuery.bindStringParameter(0, guid);
      }

      if (smartListQuery.execute() != 0) {
        throw "Failed to insert the guid to the dirty db";
      }
 
      query.execute(retval);
    }
    catch (e) {
      LOG("Exception occured: " + e);
      throw e;
    }
  },
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLibraryMigration
  ]);
}

