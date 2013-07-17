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
//  dump("----++++----++++\nsbLocalDatabaseMigrate061to070 ---> " + 
//       s + 
//       "\n----++++----++++\n");
}

function sbLocalDatabaseMigrate061to070()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 5;
  this.toVersion   = 6;
  
  this._propertyNames = {};
  this._resourceProperties = [];
  
  this._propertyManager = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                            .getService(Ci.sbIPropertyManager);
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate061to070.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Songbird Migration Handler for 0.6.1 to 0.7.0',
  classID: Components.ID("{54A1D507-D085-4bfe-B729-1FFA13291C24}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "0.6.1to0.7.0",
  
  _databaseLocation: null,
  _databaseGUID: null,
  
  _propertyNames: null,
  _resourceProperties: null,
  
  _propertyManager: null,
  
  migrate: function sbLDBM061to070_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;
    
    this._getPropertyNames();
    this._getResourceProperties();

    var query = this._createQuery();
    query.addQuery("begin");

    // Update all resource_properties entries that are text properties.
    // Regenerate all sortable values for these properties.    
    for(let i = 0; i < this._resourceProperties.length; ++i) {
      let propertyId = this._propertyNames[this._resourceProperties[i].propId];
      let mediaItemId = this._resourceProperties[i].itemId;
      let value = this._resourceProperties[i].o;
      
      if(this._propertyManager.hasProperty(propertyId)) {
        let propertyInfo = this._propertyManager.getPropertyInfo(propertyId);
        if(propertyInfo.type == "text") {
          let str = "update resource_properties set obj_sortable = ? where media_item_id = ? and property_id = ?";
          
          query.addQuery(str);
          
          let sortableValue = propertyInfo.makeSortable(value);
          
          query.bindStringParameter(0, sortableValue);
          query.bindStringParameter(1, mediaItemId);
          query.bindStringParameter(2, this._resourceProperties[i].propId);
          
          LOG("update resource_properties set obj_sortable = " + 
              sortableValue + 
              " where media_item_id = " + 
              mediaItemId + 
              " and property_id = " + this._resourceProperties[i].propId);
        }
      }
    }
    
    // Update the FTS table with the new values. Sadly this means we have to 
    // regenerate the _entire_ FTS table :(

    // First, drop the table.
    query.addQuery("drop table resource_properties_fts_all");
    
    // Second, create the table anew.
    query.addQuery("create virtual table resource_properties_fts_all using FTS3 ( alldata )");
    
    // Third, load the data in.
    var ftsDataSub = "select media_item_id, group_concat(obj_sortable) from resource_properties group by media_item_id";
    var ftsLoadData = "insert into resource_properties_fts_all (rowid, alldata) " + ftsDataSub;
    
    query.addQuery(ftsLoadData);
    
    // Finally, we updated the schema version to the destination version.
    query.addQuery("update library_metadata set value = '6' where name = 'version'");
    
    // Our queries are all generated. Time to execute the migration.
    query.addQuery("commit");

    var retval;
    query.setAsyncQuery(true);
    query.execute(retval);
    
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = this;
    
    this._titleText = "Library Migration Helper";
    this._statusText = "Migrating 0.6.1 database to 0.7.0 database...";
    this.migrationQuery = query;
    
    this.startNotificationTimer();
    SBJobUtils.showProgressDialog(sip.data, null, 0);
    this.stopNotificationTimer();
  },
  
  _createQuery: function sbLDBM061to070_createQuery() {
    var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
  },  
  
  _getPropertyNames: function sbLDBM061to070_getPropertyNames() {
    var query = this._createQuery();
    var str = "select property_id, property_name from properties";
    query.addQuery(str);
    
    var retval;
    query.execute(retval);
    
    var resultSet = query.getResultObject();
    
    var rowCount = resultSet.getRowCount();
    for(let currentRow = 0; currentRow < rowCount; ++currentRow) {
      let propertyId = resultSet.getRowCell(currentRow, 0);
      this._propertyNames[propertyId] = resultSet.getRowCell(currentRow, 1);
    }
  },
  
  _getResourceProperties: function sbLDBM061to070_getResourceProperties() {
    var query = this._createQuery();
    var str = "select media_item_id, property_id, obj from resource_properties";
    query.addQuery(str);
    
    var retval;
    query.execute(retval);
    
    var resultSet = query.getResultObject();
    
    var rowCount = resultSet.getRowCount();
    for(let currentRow = 0; currentRow < rowCount; ++currentRow) {
      let mediaItemId = resultSet.getRowCell(currentRow, 0);
      let propertyId = resultSet.getRowCell(currentRow, 1);
      let obj = resultSet.getRowCell(currentRow, 2);
      
      var entry = {itemId: mediaItemId, propId: propertyId, o: obj };
      this._resourceProperties.push(entry);
    }
  }
}


//
// Module
// 

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLocalDatabaseMigrate061to070]);

