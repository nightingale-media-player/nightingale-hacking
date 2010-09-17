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
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const SBLDBCOMP = "@songbirdnest.com/Songbird/Library/LocalDatabase/";


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
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,

  // needs to be DOM_OBJECT to allow remoteAPI to access it.
  flags: Ci.nsIClassInfo.DOM_OBJECT,

  _initialized: false,
  
  _securityMixin: null,
  _initializedSCC: false,
  
  _publicWProps: [ "" ],
  _publicRProps: [ "classinfo:classDescription",
                   "classinfo:contractID",
                   "classinfo:classID",
                   "classinfo:implementationLanguage",
                   "classinfo:flags" ],
  _publicMethods: [ "internal:canMigrate",
                    "internal:getLatestSchemaVersion",
                    "internal:migrate"],
  _publicInterfaces: [ Ci.nsISupports,
                       Ci.nsIClassInfo,
                       Ci.nsISecurityCheckedComponent,
                       Ci.sbILocalDatabaseMigrationHelper,
                       Ci.sbIJobProgress,
                       Ci.sbIJobCancelable ],

  _latestSchemaVersion: 27,
  _lowestFromSchemaVersion: Number.MAX_VALUE,

  _migrationHandlers:   null,

  //
  // Local methods
  //

  _init: function sbLDBM_init() {
    this._getMigrationHandlerList();
    this._initialized = true;
  },

  _getMigrationHandlerList: function sbLDBM_getMigrationHandlerList() {
    this._migrationHandlers = {};
    
    for(let contractID in Cc) {
      if(contractID.indexOf(SBLDBCOMP + "Migration/Handler") == 0) {

        let migrationHandler = 
          Cc[contractID].createInstance(Ci.sbILocalDatabaseMigrationHandler);
        let migrationHandlerKey = migrationHandler.fromVersion + 
                                  "," + 
                                  migrationHandler.toVersion;
        this._migrationHandlers[migrationHandlerKey] = migrationHandler;
        
        if(this._lowestFromSchemaVersion > migrationHandler.fromVersion) {
          this._lowestFromSchemaVersion = migrationHandler.fromVersion;
        }
      }
    }
  },

  // 
  // sbILocalDatabaseMigration
  // 

  getLatestSchemaVersion: function sbLDBM_getLatestSchemaVersion() {
    return this._latestSchemaVersion;
  },

  canMigrate: function sbLDBM_canMigrate(aFromVersion, aToVersion) {
    if(!this._initialized) {
      this._init();
    }
    
    var oldVersion = aFromVersion;
    var newVersion = oldVersion + 1;
    
    for(let i = 0; i < aToVersion - aFromVersion; ++i) {
      let key = oldVersion + "," + newVersion;
      
      if(!(key in this._migrationHandlers)) {
        return false;
      }
      
      oldVersion = newVersion;
      newVersion += 1;
    }
    
    return true;
  },

  migrate: function sbLDBM_migrate(aFromVersion, aToVersion, aLibrary) {
    if(!this._initialized) {
      this._init();
    }
    
    var oldVersion = aFromVersion;
    var newVersion = oldVersion + 1;
    
    for(let i = 0; i < aToVersion - aFromVersion; ++i) {
      let key = oldVersion + "," + newVersion;
      
      if(!(key in this._migrationHandlers)) {
        dump("Failed to find migration handler " + key + "\n");
        throw Cr.NS_ERROR_UNEXPECTED;
      }
      try {
        this._migrationHandlers[key].migrate(aLibrary);
      } catch (e) {
        dump("Error in migrating " + oldVersion + " to " + newVersion + ":\n" +
             e + "\n");
        throw(e);
      }
      
      oldVersion = newVersion;
      newVersion += 1;
    }
    
    return;
  },


  //
  // nsISupports
  //

  QueryInterface: XPCOMUtils.generateQI([
    Ci.sbILocalDatabaseMigrationHelper,
    Ci.sbIJobProgress,
    Ci.sbIJobCancelable,
    Ci.nsIClassInfo,
    Ci.nsISecurityCheckedComponent,
    Ci.sbISecurityAggregator
  ]),


  // 
  // nsIClassInfo
  //

  getInterfaces: function sbLDBM_getInterfaces(count) {
    var interfaces = [Ci.sbILocalDatabaseMigrationHelper, 
                      Ci.nsIClassInfo,
                      Ci.sbIJobProgress,
                      Ci.sbIJobCancelable,
                      Ci.nsISecurityCheckedComponent,
                      Ci.sbISecurityAggregator,
                      Ci.nsISupports];
    
    count.value = interfaces.length;
    
    return interfaces;
  },

  getHelperForLanguage: function sbLDBM_getHelperForLanguage(aLanguage) {
    return null;
  },


  //
  // nsISecurityCheckedComponent -- implemented by the security mixin
  //

  _initSCC: function sbLDBM__initSCC() {
    this._securityMixin = Cc["@songbirdnest.com/remoteapi/security-mixin;1"]
                            .createInstance(Ci.nsISecurityCheckedComponent);

    // initialize the security mixin with the cleared methods and props
    this._securityMixin
             .init(this, this._publicInterfaces, this._publicInterfaces.length,
                         this._publicMethods, this._publicMethods.length,
                         this._publicRProps, this._publicRProps.length,
                         this._publicWProps, this._publicWProps.length,
                         false);

    this._initializedSCC = true;
  },

  canCreateWrapper: function sbLDBM_canCreateWrapper(iid) {
    if (! this._initializedSCC)
      this._initSCC();
    return this._securityMixin.canCreateWrapper(iid);
  },

  canCallMethod: function sbLDBM_canCallMethod(iid, methodName) {
    if (! this._initializedSCC)
      this._initSCC();
    return this._securityMixin.canCallMethod(iid, methodName);
  },

  canGetProperty: function sbLDBM_canGetProperty(iid, propertyName) {
    if (! this._initializedSCC)
      this._initSCC();
    return this._securityMixin.canGetProperty(iid, propertyName);
  },

  canSetProperty: function sbLDBM_canSetProperty(iid, propertyName) {
    if (! this._initializedSCC)
      this._initSCC();
    return this._securityMixin.canSetProperty(iid, propertyName);
  }
};

//
// Module
//

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLocalDatabaseMigrationHelper
  ]);
}
