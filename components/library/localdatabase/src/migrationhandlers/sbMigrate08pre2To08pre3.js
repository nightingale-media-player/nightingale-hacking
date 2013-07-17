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
  dump("----++++----++++sbLibraryMigration " + 
       sbLibraryMigration.versionString + ": " + 
       s + 
       "\n----++++----++++\n");
}

function sbLibraryMigration()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 10;
  this.toVersion   = 11;
  this.versionString = this.fromVersion + " to " + 
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Songbird Migration Handler, version ' + this.versionString,
  classID: Components.ID("{62bb8fb0-8fd8-11dd-ad8b-0800200c9a66}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "10to11",

  _databaseLocation: null,
  _databaseGUID: null,
  
  _mediaItems: null,
  
  migrate: function sbLibraryMigration_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;

    var query = this._createQuery();

    query.addQuery("begin");

    // Add the new secondary sort column
    query.addQuery("alter table resource_properties add obj_secondary_sortable text");

    // Rebuild a new properties index
    query.addQuery("drop index idx_resource_properties_property_id_obj_sortable_media_item_id");
    query.addQuery("create index idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id " + 
                   "on resource_properties (property_id, obj_sortable, obj_secondary_sortable, media_item_id);");

    // Update analyze table
    query.addQuery("delete from sqlite_stat1");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('simple_media_lists','idx_simple_media_lists_media_item_id_ordinal','10003 5002 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('simple_media_lists','idx_simple_media_lists_member_media_item_id','10003 2');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('simple_media_lists','idx_simple_media_lists_media_item_id_member_media_item_id','10003 5002 1 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('resource_properties','idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id','133429 3707 6 4 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('resource_properties','sqlite_autoindex_resource_properties_1','133429 14 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('properties','sqlite_autoindex_properties_1','89 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('library_media_item','sqlite_autoindex_library_media_item_1','1 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('resource_properties_fts_all_segdir','sqlite_autoindex_resource_properties_fts_all_segdir_1','18 6 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_list_types','sqlite_autoindex_media_list_types_1','3 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_hidden_media_list_type_id','10009 5005 2');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_is_list','10009 5005');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_media_list_type_id','10009 2');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_content_url','10009 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_created','10009 12');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','idx_media_items_hidden','10009 5005');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('media_items','sqlite_autoindex_media_items_1','10009 1');");
    query.addQuery("INSERT INTO sqlite_stat1 VALUES('library_metadata','sqlite_autoindex_library_metadata_1','2 1');");

    // Update the schema version to the destination version.
    query.addQuery("update library_metadata set value = '" 
                   + this.toVersion + "' where name = 'version'");
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
    this._statusText = "Optimizing sort performance...";
    this.migrationQuery = query;
    
    this.startNotificationTimer();
    SBJobUtils.showProgressDialog(sip.data, null, 0);
    this.stopNotificationTimer();
    
    // Raise a flag indicating that this library will need all 
    // sort info to be recomputed.
    // Normally we'd call propertyCache.invalidateSortData(), but 
    // at this point in startup the property cache does not exist yet.
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);
    prefs.setBoolPref("songbird.propertycache." + 
        this._databaseGUID + ".invalidSortData", true);
    prefs.QueryInterface(Ci.nsIPrefService).savePrefFile(null);
  },
  
  _createQuery: function sbLibraryMigration_createQuery() {
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

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);
