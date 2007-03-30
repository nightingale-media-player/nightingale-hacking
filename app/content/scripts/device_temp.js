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

// ...and now I get to start cleaning up my mess...


try
{




//
// XUL Event Methods
//

var URL = SB_NewDataRemote( "faceplate.play.url", null );
var thePlaylistIndex = SB_NewDataRemote( "playlist.index", null );
var seen_playing = SB_NewDataRemote( "faceplate.seenplaying", null );
var theTitleText = SB_NewDataRemote( "metadata.title", null );
var theArtistText = SB_NewDataRemote( "metadata.artist", null );
var theAlbumText = SB_NewDataRemote( "metadata.album", null );
var theStatusText = SB_NewDataRemote( "faceplate.status.text", null );
var theStatusStyle = SB_NewDataRemote( "faceplate.status.style", null );


var SBCDCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_Context: null, 
  m_Table: null,
  m_DeviceName: null,

  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_rip",
    "library_cmd_edit"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.rip",
    "&command.edit"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.rip",
    "&command.tooltip.edit"
  ),

  getNumCommands: function()
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

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },


  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        case 0:
        case 1:
        case 2:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_play":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_rip":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            try
            {        
                var filterCol = "uuid";
                var filterVals = new Array();
                
                var columnObj = this.m_Playlist.tree.columns.getNamedColumn(filterCol);
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
                        
                        var value = this.m_Playlist.tree.view.getCellText(c, columnObj);
                        
                        filterVals.push(value);
                    }
                }
                onCDRip( this.m_DeviceName, this.m_Context, this.m_Table, filterCol, filterVals.length, filterVals, this.m_Device);
                // And show the download table in the chrome playlist.
                //gBrowser.onBrowserDownload();
            }
            catch( err )          
            {
                alert( err );
            }
          }
        break;
        case "library_cmd_edit":
        break;
      }
      event.stopPropagation();
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

}; // SBCDCommands definition

var SBRippingCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_DeviceName: null,

  m_Ids: new Array
  (
    "library_cmd_remove"
  ),
  
  m_Names: new Array
  (
    "&command.remove"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.remove"
  ),

  getNumCommands: function()
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

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeTracks();
          }
        break;
      }
      event.stopPropagation();
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

}; // SBRippingCommands definition


function onCDRip(deviceName, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, aCDDevice)
{
    try
    {
        theWebPlaylistQuery = null; 
          
        if (aCDDevice)
        {
            // Make a magic data object to get passed to the dialog
            var ripping_data = new Object();
            ripping_data.retval = "";
            ripping_data.value = SBDataGetStringValue( "ripping.folder" );
            
            if ( ( SBDataGetIntValue( "ripping.always" ) == 1 ) && ( ripping_data.value.length > 0 ) )
            {
                ripping_data.retval = "ok";
            }
            else
            {
                // Open the window
                SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,centerscreen", ripping_data ); 
            }

            // Pick download destination
            if ( ( ripping_data.retval == "ok" ) && ( ripping_data.value.length > 0 ) )
            {
                var rippingTable = {};
                // Passing empty string for device name as download device has just one device
                // Prepare table for download & get the name for newly prepared download table
                //downloadDevice.addCallback(theDownloadListener);
                
                aCDDevice.autoDownloadTable(deviceName, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, '', ripping_data.value, rippingTable);
                
                // Register the guid and table with the playlist source to always show special download commands.
                SBRippingCommands.m_Device = aCDDevice;
                SBRippingCommands.m_DeviceName = deviceName;
                var source = new sbIPlaylistsource();
                source.registerPlaylistCommands( guid, rippingTable.value, "download", SBRippingCommands );
            }
        }
    }
    
    catch ( err )
    {
        alert( err );
    }
}

var theCDListener = 
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
  },
  
  onDeviceConnect: function(deviceName)
  {
    OnCDInsert(deviceName);
  },
  
  onDeviceDisconnect: function(deviceName)
  {
  }
};

function OnCDInsert(deviceName)
{
  if (deviceManager)
  {
    var CDCategory = 'Songbird CD Device';
    if (deviceManager.hasDeviceForCategory(CDCategory))
    {
      var aCDDevice =
        deviceManager.getDeviceByCategory(CDCategory);
      SBCDCommands.m_DeviceName = deviceName;
      SBCDCommands.m_Device = aCDDevice;
      var cdTable = {};
      var cdContext = {};
      aCDDevice.getTrackTable(SBCDCommands.m_DeviceName, cdContext, cdTable);
      cdContext = cdContext.value;
      cdTable = cdTable.value;
      SBCDCommands.m_Context = cdContext;
      SBCDCommands.m_Table = cdTable;
      var source = new sbIPlaylistsource();
      if ( cdContext && cdTable )
      {
        try
        {
          source.registerPlaylistCommands( cdContext, cdTable, cdTable, SBCDCommands );
        }
        catch ( err )
        {
          alert( "source.registerPlaylistCommands( " + SBCDCommands.m_Context + ", " + SBCDCommands.m_Table+ " );\r\n" + err )
        }
      }
    }
  }
}

