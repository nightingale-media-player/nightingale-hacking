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
    if (aCommand == "cmd_find")
      gTabBrowser.onFindCommand();
    else if (aCommand == "cmd_findAgain")
      gTabBrowser.onFindAgainCommand();
    else if (aCommand == "cmd_metadata")
      SBTrackEditorOpen(); // open to the last selected tab
    else if (aCommand == "cmd_editmetadata")
      SBTrackEditorOpen("edit"); // open to the 'edit' tab
    else if (aCommand == "cmd_viewmetadata")
      SBTrackEditorOpen("summary"); // open to the 'summary' tab
  },
  
  supportsCommand: function(aCommand)
  {
    if (aCommand == "cmd_find" || aCommand == "cmd_findAgain")
      return true;
    if (aCommand == "cmd_metadata" ||
        aCommand == "cmd_editmetadata" ||
        aCommand == "cmd_viewmetadata")
      return true;
      
    return false;
  },
  
  isCommandEnabled: function(aCommand)
  {
    if (!gTabBrowser.isSelectedTabMediaPage() &&
        (aCommand == "cmd_find" || aCommand == "cmd_findAgain"))
      return true;
    if (aCommand == "cmd_metadata" ||
        aCommand == "cmd_editmetadata" ||
        aCommand == "cmd_viewmetadata") {
      var browser;
      if (typeof SBGetBrowser == 'function') 
        browser = SBGetBrowser();
      if (browser) {
        if (browser.currentMediaPage) {
          var view = browser.currentMediaPage.mediaListView;
          if (view) {
            return view.selection.count > 0;
          }
        }
      }
    }
    
    return false;
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
    
    window.focus();
    windowPlacementSanityChecks();
    
    gTabBrowser = document.getElementById("content");
    top.controllers.insertControllerAt(0, gSongbirdWindowController);
    
    // Set the player controls location
    var playerControlsLocation = 
      Application.prefs.getValue(PREF_PLAYER_CONTROL_LOCATION, false);
    movePlayerControls((playerControlsLocation == "top"));
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
 
        // XXX Tied to the treeView at the moment.  This needs to be made generic.
        playbackService.playView(view, 
            Math.max(view.treeView.selection.currentIndex, -1));
        
        // Since we've handled this play event, prevent any fallback action from
        // occurring.
        event.preventDefault();
      } 
    } catch (e) {
      Components.utils.reportError(e);
    }
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

