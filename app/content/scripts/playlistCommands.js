// JScript source code
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
var WEB_PLAYLIST_CONTEXT      = "webplaylist";
var WEB_PLAYLIST_TABLE        = "webplaylist";
var WEB_PLAYLIST_TABLE_NAME   = "&device.webplaylist";
var WEB_PLAYLIST_LIBRARY_NAME = "&device.weblibrary";

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;

function MediaListListener() {
}
MediaListListener.prototype = {
  _addedCount: 0,
  
  get itemAddedCount() {
    return this._addedCount;
  },
  
  onItemAdded: function onItemAdded(list, item) {
    this._addedCount++;
  },
  
  onBeforeItemRemoved: function onBeforeItemRemoved(list, item) {
  },
  
  onAfterItemRemoved: function onAfterItemRemoved(list, item) {
  },
  
  onItemUpdated: function onItemUpdated(list, item, properties) {
  },

  onBatchBegin: function onBatchBegin(list) {
  },
  
  onBatchEnd: function onBatchEnd(list) {
  },
  
  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Ci.sbIMediaListListener) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }  
}

function SelectionUnwrapper(selection) {
  this._selection = selection;
}
SelectionUnwrapper.prototype = {
  _selection: null,
  
  hasMoreElements : function() {
    return this._selection.hasMoreElements();
  },
  
  getNext : function() {
    return this._selection.getNext().mediaItem;
  },
  
  QueryInterface : function(iid) {
    if (iid.equals(Ci.nsISimpleEnumerator) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  }
}

// The house for the web playlist and download commands objects.
var SBWebPlaylistCommands = 
{
  m_Context: {
    m_Playlist: null,
    m_Window: null
  },
  
  m_root_commands :
  {
    m_Types: new Array
    (
      "action",
      "action",
      "action",
      "action",
      "action",
      "action",
      ADDTOPLAYLIST_MENU_TYPE,
      "action",
      "action",
      "separator",
      "action"
  //    "action"
  //    "action"
    ),

    m_Ids: new Array
    (
      "library_cmd_play",
      "library_cmd_play",
      "library_cmd_remove",
      "library_cmd_remove",
      "library_cmd_download",
      "library_cmd_subscribe",
      ADDTOPLAYLIST_MENU_ID,
      "library_cmd_addtolibrary",
      "library_cmd_copylocation",
      "*separator*",
      "library_cmd_showdlplaylist"
  //    "library_cmd_burntocd"
  //    "library_cmd_device"
    ),
    
    m_Names: new Array
    (
      "&command.play",
      "&command.play",
      "&command.remove",
      "&command.remove",
      "&command.download",
      "&command.subscribe",
      ADDTOPLAYLIST_MENU_NAME,
      "&command.addtolibrary",
      "&command.copylocation",
      "*separator*",
      "&command.showdlplaylist"
  //    "&command.burntocd"
  //    "&command.device"
    ),
    
    m_Tooltips: new Array
    (
      "&command.tooltip.play",
      "&command.tooltip.play",
      "&command.tooltip.remove",
      "&command.tooltip.remove",
      "&command.tooltip.download",
      "&command.tooltip.subscribe",
      ADDTOPLAYLIST_MENU_TOOLTIP,
      "&command.tooltip.addtolibrary",
      "&command.tooltip.copylocation",
      "*separator*",
      "&command.tooltip.showdlplaylist"
  //    "&command.tooltip.burntocd"
  //    "&command.tooltip.device"
    ),
    
    m_Keys: new Array
    (
      "&command.shortcut.key.play",
      "&command.shortcut.key.altplay",
      "&command.shortcut.key.remove",
      "&command.shortcut.key.altremove",
      "&command.shortcut.key.download",
      "&command.shortcut.key.subscribe",
      ADDTOPLAYLIST_MENU_KEY,
      "&command.shortcut.key.addtolibrary",
      "&command.shortcut.key.copylocation",
      "",
      "&command.shortcut.key.showdlplaylist"
    ),

    m_Keycodes: new Array
    (
      "&command.shortcut.keycode.play",
      "&command.shortcut.keycode.altplay",
      "&command.shortcut.keycode.remove",
      "&command.shortcut.keycode.altremove",
      "&command.shortcut.keycode.download",
      "&command.shortcut.keycode.subscribe",
      ADDTOPLAYLIST_MENU_KEYCODE,
      "&command.shortcut.keycode.addtolibrary",
      "&command.shortcut.keycode.copylocation",
      "",
      "&command.shortcut.keycode.showdlplaylist"
    ),

    m_Modifiers: new Array
    (
      "&command.shortcut.modifiers.play",
      "&command.shortcut.modifiers.altplay",
      "&command.shortcut.modifiers.remove",
      "&command.shortcut.modifiers.altremove",
      "&command.shortcut.modifiers.download",
      "&command.shortcut.modifiers.subscribe",
      ADDTOPLAYLIST_MENU_MODIFIERS,
      "&command.shortcut.modifiers.addtolibrary",
      "&command.shortcut.modifiers.copylocation",
      "",
      "&command.shortcut.modifiers.dhowdlplaylist"
    )
  },

  _getMenu: function(aSubMenu)
  {
    var cmds;
    
    // ADDTOPLAYLIST
    cmds = addToPlaylistHelper.handleGetMenu(aSubMenu);
    if (cmds) return cmds;
    
    switch (aSubMenu) {
      default:
        cmds = this.m_root_commands;
        break;
    }
    return cmds;
  },

  getNumCommands: function( aSubMenu, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( 
        ( cmds.m_Tooltips.length != cmds.m_Ids.length ) ||
        ( cmds.m_Names.length != cmds.m_Ids.length ) ||
        ( cmds.m_Tooltips.length != cmds.m_Names.length )
        )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return cmds.m_Ids.length;
  },

  getCommandId: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length )
    {
      return "";
    }
    return cmds.m_Ids[ aIndex ];
  },
  
  getCommandType: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length )
    {
      return "";
    }
    return cmds.m_Types[ aIndex ];
  },

  getCommandText: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Names.length )
    {
      return "";
    }
    return cmds.m_Names[ aIndex ];
  },

  getCommandFlex: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( cmds.m_Types[ aIndex ] == "separator" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Tooltips.length )
    {
      return "";
    }
    return cmds.m_Tooltips[ aIndex ];
  },

  getCommandValue: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    switch (cmds.m_Ids[aIndex]) {
      // ...
    }
    return "";
  },

  instantiateCustomCommand: function( aId, aHost ) 
  {
    return null;
  },

  refreshCustomCommand: function( aElement, aId, aHost ) 
  {
  },

  getCommandVisible: function( aSubMenu, aIndex, aHost )
  {
    if (aSubMenu == null && (aIndex == 1 || aIndex == 3)) 
      return (aHost == "shortcuts");
    return true;
  },

  getCommandFlag: function( aSubmenu, aIndex, aHost )
  {
    return false;
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    return "";
  },

  getCommandEnabled: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    var retval = false;
    switch ( cmds.m_Ids[aIndex] )
    {
      case "library_cmd_device": // Not yet implemented
        retval = false;
      break;
      case "library_cmd_remove":
        retval = this.m_Context.m_Playlist.tree.view.selection.getRangeCount() > 0;
      break;      
      case "library_cmd_download": // Only download selected tracks from the library, never the whole library
        if ( ( this.m_Context.m_Playlist.description != "library" ) || ( this.m_Context.m_Playlist.tree.view.selection.count > 0 ) )
          retval = true;
      break;
      case "library_cmd_subscribe": // Never subscribe to the library
        if ( this.m_Context.m_Playlist.description != "library" )
          retval = true;
      break;
      case "library_cmd_showdlplaylist":
        retval = true;
      break;
      case "library_cmd_copylocation":
        retval = this.m_Context.m_Playlist.tree.view.selection.getRangeCount() > 0;
      break; 
      default:
        retval = true;
      break;
    }
    return retval;
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Modifiers.length )
    {
      return "";
    }
    return cmds.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keys.length )
    {
      return "";
    }
    return cmds.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keycodes.length )
    {
      return "";
    }
    return cmds.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  onCommand: function( id, value, host )
  {
    if ( id )
    {
      // ADDTOPLAYLIST
      if (addToPlaylistHelper.handleCommand(id)) return;

      // Was it from the toolbarbutton?
      var tbb = ( host == "toolbar" );
      switch( id )
      {
        case "library_cmd_play":
          // If the user hasn't selected anything, select the first thing for him.
          if ( this.m_Context.m_Playlist.tree.currentIndex == -1 )
          {
            this.m_Context.m_Playlist.tree.view.selection.currentIndex = 0;
            this.m_Context.m_Playlist.tree.view.selection.select( 0 );
          }
          // Repurpose the command to act as if a doubleclick
          this.m_Context.m_Playlist.sendPlayEvent();
        break;
        case "library_cmd_remove":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Context.m_Playlist.removeSelectedTracks();
          }
        break;
        case "library_cmd_download":
        {
          try
          {        
            onBrowserTransfer(new SelectionUnwrapper
                                (this.m_Context.m_Playlist.treeView.selectedMediaItems));
            // And show the download table in the chrome playlist.
            gBrowser.mCurrentTab.switchToDownloadView();
          }
          catch( err )          
          {
            alert( "SBWebPlaylistCommands Error:" + err );
          }
        }
        break;
        case "library_cmd_copylocation":
        {
          var clipboardtext = "";
          var urlCol = "url";
          var columnObj = this.m_Context.m_Playlist.tree.columns.getNamedColumn(urlCol);
          var rangeCount = this.m_Context.m_Playlist.tree.view.selection.getRangeCount();
          for (var i=0; i < rangeCount; i++) 
          {
            var start = {};
            var end = {};
            this.m_Context.m_Playlist.tree.view.selection.getRangeAt( i, start, end );
            for( var c = start.value; c <= end.value; c++ )
            {
              if (c >= this.m_Context.m_Playlist.tree.view.rowCount) 
              {
                continue; 
              }
              
              var val = this.m_Context.m_Playlist.tree.view.getCellText(c, columnObj);
              if (clipboardtext != "") clipboardtext += "\n";
              clipboardtext += val;
            }
          }

          var clipboard = Components.classes["@mozilla.org/widget/clipboardhelper;1"].getService(Components.interfaces.nsIClipboardHelper);
          clipboard.copyString(clipboardtext);
        }
        break;
        case "library_cmd_subscribe":
        {
          // Bring up the subscribe dialog with the web playlist url
          SBSubscribe(null, gBrowser.currentURI);
        }
        break;
        case "library_cmd_addtolibrary":
        {
          var libraryManager =
            Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                      .getService(Components.interfaces.sbILibraryManager);
          var mediaList = libraryManager.mainLibrary;
          
          var treeView = this.m_Context.m_Playlist.treeView;
          var selectionCount = treeView.selectionCount;
          
          var unwrapper = new SelectionUnwrapper(treeView.selectedMediaItems);
          
          var mediaListListener = new MediaListListener();

          mediaList.addListener(mediaListListener);
          mediaList.addSome(unwrapper);
          mediaList.removeListener(mediaListListener);

          var itemsAdded = mediaListListener.itemAddedCount;
          this.m_Context.m_Playlist._reportAddedTracks(itemsAdded,
                                             selectionCount - itemsAdded, mediaList.name);
        }
        break;
        case "library_cmd_showdlplaylist":
        {
          if (this.m_Context.m_Window.location.pathname ==
              '/content/xul/playlist_test2.xul') {
            // we're in the web library / inner playlist view
            gBrowser.loadMediaList(gBrowser.downloadList);
          } else {
            // we're in a web playlist / outer playlist view
            gBrowser.mCurrentTab.switchToDownloadView();
          }
          
        }
        break;
      }
    }
  },
  
  // The object registered with the sbIPlaylistCommandsManager interface acts 
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

  initCommands: function(aHost) { addToPlaylistHelper.init(SBWebPlaylistCommands); },
  shutdownCommands: function() { addToPlaylistHelper.shutdown(); },
  
  setContext: function( context )
  {
    var playlist = context.playlist;
    var window = context.window;
    
    // Ah.  Sometimes, things are being secure.
    
    if ( playlist && playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    
    if ( window && window.wrappedJSObject )
      window = window.wrappedJSObject;
    
    this.m_Context.m_Playlist = playlist;
    this.m_Context.m_Window = window;
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
}; // SBWebPlaylistCommands declaration

/*

// Register the web playlist commands at startup
if ( ( WEB_PLAYLIST_CONTEXT != "" ) && ( WEB_PLAYLIST_TABLE != "" ) )
{
  var mgr = new sbIPlaylistCommandsManager();
  mgr.registerPlaylistCommandsMediaItem( WEB_PLAYLIST_TABLE, "http", SBWebPlaylistCommands );
  mgr.registerPlaylistCommandsMediaItem( "library", "http", SBWebPlaylistCommands );
}

*/

// Debugging Tool
function listProperties(obj, objName) 
{
    var columns = 3;
    var count = 0;
    var result = "";
    for (var i in obj) 
    {
        result += objName + "." + i + " = " + obj[i] + "\t\t\t";
        count = ++count % columns;
        if ( count == columns - 1 )
        {
          result += "\n";
        }
    }
    alert(result);
}

function onBrowserTransfer(mediaItems)
{
    try
    {
        deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                    getService(Components.interfaces.sbIDeviceManager);
        if (deviceManager)
        {
            var downloadCategory = 'Songbird Download Device';
            if (deviceManager.hasDeviceForCategory(downloadCategory))
            {
                var downloadDevice =
                  deviceManager.getDeviceByCategory(downloadCategory);
                
                // Make a magic data object to get passed to the dialog
                var download_data = new Object();
                download_data.retval = "";
                download_data.value = SBDataGetStringValue( "download.folder" );
                
                if ( ( SBDataGetIntValue( "download.always" ) == 1 ) && ( download_data.value.length > 0 ) )
                {
                  download_data.retval = "ok";
                }
                else
                {
                  // Open the window
                  SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,centerscreen", download_data ); 
                }

                // Pick download destination
                if ( ( download_data.retval == "ok" ) && ( download_data.value.length > 0 ) )
                {
                  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                                        .getService(Components.interfaces.nsIPrefBranch2);
                  var downloadListGUID =
                    prefs.getComplexValue("songbird.library.download",
                                          Components.interfaces.nsISupportsString);
                  
                  var libraryManager =
                    Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                              .getService(Components.interfaces.sbILibraryManager);
                  var downloadList = libraryManager.mainLibrary.getMediaItem(downloadListGUID);
                  downloadList.addSome(mediaItems);
                }
            }
        }
    }
    
    catch ( err )
    {
        alert( err );
    }
}

const PAUSERESUME_INDEX = 4;

var SBDownloadCommands = 
{
  m_Context: {
    m_Playlist: null,
    m_Window: null
  },

  m_Device: null,

  m_Types: new Array
  (
    "action",
    "action",
    "action",
    "action",
    "action",
    "separator",
    "action"
  ),
  
  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_play",
    "library_cmd_remove",
    "library_cmd_remove",
    "library_cmd_pause",
    "*separator*",
    "library_cmd_showwebplaylist"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.play",
    "&command.remove",
    "&command.remove",
    "&command.pausedl",
    "*separator*",
    "&command.showwebplaylist"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.play",
    "&command.tooltip.remove",
    "&command.tooltip.remove",
    "&command.tooltip.pause",
    "*separator*",
    "&command.tooltip.showwebplaylist"
  ),

  m_Keys: new Array
  (
    "&command.shortcut.key.play",
    "&command.shortcut.key.altplay",
    "&command.shortcut.key.remove",
    "&command.shortcut.key.altremove",
    "&command.shortcut.key.pause",
    "",
    "&command.shortcut.key.showwebplaylist"
  ),

  m_Keycodes: new Array
  (
    "&command.shortcut.keycode.play",
    "&command.shortcut.keycode.altplay",
    "&command.shortcut.keycode.remove",
    "&command.shortcut.keycode.altremove",
    "&command.shortcut.keycode.pause",
    "",
    "&command.shortcut.keycode.showwebplaylist"
  ),

  m_Modifiers: new Array
  (
    "&command.shortcut.modifiers.play",
    "&command.shortcut.modifiers.altplay",
    "&command.shortcut.modifiers.remove",
    "&command.shortcut.modifiers.altremove",
    "&command.shortcut.modifiers.pause",
    "",
    "&command.shortcut.modifiers.showwebplaylist"
  ),

  getNumCommands: function( aSubmenu, aHost )
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
       )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandType: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Types.length )
    {
      return "";
    }
    return this.m_Types[ aIndex ];
  },

  getCommandId: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex == PAUSERESUME_INDEX ) 
    {
      if ( this.m_Device )
      {
        if ( this.m_Device.getDeviceState('') == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
        {
          this.m_Ids[ aIndex ] = "library_cmd_resume";
        }
        else
        {
          this.m_Ids[ aIndex ] = "library_cmd_pause";
        }
      }
    }
    if ( aIndex >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ aIndex ];
  },

  getCommandText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex == PAUSERESUME_INDEX )
    {
      if ( this.m_Device )
      {
        if ( this.m_Device.getDeviceState('') == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
        {
          this.m_Names[ aIndex ] = "&command.resumedl";
        }
        else
        {
          this.m_Names[ aIndex ] = "&command.pausedl";
        }
      }
    }
    if ( aIndex >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ aIndex ];
  },

  getCommandFlex: function( aSubmenu, aIndex, aHost )
  {
    if ( this.m_Types[ aIndex ] == "separator" ) return 1;
    return 0;
  },

  getCommandValue: function( aSubmenu, aIndex, aHost )
  {
    return "";
  },

  getCommandFlag: function( aSubmenu, aIndex, aHost )
  {
    return false;
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    return "";
  },

  getCommandToolTipText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex == PAUSERESUME_INDEX )
    {
      if ( this.m_Device )
      {
        var deviceState = this.m_Device.getDeviceState('');
        if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
        {
          this.m_Tooltips[ aIndex ] = "&command.tooltip.resume";
        }
        else
        {
          this.m_Tooltips[ aIndex ] = "&command.tooltip.pause";
        }
      }
    }
    if ( aIndex >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ aIndex ];
  },

  instantiateCustomCommand: function( aId, aHost ) 
  {
    return null;
  },

  refreshCustomCommand: function( aElement, aId, aHost ) 
  {
  },

  getCommandVisible: function( aSubMenu, aIndex, aHost )
  {
    if (aSubMenu == null && (aIndex == 1 || aIndex == 3)) 
      return (aHost == "shortcuts");
    return true;
  },

  getCommandEnabled: function( aSubmenu, aIndex, aHost )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( aIndex )
      {
        case 3:
          var deviceState = this.m_Device.getDeviceState('');
          retval = ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING ) || 
                   ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
        break;
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Modifiers.length )
    {
      return "";
    }
    return this.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keys.length )
    {
      return "";
    }
    return this.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keycodes.length )
    {
      return "";
    }
    return this.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  onCommand: function( id, value, host )
  {
    if ( this.m_Device && id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( host == "toolbar" );
      switch( id )
      {
        case "library_cmd_play":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // If the user hasn't selected anything, select the first thing for him.
            if ( this.m_Context.m_Playlist.tree.currentIndex == -1 )
            {
              this.m_Context.m_Playlist.tree.view.selection.currentIndex = 0;
              this.m_Context.m_Playlist.tree.view.selection.select( 0 );
            }
            // Repurpose the command to act as if a doubleclick
            this.m_Context.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_remove":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Context.m_Playlist.removeSelectedTracks();
          }
        break;
        case "library_cmd_pause":
        case "library_cmd_resume":
          var deviceState = this.m_Device.getDeviceState('');
          if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING )
          {
            this.m_Device.suspendTransfer('');
          }
          else if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
          {
            this.m_Device.resumeTransfer('');
          }
          // Since we changed state, update the command buttons.
          this.m_Context.m_Playlist.refreshCommands();
        break;
        case "library_cmd_showwebplaylist":
        {
          if (this.m_Context.m_Window.location.pathname ==
              '/content/xul/playlist_test2.xul') {
            // we're in the download library / inner playlist view
            gBrowser.loadMediaList(gBrowser.webLibrary);
          } else {
            // we're in a web playlist / outer playlist view
            gBrowser.mCurrentTab.switchToWebPlaylistView();
          }
        }
        break;
      }
    }
  },
  
  // The object registered with the sbIPlaylistCommandsManager interface acts 
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

  initCommands: function(aHost) {},
  shutdownCommands: function() {},
  
  setContext: function( context )
  {
    var playlist = context.playlist;
    var window = context.window;

    // Ah.  Sometimes, things are being secure.
    if ( playlist && playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    
    if ( window && window.wrappedJSObject )
      window = window.wrappedJSObject;
    
    this.m_Context.m_Playlist = playlist;
    this.m_Context.m_Window = window;
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

}; // SBDownloadCommands definition

function registerSpecialListCommands() {
  window.removeEventListener("load", registerSpecialListCommands, false);
  
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch2);
  var downloadListGUID =
    prefs.getComplexValue("songbird.library.download",
                          Components.interfaces.nsISupportsString);
  var webListGUID =
    prefs.getComplexValue("songbird.library.web",
                          Components.interfaces.nsISupportsString);

  var mgr = new sbIPlaylistCommandsManager();
  mgr.registerPlaylistCommandsMediaItem(downloadListGUID, "",
                                          SBDownloadCommands);
  mgr.registerPlaylistCommandsMediaItem(webListGUID, "",
                                          SBWebPlaylistCommands);

  window.addEventListener("unload", unregisterSpecialListCommands, false);
}

