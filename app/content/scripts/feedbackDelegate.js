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

/**
 * \brief This script manages and decides when the Nightingale feedback
 *        dialog should be shown (based on pre-defined metrics).
 */

Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

/**
 * Feedback pref constants:
 */
const PREF_APP_SESSIONS = "feedback.app_sessions";
const PREF_FIRST_OPENED_DATE = "feedback.first_opened_date";
const PREF_TOTAL_RUNTIME = "feedback.total_runtime";
const PREF_SURVEY_DATE = "feedback.survey_date";
const PREF_DISABLE_FEEDBACK = "feedback.disabled";
const PREF_DENIED_FEEDBACK = "feedback.denied";
const PREF_NEXT_FEEDBACK_MONTH_LAG = "feedback.next_feedback_month_lag";
const PREF_MIN_APP_SESSIONS = "feedback.min_app_sessions";
const PREF_MIN_TOTAL_RUNTIME = "feedback.min_total_runtime";

/**
 * Misc. constants:
 */
const SURVEY_URL_KEY = "feedback.survey.url";
const MIN_APP_SESSIONS  = 3;
const MIN_TOTAL_RUNTIME = 1800000;  // 30 minutes
const STARTUP_DELAY     = 5000;   // 5 seconds
const NEXT_FEEDBACK_MONTH_LAG = 6;

/**
 * Global vars:
 */
var gObserverService = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
var gAppPrefs = Application.prefs;

function getIntPrefValue(name, value)
{
  return parseInt(gAppPrefs.getValue(name, value || 0).toString(), 10);
}

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
    var disablePrefs = gAppPrefs.get(PREF_DISABLE_FEEDBACK);
    if (disablePrefs != null && disablePrefs.value) {
      return;
    }

    // Don't show the feedback dialog again if the user turned down feedback.
    if (gAppPrefs.has(PREF_DENIED_FEEDBACK)) {
      return;
    }

    // Don't show the feedback dialog if we don't know where to send the survey
    if ("false" == SBString(SURVEY_URL_KEY, "false", SBStringGetBrandBundle())) {
      return;
    }

    var curDate = new Date();
    this._mAppStartTimespec = curDate.getTime();
    var hasDateRequirements = true;

    // If the user has already taken the survey, only procede showing it again
    // after six months.
    if (gAppPrefs.has(PREF_SURVEY_DATE)) {
      var surveyTakenTime =
        parseInt(gAppPrefs.get(PREF_SURVEY_DATE).value);

      var surveyTakenDate = new Date();
      surveyTakenDate.setTime(surveyTakenTime);

      var futureDate = new Date();
      futureDate = new Date();
      futureDate.setTime(surveyTakenTime);

      // JS Date range is 0-11
      var futureMonthLag = surveyTakenDate.getUTCMonth() +
                           getPrefValue(PREF_NEXT_FEEDBACK_MONTH_LAG,
                                        NEXT_FEEDBACK_MONTH_LAG);
      if (futureMonthLag >= 12) {
        futureMonthLag -= 12;
        // Since this is roll over, bump the year
        futureDate.setUTCFullYear(futureDate.getUTCFullYear() + 1);
      }
      futureDate.setUTCMonth(futureMonthLag);

      // If the timeframe has not passed the 6 month mark, bail.
      if (curDate.getTime() <= futureDate.getTime()) {
        return;
      }
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

    var numSessions = getIntPrefValue(PREF_APP_SESSIONS);
    if (numSessions >= getIntPrefValue(PREF_MIN_APP_SESSIONS, MIN_APP_SESSIONS))
    {
      this._mHasSessionRequirements = true;
    }

    this._mSavedTotalRuntime = getIntPrefValue(PREF_TOTAL_RUNTIME);
    if (this._mSavedTotalRuntime >= getIntPrefValue(PREF_MIN_TOTAL_RUNTIME,
                                                    MIN_TOTAL_RUNTIME))
    {
      this._mHasRuntimeRequirements = true;
    }

    if (this._mHasSessionRequirements &&
        this._mHasRuntimeRequirements)
    {
      // Only show the survey if we met the date requirments. If not, we'll
      // have to wait until the next day the app is started.
      //
      // To work around a scenario where the only window in th window registry
      // is the hidden window, wait 10 seconds to show the dialog.
      // @see bug 9887
      if (hasDateRequirements) {
        var self = this;
        setTimeout(function() { self._showSurvey(); }, 10000);
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

    var mainWin = winMediator.getMostRecentWindow("Nightingale:Main");
    if (mainWin && mainWin.window && mainWin.window.gBrowser) {
      mainWin.openDialog("chrome://nightingale/content/xul/feedbackDialog.xul",
                         "feedback-dialog",
                         "chrome,modal=yes,centerscreen,resizable=false",
                         retVal);

      // Mark the survey date
      gAppPrefs.setValue(PREF_SURVEY_DATE, "" + new Date().getTime());

      if (retVal.shouldLoadSurvey) {
        var surveyURL = SBString(SURVEY_URL_KEY, "false", SBStringGetBrandBundle());
        mainWin.window.gBrowser.loadURI(surveyURL, null, null, null, "_blank");
        mainWin.focus();
      }
      else {
        gAppPrefs.setValue(PREF_DENIED_FEEDBACK, true);

        // Flush to disk to make sure the dialog won't be shown again in the
        // event of a crash. See bug 11857.
        var prefService =
          Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefService);
        if (prefService) {
          // Passing null uses the currently loaded pref file.
          // (usually 'prefs.js')
          prefService.savePrefFile(null);
        }
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
      var curSessionCount = getIntPrefValue(PREF_APP_SESSIONS);
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

// initialize a new feedback delegate (no need for a refernce to it)
new FeedbackDelegate();
