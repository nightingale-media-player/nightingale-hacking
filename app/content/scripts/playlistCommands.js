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

Components.utils.import("resource://app/components/ArrayConverter.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm")

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;

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

function onBrowserTransfer(mediaItems, parentWindow)
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
                  SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,centerscreen", download_data, parentWindow ); 
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
                  
                  //XXXAus: This can be changed to use SB_AddItems(mediaItems, mainLibrary, true)
                  //when bug #3271 is fixed.
                  downloadList.addSome(mediaItems);
                }
            }
        }
    }
    
    catch ( err )
    {
        alert( "onBrowserTransfer: " + err );
    }
}

// The house for the web playlist and download commands objects.

var SBWebPlaylistCommands = new PlaylistCommandsBuilder();
var SBDownloadCommands = new PlaylistCommandsBuilder();

function registerSpecialListCommands() {
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch2);

  var mgr = new sbIPlaylistCommandsManager();

  var downloadListGUID =
    prefs.getComplexValue("songbird.library.download",
                          Components.interfaces.nsISupportsString);
  var webListGUID =
    prefs.getComplexValue("songbird.library.web",
                          Components.interfaces.nsISupportsString);
  
  // Construct the web playlist commands
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_play",
                                               SBPlaylistCommand_Play);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_remove",
                                               SBPlaylistCommand_Remove);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_download",
                                               SBPlaylistCommand_Download);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_subscribe",
                                               SBPlaylistCommand_Subscribe);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_addtoplaylist",
                                               SBPlaylistCommand_AddToPlaylist);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_addtolibrary",
                                               SBPlaylistCommand_AddToLibrary);
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_copylocation",
                                               SBPlaylistCommand_CopyTrackLocation);
  SBWebPlaylistCommands.appendSeparator(null, "separator");
  SBWebPlaylistCommands.appendPlaylistCommands(null, 
                                               "library_cmdobj_showdlplaylist",
                                               SBPlaylistCommand_ShowDownloadPlaylist);

  mgr.publish(kSONGBIRD_PLAYLIST_COMMANDS_WEBPLAYLIST, SBWebPlaylistCommands);

  // Construct the download playlist commands
  SBDownloadCommands.appendPlaylistCommands(null, 
                                            "library_cmdobj_play",
                                            SBPlaylistCommand_Play);
  SBDownloadCommands.appendPlaylistCommands(null, 
                                            "library_cmdobj_remove",
                                            SBPlaylistCommand_Remove);
  SBDownloadCommands.appendPlaylistCommands(null, 
                                            "library_cmdobj_pauseresumedownload",
                                            SBPlaylistCommand_PauseResumeDownload);
  SBDownloadCommands.appendSeparator(null, "separator");
  SBDownloadCommands.appendPlaylistCommands(null, 
                                            "library_cmdobj_showwebplaylist",
                                            SBPlaylistCommand_ShowWebPlaylist);

  mgr.publish(kSONGBIRD_PLAYLIST_COMMANDS_DOWNLOADPLAYLIST, SBDownloadCommands);

  // Register the special lists commands
  mgr.registerPlaylistCommandsMediaItem(downloadListGUID, "",
                                          SBDownloadCommands);
  mgr.registerPlaylistCommandsMediaItem(webListGUID, 
                                        "",
                                        SBWebPlaylistCommands);
}

function unregisterSpecialListCommands() {
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch2);
  var downloadListGUID =
    prefs.getComplexValue("songbird.library.download",
                          Components.interfaces.nsISupportsString);
  var webListGUID =
    prefs.getComplexValue("songbird.library.web",
                          Components.interfaces.nsISupportsString);

  var mgr = new sbIPlaylistCommandsManager();

  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMANDS_WEBPLAYLIST, SBWebPlaylistCommands);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMANDS_DOWNLOADPLAYLIST, SBDownloadCommands);

  mgr.unregisterPlaylistCommandsMediaItem(downloadListGUID, "",
                                            SBDownloadCommands);
  mgr.unregisterPlaylistCommandsMediaItem(webListGUID, "",
                                            SBWebPlaylistCommands); 
}

