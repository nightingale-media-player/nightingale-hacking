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
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbJobUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const SBLDBCOMP = "@songbirdnest.com/Songbird/Library/LocalDatabase/";

function d(s) {
  //dump("------------------> sbLocalDatabaseMigration " + s + "\n");
}

function TRACE(s) {
  //dump("------------------> sbLocalDatabaseMigration " + s + "\n");
}

function sbLocalDatabaseMigrationHelper()
{
  SBJobUtils.JobBase.call(this);
  this._migrationHandlers = {};
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrationHelper.prototype = {
  __proto__: SBJobUtils.JobBase.prototype,
  
  classDescription: "Songbird Local Database Library Migration",
  classID:          Components.ID("{744f6217-4cb9-4929-8d1d-72492c1b8c83}"),
  contractID:       SBLDBCOMP + "MigrationHelper;1",
  constructor:      sbLocalDatabaseMigrationHelper,
  
  _initialized: false,
  
  _latestSchemaVersion: 5,
  _lowestFromSchemaVersion: Number.MAX_VALUE,
  
  _migrationHandlers:   null
}

//
// Local methods
//

sbLocalDatabaseMigrationHelper.prototype._init = 
function sbLDBM_init() {
  this._getMigrationHandlerList();
  this._initialized = true;
}

sbLocalDatabaseMigrationHelper.prototype._getMigrationHandlerList = 
function sbLDBM_getMigrationHandlerList() {
  this._migrationHandlers = [];
  
  for(let contractID in Cc) {
    if(contractID.indexOf(SBLDBCOMP + "Migration/Handler") == 0) {
      let migrationHandler = 
        Cc[contractID].createInstance(Ci.sbILocalDatabaseMigrationHandler);
      let migrationHandlerKey = migrationHandler.fromVersion + 
                                "," + 
                                migrationHandler.toVersion;
      this._migrationHandlers[migrationHandlerKey] = migrationHandler;
      
      if(this._lowestSchemaVersion > migrationHandler.fromVersion) {
        this._lowestSchemaVersion = migrationHandler.fromVersion;
      }
    }
  }
}

// 
// sbILocalDatabaseMigration
// 

sbLocalDatabaseMigrationHelper.prototype.__defineGetter__("latestSchemaVersion",
function() {
  return this._latestSchemaVersion;
});

sbLocalDatabaseMigrationHelper.prototype.canMigrate = 
function sbLDBM_canMigrate(aFromVersion, aToVersion) {
  if(!this._initialized) {
    this._init();
  }
  
  //XXXAus: PLACE HOLDER
  
  return false;
}

sbLocalDatabaseMigrationHelper.prototype.migrate = 
function sbLDBM_migrate(aFromVersion, aToVersion, aLibrary) {
  if(!this._initialized) {
    this._init();
  }
  
  //XXXAus: PLACE HOLDER
  
  return;
}


//
// nsISupports
//

sbLocalDatabaseMigrationHelper.prototype.QueryInterface =
XPCOMUtils.generateQI([
  Ci.sbILocalDatabaseMigrationHelper,
  Ci.sbIJobProgress,
  Ci.nsIClassInfo
]);

// 
// nsIClassInfo
//

sbLocalDatabaseMigrationHelper.prototype.getInterfaces = 
function sbLDBM_getInterfaces(count) {
  var interfaces = [Ci.sbILocalDatabaseMigrationHelper, 
                    Ci.nsIClassInfo,
                    Ci.sbIJobProgress,
                    Ci.nsISupports];
  
  count.value = interfaces.length;
  
  return interfaces;
}


//
// Module
//

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLocalDatabaseMigrationHelper
  ]);
}
