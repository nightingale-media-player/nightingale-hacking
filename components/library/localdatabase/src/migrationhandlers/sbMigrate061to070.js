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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(s) {
  dump("----++++----++++\nsbLocalDatabaseMigrate061to070 ---> " + 
       s + 
       "\n----++++----++++\n");
}

function sbLocalDatabaseMigrate061to070()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 5;
  this.toVersion   = 6;
  
  this._propertyNames = {};
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate061to070.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Songbird Migration Handler for 0.6.1 to 0.7.0',
  classID: Components.ID("{54A1D507-D085-4bfe-B729-1FFA13291C24}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "0.6.1 to 0.7.0",
  
  constructor : sbLocalDatabaseMigrate061to070,
  
  _databaseLocation: null,
  _databaseGUID: null,
  
  _propertyNames: null,
  
  migrate: function sbLDBM061to070_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;
    
    this._getPropertyNames();
    LOG(this._propertyNames);
    
    var query = this._createQuery();

    var getDataSQL = "";
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
  }
}


//
// Module
// 

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLocalDatabaseMigrate061to070
  ]);
}
