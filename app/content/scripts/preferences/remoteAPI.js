// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

var gRemoteAPIPane = {
  // the prefs for the checkboxes in the pane
  // these are both the XUL DOM id and pref key
  _prefKeys: [
    'songbird.rapi.playback_control_disable',
    'songbird.rapi.playback_read_disable',
    'songbird.rapi.library_read_disable',
    'songbird.rapi.library_write_disable',
  ],
  isChanged: false,

  configureWhitelist: function (aType)
  {
    // get ref to the properties file string bundle
    var bundlePreferences = document.getElementById("bundlePreferences");

    // set up a parmater object to pass to permission window
    var params = {
      blockVisible: false,
      sessionVisible: false,
      allowVisible: true,
      prefilledHost: "",
      permissionType: "rapi." + aType,
      windowTitle: bundlePreferences.getString("rapi.permissions_title"),
      introText: bundlePreferences.getString("rapi." + aType + ".permissions_text"),
      blocking: {
        settings: bundlePreferences.getString("rapi." + aType + ".block_settings"),
        prompt: bundlePreferences.getString("rapi.block_prompt"),
        pref: "songbird.rapi." + aType + "_notify"
      },
      remoteAPIPane: gRemoteAPIPane
    };
    
    // open the permission window to set allow/disallow/session permissions
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "",
                                        params);
  },

  updateDisabledState: function() {
    for (var i=0; i<this._prefKeys.length; i++) {
      var pref_element = document.getElementById(this._prefKeys[i]);
      if (!pref_element) {
        continue;
      }
      var button_element = document.getElementById(this._prefKeys[i]+'.button');
      if (!button_element) {
        continue;
      }
      if (!pref_element.value) {
        button_element.setAttribute('disabled', 'true');
      } else {
        button_element.removeAttribute('disabled');
      }

    }
  },
  
  onLoad: function(event) {
    window.removeEventListener('load', gRemoteAPIPane.onLoad, false);
    gRemoteAPIPane.updateDisabledState();
  },
  
  onUnload: function(event) {
    if (gRemoteAPIPane.isChanged && window.opener && window.opener.document) {
      var evt = document.createEvent("Events");
      evt.initEvent("RemoteAPIPermissionChanged", true, false);
      window.opener.document.dispatchEvent(evt);
    }
  },
  
  onChange: function(event) {
    gRemoteAPIPane.updateDisabledState();
    gRemoteAPIPane.isChanged = true;
  },
  
  restoreDefaults: function() {
    gRemoteAPIPane.isChanged = true;
    for (var i=0; i<this._prefKeys.length; i++) {
      var pref_element = document.getElementById(this._prefKeys[i]);
      if (!pref_element) {
        continue;
      }
      pref_element.reset();
    }
  }

};

window.addEventListener('load', gRemoteAPIPane.onLoad, false);
window.addEventListener('unload', gRemoteAPIPane.onUnload, false);

