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


if (typeof(Cc) == "undefined")
  window.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  window.Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  window.Cu = Components.utils;
if (typeof(Cr) == "undefined")
  window.Cr = Components.results;


Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/kPlaylistCommands.jsm");

/**
 * Media Page Controller
 *
 * In order to display the contents of a library or list, pages
 * must provide a "window.mediaPage" object implementing
 * the Songbird sbIMediaPage interface. This interface allows
 * the rest of Songbird to talk to the page without knowledge 
 * of what the page looks like.
 *
 * In this particular page most functionality is simply 
 * delegated to the sb-playlist widget.
 */
window.mediaPage = {
    
    // The sbIMediaListView that this page is to display
  _mediaListView: null,
    
    // The sb-playlist XBL binding
  _playlist: null, 
    
  
  /** 
   * Gets the sbIMediaListView that this page is displaying
   */
  get mediaListView()  {
    return this._mediaListView;
  },
  
  
  /** 
   * Set the sbIMediaListView that this page is to display.
   * Called in the capturing phase of window load by the Songbird browser.
   * Note that to simplify page creation mediaListView may only be set once.
   */
  set mediaListView(value)  {
    
    if (!this._mediaListView) {
      this._mediaListView = value;
    } else {
      throw new Error("mediaListView may only be set once.  Please reload the page");
    }
  },


  /**
   * Called to check whether this page is the only possible view for the data.
   */
  get isOnlyView() false,
    
    
  /** 
   * Called when the page finishes loading.  
   * By this time window.mediaPage.mediaListView should have 
   * been externally set.  
   */
  onLoad:  function(e) {
    if (!this._mediaListView) {
      Components.utils.reportError("playlistPage.xul did not receive  " + 
                                   "a mediaListView before the onload event!");
      return;
    } 

    var servicePaneNode =
      Cc["@songbirdnest.com/servicepane/library;1"]
        .getService(Ci.sbILibraryServicePaneService)
        .getNodeFromMediaListView(this._mediaListView);
    if (servicePaneNode) {
      document.title = servicePaneNode.displayName;
    }
    else {
      // failed to find the node, stick with the media list name
      document.title = this._mediaListView.mediaList.name;
    }
    
    this._playlist = document.getElementById("playlist");
    
    // Clear the playlist filters. This is a filter-free view.
    this._clearFilterLists();
    
    // Get playlist commands (context menu, keyboard shortcuts, toolbar)
    // Note: playlist commands currently depend on the playlist widget.
    var mgr =
      Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Components.interfaces.sbIPlaylistCommandsManager);
    var cmds = mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT);
    
    // Set up the playlist widget
    this._playlist.bind(this._mediaListView, cmds);
  },
    
    
  /** 
   * Called as the window is about to unload
   */
  onUnload:  function(e) {
    if (this._playlist) {
      this._playlist.destroy();
      this._playlist = null;
    }
  },
    
  
  /** 
   * Show/highlight the MediaItem at the given MediaListView index.
   * Called by the Find Current Track button.
   */
  highlightItem: function(aIndex) {
    this._playlist.highlightItem(aIndex);
  },
    
  
  /** 
   * Called when something is dragged over the tabbrowser tab for this window
   */
  canDrop: function(aEvent, aSession) {
    return this._playlist.nsDragAndDropObserver.canDrop(aEvent, aSession);
  },
    
  
  /** 
   * Called when something is dropped on the tabbrowser tab for this window
   */
  onDrop: function(aEvent, aSession) {
    return this._playlist.
        _dropOnTree(this._playlist.mediaListView.length,
                    Ci.sbIMediaListViewTreeViewObserver.DROP_AFTER,
                    aSession);
  },
    
    
    
  /** 
   * Helper function to parse and unescape the query string. 
   * Returns a object mapping key->value
   */
  _parseQueryString: function() {
    var queryString = (location.search || "?").substr(1).split("&");
    var queryObject = {};
    var key, value;
    for each (var pair in queryString) {
      [key, value] = pair.split("=");
      queryObject[key] = unescape(value);
    }
    return queryObject;
  },
    
    
  /**
   * Configure the playlist filter lists
   */
  _clearFilterLists: function() {
    // Don't use filters for this version. We'll clear them out.
    var filters = this._mediaListView.cascadeFilterSet;
    
    for (var i = filters.length - 1; i > 0; i--) {
     filters.remove(i);
    }
    if (filters.length == 0 || !filters.isSearch(0)) {
      if (filters.length == 1) {
        filters.remove(0);
      }
      filters.appendSearch(["*"], 1);
    }
  }
  
} // End window.mediaPage 


