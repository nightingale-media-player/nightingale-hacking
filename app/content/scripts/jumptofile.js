/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
      window.openDialog( "chrome://songbird/content/xul/jumptofile.xul", "jump_to_file", "chrome,titlebar=no,resizable=yes", document );
  }

  function onKeyPress(evt) {
    if (evt.charCode == 106 && evt.ctrlKey && !evt.altKey) {
      evt.preventBubble();
      onJumpToFileKey();
    }
  }

  var SBEmptyPlaylistCommands = 
  {
    m_Playlist: null,

    getNumCommands: function()
    {
      return 0;
    },

    getCommandId: function( index )
    {
      return -1;
    },

    getCommandText: function( index )
    {
      return "";
    },

    getCommandFlex: function( index )
    {
      return 0;
    },

    getCommandToolTipText: function( index )
    {
      return "";
    },

    getCommandEnabled: function( index )
    {
      return 0;
    },

    onCommand: function( event )
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
    
    setPlaylist: function( playlist )
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
  
  function onLoadJumpToFile() {
    try {
      fixOSXWindow("window_top", "app_title");
    }
    catch (e) { }

    onWindowLoadSizeAndPosition();
    window.arguments[0].__JUMPTO__ = document;
    service_tree = window.arguments[0].__THESERVICETREE__;
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
        guid = source_playlist.guid;
        table = source_playlist.table
        ref = source_playlist.ref;
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
    _setPlaylist( guid, table, search, filters, filtersColumn );
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
  
  function _setPlaylist( guid, table, search, filters, filtersColumn ) {
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
    window.focus();
    textbox.focus();
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
    if ( ! source.isQueryExecuting( jumpto_ref ) )
    {
      // ...before attempting to override.
      source.setSearchString( jumpto_ref, search );
    }
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
      source.setSearchString(source_ref, "");
      if (search_widget) search_widget.loadPlaylistSearchString();
      //if (source_playlist) source_playlist.resetFilterLists();
      resetFilterLists(source, source_ref);
    }
    var source = new sbIPlaylistsource();
    while( source.isQueryExecuting( source_ref ) )
      ;
    source.forceGetTargets( source_ref, true );
    setTimeout("playSourceRefAndClose("+rowid+");", 0);
  }
  
  function playSourceRefAndClose(rowid) {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.playRefByID(source_ref, rowid);
    onExit();
  }

  function onUnloadJumpToFile() {
    var playlist = document.getElementById("jumpto.playlist");
    playlist.removeEventListener("playlist-esc", onExit, false);
    playlist.removeEventListener("playlist-play", onJumpToPlay, false);
    window.arguments[0].__JUMPTO__ = null;
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
      var label = "Current Play Queue"
      if (!(no_search && no_filter)) {
        var cat = "";
        if (!no_search) cat = 'Searched by "' + search + '"';
        if (_hasFilters(filters)) { // test the actual content of each filter entry
          if (cat != "") cat += ", ";
          cat += 'Filtered by ';
          for (var i=0;i<filtersColumn.length;i++) {
            if (filters[i] != "") {
              if (i > 0) cat += ", ";
              cat += _mixedCase(filtersColumn[i]);
            }
          }
        }
        if (cat != "") {
          if (source_ref == "NC:songbird_library") label += " (Library " + cat + ")";
          else {
            var pllabel = getPlaylistLabel(source_guid, source_table);
            if (pllabel) label += ' (Playlist "' + pllabel + '" ' + cat + ")";
            else label += " (" + cat + ")";
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
    if (url != "") service_tree.launchServiceURL(url);
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
  
  function refExists( ref ) {
    var exists = false;
    try {
      var source = new sbIPlaylistsource();
      source.getSearchString(ref);
      exists = true;
    } catch (e) {}
    return exists;
  }
  
  function ensureRefExists( ref, guid, table ) {
    var exists = refExists(ref);
    if (!exists) {
      var source = new sbIPlaylistsource();
      source.feedPlaylist( ref, guid, table );
      source.executeFeed( ref );
      // Synchronous call!  Woo hoo!
      while( source.isQueryExecuting( ref ) )
        ;
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
    // the user may have disabled the hotkey, we still want him to be able to access the jumpto dialog as a non-global hotkey
    document.addEventListener("keypress", onKeyPress, false);
  }
   
  function resetJumpToFileHotkey()
  {
    var hotkeyActionsComponent = Components.classes["@songbirdnest.com/Songbird/HotkeyActions;1"];
    if (hotkeyActionsComponent) {
      var hotkeyactionsService = hotkeyActionsComponent.getService(Components.interfaces.sbIHotkeyActions);
      if (hotkeyactionsService) hotkeyactionsService.unregisterHotkeyActionBundle(jumptoHotkeyActions);
    }
    document.removeEventListener("keypress", onKeyPress, false);
  }

}
catch ( err )
{
  alert( err );
}
