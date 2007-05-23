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

const URL_ADDTOPLAYLIST_DIALOG =
  "chrome://songbird/content/xul/add_to_playlist.xul";

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
  
  onItemUpdated: function onItemUpdated(list, item) {
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

function getPlaylistGUIDFromDialog() {
  // Make a data object to get the playlist to add to from the dialog.
  var dialogData = {};

  // Open the modal dialog.
  SBOpenModalDialog(URL_ADDTOPLAYLIST_DIALOG, "add_to_playlist",
                    "chrome,centerscreen", dialogData);

  return dialogData.mediaListGUID;
}

// The house for the web playlist and download commands objects.
var SBWebPlaylistCommands = 
{
  m_Playlist: null,
  
  m_Types: new Array
  (
    "action",
    "action",
    "action",
    "action",
    "action",
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
    "library_cmd_remove",
    "library_cmd_download",
    "library_cmd_subscribe",
    "library_cmd_addtoplaylist",
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
    "&command.remove",
    "&command.download",
    "&command.subscribe",
    "&command.addtoplaylist",
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
    "&command.tooltip.remove",
    "&command.tooltip.download",
    "&command.tooltip.subscribe",
    "&command.tooltip.addtoplaylist",
    "&command.tooltip.addtolibrary",
    "&command.tooltip.copylocation",
    "*separator*",
    "&command.tooltip.showdlplaylist"
//    "&command.tooltip.burntocd"
//    "&command.tooltip.device"
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

  getCommandId: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ aIndex ];
  },

  getCommandType: function( aSubmenu, aIndex, aHost )
  {
    if ( aIndex >= this.m_Types.length )
    {
      return "";
    }
    return this.m_Types[ aIndex ];
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
    if (aIndex == 6) return (aHost != "toolbar");
    return true;
  },
  
  getCommandEnabled: function( aSubmenu, aIndex, aHost )
  {
    var retval = false;
    switch ( this.m_Ids[aIndex] )
    {
      case "library_cmd_device": // Not yet implemented
        retval = false;
      break;
      case "library_cmd_remove":
        retval = this.m_Playlist.tree.view.selection.getRangeCount() > 0;
      break;      
      case "library_cmd_download": // Only download selected tracks from the library, never the whole library
        if ( ( this.m_Playlist.description != "library" ) || ( this.m_Playlist.tree.view.selection.count > 0 ) )
          retval = true;
      break;
      case "library_cmd_subscribe": // Never subscribe to the library
        if ( this.m_Playlist.description != "library" )
          retval = true;
      break;
      case "library_cmd_showdlplaylist":
        retval = true;
      break;
      case "library_cmd_copylocation":
        retval = this.m_Playlist.tree.view.selection.getRangeCount() > 0;
      break; 
      default:
        retval = true;
      break;
    }
    return retval;
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
          // If the user hasn't selected anything, select the first thing for him.
          if ( this.m_Playlist.tree.currentIndex == -1 )
          {
            this.m_Playlist.tree.view.selection.currentIndex = 0;
            this.m_Playlist.tree.view.selection.select( 0 );
          }
          // Repurpose the command to act as if a doubleclick
          this.m_Playlist.sendPlayEvent();
        break;
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeSelectedTracks();
          }
        break;
        case "library_cmd_download":
        {
          try
          {        
            onBrowserTransfer(new SelectionUnwrapper
                                (this.m_Playlist.treeView.selectedMediaItems));
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
          var columnObj = this.m_Playlist.tree.columns.getNamedColumn(urlCol);
          var rangeCount = this.m_Playlist.tree.view.selection.getRangeCount();
          for (var i=0; i < rangeCount; i++) 
          {
            var start = {};
            var end = {};
            this.m_Playlist.tree.view.selection.getRangeAt( i, start, end );
            for( var c = start.value; c <= end.value; c++ )
            {
              if (c >= this.m_Playlist.tree.view.rowCount) 
              {
                continue; 
              }
              
              var val = this.m_Playlist.tree.view.getCellText(c, columnObj);
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
          var url = window.location.href;
          var readable_name = unescape(url);
          SBSubscribe(url, this.m_Playlist.mediaListView, readable_name);
        }
        break;
        case "library_cmd_addtoplaylist":
        {
          var mediaListGUID = getPlaylistGUIDFromDialog();
          if (!mediaListGUID) {
            // User clicked Cancel
            return;
          }

          var libraryManager =
            Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                      .getService(Components.interfaces.sbILibraryManager);
          var mediaList = libraryManager.mainLibrary.getMediaItem(mediaListGUID);

          var treeView = this.m_Playlist.treeView;
          var selectionCount = treeView.selectionCount;
          
          var unwrapper = new SelectionUnwrapper(treeView.selectedMediaItems);

          // Add all the items that are selected.
          var libraryManager =
            Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                      .getService(Components.interfaces.sbILibraryManager);
          var mediaList = libraryManager.mainLibrary.getMediaItem(mediaListGUID);

          var mediaListListener = new MediaListListener();
          
          mediaList.addListener(mediaListListener);
          mediaList.addSome(unwrapper);
          mediaList.removeListener(mediaListListener);
          
          var itemsAdded = mediaListListener.itemAddedCount;
          this.m_Playlist._reportAddedTracks(itemsAdded,
                                             selectionCount - itemsAdded);
        }
        break;
        case "library_cmd_addtolibrary":
        {
          var libraryManager =
            Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                      .getService(Components.interfaces.sbILibraryManager);
          var mediaList = libraryManager.mainLibrary;
          
          var treeView = this.m_Playlist.treeView;
          var selectionCount = treeView.selectionCount;
          
          var unwrapper = new SelectionUnwrapper(treeView.selectedMediaItems);
          
          var mediaListListener = new MediaListListener();

          mediaList.addListener(mediaListListener);
          mediaList.addSome(unwrapper);
          mediaList.removeListener(mediaListListener);

          var itemsAdded = mediaListListener.itemAddedCount;
          this.m_Playlist._reportAddedTracks(itemsAdded,
                                             selectionCount - itemsAdded);
        }
        break;
        case "library_cmd_showdlplaylist":
        {
          gBrowser.mCurrentTab.switchToDownloadView();
        }
        break;
      }
    }
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
}; // SBWebPlaylistCommands declaration

/*

// Register the web playlist commands at startup
if ( ( WEB_PLAYLIST_CONTEXT != "" ) && ( WEB_PLAYLIST_TABLE != "" ) )
{
  var source = new sbIPlaylistsource();
  source.registerPlaylistCommands( WEB_PLAYLIST_CONTEXT, WEB_PLAYLIST_TABLE, "http", SBWebPlaylistCommands );
  source.registerPlaylistCommands( WEB_PLAYLIST_CONTEXT, "library", "http", SBWebPlaylistCommands );
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

/*
var theDownloadListener = 
{
  m_queryObj: null,
  m_libraryObj: null,
  
  CreateQueryObj: function()
  {
    this.m_queryObj = new sbIDatabaseQuery();
    this.m_queryObj.setAsyncQuery(true);
    this.m_queryObj.setDatabaseGUID("songbird");
  },

  CreateLibraryObj: function()
  {
    if(this.m_libraryObj == null)
    {
      const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
      this.m_libraryObj = (new MediaLibrary()).QueryInterface(Components.interfaces.sbIMediaLibrary);
    
      if(this.m_queryObj == null)
          this.CreateQueryObj();
        
      this.m_libraryObj.setQueryObject(this.m_queryObj);
    }
  },
  
  QueryInterface : function(aIID) 
  {
    if (!aIID.equals(Components.interfaces.sbIDeviceBaseCallback) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  
  onTransferStart: function(sourceURL, destinationURL)
  {
  },
  
  onTransferComplete: function(sourceURL, destinationURL, transferStatus)
  {
    if(transferStatus == 1)
    {
      this.CreateLibraryObj(); 
      
      var aKeys = ["title"];
      var aValues = [];
      
      var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      aLocalFile.initWithPath(destinationURL);
    
      aValues.push(aLocalFile.leafName);
      this.m_libraryObj.addMedia(destinationURL, aKeys.length, aKeys, aValues.length, aValues, false, false);
    }
  }
};
*/

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
                  var downloadLibrary = downloadDevice.getLibrary("download");
                  while (mediaItems.hasMoreElements())
                  {
                      downloadLibrary.add(mediaItems.getNext());
                  }
                }
            }
        }
    }
    
    catch ( err )
    {
        alert( err );
    }
}

