/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

// Global Hotkeys

var hotkey_service;

function initGlobalHotkeys() {
  var globalHotkeys = Components.classes["@songbird.org/Songbird/GlobalHotkeys;1"];
  if (globalHotkeys) {
    hotkey_service = globalHotkeys.getService(Components.interfaces.sbIGlobalHotkeys);
    if (hotkey_service) loadHotkeysFromPrefs()
  }
}

function resetGlobalHotkeys() {
  if (hotkey_service) hotkey_service.RemoveAllHotkeys();
  
}

var hotkeyHandler = 
{
    OnHotkey: function( hotkeyid )
    {
      switch ( hotkeyid )
      {
        case "Playback: Volume up": hotkey_volumeUp(); break;
        case "Playback: Volume down": hotkey_volumeDown(); break;
        case "Playback: Next in playlist": hotkey_nextTrack(); break;
        case "Playback: Previous in playlist": hotkey_previousTrack(); break;
        case "Playback: Play/Pause": hotkey_playPause(); break;
        case "Playback: Pause": hotkey_pause(); break;
        default: alert("Unknown hotkey action '" + hotkeyid + "'"); break;
      }
    },

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIGlobalHotkeyCallback) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
}

function loadHotkeysFromPrefs() {
  var count = SBDataGetValue("globalhotkey.count");
  if (count == 0) count = setDefaultGlobalHotkeys();
  for (var i=0;i<count;i++) {
    // todo:
    // - read pref data string for 'globalhotkey.n.key' and 'globalhotkey.n.action'
    // - parse out key code and modifier flags
    // - register hotkey (hotkey_service.AddHotkey(DOM_VK_*, altflag, ctrlflag, shiftflag, metaflag, action_string, hotkeyHandler);
  }
  //hotkey_service.AddHotkey(0x71, 0, 0, 0, 1, "hello", hotkeyHandler);
}

function setDefaultGlobalHotkeys() {
  SBDataSetValue("globalhotkey.count", 5);
  SBDataSetValue("globalhotkey.0.key",    "meta-up");
  SBDataSetValue("globalhotkey.0.action", "Playback: Volume up");
  SBDataSetValue("globalhotkey.1.key",    "meta-down");
  SBDataSetValue("globalhotkey.1.action", "Playback: Volume down");
  SBDataSetValue("globalhotkey.2.key",    "meta-right");
  SBDataSetValue("globalhotkey.2.action", "Playback: Next in playlist");
  SBDataSetValue("globalhotkey.3.key",    "meta-left");
  SBDataSetValue("globalhotkey.3.action", "Playback: Previous in playlist");
  SBDataSetValue("globalhotkey.4.key",    "meta-numpad0");
  SBDataSetValue("globalhotkey.4.action", "Playback: Play/Pause");
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Hotkey Actions
// -------------------------------------------------------------------------------------------------------------------------------------------------------------

// Playback

// todo

function hotkey_volumeUp() {
}

function hotkey_volumeDown() {
}

function hotkey_nextTrack() {
}

function hotkey_previousTrack() {
}

function hotkey_playPause() {
}

hotkey_pause() {
}

