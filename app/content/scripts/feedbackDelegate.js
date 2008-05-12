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

/**
 * \brief This script manages and decides when the Songbird feedback
 *        dialog should be shown (based on pre-defined metrics).
 */

/**
 * Feedback pref constants:
 */
const PREF_APP_SESSIONS = "feedback.app_sessions";
const PREF_FIRST_OPENED_DATE = "feedback.first_opened_date";
const PREF_TOTAL_RUNTIME = "feedback.total_runtime";
const PREF_SURVEY_DATE = "feedback.survey_date";
const PREF_DISABLE_FEEDBACK = "feedback.disabled"

/**
 * Misc. constants:
 */
const LAST_FEEDBACK_DATE = "Mon, 30 Jun 2008 00:00:00 GMT";
const SURVEY_URL = "http://www.surveymonkey.com/s.aspx?sm=29bC_2bCt4FtbeKVqBU11jkA_3d_3d";
const MIN_APP_SESSIONS  = 3;
const MIN_TOTAL_RUNTIME = 30000;  // 30 minutes
const STARTUP_DELAY     = 5000;     // 5 seconds

/**
 * Global vars:
 */ 
var gObserverService = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
var gAppPrefs = Application.prefs;


/**
 * Feedback survey controller:
 */
function FeedbackDelegate()
{
  var self = this;
  setTimeout(function() { self._init(); }, STARTUP_DELAY);
}

FeedbackDelegate.prototype = 
{
  _mAppStartTimespec: 0,
  _mSavedTotalRuntime: 0,
  _mHasSessionRequirements: false,
  _mHasRuntimeRequirements: false,
  
  _init: function()
  {
    // If feedback has been disabled (i.e. debug build), just exit.
    if (gAppPrefs.get(PREF_DISABLE_FEEDBACK).value) {
      return;
    }
    
    var curDate = new Date();
    this._mAppStartTimespec = curDate.getTime();
    var hasDateRequirements = true;
    
    var lastSurveyDate = new Date(LAST_FEEDBACK_DATE);
    
    // If we are passed the last date to show the survey, or the feedback
    // dialog has been shown already - just return.
    if (curDate > lastSurveyDate || gAppPrefs.has(PREF_SURVEY_DATE)) {
      return;
    }
    
    // Check to see if we are not on the same day we first opened the app.
    if (gAppPrefs.has(PREF_FIRST_OPENED_DATE)) {
      var firstSessionTime = 
        parseInt(gAppPrefs.get(PREF_FIRST_OPENED_DATE).value);
      
      var firstSessionDate = new Date();
      firstSessionDate.setTime(firstSessionTime);
      
      if (curDate.getUTCDate() == firstSessionDate.getUTCDate() &&
          curDate.getUTCMonth() == firstSessionDate.getUTCMonth() &&
          curDate.getUTCFullYear() == firstSessionDate.getUTCFullYear())
      {
        hasDateRequirements = false;
      }
    }
    else {
      // First time application was launched, we need to save the current date.
      gAppPrefs.setValue(PREF_FIRST_OPENED_DATE, "" + curDate.getTime());
    }
    
    var numSessions = 
      parseInt(gAppPrefs.getValue(PREF_APP_SESSIONS, 0).toString());
    if (numSessions >= MIN_APP_SESSIONS) {
      this._mHasSessionRequirements = true;
    }
    
    this._mSavedTotalRuntime = 
      parseInt(gAppPrefs.getValue(PREF_TOTAL_RUNTIME, 0).toString());
    
    if (this._mSavedTotalRuntime >= MIN_TOTAL_RUNTIME) {
      this._mHasRuntimeRequirements = true;
    }
    
    if (this._mHasSessionRequirements && 
        this._mHasRuntimeRequirements) 
    {
      // Only show the survey if we met the date requirments. If not, we'll
      // have to wait until the next day the app is started.
      if (hasDateRequirements) {
        this._showSurvey();
      }
    }
    else {
      gObserverService.addObserver(this, "quit-application-granted", false);
    }
  },
  
  _showSurvey: function()
  {
    var retVal = { shouldLoadSurvey: false};
    
    var winMediator = 
      Components.classes["@mozilla.org/appshell/window-mediator;1"]
      .getService(Components.interfaces.nsIWindowMediator);
    
    var mainWin = winMediator.getMostRecentWindow("Songbird:Main");
    if (mainWin && mainWin.window && mainWin.window.gBrowser) {
      mainWin.openDialog("chrome://songbird/content/xul/feedbackDialog.xul",
                         "feedback-dialog",
                         "chrome,modal=yes,centerscreen,resizable=false", 
                         retVal);
      
      // Mark the survey date
      gAppPrefs.setValue(PREF_SURVEY_DATE, "" + new Date().getTime());
      
      if (retVal.shouldLoadSurvey) {
        mainWin.window.gBrowser.loadURI(SURVEY_URL, null, null, null, "_blank");
        mainWin.focus();
      }
    }
  },
  
  observe: function(aSubject, aTopic, aData)
  {
    if (aTopic != "quit-application-granted")
      return;
    
    // We are shutting down - it's now time to save the elapsed time to our
    // runtime variable and increment the session count. Note that we only
    // should update these prefs if the requirements have not been met when
    // the application started up.
    if (!this._mHasRuntimeRequirements) {
      var curTimespec = new Date().getTime();
      var elapsedTime = curTimespec - this._mAppStartTimespec;
      
      gAppPrefs.setValue(PREF_TOTAL_RUNTIME, 
                         "" + (this._mSavedTotalRuntime + elapsedTime));
    }
    
    if (!this._mHasSessionRequirements) {
      var curSessionCount = 
        parseInt(gAppPrefs.getValue(PREF_APP_SESSIONS, 0).toString());
      gAppPrefs.setValue(PREF_APP_SESSIONS, (curSessionCount + 1));
    }
    
    gObserverService.removeObserver(this, "quit-application-granted");
  },
  
  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.nsIObserver) &&
        !iid.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
};

var gFeedbackDelegate = new FeedbackDelegate();
