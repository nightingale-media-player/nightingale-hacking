/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
  
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");

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

    // Prevent window from being resized inappropriately
    this._setMinMaxCallback();

    // Set attributes on the Window element so we can use them in CSS.
    var windowElement = document.getElementsByTagName("window")[0];
    windowElement.setAttribute("hasTitlebar",this._hasTitlebar());

    windowPlacementSanityChecks();
    initializeDocumentPlatformAttribute();
    
    // so, right now the height is correct but something somewhere after this is going to go
    // and screw it right up. we don't know why, or how, but on windows it ends up being 100px
    // instead of 23px and this setTimeout makes it okay again.
    setTimeout( function() {
      if (parseInt(windowElement.height)) {
        window.resizeTo(windowElement.width, windowElement.height);
      }
    }, 0); // bump to back of current queue
    
  },




  /**
   * Called when the window is closing. Removes all listeners.
   */
  onUnload: function onUnload()
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");
        
    this._resetMinMaxCallback();

  },

  /**
   * Handles keyboard shortcuts
   */
  onKeypress: function onKeypress( evt )
  {
    dump("\nMiniplayer." + arguments.callee.name + "\n");

    dump("\nChecking for quit key\n");
    // Check if the user wishes to quit.
    checkQuitKey(evt);

    // TODO Does this not interfere with global hotkeys?
    // Should this be a consistent thing that anyone can use?
    var sbIMediacoreStatus = Components.interfaces.sbIMediacoreStatus;
    switch ( evt.keyCode )
    {
      case 37: // Arrow Left
        gMM.sequencer.previous();
        break;
      case 39: // Arrow Right
        gMM.sequencer.next();
        break;
      case 40: // Arrow Down
      case 13: // Return
        if ( gMM.status.state == sbIMediacoreStatus.STATUS_PLAYING ||
             gMM.status.state == sbIMediacoreStatus.STATUS_BUFFERING )
          gMM.playbackControl.pause();
        else if(gMM.primaryCore)
          gMM.playbackControl.play();
        else
          // If we have no context, initiate playback
          // via the root application controller
          var app = Components.classes["@songbirdnest.com/Songbird/ApplicationController;1"]
                              .getService(Components.interfaces.sbIApplicationController);
          app.playDefault();
        break;
    }
    switch ( evt.charCode )
    {
      case 32: // Space
          if ( gMM.status.state == sbIMediacoreStatus.STATUS_PLAYING ||
              gMM.status.state == sbIMediacoreStatus.STATUS_BUFFERING )
          gMM.playbackControl.pause();
        else
          gMM.playbackControl.play();
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
      var minHeight = getComputedStyle(document.documentElement, "").minHeight;
      return (minHeight == "none") ? -1 : parseInt(minHeight);
    },

    GetMaxWidth: function()
    {
      var maxWidth = getComputedStyle(document.documentElement, "").maxWidth;
      return (maxWidth == "none") ? -1 : parseInt(maxWidth);
    },

    GetMaxHeight: function()
    {
      var maxHeight = getComputedStyle(document.documentElement, "").maxHeight;
      return (maxHeight == "none") ? -1 : parseInt(maxHeight);
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

    return;
  },

  /**
   * Figure out which operating system we are on
   */
  // XXXpvh: can this be removed and replaced with the GetPlatformString used everywhere else?
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
