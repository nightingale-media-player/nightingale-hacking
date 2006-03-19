/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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

function IsMediaUrl( the_url )
{
  if ( ( the_url.indexOf ) && 
        (
          // Protocols at the beginning
          ( the_url.indexOf( "mms:" ) == 0 ) || 
          ( the_url.indexOf( "rtsp:" ) == 0 ) || 
          // File extensions at the end
          ( the_url.indexOf( ".pls" ) != -1 ) || 
          ( the_url.indexOf( "rss" ) != -1 ) || 
          ( the_url.indexOf( ".m3u" ) == ( the_url.length - 4 ) ) || 
//          ( the_url.indexOf( ".rm" ) == ( the_url.length - 3 ) ) || 
//          ( the_url.indexOf( ".ram" ) == ( the_url.length - 4 ) ) || 
//          ( the_url.indexOf( ".smil" ) == ( the_url.length - 5 ) ) || 
          ( the_url.indexOf( ".mp3" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".ogg" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".flac" ) == ( the_url.length - 5 ) ) ||
          ( the_url.indexOf( ".wav" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".m4a" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".wma" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".wmv" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".asx" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".asf" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".avi" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mov" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mpg" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mp4" ) == ( the_url.length - 4 ) )
        )
      )
  {
    return true;
  }
  return false;
}

function WFInit()
{
  const MediaLibrary = new Components.Constructor("@songbird.org/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  
  if(!wfMediaLibrary)
    wfMediaLibrary = new MediaLibrary();

  wfMediaScan = new sbIMediaScan();
  wfMediaScanQuery = new sbIMediaScanQuery();
  wfQuery = new sbIDatabaseQuery();
  
  wfQuery.SetAsyncQuery(true);
  wfQuery.SetDatabaseGUID("songbird");
  wfMediaLibrary.SetQueryObject(wfQuery);
  
  wfManager.CreateWatchFolderManager();
  
  if(wfWakeUpTimer)
    clearInterval(wfWakeUpTimer);
    
  wfWakeUpTimer = 0;
  
  setTimeout( onWFWakeUpScan, 30 * 1000 );
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
      
      wfMediaScanQuery.SetDirectory(aFolders[0]);
      wfMediaScanQuery.SetRecurse(true);
      
      wfMediaScan.SubmitQuery(wfMediaScanQuery);
      wfPollScanTimer = setInterval(onWFPollScan, 333);
    }
  }
}

function onWFPollScan()
{
  if(!wfMediaScanQuery.IsScanning())
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
  if(wfQuery.IsExecuting())
    return;
    
  if(wfIsProcessing)
  {
    wfQuery.ResetQuery();
    wfIsProcessing = false;
  }

  var fileCount = wfMediaScanQuery.GetFileCount();
  
  if(wfCurrentFile < fileCount)
  {
    var strURL = wfMediaScanQuery.GetFilePath(wfCurrentFile);
    
    if(IsMediaUrl(strURL))
    {
      var keys = new Array( "title" );
      var values = new Array();
          
      values.push( ConvertUrlToDisplayName( strURL ) );
      
      wfMediaLibrary.AddMedia(strURL, keys.length, keys, values.length, values, false, true);
      
      if(wfQuery.GetQueryCount() > wfMaxTracksBeforeProcess)
      {
        wfQuery.Execute();
        wfIsProcessing = true;
      }
    }
    
    wfCurrentFile++;
    
    return;
  }
  else if(wfQuery.GetQueryCount() > 0)
  {
    dump("Dumping left overs. Count: " + wfQuery.GetQueryCount() + "\n");
    
    wfQuery.Execute();
    wfIsProcessing = true;
  }
  else if(wfCurrentFolder + 1 < wfCurrentFolderList.length)
  {
    clearInterval(wfMediaLibraryAddTimer);
    
    wfQuery.ResetQuery();
    wfCurrentFile = 0;
    wfCurrentFolder++;

    wfMediaScanQuery.SetDirectory(wfCurrentFolderList[wfCurrentFolder]);
    wfMediaScanQuery.SetRecurse(true);
      
    wfMediaScan.SubmitQuery(wfMediaScanQuery);
    wfPollScanTimer = setInterval(onWFPollScan, 333);
  }
  else
  {
    clearInterval(wfMediaLibraryAddTimer);

    wfQuery.ResetQuery();
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
    
    this.m_queryObj.SetAsyncQuery(false);
    this.m_queryObj.SetDatabaseGUID(this.m_watchDBGUID);
    this.m_queryObj.ResetQuery();
    
    this.m_queryObj.AddQuery(watchFolderSettingsCreate);
    this.m_queryObj.AddQuery(watchFolderCreate);    
    
    this.m_queryObj.Execute();
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
      this.m_queryObj.ResetQuery();
      for(var i = 0; i < aFolders.length; i++)
      {
        this.m_queryObj.AddQuery("INSERT OR REPLACE INTO " + this.m_watchFolderTable + " VALUES (\"" + aFolders[i] + "\")");
        ++folderCount;
      }
      this.m_queryObj.Execute();
    }
    
    return folderCount;
  }
  
  this.SetWatchFolder = function SetWatchFolder(strOldFolder, strNewFolder)
  {
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("UPDATE " + this.m_watchFolderTable + " SET folder = \"" + strNewFolder + "\" WHERE folder = \"" + strOldFolder + "\"");
    this.m_queryObj.Execute();
  }
  
  this.RemoveWatchFolder = function RemoveWatchFolder(strFolder)
  {
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("DELETE FROM " + this.m_watchFolderTable + " WHERE folder = \"" + strFolder + "\"");
    this.m_queryObj.Execute();
  }

  this.RemoveWatchFolders = function RemoveWatchFolders(aFolders)
  {
    var folderCount = 0;
    
    if(aFolders.length)
    {
      this.m_queryObj.ResetQuery();
      for(var i = 0; i < aFolders.length; i++)
      {
        this.m_queryObj.AddQuery("DELETE FROM " + this.m_watchFolderTable + " WHERE folder = \"" + aFolders[i] + "\"");
        folderCount++;
      }
      this.m_queryObj.Execute();
    }
    
    return folderCount;
  }

  this.RemoveAllWatchFolders = function RemoveAllWatchFolders()
  {
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("DELETE FROM " + this.m_watchFolderTable);
    this.m_queryObj.Execute();
  }

  this.GetWatchFolderCount = function GetWatchFolderCount()
  {
    var folderCount = 0;
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("SELECT COUNT(folder) FROM " + this.m_watchFolderTable);
    this.m_queryObj.Execute();
    
    var resObj = this.m_queryObj.GetResultObject();
    if(resObj.GetRowCount())
      folderCount = parseInt(resObj.GetRowCell(0, 0));
      
    return folderCount;
  }
  
  this.GetWatchFolders = function GetWatchFolders()
  {
    var aFolders = new Array();
    
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("SELECT folder FROM " + this.m_watchFolderTable);
    this.m_queryObj.Execute();
    
    var resObj = this.m_queryObj.GetResultObject();
    var rowCount = resObj.GetRowCount();
    if(rowCount)
    {
      for(var i = 0; i < rowCount; i++)
      {
        var strFolder = resObj.GetRowCellByColumn(i, "folder");
        aFolders.push(strFolder);
      }
    }
    
    return aFolders;
  }
  
  this.GetFolderScanInterval = function GetFolderScanInterval()
  {
    var scanInterval = 0;
    
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("SELECT value FROM " + this.m_watchFolderSettingsTable + " WHERE name = 'scan_interval'");
    this.m_queryObj.Execute();
    
    var resObj = this.m_queryObj.GetResultObject();
    if(resObj.GetRowCount())
      scanInterval = parseInt(resObj.GetRowCell(0, 0));
    
    return scanInterval;
  }
  
  this.SetFolderScanInterval = function SetFolderScanInterval(scanInterval)
  {
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("INSERT OR REPLACE INTO " + this.m_watchFolderSettingsTable + " VALUES ('scan_interval', '" + scanInterval + "')");
    this.m_queryObj.Execute();
  }
  
  this.GetFolderLastScanTime = function GetFolderLastScanTime()
  {
    var lastScanTime = 0;
    
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("SELECT value FROM " + this.m_watchFolderSettingsTable + " WHERE name = 'last_scan_time'");
    this.m_queryObj.Execute();
    
    var resObj = this.m_queryObj.GetResultObject();
    if(resObj.GetRowCount())
      lastScanTime = parseInt(resObj.GetRowCell(0, 0));
    
    return lastScanTime;
  }
  
  this.SetFolderLastScanTime = function SetFolderLastScanTime()
  {
    var dNow = new Date();
    this.m_queryObj.ResetQuery();
    this.m_queryObj.AddQuery("INSERT OR REPLACE INTO " + this.m_watchFolderSettingsTable + " VALUES ('last_scan_time', '" + dNow.getTime() + "')");
    this.m_queryObj.Execute();
  }
}

var wfManager = new CWatchFolderManager();

