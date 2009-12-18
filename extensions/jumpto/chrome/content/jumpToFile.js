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

//
// Jump To File
//

// todo:

// function onPlaylistFilterChange() {
//   if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
// }


Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");

try
{
  if (typeof(LibraryUtils) == "undefined") {
    Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
  }

  function closeJumpTo() {
    if (document.__JUMPTO__) {
      document.__JUMPTO__.defaultView.close();
    }
  }
  
  function onJumpToFileKey(evt) {
    // "popup=yes" causes nasty issues on the mac with the resizers?
    if (!document.__JUMPTO__)
      SBOpenWindow( "chrome://jumpto/content/jumpToFile.xul", 
          "jump_to_file", 
          "chrome,toolbar=no,popup=no,dialog=no,resizable=yes", 
          [document, window.gBrowser?gBrowser:null] );
    else {
      if (document.__JUMPTO__) {
        document.__JUMPTO__.defaultView.focus();
        var textbox = document.__JUMPTO__.getElementById("jumpto.textbox");
        textbox.focus();
      }
    }
  }

 
  var jumpto_view;
  var source_view;
  var displayed_view;
  var defaultlibraryguid;
  var source_guid;
  var source_libraryguid;
  var source_search;
  var source_filters = [];
  var source_filtersColumn = [];
  var editIdleInterval;
  var service_tree;
  var libraryManager;
  var previous_play_view;
  
  // setting this variable to true will make the play function operate on the 
  // jumpto view itself, with its search parameters, rather that the source, 
  // unfiltered view
  var play_own_view = false;
  
  // setting this variable to false will prevent the jumpto box from monitoring 
  // playback and automatically switching to the currently playing playlist
  var monitor_playback = true;
  
  // setting this variable to true will cause the jumpto sort order to be applied
  // to the source view when playback is requested
  var sync_sort = false;
  
  var search_widget;
  
  var string_searchedby = "Searched by";
  var string_filteredby = "Filtered by";
  var string_library = "Library";
  var string_unknown = "Unknown Playlist";
  
  var playingUrl_remote;
  
  function syncJumpTo(focus) {
    // because the change to the playing view may happen after this function has been triggered by an event or a remote observer, introduce a timeout
    setTimeout("doSyncJumpTo(" + !focus + ");", 0);
  }
  
  function onLoadJumpToFile() {
    libraryManager =
      Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                .getService(Components.interfaces.sbILibraryManager);
    defaultlibraryguid = libraryManager.mainLibrary.guid;

    if (!SBDataGetBoolValue("jumpto.nosavestate")) {
      SBDataSetIntValue("jumpto.visible", 1)
    }
    
    windowPlacementSanityChecks();
    
    setJumptoMinMaxCallback();

    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    try {
      string_searchedby = songbirdStrings.GetStringFromName("jumptofile.searchedby");
      string_filteredby = songbirdStrings.GetStringFromName("jumptofile.filteredby");
      string_library = songbirdStrings.GetStringFromName("jumptofile.library");
      string_unknown = songbirdStrings.GetStringFromName("jumptofile.unknown");
    } catch (err) { /* ignore error, we have default strings */ }

    playingUrl_remote = SB_NewDataRemote( "faceplate.play.url", null );

    const on_playingUrl_change = { 
      observe: function( aSubject, aTopic, aData ) { syncJumpTo(); } 
    };

    playingUrl_remote.bindObserver(on_playingUrl_change, true);

    var servicepane =
      Components.classes['@songbirdnest.com/servicepane/service;1']
      .getService(Components.interfaces.sbIServicePaneService);
    
    var menulist = document.getElementById("playable_list");
    var menupopup = menulist.menupopup;

    menulist.addEventListener("playlist-menuitems-changed", _onMenuItemsChanged, true);
    
    document.addEventListener("keypress", onJumpToKeypress, true);

    syncJumpTo(true);
  }

  function doSyncJumpTo(nofocus) {
    if (window.arguments && 
        window.arguments[0] &&
        window.arguments[0][0]) {
      window.arguments[0][0].__JUMPTO__ = document;
      search_widget = window.arguments[0][0].__SEARCHWIDGET__;
      displayed_view = window.arguments[0][1]?window.arguments[0][1].currentMediaListView:null;
    } else {
      displayed_view = null;
    }
    document.syncJumpTo = syncJumpTo;
    var guid;
    var search;
    var filters;
    var plsource;
    var libraryguid;
    var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                       .getService(Components.interfaces.sbIMediacoreManager);
    var view = mm.sequencer.view;

    // Don't use the view if it's not valid.  This can happen if the sequencer
    // obtained a view and then the feathers changed.  The view could then be
    // invalid (see bug 15713).  Ideally, the sequencer would remove its view if
    // it becomes invalid.  Not doing this check here can result in an exception
    // in _resetSearchString because it will cause a select call to be made to
    // the selection, and the selection won't have a tree.
    if (view &&
        (!view.treeView ||
         !view.treeView.selection ||
         !view.treeView.selection.tree)) {
      view = null;
    }

    if (view) {
      // a view is playing, use it
      guid = view.mediaList.guid;
      libraryguid = view.mediaList.library.guid;
    } else {
      if (displayed_view) {
        // nothing playing, but a playlist object is visible in the source window
        guid = displayed_view.mediaList.guid;
        libraryguid = displayed_view.mediaList.library.guid;
        view = displayed_view;
      } else {
        // nothing playing, no playlist object in source window, use the library
        guid = libraryManager.mainLibrary.guid;
        libraryguid = guid;
      }
    }
    if (view) {
      // we have a view, it's either the currently playing view or the displayed view, in both cases we need to read its search string
      search = _getSearchString(view);
      filters = _getFilters(view);
    } else {
      search = "";
      filters = [];
    }
    if (monitor_playback) {
      _setPlaylist( guid, libraryguid, search, filters, view, nofocus );
      _selectPlaylist( guid, libraryguid );
    }
    _updateSubSearchItem(search, filters);
  }

  function onPlaylistlistSelect( evt ) {
    var guid, libraryguid, search, view;
    var filters = [];
    if (evt && evt.target.getAttribute("id") != "current_play_queue") {
      var guid = evt.target.getAttribute("guid");
      var libraryguid = evt.target.getAttribute("library");
      //alert(guid + ' - ' + libraryguid);
      if (guid == "") guid = defaultlibraryguid;
      if (libraryguid == "") libraryguid = defaultlibraryguid;
      search = "";
      var library = libraryManager.getLibrary( libraryguid );
      var mediaList = null;
      if (guid == libraryguid) mediaList = library;
      else mediaList = library.getMediaItem(guid);
      if ( mediaList ) 
        mediaList = mediaList.QueryInterface(Components.interfaces.sbIMediaList);
      if ( mediaList ) {
        view = mediaList.createView();
        view.filterConstraint = LibraryUtils.standardFilterConstraint;
      }
    } else {
      view = mm.sequencer.view;
      if (!view) 
        view = displayed_view;
      search = _getSearchString( view );
      filters = _getFilters( view );
      guid = view.mediaList.guid;
      libraryguid = view.mediaList.library.guid;
    }
    _setPlaylist( guid, libraryguid, search, filters, view );
  }

  function _selectPlaylist( guid, libraryguid ) {
    var menulist = document.getElementById("playable_list");
    // set default to null, if we don't find the list, we'll use the extra item to display
    // the unknown playlist
    menulist.selectedItem = null;
    // look for the playlist and select it
    _onMenuItemsChanged();
  }
  
  function _onMenuItemsChanged() {
    if (!monitor_playback) return;
    var menulist = document.getElementById("playable_list");
    var menupopup = menulist.menupopup;
    if (source_search != "" || _hasFilters(source_filters)) {
      menulist.selectedItem = menupopup.firstChild;
    } else {
      for ( var i = 0, item = menupopup.firstChild; item; item = item.nextSibling, i++ ) {
        var sbtype = item.getAttribute("sbtype");
        var label = item.getAttribute("label");
        if ( sbtype == "playlist-menuitem" )
        {
          var item_guid = item.getAttribute("guid");
          var item_libraryguid = item.getAttribute("library");
          if ( source_guid == item_guid && source_libraryguid == item_libraryguid ) {
            menulist.selectedItem = item;
            //menulist.setAttribute( "value", value );
            menulist.setAttribute( "label", label );
            break;
          }
        }
      }
    }
  }
  
  function _setPlaylist( guid, libraryguid, search, filters, sourceview, nofocus ) {
    if (source_guid == guid &&
        source_libraryguid == libraryguid &&
        /*source_view && source_view == sourceview &&*/
        source_search == search &&
        filtersEqual(filters, source_filters)) return;
    
    /*
    dump("source_guid = " + source_guid + "\n");
    dump("guid = " + guid + "\n");
    dump("source_view = " + source_view + "\n");
    dump("sourceview = " + sourceview + "\n");
    dump("source_search = " + source_search + "\n");
    dump("search = " + search + "\n");
    dump("source_filters.length = " + (source_filters ? source_filters.length : 0) + "\n");
    dump("filters.length = " + (filters ? filters.length : 0) + "\n");
    /* */
    
    if (libraryguid == null) {
      libraryguid = defaultlibraryguid;
    }
    if (guid == null) {
      var library = libraryManager.getLibrary(libraryguid);
      if (library) {
        guid = library.guid;
      }
    }    

    if (!sourceview) {
      var library = libraryManager.getLibrary(libraryguid);
      if ( library ) {
        var mediaItem;
        if (guid == libraryguid) mediaItem = library;
        else mediaItem = library.getMediaItem(guid);
        if (mediaItem) {
          sourceview = mediaItem.createView();
          sourceview.filterConstraint = LibraryUtils.standardFilterConstraint;
        }
      }
    }

    if (!sourceview) {
      dump("error in jumptofile::_setPlaylist, sourceview is null");
      return;
    }
    
    jumpto_view = sourceview.clone();
    
    var playlist = document.getElementById("jumpto.playlist");
    playlist.tree.setAttribute("seltype", "single");
    playlist.bind(jumpto_view);
    
    if (search) source_search = search; else source_search = "";
    if (filters.length) source_filters = filters; else source_filters = [];
    var textbox = document.getElementById("jumpto.textbox");
    if (!nofocus) {
      window.focus();
      textbox.focus();
    }
    playlist.addEventListener("Play", onJumpToPlay, true);
    playlist.addEventListener("keypress", onListKeypress, true);
    _applySearch();
    source_guid = guid;
    source_libraryguid = libraryguid;
    source_view = sourceview;
  }
  
  function filtersEqual(a, b) {
    if (a.length != b.length) return false;
    for (var j=0;j<a.length;j++) {
      if (a[j].length != b[j].length) return false;
      for (var i=0;i<a[j].length;i++) {
        if (a[j][i] != b[j][i]) return false;
      }
    }
    return true;
  }
  
  // for the playlist 
  function onListKeypress(evt) {
    try
    {
      switch ( evt.keyCode )
      {
        case 27: // Esc
          // close the window          
          onExit();
          break;
        case 13: // Esc
          // close the window          
          onJumpToPlay();
          evt.preventDefault();
          evt.stopPropagation();
          break;
      }
    }
    catch( err )
    {
      alert( err )
    }
  }
  
  // for the edit box
  function onSearchKeypress(evt) {
    try
    {
      switch ( evt.keyCode )
      {
        case 27: // Esc
          // close the window          
          onExit();
          break;
        case 13: // Return
          onSearchEditEnter();
          // reselect
          break;      
        default:
          if ( editIdleInterval )
          {
            clearInterval( editIdleInterval );
          }
          editIdleInterval = setInterval( onSearchEditIdle, 1000 );
          break;      
      }
    }
    catch( err )
    {
      alert( err )
    }
  }

  // for the dialog 
  function onJumpToKeypress(evt) {
    try
    {
      switch ( evt.keyCode )
      {
        case evt.DOM_VK_ESCAPE:
          onExit();
          break;
        case 13: // Enter
          // ignore
          evt.preventDefault();
          break;
      }
    }
    catch( err )
    {
      alert( err )
    }
  }
  
  function onPlaylistlistKeypress(evt) {
    var menulist = document.getElementById("playable_list");
    if (!menulist.open) {
      switch (evt.keyCode) {
        case evt.DOM_VK_DOWN:
        case evt.DOM_VK_UP:
          menulist.open = true;
          break;
        default:
          return;
      }
      evt.preventDefault();
      evt.stopPropagation();
    }
  }

  function onSearchEditIdle(evt) {
    if ( editIdleInterval )
    {
      clearInterval( editIdleInterval );
    }
    _applySearch();
  }

  function onSearchEditEnter(evt) {
    _applySearch();
    var playlist = document.getElementById("jumpto.playlist");
    // change focus to playlist
    if (playlist.mediaListView.length > 0) {
      playlist.tree.focus();
      playlist.mediaListView.treeView.selection.clearSelection();
      setTimeout(
        function() { 
          playlist.mediaListView.treeView.selection.toggleSelect(0); 
        }, 100);
    }
  }
  
  function _applySearch() {
    _resetSearchString(jumpto_view);

    var search = document.getElementById("jumpto.textbox").value;
    if (source_search != "") search = source_search + " " + search;

    var cfs = jumpto_view.cascadeFilterSet;
    var searchIndex;
    for (searchIndex = 0; searchIndex < cfs.length; ++searchIndex) {
      if (cfs.isSearch(searchIndex)) {
        break;
      }
    }
    if (searchIndex == cfs.length) {
      searchIndex = cfs.appendSearch(["*"], 1);
    }
    if (search && search != "") {
      var searchArray = search.split(" ");
      cfs.set(searchIndex, searchArray, searchArray.length);
    } else { 
      cfs.set(searchIndex, [], 0);
    }
  }
  
  function onJumpToPlay(event) {
    var playlist = document.getElementById("jumpto.playlist");

    // check whether the user has selected an unfiltered entry, and if that's the case, reset the search and filters for the playlist.
    if (source_search == "" && source_filters.length == 0) {
      _resetSearchString(source_view);
      _resetFilters(source_view);
      if (search_widget) search_widget.loadPlaylistSearchString();
    }

    var mediaItem = playlist.mediaListView.selection.currentMediaItem;
    if (!mediaItem && (playlist.mediaListView.length > 0)) {
      mediaItem = playlist.mediaListView.getItemByIndex(0);
    }
    if (!mediaItem)
      return;

    var rowid;
    if (!play_own_view) {
      if (sync_sort) 
        source_view.setSort(jumpto_view.currentSort);
      rowid = source_view.getIndexForItem( mediaItem );
    } else {
      rowid = first;
    }

    setTimeout("playSourceViewAndClose("+rowid+");", 0);
  }
  
  function playSourceViewAndClose(rowid) {
    var view = play_own_view ? jumpto_view : source_view;
    var mm = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"].getService(Components.interfaces.sbIMediacoreManager);
    mm.sequencer.playView(view, rowid);
    onExit();
  }

  function onUnloadJumpToFile() {
    document.removeEventListener("keypress", onJumpToKeypress, true);

    var menulist = document.getElementById("playable_list");
    menulist.removeEventListener("playlist-menuitems-changed", _onMenuItemsChanged, true);

    var playlist = document.getElementById("jumpto.playlist");
    playlist.removeEventListener("Play", onJumpToPlay, true);
    playlist.removeEventListener("keypress", onListKeypress, true);
    window.arguments[0][0].__JUMPTO__ = null;
    
    playingUrl_remote.unbind();

    if (!SBDataGetBoolValue("jumpto.nosavestate")) {
      SBDataSetIntValue("jumpto.visible", 0)
    }
  }
  
  function _updateSubSearchItem(search, filters) {
    var item = document.getElementById("current_play_queue");
    if (!monitor_playback) {
      item.setAttribute("hidden", "true");
      return;
    }
    var menulist = document.getElementById("playable_list");
    // show or hide and customize the "Current Play Queue" item according to the presence or absence of a search string and filters
    var no_search = (!search || search == "");
    var no_filter = !_hasFilters(filters);
    if (no_search && no_filter && menulist.selectedItem != null) {
      // We do not need the extra item, what we're showing was found in the
      // xul, and has no extra filter or search. Hide the extra item.
      item.setAttribute("hidden", "true");
    } else {
      // We need the extra item, show it.
      var label = ""
      item.setAttribute("hidden", "false");
      // Did we find the playlist we were looking for in the xul ?
      if (menulist.selectedItem == null) {
        // No, we didn't ! To summarize,
        // The selected playlist is the current play queue, however it was not
        // found in the list of playlists. This means one of several possible scenarios:
        // 1) This is a normal playlist that is just not part of the xul on
        // purpose, maybe an extension manages it ? It's not in the servicetree 
        // but it may still have a name, so try to grab it.
        label = this.getPlaylistLabel(source_guid, source_libraryguid);
        // 2) If no name was found, this may be because the playlist in question
        // is a webplaylist, these have no assigned name, so try to grab the
        // originPage property.
        if (!label) label = this.getPlaylistOriginPage(source_guid, source_libraryguid);
        // 3) If that's still not it, we are showing a playlist that has no name,
        // no originPage property, no nothing that we can show to describe it
        // in any meaningful way to the user, so use a default string.
        if (!label) label = string_unknown;
      } else {
        // We did find the playlist we want to show, but it has filters and/or
        // search terms, cook up a string that describes these.
        if (!(no_search && no_filter)) {
          var cat = "";
          if (!no_search) cat = string_searchedby + ' "' + search + '"';
          if (_hasFilters(filters)) { // test the actual content of each filter entry
            if (cat != "") cat += ", ";
            cat += string_filteredby + ' ';
            var nactualfilters = 0;
            for (var i=0;i<filters.length;i++) {
              if (nactualfilters > 0) cat += ", ";
              cat += _mixedCase(filters[i][2]);
              nactualfilters++;
            }
          }
          if (cat != "") {
            if (source_guid == defaultlibraryguid) label = string_library + " " + cat;
            else {
              var pllabel = getPlaylistLabel(source_guid, source_libraryguid);
              if (pllabel) label += pllabel + ' ' + cat;
              else label += cat ;
            }
          }
        }
      }
      // Set the extra item's label, it describes what we are showing.
      item.setAttribute("label", label);
      // Select the item
      menulist.selectedItem = item;
    }
  }
  
  function getPlaylistLabel(guid, libraryguid) {
    try {
      var library = libraryManager.getLibrary( libraryguid );
      if (library) {
        if (libraryguid == guid) return library.name;
        var item = library.getMediaItem(guid);
        return item.name;
      }
    } catch (e) {}
    return "";
  }
  
  function getPlaylistOriginPage(guid, libraryguid) {
    try {
      var library = libraryManager.getLibrary( libraryguid );
      if (library) {
        if (libraryguid == guid) return library.getProperty(SBProperties.originPage);
        var item = library.getMediaItem(guid);
        return item.getProperty(SBProperties.originPage);
      }
    } catch (e) {}
    return "";
  }

  function _getSearchString( view ) {
    // XXXsteve We need a better way to discover the actual search terms
    // rather than reverse engineer it from the search constaint
    var search = view.searchConstraint;
    if (search) {
      var terms = [];
      var groupCount = search.groupCount;
      for (var i = 0; i < groupCount; i++) {
        var group = search.getGroup(i);
        var property = group.properties.getNext();
        terms.push(group.getValues(property).getNext());
      }
      return terms.join(" ");
    }
    return "";
  }

  function _resetSearchString( view ) {
    var cfs = view.cascadeFilterSet;
    for ( var i=0; i < cfs.length; i++ ) {
      if (cfs.isSearch(i)) {
        cfs.set(i, [], 0);
        break;
      }
    }
  }
    
  function _getFilters( view ) {

    var filters = [];

    var filter = view.filterConstraint;
    if (!filter) {
      return filters;
    }
	    var pm = Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
					    .getService(Components.interfaces.sbIPropertyManager);
    var groupCount = filter.groupCount;
    for (var i = 0; i < groupCount; i++) {
      var group = filter.getGroup(i);
      var properties = group.properties;
      while (properties.hasMore()) {
        var prop = properties.getNext();

//xxxlone          alert(prop + " - " + group.getValues(prop).getNext());
        if (this._isInCascadeFilterSet(view, prop)) {
          var info = pm.getPropertyInfo(prop);
	      
          filters.push( [ prop,
                          group.getValues(prop).getNext(),
                        info.displayName ] );
      }
    }
    }
    return filters;
  }
  
  function _isInCascadeFilterSet(view, prop) {
    var cfs = view.cascadeFilterSet;
    if (!cfs || cfs.length <= 0) return false;
    for (var i=0;i<cfs.length;i++) {
      if (cfs.isSearch(i)) continue;
      if (cfs.getProperty(i) == prop) return true;
    }
    return false;
  }

  function _resetFilters( view ) {
    // don't assign null here, use standard constraint.
    view.filterConstraint = LibraryUtils.standardFilterConstraint;
  }
  
  function _mixedCase(str) {
    return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
  }
  
  function _hasFilters(filters) {
    return (filters && filters.length > 0);
  }


  var jumptoHotkeyActions = {
   
    _packagename: "jumpto",
    _actions: [ "open" ],
    _stringbundle: "chrome://songbird/locale/songbird.properties", 
    get_actionCount: function() { return this._actions.length; },
    enumActionLocaleDescription: function (idx) { return this._getLocalizedPackageName() + ": " + this._getLocalizedActionName(this._actions[idx]); },
    enumActionID: function(idx) { return this._packagename + "." + this._actions[idx]; },

    // this is called when an action has been triggered
    onAction: function(idx) {
      switch (idx) {
        case 0: onJumpToFileKey(); break;
      }
    },
    _localpackagename: null,
    _sbs: null,
    _songbirdStrings: null,
    
    _getLocalizedString: function(str, defaultstr) {
      var r = defaultstr;
      if (!this._sbs) {
        this._sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
        this._songbirdStrings = this._sbs.createBundle(this._stringbundle);
      }
      try {
        r = this._songbirdStrings.GetStringFromName(str);
      } catch (err) { /* we have default */ }
      return r;
    },
    
    // the local package name is taken from the specified string bundle, using the key "hotkeys.actions.package", where package is
    // the internal name of the package, as specified in this._packagename
    
    _getLocalizedPackageName: function() {
      if (!this._localpackage) 
        this._localpackage = this._getLocalizedString("hotkeys.actions." + this._packagename, this._packagename); 
      return this._localpackage;
    },
    
    // the local action name is taken from the specified string bundle, using the key "hotkeys.actions.package.action", where package
    // is the internal name of the package, as specified in this._packagename, and action is the internal name of the action, as 
    // specified in the this._actions array.
    
    _getLocalizedActionName: function(action) {
      return this._getLocalizedString("hotkeys.actions." + this._packagename + "." + action, action);
    },

    // -------------------------
    // interface code

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIHotkeyActionBundle) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
  };

  jumptoHotkeyActions.__defineGetter__( "actionCount", jumptoHotkeyActions.get_actionCount);

  function initJumpToFileHotkey()
  {
    // Only initialize jump to for main Songbird windows.
    var windowType = document.documentElement.getAttribute("windowtype");
    if (windowType != "Songbird:Main")
      return;

    // Set up to continue initialization after loading the window.  Reset after
    // window unloaded.
    window.addEventListener("load", initJumpToFileHotkeyAfterLoad, false);
    window.addEventListener("unload", resetJumpToFileHotkey, false);
  }

  function initJumpToFileHotkeyAfterLoad() {
    var hotkeyActionsComponent = Components.classes["@songbirdnest.com/Songbird/HotkeyActions;1"];
    if (hotkeyActionsComponent) {
      var hotkeyactionsService = hotkeyActionsComponent.getService(Components.interfaces.sbIHotkeyActions);
      if (hotkeyactionsService) hotkeyactionsService.registerHotkeyActionBundle(jumptoHotkeyActions);
    }
    SBDataSetBoolValue("jumpto.nosavestate", false);
    if (SBDataGetIntValue("jumpto.visible")) {
      onJumpToFileKey();
    }
  }
   
  function resetJumpToFileHotkey()
  {
    window.removeEventListener("load", initJumpToFileHotkeyAfterLoad, false);
    window.removeEventListener("unload", resetJumpToFileHotkey, false);

    SBDataSetBoolValue("jumpto.nosavestate", true);
    var hotkeyActionsComponent = Components.classes["@songbirdnest.com/Songbird/HotkeyActions;1"];
    if (hotkeyActionsComponent) {
      var hotkeyactionsService = hotkeyActionsComponent.getService(Components.interfaces.sbIHotkeyActions);
      if (hotkeyactionsService) hotkeyactionsService.unregisterHotkeyActionBundle(jumptoHotkeyActions);
    }
  }
  
  function toggleJumpTo() {
    if (SBDataGetIntValue("jumpto.visible")) closeJumpTo();
    else onJumpToFileKey();
  }

  function setJumptoMinMaxCallback()
  {
    try {
      var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
      if (windowMinMax) {
        var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
        if (service)
          service.setCallback(document, SBJumptoWindowMinMaxCB);
      }
    }
    catch (err) {
      // No component
      dump("Error. jumpToFile.js:setMinMaxCallback() \n " + err + "\n");
    }
  }
  
  var SBJumptoWindowMinMaxCB = 
  {
    // Shrink until the box doesn't match the window, then stop.
    _minwidth: -1,
    _minheight: -1,
    _cssminwidth: -1,
    _cssminheight: -1,
    GetMinWidth: function()
    {
      if (this._cssminwidth == -1) {
        this._cssminwidth = parseInt(getStyle(document.documentElement, "min-width"));
      }
      var frame = document.getElementById('dialog-outer-frame');
      if (!frame) {
        // using plucked or other feather not using the hidechrome dialog binding
        frame = document.getAnonymousNodes(document.documentElement)[0];
      }
      // If min size is not yet known and if the window size is different from the document's box object, 
      if (this._minwidth == -1 && window.innerWidth != frame.boxObject.width)
      { 
        // Then we know we've hit the minimum width, record it. Because you can't query it directly.
        this._minwidth = frame.boxObject.width + 1;
      }
      return Math.max(this._minwidth, this._cssminwidth);
    },

    GetMinHeight: function()
    {
      if (this._cssminheight == -1) {
        this._cssminheight = parseInt(getStyle(document.documentElement, "min-height"));
      }
      var frame = document.getElementById('dialog-outer-frame');
      if (!frame) {
        // using plucked or other feather not using the hidechrome dialog binding
        frame = document.getAnonymousNodes(document.documentElement)[0];
      }
      // If min size is not yet known and if the window size is different from the document's box object, 
      if (this._minheight == -1 && window.innerHeight != frame.boxObject.height)
      { 
        // Then we know we've hit the minimum width, record it. Because you can't query it directly.
        this._minheight = frame.boxObject.height + 1;
      }
      return Math.max(this._minheight, this._cssminheight);
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
      setTimeout(onExit, 0);
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

  function getStyle(el,styleProp)
  {
    var v;
    if (el) {
      var s = document.defaultView.getComputedStyle(el,null);
      v = s.getPropertyValue(styleProp);
    }
    return v;
  }


}
catch ( err )
{
  alert( err );
}