// Register the download commands at startup if we know what the download table is.
var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                getService(Components.interfaces.sbIDeviceManager);


// Register for CD notifications
if (deviceManager)
{
    var CDCategory = 'Songbird CD Device';
    if (deviceManager.hasDeviceForCategory(CDCategory))
    {
        var aCDDevice = deviceManager.getDeviceByCategory(CDCategory);
        try
        {
            aCDDevice.addCallback(theCDListener);
            if (aCDDevice.deviceCount > 0)
            {
			  // Currently handling just the first device
			  OnCDInsert(aCDDevice.getDeviceStringByIndex(0));
            }
        }
        catch ( err )
        {
            alert( "aCDDevice.addCallback(theCDListener);\r\n" + err );
        }
    }
}


var SBCDBurningCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_DeviceName: null,
  m_TableName: null,

  m_Ids: new Array
  (
    "library_cmd_start",
    "library_cmd_stop"
  ),
  
  m_Names: new Array
  (
    "&command.start",
    "&command.stop"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.start",
    "&command.tooltip.stop"
  ),

  getNumCommands: function()
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

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "toolbarbutton" || event.target.tagName == "xul:toolbarbutton" );
      switch( event.target.id )
      {
        case "library_cmd_start":
            // start CD rip
            //onBrowserCDTransfer(this.m_Device, this.m_DeviceName, 0 );
            onStartCDBurn(this.m_Device, this.m_DeviceName, this.m_TableName);
        break;
        case "library_cmd_stop":
            // stop CD rip
            onStopCDBurn(this.m_DeviceName, this.m_Device);
        break;
      }
      event.stopPropagation();
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

};  // SBCDBurningCommands class definition

function onStartCDBurn(cdDevice, deviceName, table)
{
  try
  {
    if (cdDevice != null)
    {
      CheckCDAvailableForBurn();
      if (cdAvailableForWrite)
      {
        cdDevice.uploadTable(deviceName, table);
      }
    }
  }
  
  catch (err)
  {
    alert(err);
  }
}

var cdAvailableForWrite = 0;
var writableCDDeviceString = '';

function onAddToCDBurn(guid, table, strFilterColumn, nFilterValueCount, aFilterValues)
{
    try
    {
        theWebPlaylistQuery = null; 
        // deviceName is ignored for now and we ask cd device for the writable cd drive
        CheckCDAvailableForBurn();
        if (cdAvailableForWrite == 0)
           return; 
                
        var burnTable = {};
        
        // CD burning will be a two step process, user has to click the 'burn' button to initiate a CD burn
        aCDDevice.makeTransferTable(writableCDDeviceString, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, '', '', false, burnTable);
        
        // Register the guid and table with the playlist source to always show special burn commands.
        SBCDBurningCommands.m_Device = aCDDevice;
        SBCDBurningCommands.m_DeviceName = writableCDDeviceString;
        SBCDBurningCommands.m_TableName = burnTable.value;
        var source = new sbIPlaylistsource();
        source.registerPlaylistCommands( aCDDevice.getContext(writableCDDeviceString), burnTable.value, burnTable.value, SBCDBurningCommands );

        // And show the download table in the chrome playlist.
        //onBrowserCDTransfer(aCDDevice, writableCDDeviceString, 0 /*Burning*/);
    }
    
    catch ( err )
    {
        alert( err );
    }
}

function CheckCDAvailableForBurn()
{
    cdAvailableForWrite = 0;
    var aCDDevice = null;
    deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].getService(Components.interfaces.sbIDeviceManager);
    if (deviceManager)
    {
        var CDCategory = 'Songbird CD Device';
        if (deviceManager.hasDeviceForCategory(CDCategory))
          aCDDevice = deviceManager.getDeviceByCategory(CDCategory);
    }

    if (!aCDDevice)
    {
        return;
    }
    aCDDevice = aCDDevice.QueryInterface(Components.interfaces.sbICDDevice);
    if (!aCDDevice)
    {
        return;
    }

    var temp = {};
    if (!aCDDevice.getWritableCDDrive(temp))
    {
        return;
    }
    writableCDDeviceString = temp.value;
    
    if (!aCDDevice.isUploadSupported(writableCDDeviceString))
    {
        return;
    }

    if (aCDDevice.getAvailableSpace(writableCDDeviceString) == 0)
    {
        return;
    }
    
	alert("Writable CD Found at " + writableCDDeviceString);
    cdAvailableForWrite = 1;
}

