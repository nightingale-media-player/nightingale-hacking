/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 
 
var gTabBrowser = null;
var PREF_PLAYER_CONTROL_LOCATION = "songbird.playercontrol.location";


// Assist with moving player controls by setting an attribute on the layout.
function movePlayerControls(aIsOnTop)
{
  var locationVal = aIsOnTop ? "top" : "bottom";
  var contentPlayerWrapper = document.getElementById("content_player_wrapper");
  if (contentPlayerWrapper) {
    if (Application.prefs.getValue(PREF_PLAYER_CONTROL_LOCATION, "") != locationVal) {
      // the location changed, send a metrics ping
      // see http://bugzilla.songbirdnest.com/show_bug.cgi?id=11509
      Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                .createInstance(Components.interfaces.sbIMetrics)
                .metricsInc("mainplayer.playercontrols", "location", locationVal);
    }
    
    contentPlayerWrapper.setAttribute("playercontrols", locationVal);
    Application.prefs.setValue(PREF_PLAYER_CONTROL_LOCATION, locationVal);
    
    // Invoke the broadcasters
    var broadcasterTop = document.getElementById("playercontrols_top");
    var broadcasterBottom = document.getElementById("playercontrols_bottom");
    if (aIsOnTop) {
      broadcasterTop.setAttribute("checked", "true");
      broadcasterBottom.removeAttribute("checked");
    }
    else {
      broadcasterBottom.setAttribute("checked", "true");
      broadcasterTop.removeAttribute("checked");
    }
  }
}


