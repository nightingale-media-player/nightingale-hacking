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

var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
var wfMediaLibrary = null;
var wfMediaScan = null;
var wfMediaScanQuery = null;
var wfQuery = null;

//Default to 10 minute update interval.
var wfDefaultInterval = 10;
var wfInterval = 0;

var wfIsProcessing = false;
var wfMaxTracksBeforeProcess = 99;

var wfCurrentFolder = 0;
var wfCurrentFolderList = new Array();

var wfCurrentFile = 0;

var wfWakeUpTimer = null;
var wfPollScanTimer = 0;
var wfMediaLibraryAddTimer = 0;

function WFInit()
{
  const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  
  if(!wfMediaLibrary)
    wfMediaLibrary = new MediaLibrary();

  wfMediaScan = new sbIMediaScan();
  wfMediaScanQuery = new sbIMediaScanQuery();
  wfQuery = new sbIDatabaseQuery();
  
  wfQuery.setAsyncQuery(true);
  wfQuery.setDatabaseGUID("songbird");
  wfMediaLibrary.setQueryObject(wfQuery);
  
  wfManager.CreateWatchFolderManager();
  
  if(wfWakeUpTimer)
    clearInterval(wfWakeUpTimer);
    
  wfWakeUpTimer = 0;
  
  setTimeout( onWFWakeUpScan, 30 * 1000 );
}

function WFShutdown()
{
  wfMediaScan = null;
  wfMediaScanQuery = null;
  wfQuery = null;
  
  if(wfWakeUpTimer)
    clearInterval(wfWakeUpTimer); 
}

function onWFWakeUpScan()
{
  if ( ! wfWakeUpTimer )
  {
    wfWakeUpTimer = setInterval(onWFWakeUpScan, 10000);
  }
    
  if(wfCurrentFolderList.length == 0)
  {
    var aFolders = wfManager.GetWatchFolders();
    if(aFolders && aFolders.length)
    {
      wfCurrentFolder = 0;
      wfCurrentFolderList = aFolders;
      
      wfMediaScanQuery.setDirectory(aFolders[0]);
      wfMediaScanQuery.setRecurse(true);
      
      wfMediaScan.submitQuery(wfMediaScanQuery);
      wfPollScanTimer = setInterval(onWFPollScan, 333);
    }
  }
}

function onWFPollScan()
{
  if(!wfMediaScanQuery.isScanning())
  {
    clearInterval(wfPollScanTimer);
    onWFScanComplete();
  }
}

function onWFScanComplete()
{
  wfMediaLibraryAddTimer = setInterval(onWFLibraryAdd, 66);
}

function onWFLibraryAdd()
{
  if(wfQuery.isExecuting())
    return;
    
  if(wfIsProcessing)
  {
    wfQuery.resetQuery();
    wfIsProcessing = false;
  }

  var fileCount = wfMediaScanQuery.getFileCount();
  
  if(wfCurrentFile < fileCount)
  {
    var strURL = wfMediaScanQuery.getFilePath(wfCurrentFile);
    
    if(gPPS.isMediaURL(strURL))
    {
      var keys = new Array( "title" );
      var values = new Array();
          
      values.push( gPPS.convertURLToDisplayName( strURL ) );
      
      wfMediaLibrary.addMedia(strURL, keys.length, keys, values.length, values, false, true);
      
      if(wfQuery.getQueryCount() > wfMaxTracksBeforeProcess)
      {
        wfQuery.execute();
        wfIsProcessing = true;
      }
    }
    
    wfCurrentFile++;
    
    return;
  }
  else if(wfQuery.getQueryCount() > 0)
  {
    dump("Dumping left overs. Count: " + wfQuery.getQueryCount() + "\n");
    
    wfQuery.execute();
    wfIsProcessing = true;
  }
  else if(wfCurrentFolder + 1 < wfCurrentFolderList.length)
  {
    clearInterval(wfMediaLibraryAddTimer);
    
    wfQuery.resetQuery();
    wfCurrentFile = 0;
    wfCurrentFolder++;

    wfMediaScanQuery.setDirectory(wfCurrentFolderList[wfCurrentFolder]);
    wfMediaScanQuery.setRecurse(true);
      
    wfMediaScan.submitQuery(wfMediaScanQuery);
    wfPollScanTimer = setInterval(onWFPollScan, 333);
  }
  else
  {
    clearInterval(wfMediaLibraryAddTimer);

    wfQuery.resetQuery();
    wfCurrentFile = 0;
    wfCurrentFolder = 0;
    wfCurrentFolderList = new Array();
  }
 
}

