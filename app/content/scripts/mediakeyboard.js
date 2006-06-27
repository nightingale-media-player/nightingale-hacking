/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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

//
//  Media keyboard support
//

try 
{

  var SBMediaKeyboardCB = 
  {
    OnMute: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      if (SBDataGetIntValue("faceplate.volume") == 0 && !PPS.getMute())  // fake mute state ? (volume is 0, faceplate indicates mute but core mute flag is not on)
      {
        // get out of fake mute state
        SBDataSetValue("faceplate.volume", SBDataGetIntValue("faceplate.volume.last"));
        SBDataSetValue("faceplate.mute", false);
      } 
      else 
      {
        var newmute = !SBDataGetBoolValue("faceplate.mute"); 
        PPS.setMute(newmute);
      }
    },
    
    OnVolumeUp: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      var s = PPS.getVolume();
      var v = parseInt(s)+8;
      if (v > 255) v = 255;
      PPS.setVolume(v);
      SBDataSetValue("faceplate.volume.last", v);
    },

    OnVolumeDown: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      var s = PPS.getVolume();
      var v = parseInt(s)-8;
      if (v < 0) v = 0;
      PPS.setVolume(0);
      if (v != 0) SBDataSetValue("faceplate.volume.last", v);
    },

    OnNextTrack: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.next();
    },

    OnPreviousTrack: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.previous();
    },

    OnStop: function()
    {
      // yeah... no stop state... hmpf
      if ( SBDataGetIntValue("faceplate.seenplaying") )
      {
        var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
        PPS.pause();
      }
    },

    OnPlayPause: function()
    {
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.play(); // automatically selects the right behavior
    },

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIMediaKeyboardCallback) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
  }

  function setMediaKeyboardCallback()
  {
    try {
      var mediakeyboard = Components.classes["@songbirdnest.com/Songbird/MediaKeyboard;1"];
      if (mediakeyboard) {
        var service = mediakeyboard.getService(Components.interfaces.sbIMediaKeyboard);
        if (service)
          service.AddCallback(SBMediaKeyboardCB);
      }
    }
    catch (err) {
      // No component
      dump("Error. mediakeyboard.js:setMediaKeyboardCallback() \n" + err + "\n");
    }
  }

  function resetMediaKeyboardCallback()
  {
    try {
      var mediakeyboard = Components.classes["@songbirdnest.com/Songbird/MediaKeyboard;1"];
      if (mediakeyboard) {
        var service = mediakeyboard.getService(Components.interfaces.sbIMediaKeyboard);
        if (service)
          service.RemoveCallback(SBMediaKeyboardCB);
      }
    }
    catch (err) {
      // No component
      dump("Error. mediakeyboard.js:resetMediaKeyboardCallback() \n" + err + "\n");
    }
  }
}
catch (e)
{
  alert("mediakeyboard.js - " + e);
}

