/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
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
  
  this.fromVersion = 14;
  this.toVersion   = 15;
  this.versionString = this.fromVersion + " to " + 
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Nightingale Migration Handler, version 14 to 15',
  classID: Components.ID("{3e473356-a84a-4243-a597-6919cb1a26ff}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + ' 14 to 15',

  _databaseLocation: null,
  _databaseGUID: null,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try{

      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      var query = this._createQuery();
      
      // because sqlite does not support ALTER COLUMN, we have to create
      // a new table and replace the old one.
      
      // create the new resource_properties table with correct collation sequences
      query.addQuery("create table resource_properties_2 (" +
                       "media_item_id integer not null," +
                       "property_id integer not null," +
                       "obj text not null," +
                       "obj_searchable text," +
                       "obj_sortable text collate library_collate," +
                       "obj_secondary_sortable text collate library_collate," +
                       "primary key (media_item_id, property_id)" +
                     ");");

      // copy the data from the old table to the new. note that we drop the obj_*
      // data because we are going to recompute it anyway. this boosts the
      // creation of the index, since all the comparisons will be on empty
      // strings.
      query.addQuery("insert into resource_properties_2 (media_item_id, property_id, obj) select media_item_id, property_id, obj from resource_properties;");

      // drop the old table
      query.addQuery("drop table resource_properties;");

      // rename the new table into the old one
      query.addQuery("alter table resource_properties_2 rename to resource_properties;");

      // recreate the index
      query.addQuery("create index idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id on resource_properties " +
                     "(property_id, obj_sortable, obj_secondary_sortable, media_item_id);");
      
      // Update the schema version to the destination version.
      query.addQuery("update library_metadata set value = '" 
                     + this.toVersion + "' where name = 'version'");
      query.addQuery("commit");

      // Try to reduce db size
      query.addQuery("VACUUM");

      var retval;
      query.setAsyncQuery(true);
      query.execute(retval);
      
      this._titleText = "Library Migration Helper";
      this._statusText = "Improving sorting data...";
      this.migrationQuery = query;
      
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                   .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = this;

      this.startNotificationTimer();
      SBJobUtils.showProgressDialog(sip.data, null, 0);
      this.stopNotificationTimer();

      // Raise a flag indicating that this library will need all 
      // sort info to be recomputed.
      // Normally we'd call propertyCache.invalidateSortData(), but 
      // at this point in startup the property cache does not exist yet.
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("nightingale.propertycache." + 
            this._databaseGUID + ".invalidSortData", true);
      prefs.QueryInterface(Ci.nsIPrefService).savePrefFile(null);
    }
    catch (e) {
      dump("Exception occured: " + e);
      throw e;
    }
  },

  _createQuery: function sbLibraryMigration_createQuery() {
    var query = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
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

