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

  // make sure first-run has already run - gets set by first run wizard
  // after the EULA is accepted and the welcome screen is shown.
  var ranFirstRun = Application.prefs.get('songbird.firstrun.check.0.3');
  if (!ranFirstRun || !ranFirstRun.value) {
    return;
  }

  // make sure we haven't run
  var PREF_SONGBIRD_FIRSTRUN_UPDATEONCE = 'songbird.firstrun.update-once';
  var ranUpdate = Application.prefs.get(PREF_SONGBIRD_FIRSTRUN_UPDATEONCE);
  if (ranUpdate && ranUpdate.value) {
    return;
  }

  // request an update check
  var updateSvc = Components.classes['@mozilla.org/updates/update-service;1']
    .getService(Components.interfaces.nsIApplicationUpdateService);
  var updateChecker = updateSvc.backgroundChecker;
  // dummy update check listener - it doesn't do anything
  var updateCheckListener = {
    onCheckComplete: function (request, updates, updatecount) {
      var update = updateSvc.selectUpdate(updates, updatecount);
      if (!update)
        return;

      window.openDialog("chrome://mozapps/content/update/updates.xul",
                        "",
                        "chrome,centerscreen,dialog=no,resizable=no,titlebar,toolbar=no",
                        update);
    },
    onError: function (request, update) { },
    onProgress: function (request, position, totalSize) { }
  };
  updateChecker.checkForUpdates(updateCheckListener, true);

  // set a pref so we don't run again
  Application.prefs.setValue(PREF_SONGBIRD_FIRSTRUN_UPDATEONCE, true);
}

window.addEventListener('load', updateOnceAfterFirstRun, false);
