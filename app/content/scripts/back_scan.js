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
 
try 
{
  var ENABLE_BACKSCAN = 1;
  
//  const sbDatabaseGUID = "songbird";
  const sbDatabaseGUID = "*";

  const MetadataManager = new Components.Constructor("@songbirdnest.com/Songbird/MetadataManager;1", "sbIMetadataManager");
  var aMetadataManager = new MetadataManager();
  const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
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

  var bsScanningText = SB_NewDataRemote( "backscan.status", null );
  var bsPlaylistRef = SB_NewDataRemote( "playlist.ref", null );
  var bsScanningPaused = SB_NewDataRemote( "backscan.paused", null );
  var bsSongbirdStrings = document.getElementById( "songbird_strings" );
  
  var bsQueryString = "SELECT uuid, url, length, title, service_uuid FROM library where length=\"0\"";
  
  // Init the text box to the last url played (shrug).
  function BSInitialize()
  {
    try
    {
      if ( ENABLE_BACKSCAN )
      {
        // Bind a function to let us pause and unpause
        var bs_data_change = {
          observe: function ( aSubject, aTopic, aData) { BSDataChange(aData); }
        };
        bsScanningPaused.bindObserver( bs_data_change, true );
      
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

  function BSDeInitialize()
  {
    try {
      bsScanningPaused.unbind();
    }
    catch ( err ) {
      dump("ERROR! SBDeInitialized()\n" + err + "\n");
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
      
      // If we're waiting on an asynchronous read,
      if ( bsMDHandler )
      {
        // Ask if it is done.
        if ( bsMDHandler.GetCompleted() )
        {
          // Always submit after an http read.  (for now?)
          var channel = bsMDHandler.GetChannel();
          if ( channel && channel.URI.scheme.indexOf( "http" ) != -1 )
            bsSubmitQueries = true;
          BSReadHandlerValues();
        }
        return;
      }

      // People can pause us
      if ( ! bsScanningPaused.intValue )
      {
        // So can executing queries, or the media scan.
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
            bsScanningText.stringValue = "";
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
    if ( SBDataGetIntValue( "media_scan.open" ) == 0 )
    {
      bsDBQuery.ResetQuery();
      bsDBQuery.AddQuery( bsQueryString );
      var ret = bsDBQuery.Execute();
      bsWaitForExecuting = false;
      bsLastRow = 0;
    }
    else
    {
      // Try again in a bit.
      setTimeout( BSExecute, 2000 );
    }
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
      bsScanningText.stringValue = scanning + "...";
      
      var bad_url = false;
      var url = result.GetRowCellByColumn( bsLastRow, "url" );
      try
      {
        bsMDHandler = aMetadataManager.GetHandlerForMediaURL(url);
      } 
      catch(e)
      {
        bad_url = true; // If the url is bad, this will catch.
      }
      
      if ( bad_url || !bsMDHandler )
      {
        BSReadHandlerValues( true );
      }
      else
      {
        // Phew -- maybe it's okay to read, now?
        var retval = bsMDHandler.Read();
        // If it's immediately done, read the values.
        if ( bsMDHandler.GetCompleted() )
          BSReadHandlerValues();
      }
    }
  }

  function BSSubmitQueries()
  {
    bsSubmitQueries = false;
    var anything = false;
    
    // Make sure there's something to do
    if ( bsMetadataArray.length > 0 )
    {
      // Get the database guid of the first entry in the list
      var dbGUID = bsMetadataArray[ 0 ][ bsDBGuidIdx ];
      
      var workGUIDArray = []; // To be worked on now
      var workMetadataArray = [];
      var saveGUIDArray = []; // To be saved for next time
      var saveMetadataArray = [];
      
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
  
  // If blank is true, we've failed.  Just create an empty entry for this item.
  function BSReadHandlerValues( blank ) 
  {
    var values = null;
    if ( ! blank && bsMDHandler )
    {
      values = bsMDHandler.GetValuesMap();
      // clear the bsMDHandler variable so we don't track it.
      bsMDHandler.Close();
      bsMDHandler = null;
    }

    var result = bsDBQuery.GetResultObject();
    var time = result.GetRowCellByColumn( bsLastRow, "length" );
    var title = result.GetRowCellByColumn( bsLastRow, "title" );
    var uuid = result.GetRowCellByColumn( bsLastRow, "uuid" );
    var url = result.GetRowCellByColumn( bsLastRow, "url" );
    var service_uuid = result.GetRowCellByColumn( bsLastRow, "service_uuid" );
//    BSCalcMaxArray( result.GetRowCount() ); // leave it at 20.
    
    var metadata = new Array();
    if ( ! blank )
    {
      values.setValue( "service_uuid", service_uuid, 0 ); // Pretend like it came from the file.
      var text = "";
      for ( var i in keys )
      {
        metadata[ i ] = values.getValue( keys[ i ] );
        text += keys[ i ] + ": " + metadata[ i ] + "\n";
      }
      //alert(text);
    }
    else
    {
      // Setup a blank entry.
      for ( var i in keys )
      {
        metadata[ i ] = "";
      }
    }
    
    // GRRRRR.
    for ( var i in metadata )
    {
      if ( metadata[i] == null )
      {
        metadata[i] = "";
      }
    }

    // If the title is null, use the title
    if ( metadata[0] == "" )
    {
      metadata[0] = title;
    }
    // If the service_uuid is null, use the service_uuid;
    if ( metadata[ bsDBGuidIdx ] == "" )
    {
      metadata[ bsDBGuidIdx ] = service_uuid;
    }
    // If the length is null parse a 0, otherwise strip hours. 
    if ( metadata[1] == "" )
    {
      metadata[1] = "0:00";
    }
    else
    {
      metadata[1] = BSEmitSecondsToTimeString( metadata[1] / 1000 );
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
  
  function BSDataChange( aIsPaused )
  {
    var scanning = "Scanning";
    var paused = "Paused";
    try {
      scanning = bsSongbirdStrings.getString("back_scan.scanning");
      paused = bsSongbirdStrings.getString("back_scan.paused");
    } catch(e) { /* ignore the error, we have default strings */ }

    if ( bsScanningText.stringValue.length )
    {
      if ( aIsPaused )
      {
        bsScanningText.stringValue = scanning + " " + paused;
      }
      else
      {
        bsScanningText.stringValue = scanning + "...";
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

  // Just a useful function to parse down some seconds.
  function BSEmitSecondsToTimeString( seconds )
  {
    if ( seconds < 0 )
      return "00:00";
    seconds = parseFloat( seconds );
    var minutes = parseInt( seconds / 60 );
    seconds = parseInt( seconds ) % 60;
    var hours = parseInt( minutes / 60 );
    if ( hours > 50 ) // lame
      return "Error";
    minutes = parseInt( minutes ) % 60;
    var text = ""
    if ( hours > 0 )
      text += hours + ":";
    if ( minutes < 10 )
      text += "0";
    text += minutes + ":";
    if ( seconds < 10 )
      text += "0";
    text += seconds;
    return text;
  }
}
catch ( err )
{
  alert( "back_scan *script init*\n" + err );
}
