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

EXPORTED_SYMBOLS = ["SBLocalDatabaseMigrationUtils"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");

var SBLocalDatabaseMigrationUtils = {
  baseHandlerContractID: "@songbirdnest.com/Songbird/Library/LocalDatabase/Migration/Handler/"
}

/**
 *
 */
SBLocalDatabaseMigrationUtils.BaseMigrationHandler = function() {
  SBJobUtils.JobBase.call(this);
}

SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype = {
  __proto__               : SBJobUtils.JobBase.prototype,
  
  QueryInterface          : XPCOMUtils.generateQI(
      [Ci.sbIJobProgress, Ci.sbIJobCancelable, 
       Ci.sbILocalDatabaseMigrationHandler, Ci.nsIClassInfo]),

  /** nsIClassInfo, so callers don't have to QI from JS **/
  
  classDescription        : 'Songbird Base Local Database Migration Class',
  classID                 : null,
  contractID              : null,
  flags                   : Ci.nsIClassInfo.MAIN_THREAD_ONLY,
  implementationLanguage  : Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage    : function(aLanguage) { return null; },
  getInterfaces : function(count) {
    var interfaces = [Ci.sbIJobProgress,
                      Ci.sbIJobCancelable,
                      Ci.sbILocalDatabaseMigrationHandler,
                      Ci.nsIClassInfo,
                      Ci.nsISupports
                     ];
    count.value = interfaces.length;
    return interfaces;
  },
  
  migrationQuery: null,

  //
  // Helpers to pump progress
  //
  _notifyJobsTimer: null,
  
  startNotificationTimer: function BaseMigrationHandler_startNotificationTimer(aTimeout) {
    if(this._notifyJobsTimer) {
      this._notifyJobsTimer.cancel();
      this._notifyJobsTimer = null;
    }
    
    if(aTimeout == null) {
      aTimeout = 66;
    }
    
    var self = this;
    function notifyListeners() {
      self.notifyJobProgressListeners();
    }
    
    this._notifyJobsTimer = new SBTimer(notifyListeners,
                                        aTimeout, 
                                        Ci.nsITimer.TYPE_REPEATING_SLACK);
  },
  
  stopNotificationTimer: function BaseMigrationHandler_stopNotificationTimer() {
    if(this._notifyJobsTimer) {
      this._notifyJobsTimer.cancel();
      this._notifyJobsTimer = null;
    }
  },
  
  //
  // sbIJobProgress overrides
  //
  get status() {
    if(this.migrationQuery) {
      var executing = this.migrationQuery.isExecuting();
      if(executing) {
        return Ci.sbIJobProgress.STATUS_RUNNING;
      }
      
      var complete = (this.migrationQuery.currentQuery() == 
                      this.migrationQuery.getQueryCount() - 1);
                      
      if(complete && !executing) {
        return Ci.sbIJobProgress.STATUS_SUCCEEDED;
      }
    }
    
    return this._status;
  },
  
  get progress() {
    if(this.migrationQuery) {
      return this.migrationQuery.currentQuery();
    }
    
    return this._progress;    
  },
  
  get total() {
    if(this.migrationQuery) {
      return this.migrationQuery.getQueryCount();
    }
    
    return this._total;
  },
  
  get canCancel() {
    if(this.migrationQuery) {
      return true;
    }
    
    return this._canCancel;
  },
  
  cancel: function BaseMigrationHandler_cancel() {
    if(this.migrationQuery) {
      this.migrationQuery.abort();
    }
    
    return;
  },
  
  //
  // sbILocalDatabaseMigrationHandler
  //
  
  /**
   * Set this to the version the handler can migrate from.
   */
  fromVersion : 0,
  
  /**
   * Set this to the version the handler migrates to.
   */
  toVersion   : 0,
  
  /**
   * Override this method to provide migration of a library.
   */
  migrate     : function BaseMigrationHandler_migrate(aLibrary) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
}