// Define various playlist commands, they will be exposed to the playlist commands
// manager so that they can later be retrieved and concatenated into bigger
// playlist commands objects.

// play the selected track
var SBPlaylistCommand_Play;
// remove the selected track(s)
var SBPlaylistCommand_Remove;
// download the selected track(s)
var SBPlaylistCommand_Download;
// subscribe to the currently displayed site
var SBPlaylistCommand_Subscribe;
// add the selection to the library
var SBPlaylistCommand_AddToLibrary;
// copy the select track(s) location(s)
var SBPlaylistCommand_CopyTrackLocation;
// switch the outer playlist contaner to the download playlist
var SBPlaylistCommand_ShowDownloadPlaylist;
// switch the outer playlist contaner to the web playlist
var SBPlaylistCommand_ShowWebPlaylist;
// autoswitching pause/resume track download
var SBPlaylistCommand_PauseResumeDownload;


// Called when the play action is triggered
function plCmd_Play_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // if something is selected, trigger the play event on the playlist
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    // Repurpose the command to act as a doubleclick
    aContext.m_Playlist.sendPlayEvent();
  }
}

// Called when the remove action is triggered
function plCmd_Remove_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // remove the selected tracks, if any
  if (plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost)) {
    aContext.m_Playlist.removeSelectedTracks();
  }
}

// Called when the download action is triggered
function plCmd_Download_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  try
  {
    if(aContext.m_Playlist.treeView.selectionCount) {
      onBrowserTransfer(new SelectionUnwrapper
                      (aContext.m_Playlist.treeView.selectedMediaItems), aContext.m_Window);
    }
    else {
      var allItems = {
        items: [],
        onEnumerationBegin: function(aMediaList) {
          return true;
        },
        onEnumeratedItem: function(aMediaList, aMediaItem) {
          this.items.push(aMediaItem);
          return true;
        },
        onEnumerationEnd: function(aMediaList, aResultCode) {
          return;
        }
      };

      aContext.m_Playlist.mediaList.enumerateAllItems(allItems, 
        Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
      
      var itemEnum = ArrayConverter.enumerator(allItems.items);
      onBrowserTransfer(itemEnum, aContext.m_Window);
    }
    
    // And show the download table in the chrome playlist if possible.
    var browser = aContext.m_Window.gBrowser;
    if (browser) browser.mCurrentTab.switchToDownloadView();
  }
  catch( err )          
  {
    alert( "plCmd_Download_TriggerCallback Error:" + err );
  }
}

// Called when the subscribe action is triggered
function plCmd_Subscribe_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  // Bring up the subscribe dialog with the web playlist url
  // XXX this should instead extract the url from an #originPage property on the list
  var browser = aContext.m_Window.gBrowser;
  if (browser) {
    SBSubscribe(null, browser.currentURI, aContext.m_Window); 
  }
}

// Called when the "add to library" action is triggered
function plCmd_AddToLibrary_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var libraryManager =
    Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
              .getService(Components.interfaces.sbILibraryManager);
  var mediaList = libraryManager.mainLibrary;
  
  var treeView = aContext.m_Playlist.treeView;
  var selectionCount = treeView.selectionCount;
  
  var unwrapper = new SelectionUnwrapper(treeView.selectedMediaItems);
  
  var oldLength = mediaList.length;
  mediaList.addSome(unwrapper);

  var itemsAdded = mediaList.length - oldLength;
  aContext.m_Playlist._reportAddedTracks(itemsAdded,
                                         selectionCount - itemsAdded, 
                                         mediaList.name); 
}

