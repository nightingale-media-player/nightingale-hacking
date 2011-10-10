/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

//------------------------------------------------------------------------------

//
// \brief This test will test the shutdown service by creating three dummy
//        shutdown tasks and invoking the shutdown service manually.
//
function runTest()
{
  LOG("Starting the shutdown service unit test");

  var controller = new sbShutdownTestController();
  controller.startTest();

  testPending();
}

//------------------------------------------------------------------------------
// Shutdown test controller class

function sbShutdownTestController()
{
}

sbShutdownTestController.prototype = {
  _listeners: [],

  //
  // \brief Init the test 
  //
  startTest: function() {
    this._listeners.push(new sbTestTask("Test Task 1"));
    this._listeners.push(new sbTestTask("Test Task 2"));
    this._listeners.push(new sbTestTask("Test Task 3"));

    // Listen to the shutdown service via |sbIJobProgressListener|.
    var shutdownService = Cc["@getnightingale.com/shutdown-service;1"]
                            .getService(Ci.sbIJobProgress);
    shutdownService.addJobProgressListener(this);

    // Trick invoke the shutdown service
    shutdownService.QueryInterface(Ci.nsIObserver);
    var fakeBool = Cc["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Ci.nsISupportsPRBool);
    shutdownService.observe(fakeBool, 
                            "quit-application-requested", 
                            "is-unit-test");

    // Make sure that the service modified the bool to cancel shutdown
    assertTrue(fakeBool.data);
  },

  // sbIJobProgressListener
  onJobProgress: function(aJobProgress) {
    // Abort if the shutdown service failed.
    assertNotEqual(aJobProgress.status, Ci.sbIJobProgress.STATUS_FAILED);
  
    if (aJobProgress.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      LOG("The shutdown service has successfully completed");

      // Remove ourselves as a job listener
      var shutdownService = Cc["@getnightingale.com/shutdown-service;1"]
                              .getService(Ci.sbIJobProgress);
      shutdownService.removeJobProgressListener(this);
      
      // Cleanup the tasks
      this._listeners.splice(0);
      
      testFinished();
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIJobProgressListener])
};

//------------------------------------------------------------------------------
// Dummy test shutdown task 

function sbTestTask(aTitleText)
{
  this._titleText = aTitleText;

  var observerService = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  observerService.addObserver(this, "nightingale-shutdown", false);
}

sbTestTask.prototype = 
{
  _jobListener   : null,
  _timer         : null,
  status         : Ci.sbIJobProgress.STATUS_RUNNING,
  blocked        : false,
  statusText     : "Testing Shutdown Job",
  titleText      : "",
  progress       : 0,
  total          : 0,

  // sbIJobProgress
  get errorCount() {
    return 0;
  },

  getErrorMessages: function() {
    return null;
  },

  addJobProgressListener: function(aJobListener) {
    // Don't bother with tracking multiple listeners right now.
    this._jobListener = aJobListener;
  },

  removeJobProgressListener: function(aJobListener) {
    this._jobListener = null;
  },

  // sbIShutdownTask
  get needsToRunTask() {
    return true;
  },
  
  startTask: function() {
    LOG("Starting shutdown task: " + this._titleText);
    
    this.status = Ci.sbIJobProgress.STATUS_RUNNING;
    this._jobListener.onJobProgress(this);

    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback(this, 3000, Ci.nsITimerCallback.TYPE_ONE_SHOT); 
  },

  // nsIObserver
  observe: function(aSubject, aTopic, aData) {
    // Do nothing
  },

  // nsITimerCallback
  notify: function(aTimer) {
    LOG("Completed shutdown task: " + this._titleText);

    // Cleanup with the observer service
    var observerService = Cc["@mozilla.org/observer-service;1"]
                            .getService(Ci.nsIObserverService);
    observerService.removeObserver(this, "nightingale-shutdown");

    this.status = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._jobListener.onJobProgress(this);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, 
                                         Ci.nsITimerCallback,
                                         Ci.sbIJobProgress,
                                         Ci.sbIShutdownJob])
};

//------------------------------------------------------------------------------
// Logging utility function

function LOG(aMessage)
{
  dump("----------------------------------------------------------\n");
  dump(" " + aMessage + "\n");
  dump("----------------------------------------------------------\n");
}

