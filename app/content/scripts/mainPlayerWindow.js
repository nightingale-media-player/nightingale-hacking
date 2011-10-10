/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */
 
 
var gTabBrowser = null;
var PREF_PLAYER_CONTROL_LOCATION = "nightingale.playercontrol.location";

if ("undefined" == typeof(Ci)) {
  this.Ci = Components.interfaces;
}
if ("undefined" == typeof(Cc)) {
  this.Cc = Components.classes;
}
if ("undefined" == typeof(Cr)) {
  this.Cr = Components.results;
}

Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

// Assist with moving player controls by setting an attribute on the layout.
function movePlayerControls(aIsOnTop)
{
  var locationVal = aIsOnTop ? "top" : "bottom";
  var contentPlayerWrapper = document.getElementById("content_player_wrapper");
  if (contentPlayerWrapper) {
    if (Application.prefs.getValue(PREF_PLAYER_CONTROL_LOCATION, "") != locationVal) {
      // the location changed, send a metrics ping
      // see http://bugzilla.getnightingale.com/show_bug.cgi?id=11509
      Components.classes["@getnightingale.com/Nightingale/Metrics;1"]
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


var gNightingaleWindowController = 
{
  doCommand: function(aCommand)
  {
    function dispatchEvent(aType, aCanBubble, aCanCancel, aTarget) {
      var newEvent = document.createEvent("Events");
      newEvent.initEvent(aType, aCanBubble, aCanCancel);
      return (aTarget || window).dispatchEvent(newEvent);
    }
    
    var mm = gMM ||
             Cc["@getnightingale.com/Nightingale/Mediacore/Manager;1"]
               .getService(Components.interfaces.sbIMediacoreManager);
    var status = mm.status;
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
    } else if (aCommand == "cmd_getartwork") {
      SBGetArtworkOpen(); // Show the get artwork dialog
    } else if (aCommand == "cmd_exportmedia") {
      var exportService = Cc["@getnightingale.com/media-export-service;1"]
                            .getService(Ci.sbIMediaExportService);
      exportService.exportNightingaleData();
    } else if (aCommand == "cmd_reveal") {
      SBRevealFile(); // reveal the selected file
    } else if (aCommand == "cmd_find_current_track") {
      if (!mm.sequencer.view) {
        // nothing to see here, move along
        return;
      }

      // steal the focus before going to the track, because of a bug in XR that
      // will crash the app if the tree is currently in edit mode and we try to
      // execute the command (crash in nsFrame::HasView during repaint when the
      // selection moves). Fixes bug 13493.
      document.documentElement.focus();

      // Before showing the current track we trigger
      // a custom event, so that if this object is used wihout
      // a gBrowser object, the current window may still perform
      // its own custom action.
      var handled = !dispatchEvent("ShowCurrentTrack", false, true);
      if (handled) { 
        return;
      }
      gTabBrowser.showIndexInView(mm.sequencer.view, mm.sequencer.viewPosition);
    } else if (aCommand == "cmd_control_playpause") {
      // If we are already playing something just pause/unpause playback
      if (status.state == status.STATUS_PLAYING ||
          status.state == status.STATUS_BUFFERING) {
        mm.playbackControl.pause();
      }
      else if(status.state == status.STATUS_PAUSED) {
        mm.playbackControl.play();
      // Otherwise dispatch a play event.  Someone should catch this
      // and intelligently initiate playback.  If not, just have
      // the playback service play the default.
      } 
      else {
        var event = document.createEvent("Events");
        event.initEvent("Play", true, true);
        window.dispatchEvent(event);
      }
    } else if (aCommand == "cmd_control_next") {
      mm.sequencer.next();
    } else if (aCommand == "cmd_control_previous") {
      mm.sequencer.previous();
    } else if (aCommand == "cmd_volume_down") {
      mm.volumeControl.volume = Math.max(0, mm.volumeControl.volume - 0.05);
    } else if (aCommand == "cmd_volume_up") {
      mm.volumeControl.volume = Math.min(1, mm.volumeControl.volume + 0.05);
    } else if (aCommand == "cmd_volume_mute") {
      mm.volumeControl.mute = !mm.volumeControl.mute;
    } else if (aCommand == "cmd_delete") {
      SBDeleteMediaList(this._getTargetPlaylist());
    } else if (aCommand == "cmd_mediapage_next") {
      gNightingalePlayerWindow.nextMediaPage();
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
      case "cmd_find_current_track":
      case "cmd_exportmedia":
        return true;
      case "cmd_control_playpause":
      case "cmd_control_next":
      case "cmd_control_previous":
        return true;
      case "cmd_getartwork":
      case "cmd_delete":
        return (this._getTargetPlaylist() != null);
      case "cmd_volume_down":
      case "cmd_volume_up":
      case "cmd_volume_mute":
        return true;
      case "cmd_mediapage_next":
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

    var mm = gMM||
             Cc["@getnightingale.com/Nightingale/Mediacore/Manager;1"]
               .getService(Components.interfaces.sbIMediacoreManager);
    var status = mm.status;
    
    var playing = ( status.state == status.STATUS_BUFFERING ||
                    status.state == status.STATUS_PLAYING  ||
                    status.state == status.STATUS_PAUSED );
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
      case "cmd_getartwork":
        return (view != null);
      case "cmd_exportmedia":
        var exportService = Cc["@getnightingale.com/media-export-service;1"]
                              .getService(Ci.sbIMediaExportService);
        return exportService.hasPendingChanges;
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
      case "cmd_find_current_track":
        return mm.sequencer.view ? true : false;
      case "cmd_control_playpause":
        return true;
      case "cmd_control_next":
      case "cmd_control_previous":
        var status = mm.status;
        var playing = ( status.state == status.STATUS_PLAYING ||
                        status.state == status.STATUS_BUFFERING ||
                        status.state == status.STATUS_PAUSED );
        return playing && document.commandDispatcher.focusedWindow == window;
      case "cmd_delete": {
        var node = gServicePane.getKeyboardFocusNode();
        if(node && node.editable == false) {
          return false;
        }
      }
      case "cmd_volume_down":
        return mm.volumeControl.volume > 0;
      case "cmd_volume_up":
        return mm.volumeControl.volume < 1;
      case "cmd_volume_mute":
        return true;
      case "cmd_mediapage_next":
        return (view != null);
    }
    return false;
  },
  
  _getTargetPlaylist: function() 
  {
    var list;
    var knode = gServicePane.getKeyboardFocusNode(true);
    if (knode) {
      var libraryServicePane =
        Components.classes['@getnightingale.com/servicepane/library;1']
        .getService(Components.interfaces.sbILibraryServicePaneService);
      list = libraryServicePane.getLibraryResourceForNode(knode);
      
    } else {
      var browser;
      if (typeof SBGetBrowser == 'function') 
        browser = SBGetBrowser();
      if (browser) {
        if (browser.currentMediaPage) {
          var view = browser.currentMediaPage.mediaListView;
          if (view) {
            list = view.mediaList;
          }
        }
      }
    }
    if (list) {
      var outerListGuid = 
        list.getProperty(SBProperties.outerGUID);
      if (outerListGuid) {
        return list.library.getMediaItem(outerListGuid);
      }
      return list;
    }
    return null;
  }
};


/**
 * Primary controller for a Nightingale XUL layout containing a browser window.
 * Much of playerOpen.js and mainWinInit.js should eventually be moved here.
 */
var gNightingalePlayerWindow = {


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

    this._onUnloadCallback = function(e) { gNightingalePlayerWindow.onUnload(e); };
    window.addEventListener("unload", 
        this._onUnloadCallback, false);

    this._onPlayCallback = function(e) { gNightingalePlayerWindow.onPlay(e); };    
    window.addEventListener("Play", this._onPlayCallback, false);

    window.addEventListener("keypress", this.onMainWindowKeyPress, false);
    
    window.focus();
    windowPlacementSanityChecks();
    
    gTabBrowser = document.getElementById("content");
    top.controllers.insertControllerAt(0, gNightingaleWindowController);
    
    // Set the player controls location
    var playerControlsLocation = 
      Application.prefs.getValue(PREF_PLAYER_CONTROL_LOCATION, false);
    movePlayerControls((playerControlsLocation == "top"));
    try
    {
      var timingService = Cc["@getnightingale.com/Nightingale/TimingService;1"]
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
   
    window.removeEventListener("Play",  this._onPlayCallback, false);
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
      if (!(view && view.length > 0)) {
        view = gBrowser.currentMediaListView;
      }
      
      // If the current tab has failed, try the media tab (if it exists)
      if (!(view && view.length > 0) && gBrowser.mediaTab) {
        view = gBrowser.mediaTab.mediaListView;
      }
      
      // If we've got a view, try playing it.
      if (view && view.length > 0) {
        var mm = 
          Cc["@getnightingale.com/Nightingale/Mediacore/Manager;1"]
                           .getService(Ci.sbIMediacoreManager);
         
        mm.sequencer.playView(view, 
                              Math.max(view.selection.currentIndex, 
                                       Ci.sbIMediacoreSequencer.AUTO_PICK_INDEX));
        
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

  nextMediaPage: function gNightingalePlayerWindow_nextMediaPage() {
    // no need to do this if we can't get the browser
    if (typeof SBGetBrowser != 'function')
      return;

    var browser = SBGetBrowser();

    var mediaListView = browser.currentMediaListView;
    if (!mediaListView) {
      // no media list view, can't switch anything
      return;
    }

    var mediaPageMgr = Cc["@getnightingale.com/Nightingale/MediaPageManager;1"]
                         .getService(Ci.sbIMediaPageManager);
    var pages = mediaPageMgr.getAvailablePages(mediaListView.mediaList);
    var LSP = Cc["@getnightingale.com/servicepane/library;1"]
                .getService(Ci.sbILibraryServicePaneService);
    var type = LSP.getNodeContentTypeFromMediaListView(mediaListView);
    var current = mediaPageMgr.getPage(mediaListView.mediaList, null, type);
    var page = null;

    while (pages.hasMoreElements()) {
      page = pages.getNext().QueryInterface(Ci.sbIMediaPageInfo);
      if (page.contentUrl == current.contentUrl) {
        if (!pages.hasMoreElements()) {
          // we're at the last page, restart enumeration and go to first
          pages = mediaPageMgr.getAvailablePages(mediaListView.mediaList);
        }
        page = pages.getNext().QueryInterface(Ci.sbIMediaPageInfo);
        break;
      }
    }

    if (!page) {
      Components.reportError(new Components.Exception(
        "Failed to find any pages from media list view",
        Components.results.NS_ERROR_NOT_AVAILABLE));
      return;
    }

    mediaPageMgr.setPage(mediaListView.mediaList, page);
    browser.loadMediaList(mediaListView.mediaList,  
                          null,
                          null, 
                          mediaListView, 
                          null);
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

}  // End of gNightingalePlayerWindow

// Set up bubbling load listener
gNightingalePlayerWindow._onLoadCallback = 
    function(e) { gNightingalePlayerWindow.onLoad(e) };
window.addEventListener("load", gNightingalePlayerWindow._onLoadCallback, false);