function onStopCDBurn(deviceName, aCDDevice)
{
  if (aCDDevice)
  {
    aCDDevice.abortTransfer(deviceName);
  }
}

function onBrowserCDTransfer(cdDevice, deviceString, ripping)
{  
  if  (ripping == 1)
	metrics_inc("player", "cd ripping", null);
  else
	metrics_inc("player", "cd burning", null);
    
  // Work to figure out guid and table
  var guid = cdDevice.getContext(deviceString);
  var table;
  if (ripping)
    table = cdDevice.getDownloadTable(deviceString);
  else
    table = cdDevice.getUploadTable(deviceString);

  // Actual functionality
  if (!gBrowser.playlistTree)
  {
    // Errrr, nope?
    if ( ( guid == "" ) || ( table == "" ) )
    {
      return;
    }

    if ( theWebPlaylist.ref != ( "NC:" + guid + "_" + table ) )
    {
      if (ripping)
        theWebPlaylist.bind( guid, table, null, SBCDRippingCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
      else
        theWebPlaylist.bind( guid, table, null, SBCDBurningCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
    }
    
    // Show/hide them
    gBrowser.showWebPlaylist = true;
  }
  else
  {
    gBrowser.loadURI( "chrome://songbird/content/xul/playlist_test.xul?" + table + "," + guid);
  }
}

/*

// WMD 
var theSBWMDCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_Context: null, 
  m_Table: null,
  m_DeviceName: null,

  m_Ids: new Array
  (
    "library_cmd_edit"
  ),
  
  m_Names: new Array
  (
    "&command.edit"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.edit"
  ),

  getNumCommands: function()
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

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },


  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        case 0:
        case 1:
        case 2:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_edit":
        break;
      }
      event.stopPropagation();
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

}; // SBWMDCommands definition

var theWMDListener = 
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
  },
  
  onDeviceConnect: function(deviceName)
  {
    OnWMDConnect(deviceName);
  },
  
  onDeviceDisconnect: function(deviceName)
  {
  }
};

function OnWMDConnect(deviceName)
{
  if (deviceManager)
  {
    var WMDCategory = 'Songbird WM Device';
    if (deviceManager.hasDeviceForCategory(WMDCategory))
    {
      var aWMDDevice =
        deviceManager.getDeviceByCategory(WMDCategory);
      if (aWMDDevice)
	  {
		theSBWMDCommands.m_DeviceName = deviceName;
		theSBWMDCommands.m_Device = aWMDDevice;
		var wmdTable = {};
		var wmdContext = {};
		aWMDDevice.getTrackTable(theSBWMDCommands.m_DeviceName, wmdContext, wmdTable);
		wmdContext = wmdContext.value;
		wmdTable = wmdTable.value;
		theSBWMDCommands.m_Context = wmdContext;
		theSBWMDCommands.m_Table = wmdTable;
		var source = new sbIPlaylistsource();
		if ( wmdContext && wmdTable )
		{
			try
			{
			source.registerPlaylistCommands( wmdContext, wmdTable, wmdTable, theSBWMDCommands );
			}
			catch ( err )
			{
			alert( "source.registerPlaylistCommands( " + theSBWMDCommands.m_Context + ", " + theSBWMDCommands.m_Table+ " );\r\n" + err )
			}
		}
	  }
    }
  }
}

// Register for WMD notifications
if (deviceManager)
{
    var WMDCategory = 'Songbird WM Device';
    if (deviceManager.hasDeviceForCategory(WMDCategory))
    {
        var aWMDDevice = deviceManager.getDeviceByCategory(WMDCategory);
        try
        {
            aWMDDevice.addCallback(theWMDListener);
            if (aWMDDevice.deviceCount > 0)
            {
			  // Currently handling just the first device
			  OnWMDConnect(aWMDDevice.getDeviceStringByIndex(0));
            }
        }
        catch ( err )
        {
            alert( "aWMDevice.addCallback(theWMListener);\r\n" + err );
        }
    }
}
*/

}// END

catch ( err )
{
  alert( err );
}

// alert( "success!" );
// END