var SBDownloadCommands = 
{
  m_Playlist: null,
  m_Device: null,

  // XXXben stupid hack until getDeviceState is implemented on the new download
  //        device.
  getDeviceState: function getDeviceState(deviceID) {
    var deviceState;
    try {
      deviceState = this.m_Device.getDeviceState(deviceID);
    }
    catch (e) {
      if (e.result == Cr.NS_ERROR_NOT_IMPLEMENTED) {
        deviceState = Ci.sbIDeviceBase.STATE_DOWNLOADING;
      }
      else {
        throw e;
      }
    }
    return deviceState;
  },
  
  m_Types: new Array
  (
    "action",
    "action",
    "action",
    "separator",
    "action"
  ),
  
  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_remove",
    "library_cmd_pause",
    "*separator*",
    "library_cmd_showwebplaylist"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.remove",
    "&command.pausedl",
    "*separator*",
    "&command.showwebplaylist"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.remove",
    "&command.tooltip.pause",
    "*separator*",
    "&command.tooltip.showwebplaylist"
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
    // Ah! magic number - what does it mean???
    if ( aIndex == 2 ) 
    {
      if ( this.m_Device )
      {
        if ( this.getDeviceState('') == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
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
    if ( aIndex == 2 )
    {
      if ( this.m_Device )
      {
        if ( this.getDeviceState('') == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
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
    if ( aIndex == 2 )
    {
      if ( this.m_Device )
      {
        var deviceState = this.getDeviceState('');
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
    return true;
  },

  getCommandEnabled: function( aSubmenu, aIndex, aHost )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( aIndex )
      {
        case 2:
          var deviceState = this.getDeviceState('');
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

  onCommand: function( id, value, host )
  {
    if ( this.m_Device && id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( host == "toolbar" );
      switch( id )
      {
        case "library_cmd_play":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // If the user hasn't selected anything, select the first thing for him.
            if ( this.m_Playlist.tree.currentIndex == -1 )
            {
              this.m_Playlist.tree.view.selection.currentIndex = 0;
              this.m_Playlist.tree.view.selection.select( 0 );
            }
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeSelectedTracks();
          }
        break;
        case "library_cmd_pause":
        case "library_cmd_resume":
          var deviceState = this.getDeviceState('');
          if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOADING )
          {
            this.m_Device.suspendTransfer('');
          }
          else if ( deviceState == Ci.sbIDeviceBase.STATE_DOWNLOAD_PAUSED )
          {
            this.m_Device.resumeTransfer('');
          }
          // Since we changed state, update the command buttons.
          this.m_Playlist.refreshCommands();
        break;
        case "library_cmd_showwebplaylist":
        {
          gBrowser.mCurrentTab.switchToWebPlaylistView();
        }
        break;
      }
    }
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

}; // SBDownloadCommands definition

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
  m_Playlist: null,

  m_Types: new Array
  (
    "action",
    "action",
    "action",
    "action",
    "action",
    "action"
  ),
  
  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_remove",
    "library_cmd_edit",
    "library_cmd_addtoplaylist",
    "library_cmd_burntocd",
    "library_cmd_device"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.remove",
    "&command.edit",
    "&command.addtoplaylist",
    "&command.burntocd",
    "&command.device"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.remove",
    "&command.tooltip.edit",
    "&command.tooltip.addtoplaylist",
    "&command.tooltip.burntocd",
    "&command.tooltip.device"
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
    var playlist = this.m_Playlist;
    
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

  onCommand: function( id, value, host )
  {
    if ( id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( host == "toolbar" );
      switch( id )
      {
        case "library_cmd_play":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_edit":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            if ( tbb || this.m_Playlist.tree.view.selection.count > 1 )
            {
              // Edit the entire track
              this.m_Playlist.sendEditorEvent();
            }
            else
            {
              // Edit the context cell
              this.m_Playlist.startCellEditing();
            }
          }
        break;
        case "library_cmd_addtoplaylist":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // add the currently selected track to a (possibly new) playlist
            this.m_Playlist.addToPlaylist();
          }
        break;
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeTracks();
          }
        break;
        case "library_cmd_burntocd":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendBurnToCDEvent();
          }
        break;
      }
    }
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
  
} // end of sbPlaylistCommands

