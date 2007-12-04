/*
//
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
//
 */


/**
 * Miniplayer controller.  Handles events and controls
 * platform specific presentation details.
 */
var gMiniplayer = {


  ///////////////////////////
  // Window Event Handling //
  ///////////////////////////

  /**
   * Called when the window loads.  Sets up listeners and
   * configures the look of the window.
   */
  onLoad: function onLoad()
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");

    window.focus();
    window.dockDistance = 10;

    // Note, most listeners are hardcoded in miniplayer.xul

    // Hook up the jumpto hotkey. Note that this function is
    // defined in jumpToFile.js
    initJumpToFileHotkey();

    // Prevent window from being resized inappropriately
    this._setMinMaxCallback();


    // Perform platform specific customization
    var platform = this._getPlatform();

    // Set attributes on the Window element so we can use them in CSS.
    var windowElement = document.getElementsByTagName("window")[0];
    windowElement.setAttribute("platform",platform);
    windowElement.setAttribute("hasTitlebar",this._hasTitlebar());

    windowPlacementSanityChecks();
  },




  /**
   * Called when the window is closing. Removes all listeners.
   */
  onUnload: function onUnload()
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");

    resetJumpToFileHotkey();
    closeJumpTo();

    this._resetMinMaxCallback();

  },

  /**
   * Handles keyboard shortcuts
   */
  onKeypress: function onKeypress( evt )
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");

    // Did the user press Alt-F4?
    if (evt.keyCode == 0x73 && evt.altKey)
    {
      evt.preventDefault();
      quitApp();
      return;
    }

    // TODO Does this not interfere with global hotkeys?
    // Should this be a consistent thing that anyone can use?

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
  },


  /**
   * Handles double clicking. Double clicking on most surfaces
   * takes the user back to their previous feathers
   */
  onDblClick: function onDblClick( evt )
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");
    if (evt.target.localName == 'sb-draggable') {
      this.revertFeathers();
    }
  },


  /**
   * Triggers the feathers toggle button
   */
  revertFeathers: function revertFeathers()
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");

    var feathersButton = document.getElementsByTagName("sb-feathers-toggle-button")[0];
    if (feathersButton)
      feathersButton.doCommand();
  },



  ///////////////////////////
  // Drag and Drop Support //
  ///////////////////////////



  /**
   * Return mimetype-ish information indicating what is supported
   */
  getSupportedFlavours: function getSupportedFlavours()
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage("get flavours");
    var flavours = new FlavourSet();

    // TODO does this work under linux? I'm thinking no.

    flavours.appendFlavour("application/x-moz-file","nsIFile");
    //  flavours.appendFlavour("application/x-moz-url");
    return flavours;
  },

  /**
   * Called when an object is dragged over the miniplayer
   */
  onDragOver: function onDragOver( evt, flavour, session )
  {
    // Don't care...
  },


  /**
   * Called when an object is released over the miniplayer
   */
  onDrop: function onDrop( evt, dropdata, session )
  {
    if ( dropdata.data != "" )
    {
      // if it has a path property
      if ( dropdata.data.path )
      {
        var path = dropdata.data.path;
        var isDir = dropdata.data.isDirectory();

        // Handle drop on next frame
        setTimeout( function(obj) { obj._handleDrop(path, isDir) }, 10, this);
      }
    }
  },


  ////////////////////////////
  // Window Min/Max Support //
  ////////////////////////////



  /**
   * Implements sbIWindowMinMaxCallback and is submitted to sbIWindowMinMax.
   * Prevents the window from being resized beyond given limits.
   */
  _minMaxHandler: {

    // Shrink until the box doesn't match the window, then stop.
    _minwidth: -1,
    GetMinWidth: function()
    {
      try // I guess this is just a fallback for if a page doesn't provide its own.
      {
        // If min size is not yet known and if the window size is different from the document's box object,
        if (this._minwidth == -1 && window.innerWidth != document.getElementById('miniplayer_box').boxObject.width)
        {
          // Then we know we've hit the minimum width, record it. Because you can't query it directly.
          this._minwidth = document.getElementById('miniplayer_box').boxObject.width + 1;
        }
      } catch(e) {};
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
  },



  //////////////////////
  // Helper Functions //
  //////////////////////


  /**
   * Installs our sbIWindowMinMaxCallback to sbIWindowMinMax,
   * preventing the window from being resized inappropriately
   */
  _setMinMaxCallback: function _setMinMaxCallback()
  {
    var platfrom = getPlatformString();

    try {

      if (platfrom == "Windows_NT") {
        var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
        var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);

        service.setCallback(document, this._minMaxHandler);
        return;
      }

    }
    catch (err) {
      // No component
      dump("Error. songbird_hack.js:setMinMaxCallback() \n " + err + "\n");
    }

    window.addEventListener("resize", onMinimumWindowSize, false);

    // Read minimum width and height from CSS.
    var minWidth = getStyle(document.documentElement, "min-width");
    var minHeight = getStyle(document.documentElement, "min-height");

    if (minWidth) {
      minWidth = parseInt(minWidth);
    }

    if (!minWidth) {
      minWidth = 480;
    }

    if (minHeight) {
      minHeight = parseInt(minHeight);
    }

    if (!minHeight) {
      minHeight = 22;
    }

    gMinWidth = minWidth;
    gMinHeight = minHeight;

    return;
  },

  _resetMinMaxCallback: function _resetMinMaxCallback()
  {
    var platform = getPlatformString();

    try
    {
      if (platform == "Windows_NT") {
        var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
        var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
        service.resetCallback(document);

        return;
      }

    }
    catch(err) {
      dump("Error. songbird_hack.js: SBUnitialize() \n" + err + "\n");
    }

    dump("\n\n REMOVED LISTENER FOR MINWIDTH/MINHEIGHT\n\n");
    window.removeEventListener("resize", onMinimumWindowSize, false);

    return;
  },

  /**
   * Helper function that acts on a dropped item.
   * Called just after an object is dropped on the player.
   */
  _handleDrop: function _handleDrop(path, isDir) {
    if ( isDir )
    {
      SBDataSetBoolValue( "media_scan.open", true );
      theFileScanIsOpen.boolValue = true;
      // otherwise, fire off the media scan page.
      var media_scan_data = new Object();
      media_scan_data.URL = path;
      media_scan_data.retval = "";
      // Open the non-modal dialog
      SBOpenModalDialog( "chrome://songbird/content/xul/mediaScan.xul", "media_scan", "chrome,centerscreen", media_scan_data );
      SBDataSetBoolValue( "media_scan.open", false );
    }
    else if ( gPPS.isMediaURL( path ) )
    {
      // add it to the db and play it.
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.playAndImportURL(path); // if the url is already in the lib, it is not added twice
    }
  },


  /**
   * Figure out which operating system we are on
   */
  _getPlatform: function _getPlatform() {
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
    return platform;
  },


  /**
   * Has this window been opened with a titlebar?
   */
  _hasTitlebar: function _hasTitlebar() {

    // Jump through some hoops to get at nsIWebBrowserChrome.chromeFlags
    window.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var webnav = window.getInterface(Components.interfaces.nsIWebNavigation);
    var treeItem = webnav.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
    var treeOwner = treeItem.treeOwner;
    var requestor = treeOwner.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var windowChrome = requestor.getInterface(Components.interfaces.nsIWebBrowserChrome);

    // Phew... now, do we have a titlebar?
    return windowChrome.chromeFlags & windowChrome.CHROME_TITLEBAR;
  }

}  // End of gMiniplayer

