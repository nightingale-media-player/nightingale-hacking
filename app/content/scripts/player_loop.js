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

// Hear ye, hear ye:

// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================

// This file is, like, TOTALLY DEPRECATED, dude.

// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================

// try
if ( false )
{

  var playerLoop_positionData = new sbIDataRemote( "metadata.position" );
  var playerLoop_lengthData = new sbIDataRemote( "metadata.length" );
  var playerLoop_seekbarTracker = new sbIDataRemote( "faceplate.seek.tracker" );
  var playerLoop_resetSearchData = new sbIDataRemote( "faceplate.search.reset" );

  var playerLoop_elapsedTime = new sbIDataRemote( "metadata.position.str" );
  var playerLoop_totalTime = new sbIDataRemote( "metadata.length.str" );
  var playerLoop_showRemainingTime = new sbIDataRemote( "faceplate.showremainingtime" );

  var playerLoop_volume = new sbIDataRemote( "faceplate.volume" );
  var playerLoop_volumeTracker = new sbIDataRemote( "faceplate.volume.tracker" );
  var playerLoop_volumeLastReadback = new sbIDataRemote( "faceplate.volume.lastreadback" );
  var playerLoop_mute = new sbIDataRemote( "faceplate.mute" );
  var playerLoop_seenPlaying = new sbIDataRemote("faceplate.seenplaying");
  var playerLoop_playButton = new sbIDataRemote( "faceplate.play" );

  var playerLoop_playlistRef = new sbIDataRemote( "playlist.ref" );
  var playerLoop_playlistIndex = new sbIDataRemote( "playlist.index" );

  var playerLoop_titleText = new sbIDataRemote( "metadata.title" );
  var playerLoop_artistText = new sbIDataRemote( "metadata.artist" );
  var playerLoop_albumText = new sbIDataRemote( "metadata.album" );
  var playerLoop_faceplateState = new sbIDataRemote( "faceplate.state" );

  var playerLoop_restartOnPlaybackEnd = new sbIDataRemote( "restart.onplaybackend" );

  var playerLoop_shuffle = new sbIDataRemote( "playlist.shuffle" ); 
  var playerLoop_repeat = new sbIDataRemote( "playlist.repeat" ); 

  var playerLoop_playURL = new sbIDataRemote("faceplate.play.url");
  
  // Initialize core poll loop. Called by each new faceplate
  var test = false;
  function SBInitPlayerLoop()
  {
    try
    {
      if ( gPPS.getPlaying() )
      {
        test = true;
        SBStartPlayerLoop();
      }
    }
    catch (err)
    {
      alert( err );
    }
  }
  
  // Set defaults for local remotes, called once at startup
  
  function SBSetPlayerLoopDefaults()
  {
    playerLoop_positionData.SetValue( 0 );
    playerLoop_lengthData.SetValue( 0 );
    playerLoop_seenPlaying.SetValue(false);
    playerLoop_elapsedTime.SetValue( "0:00:00" );
    playerLoop_totalTime.SetValue( "0:00:00" );
    playerLoop_showRemainingTime.SetValue(false);
    playerLoop_volumeTracker.SetValue(false);
    playerLoop_volumeLastReadback.SetValue( -1 );
    playerLoop_mute.SetValue( false );
    playerLoop_playlistRef.SetValue( "" );
    playerLoop_playlistIndex.SetValue( -1 );
    playerLoop_faceplateState.SetValue( 0 );
    playerLoop_restartOnPlaybackEnd.SetValue( false );

    playerLoop_titleText.SetValue( "" );
    playerLoop_artistText.SetValue( "" );
    playerLoop_albumText.SetValue( "" );
  }
  
  // Start polling
  
  var position_interval = null;

  function SBStartPlayerLoop()
  {
    SBStopPlayerLoop();
    playerLoop_seenPlaying.SetValue(false);
    position_interval = setInterval( "onSBPositionLoop()", 250 );
  }
  
  // Stop polling
  
  function SBStopPlayerLoop()
  {
    if ( position_interval )
    {
      clearInterval( position_interval );
    }
  }
  
  // Poll function

  function onSBPositionLoop()
  {
    try
    {  
      var len = gPPS.getLength();
      var pos = gPPS.getPosition();
      
      // Don't update the data if we're currently tracking the seekbar.
      if ( playerLoop_seekbarTracker.GetIntValue() == 0 )
      {
        playerLoop_lengthData.SetValue( len );
        playerLoop_positionData.SetValue( pos );
      }
      
      onSBSetTimeText( len, pos );
      onSBPollVolume();
      onSBPollButtons( len );
      onSBPollMetadata( len, pos );
      
      // Basically, this logic means that posLoop will run until the player first says it is playing, and then stops.
      if ( gPPS.getPlaying() && ( len > 0.0 ) )
      {
        // First time we see it playing, 
        if ( ! playerLoop_seenPlaying.GetIntValue() )
        {
          // Clear the search popup
          playerLoop_resetSearchData.SetValue( playerLoop_resetSearchData.GetIntValue() + 1 );
        }
        // Then remember we saw it
        playerLoop_seenPlaying.SetValue(true);
      }
      // If we haven't seen ourselves playing, yet, we couldn't have stopped.
      else if ( playerLoop_seenPlaying.GetIntValue() )
      {
        // Oh, NOW you say we've stopped, eh?
        playerLoop_seenPlaying.SetValue(false);
        clearInterval( position_interval );
        position_interval = null;
        
        if ( playerLoop_playlistIndex.GetIntValue() > -1 )
        {
          // Play the next playlist entry tree index (or whatever, based upon state.)
          var cur_index = SBGetCurrentPlaylistIndex();
          var num_items = SBGetNumPlaylistItems();
          var next_index = -1;
          if ( cur_index != -1 )
          {
            if ( playerLoop_repeat.GetIntValue() == 1 )
            {
              next_index = cur_index;
            }
            else if ( playerLoop_shuffle.GetIntValue() == 1 )
            {
              var rand = num_items * Math.random();
              next_index = Math.floor( rand );
            }
            else
            {
              next_index = cur_index + 1;
              if ( next_index >= num_items ) // at the end
              {
                if ( playerLoop_repeat.GetIntValue() == 2 )
                {
                  next_index = 0;
                }
                else
                {
                  next_index = -1;
                  // FIXME 
                  // if (playerLoop_restartOnPlaybackEnd.GetIntValue()) restartApp();
                }
              }
            }
          }
          
          if ( next_index != -1 )
          {
            SBPlayPlaylistIndex( next_index );
          }
        }
      }
    }       
    catch ( err )  
    {
      alert( err );
    }
    
  }

  // Elapsed / Total display
 
  function onSBSetTimeText( len, pos )
  {
    try
    {
      playerLoop_elapsedTime.SetValue( EmitSecondsToTimeString( pos / 1000.0 ) + " " );

      if ( ( len > 0 ) && ( playerLoop_showRemainingTime.GetIntValue() ) )
      {
        playerLoop_totalTime.SetValue( "-" + EmitSecondsToTimeString( ( len - pos ) / 1000.0 ) );
      }
      else
      {
        playerLoop_totalTime.SetValue( " " + EmitSecondsToTimeString( len / 1000.0 ) );
      }
    }
    catch ( err )
    {
      alert( err );
    }
  }

  // Just a useful function to parse down some seconds.
  function EmitSecondsToTimeString( seconds )
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
    {
      text += hours + ":";
    }
    if ( minutes < 10 )
    {
      text += "0";
    }
    text += minutes + ":";
    if ( seconds < 10 )
    {
      text += "0";
    }
    text += seconds;
    return text;
  }


  // Routes volume changes from the core to the volume ui data remote 
  // (faceplate sets "faceplate.volume.tracker" to true for bypass while user is changing the volume)
  
  function onSBPollVolume( )
  {
    try
    {
      if ( ! playerLoop_volumeTracker.GetIntValue() )
      {
        var v = gPPS.getVolume();
        if ( v != playerLoop_volumeLastReadback.GetIntValue()) 
        {
          playerLoop_volumeLastReadback.SetValue( -1 );
          playerLoop_volume.SetValue(gPPS.getVolume());
        }
      }
    }
    catch ( err )
    {
    }
  }

  // Routes core playback status changes to the play/pause button ui data remote

  function onSBPollButtons( len )
  {
    if ( gPPS.getPlaying() && ( ! gPPS.getPaused() ) && ( len > 0 ) )
    {
      playerLoop_playButton.SetValue( 0 );
    }
    else
    {
      playerLoop_playButton.SetValue( 1 );
    }
  }

  // Routes metadata (and their possible updates) to the metadata ui data remotes
  // Also updates the database if the core reads metadata that is different from
  // what we had so far
  
  var MetadataPollCount = 0;

  function onSBPollMetadata( len_ms, pos_ms )
  {
    // A two second window to grab metadata after playback starts?
    if ( ( pos_ms > 250 ) /* && ( pos_ms < 1500 ) */ && ( ( MetadataPollCount++ % 20 ) == 0 ) )
    {
      try
      {
        // This is broken.  Frk.
        var title = "" + myPlayerRemote.getMetadata("title");
        var album = "" + myPlayerRemote.getMetadata("album");
        var artist = "" + myPlayerRemote.getMetadata("artist");
        var genre = "" + myPlayerRemote.getMetadata("genre");
        var length = "";
        if ( len_ms >= 0 )
        {
          length = EmitSecondsToTimeString( len_ms / 1000.0 );
        }

        // Glaaaaaaah!        
        if ( title == "null" ) title = "";
        if ( album == "null" ) album = "";
        if ( artist == "null" ) artist = "";
        if ( genre == "null" ) genre = "";
        
        if ( length == "Error" )
        {
          return;
        }
        
        var set_metadata = false;
        if ( title.length && ( playerLoop_titleText.GetValue() != title ) )
        {
          playerLoop_titleText.SetValue( title );
          set_metadata = true;
        }

/*
        //
        // This would suck to rewrite and I don't think we need it.
        // Keeping it around "for awhile, just in case."
        //

        var time = "";
        if ( playerLoop_playlistRef.GetValue().length )
        {
          var source = new sbIPlaylistsource();
          var resultset = source.GetQueryResult( playerLoop_playlistRef.GetValue() );
          if ( resultset )
          {
            var kTime = "length";
            time = resultset.GetRowCellByColumn( playerLoop_playlistIndex.GetIntValue(), kTime );
          }
        }
        
        if ( time != length )
        {
          set_metadata = true;
        }
*/

        if ( set_metadata )
        {
          // Set the metadata into the database table
          SetURLMetadata( playerLoop_playURL.GetValue(), title, length, album, artist, genre, true );
          // Tell the search popup the metadata has changed
          playerLoop_resetSearchData.SetValue( playerLoop_resetSearchData.GetIntValue() + 1 );
        }
        
        playerLoop_artistText.SetValue( artist );
        playerLoop_albumText.SetValue( album );
      }
      catch(err)
      {
        alert(err);
      }
    }
  }
  
  // Updates the database with metadata changes

  function SetURLMetadata( aURL, aTitle, aLength, aAlbum, aArtist, aGenre, boolSync, aDBQuery, execute )
  {
    if ( aDBQuery == null )
    {
      aDBQuery = new sbIDatabaseQuery();
      aDBQuery.SetAsyncQuery(true);
      aDBQuery.SetDatabaseGUID("songbird");
    }
    if ( execute == null )
    {
      execute = true;
    }
      
    if ( aTitle && aTitle.length )
    {
      var q = 'update library set title="'   + aTitle  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aLength && aLength.length )
    {
      var q = 'update library set length="'    + aLength + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aAlbum && aAlbum.length )
    {
      var q = 'update library set album="'  + aAlbum  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aArtist && aArtist.length )
    {
      var q = 'update library set artist="' + aArtist + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aGenre && aGenre.length )
    {
      var q = 'update library set genre="'  + aGenre  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    
    if ( execute )
    {
      var ret = aDBQuery.Execute();
    }
      
    return aDBQuery; // So whomever calls this can keep track if they want.
  }

}
/*
catch ( err )
{
  alert( err );
}
*/