var gSongbirdWindowController = 
{
  doCommand: function(aCommand)
  {
    var pps = gPPS ||
              Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                        .getService(Components.interfaces.sbIPlaylistPlayback);
    if (aCommand == "cmd_find") {
      gTabBrowser.onFindCommand();
    } else if (aCommand == "cmd_findAgain") {
      gTabBrowser.onFindAgainCommand();
    } else if (aCommand == "cmd_print") {
      PrintUtils.print(gTabBrowser.content);
    } else if (aCommand == "cmd_metadata") {
      SBTrackEditorOpen(); // open to the last selected tab
    } else if (aCommand == "cmd_editmetadata") {
      SBTrackEditorOpen("edit"); // open to the 'edit' tab
    } else if (aCommand == "cmd_viewmetadata") {
      SBTrackEditorOpen("summary"); // open to the 'summary' tab
    } else if (aCommand == "cmd_reveal") {
      SBRevealFile(); // reveal the selected file
    } else if (aCommand == "cmd_control_playpause") {
      // If we are already playing something just pause/unpause playback
      if (pps.playing) {
        // if we're playing already then play / pause
        if (pps.paused) {
          pps.play();
        } else {
          pps.pause();
        }
      // Otherwise dispatch a play event.  Someone should catch this
      // and intelligently initiate playback.  If not, just have
      // the playback service play the default.
      } else {
        var event = document.createEvent("Events");
        event.initEvent("Play", true, true);
        var notHandled = window.dispatchEvent(event);
        if (notHandled) {
          pps.play();
        }
      }
    } else if (aCommand == "cmd_control_next") {
      pps.next();
    } else if (aCommand == "cmd_control_previous") {
      pps.previous();
    } else if (aCommand == "cmd_volume_down") {
      pps.volume = Math.max(0, pps.volume - 12.5);
    } else if (aCommand == "cmd_volume_up") {
      pps.volume = Math.min(255, pps.volume + 12.5);
    } else if (aCommand == "cmd_volume_mute") {
      pps.mute = !pps.mute;
    } else if (aCommand == "cmd_delete") {
      SBDeleteMediaList(this._getVisiblePlaylist());
    }
  },
  
  supportsCommand: function(aCommand)
  {
    switch(aCommand) {
      case "cmd_find":
      case "cmd_findAgain":
      case "cmd_print":
        return true;
      case "cmd_metadata":
      case "cmd_editmetadata":
      case "cmd_viewmetadata":
      case "cmd_reveal":
        return true;
      case "cmd_control_playpause":
      case "cmd_control_next":
      case "cmd_control_previous":
        return true;
      case "cmd_delete":
        return (this._getVisiblePlaylist() != null);
      case "cmd_volume_down":
      case "cmd_volume_up":
      case "cmd_volume_mute":
        return true;
    }
    return false;
  },
  
  isCommandEnabled: function(aCommand)
  {
    var browser = null;
    if (typeof SBGetBrowser == 'function') {
      browser = SBGetBrowser();
    }
    var view = null;
    if (browser && browser.currentMediaPage) {
      view = browser.currentMediaPage.mediaListView;
    }

    var pps = gPPS ||
              Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                        .getService(Components.interfaces.sbIPlaylistPlayback);

    switch(aCommand) {
      case "cmd_find":
      case "cmd_findAgain":
        return (!browser.shouldDisableFindForSelectedTab());
      case "cmd_print":
        // printing XUL is not supported, see NS_ERROR_GFX_PRINTER_NO_XUL
        return !(browser.contentDocument instanceof XULDocument);
      case "cmd_metadata":
      case "cmd_editmetadata":
      case "cmd_viewmetadata": {
        if (view) {
          return view.selection.count > 0;
        }
        return false;
      }
      case "cmd_reveal": {
        if (view && view.selection.count == 1) {
          var selection = view.selection.selectedIndexedMediaItems;
          var item = selection.getNext()
                              .QueryInterface(Ci.sbIIndexedMediaItem).mediaItem;
          var uri = item.contentSrc;
          return (uri && uri.scheme == "file");
        }
        return false;
      }
      case "cmd_control_playpause":
        return true;
      case "cmd_control_next":
      case "cmd_control_previous":
        return pps.playing && document.commandDispatcher.focusedWindow == window;
      case "cmd_delete": {
        var list = this._getVisiblePlaylist();
        return (list && list.userEditable);
      }
      case "cmd_volume_down":
        return pps.volume > 0;
      case "cmd_volume_up":
        return pps.volume < 255;
      case "cmd_volume_mute":
        return true;
    }
    return false;
  },
  
  _getVisiblePlaylist: function() 
  {
    var browser;
    if (typeof SBGetBrowser == 'function') 
      browser = SBGetBrowser();
    if (browser) {
      if (browser.currentMediaPage) {
        var view = browser.currentMediaPage.mediaListView;
        if (view) {
          var list = view.mediaList;
          var outerListGuid = 
            list.getProperty(SBProperties.outerGUID);
          if (outerListGuid) {
            return list.library.getMediaItem(outerListGuid);
          }
          return list;
        }
      }
    }
    return null;
  }
};


/**
 * Primary controller for a Songbird XUL layout containing a browser window.
 * Much of playerOpen.js and mainWinInit.js should eventually be moved here.
 */
