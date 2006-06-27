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
        SBDataSetValue( "media_scan.open", true ); // ?  Don't let this go?
      
        aMediaScanQuery.SetDirectory(window.arguments[0].URL);
        aMediaScanQuery.SetRecurse(true);
                
        aMediaScan.SubmitQuery(aMediaScanQuery);
        
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
    if ( !aMediaScanQuery.IsScanning() )
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
    }   
    else
    {
      var len = aMediaScanQuery.GetFileCount();
      if ( len )
      {
        var value = aMediaScanQuery.GetLastFileFound();
        theLabel.value = ConvertUrlToFolder(value);
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

  if ( mediaScanQuery.GetFileCount() )
  {
    try
    {
      //msDBQuery = new sbIDatabaseQuery();
      aMediaLibrary = new MediaLibrary();
      aMediaLibrary = aMediaLibrary.QueryInterface( Components.interfaces.sbIMediaLibrary );
            
      if ( ! msDBQuery || ! aMediaLibrary )
      {
        return -1;
      }
      
      msDBQuery.ResetQuery();
      msDBQuery.SetAsyncQuery( true );
      msDBQuery.SetDatabaseGUID( "songbird" );
      
      aMediaLibrary.SetQueryObject( msDBQuery );

      // Take the file array and for everything that seems to be a media url, add it to the database.
      var i = 0, count = 0, total = 0;
      total = aMediaScanQuery.GetFileCount();

      for ( i = 0, count = 0; i < total; i++ )
      {
        var the_url = null;
        var is_url = null;
        the_url = aMediaScanQuery.GetFilePath( i );
        is_url = IsMediaUrl( the_url );

        if ( is_url )
        {
          var keys = new Array( "title" );
          var values = new Array();
          values.push( MSConvertUrlToDisplayName( the_url ) );
          aMediaLibrary.AddMedia( the_url, keys.length, keys, values.length, values, false, true );
          aQueryFileArray.push( values[0] );
        }
      }
      
      count = msDBQuery.GetQueryCount();

      if ( count )
      {
        var adding = "Adding";
        try
        {
          adding = theSongbirdStrings.getString("media_scan.adding");
        } catch(e) {}
        theTitle.value = adding + " (" + count + ")";
        theLabel.value = "";
        msDBQuery.Execute();
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
      var len = msDBQuery.GetQueryCount();
      var pos = msDBQuery.CurrentQuery() + 1;

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
  SBDataSetValue( "media_scan.open", false ); // ?  Don't let this go?
  document.defaultView.close();
  return true;
}
function doCancel()
{
  SBDataSetValue( "media_scan.open", false ); // ?  Don't let this go?
  document.defaultView.close();
  return true;
}

function MSConvertUrlToDisplayName( the_url )
{
  var url = decodeURI( the_url );
  // Set the title display  
  var the_value = "";
  if ( url.lastIndexOf('/') != -1 )
  {
    the_value = url.substring( url.lastIndexOf('/') + 1, url.length );
  }
  else if ( url.lastIndexOf('\\') != -1 )
  {
    the_value = url.substring( url.lastIndexOf('\\') + 1, url.length );
  }
  else
  {
    the_value = url;
  }
  if ( ! the_value.length )
  {
    the_value = url;
  }
  return the_value;
}

function ConvertUrlToFolder( url )
{
  // Set the title display  
  url = decodeURI( url );
  var the_value = "";
  if ( url.lastIndexOf('/') != -1 )
  {
    the_value = url.substring( 0, url.lastIndexOf('/') );
  }
  else if ( url.lastIndexOf('\\') != -1 )
  {
    the_value = url.substring( 0, url.lastIndexOf('\\') );
  }
  else
  {
    the_value = url;
  }
  return the_value;
}

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
