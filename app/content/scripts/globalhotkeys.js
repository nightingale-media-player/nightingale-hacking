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

const DOM_VK_CANCEL         = 0x03;
const DOM_VK_HELP           = 0x06;
const DOM_VK_BACK_SPACE     = 0x08;
const DOM_VK_TAB            = 0x09;
const DOM_VK_CLEAR          = 0x0C;
const DOM_VK_RETURN         = 0x0D;
const DOM_VK_ENTER          = 0x0E;
const DOM_VK_SHIFT          = 0x10;
const DOM_VK_CONTROL        = 0x11;
const DOM_VK_ALT            = 0x12;
const DOM_VK_PAUSE          = 0x13;
const DOM_VK_CAPS_LOCK      = 0x14;
const DOM_VK_ESCAPE         = 0x1B;
const DOM_VK_SPACE          = 0x20;
const DOM_VK_PAGE_UP        = 0x21;
const DOM_VK_PAGE_DOWN      = 0x22;
const DOM_VK_END            = 0x23;
const DOM_VK_HOME           = 0x24;
const DOM_VK_LEFT           = 0x25;
const DOM_VK_UP             = 0x26;
const DOM_VK_RIGHT          = 0x27;
const DOM_VK_DOWN           = 0x28;
const DOM_VK_PRINTSCREEN    = 0x2C;
const DOM_VK_INSERT         = 0x2D;
const DOM_VK_DELETE         = 0x2E;

// DOM_VK_0 - DOM_VK_9 match their ascii values
const DOM_VK_0              = 0x30;
const DOM_VK_1              = 0x31;
const DOM_VK_2              = 0x32;
const DOM_VK_3              = 0x33;
const DOM_VK_4              = 0x34;
const DOM_VK_5              = 0x35;
const DOM_VK_6              = 0x36;
const DOM_VK_7              = 0x37;
const DOM_VK_8              = 0x38;
const DOM_VK_9              = 0x39;

const DOM_VK_SEMICOLON      = 0x3B;
const DOM_VK_EQUALS         = 0x3D;

// DOM_VK_A - DOM_VK_Z match their ascii values
const DOM_VK_A              = 0x41;
const DOM_VK_B              = 0x42;
const DOM_VK_C              = 0x43;
const DOM_VK_D              = 0x44;
const DOM_VK_E              = 0x45;
const DOM_VK_F              = 0x46;
const DOM_VK_G              = 0x47;
const DOM_VK_H              = 0x48;
const DOM_VK_I              = 0x49;
const DOM_VK_J              = 0x4A;
const DOM_VK_K              = 0x4B;
const DOM_VK_L              = 0x4C;
const DOM_VK_M              = 0x4D;
const DOM_VK_N              = 0x4E;
const DOM_VK_O              = 0x4F;
const DOM_VK_P              = 0x50;
const DOM_VK_Q              = 0x51;
const DOM_VK_R              = 0x52;
const DOM_VK_S              = 0x53;
const DOM_VK_T              = 0x54;
const DOM_VK_U              = 0x55;
const DOM_VK_V              = 0x56;
const DOM_VK_W              = 0x57;
const DOM_VK_X              = 0x58;
const DOM_VK_Y              = 0x59;
const DOM_VK_Z              = 0x5A;

const DOM_VK_CONTEXT_MENU   = 0x5D;

