/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
    // initialize player controls for this faceplate  
    window.focus();
    SBInitMouseWheel();
    
    window.dockDistance = 10;

    var platform;
    try {
      var sysInfo =
        Components.classes["@mozilla.org/system-info;1"]
                  .getService(Components.interfaces.nsIPropertyBag2);
      platform = sysInfo.getProperty("name");                                          
    }
    catch (e) {
      dump("System-info not available, trying the user agent string.\n");
      var user_agent = navigator.userAgent;
      if (user_agent.indexOf("Mac OS X") != -1)
        platform = "Darwin";
    }

    var location = "" + window.location; // Grrr.  Dumb objects.
    if ( location.indexOf("?video") == -1 ) {
      initJumpToFileHotkey();
      setMinMaxCallback();
      document.getElementById("mini_btn_fullscreen").setAttribute("hidden", "true");
    } else {
      document.getElementById("mini_close").hidden = true;

      if (platform == "Darwin")
        document.getElementById("mini_btn_fullscreen").setAttribute("hidden", "true");

      document.getElementById("mini_btn_max").setAttribute("hidden", "true");
      document.getElementById("frame_mini").setAttribute("style", "-moz-border-radius: 0px !important; border-color: transparent !important;"); // Square the frame and remove the border.
    }
    
    if ( (platform == "Darwin") || (platform == "Linux") ){
      document.getElementById("frame_mini").setAttribute("style", "-moz-border-radius: 0px !important; border-color: transparent !important;"); // Square the frame and remove the border.
    } else {
      document.getElementById("mini").setAttribute("style", "background-color: transparent !important;"); // Only on windows. (Bug 1656)
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
  
  
  function onDblClick( evt ) 
  {
    switch (evt.target.nodeName.toLowerCase())
    {
      // Songbird Custom Elements
      case "player_seekbar":
      case "player_volume":
      case "player_repeat":
      case "player_shuffle":
      case "player_playpause":
      case "player_back":
      case "player_forward":
      case "player_mute":
      case "player_numplaylistitems":
      case "player_scaning":
      case "dbedit_textbox":
      case "dbedit_menulist":
      case "exttrackeditor":
      case "servicetree":
      case "playlist":
      case "search":
      case "smartsplitter":
      case "sbextensions":
      case "smart_conditions":
      case "watch_folders":
      case "clickholdbutton":
      // XUL Elements
      case "splitter":
      case "grippy":
      case "button":
      case "toolbarbutton":
      case "scrollbar":
      case "slider":
      case "thumb":
      case "checkbox":
      case "resizer":
      case "textbox":
      case "tree":
      case "listbox":
      case "listitem":
      case "menu":
      case "menulist":
      case "menuitem":
      case "menupopup":
      case "menuseparator":
      // HTML Elements
      case "img":
      case "input":
      case "select":
      case "option":
      case "object":
      case "embed":
      case "body":
      case "html":
      case "div":
      case "a":
      case "ul":
      case "ol":
      case "dl":
      case "dt":
      case "dd":
      case "li":
      case "#text":
        return;
    }  
  
    SBMainWindowOpen();
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
      case 32: // Space
        if ( gPPS.playing )
          onPause( );
        else
          onPlay( );
        break;
    }
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

var theDropPath = "";
var theDropIsDir = false;
var SBMiniDropObserver = 
{
  getSupportedFlavours : function () 
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage("get flavours");
    var flavours = new FlavourSet();
    flavours.appendFlavour("application/x-moz-file","nsIFile");
//    flavours.appendFlavour("application/x-moz-url");
    return flavours;
  },
  onDragOver: function ( evt, flavour, session )
  {
    alert("whoo?");
  },
  onDrop: function ( evt, dropdata, session )
  {
    alert("woot!");
    if ( dropdata.data != "" )
    {
      // if it has a path property
      if ( dropdata.data.path )
      {
        theDropPath = dropdata.data.path;
        theDropIsDir = dropdata.data.isDirectory();
        setTimeout( SBMiniDropped, 10 ); // Next frame
        
      }
    }
  }
};

function SBMiniDropped()
{
  if ( theDropIsDir )
  {
    SBDataSetBoolValue( "media_scan.open", true );
    theMediaScanIsOpen.boolValue = true;
    // otherwise, fire off the media scan page.
    var media_scan_data = new Object();
    media_scan_data.URL = theDropPath;
    media_scan_data.retval = "";
    // Open the non-modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,modal=yes,centerscreen", media_scan_data );
    SBDataSetBoolValue( "media_scan.open", false );
  }
  else if ( gPPS.isMediaURL( theDropPath ) )
  {
    // add it to the db and play it.
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.playAndImportURL(theDropPath); // if the url is already in the lib, it is not added twice
  }
}