function unregisterSpecialListCommands() {
  window.removeEventListener("unload", unregisterSpecialListCommands, false);
  
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch2);
  var downloadListGUID =
    prefs.getComplexValue("songbird.library.download",
                          Components.interfaces.nsISupportsString);
  var webListGUID =
    prefs.getComplexValue("songbird.library.web",
                          Components.interfaces.nsISupportsString);

  var mgr = new sbIPlaylistCommandsManager();
  mgr.unregisterPlaylistCommandsMediaItem(downloadListGUID, "",
                                            SBDownloadCommands);
  mgr.unregisterPlaylistCommandsMediaItem(webListGUID, "",
                                            SBWebPlaylistCommands); 
}

window.addEventListener("load", registerSpecialListCommands, false);

try
{
  // Register the download commands at startup if we know what the download table is.
  var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                  getService(Components.interfaces.sbIDeviceManager);

  if (deviceManager)
  {
    var downloadCategory = 'Songbird Download Device';
    if (deviceManager.hasDeviceForCategory(downloadCategory))
    {
      var downloadDevice =
        deviceManager.getDeviceByCategory(downloadCategory);
      SBDownloadCommands.m_Device = downloadDevice;
    }
  }
} catch(e) {}

//
// Default playlist commands.
var SBDefaultCommands = 
{
  m_Context: {
    m_Playlist: null,
    m_Window: null
  },

  m_Types: new Array
  (
    "action",
    "action",
    "action",
    "action",
    "action",
    ADDTOPLAYLIST_MENU_TYPE,
    "action",
    "action"
  ),
  
  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_play",
    "library_cmd_remove",
    "library_cmd_remove",
    "library_cmd_edit",
    ADDTOPLAYLIST_MENU_ID,
    "library_cmd_burntocd",
    "library_cmd_device"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.play",
    "&command.remove",
    "&command.remove",
    "&command.edit",
    ADDTOPLAYLIST_MENU_NAME,
    "&command.burntocd",
    "&command.device"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.play",
    "&command.tooltip.remove",
    "&command.tooltip.remove",
    "&command.tooltip.edit",
    ADDTOPLAYLIST_MENU_TOOLTIP,
    "&command.tooltip.burntocd",
    "&command.tooltip.device"
  ),

  m_Keys: new Array
  (
    "&command.shortcut.key.play",
    "&command.shortcut.key.altplay",
    "&command.shortcut.key.remove",
    "&command.shortcut.key.altremove",
    "&command.shortcut.key.edit",
    ADDTOPLAYLIST_MENU_KEY,
    "&command.shortcut.key.burntocd",
    "&command.shortcut.key.device"
  ),

  m_Keycodes: new Array
  (
    "&command.shortcut.keycode.play",
    "&command.shortcut.keycode.altplay",
    "&command.shortcut.keycode.remove",
    "&command.shortcut.keycode.altremove",
    "&command.shortcut.keycode.edit",
    ADDTOPLAYLIST_MENU_KEYCODE,
    "&command.shortcut.keycode.burntocd",
    "&command.shortcut.keycode.device"
  ),

  m_Modifiers: new Array
  (
    "&command.shortcut.modifiers.play",
    "&command.shortcut.modifiers.altplay",
    "&command.shortcut.modifiers.remove",
    "&command.shortcut.modifiers.altremove",
    "&command.shortcut.modifiers.edit",
    ADDTOPLAYLIST_MENU_MODIFIERS,
    "&command.shortcut.modifiers.burntocd",
    "&command.shortcut.modifiers.device"
  ),


  getNumCommands: function( aSubmenu, aHost )
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
        )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandType: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Types.length )
    {
      return "";
    }
    return this.m_Types[ aIndex ];
  },

  getCommandId: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ aIndex ];
  },

  getCommandText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ aIndex ];
  },

  getCommandFlex: function( aSubmenu, aIndex, aHost )
  {
    if ( this.m_Typess[ aIndex ] == "separator" ) return 1;
    return 0;
  },

  getCommandValue: function( aSubmenu, aIndex, aHost )
  {
    return "";
  },

  getCommandFlag: function( aSubmenu, aIndex, aHost )
  {
    return false;
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    return "";
  },

  getCommandToolTipText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ aIndex ];
  },

  instantiateCustomCommand: function( aId, aHost ) 
  {
    return null;
  },

  refreshCustomCommand: function( aElement, aId, aHost ) 
  {
  },

  getCommandVisible: function( aSubMenu, aIndex, aHost )
  {
    if (aSubMenu == null && (aIndex == 1 || aIndex == 3)) 
      return (aHost == "shortcuts");
    return true;
  },

  getCommandEnabled: function( aSubmenu, aIndex, aHost )
  {
    var playlist = this.m_Context.m_Playlist;
    
    // Bail out early if we don't have our playlist somehow
    if ( ! playlist )
      return false;

    var command = this.m_Ids[aIndex];    

    switch ( command )
    {
      case "library_cmd_device":
      case "library_cmd_burntocd":
      {
        // These commands are not fully implemented yet
        return false;
      }
      break;

      case "library_cmd_remove":
      {
        // Special case for the Smart Playlist - "Remove" makes no sense and
        // should be disabled. If this isn't a Smart Playlist then default
        // logic is appropriate.
        if ( playlist.base_type == "smart" )
          return false;
      }
      break;
    }

    // By default return true if there is at least one item selected
    return playlist.tree.view.selection.getRangeCount() > 0;
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Modifiers.length )
    {
      return "";
    }
    return this.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keys.length )
    {
      return "";
    }
    return this.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keycodes.length )
    {
      return "";
    }
    return this.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  onCommand: function( id, value, host )
  {
    if ( id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( host == "toolbar" );
      switch( id )
      {
        case "library_cmd_play":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Context.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_edit":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            if ( tbb || this.m_Context.m_Playlist.tree.view.selection.count > 1 )
            {
              // Edit the entire track
              this.m_Context.m_Playlist.sendEditorEvent();
            }
            else
            {
              // Edit the context cell
              this.m_Context.m_Playlist.startCellEditing();
            }
          }
        break;
        case "library_cmd_addtoplaylist":
          // XXX fix ! use addtoplaylist.js !
          alert("eek");
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // add the currently selected track to a (possibly new) playlist
            this.m_Context.m_Playlist.addToPlaylist();
          }
        break;
        case "library_cmd_remove":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Context.m_Playlist.removeTracks();
          }
        break;
        case "library_cmd_burntocd":
          if ( this.m_Context.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Context.m_Playlist.sendBurnToCDEvent();
          }
        break;
      }
    }
  },
  
  // The object registered with the sbIPlaylistCommandsManager interface acts 
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

  initCommands: function(aHost) {},
  shutdownCommands: function() {},
  
  setContext: function( context )
  {
    var playlist = context.playlist;
    var window = context.window;
    
    // Ah.  Sometimes, things are being secure.
    
    if ( playlist && playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    
    if ( window && window.wrappedJSObject )
      window = window.wrappedJSObject;
    
    this.m_Context.m_Playlist = playlist;
    this.m_Context.m_Window = window;
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
  
} // end of sbPlaylistCommands


// Default servicetree playlist commands

var SBDefaultServiceCommands = 
{
  m_Context: {
    m_Medialist: null,
    m_Window: null
  },

  m_Types: new Array
  (
    "action",
    "action"
  ),
  
  m_Ids: new Array
  (
    "playlist_cmd_remove",
    "playlist_cmd_rename"
  ),
  
  m_Names: new Array
  (
    "&command.playlist.remove",
    "&command.playlist.rename"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.playlist.remove",
    "&command.tooltip.playlist.rename"
  ),

  m_Keys: new Array
  (
    "&command.playlist.shortcut.key.remove",
    "&command.playlist.shortcut.key.rename"
  ),

  m_Keycodes: new Array
  (
    "&command.playlist.shortcut.keycode.remove",
    "&command.playlist.shortcut.keycode.rename"
  ),

  m_Modifiers: new Array
  (
    "&command.playlist.shortcut.modifiers.remove",
    "&command.playlist.shortcut.modifiers.rename"
  ),



  getNumCommands: function( aSubmenu, aHost )
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
        )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandType: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Types.length )
    {
      return "";
    }
    return this.m_Types[ aIndex ];
  },

  getCommandId: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ aIndex ];
  },

  getCommandText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ aIndex ];
  },

  getCommandFlex: function( aSubmenu, aIndex, aHost )
  {
    if ( this.m_Typess[ aIndex ] == "separator" ) return 1;
    return 0;
  },

  getCommandValue: function( aSubmenu, aIndex, aHost )
  {
    return "";
  },

  getCommandFlag: function( aSubmenu, aIndex, aHost )
  {
    return false;
  },

  getCommandChoiceItem: function( aChoiceMenu, aHost )
  {
    return "";
  },

  getCommandToolTipText: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ aIndex ];
  },

  instantiateCustomCommand: function( aId, aHost ) 
  {
    return null;
  },

  refreshCustomCommand: function( aElement, aId, aHost ) 
  {
  },

  getCommandVisible: function( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  getCommandEnabled: function( aSubmenu, aIndex, aHost )
  {
    return true;
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Modifiers.length )
    {
      return "";
    }
    return this.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keys.length )
    {
      return "";
    }
    return this.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Keycodes.length )
    {
      return "";
    }
    return this.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  onCommand: function( id, value, host )
  {
    if ( id )
    {
      switch( id )
      {
        case "playlist_cmd_remove":
          this.m_Context.m_Medialist.library.remove(this.m_Context.m_Medialist);
        break;
        case "playlist_cmd_rename":
          var servicePane = this.m_Context.m_Window.gServicePane;
          // If we have a servicetree, tell it to make the new playlist node editable
          if (servicePane) {
            // Ask the library service pane provider to suggest where
            // a new playlist should be created
            var librarySPS = Components.classes['@songbirdnest.com/servicepane/library;1']
                                      .getService(Components.interfaces.sbILibraryServicePaneService);
            // Find the servicepane node for our new medialist
            var node = librarySPS.getNodeForLibraryResource(this.m_Context.m_Medialist);
            
            if (node) {
              // Ask the service pane to start editing our new node
              // so that the user can give it a name
              servicePane.startEditingNode(node);
            } else {
              throw("Error: Couldn't find a service pane node for the list we just created\n");
            }

          // Otherwise pop up a dialog and ask for playlist name
          } else {
            var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"  ]
                                          .getService(Components.interfaces.nsIPromptService);

            var input = {value: this.m_Context.m_Medialist.name};
            var title = SBString("renamePlaylist.title", "Rename Playlist");
            var prompt = SBString("renamePlaylist.prompt", "Enter the new name of the playlist.");

            if (promptService.prompt(window, title, prompt, input, null, {})) {
              this.m_Context.m_Medialist.name = input.value;
              this.m_Context.m_Medialist.write();
            }
          }
        break;
      }
    }
  },
  
  // The object registered with the sbIPlaylistCommandsManager interface acts 
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

  initCommands: function(aHost) {},
  shutdownCommands: function() {},
  
  setContext: function( context )
  {
    var medialist = context.medialist;
    var window = context.window;
    
    // Ah.  Sometimes, things are being secure.
    
    if ( window && window.wrappedJSObject )
      window = window.wrappedJSObject;
    
    this.m_Context.m_Medialist = medialist;
    this.m_Context.m_Window = window;
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
  
} // end of sbPlaylistCommands

try {
var mgr = new sbIPlaylistCommandsManager();
mgr.registerPlaylistCommandsMediaList( "", "simple", SBDefaultServiceCommands );
mgr.registerPlaylistCommandsMediaList( "", "smart", SBDefaultServiceCommands );
} catch (e) { alert(e); }

