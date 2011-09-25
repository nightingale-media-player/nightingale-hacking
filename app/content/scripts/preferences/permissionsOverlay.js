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

var gNightingalePermissionsOverlay = {
  prompt: null,
  blocking: null,
  prefs: null,
  onLoad: function(event) {
    window.removeEventListener('load', gNightingalePermissionsOverlay.onLoad, false);
    
    if (!window.arguments || !window.arguments[0] ||
        !window.arguments[0].blocking) {
      // a blocking object wasn't passed in. nothing to do
      return;
    }
    gNightingalePermissionsOverlay.blocking = window.arguments[0].blocking;
    gNightingalePermissionsOverlay.remoteAPIPane = window.arguments[0].remoteAPIPane;

    var permissionsText = document.getElementById('permissionsText');
    if (!permissionsText) {
      return;
    }
    
    gNightingalePermissionsOverlay.prefs =
      Components.classes['@mozilla.org/preferences-service;1']
      .getService(Components.interfaces.nsIPrefBranch);
    
    var settings = document.createElement('description');
    settings.appendChild(document.createTextNode(gNightingalePermissionsOverlay.blocking.settings));
    permissionsText.parentNode.insertBefore(settings, permissionsText);    
    
    gNightingalePermissionsOverlay.prompt = document.createElement('checkbox');
    gNightingalePermissionsOverlay.prompt.id = 'blockChkbx';
    permissionsText.parentNode.insertBefore(gNightingalePermissionsOverlay.prompt,
      permissionsText);
    gNightingalePermissionsOverlay.prompt.label = gNightingalePermissionsOverlay.blocking.prompt;
    try {
      gNightingalePermissionsOverlay.prompt.checked =
        gNightingalePermissionsOverlay.prefs.getBoolPref(
          gNightingalePermissionsOverlay.blocking.pref);
    } catch (e) {
      // don't fail on a missing pref
    }
    gNightingalePermissionsOverlay.originalBlocked = 
      gNightingalePermissionsOverlay.prompt.checked ? true : false;
    gNightingalePermissionsOverlay.prompt.addEventListener('command',
      gNightingalePermissionsOverlay.onCheckboxCommand, false);

    window.addEventListener('unload', gNightingalePermissionsOverlay.onClose, false);
    window.addEventListener('command', gNightingalePermissionsOverlay.onCommand, false);
  },
  onCheckboxCommand: function(event) {
    gNightingalePermissionsOverlay.prefs.setBoolPref(
        gNightingalePermissionsOverlay.blocking.pref,
        gNightingalePermissionsOverlay.prompt.checked);
  },
  onCommand: function(event) {
    if ( event.target.id ) {
      // the close button doesn't have an id - very fragile!
      gNightingalePermissionsOverlay.remoteAPIPane.isChanged = true;
    }
  },
  onClose: function(event) {
    window.removeEventListener('command', gNightingalePermissionsOverlay.onCommand, false);
    gNightingalePermissionsOverlay.prompt.removeEventListener('command',
      gNightingalePermissionsOverlay.onCheckboxCommand, false);
    window.removeEventListener('unload',
                               gNightingalePermissionsOverlay.onClose,
                               false);
  }
};
window.addEventListener('load', gNightingalePermissionsOverlay.onLoad, false);