// Called when the "copy track location" action is triggered
function plCmd_CopyTrackLocation_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var clipboardtext = "";
  var urlCol = "url";
  var columnElem = aContext.m_Window.
                   document.getAnonymousElementByAttribute(aContext.m_Playlist,
                                                           "bind",
                                                           SBProperties.contentURL);
  var columnObj = aContext.m_Playlist.tree.columns.getColumnFor(columnElem);
  var rangeCount = aContext.m_Playlist.mediaListView.treeView.selection.getRangeCount();
  for (var i=0; i < rangeCount; i++) 
  {
    var start = {};
    var end = {};
    aContext.m_Playlist.mediaListView.treeView.selection.getRangeAt( i, start, end );
    for( var c = start.value; c <= end.value; c++ )
    {
      if (c >= aContext.m_Playlist.mediaListView.treeView.rowCount) 
      {
        continue; 
      }
      
      var val = aContext.m_Playlist.mediaListView.treeView.getCellText(c, columnObj);
      if (clipboardtext != "") clipboardtext += "\n";
      clipboardtext += val;
    }
  }

  var clipboard = Components.classes["@mozilla.org/widget/clipboardhelper;1"]
                            .getService(Components.interfaces.nsIClipboardHelper);
  clipboard.copyString(clipboardtext);
}

// Called whne the "show download playlist" action is triggered
function plCmd_ShowDownloadPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var browser = aContext.m_Window.gBrowser;
  if (browser) {
    if (aContext.m_Window.location.pathname ==
        '/content/xul/sb-library-page.xul') {
      // we're in the web library / inner playlist view
      browser.loadMediaList(browser.downloadList);
    } else {
      // we're in a web playlist / outer playlist view
      browser.mCurrentTab.switchToDownloadView();
    } 
  }
}

// Called whne the "show web playlist" action is triggered
function plCmd_ShowWebPlaylist_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var browser = aContext.m_Window.gBrowser;
  if (browser) {
    if (aContext.m_Window.location.pathname ==
        '/content/xul/sb-library-page.xul') {
      // we're in the download library / inner playlist view
      browser.loadMediaList(browser.webLibrary);
    } else {
      // we're in a web playlist / outer playlist view
      browser.mCurrentTab.switchToWebPlaylistView();
    }
  }
}

// Called when the pause/resume download action is triggered
function plCmd_PauseResumeDownload_TriggerCallback(aContext, aSubMenuId, aCommandId, aHost) {
  var device = aContext.m_Commands.getProperty("m_Device");
  var deviceState = device.getDeviceState('');
  if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING )
  {
    device.suspendTransfer('');
  }
  else if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
  {
    device.resumeTransfer('');
  }
  // Since we changed state, update the command buttons.
  aContext.m_Playlist.refreshCommands(); 
}

// Returns true when at least one track is selected in the playlist
function plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) {
  return ( aContext.m_Playlist.tree.currentIndex != -1 );
}

// Returns true if the host is the shortcuts instantiator
function plCmd_IsShortcutsInstantiator(aContext, aSubMenuId, aCommandId, aHost) {
  return (aHost == "shortcuts");
}

// Returns true if the current playlist isn't the library
function plCmd_IsNotLibraryContext(aContext, aSubMenuId, aCommandId, aHost) {
  return (aContext.m_Playlist.description != "library" );
}

