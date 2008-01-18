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


// The first run after first-run we want to do a special update check.

function updateOnceAfterFirstRun() {
  // only once
  window.removeEventListener('load', updateOnceAfterFirstRun, false);

  // make sure first-run has already run
  var ran_first_run = Application.prefs.get('songbird.firstrun.check.0.3');
  if (!ran_first_run || !ran_first_run.value) {
    return;
  }

  // make sure we haven't run
  var PREF_SONGBIRD_FIRSTRUN_UPDATEONCE = 'songbird.firstrun.update-once';
  var ran_uoafr = Application.prefs.get(PREF_SONGBIRD_FIRSTRUN_UPDATEONCE);
  if (ran_uoafr && ran_uoafr.value) {
    return;
  }

  // set the update url override
  var PREF_APP_UPDATE_URL = 'app.update.url';
  var PREF_APP_UPDATE_URL_OVERRIDE = 'app.update.url.override';
  // add "?firstrun=1" to the end of the update url temporarilly
  var app_update_url = Application.prefs.get(PREF_APP_UPDATE_URL);
  Application.prefs.setValue(PREF_APP_UPDATE_URL_OVERRIDE, 
      app_update_url.value + "?firstrun=1");
  
  // request an update check
  var updateSvc = Components.classes['@mozilla.org/updates/update-service;1']
    .getService(Components.interfaces.nsIApplicationUpdateService);
  var updateChecker = updateSvc.backgroundChecker;
  // dummy update check listener - it doesn't do anything
  var updateCheckListener = {
    onCheckComplete: function (request, updates, updatecount) { },
    onError: function (request, update) { },
    onProgress: function (request, position, totalSize) { }
  };
  updateChecker.checkForUpdates(updateCheckListener, true);

  // reset the override url
  Application.prefs.get(PREF_APP_UPDATE_URL_OVERRIDE).reset();

  // set a pref so we don't run again
  Application.prefs.setValue(PREF_SONGBIRD_FIRSTRUN_UPDATEONCE, true);
}

window.addEventListener('load', updateOnceAfterFirstRun, false);
