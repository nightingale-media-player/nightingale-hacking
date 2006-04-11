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
  
//  const sbDatabaseGUID = "songbird";
  const sbDatabaseGUID = "*";

  const MetadataManager = new Components.Constructor("@songbird.org/Songbird/MetadataManager;1", "sbIMetadataManager");
  var aMetadataManager = new MetadataManager();
  const MediaLibrary = new Components.Constructor("@songbird.org/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  var aMediaLibrary = new MediaLibrary();
  const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer", "service_uuid");
  const bsDBGuidIdx = 7;

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
  var bsSubmitQueries = false;
  
  var bsMaxArray = 20; // How many tracks to enqueue before writing to the db.
  var bsMaxArrayMax = 20;
  var bsMaxArrayMin = 1;
  
  function BSCalcMaxArray( items )
  {
    bsMaxArray = parseInt( items / 10 );
    bsMaxArray = ( bsMaxArray > bsMaxArrayMax ) ? bsMaxArrayMax : bsMaxArray;
    bsMaxArray = ( bsMaxArray < bsMaxArrayMin ) ? bsMaxArrayMin : bsMaxArray;
  }

  var bsScanningText = new sbIDataRemote( "backscan.status" );
  var bsPlaylistRef = new sbIDataRemote( "playlist.ref" );
  var bsSongbirdStrings = document.getElementById( "songbird_strings" );
  var bsScanningPaused = new sbIDataRemote( "backscan.paused" );
  
  var bsQueryString = "SELECT uuid, url, length, title, service_uuid FROM library where length=\"0\"";
  
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
          if ( bsSubmitQueries )
          {
            BSSubmitQueries();
            return;
          }
          
          // If we're at the end of the list,
          var result = bsDBQuery.GetResultObject();
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
   
    if ( ! bsSubmitQueries && ( bsLastRow < result.GetRowCount() ) ) 
    {
      var scanning = "Scanning";
      try
      {
        scanning = bsSongbirdStrings.getString("back_scan.scanning");
      } catch(e) {}
      bsScanningText.SetValue( scanning + "..." );
      
      var url = result.GetRowCellByColumn( bsLastRow, "url" );
      bsMDHandler = aMetadataManager.GetHandlerForMediaURL(url);
      var retval = bsMDHandler.Read();
      // If it's immediately done, read the values.
      if ( bsMDHandler.Completed() )
        BSReadHandlerValues();
    }
  }

  function BSSubmitQueries()
  {
    bsSubmitQueries = false;
    var anything = false;
    
    if ( bsMetadataArray.length > 0 )
    {
      var dbGUID = bsMetadataArray[ 0 ][ bsDBGuidIdx ];
      
      var saveGUIDArray = []; // To be saved for next time
      var saveMetadataArray = [];
      var workGUIDArray = []; // To be worked on now
      var workMetadataArray = [];
      // We can only submit for one database at a time.
      for ( var i = 0; i < bsMetadataArray.length; i++ )
      {
        if ( bsMetadataArray[ i ][ bsDBGuidIdx ] == dbGUID )
        {
          // So pull all the items that match the first item.
          workGUIDArray.push( bsGUIDArray[ i ] );
          workMetadataArray.push( bsMetadataArray[ i ] );
        }
        else
        {
          // And save for later the ones that don't.
          saveGUIDArray.push( bsGUIDArray[ i ] );
          saveMetadataArray.push( bsMetadataArray[ i ] );
        }
      }      
      bsGUIDArray = workGUIDArray;
      bsMetadataArray = workMetadataArray;
    
      // Prepare the query
      bsCurQuery.ResetQuery();
      bsCurQuery.SetDatabaseGUID( dbGUID );
      aMediaLibrary.SetQueryObject(bsCurQuery);
      
      // Add the metadata
      for ( var i = 0; i < bsMetadataArray.length; i++ )
      {
        // Go submit the metdadata update, and stash the query so we know when the update is done.
        aMediaLibrary.SetValuesByGUID( bsGUIDArray[i], keys.length, keys, bsMetadataArray[i].length, bsMetadataArray[i], true );
      }
      bsCurQuery.Execute();

      // See who is next.  Maybe nobody.      
      bsGUIDArray = saveGUIDArray;
      bsMetadataArray = saveMetadataArray;
    }
    // And if there's somebody still next, keep submitting until we're empty.
    if ( bsMetadataArray.length > 0 )
    {
      bsSubmitQueries = true;
    }    
  }
  
  function BSReadHandlerValues()
  {
    var values = bsMDHandler.GetValuesMap();
    // clear the bsMDHandler variable so we don't track it.
    bsMDHandler.Close();
    bsMDHandler = null;

    var result = bsDBQuery.GetResultObject();
    var time = result.GetRowCellByColumn( bsLastRow, "length" );
    var title = result.GetRowCellByColumn( bsLastRow, "title" );
    var uuid = result.GetRowCellByColumn( bsLastRow, "uuid" );
    var url = result.GetRowCellByColumn( bsLastRow, "url" );
    var service_uuid = result.GetRowCellByColumn( bsLastRow, "service_uuid" );
    values.setValue( "service_uuid", service_uuid, 0 ); // Pretend like it came from the file.
    BSCalcMaxArray( result.GetRowCount() );
    
    var text = "";
    var metadata = new Array();
    for ( var i in keys )
    {
      metadata[ i ] = values.getValue( keys[ i ] );
      text += keys[ i ] + ": " + metadata[ i ] + "\n";
    }
    //alert(text);
    
    // GRRRRR.
    for ( var i in metadata )
    {
      if ( metadata[i] == null )
      {
        metadata[i] = "";
      }
    }

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
    
    // If we get to the end of the list and we have things to submit, submit them!
    if ( ( bsLastRow == result.GetRowCount() ) && ( bsGUIDArray.length > 0 ) )
    {
      bsSubmitQueries = true;
    }
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