const DOM_VK_NUMPAD0        = 0x60;
const DOM_VK_NUMPAD1        = 0x61;
const DOM_VK_NUMPAD2        = 0x62;
const DOM_VK_NUMPAD3        = 0x63;
const DOM_VK_NUMPAD4        = 0x64;
const DOM_VK_NUMPAD5        = 0x65;
const DOM_VK_NUMPAD6        = 0x66;
const DOM_VK_NUMPAD7        = 0x67;
const DOM_VK_NUMPAD8        = 0x68;
const DOM_VK_NUMPAD9        = 0x69;
const DOM_VK_MULTIPLY       = 0x6A;
const DOM_VK_ADD            = 0x6B;
const DOM_VK_SEPARATOR      = 0x6C;
const DOM_VK_SUBTRACT       = 0x6D;
const DOM_VK_DECIMAL        = 0x6E;
const DOM_VK_DIVIDE         = 0x6F;
const DOM_VK_F1             = 0x70;
const DOM_VK_F2             = 0x71;
const DOM_VK_F3             = 0x72;
const DOM_VK_F4             = 0x73;
const DOM_VK_F5             = 0x74;
const DOM_VK_F6             = 0x75;
const DOM_VK_F7             = 0x76;
const DOM_VK_F8             = 0x77;
const DOM_VK_F9             = 0x78;
const DOM_VK_F10            = 0x79;
const DOM_VK_F11            = 0x7A;
const DOM_VK_F12            = 0x7B;
const DOM_VK_F13            = 0x7C;
const DOM_VK_F14            = 0x7D;
const DOM_VK_F15            = 0x7E;
const DOM_VK_F16            = 0x7F;
const DOM_VK_F17            = 0x80;
const DOM_VK_F18            = 0x81;
const DOM_VK_F19            = 0x82;
const DOM_VK_F20            = 0x83;
const DOM_VK_F21            = 0x84;
const DOM_VK_F22            = 0x85;
const DOM_VK_F23            = 0x86;
const DOM_VK_F24            = 0x87;

const DOM_VK_NUM_LOCK       = 0x90;
const DOM_VK_SCROLL_LOCK    = 0x91;

const DOM_VK_COMMA          = 0xBC;
const DOM_VK_PERIOD         = 0xBE;
const DOM_VK_SLASH          = 0xBF;
const DOM_VK_BACK_QUOTE     = 0xC0;
const DOM_VK_OPEN_BRACKET   = 0xDB;
const DOM_VK_BACK_SLASH     = 0xDC;
const DOM_VK_CLOSE_BRACKET  = 0xDD;
const DOM_VK_QUOTE          = 0xDE;

const DOM_VK_META           = 0xE0;

var   hotkey_service;

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

