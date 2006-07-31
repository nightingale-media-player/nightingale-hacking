/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);

var theSongbirdStrings = document.getElementById( "songbird_strings" );

var aMediaLibrary = null;
var msDBQuery = new sbIDatabaseQuery();
var polling_interval = null;

var aQueryFileArray = new Array();

var theTitle = document.getElementById( "songbird_media_scan_title" );
theTitle.value = "";

var theLabel = document.getElementById( "songbird_media_scan_label" );
theLabel.value = "";

var theProgress = document.getElementById( "songbird_media_scan" );
theProgress.setAttribute( "mode", "undetermined" );
theProgress.value = 0;

var theProgressValue = 0; // to 100;
var theProgressText = "";

var aMediaScan = null;
var aMediaScanQuery = null;

// Init the text box to the last url played (shrug).
var polling_interval;

function onLoad()
{
  if ( ( typeof( window.arguments[0] ) != 'undefined' ) && ( typeof( window.arguments[0].URL ) != 'undefined' ) )
  {
    try
    {
      aMediaScan = new sbIMediaScan();
      aMediaScanQuery = new sbIMediaScanQuery();
      
      if (aMediaScan && aMediaScanQuery)
      {
        SBDataSetBoolValue( "media_scan.open", true ); // ?  Don't let this go?
      
        aMediaScanQuery.setDirectory(window.arguments[0].URL);
        aMediaScanQuery.setRecurse(true);
                
        aMediaScan.submitQuery(aMediaScanQuery);
        
        polling_interval = setInterval( onPollScan, 333 );
        
        var scanning = "Scanning";
        try
        {
          scanning = theSongbirdStrings.getString("media_scan.scanning");
        } catch(e) {}
        theTitle.value = scanning + " -- " + window.arguments[0].URL;
      }
    }
    catch(err)
    {
      alert("onLoad\n\n"+err);
    }
  }
  return true;
}

function onPollScan()
{
  try
  {
    if ( !aMediaScanQuery.isScanning() )
    {
      clearInterval( polling_interval );

      var complete = "Complete";
      try
      {
        complete = theSongbirdStrings.getString("media_scan.complete");
      } catch(e) {}
      theLabel.value = complete;
      onScanComplete( aMediaScanQuery );
      document.getElementById("button_ok").removeAttribute( "disabled" );
      document.getElementById("button_ok").focus();
      document.getElementById("button_cancel").setAttribute( "disabled", "true" );
    }   
    else
    {
      var len = aMediaScanQuery.getFileCount();
      if ( len )
      {
        var value = aMediaScanQuery.getLastFileFound();
        theLabel.value = gPPS.convertUrlToFolder(value);
      }
    }
  }
  catch ( err )  
  {
    alert( "onPollScan\n\n"+err );
  }
}

function onScanComplete( mediaScanQuery )
{
  mediaScanQuery = aMediaScanQuery;
  theProgress.removeAttribute( "mode" );

  if ( mediaScanQuery.getFileCount() )
  {
    try
    {
      aMediaLibrary = new MediaLibrary();
      aMediaLibrary = aMediaLibrary.QueryInterface( Components.interfaces.sbIMediaLibrary );
            
      if ( ! msDBQuery || ! aMediaLibrary )
      {
        return -1;
      }
      
      msDBQuery.resetQuery();
      msDBQuery.setAsyncQuery( true );
      msDBQuery.setDatabaseGUID( "songbird" );
      
      aMediaLibrary.setQueryObject( msDBQuery );

      // Take the file array and for everything that seems to be a media url, add it to the database.
      var i = 0, count = 0, total = 0;
      total = aMediaScanQuery.getFileCount();

      for ( i = 0, count = 0; i < total; i++ )
      {
        var the_url = null;
        var is_url = null;
        the_url = aMediaScanQuery.getFilePath( i );
        is_url = gPPS.isMediaUrl( the_url );

        if ( is_url )
        {
          var keys = new Array( "title" );
          var values = new Array();
          values.push( gPPS.convertUrlToDisplayName( the_url ) );
          aMediaLibrary.addMedia( the_url, keys.length, keys, values.length, values, false, true );
          aQueryFileArray.push( values[0] );
        }
      }
      
      count = msDBQuery.getQueryCount();
      if ( count )
      {
        var adding = "Adding";
        try
        {
          adding = theSongbirdStrings.getString("media_scan.adding");
        } catch(e) {}
        theTitle.value = adding + " (" + count + ")";
        theLabel.value = "";
        msDBQuery.execute();
        polling_interval = setInterval( onPollQuery, 333 );
      }
      else
      {
        var none = "Nothing";
        try
        {
          none = theSongbirdStrings.getString("media_scan.none");
        } catch(e) {}
        theTitle.value = none;
      }
    }
    catch(err)
    {
      alert("onScanComplete\n\n"+err);
    }
  }
  else
  {
    var none = "Nothing";
    try
    {
      none = theSongbirdStrings.getString("media_scan.none");
    } catch(e) {}
    theTitle.value = none;
  }
}

var lastpos = 0;
function onPollQuery()
{
  try
  {
    if ( msDBQuery )
    {
      var len = msDBQuery.getQueryCount();
      var pos = msDBQuery.currentQuery() + 1;

      lastpos = pos;
      
      if ( len == pos )
      {
        var added = "Added";
        var complete = "Complete";
        try
        {
          added = theSongbirdStrings.getString("media_scan.added");
          complete = theSongbirdStrings.getString("media_scan.complete");
        } catch(e) {}
        theTitle.value = len + " " + added;
        theLabel.value = complete;
        theProgress.value = 100.0;
        clearInterval( polling_interval );
      }   
      else
      {
        var adding = "Adding";
        try
        {
          adding = theSongbirdStrings.getString("media_scan.adding");
        } catch(e) {}
        theTitle.value = adding + " (" + pos + "/" + len + ")";
        theLabel.value = aQueryFileArray[ pos ];
        theProgress.value = ( pos * 100.0 ) / len;
      }
    }
  }
  catch ( err )
  {
    alert( "onPollQuery\n\n"+err );
  }
}

function doOK()
{
  SBDataSetBoolValue( "media_scan.open", false ); // ?  Don't let this go?
  document.defaultView.close();
  return true;
}
function doCancel()
{
  SBDataSetBoolValue( "media_scan.open", false ); // ?  Don't let this go?
  document.defaultView.close();
  return true;
}