// Returns true if the conditions are ok for subscribing to a page
function plCmd_CanSubscribe(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsNotLibraryContext(aContext, aSubMenuId, aCommandId, aHost) &&
         plCmd_ContextHasBrowser(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if the conditions are ok for adding tracks to the library
function plCmd_CanAddToLibrary(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsAnyTrackSelected(aContext, aSubMenuId, aCommandId, aHost) &&
         plCmd_IsNotLibraryContext(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if a download is currently in progress (not paused)
function plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) {
  var device = aContext.m_Commands.getProperty("m_Device");
  var deviceState = device.getDeviceState('');
  return (deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING);
}

// Returns true if a download is currently paused
function plCmd_IsDownloadPaused(aContext, aSubMenuId, aCommandId, aHost) {
  var device = aContext.m_Commands.getProperty("m_Device");
  var deviceState = device.getDeviceState('');
  return (deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED);
}

// Returns true if a download has been started, regardless of wether it has
// been paused or not
function plCmd_IsDownloadActive(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) ||
         plCmd_IsDownloadPaused(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if a download is in progress or if there is not download that
// has been started at all
function plCmd_IsDownloadingOrNotActive(aContext, aSubMenuId, aCommandId, aHost) {
  return plCmd_IsDownloading(aContext, aSubMenuId, aCommandId, aHost) ||
         !plCmd_IsDownloadActive(aContext, aSubMenuId, aCommandId, aHost);
}

// Returns true if the supplied context contains a gBrowser object
function plCmd_ContextHasBrowser(aContext, aSubMenuId, aCommandId, aHost) {
  return (aContext.m_Window.gBrowser);
}

// ----------------------------------------------------------------------------
// INIT
// ----------------------------------------------------------------------------

function initPlaylistCommands() {
  window.removeEventListener("load", initPlaylistCommands, false);
  
  // Build playlist command actions

  // --------------------------------------------------------------------------
  // The PLAY button is made of two commands, one that is always there for
  // all instantiators, the other that is only created by the shortcuts
  // instantiator. This makes one toolbar button and menu item, and two
  // shortcut keys.
  // --------------------------------------------------------------------------
  
  SBPlaylistCommand_Play = new PlaylistCommandsBuilder();

  // The first item, always created
  SBPlaylistCommand_Play.appendAction(null, 
                                      "library_cmd_play",
                                      "&command.play",
                                      "&command.tooltip.play",
                                      plCmd_Play_TriggerCallback);
  
  SBPlaylistCommand_Play.setCommandShortcut(null,
                                            "library_cmd_play",
                                            "&command.shortcut.key.play",
                                            "&command.shortcut.keycode.play",
                                            "&command.shortcut.modifiers.play",
                                            true);

  SBPlaylistCommand_Play.setCommandEnabledCallback(null,
                                                   "library_cmd_play",
                                                   plCmd_IsAnyTrackSelected);

  // The second item, only created by the keyboard shortcuts instantiator
  SBPlaylistCommand_Play.appendAction(null, 
                                      "library_cmd_play2",
                                      "&command.play",
                                      "&command.tooltip.play",
                                      plCmd_Play_TriggerCallback);

  SBPlaylistCommand_Play.setCommandShortcut(null,
                                            "library_cmd_play2",
                                            "&command.shortcut.key.altplay",
                                            "&command.shortcut.keycode.altplay",
                                            "&command.shortcut.modifiers.altplay",
                                            true);

  SBPlaylistCommand_Play.setCommandEnabledCallback(null,
                                                   "library_cmd_play2",
                                                   plCmd_IsAnyTrackSelected);

  SBPlaylistCommand_Play.setCommandVisibleCallback(null,
                                                   "library_cmd_play2",
                                                   plCmd_IsShortcutsInstantiator);

  // --------------------------------------------------------------------------
  // The REMOVE button, like the play button, is made of two commands, one that 
  // is always there for all instantiators, the other that is only created by 
  // the shortcuts instantiator. This makes one toolbar button and menu item, 
  // and two shortcut keys.
  // --------------------------------------------------------------------------

  SBPlaylistCommand_Remove = new PlaylistCommandsBuilder();

  // The first item, always created
  SBPlaylistCommand_Remove.appendAction(null, 
                                        "library_cmd_remove",
                                        "&command.remove",
                                        "&command.tooltip.remove",
                                        plCmd_Remove_TriggerCallback);
  
  SBPlaylistCommand_Remove.setCommandShortcut(null,
                                              "library_cmd_remove",
                                              "&command.shortcut.key.remove",
                                              "&command.shortcut.keycode.remove",
                                              "&command.shortcut.modifiers.remove",
                                              true);

  SBPlaylistCommand_Remove.setCommandEnabledCallback(null,
                                                   "library_cmd_remove",
                                                   plCmd_IsAnyTrackSelected);

  // The second item, only created by the keyboard shortcuts instantiator
  SBPlaylistCommand_Remove.appendAction(null, 
                                      "library_cmd_remove2",
                                      "&command.remove",
                                      "&command.tooltip.remove",
                                      plCmd_Remove_TriggerCallback);

  SBPlaylistCommand_Remove.setCommandShortcut(null,
                                            "library_cmd_remove2",
                                            "&command.shortcut.key.altremove",
                                            "&command.shortcut.keycode.altremove",
                                            "&command.shortcut.modifiers.altremove",
                                            true);

  SBPlaylistCommand_Remove.setCommandEnabledCallback(null,
                                                   "library_cmd_remove2",
                                                   plCmd_IsAnyTrackSelected);

  SBPlaylistCommand_Remove.setCommandVisibleCallback(null,
                                                   "library_cmd_remove2",
                                                   plCmd_IsShortcutsInstantiator);
  
  // --------------------------------------------------------------------------
  // The DOWNLOAD button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_Download = new PlaylistCommandsBuilder();

  SBPlaylistCommand_Download.appendAction(null, 
                                        "library_cmd_download",
                                        "&command.download",
                                        "&command.tooltip.download",
                                        plCmd_Download_TriggerCallback);
  
  SBPlaylistCommand_Download.setCommandShortcut(null,
                                              "library_cmd_download",
                                              "&command.shortcut.key.download",
                                              "&command.shortcut.keycode.download",
                                              "&command.shortcut.modifiers.download",
                                              true);

  SBPlaylistCommand_Download.setCommandEnabledCallback(null,
                                                   "library_cmd_download",
                                                   plCmd_IsAnyTrackSelected);
  
  // --------------------------------------------------------------------------
  // The SUBSCRIBE button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_Subscribe = new PlaylistCommandsBuilder();

  SBPlaylistCommand_Subscribe.appendAction(null, 
                                           "library_cmd_subscribe",
                                           "&command.subscribe",
                                           "&command.tooltip.subscribe",
                                           plCmd_Subscribe_TriggerCallback);

  SBPlaylistCommand_Subscribe.setCommandShortcut(null,
                                                 "library_cmd_subscribe",
                                                 "&command.shortcut.key.subscribe",
                                                 "&command.shortcut.keycode.subscribe",
                                                 "&command.shortcut.modifiers.subscribe",
                                                 true);

  SBPlaylistCommand_Subscribe.setCommandEnabledCallback(null,
                                                   "library_cmd_subscribe",
                                                   plCmd_CanSubscribe);

  // --------------------------------------------------------------------------
  // The ADDTOLIBRARY button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_AddToLibrary = new PlaylistCommandsBuilder();

  SBPlaylistCommand_AddToLibrary.appendAction(null, 
                                              "library_cmd_addtolibrary",
                                              "&command.addtolibrary",
                                              "&command.tooltip.addtolibrary",
                                              plCmd_AddToLibrary_TriggerCallback);

  SBPlaylistCommand_AddToLibrary.setCommandShortcut(null,
                                                    "library_cmd_addtolibrary",
                                                    "&command.shortcut.key.addtolibrary",
                                                    "&command.shortcut.keycode.addtolibrary",
                                                    "&command.shortcut.modifiers.addtolibrary",
                                                    true);

  SBPlaylistCommand_AddToLibrary.setCommandEnabledCallback(null,
                                                           "library_cmd_addtolibrary",
                                                           plCmd_CanAddToLibrary);

  // --------------------------------------------------------------------------
  // The COPY TRACK LOCATION button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_CopyTrackLocation = new PlaylistCommandsBuilder();

  SBPlaylistCommand_CopyTrackLocation.appendAction(null, 
                                              "library_cmd_copylocation",
                                              "&command.copylocation",
                                              "&command.tooltip.copylocation",
                                              plCmd_CopyTrackLocation_TriggerCallback);

  SBPlaylistCommand_CopyTrackLocation.setCommandShortcut(null,
                                                    "library_cmd_copylocation",
                                                    "&command.shortcut.key.copylocation",
                                                    "&command.shortcut.keycode.copylocation",
                                                    "&command.shortcut.modifiers.copylocation",
                                                    true);

  SBPlaylistCommand_CopyTrackLocation.setCommandEnabledCallback(null,
                                                                "library_cmd_copylocation",
                                                                plCmd_IsAnyTrackSelected);

  // --------------------------------------------------------------------------
  // The SHOW DOWNLOAD PLAYLIST button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_ShowDownloadPlaylist = new PlaylistCommandsBuilder();

  SBPlaylistCommand_ShowDownloadPlaylist.appendAction(null, 
                                              "library_cmd_showdlplaylist",
                                              "&command.showdlplaylist",
                                              "&command.tooltip.showdlplaylist",
                                              plCmd_ShowDownloadPlaylist_TriggerCallback);

  SBPlaylistCommand_ShowDownloadPlaylist.setCommandShortcut(null,
                                                    "library_cmd_showdlplaylist",
                                                    "&command.shortcut.key.showdlplaylist",
                                                    "&command.shortcut.keycode.showdlplaylist",
                                                    "&command.shortcut.modifiers.showdlplaylist",
                                                    true);

  SBPlaylistCommand_ShowDownloadPlaylist.setCommandVisibleCallback(null,
                                                                   "library_cmd_showdlplaylist",
                                                                   plCmd_ContextHasBrowser);

  // --------------------------------------------------------------------------
  // The PAUSE/RESUME DOWNLOAD button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_PauseResumeDownload = new PlaylistCommandsBuilder();

  SBPlaylistCommand_PauseResumeDownload.appendAction(null, 
                                              "library_cmd_pause",
                                              "&command.pausedl",
                                              "&command.tooltip.pause",
                                              plCmd_PauseResumeDownload_TriggerCallback);

  SBPlaylistCommand_PauseResumeDownload.setCommandShortcut(null,
                                                    "library_cmd_pause",
                                                    "&command.shortcut.key.pause",
                                                    "&command.shortcut.keycode.pause",
                                                    "&command.shortcut.modifiers.pause",
                                                    true);

  SBPlaylistCommand_PauseResumeDownload.setCommandVisibleCallback(null,
                                                   "library_cmd_pause",
                                                   plCmd_IsDownloadingOrNotActive);

  SBPlaylistCommand_PauseResumeDownload.setCommandEnabledCallback(null,
                                                   "library_cmd_pause",
                                                   plCmd_IsDownloadActive);

  SBPlaylistCommand_PauseResumeDownload.appendAction(null, 
                                              "library_cmd_resume",
                                              "&command.resumedl",
                                              "&command.tooltip.resume",
                                              plCmd_PauseResumeDownload_TriggerCallback);

  SBPlaylistCommand_PauseResumeDownload.setCommandShortcut(null,
                                                    "library_cmd_resume",
                                                    "&command.shortcut.key.resume",
                                                    "&command.shortcut.keycode.resume",
                                                    "&command.shortcut.modifiers.resume",
                                                    true);

  SBPlaylistCommand_PauseResumeDownload.setCommandVisibleCallback(null,
                                                   "library_cmd_resume",
                                                   plCmd_IsDownloadPaused);

  // --------------------------------------------------------------------------
  // The SHOW WEB PLAYLIST button
  // --------------------------------------------------------------------------

  SBPlaylistCommand_ShowWebPlaylist = new PlaylistCommandsBuilder();

  SBPlaylistCommand_ShowWebPlaylist.appendAction(null, 
                                                 "library_cmd_showwebplaylist",
                                                 "&command.showwebplaylist",
                                                 "&command.tooltip.showwebplaylist",
                                                plCmd_ShowWebPlaylist_TriggerCallback);

  SBPlaylistCommand_ShowWebPlaylist.setCommandShortcut(null,
                                                    "library_cmd_showwebplaylist",
                                                    "&command.shortcut.key.showwebplaylist",
                                                    "&command.shortcut.keycode.showwebplaylist",
                                                    "&command.shortcut.modifiers.showwebplaylist",
                                                    true);

  SBPlaylistCommand_ShowWebPlaylist.setCommandVisibleCallback(null,
                                                              "library_cmd_showwebplaylist",
                                                              plCmd_ContextHasBrowser);

  // --------------------------------------------------------------------------
  
  // Expose playlist commands

  var mgr = new sbIPlaylistCommandsManager();
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_PLAY, SBPlaylistCommand_Play);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_REMOVE, SBPlaylistCommand_Remove);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_DOWNLOAD, SBPlaylistCommand_Download);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_SUBSCRIBE, SBPlaylistCommand_Subscribe);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_ADDTOLIBRARY, SBPlaylistCommand_AddToLibrary);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_COPYTRACKLOCATION, SBPlaylistCommand_CopyTrackLocation);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_SHOWDOWNLOADPLAYLIST, SBPlaylistCommand_ShowDownloadPlaylist);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_SHOWWEBPLAYLIST, SBPlaylistCommand_ShowWebPlaylist);
  mgr.publish(kSONGBIRD_PLAYLIST_COMMAND_PAUSERESUMEDOWNLOAD, SBPlaylistCommand_PauseResumeDownload);

  registerCommands();
  registerSpecialListCommands();
  window.addEventListener("unload", shutdownPlaylistCommands, false);
}

