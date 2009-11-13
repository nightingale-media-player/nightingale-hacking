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


var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/kPlaylistCommands.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");

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
    
    // Collection of splitters  
  _splitters: [],
  
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

    // Configure the playlist filters based on the querystring
    this._updateFilterLists();
    this._generateFilters();

    // Get playlist commands (context menu, keyboard shortcuts, toolbar)
    // Note: playlist commands currently depend on the playlist widget.
    var mgr =
      Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Components.interfaces.sbIPlaylistCommandsManager);
    var cmds = mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT);

    // Set up the playlist widget
    this._playlist.bind(this._mediaListView, cmds);

    // Adjust the height of the filter pane to be proportional to the height
    // of the display pane. Only do this if the |sb-playlist-splitter| sizing
    // prefs do not exist. (i.e. first run).
    var prefs = Application.prefs;
    if (!prefs.has("songbird.splitter.sb-playlist-splitter.before.height") &&
        !prefs.has("songbird.splitter.sb-playlist-splitter.after.height"))
    {
      var splitter = document.getElementById("sb-playlist-splitter");
      if (splitter) {
        var page = document.getElementById("sb-playlist-media-page");
        var pageHeight = page.boxObject.height;

        // Make the filter pane cover 30% of the area
        const FILTER_COVER_AREA = 0.30;
        var splitFactor = parseInt(pageHeight * FILTER_COVER_AREA);

        // Make sure the splitter covers lines completely
        var filterTree = document.getElementById("sb-filterlist-tree");
        var rowLineHeight = parseInt(getComputedStyle(filterTree, "").lineHeight);
        if (splitFactor % rowLineHeight != 0) {
          splitFactor = rowLineHeight * (Math.round(splitFactor / rowLineHeight));
        }

        // Finally, adjust the splitter spacing
        splitter.setBeforeHeight(splitFactor);
        splitter.setAfterHeight(pageHeight - splitFactor);
      }
    }
  },
    
    
  /** 
   * Called as the window is about to unload
   */
  onUnload:  function(e) {
    for (var s = 0; s < this._splitters.length; s++) {
      this._splitters[s].removeEventListener("mousemove", this._onFilterSplitterMove, true);
      this._splitters[s].removeEventListener("mousedown", this._onFilterSplitterDown, true);
      this._splitters[s].removeEventListener("mouseup", this._onFilterSplitterUp, true);
    }
    
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
  _updateFilterLists: function() {
    
    // This URL is registered as two different media pages. 
    // One with filters, one without. 
    // Use the querystring to determine what to show.
    var queryString = this._parseQueryString();
    
    var filters = this._mediaListView.cascadeFilterSet;
    
    // Set up standard filters if not already present.
    // Note that the first filter should be search.
    if (filters.length <= 1) {
      // Restore the last library filterset or set our default
      var filterSet = SBDataGetStringValue( "library.filterset" );
      if ( filterSet.length > 0 ) {
        filterSet = filterSet.split(";");
      }
      else {
        filterSet = [];
      }
      
      // if we have fewer than the default number of filters, append some extra
      var defaultFilterSet = [
        SBProperties.genre,
        SBProperties.artistName,
        SBProperties.albumName
      ];
      for (var i = filterSet.length; i < defaultFilterSet.length; i++) {
        filterSet.push(defaultFilterSet[i]);
      }
      
      for each (var filter in filterSet) {
        filters.appendFilter(filter);
      }
    }
  },

  /**
   * Generate the filter elements
   */
  _generateFilters: function() {
    // Hide the playlist splitter
    var psplitter = document.getElementById("sb-playlist-splitter");
    psplitter.setAttribute("hidden","true");

    // Get and empty the parent
    var parent = document.getElementById("sb-playlist-filters");
    while (parent.firstChild) {
      parent.removeChild(parent.firstChild);
    };

    // Create the filters based upon the cascadeFilterSet data.
    var cfs = this.mediaListView.cascadeFilterSet;
    var length = cfs.length;
    var filters = [];
    
    for (var i = 0; i < length; i++) {
      // If this is a search then keep going.
      if (cfs.isSearch(i)) {
        continue;
      }

      // And start creating the filter
      var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
      var filterlist = document.createElementNS(XUL_NS, "sb-filterlist");
      filterlist.setAttribute("enableColumnDrag", "false");
      filterlist.setAttribute("flex", "1");

      if (this.mediaListView instanceof Ci.sbIFilterableMediaListView) {
        let constraint = this.mediaListView.filterConstraint;
        if (constraint) {
          for (let group in ArrayConverter.JSEnum(constraint.groups)) {
            if (!(group instanceof Ci.sbILibraryConstraintGroup )) {
              continue;
            }
            if (!group.hasProperty(SBProperties.contentType)) {
              continue;
            }
            let types =
              ArrayConverter.JSArray(group.getValues(SBProperties.contentType));
            if (types.length == 1) {
              filterlist.setAttribute("filter-category-value", types[0]);
              break;
            }
          }
        }
      }

      parent.appendChild(filterlist);
      
      filterlist.bind(this.mediaListView, i);
      filters.push(filterlist);

      if (i < length - 1) {
        var splitter = document.createElement("sb-smart-splitter");
        splitter.setAttribute("id", "filter_splitter" );
        splitter.setAttribute("stateid", "filter_splitter_" + cfs.getProperty(i) + this.mediaListView.mediaList.guid );
        splitter.setAttribute("state", "open");
        splitter.setAttribute("resizebefore", "closest");
        splitter.setAttribute("resizeafter", "closest");
        splitter.setAttribute("collapse", "never");

        var grippy = document.createElement( "grippy" );
        splitter.appendChild(grippy);
        // do not appendChild now, wait until all filterlists have been created (see below)
        this._splitters.push(splitter);
      }
    }
    
    // once the filterlists have been created, insert the splitters are their appropriate spot.
    // we do this now rather than during the filter creation loop because the splitters
    // need to be able to find their before/after targets on construction (so as to be able
    // to reload their positions correctly), so if we did do appendChild in the previous loop,
    // the after target would never be found, and the positions would be wrong
    for (var s=0;s<this._splitters.length;s++) {
      var splitter = this._splitters[s];
      parent.insertBefore(splitter, filters[s+1]);
    }

    // If we didn't actually add any filters then hide the filter box.
    if (!parent.firstChild) {
      parent.setAttribute("hidden", "true");
    } else {
      // Once we know we have at least one filter, show the playlist splitter.
      psplitter.removeAttribute("hidden");
    }
  }
} // End window.mediaPage 


