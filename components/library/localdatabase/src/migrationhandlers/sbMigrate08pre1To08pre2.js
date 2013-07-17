/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(s) {
  dump("----++++----++++\sbLocalDatabaseMigrate08pre1to08pre2 ---> " + 
       s + 
       "\n----++++----++++\n");
}

function sbLocalDatabaseMigrate08pre1to08pre2()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 9;
  this.toVersion   = 10;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate08pre1to08pre2.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Songbird Migration Handler for 0.8pre1 to 0.8pre2, removing unused indices',
  classID: Components.ID("{3a67a390-8b2e-11dd-ad8b-0800200c9a66}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "0.8.0pre1to0.8.0pre2",
  
  _databaseLocation: null,
  _databaseGUID: null,
  
  _mediaItems: null,
  
  migrate: function sbLDBM08pre1to08pre2_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;

    var query = this._createQuery();

    // Drop unused indices
    query.addQuery("begin");
    query.addQuery("drop index idx_media_items_content_hash");
    query.addQuery("drop index idx_resource_properties_property_id_obj");
    query.addQuery("drop index idx_resource_properties_obj_sortable");
    query.addQuery("drop index idx_resource_properties_media_item_id_property_id_obj_sortable");

    // Update the schema version to the destination version.
    query.addQuery("update library_metadata set value = '10' where name = 'version'");
    query.addQuery("commit");

    // Try to reduce db size
    query.addQuery("VACUUM");

    var retval;
    query.setAsyncQuery(true);
    query.execute(retval);
    
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = this;
    
    this._titleText = "Library Migration Helper";
    this._statusText = "Compacting library database...";
    this.migrationQuery = query;
    
    this.startNotificationTimer();
    SBJobUtils.showProgressDialog(sip.data, null, 0);
    this.stopNotificationTimer();
  },
  
  _createQuery: function sbLDBM08pre1to08pre2_createQuery() {
    var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
  }
}


//
// Module
// 

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLocalDatabaseMigrate08pre1to08pre2]);

