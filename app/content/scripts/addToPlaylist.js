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
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

const ADDTOPLAYLIST_MENU_TYPE      = "submenu";
const ADDTOPLAYLIST_MENU_ID        = "library_cmd_addtoplaylist";
const ADDTOPLAYLIST_MENU_NAME      = "&command.addtoplaylist";
const ADDTOPLAYLIST_MENU_TOOLTIP   = "&command.tooltip.addtoplaylist";
const ADDTOPLAYLIST_MENU_KEY       = "&command.shortcut.key.addtoplaylist";
const ADDTOPLAYLIST_MENU_KEYCODE   = "&command.shortcut.keycode.addtoplaylist";
const ADDTOPLAYLIST_MENU_MODIFIERS = "&command.shortcut.modifiers.addtoplaylist";


const ADDTOPLAYLIST_COMMAND_ID = "library_cmd_addtoplaylist:";
const ADDTOPLAYLIST_NEWPLAYLIST_COMMAND_ID = "library_cmd_addtoplaylist_createnew";

// ----------------------------------------------------------------------------
// The "Add to playlist" dynamic command object
// ----------------------------------------------------------------------------
var SBPlaylistCommand_AddToPlaylist = 
{
  m_Context: {
    m_Playlist: null,
    m_Window: null
  },

  m_addToPlaylist: null,
  
  m_root_commands :
  {
    m_Types: new Array
    (
      ADDTOPLAYLIST_MENU_TYPE
    ),

    m_Ids: new Array
    (
      ADDTOPLAYLIST_MENU_ID
    ),
    
    m_Names: new Array
    (
      ADDTOPLAYLIST_MENU_NAME
    ),
    
    m_Tooltips: new Array
    (
      ADDTOPLAYLIST_MENU_TOOLTIP
    ),
    
    m_Keys: new Array
    (
      ADDTOPLAYLIST_MENU_KEY
    ),

    m_Keycodes: new Array
    (
      ADDTOPLAYLIST_MENU_KEYCODE
    ),

    m_Modifiers: new Array
    (
      ADDTOPLAYLIST_MENU_MODIFIERS
    ),

    m_PlaylistCommands: new Array
    (
      null
    )
  },

  _getMenu: function(aSubMenu)
  {
    var cmds;
    
    cmds = this.m_addToPlaylist.handleGetMenu(aSubMenu);
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
    return cmds.m_Ids.length;
  },

  getCommandId: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length ) return "";
    return cmds.m_Ids[ aIndex ];
  },
  
  getCommandType: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Ids.length ) return "";
    return cmds.m_Types[ aIndex ];
  },

  getCommandText: function( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Names.length ) return "";
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
    if ( aIndex >= cmds.m_Tooltips.length ) return "";
    return cmds.m_Tooltips[ aIndex ];
  },

  getCommandValue: function( aSubMenu, aIndex, aHost )
  {
  },

  instantiateCustomCommand: function( aDocument, aId, aHost ) 
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
    return (this.m_Context.m_Playlist.tree.currentIndex != -1);
  },

  getCommandShortcutModifiers: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Modifiers.length ) return "";
    return cmds.m_Modifiers[ aIndex ];
  },

  getCommandShortcutKey: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keys.length ) return "";
    return cmds.m_Keys[ aIndex ];
  },

  getCommandShortcutKeycode: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_Keycodes.length ) return "";
    return cmds.m_Keycodes[ aIndex ];
  },

  getCommandShortcutLocal: function ( aSubMenu, aIndex, aHost )
  {
    return true;
  },

  getCommandSubObject: function ( aSubMenu, aIndex, aHost )
  {
    var cmds = this._getMenu(aSubMenu);
    if ( aIndex >= cmds.m_PlaylistCommands.length ) return null;
    return cmds.m_PlaylistCommands[ aIndex ];
  },

  onCommand: function( aSubMenu, aIndex, aHost, id, value )
  {
    if ( id )
    {
      // ADDTOPLAYLIST
      if (this.m_addToPlaylist.handleCommand(id)) return;
      
      // ...
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

  initCommands: function(aHost) { 
    this.m_addToPlaylist = new addToPlaylistHelper(); 
    this.m_addToPlaylist.init(this); 
  },
  
  shutdownCommands: function() { 
    if (!this.m_addToPlaylist) {
      dump("this.m_addToPlaylist is null in SBPlaylistCommand_AddToPlaylist ?!!\n");
      return;
    }
    this.m_addToPlaylist.shutdown(); 
    this.m_addToPlaylist = null;
  },
  
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
}; // SBPlaylistCommand_AddToPlaylist declaration


function addToPlaylistHelper() {
}

addToPlaylistHelper.prototype.constructor = addToPlaylistHelper;

addToPlaylistHelper.prototype = {
  m_listofplaylists: null,
  m_commands: null,
  m_reglist: null,

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },
  init: function(aCommands) {
    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                        .getService(Components.interfaces.sbILibraryManager);
    libraryManager.addListener(this);
    this.m_commands = aCommands;
    this.makeListOfPlaylists();
  },

  shutdown: function() {
    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                        .getService(Components.interfaces.sbILibraryManager);
    libraryManager.removeListener(this);
    this.removeLibraryListeners();
  },
  
  removeLibraryListeners: function() {
    if (this.m_reglist) {
      for (var i in this.m_reglist) {
        this.m_reglist[i].removeListener(this);
      }
    }
    this.m_reglist = new Array();
  },
    
  makeListOfPlaylists: function( ) {
    // remove previous listeners
    this.removeLibraryListeners();
    
    // todo: make this smarter :(
    var typearray = new Array('simple');

    this.m_listofplaylists = {};
    this.m_listofplaylists.m_Types = new Array();
    this.m_listofplaylists.m_Ids = new Array();
    this.m_listofplaylists.m_Names = new Array();
    this.m_listofplaylists.m_Tooltips = new Array();
    this.m_listofplaylists.m_Modifiers = new Array();
    this.m_listofplaylists.m_Keys = new Array();
    this.m_listofplaylists.m_Keycodes = new Array();
    this.m_listofplaylists.m_PlaylistCommands = new Array();
    
    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                        .getService(Components.interfaces.sbILibraryManager);
    
    var libs = libraryManager.getLibraries();
    while (libs.hasMoreElements()) {
      var library = libs.getNext();
      library.addListener(this, false);
      this.m_reglist.push(library);
      this.makePlaylistsForLibrary(library, typearray);
    }
    
    if (this.m_listofplaylists.m_Types.length == 0) {
      this.m_listofplaylists.m_Types.push("action");
      this.m_listofplaylists.m_Ids.push("noplaylist");
      this.m_listofplaylists.m_Names.push("&command.addtoplaylist.noexistingplaylist");
      this.m_listofplaylists.m_Tooltips.push("&command.tooltip.addtoplaylist.noexistingplaylist");
      this.m_listofplaylists.m_Modifiers.push("");
      this.m_listofplaylists.m_Keys.push("");
      this.m_listofplaylists.m_Keycodes.push("");
      this.m_listofplaylists.m_PlaylistCommands.push(null);
    }

    this.m_listofplaylists.m_Types.push("separator");
    this.m_listofplaylists.m_Ids.push("separator");
    this.m_listofplaylists.m_Names.push("separator");
    this.m_listofplaylists.m_Tooltips.push("separator");
    this.m_listofplaylists.m_Modifiers.push("");
    this.m_listofplaylists.m_Keys.push("");
    this.m_listofplaylists.m_Keycodes.push("");
    this.m_listofplaylists.m_PlaylistCommands.push(null);
    
    this.m_listofplaylists.m_Types.push("action");
    this.m_listofplaylists.m_Ids.push(ADDTOPLAYLIST_NEWPLAYLIST_COMMAND_ID);
    this.m_listofplaylists.m_Names.push("&command.addtoplaylist.createnew");
    this.m_listofplaylists.m_Tooltips.push("&command.addtoplaylist.createnew");
    this.m_listofplaylists.m_Modifiers.push("");
    this.m_listofplaylists.m_Keys.push("");
    this.m_listofplaylists.m_Keycodes.push("");
    this.m_listofplaylists.m_PlaylistCommands.push(null);
  },
  
    makePlaylistsForLibrary: function(aLibrary, typearray) {
    var listener = {
      obj: this,
      items: [],
      _downloadListGUID: null,
      onEnumerationBegin: function() {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefBranch2);
        this._downloadListGUID =
          prefs.getComplexValue("songbird.library.download",
                                Components.interfaces.nsISupportsString);
        return true;
      },
      onEnumerationEnd: function() {return true; },
      onEnumeratedItem: function(list, item) {
        var hidden = item.getProperty("http://songbirdnest.com/data/1.0#hidden");
        if (hidden == "1") return true;
        var goodtype = false;
        for (var i in typearray) {
          if (typearray[i] == item.type) { 
            goodtype = true; 
            break; 
          }
        }
        if (!goodtype) return true;

        // XXXsteve Prevent the download playlist from appearing in the list.
        // this should be fixable once we close bug 4017 and have a way to
        // interrogate the policy on each playlist to see if it should be
        // put in this menu
        if (item.guid == this._downloadListGUID) return true;

        this.obj.m_listofplaylists.m_Types.push("action");
        this.obj.m_listofplaylists.m_Ids.push(ADDTOPLAYLIST_COMMAND_ID + item.library.guid + ";" + item.guid);
        this.obj.m_listofplaylists.m_Names.push(item.name ? item.name : "Unnamed Playlist");
        this.obj.m_listofplaylists.m_Tooltips.push(item.name ? item.name : "Unnamed Playlist");
        this.obj.m_listofplaylists.m_Modifiers.push("");
        this.obj.m_listofplaylists.m_Keys.push("");
        this.obj.m_listofplaylists.m_Keycodes.push("");
        this.obj.m_listofplaylists.m_PlaylistCommands.push(null);
        return true;
      }
    };

    // Enumerate all lists in this library
    aLibrary.enumerateItemsByProperty("http://songbirdnest.com/data/1.0#isList", "1",
                                    listener,
                                    Components.interfaces.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  },

  handleGetMenu: function(aSubMenu) {
    if (this.m_listofplaylists == null) {
      // handleGetMenu called before makeListOfPlaylists, this would cause infinite recursion :
      // the command object would not find the menu either, would return null to getMenu which 
      // corresponds to the root menu, and it'd recurse infinitly.
      throw Components.results.NS_ERROR_FAILURE; 
    }
    if (aSubMenu == ADDTOPLAYLIST_MENU_ID) return this.m_listofplaylists;
    return null;
  },

  handleCommand: function(id) {
    try {
      var context = this.m_commands.m_Context;
      if (id == ADDTOPLAYLIST_NEWPLAYLIST_COMMAND_ID) {
        var newMediaList = context.m_Window.makeNewPlaylist("simple");
        this.addToPlaylist(newMediaList.library.guid, newMediaList.guid, context.m_Playlist);
        return true;
      }
      var addtoplstr = ADDTOPLAYLIST_COMMAND_ID;
      if ( id.slice(0, addtoplstr.length) == addtoplstr) {
        var r = id.slice(addtoplstr.length);
        var guids = r.split(';');
        if (guids.length >= 2) {
          var libraryguid = guids[0];
          var playlistguid = guids[1];

          this.addToPlaylist(libraryguid, playlistguid, context.m_Playlist);
          
          return true;
        }
      } 
    } catch (e) {
      alert("addToPlaylist.js - handleCommand - " + e);
    }
    return false;
  },
  
  addToPlaylist: function(libraryguid, playlistguid, sourceplaylist) {
    var libraryManager =
      Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                .getService(Components.interfaces.sbILibraryManager);
    var library = libraryManager.getLibrary(libraryguid);
    var medialist;
    if (libraryguid == playlistguid) medialist = library;
    else medialist = library.getMediaItem(playlistguid);
                
    if (medialist) {
    
      var oldLength = medialist.length;
      var selection = sourceplaylist.treeView.selectedMediaItems;
      // Create an enumerator that wraps the enumerator we were handed since
      // the enumerator we get hands back sbIIndexedMediaItem, not just plain
      // 'ol sbIMediaItems

      var unwrapper = {
        enumerator: selection,
        
        hasMoreElements : function() {
          return this.enumerator.hasMoreElements();
        },
        getNext : function() {
          return this.enumerator.getNext().mediaItem;
        },
        QueryInterface : function(iid) {
          if (iid.equals(Components.interfaces.nsISimpleEnumerator) ||
              iid.equals(Components.interfaces.nsISupports))
            return this;
          throw Components.results.NS_NOINTERFACE;
        }
      }

      medialist.addSome(unwrapper);
      var added = medialist.length - oldLength;
      sourceplaylist._reportAddedTracks(added, 0, medialist.name);
    }
  },
  
  //-----------------------------------------------------------------------------
  _batch         : new BatchHelper(),
  _deferredevent : false,
  
  refreshCommands: function() {
    if (this.m_commands) {
      if (this.m_commands.m_Context && this.m_commands.m_Context.m_Playlist) {
        this.makeListOfPlaylists();
        this.m_commands.m_Context.m_Playlist.refreshCommands();
      }
    }
  },

  onUpdateEvent: function(item) {
    if (item instanceof Components.interfaces.sbIMediaList) {
      if (this._batch.isActive()) {
        // if we are in a batch, remember that we saw a playlist event in it
        this._deferredevent = true;
      } else {
        // if we're not in a batch, proceed with refreshing the commands
        this.refreshCommands();
      }
    }
  },

  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Ci.sbIMediaListListener) &&
        !iid.equals(Ci.sbILibraryManagerListener) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },

  onItemAdded: function onItemAdded(list, item) {
    // If we are in a batch, ignore future notifications
    if (this._batch.isActive()) {
      return true;
    }
    this.onUpdateEvent(item);
  },
  
  onBeforeItemRemoved: function onBeforeItemRemoved(list, item) {
  },
  
  onAfterItemRemoved: function onAfterItemRemoved(list, item) {
    // If we are in a batch, ignore future notifications
    if (this._batch.isActive()) {
      return true;
    }
    this.onUpdateEvent(item);
  },
  
  onItemUpdated: function onItemUpdated(list, item, properties) {
    // If we are in a batch, ignore future notifications
    if (this._batch.isActive()) {
      return true;
    }
    this.onUpdateEvent(item);
  },

  onListCleared: function onListCleared(list) {
    // If we are in a batch, ignore future notifications
    if (this._batch.isActive()) {
      return true;
    }
    this.onUpdateEvent(list);
  },

  onBatchBegin: function onBatchBegin(list) {
    // start deferring the events
    this._batch.begin();
    this._deferredevent = false;
  },
  
  onBatchEnd: function onBatchEnd(list) {
    // stop deferring the events
    this._batch.end();
    // if an event was deferred, handle it
    if (!this._batch.isActive() && this._deferredevent) {
      // since we're no longer in a batch, this does call refreshCommands
      this.onUpdateEvent(list); 
    }
    this._deferredevent = false;
  },
  
  onLibraryRegistered: function onLibraryRegistered(library) {
    this.onUpdateEvent(library); 
  },

  onLibraryUnregistered: function onLibraryUnregistered(library) {
    this.onUpdateEvent(library); 
  }

};

