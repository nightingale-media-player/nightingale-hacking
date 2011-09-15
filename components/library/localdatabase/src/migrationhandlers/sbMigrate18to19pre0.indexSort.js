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
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 27;
const TO_VERSION = 28;

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
  classID: Components.ID("{7f77e6fa-b3b0-40d5-88f8-203482abfbd9}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try {
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Run a query that will mark the library as migrated
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("CREATE INDEX idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id_asc on resource_properties (property_id, obj_sortable ASC, obj_secondary_sortable ASC, media_item_id ASC)");
      query.addQuery("CREATE INDEX idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id_desc on resource_properties (property_id, obj_sortable DESC, obj_secondary_sortable DESC, media_item_id DESC)");
      query.addQuery("REINDEX");
      query.addQuery("ANALYZE");
      query.addQuery("COMMIT");

      this.migrationQuery = query;
      
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                  .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = this;
      
      this._titleText = "Library Migration Helper";
      this._statusText = "Migrating 1.8 database to 1.9 database...";

      query.setAsyncQuery(true);
      query.execute();
      
      this.startNotificationTimer();
      SBJobUtils.showProgressDialog(sip.data, null, 0);
      this.stopNotificationTimer();
    }
    catch (e) {
      dump("Exception occured: " + e);
      throw e;
    }
  }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLibraryMigration
  ]);
}

