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

//
// Jump To File
//

// todo:

// function onPlaylistFilterChange() {
//   if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
// }


try
{
  function closeJumpTo() {
    if (document.__JUMPTO__) {
      document.__JUMPTO__.defaultView.close();
    }
  }
  
  function onJumpToFileKey(evt) {
    // "popup=yes" causes nasty issues on the mac with the resizers?
    if (!document.__JUMPTO__)
      SBOpenWindow( "chrome://songbird/content/xul/jumpToFile.xul", 
          "jump_to_file", 
          "chrome,toolbar=no,popup=no,dialog=no,resizable=yes", 
          [document, window.gBrowser?gBrowser:null] );
    else {
      document.__JUMPTO__.defaultView.focus();
      var textbox = document.__JUMPTO__.getElementById("jumpto.textbox");
      textbox.focus();
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
  
  var search_widget;
  var source_playlist;
  
  var string_searchedby = "Searched by";
  var string_filteredby = "Filtered by";
  var string_library = "Library";
  var string_unknown = "Unknown Playlist";

  //var playingRef_remote;
  var showWebPlaylist_remote;
  
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

    onWindowLoadSizeAndPosition();
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
    //XXXlone > this needs to be replaced by a listener on gBrowser, to detect
    // the opening and closing of web playlists, and active tab changes !
    showWebPlaylist_remote = SB_NewDataRemote( "browser.playlist.show", null );

    const on_playingUrl_change = { 
      observe: function( aSubject, aTopic, aData ) { syncJumpTo(); } 
    };
    const on_showWebPlaylist_change = { 
      observe: function( aSubject, aTopic, aData ) { syncJumpTo(); } 
    };

    playingUrl_remote.bindObserver(on_playingUrl_change, true);
    showWebPlaylist_remote.bindObserver(on_showWebPlaylist_change, true);

    var servicepane =
      Components.classes['@songbirdnest.com/servicepane/service;1']
      .getService(Components.interfaces.sbIServicePaneService);
    
    var menulist = document.getElementById("playable_list");
    var menupopup = menulist.menupopup;

    while (menupopup.database.GetDataSources().hasMoreElements()) {
      menupopup.database.RemoveDataSource(
          menupopup.database.GetDataSources().getNext());
    }
    menupopup.database.AddDataSource(servicepane.dataSource);
    menupopup.ref = servicepane.root.id;

    syncJumpTo(true);
  }

  function doSyncJumpTo(nofocus) {
    window.arguments[0][0].__JUMPTO__ = document;
    document.syncJumpTo = syncJumpTo;
    search_widget = window.arguments[0][0].__SEARCHWIDGET__;
    var guid;
    var search;
    var filters;
    var plsource;
    var libraryguid;
    displayed_guid = displayed_libraryguid = null;
    source_playlist = window.arguments[0][1]?window.arguments[0][1].currentPlaylist:null;
    if (source_playlist) displayed_view = source_playlist.mediaListView;
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
               .getService(Components.interfaces.sbIPlaylistPlayback);
    var view = gPPS.playingView;
    if (view) {
      // a view is playing, use it
      guid = view.mediaList.guid;
      libraryguid = view.mediaList.library.guid;
    } else {
      if (source_playlist) {
        // nothing playing, but a playlist object is visible in the source window
        displayed_view = source_playlist.mediaListView;
        if (!displayed_view) {
          // use main library
          guid = libraryManager.mainLibrary.guid;
          libraryguid = guid;
        } else {
          guid = displayed_view.mediaList.guid;
          libraryguid = displayed_view.mediaList.library.guid;
          view = displayed_view;
        }
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
    if (displayed_view) { 
      displayed_guid = displayed_view.mediaList.guid;
      displayed_factoryguid = displayed_view.mediaList.library.guid;
    }
    _setPlaylist( guid, libraryguid, search, filters, view, nofocus );
    _selectPlaylist( guid, libraryguid );
    _updateSubSearchItem(search, filters);
  }

  function onPlaylistlistSelect( evt ) {
    var guid, libraryguid, search, view;
    var filters = [];
    if (evt.target.getAttribute("id") != "current_play_queue") {
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
      if ( mediaList )
        view = mediaList.createView();
    } else {
      view = gPPS.playingView;
      if (!view) 
        view = source_playlist.mediaListView;
      search = _getSearchString( view );
      filters = _getFilters( view );
    }
    _setPlaylist( guid, libraryguid, search, filters, view );
  }

  var JumpToPlaylistListListener =
  {
    junk: "stuff",
  
    didRebuild: function ( builder ) {
      var menulist = document.getElementById("playable_list");
      var menupopup = menulist.menupopup;
      if (source_search != "" || _hasFilters(source_filters)) {
        menulist.selectedItem = menupopup.firstChild;
      } else {
        for ( var i = 0, item = menupopup.firstChild; item; item = item.nextSibling, i++ ) {
          var value = item.getAttribute("value");
          var label = item.getAttribute("label");
          if ( value.length > 0 )
          {
            var item_guid = item.getAttribute("guid");
            var item_libraryguid = item.getAttribute("library");
            if ( source_guid == item_guid && source_libraryguid == item_libraryguid ) {
              menulist.selectedItem = item;
              menulist.setAttribute( "value", value );
              menulist.setAttribute( "label", label );
              break;
            }
          }
        }
      }
    },
    
    willRebuild: function ( builder ) {
    },
    
	  QueryInterface: function(iid) {
      if (!iid.equals(Components.interfaces.nsIXULBuilderListener) &&
          !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
          !iid.equals(Components.interfaces.nsISupports)) {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
		  return this;
	  }    
  }

  function _selectPlaylist( guid, libraryguid ) {
    var menulist = document.getElementById("playable_list");
    var item = document.getElementById("current_play_queue");
    // set default to null, if we don't find the list, we'll use the extra item to display
    // the unknown playlist
    menulist.selectedItem = null;
    // look for the playlist and select it
    menulist.menupopup.builder.addListener( JumpToPlaylistListListener );
    JumpToPlaylistListListener.didRebuild();
  }
  
  function _setPlaylist( guid, libraryguid, search, filters, sourceview, nofocus ) {
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
        if (guid == libraryguid) mediaItem = library;
        else mediaItem = library.getMediaItem(guid);
        if (mediaItem) {
          sourceview = mediaItem.createView();
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
    playlist.addEventListener("playlist-play", onJumpToPlay, false);
    playlist.addEventListener("playlist-esc", onExit, false);
    _applySearch();
    source_guid = guid;
    source_libraryguid = libraryguid;
    source_view = sourceview;
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
    playlist.tree.focus();
    if (playlist.mediaListView.treeView.rowCount > 0) {
      playlist.mediaListView.treeView.selection.clearSelection();
      playlist.mediaListView.treeView.selection.rangedSelect(0, 0, true);
    }
  }
  
  function _applySearch() {
    var search = document.getElementById("jumpto.textbox").value;
    if (source_search != "") search = source_search + " " + search;
    var propArray = Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                              .createInstance(Components.interfaces.sbIMutablePropertyArray);
    propArray.appendProperty("*", search);
    jumpto_view.setSearch(propArray);
  }
  
  function onJumpToPlay(event) {
    var playlist = document.getElementById("jumpto.playlist");
    var first=0;
    var rangeCount = playlist.mediaListView.treeView.selection.getRangeCount();
    if (rangeCount > 0)
    {
      var start = {};
      var end = {};
      playlist.mediaListView.treeView.selection.getRangeAt( 0, start, end );
      first = start.value;
    }
        
    // check whether the user has selected an unfiltered entry, and if that's the case, reset the search and filters for the playlist.
    if (source_search == "" && source_filters.length == 0) {
      _resetSearchString(source_view);
      _resetFilters(source_view);
      if (search_widget) search_widget.loadPlaylistSearchString();
    }

    var mediaItem = playlist.mediaListView.getItemByIndex(first);
    var rowid = source_view.getIndexForItem( mediaItem );

    setTimeout("playSourceViewAndClose("+rowid+");", 0);
  }
  
  function playSourceViewAndClose(rowid) {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.playView(source_view, rowid);
    onExit();
  }

  function onUnloadJumpToFile() {
    onWindowSaveSizeAndPosition();

    var playlist = document.getElementById("jumpto.playlist");
    playlist.removeEventListener("playlist-esc", onExit, false);
    playlist.removeEventListener("playlist-play", onJumpToPlay, false);
    window.arguments[0][0].__JUMPTO__ = null;
    
    playingUrl_remote.unbind();
    showWebPlaylist_remote.unbind();

    if (!SBDataGetBoolValue("jumpto.nosavestate")) {
      SBDataSetIntValue("jumpto.visible", 0)
    }
  }
  
  function _updateSubSearchItem(search, filters) {
    var item = document.getElementById("current_play_queue");
    var menulist = document.getElementById("playable_list");
    // show or hide and customize the "Current Play Queue" item according to the presence or absence of a search string and filters
    var no_search = (!search || search == "");
    var no_filter = !_hasFilters(filters);
    if (no_search && no_filter && menulist.selectedItem != null) {
      // We do not need the extra item, what we're showing was found in the
      // datasource, and has no extra filter or search. Hide the extra item.
      item.setAttribute("hidden", "true");
    } else {
      // We need the extra item, show it.
      var label = ""
      item.setAttribute("hidden", "false");
      // Did we find the playlist we were looking for in the datasource ?
      if (menulist.selectedItem == null) {
        // No, we didn't ! To summarize,
        // The selected playlist is the current play queue, however it was not
        // found in the list of playlists. This means one of several possible scenarios:
        // 1) This is a normal playlist that is just not part of the datasource on
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
    var properties = view.currentSearch;
    if (properties.length) {
      var terms = [];
      var property = null;
      var previousProperty = null;
      for (var i = 0; i < properties.length; i++) {
        property = properties.getPropertyAt(i);
        // Continue adding terms until we get to the next property
        if (previousProperty && property.name != previousProperty.name) {
          break;
        }
        terms.push(property.value);
        previousProperty = property;
      }
      return terms.join(" ");
    }
    return "";
  }

  function _resetSearchString( view ) {
    view.clearSearch();
  }
    
  function _getFilters( view ) {
    var filters = [];
    var properties = view.currentFilter;

    if (properties) {
	    var pm = Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
					    .getService(Components.interfaces.sbIPropertyManager);
      var n = properties.length;
      var filters = [];
      for (var i=0;i<n;i++) {
	      var prop = properties.getPropertyAt(i);

	      if (!this._isInCascadeFilterSet(view, prop.name)) continue;

	      // Get the property info for the property
	      var info = pm.getPropertyInfo(prop.name);
	      
        filters.push( [ prop.name, 
                        prop.value,
                        info.displayName ] );
      }
    }
    return filters;
  }
  
  function _isInCascadeFilterSet(view, prop) {
    var cfs = view.cascadeFilterSet;
    if (!cfs || cfs.length <= 0) return false;
    for (var i=0;i<cfs.length;i++) {
      if (cfs.getProperty(i) == prop) return true;
    }
    return false;
  }

  function _resetFilters( view ) {
    view.clearFilters();
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

  // called by windows that want to support the J hotkey
  function initJumpToFileHotkey()
  {
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
