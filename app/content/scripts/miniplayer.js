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
// Remote Initialization
//

try
{

  // Hooray for event handlers!
  function myPlaybackEvent( key, value )
  {
  }

  // NOW we have the playlist playback service!
  var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                       .getService(Components.interfaces.sbIPlaylistPlayback);

  //
  // Core Wrapper Initialization (in XUL, this must happen after the entire page loads). 
  //

  function SBInitialize()
  {
/*  
    // Watch the metadata if the core is playing.
    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 )
        SBInitPlayerLoop(); // Don't loop if we're initializing into the video window.
*/
    
    // initialize player controls for this faceplate  
    window.focus();
    SBInitMouseWheel();

    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 ) {
      initJumpToFileHotkey();
      setMinMaxCallback();
    } else {
      document.getElementById("mini_close").hidden = true;
      document.getElementById("mini_btn_max").setAttribute("oncommand", "SBFullscreen();");
      document.getElementById("frame_mini").setAttribute("style", "-moz-border-radius: 0px !important; border-color: transparent !important;"); // Square the frame and remove the border.
    }
    
    if ( PLATFORM_MACOSX ) {
      document.getElementById("frame_mini").setAttribute("style", "-moz-border-radius: 0px !important; border-color: transparent !important;"); // Square the frame and remove the border.
    }
    
    window.addEventListener( "keydown", checkAltF4, true );
    
    onWindowLoadSizeAndPosition();
  }
  
  function SBUninitialize()
  {
    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 ) {
      resetJumpToFileHotkey();
      closeJumpTo();
      try {
        var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
        if (windowMinMax) {
          var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
          if (service)
            service.resetCallback(document);
        }
      }
      catch(err) {
        dump("Error. miniplayer.js: SBUnitialize() \n" + err + "\n");
      }
    }
  }
   
  //
  // XUL Event Methods
  //
  
  function SBFullscreen() 
  {
    gPPS.goFullscreen();
  }


  function checkAltF4(evt)
  {
    if (evt.keyCode == 0x73 && evt.altKey) 
    {
      evt.preventDefault();
      quitApp();
    }
  }

  function onExit()
  {
    try
    {
      var location = "" + window.location; // Grrr.  Dumb objects.
      if ( location.indexOf("?video") == -1 )
      {
        onWindowSaveSizeAndPosition();
      }
    }
    catch ( err )
    {
      // If you don't have data functions, just close.
      alert(err);
    }
    document.defaultView.close();
  }

  function quitApp()
  {
    var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
    var MetricsService = new nsIMetrics();
    MetricsService.setSessionFlag(false); // mark this session as clean, we did not crash
    var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
    if (as)
    {
      const V_ATTEMPT = 2;
      as.quit(V_ATTEMPT);
    }
    onExit();
  }

  // Help
  function onHelp()
  {
    alert( "Aieeeeee, ayudame!" );
  }

  // Menubar handling
  function onMenu( target )
  {
    var v = target.getAttribute( "id" );
    switch ( v )
    {
      case "file.file":
        SBFileOpen();
      break;
      case "file.url":
        SBUrlOpen();
      break;
      case "file.mab":
        SBMabOpen();
      break;
      case "file.playlist":
        SBNewPlaylist();
      break;
      case "file.window":
        SBMainWindowOpen();
      break;
      case "file.about":
        About(); 
      break;
      case "file.exit":
        quitApp();
        document.defaultView.close();
      break;
    }
  }

  function SBMabOpen()
  {
    var mab_data = new Object();
    mab_data.retval = "";
    
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/mab.xul", "Mozilla Amazon Browser", "chrome,modal=no", mab_data );
  }

  function SBNewPlaylist()
  {
    var playlist_data = new Object();
    playlist_data.retval = "";
    
    // Open the window
    window.open( "chrome://songbird/content/xul/playlist_test.xul", "Testing Playlist Datafeed", "chrome,modal=no", playlist_data );
  }

  function SBMainWindowOpen()
  {
    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 )
    {
      // Get mainwin URL
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
      var mainwin = "chrome://rubberducky/content/xul/mainwin.xul";
      try {
        mainwin = prefs.getCharPref("general.bones.selectedMainWinURL", mainwin);  
      } catch (err) {}
     
      // Open the window
      window.open( mainwin, "", "chrome,modal=no,titlebar=no,resizable=no,toolbar=no,popup=no,titlebar=no" );
      onExit();
    }
  }

  function About( )
  {
    // Make a magic data object to get passed to the dialog
    var about_data = new Object();
    about_data.retval = "";
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/about.xul", "about", "chrome,modal=yes,centerscreen", about_data );
    if ( about_data.retval == "ok" )
    {
    }  
  }

  function onMiniplayerKeypress( evt )
  {
    switch ( evt.keyCode )
    {
      case 37: // Arrow Left
        onBack( );
        break;
      case 39: // Arrow Right
        onFwd( );
        break;
      case 40: // Arrow Down
      case 13: // Return
        if ( gPPS.playing )
          onPause( );
        else
          onPlay( );
        break;
    }
    switch ( evt.charCode )
    {
      case 109: // Ctrl-Alt-M
        if ( evt.altKey && evt.ctrlKey )
        {
          SBMainWindowOpen( );
        }
        break;
      case 32: // Space
        if ( gPPS.playing )
          onPause( );
        else
          onPlay( );
        break;
    }
  }

  function onWindowSaveSizeAndPosition()
  {
    var root = "window." + document.documentElement.id;
    SBDataSetIntValue( root + ".x", document.documentElement.boxObject.screenX );
    SBDataSetIntValue( root + ".y", document.documentElement.boxObject.screenY );
    SBDataSetIntValue( root + ".w", document.documentElement.boxObject.width );
    SBDataSetIntValue( root + ".h", document.documentElement.boxObject.height );
  }

  function onWindowLoadSizeAndPosition()
  {
    var root = "window." + document.documentElement.id;
    if (SBDataGetStringValue( root + ".x" ) == "" && SBDataGetStringValue( root + ".w" ) == "") { return; }
    if ( SBDataGetIntValue( root + ".w" ) && SBDataGetIntValue( root + ".h" ) )
    {
      // https://bugzilla.mozilla.org/show_bug.cgi?id=322788
      // YAY YAY YAY the windowregion hack actualy fixes this :D
      window.resizeTo( SBDataGetIntValue( root + ".w" ), SBDataGetIntValue( root + ".h" ) );
      // for some reason, the resulting size isn't what we're asking (window currently has a border?) so determine what the difference is and add it to the resize
      var diffw = SBDataGetIntValue( root + ".w" ) - document.documentElement.boxObject.width;
      var diffh = SBDataGetIntValue( root + ".h" ) - document.documentElement.boxObject.height;
      window.resizeTo( SBDataGetIntValue( root + ".w" ) + diffw, SBDataGetIntValue( root + ".h" ) + diffh);
    }
    window.moveTo( SBDataGetIntValue( root + ".x" ), SBDataGetIntValue( root + ".y" ) );
    // do the (more or less) same adjustment for x,y as we did for w,h
    var diffx = SBDataGetIntValue( root + ".x" ) - document.documentElement.boxObject.screenX;
    var diffy = SBDataGetIntValue( root + ".y" ) - document.documentElement.boxObject.screenY;
    window.moveTo( SBDataGetIntValue( root + ".x" ) - diffx, SBDataGetIntValue( root + ".y" ) - diffy );
  }

  var SBWindowMinMaxCB = 
  {
    // Shrink until the box doesn't match the window, then stop.
    _minwidth: -1,
    GetMinWidth: function()
    {
      // If min size is not yet known and if the window size is different from the document's box object, 
      if (this._minwidth == -1 && window.innerWidth != document.getElementById('frame_mini').boxObject.width)
      { 
        // Then we know we've hit the minimum width, record it. Because you can't query it directly.
        this._minwidth = document.getElementById('frame_mini').boxObject.width + 1;
      }
      return this._minwidth;
    },

    GetMinHeight: function()
    {
      return -1;
    },

    GetMaxWidth: function()
    {
      return -1;
    },

    GetMaxHeight: function()
    {
      return -1;
    },

    OnWindowClose: function()
    {
      setTimeout(quitApp, 0);
    },

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIWindowMinMaxCallback) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
  }

  function setMinMaxCallback()
  {
    try {
      var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
      if (windowMinMax) {
        var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
        if (service)
          service.setCallback(document, SBWindowMinMaxCB);
      }
    }
    catch (err) {
      // No component
      dump("Error. songbird_hack.js:setMinMaxCallback() \n " + err + "\n");
    }
  }

}
catch ( err )
{
  alert( err );
}

