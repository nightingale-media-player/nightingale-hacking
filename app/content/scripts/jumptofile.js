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
      SBOpenWindow( "chrome://songbird/content/xul/jumptofile.xul", "jump_to_file", "chrome,toolbar=no,popup=no,dialog=no,resizable=yes", document );
    else {
      document.__JUMPTO__.defaultView.focus();
      var textbox = document.__JUMPTO__.getElementById("jumpto.textbox");
      textbox.focus();
    }
  }

  var SBEmptyPlaylistCommands = 
  {
    m_Playlist: null,

    getNumCommands: function( aSubMenu, aHost )
    {
      return 0;
    },

    getCommandId: function( aSubMenu, aIndex, aHost )
    {
      return "";
    },

    getCommandText: function( aSubMenu, aIndex, aHost )
    {
      return "";
    },

    getCommandFlex: function( aSubMenu, aIndex, aHost )
    {
      return 0;
    },

    getCommandValue: function( aSubMenu, aIndex, aHost )
    {
      return "";
    },

    getCommandFlag: function( aSubMenu, aIndex, aHost )
    {
      return false;
    },

    getCommandChoiceItem: function( aChoiceMenu, aHost )
    {
      return "";
    },

    getCommandToolTipText: function( aSubMenu, aIndex, aHost )
    {
      return "";
    },

    instantiateCustomCommand: function( aId, aHost ) 
    {
      return null;
    },

    refreshCustomCommand: function( aElement, aId, aHost ) 
    {
    },

    getCommandEnabled: function( aSubMenu, aIndex, aHost )
    {
      return 0;
    },

    onCommand: function( id, value, host)
    {
    },
    
    // The object registered with the sbIPlaylistSource interface acts 
    // as a template for instances bound to specific playlist elements
    duplicate: function()
    {
      var obj = {};
      for ( var i in this )
      {
        obj[ i ] = this[ i ];
      }
      return obj;
    },
    
    setMediaList: function( playlist )
    {
      // Ah.  Sometimes, things are being secure.
      if ( playlist.wrappedJSObject )
        playlist = playlist.wrappedJSObject;
      this.m_Playlist = playlist;
    },
    
    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
  }
 
  var jumpto_ref;
  var source_ref;
  var displayed_ref;
  var source_guid;
  var source_table;
  var source_search;
  var source_filters = [];
  var source_filtersColumn = [];
  var editIdleInterval;
  var service_tree;
  var search_widget;
  var source_playlist;
  
  var string_searchedby = "Searched by";
  var string_filteredby = "Filtered by";
  var string_library = "Library";

  var playingRef_remote;
  var showWebPlaylist_remote;
  
  function syncJumpTo(focus) {
    // because the change to the playlist.ref property may happen after this function has been triggered by an event or a remote observer, introduce a timeout
    setTimeout("doSyncJumpTo(" + !focus + ");", 0);
  }
  
  function onLoadJumpToFile() {
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
    } catch (err) { /* ignore error, we have default strings */ }

    playingRef_remote = SB_NewDataRemote( "playing.ref", null );
    showWebPlaylist_remote = SB_NewDataRemote( "browser.playlist.show", null );

    const on_playingRef_change = { 
      observe: function( aSubject, aTopic, aData ) { syncJumpTo(); } 
    };
    const on_showWebPlaylist_change = { 
      observe: function( aSubject, aTopic, aData ) { syncJumpTo(); } 
    };

    playingRef_remote.bindObserver(on_playingRef_change, true);
    showWebPlaylist_remote.bindObserver(on_showWebPlaylist_change, true);

    syncJumpTo(true);
  }
  
  function doSyncJumpTo(nofocus) {
    window.arguments[0].__JUMPTO__ = document;
    document.syncJumpTo = syncJumpTo;
    search_widget = window.arguments[0].__SEARCHWIDGET__;
    var source = new sbIPlaylistsource();
    var guid;
    var table;
    var search;
    var filters, filtersColumn;
    var plsource;
    var ref;
    source_playlist = window.arguments[0].__CURRENTPLAYLIST__;
    if (!source_playlist) source_playlist = window.arguments[0].__CURRENTWEBPLAYLIST__;
    if (source_playlist) displayed_ref = source_playlist.ref;
    ref = SBDataGetStringValue("playing.ref");
    if (ref != "") {
      // a ref is playing, use it
      guid = source.getRefGUID( ref );
      table = source.getRefTable( ref );
    } else {
      if (source_playlist) {
        // nothing playing, but a playlist object is visible in the source window
        displayed_ref = source_playlist.ref;
        if (!displayed_ref || displayed_ref == "") {
          guid = "songbird";
          table = "library";
          ref = "NC:songbird_library";
        } else {
          guid = source_playlist.guid;
          table = source_playlist.table
          ref = displayed_ref;
        }
      } else {
        // nothing playing, no playlist object in source window, use the library
        guid = "songbird";
        table = "library";
        ref = "NC:songbird_library";
      }
    }
    try { // if the user presses ctrl-j before the page has finished loading, the ref may not have been created yet (grr)
      search = source.getSearchString( ref );
    } catch (e) {
      ensureRefExists(ref, guid, table);
      search = source.getSearchString( ref );
    }
    filters = _getFilters( source, ref );
    filtersColumn = _getFiltersColumn( source, ref );
    _setPlaylist( guid, table, search, filters, filtersColumn, nofocus );
    _selectPlaylist( guid, table );
    _updateSubSearchItem(search, filters, filtersColumn);
  }

  function onPlaylistlistSelect( evt ) {
    var guid, table, search, filters, filtersColumn;
    if (evt.target.getAttribute("id") != "current_play_queue") {
      var guid = evt.target.getAttribute("guid");
      if (guid == "") guid = "songbird";
      var table = evt.target.getAttribute("table");
      if (table == "") table = "library";
      search = "";
      filters = [];
      filtersColumn = [];
    } else {
      var ref = SBDataGetStringValue("playing.ref");
      if (!ref || ref == "") ref = displayed_ref;
      if (!ref || ref == "") ref = "NC:songbird_library";
      var source = new sbIPlaylistsource();
      guid = source.getRefGUID(ref);
      table = source.getRefTable(ref);
      search = source.getSearchString( ref );
      filters = _getFilters( source, ref );
      filtersColumn = _getFiltersColumn( source, ref );
    }
    _setPlaylist( guid, table, search, filters, filtersColumn );
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
            if (item_guid == "") item_guid = "songbird";
            var item_table = item.getAttribute("table");
            if (item_table == "") item_table = "library";
            
            if ( source_guid == item_guid && source_table == item_table ) {
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

  function _selectPlaylist( guid, table ) {
    var menulist = document.getElementById("playable_list");
    menulist.menupopup.builder.addListener( JumpToPlaylistListListener );
    JumpToPlaylistListListener.didRebuild();
  }
  
  function _setPlaylist( guid, table, search, filters, filtersColumn, nofocus ) {
    var playlist = document.getElementById("jumpto.playlist");
    playlist.tree.setAttribute("seltype", "single");
    playlist.forcedcommands = SBEmptyPlaylistCommands;
    playlist.bind(guid, table, null, null, null, null, "jumpto");
    jumpto_ref = playlist.ref;
    if (search) source_search = search; else source_search = "";
    if (filters) { 
      source_filters = filters; 
      source_filtersColumn = filtersColumn; 
    } else {
      source_filters = [];
      source_filtersColumn = [];
    }
    var textbox = document.getElementById("jumpto.textbox");
    if (!nofocus) {
      window.focus();
      textbox.focus();
    }
    playlist.addEventListener("playlist-play", onJumpToPlay, false);
    playlist.addEventListener("playlist-esc", onExit, false);
    _applySearch();
    source_guid = guid;
    source_table = table;
    var source = new sbIPlaylistsource();
    source_ref = source.calculateRef(source_guid, source_table);
    _setFilters( source, jumpto_ref, source_filters, source_filtersColumn );
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
    if (playlist.tree.view.rowCount > 0) {
      playlist.tree.view.selection.clearSelection();
      playlist.tree.view.selection.rangedSelect(0, 0, true);
    } 
  }
  
  function _applySearch() {
    var search = document.getElementById("jumpto.textbox").value;
    if (source_search != "") search = source_search + " " + search;
    // Feed the new search into the list.
    var source = new sbIPlaylistsource();
    // Wait until it is done executing
    waitForQueryCompletionTimeout(source, jumpto_ref, 5000);
    // ...before attempting to override.
    source.setSearchString( jumpto_ref, search, false /* don't reset the filters */ );
  }
  
  function onJumpToPlay(event) {
    var playlist = document.getElementById("jumpto.playlist");
    var first=0;
    var rangeCount = playlist.tree.view.selection.getRangeCount();
    if (rangeCount > 0)
    {
      var start = {};
      var end = {};
      playlist.tree.view.selection.getRangeAt( 0, start, end );
      first = start.value;
    }
    var idcolumn = playlist.tree.columns.getNamedColumn("id"); 
    var rowid=0;
    if (idcolumn != null) 
    {
      rowid = playlist.tree.view.getCellText( first, idcolumn );
    }
    
    // if the user is requesting a playlist other than the one playing currently, switch to it
    //if (source_ref != displayed_ref) {
      // this does not ensure that the ref gets created because the window may not have a playlist object (ie, minibrowser)
      //loadPlaylistPage(source_guid, source_table); 
    //}

    // if the user is requesting a playlist other than the one playing currently, ensure it exists, and reset whatever search or filter it may have 
    var playing_ref = SBDataGetStringValue("playing.ref");
    if (source_ref != playing_ref) {
      ensureRefExists( source_ref, source_guid, source_table );
    } 
    // check whether the user has selected an unfiltered entry, and if that's the case, reset the search and filters for the playlist.
    if (source_search == "" && source_filters.length == 0) {
      var source = new sbIPlaylistsource();
      source.setSearchString(source_ref, "", true /* reset the filters */);
      if (search_widget) search_widget.loadPlaylistSearchString();
    }
    var source = new sbIPlaylistsource();
    waitForQueryCompletionTimeout(source, source_ref, 5000);
    source.forceGetTargets( source_ref, true );
    setTimeout("playSourceRefAndClose("+rowid+");", 0);
  }
  
  function playSourceRefAndClose(rowid) {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.playRefByID(source_ref, rowid);
    onExit();
  }

  function onUnloadJumpToFile() {
    onWindowSaveSizeAndPosition();

    var playlist = document.getElementById("jumpto.playlist");
    playlist.removeEventListener("playlist-esc", onExit, false);
    playlist.removeEventListener("playlist-play", onJumpToPlay, false);
    window.arguments[0].__JUMPTO__ = null;
    
    playingRef_remote.unbind();
    showWebPlaylist_remote.unbind();

    if (!SBDataGetBoolValue("jumpto.nosavestate")) {
      SBDataSetIntValue("jumpto.visible", 0)
    }
  }
  
  function _updateSubSearchItem(search, filters, filtersColumn) {
    var item = document.getElementById("current_play_queue");
    // show or hide and customize the "Current Play Queue" item according to the presence or absence of a search string and filters
    var no_search = (!search || search == "");
    var no_filter = !_hasFilters(filters);
    if (no_search && no_filter) {
      item.setAttribute("hidden", "true");
    } else {
      item.setAttribute("hidden", "false");
      var label = ""
      if (!(no_search && no_filter)) {
        var cat = "";
        if (!no_search) cat = string_searchedby + ' "' + search + '"';
        if (_hasFilters(filters)) { // test the actual content of each filter entry
          if (cat != "") cat += ", ";
          cat += string_filteredby + ' ';
          var nactualfilters = 0;
          for (var i=0;i<filtersColumn.length;i++) {
            if (filters[i] != "") {
              if (nactualfilters > 0) cat += ", ";
              cat += _mixedCase(filtersColumn[i]);
              nactualfilters++;
            }
          }
        }
        if (cat != "") {
          if (source_ref == "NC:songbird_library") label = string_library + " " + cat;
          else {
            var pllabel = getPlaylistLabel(source_guid, source_table);
            if (pllabel) label += pllabel + ' ' + cat;
            else label += cat ;
          }
        }
      }
      item.setAttribute("label", label);
      var list = document.getElementById("playable_list");
      if (list.selectedItem == item) {
        list.selectedItem = null; 
        list.selectedItem = item; 
      }
    }
  }
  
  /*function loadPlaylistPage(guid, table) {
    // if the parent window does not have a service pane (ie, miniplayer), do not attempt to switch to the new page
    if (!service_tree) return; 
    
    var url = "";

    if (guid == "songbird" && table == "library") url = "chrome://songbird/content/xul/main_pane.xul?library";
    else {
      var menulist = document.getElementById("playable_list");
      var menupopup = menulist.menupopup;

      for ( var i = 0, item = menupopup.firstChild; item; item = item.nextSibling, i++ ) {
        var value = item.getAttribute("value");
        if ( value.length > 0 )
        {
          var itemguid = item.getAttribute("guid");
          var itemtable = item.getAttribute("table");
          if (itemguid == guid && itemtable == table)
          {
            url = value;
            break;
          }
        }
      }    
    }
    if (url != "") gBrowser.loadURI(url);
  }*/
  
  function getPlaylistLabel(guid, table) {
    var menulist = document.getElementById("playable_list");
    var menupopup = menulist.menupopup;

    for ( var i = 0, item = menupopup.firstChild; item; item = item.nextSibling, i++ ) {
      var label = item.getAttribute("label");
      if ( label.length > 0 )
      {
        var itemguid = item.getAttribute("guid");
        var itemtable = item.getAttribute("table");
        if (itemguid == guid && itemtable == table) 
        {
          return label;
        }
      }
    }
    return "";
  }
  
  function ensureRefExists( ref, guid, table ) {
    var source = new sbIPlaylistsource();
    var exists = source.refExists(ref);
    if (!exists) {
      source.feedPlaylist( ref, guid, table );
      source.executeFeed( ref );
      waitForQueryCompletionTimeout(source, ref, 5000);
      // After the call is done, force GetTargets
      source.forceGetTargets( ref, false );
    }
  }
  
  function _getFilters( source, ref ) {
    var n = source.getNumFilters( ref );
    var filters = [];
    for (var i=0;i<n;i++) {
      filters.push(source.getFilter( ref, i ));
    }
    return filters;
  }

  function _getFiltersColumn( source, ref ) {
    var n = source.getNumFilters( ref );
    var filtersColumn = [];
    for (var i=0;i<n;i++) {
      filtersColumn.push(source.getFilterColumn( ref, i ));
    }
    return filtersColumn;
  }
  
  function _setFilters( source, ref, filters, filtersColumn ) {
    var n = source.getNumFilters( ref );
    for (var i=0;i<n;i++) {
      source.removeFilter( ref, i );
    }
    n = filters.length;
    for (var i=0;i<n;i++) {
      source.setFilter( ref, i, filters[i], ref + "_" + filtersColumn[i], filtersColumn[i] );
    }
    source.executeFeed( ref );
  }
  
  function _mixedCase(str) {
    return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
  }
  
  function _hasFilters(filters) {
    var n=filters.length;
    for (var i=0;i<n;i++) {
      if (filters[i] != "") return true;
    }
    return false;
  }
  
  function resetFilterLists(source, ref) {
    var n = source.getNumFilters(ref);
    for (var i=0;i<n;i++) {
      source.setFilter( ref, i, "", source.getFilterRef(ref, i), source.getFilterColumn(ref, i));
    }
    source.executeFeed( ref );
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
      dump("Error. jumptofile.js:setMinMaxCallback() \n " + err + "\n");
    }
  }
  
  function waitForQueryCompletionTimeout( source, ref, timeout ) {
    var start = new Date().getTime();
    while ( source.isQueryExecuting( ref ) && ( new Date().getTime() - start < timeout ) ) {
      _sleep( 100 );
    }
  }

  function _sleep( ms ) {
    var thread = Components.classes["@mozilla.org/thread;1"].createInstance();
    thread = thread.QueryInterface(Components.interfaces.nsIThread);
    thread.currentThread.sleep(ms);
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
      // If min size is not yet known and if the window size is different from the document's box object, 
      if (this._minwidth == -1 && window.innerWidth != document.getElementById('jumpto_frame').boxObject.width)
      { 
        // Then we know we've hit the minimum width, record it. Because you can't query it directly.
        this._minwidth = document.getElementById('jumpto_frame').boxObject.width + 1;
      }
      return Math.max(this._minwidth, this._cssminwidth);
    },

    GetMinHeight: function()
    {
      if (this._cssminheight == -1) {
        this._cssminheight = parseInt(getStyle(document.documentElement, "min-height"));
      }
      // If min size is not yet known and if the window size is different from the document's box object, 
      if (this._minheight == -1 && window.innerHeight != document.getElementById('jumpto_frame').boxObject.height)
      { 
        // Then we know we've hit the minimum width, record it. Because you can't query it directly.
        this._minheight = document.getElementById('jumpto_frame').boxObject.height + 1;
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
