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
  dump("----++++----++++\nsbLocalDatabaseMigrate070RC2to070 ---> " + 
       s + 
       "\n----++++----++++\n");
}

function sbLocalDatabaseMigrate070RC1to070()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 7;
  this.toVersion   = 8;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate070RC1to070.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Nightingale Migration Handler for 0.7.0 RC2 to 0.7.0',
  classID: Components.ID("{116C3A20-DD68-45E9-81FD-EBEEBFD21BF6}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "0.7.0 RC2 to 0.7.0",
  
  _databaseLocation: null,
  _databaseGUID: null,
  
  _mediaItems: null,
  
  migrate: function sbLDBM070RC1to070_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;
    
    this._getMediaItems();
    
    var query = this._createQuery();
    query.addQuery("begin");

    for(let i = 0; i < this._mediaItems.length; ++i) {
      let str = "update media_items set is_list = ? where media_item_id = ?";
      query.addQuery(str);
			
      query.bindInt32Parameter(0, this._mediaItems[i].isList);
      query.bindInt32Parameter(1, this._mediaItems[i].itemId);
    }
    
    // Finally, we updated the schema version to the destination version.
    query.addQuery("update library_metadata set value = '8' where name = 'version'");

    // Our queries are all generated. Time to execute the migration.
    query.addQuery("commit");

    var retval;
    query.setAsyncQuery(true);
    query.execute(retval);
    
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = this;
    
    this._titleText = "Library Migration Helper";
    this._statusText = "Migrating 0.7.0 RC2 database to 0.7.0 database...";
    this.migrationQuery = query;
    
    this.startNotificationTimer();
    SBJobUtils.showProgressDialog(sip.data, null, 0);
    this.stopNotificationTimer();
  },
  
  _createQuery: function sbLDBM070RC1to070_createQuery() {
    var query = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
  },
  
  _getMediaItems: function sbLDBM070RC1to070_getMediaItems() {
    this._mediaItems = [];
    
    let sql = "select media_item_id, media_list_type_id from media_items where media_list_type_id is not null";
    
    var query = this._createQuery();
    query.addQuery(sql);
    
    var retval;
    query.execute(retval);
    
    var resultSet = query.getResultObject();
    
    var rowCount = resultSet.getRowCount();
    for(let currentRow = 0; currentRow < rowCount; ++currentRow) {
      let _mediaItemId = resultSet.getRowCell(currentRow, 0);
      let _listType = resultSet.getRowCell(currentRow, 1);
      let _isList = Number(parseInt(_listType) > 0);

      var entry = {itemId: _mediaItemId, isList: _isList};
      this._mediaItems.push(entry);
    }
  }
}


//
// Module
// 

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLocalDatabaseMigrate070RC1to070
  ]);
}