var gSongbirdPlayerWindow = {


  ///////////////////////////
  // Window Event Handling //
  ///////////////////////////

  /**
   * Called when the window loads.  Sets up listeners and
   * configures the look of the window.
   */
  onLoad: function onLoad()
  {
    window.removeEventListener("load", this._onLoadCallback, false);
    this._onLoadCallback = null;

    this._onUnloadCallback = function(e) { gSongbirdPlayerWindow.onUnload(e); };
    window.addEventListener("unload", 
        this._onUnloadCallback, false);

    this._onPlayCallback = function(e) { gSongbirdPlayerWindow.onPlay(e); };    
    window.addEventListener("Play", 
        this._onPlayCallback, true);

    window.addEventListener("keypress", this.onMainWindowKeyPress, false);
    
    window.focus();
    windowPlacementSanityChecks();
    
    gTabBrowser = document.getElementById("content");
    top.controllers.insertControllerAt(0, gSongbirdWindowController);
    
    // Set the player controls location
    var playerControlsLocation = 
      Application.prefs.getValue(PREF_PLAYER_CONTROL_LOCATION, false);
    movePlayerControls((playerControlsLocation == "top"));
    try
    {
      var timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                          .getService(Ci.sbITimingService);
      // NOTE: Must be in this order, CSPerfEndEULA doesn't always exist and
      // will throw an error. CSPerfLibrary is just a timestamp for non-first 
      // runs.
      timingService.startPerfTimer("CSPerfLibrary");
      timingService.stopPerfTimer("CSPerfLibrary");
      timingService.stopPerfTimer("CSPerfEndEULA");
    }
    catch (e)
    {
      // Ignore errors
    }
 },


  /**
   * Called when the window is closing. Removes all listeners.
   */
  onUnload: function onUnload()
  {
    window.removeEventListener("unload", this._onUnloadCallback, false);
    this._onUnloadCallback = null;
   
    window.removeEventListener("Play",  this._onPlayCallback, true);
    this._onPlayCallback = null;

    window.removeEventListener("keypress", this.onMainWindowKeyPress, false);
  },

  
  /**
   * Called in the capturing phase of a Play event.
   * Looks for a mediaListView that is appropriate to play
   * and if found, cancels the event.
   */
  onPlay: function onPlay( event )
  {
    try {
      // Try to find a view from the event. If one exists that's probably
      // what we should play from.
      var view = this._getMediaListViewForEvent(event);
      
      // If no view could be found, try getting one from the current tab
      if (!view) {
        view = gBrowser.currentMediaListView;
      }
      
      // If we've got a view, try playing it.
      if (view && view.length > 0) {
        var playbackService = 
          Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                    .getService(Components.interfaces.sbIPlaylistPlayback);
 
        playbackService.playView(view, Math.max(view.selection.currentIndex, -1));
        
        // Since we've handled this play event, prevent any fallback action from
        // occurring.
        event.preventDefault();
      } 
    } catch (e) {
      Components.utils.reportError(e);
    }
  },


  onMainWindowKeyPress: function onMainWindowKeyPress( event )
  {
    // the key press handler for pressing spacebar to play, at the top level
    if (document.commandDispatcher.focusedWindow != window ||
        document.commandDispatcher.focusedElement)
    {
      // somebody else has focus
      return true;
    }

    // note that we accept shift key being pressed.
    if (event.charCode != KeyboardEvent.DOM_VK_SPACE ||
        event.ctrlKey || event.altKey || event.metaKey || event.altGraphKey)
    {
      return true;
    }
    
    doMenu("menuitem_control_play");
    event.preventDefault();
    event.stopPropagation();
    return false;
  },



  //////////////////////
  // Helper Functions //
  //////////////////////


  /**
   * Attempts to get a mediaListView from the source of the given event.
   * Used by the onPlay event listener.
   */
  _getMediaListViewForEvent: function _getMediaListViewForEvent( event )
  {
    var target = event.target;
    if (!target) {
      return null;
    }
    
    // If the target has a media list view, use that
    if (target.mediaListView) {
      return target.mediaListView;
    }
    if (target.currentMediaListView) {
      return target.currentMediaListView;
    }    
    
    // If the event came from within a binding, perhaps
    // the view is on the inner anon element.
    target = event.originalTarget;
    if (target.mediaListView) {
      return target.mediaListView;
    }
    if (target.currentMediaListView) {
      return target.currentMediaListView;
    }  
    
    // Maybe this event is from an inner document (browser or iframe)
    if (event.target.ownerDocument != document && window.gBrowser) {
      target = gBrowser.getTabForDocument(event.target.ownerDocument);
      if (target && target.mediaListView) {
        return target.mediaListView;
      }
    }
    return null;
  },

}  // End of gSongbirdPlayerWindow

// Set up bubbling load listener
gSongbirdPlayerWindow._onLoadCallback = 
    function(e) { gSongbirdPlayerWindow.onLoad(e) };
window.addEventListener("load", gSongbirdPlayerWindow._onLoadCallback, false);