// ----------------------------------------------------------------------------
// SHUTDOWN
// ----------------------------------------------------------------------------

function shutdownPlaylistCommands() {
  window.removeEventListener("unload", shutdownPlaylistCommands, false);

  // Un-expose playlist commands
  var mgr = new sbIPlaylistCommandsManager();
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_PLAY, SBPlaylistCommand_Play);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_REMOVE, SBPlaylistCommand_Remove);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_DOWNLOAD, SBPlaylistCommand_Download);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_SUBSCRIBE, SBPlaylistCommand_Subscribe);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_ADDTOLIBRARY, SBPlaylistCommand_AddToLibrary);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_ADDTOPLAYLIST, SBPlaylistCommand_AddToPlaylist);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_COPYTRACKLOCATION, SBPlaylistCommand_CopyTrackLocation);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_SHOWDOWNLOADPLAYLIST, SBPlaylistCommand_ShowDownloadPlaylist);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_SHOWWEBPLAYLIST, SBPlaylistCommand_ShowWebPlaylist);
  mgr.withdraw(kSONGBIRD_PLAYLIST_COMMAND_PAUSERESUMEDOWNLOAD, SBPlaylistCommand_PauseResumeDownload);

  unregisterCommands();
  unregisterSpecialListCommands();
}


window.addEventListener("load", initPlaylistCommands, false);

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
      SBDownloadCommands.setProperty("m_Device", downloadDevice);
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
  
  m_PlaylistCommands: new Array
  (
    null,
    null,
    null,
    null,
    null,
    null,
    null,
    null
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

  instantiateCustomCommand: function( aDocument, aId, aHost ) 
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
    return playlist.mediaListView.treeView.selection.getRangeCount() > 0;
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

  getCommandSubObject: function ( aSubMenu, aIndex, aHost )
  {
    return null;
  },

  onCommand: function( aSubMenu, aIndex, aHost, id, value )
  {
    if ( id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( aHost == "toolbar" );
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
            if ( tbb || this.m_Context.m_Playlist.mediaListView.treeView.selection.count > 1 )
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

  m_PlaylistCommands: new Array
  (
    null,
    null
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

  getCommandSubObject: function ( aSubMenu, aIndex, aHost )
  {
    return null;
  },

  onCommand: function( aSubMenu, aIndex, aHost, id, value )
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

function registerCommands() {

  var mgr = new sbIPlaylistCommandsManager();
  mgr.registerPlaylistCommandsMediaList( "", "simple", SBDefaultServiceCommands );
  mgr.registerPlaylistCommandsMediaList( "", "smart", SBDefaultServiceCommands );

}

function unregisterCommands() {

  var mgr = new sbIPlaylistCommandsManager();
  mgr.unregisterPlaylistCommandsMediaList("",
                                          "simple",
                                          SBDefaultServiceCommands);
  mgr.unregisterPlaylistCommandsMediaList("",
                                          "smart",
                                          SBDefaultServiceCommands); 
}
