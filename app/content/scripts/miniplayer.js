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
  var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
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
    SBInitMouseWheel();

    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 ) setMediaKeyboardCallback();

    window.addEventListener( "keydown", checkAltF4, true );
  }
  
  function SBUninitialize()
  {
    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 ) resetMediaKeyboardCallback();
  }
   
  //
  // XUL Event Methods
  //

  // The background image allows us to move the window around the screen
  function onBkgDown( theEvent ) 
  {
    var windowDragger = Components.classes["@songbird.org/Songbird/WindowDragger;1"].getService(Components.interfaces.sbIWindowDragger);
    windowDragger.BeginWindowDrag(10); // automatically ends
  }
  function onBkgUp( ) 
  {
    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 )
    {
      var root = "window." + document.documentElement.id;
      SBDataSetValue( root + ".x", document.documentElement.boxObject.screenX );
      SBDataSetValue( root + ".y", document.documentElement.boxObject.screenY );
    }
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
        var root = "window." + document.documentElement.id;
        SBDataSetValue( root + ".x", document.documentElement.boxObject.screenX );
        SBDataSetValue( root + ".y", document.documentElement.boxObject.screenY );
      }
    }
    catch ( err )
    {
      // If you don't have data functions, just close.
    }
    document.defaultView.close();
  }

  function quitApp()
  {
    thePlayerRepeater.doStop();
    onExit();

    var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
    if (as)
    {
      const V_ATTEMPT = 2;
      as.quit(V_ATTEMPT);
    }
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
      // Open the window
      window.open( "chrome://songbird/content/xul/mainwin.xul", "", "chrome,modal=no" );
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
        if ( gPPS.getPlaying() )
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
        if ( gPPS.getPlaying() )
          onPause( );
        else
          onPlay( );
        break;
    }
  }

}
catch ( err )
{
  alert( err );
}