function CWatchFolderManager()
{
  this.m_queryObj = new sbIDatabaseQuery();
  
  this.m_watchDBGUID = "watch_folders";
  this.m_watchFolderTable = "watch_folders";
  this.m_watchFolderSettingsTable = "watch_folders_settings";
  
  this.CreateWatchFolderManager = function CreateWatchFolderManager()
  {
    var watchFolderCreate = "CREATE TABLE " + this.m_watchFolderTable + " (folder TEXT UNIQUE NOT NULL)";
    var watchFolderSettingsCreate = "CREATE TABLE " + this.m_watchFolderSettingsTable + " (name TEXT UNIQUE NOT NULL, value TEXT NOT NULL DEFAULT '')";
    
    this.m_queryObj.setAsyncQuery(false);
    this.m_queryObj.setDatabaseGUID(this.m_watchDBGUID);
    this.m_queryObj.resetQuery();
    
    this.m_queryObj.addQuery(watchFolderSettingsCreate);
    this.m_queryObj.addQuery(watchFolderCreate);    
    
    this.m_queryObj.execute();
  }
  
  this.AddWatchFolder = function AddWatchFolder(strFolder)
  {
    var aFolders = new Array();
    aFolders.push(strFolder);
    return this.AddWatchFolders(aFolders);
  }
  
  this.AddWatchFolders = function AddWatchFolders(aFolders)
  {
    var folderCount = 0;
    if(aFolders.length)
    {
      this.m_queryObj.resetQuery();
      for(var i = 0; i < aFolders.length; i++)
      {
        this.m_queryObj.addQuery("INSERT OR REPLACE INTO " + this.m_watchFolderTable + " VALUES (\"" + aFolders[i] + "\")");
        ++folderCount;
      }
      this.m_queryObj.execute();
    }
    
    return folderCount;
  }
  
  this.SetWatchFolder = function SetWatchFolder(strOldFolder, strNewFolder)
  {
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("UPDATE " + this.m_watchFolderTable + " SET folder = \"" + strNewFolder + "\" WHERE folder = \"" + strOldFolder + "\"");
    this.m_queryObj.execute();
  }
  
  this.RemoveWatchFolder = function RemoveWatchFolder(strFolder)
  {
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("DELETE FROM " + this.m_watchFolderTable + " WHERE folder = \"" + strFolder + "\"");
    this.m_queryObj.execute();
  }

  this.RemoveWatchFolders = function RemoveWatchFolders(aFolders)
  {
    var folderCount = 0;
    
    if(aFolders.length)
    {
      this.m_queryObj.resetQuery();
      for(var i = 0; i < aFolders.length; i++)
      {
        this.m_queryObj.addQuery("DELETE FROM " + this.m_watchFolderTable + " WHERE folder = \"" + aFolders[i] + "\"");
        folderCount++;
      }
      this.m_queryObj.execute();
    }
    
    return folderCount;
  }

  this.RemoveAllWatchFolders = function RemoveAllWatchFolders()
  {
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("DELETE FROM " + this.m_watchFolderTable);
    this.m_queryObj.execute();
  }

  this.GetWatchFolderCount = function GetWatchFolderCount()
  {
    var folderCount = 0;
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("SELECT COUNT(folder) FROM " + this.m_watchFolderTable);
    this.m_queryObj.execute();
    
    var resObj = this.m_queryObj.eetResultObject();
    if(resObj.getRowCount())
      folderCount = parseInt(resObj.getRowCell(0, 0));
      
    return folderCount;
  }
  
  this.GetWatchFolders = function GetWatchFolders()
  {
    var aFolders = new Array();
    
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("SELECT folder FROM " + this.m_watchFolderTable);
    this.m_queryObj.execute();
    
    var resObj = this.m_queryObj.getResultObject();
    var rowCount = resObj.getRowCount();
    if(rowCount)
    {
      for(var i = 0; i < rowCount; i++)
      {
        var strFolder = resObj.getRowCellByColumn(i, "folder");
        aFolders.push(strFolder);
      }
    }
    
    return aFolders;
  }
  
  this.GetFolderScanInterval = function GetFolderScanInterval()
  {
    var scanInterval = 0;
    
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("SELECT value FROM " + this.m_watchFolderSettingsTable + " WHERE name = 'scan_interval'");
    this.m_queryObj.execute();
    
    var resObj = this.m_queryObj.getResultObject();
    if(resObj.getRowCount())
      scanInterval = parseInt(resObj.getRowCell(0, 0));
    
    return scanInterval;
  }
  
  this.SetFolderScanInterval = function SetFolderScanInterval(scanInterval)
  {
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("INSERT OR REPLACE INTO " + this.m_watchFolderSettingsTable + " VALUES ('scan_interval', '" + scanInterval + "')");
    this.m_queryObj.execute();
  }
  
  this.GetFolderLastScanTime = function GetFolderLastScanTime()
  {
    var lastScanTime = 0;
    
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("SELECT value FROM " + this.m_watchFolderSettingsTable + " WHERE name = 'last_scan_time'");
    this.m_queryObj.execute();
    
    var resObj = this.m_queryObj.getResultObject();
    if(resObj.getRowCount())
      lastScanTime = parseInt(resObj.getRowCell(0, 0));
    
    return lastScanTime;
  }
  
  this.SetFolderLastScanTime = function SetFolderLastScanTime()
  {
    var dNow = new Date();
    this.m_queryObj.resetQuery();
    this.m_queryObj.addQuery("INSERT OR REPLACE INTO " + this.m_watchFolderSettingsTable + " VALUES ('last_scan_time', '" + dNow.getTime() + "')");
    this.m_queryObj.execute();
  }
}

var wfManager = new CWatchFolderManager();

