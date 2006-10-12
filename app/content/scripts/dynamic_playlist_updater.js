/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
 
const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const PlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");

var dpPlaylistManager = (new PlaylistManager()).QueryInterface(Components.interfaces.sbIPlaylistManager);
var dpPlaylistReaderManager = (new PlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);

var dpCurrentTime = new Date();
var dpUpdaterCurrentRow = 0;
var dpUpdaterQuery = new sbIDatabaseQuery();
var dpUpdaterPlaylistQuery = new sbIDatabaseQuery();

var dpUpdaterPlaylistListener = null;

//Default to one minute update interval.
var dpUpdaterPeriod = 1;
var dpUpdaterInterval = null;

function DPUpdaterInit(interval)
{
  try
  {
    if(interval != 0)
      dpUpdaterPeriod = interval;

    // create a closure to keep the manager around (and its web progress listener)
    var DPUpdaterStart = function ()
    {
      // Wake up every 30 seconds times the period.
      dpUpdaterInterval = setInterval( DPUpdaterRun, dpUpdaterPeriod * 30 * 1000 );
    }

    setTimeout( DPUpdaterStart, 30 * 1000 ); // Don't do nothing for 30 seconds
  }
  catch(err)
  {
    alert(err);
  }
}

function DPUpdaterDeinit()
{
  if (dpUpdaterInterval)
    clearInterval(dpUpdaterInterval);
}


function DPUpdaterRun()
{
  try
  {
    // Sanity check
    if(dpUpdaterInterval == null)
      return;
    if(dpUpdaterQuery.isExecuting())
      return;
   
    // if there is more stuff to process in the query, do that 
    if(dpUpdaterCurrentRow > 0 && dpUpdaterCurrentRow < dpUpdaterQuery.getResultObject().getRowCount())
    {
      //Next Row
      DPUpdaterUpdatePlaylist(dpUpdaterCurrentRow);
    }
    // otherwise run another query
    else
    {
      //Check to see if any dynamic playlists need to be updated.
      var nCount = dpPlaylistManager.getDynamicPlaylistsForUpdate(dpUpdaterQuery);
      
      if(nCount)
        DPUpdaterUpdatePlaylist(0);
    }
  }
  catch(err)
  {
    alert(err);
  }
}

function DPUpdaterUpdatePlaylist(row)
{
  try
  {
    var resObj = dpUpdaterQuery.getResultObject();
    var rowCount = resObj.getRowCount();

    var strGUID = resObj.getRowCellByColumn(row, "service_uuid");
    var strURL = resObj.getRowCellByColumn(row, "url");
    var strName = resObj.getRowCellByColumn(row, "name");
    var strReadableName = resObj.getRowCellByColumn(row, "readable_name");

    // true causes strURL to be compared to the origin_url before being added to library and playlist
    var success = dpPlaylistReaderManager.loadPlaylist(strURL, strGUID, strName, strReadableName, "user", strURL, "", true, null);
   
    if(success)
    {
      dpUpdaterPlaylistQuery.resetQuery();
      dpUpdaterPlaylistQuery.setDatabaseGUID(strGUID);
      dpPlaylistManager.setDynamicPlaylistLastUpdate(strName, dpUpdaterPlaylistQuery);
    }
    
    dpUpdaterCurrentRow++;
  
    if(dpUpdaterCurrentRow >= rowCount)
      dpUpdaterCurrentRow = 0;
  }
  catch(err)
  {
    alert(err);
  }
}

function DPUpdaterManualRun()
{
  var nCount = dpPlaylistManager.getDynamicPlaylistsForUpdate(dpUpdaterQuery);
  
  if(nCount)
    DPUpdaterUpdatePlaylist(0);
}

