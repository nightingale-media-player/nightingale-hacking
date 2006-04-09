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
 
try 
{
  var ENABLE_BACKSCAN = 1;
  
  const sbDatabaseGUID = "songbird";
//  const sbDatabaseGUID = "webplaylist";

  const MetadataManager = new Components.Constructor("@songbird.org/Songbird/MetadataManager;1", "sbIMetadataManager");
  var aMetadataManager = new MetadataManager();
  const MediaLibrary = new Components.Constructor("@songbird.org/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  var aMediaLibrary = new MediaLibrary();
  const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer");

  var bsPaused = false;
  var bsDBQuery = null;
  var bsLastRow = 0;
  var bsTry = 0;
  var bsMaxTries = 5;
  var bsSource = new sbIPlaylistsource();
  var bsSkip = 0;
  var bsMDHandler = null;

  var bsCurQuery = new sbIDatabaseQuery();
  bsCurQuery.SetAsyncQuery( true );
  bsCurQuery.SetDatabaseGUID( sbDatabaseGUID );
 
  var bsWaitForExecuting = false;
  
  var bsGUIDArray = new Array();
  var bsMetadataArray = new Array();
  var bsMaxArray = 20; // How many tracks to enqueue before writing to the db.
  var bsSubmitQueries = false;

  var bsScanningText = new sbIDataRemote( "backscan.status" );
  var bsPlaylistRef = new sbIDataRemote( "playlist.ref" );
  var bsSongbirdStrings = document.getElementById( "songbird_strings" );
  var bsScanningPaused = new sbIDataRemote( "backscan.paused" );
  
  var bsQueryString = "SELECT uuid, url, length, title FROM library where length=\"0\"";
  
  // Init the text box to the last url played (shrug).
  function BSInitialize()
  {
    try
    {
      if ( ENABLE_BACKSCAN )
      {
        // Bind a function to let us pause and unpause
        bsScanningPaused.BindCallbackFunction( BSDataChange );
      
        // Go start an async query on the library
        bsDBQuery = new sbIDatabaseQuery();
        bsDBQuery.SetAsyncQuery( true );
        bsDBQuery.SetDatabaseGUID( sbDatabaseGUID );

        // Fire us off in a little bit
        bsWaitForExecuting = true;
        setTimeout( BSExecute, 5000 );
        
        // And start scanning in the background.
        setInterval( onBackScan, 150 );
      }
    }
    catch ( err )
    {
      alert( err );
    }
  }

  function onBackScan()
  {
    try
    {
      // Wait till we're done with things.
      if ( bsWaitForExecuting )
      {
        return;
      }
      if ( ( bsCurQuery ) && ( bsCurQuery.IsExecuting() ) )
      {
        return;
      }
      
      if ( bsMDHandler )
      {
        if ( bsMDHandler.Completed() )
          BSReadHandlerValues();
        return;
      }

      if ( ! bsScanningPaused.GetIntValue() )
      {
        if ( ! bsDBQuery.IsExecuting() && ( SBDataGetIntValue( "media_scan.open" ) == 0 ) )
        {
          // ...otherwise, we need to start the next track
          var result = bsDBQuery.GetResultObject();
          
          // If we're at the end of the list,
          if ( ( result.GetRowCount() == 0 ) || ( bsLastRow >= result.GetRowCount() ) )
          {
            // ...we need to resubmit the query.
            bsWaitForExecuting = true;
            setTimeout( BSExecute, 2000 );
            bsScanningText.SetValue( "" );
          }
          else
          {
            // ...otherwise, try the next one.
            BSNextTrack();
          }
        }
      }
    }
    catch ( err )  
    {
      alert( "Backscan: " + err );
    }
  }

  function BSExecute()
  {
    bsDBQuery.ResetQuery();
    bsDBQuery.AddQuery( bsQueryString );
    var ret = bsDBQuery.Execute();
    bsWaitForExecuting = false;
    bsLastRow = 0;
  }
  
  function BSNextTrack()
  {
    // ...otherwise, try the next one.
    var result = bsDBQuery.GetResultObject();
    var count = 0;
    var quit = false;
   
    while ( ! bsSubmitQueries && ( bsLastRow < result.GetRowCount() ) ) 
    {
      var time = result.GetRowCellByColumn( bsLastRow, "length" );
      
      if ( ( time && time.length && time != "0" ) )
      {
        // Skip it, we already have metadata
        bsScanningText.SetValue( "" );
        if ( count++ > 200 )
        {
          bsSubmitQueries = true;
          quit = true;
        }
      }
      else
      {
        var title = result.GetRowCellByColumn( bsLastRow, "title" );
        var uuid = result.GetRowCellByColumn( bsLastRow, "uuid" );
        var url = result.GetRowCellByColumn( bsLastRow, "url" );
        
        var scanning = "Scanning";
        try
        {
          scanning = bsSongbirdStrings.getString("back_scan.scanning");
        } catch(e) {}
        bsScanningText.SetValue( scanning + "..." );
        
        bsMDHandler = aMetadataManager.GetHandlerForMediaURL(url);
        var retval = bsMDHandler.Read();
        // If it's immediately done, read the values.
        if ( bsMDHandler.Completed() )
        {
          // local
          BSReadHandlerValues();
        }
        
        quit = true;
      }
      
      if ( quit )
      {
        break;
      }
    }

    {
      // If we get to the end of the list and we have things to submit, submit them!
      if ( ( bsLastRow == result.GetRowCount() ) && ( bsGUIDArray.length > 0 ) )
      {
        bsSubmitQueries = true;
      }

      if ( bsSubmitQueries )
      {
      
        bsSubmitQueries = false;
        var anything = false;
        
        bsCurQuery.ResetQuery();
        aMediaLibrary.SetQueryObject(bsCurQuery);
        
        var text = "";
        for ( var i = 0; i < bsGUIDArray.length; i++ )
        {
          text += bsGUIDArray[ i ] + " - " + keys[ i ] + " - " + bsMetadataArray[ i ] + "\n";
        
          // Go submit the metdadata update, and stash the query so we know when the update is done.
          aMediaLibrary.SetValuesByGUID( bsGUIDArray[i], keys.length, keys, bsMetadataArray[i].length, bsMetadataArray[i], true );
          anything = true;
        }
//        alert( text );
        
        bsGUIDArray = [];
        bsMetadataArray = [];

        if ( anything )
        {
          bsCurQuery.Execute();
        }
      }
    }
  }

  function BSReadHandlerValues()
  {
    var values = bsMDHandler.GetValuesMap();
    // clear the bsMDHandler variable so we don't track it.
    bsMDHandler = null;
    
    var metadata = new Array();
    for ( var i in keys )
    {
      metadata[ i ] = values.getValue( keys[ i ] );
    }        
    
    // GRRRRR.
    for ( var i in metadata )
    {
      if ( metadata[i] == null )
      {
        metadata[i] = "";
      }
    }

    var result = bsDBQuery.GetResultObject();
    var time = result.GetRowCellByColumn( bsLastRow, "length" );
    var title = result.GetRowCellByColumn( bsLastRow, "title" );
    var uuid = result.GetRowCellByColumn( bsLastRow, "uuid" );
    var url = result.GetRowCellByColumn( bsLastRow, "url" );
    
    // If the title is null, use the url
    if ( metadata[0] == "" )
    {
      // No, this should already have the url (or something else) there, so just leave it alone.
      metadata[0] = title; // ConvertUrlToDisplayName( url );
    }
    // If the length is null parse a 0, otherwise strip hours. 
    if ( metadata[1] == "" )
    {
      metadata[1] = "0:00";
    }
    else
    {
      metadata[1] = BSStripHoursFromTimeString( metadata[1] );
    }
    if ( metadata[1] == "0" )
    {
      metadata[1] = "0:00";
    }

    bsGUIDArray.push( uuid );
    bsMetadataArray.push( metadata );
    
    if ( bsGUIDArray.length >= bsMaxArray )
    {
      bsSubmitQueries = true;
    }

    bsLastRow++;
  }
  
  function BSDataChange()
  {
    var scanning = "Scanning";
    var paused = "Paused";
    
    try
    {
      scanning = bsSongbirdStrings.getString("back_scan.scanning");
      paused = bsSongbirdStrings.getString("back_scan.paused");
    } catch(e) {}
    if ( bsScanningText.GetValue() != "" )
    {
      if ( bsScanningPaused.GetIntValue() )
      {
        bsScanningText.SetValue( scanning + " " + paused );
      }
      else
      {
        bsScanningText.SetValue( scanning + "..." );
      }
    }
  }

  function BSStripHoursFromTimeString( str )
  {
    if ( str == null )
      str = "";
    var retval = str;
    if ( ( str.length == 7 ) && ( str[ 0 ] == "0" ) && ( str[ 1 ] == ":" ) )
    {
      retval = str.substring( 2, str.length );
    }
    return retval;
  }
}
catch ( err )
{
  alert( "back_scan *script init*\n" + err );
}