function stringToKeyCode( str ) {
  switch (str.toLowerCase()) {
    case "cancel": return DOM_VK_CANCEL;
    case "help": return DOM_VK_HELP;
    case "backspace": return DOM_VK_BACK_SPACE;
    case "tab": return DOM_VK_TAB;
    case "clear": return DOM_VK_CLEAR;
    case "return": return DOM_VK_RETURN;
    case "enter": return DOM_VK_ENTER;
    case "shift": return DOM_VK_SHIFT;
    case "control": return DOM_VK_CONTROL;
    case "alt": return DOM_VK_ALT;
    case "pause": return DOM_VK_PAUSE;
    case "capslock": return DOM_VK_CAPS_LOCK;
    case "escape": return DOM_VK_ESCAPE;
    case "space": return DOM_VK_SPACE;
    case "pageup": return DOM_VK_PAGE_UP;
    case "pagedown": return DOM_VK_PAGE_DOWN;
    case "end": return DOM_VK_END;
    case "home": return DOM_VK_HOME;
    case "left": return DOM_VK_LEFT;
    case "up": return DOM_VK_UP;
    case "right": return DOM_VK_RIGHT;
    case "down": return DOM_VK_DOWN;
    case "printscreen": return DOM_VK_PRINTSCREEN;
    case "insert": return DOM_VK_INSERT;
    case "delete": return DOM_VK_DELETE;
    case "0": return DOM_VK_0;
    case "1": return DOM_VK_1;
    case "2": return DOM_VK_2;
    case "3": return DOM_VK_3;
    case "4": return DOM_VK_4;
    case "5": return DOM_VK_5;
    case "6": return DOM_VK_6;
    case "7": return DOM_VK_7;
    case "8": return DOM_VK_8;
    case "9": return DOM_VK_9;
    case "semicolon": return DOM_VK_SEMICOLON;
    case "equals": return DOM_VK_EQUALS;
    case "a": return DOM_VK_A;
    case "b": return DOM_VK_B;
    case "c": return DOM_VK_C;
    case "d": return DOM_VK_D;
    case "e": return DOM_VK_E;
    case "f": return DOM_VK_F;
    case "g": return DOM_VK_G;
    case "h": return DOM_VK_H;
    case "i": return DOM_VK_I;
    case "j": return DOM_VK_J;
    case "k": return DOM_VK_K;
    case "l": return DOM_VK_L;
    case "m": return DOM_VK_M;
    case "n": return DOM_VK_N;
    case "o": return DOM_VK_O;
    case "p": return DOM_VK_P;
    case "q": return DOM_VK_Q;
    case "r": return DOM_VK_R;
    case "s": return DOM_VK_S;
    case "t": return DOM_VK_T;
    case "u": return DOM_VK_U;
    case "v": return DOM_VK_V;
    case "w": return DOM_VK_W;
    case "x": return DOM_VK_X;
    case "y": return DOM_VK_Y;
    case "z": return DOM_VK_Z;
    case "contextmenu": return DOM_VK_CONTEXT_MENU;
    case "numpad0": return DOM_VK_NUMPAD0;
    case "numpad1": return DOM_VK_NUMPAD1;
    case "numpad2": return DOM_VK_NUMPAD2;
    case "numpad3": return DOM_VK_NUMPAD3;
    case "numpad4": return DOM_VK_NUMPAD4;
    case "numpad5": return DOM_VK_NUMPAD5;
    case "numpad6": return DOM_VK_NUMPAD6;
    case "numpad7": return DOM_VK_NUMPAD7;
    case "numpad8": return DOM_VK_NUMPAD8;
    case "numpad9": return DOM_VK_NUMPAD9;
    case "multiply": return DOM_VK_MULTIPLY;
    case "add": return DOM_VK_ADD;
    case "separator": return DOM_VK_SEPARATOR;
    case "subtract": return DOM_VK_SUBTRACT;
    case "decimal": return DOM_VK_DECIMAL;
    case "divide": return DOM_VK_DIVIDE;
    case "f1": return DOM_VK_F1;
    case "f2": return DOM_VK_F2;
    case "f3": return DOM_VK_F3;
    case "f4": return DOM_VK_F4;
    case "f5": return DOM_VK_F5;
    case "f6": return DOM_VK_F6;
    case "f7": return DOM_VK_F7;
    case "f8": return DOM_VK_F8;
    case "f9": return DOM_VK_F9;
    case "f10": return DOM_VK_F10;
    case "f11": return DOM_VK_F11;
    case "f12": return DOM_VK_F12;
    case "f13": return DOM_VK_F13;
    case "f14": return DOM_VK_F14;
    case "f15": return DOM_VK_F15;
    case "f16": return DOM_VK_F16;
    case "f17": return DOM_VK_F17;
    case "f18": return DOM_VK_F18;
    case "f19": return DOM_VK_F19;
    case "f20": return DOM_VK_F20;
    case "f21": return DOM_VK_F21;
    case "f22": return DOM_VK_F22;
    case "f23": return DOM_VK_F23;
    case "f24": return DOM_VK_F24;
    case "numlock": return DOM_VK_NUM_LOCK;
    case "scrolllock": return DOM_VK_SCROLL_LOCK;
    case "comma": return DOM_VK_COMMA;
    case "period": return DOM_VK_PERIOD;
    case "slash": return DOM_VK_SLASH;
    case "backquote": return DOM_VK_BACK_QUOTE;
    case "openbracket": return DOM_VK_OPEN_BRACKET;
    case "backslash": return DOM_VK_BACK_SLASH;
    case "closebracket": return DOM_VK_CLOSE_BRACKET;
    case "quote": return DOM_VK_QUOTE;
    case "meta": return DOM_VK_META;
  }
  return 0;
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

function hotkey_pause() {
}

