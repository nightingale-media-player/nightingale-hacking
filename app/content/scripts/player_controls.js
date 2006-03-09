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

//
//  Player controls
//

try
{
  // NOW we have the playlist playback service!
  var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
                       .getService(Components.interfaces.sbIPlaylistPlayback);


  // constants
  const kURL = "url";
  const kIndex = "id";
  const kName = "title";
  const kTime = "length";
  const kArtist = "artist";
  const kAlbum = "album";
  const kGenre = "genre";
  const kNumMetadataColumns = 7;
  
  // data remotes
  var playerControls_playButton = new sbIDataRemote( "faceplate.play" );
  var playerControls_playURL = new sbIDataRemote("faceplate.play.url");
  var playerControls_seenPlaying = new sbIDataRemote("faceplate.seenplaying");

  var playerControls_lastVolume = new sbIDataRemote( "faceplate.volume.last" );
  var playerControls_volumeLastReadback = new sbIDataRemote( "faceplate.volume.lastreadback" );
  var playerControls_trackerVolume = new sbIDataRemote( "faceplate.volume.tracker" );
  var playerControls_volume = new sbIDataRemote( "faceplate.volume" );
  var playerControls_muteData = new sbIDataRemote( "faceplate.mute" );

  var playerControls_seekbarTracker = new sbIDataRemote("faceplate.seek.tracker");

  var playerControls_playingRef = new sbIDataRemote( "playing.ref" );
  var playerControls_playlistRef = new sbIDataRemote( "playlist.ref" );
  var playerControls_playlistIndex = new sbIDataRemote( "playlist.index" );

  var playerControls_repeat = new sbIDataRemote( "playlist.repeat" ); 
  var playerControls_shuffle = new sbIDataRemote( "playlist.shuffle" ); 

  var playerControls_showRemaining = new sbIDataRemote( "faceplate.showremainingtime" );

  var playerControls_titleText = new sbIDataRemote( "metadata.title" );
  var playerControls_artistText = new sbIDataRemote( "metadata.artist" );
  var playerControls_albumText = new sbIDataRemote( "metadata.album" );
  var playerControls_playbackURL = new sbIDataRemote( "metadata.url" );

  var playerControls_statusText = new sbIDataRemote( "faceplate.status.text" );
  var playerControls_statusStyle = new sbIDataRemote( "faceplate.status.style" );
  
  // The following variables are not set when this file is loaded by the core window (SBInitPlayerControls not called)
  var theVolumeSlider;
  var theSeekbarSlider;
  
  // Called by each faceplate
  function SBInitPlayerControls()
  {
    theVolumeSlider = document.getElementById( "songbird_volume" );
    theVolumeSlider.maxpos = 255;
    theVolumeSlider.value = 0;
    theVolumeSlider.addEventListener("seekbar-change", onVolumeChange, true);
    theVolumeSlider.addEventListener("seekbar-release", onVolumeUp, true);
    window.addEventListener("DOMMouseScroll", onMouseWheel, false);

    playerControls_seekbarTracker.SetValue(false);
    theSeekbarSlider = document.getElementById( "songbird_seekbar" );
    theSeekbarSlider.maxpos = 0;
    theSeekbarSlider.value = 0;
    theSeekbarSlider.addEventListener("seekbar-change", onSeekbarChange, true);
    theSeekbarSlider.addEventListener("seekbar-release", onSeekbarUp, true);

/*    
    Hmmm, it should be fine for all this data to come from the skin default values.


    if ( ! gPPS.getPlaying() ) 
    {
      var welcome = "Welcome";

      try
      {
        welcome = theSongbirdStrings.getString("faceplate.welcome");
      } catch(e) {}

      playerControls_titleText.SetValue( welcome );
      playerControls_artistText.SetValue( "" );
      playerControls_albumText.SetValue( "" );
      playerControls_statusText.SetValue( "" );
      playerControls_statusStyle.SetValue( "" );
      playerControls_playbackURL.SetValue( "" );
    }
*/
    
    setTimeout( SBFirstVolume, 250 );
  }
  
  // Initialize default values (called only by the core window, not by faceplates)
  
  function SBSetPlayerControlsDefaults()
  {
    playerControls_playButton.SetValue( 1 ); // Start on.
    playerControls_playingRef.SetValue( "" );
    playerControls_repeat.SetValue( 0 ); // start with no shuffle
    playerControls_shuffle.SetValue( 0 ); // start with no shuffle
    playerControls_playURL.SetValue( "" ); 

    playerControls_statusText.SetValue( "" );
    playerControls_statusStyle.SetValue( "" );
    playerControls_playbackURL.SetValue( "" );
  }

  function SBFirstVolume()
  {
    if ( playerControls_lastVolume.GetValue() == "" )
    {
      playerControls_lastVolume.SetValue( 128 );
    }
    gPPS.setVolume( playerControls_lastVolume.GetIntValue() );
    var mute = playerControls_muteData.GetIntValue() != 0;
    setMute( mute );
    onSBPollVolume();
  }

  function onVolumeChange( event ) 
  {
    playerControls_trackerVolume.SetValue(true);
    if ( theVolumeSlider.value > 0 )
    {
      playerControls_muteData.SetValue( false );
      gPPS.setMute( false );
    }
    else
    {
      playerControls_muteData.SetValue( true );
      gPPS.setMute( true );
    }
    gPPS.setVolume( theVolumeSlider.value );
  }

  function onVolumeUp( event ) 
  {
    if ( theVolumeSlider.value > 0 )
    {
      playerControls_muteData.SetValue( false );
      gPPS.setMute( false );
      playerControls_lastVolume.SetValue( theVolumeSlider.value );
    }
    else
    {
      playerControls_muteData.SetValue( true );
      gPPS.setMute( true );
    }
    gPPS.setVolume( theVolumeSlider.value );
    playerControls_volumeLastReadback.SetValue(gPPS.getVolume());
    playerControls_trackerVolume.SetValue(false);
  }

  function onMouseWheel(event)
  {
    try
    {
      var node = event.originalTarget;
      while (node != document && node != null)
      {
        // if your object implements an event on the wheel,
        // but is not one of these, you should prevent the 
        // event from bubbling
        if (node.tagName == "tree") return;
        if (node.tagName == "xul:tree") return;
        if (node.tagName == "listbox") return;
        if (node.tagName == "xul:listbox") return;
        if (node.tagName == "browser") return;
        if (node.tagName == "xul:browser") return;
        node = node.parentNode;
      }

      if (node == null)
      {
        // could not walk up to the window before hitting a document, 
        // we're inside a sub document. the event will continue bubbling, 
        // and we'll catch it on the next pass
        return;
      }
    
      // walked up to the window
      var s = theVolumeSlider.value;
      if (s == '') s = playerControls_volume.GetValue();
      var v = parseInt(s)+((event.detail > 0) ? -8 : 8);
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      theVolumeSlider.value = v;
      onVolumeChange();
      onVolumeUp();
    }
    catch (err)
    {
      alert("onMouseWheel - " + err);
    }
  }

  function onPlay()
  {
    if ( playerControls_seenPlaying.GetIntValue() && gPPS.getPaused() )
    {
      gPPS.play();
    }
    else
    {
      playCurrentUrl( false );
    }
  }

  function playCurrentUrl( setdisplay )
  {
    // Hide the intro box and show the normal faceplate box
    sbIDataRemote( "faceplate.state" ).SetValue( 1 );
    
    // No current url?! naughty naughty!
    if ( playerControls_playURL.GetValue().length == 0 )
    {
      if ( thePlaylistTree ) // OPTIONAL CONNECTION
      {
        var idx = thePlaylistTree.currentIndex;
        if ( idx == -1 )
        {
          idx = 0;
        }
        SBPlayPlaylistIndex( idx );
      }
      else
      {
        if (LaunchMainPaneURL) // OPTIONAL CONNECTION -- this may fail because the faceplate's onMainPaneLoad may take time to retrieve a reference to the playlist tree
        {
          LaunchMainPaneURL( "chrome://songbird/content/xul/main_pane.xul?library" );
          SBPlayPlaylistIndex( 0 );
        }
      }
      return;
    }

    // Stop whatever is currently playing.
    gPPS.stop();
    // Just play the url.
    gPPS.playUrl( playerControls_playURL.GetValue() );
    // Tell __EVERYBODY__
    playerControls_playbackURL.SetValue( playerControls_playURL.GetValue() )
    // Set our timeout tracking variable
    playerControls_seenPlaying.SetValue(false);
    
    // Assign to the display if that is desired
    if ( setdisplay )
    {
      theTitleText.SetValue( ConvertUrlToDisplayName( playerControls_playURL.GetValue() ) );
      theAlbumText.SetValue( "" );
      theArtistText.SetValue( "" );
    }
    // If we've never seen it before, put it in the library
    SBAddUrlToDatabase( playerControls_playURL.GetValue() );
  }

  function ConvertUrlToDisplayName( url )
  {
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
    // Convert any %XX to space
    var percent = the_value.indexOf('%');
    if ( percent != -1 )
    {
      var remainder = the_value;
      the_value = "";
      while ( percent != -1 )
      {
        the_value += remainder.substring( 0, percent );
        remainder = remainder.substring( percent + 3, url.length );
        percent = remainder.indexOf('%');
        the_value += " ";
        if ( percent == -1 )
        {
          the_value += remainder;
        }
      }
    }
    if ( ! the_value.length )
    {
      the_value = url;
    }
    return the_value;
  }

  function setMute( theMute )
  {
try
{  
    if ( theMute )
    {
      playerControls_lastVolume.SetValue( theVolumeSlider.value );
      theVolumeSlider.value = 0;
    }
    else
    {
      var value = playerControls_lastVolume.GetValue();
      theVolumeSlider.value = playerControls_lastVolume.GetValue();
    }
    playerControls_muteData.SetValue( theMute );
    //alert( theMute ); ??
    gPPS.setMute( theMute );
    gPPS.setVolume( theVolumeSlider.value );
    playerControls_volumeLastReadback.SetValue(gPPS.getVolume());
}
catch ( err )    
{
  alert( err );
}
  }
  
  function onMute()
  {
    var theMute = ! gPPS.getMute();
    setMute( theMute );
  }

  function SBAddUrlToDatabase( the_url )
  {
    retval = -1;
    try
    {
      aDBQuery = Components.classes["@songbird.org/Songbird/DatabaseQuery;1"];
      if (aDBQuery)
      {
        aDBQuery = aDBQuery.createInstance();
        aDBQuery = aDBQuery.QueryInterface(Components.interfaces.sbIDatabaseQuery);
        
        if ( ! aDBQuery )
        {
          return -1;
        }
        aDBQuery.SetAsyncQuery(false);
        aDBQuery.SetDatabaseGUID("testdb-0000");
        if ( ! SBUrlExistsInDatabase( the_url ) )
        {
          var display_name = ConvertUrlToDisplayName( the_url );
          var str = "insert into test (url, name) values " + '("' + the_url + '", "' + display_name + '") ';
          aDBQuery.AddQuery( str );
          ret = aDBQuery.Execute();
          retval = SBGetNumPlaylistItems() - 1; // the end
        }
        else // Find it and set the playlist index.
        {
          aDBQuery.AddQuery( "select * from test" );
          ret = aDBQuery.Execute();
          
          var resultset = aDBQuery.GetResultObject();
          
          var i;
          for ( i = 0; i < resultset.GetRowCount(); i++ )
          {
            var track_url = resultset.GetRowCellByColumn( i, kURL );
            if ( track_url == the_url )
            {
              retval = i;
              break;
            }
          }
        }
      }
      else
      {
        alert( "@songbird.org/Songbird/DatabaseQuery;1 -- Failed" );
      }
    }
    catch(err)
    {
      alert(err);
    }
    return retval;
  }

  function SBUrlExistsInDatabase( the_url )
  {
    var retval = false;
    try
    {
      aDBQuery = Components.classes["@songbird.org/Songbird/DatabaseQuery;1"];
      if (aDBQuery)
      {
        aDBQuery = aDBQuery.createInstance();
        aDBQuery = aDBQuery.QueryInterface(Components.interfaces.sbIDatabaseQuery);
        
        if ( ! aDBQuery )
        {
          return;
        }
        
        aDBQuery.SetAsyncQuery(false);
        aDBQuery.SetDatabaseGUID("testdb-0000");
        aDBQuery.AddQuery('select * from test where url="' + the_url + '"' );
        var ret = aDBQuery.Execute();
        
        resultset = aDBQuery.GetResultObject();
       
        // we didn't find anything that matches our url
        if ( resultset.GetRowCount() != 0 )
        {
          retval = true;
        }
      }
    }
    catch(err)
    {
      alert(err);
    }
    return retval;
  }

  // The Seek Bar

  function onSeekbarChange( ) 
  {
    playerControls_seekbarTracker.SetValue(true);
  }

  function onSeekbarUp( ) 
  {
    gPPS.setPosition( theSeekbarSlider.value );
    playerControls_seekbarTracker.SetValue(false);
  }

  // Fwd and back skipping.  Pleah.
  var skip = false;
  var seen_skip = false;
  var skip_time_min = 300;
  var skip_time_step = 25;
  var skip_time = 0;
  var skip_direction = 1;
  var skip_interval = null;
  
  function onBackDown()
  {
    seen_skip = false;
    if ( gPPS.getPlaying() )
    {
      skip = true;
      skip_direction = -1;
      if ( ! skip_interval )
      {
        skip_interval = setInterval( "SBSkipLoop()", 1000 );
      }
    }
  }

  function onBackUp()
  {
    skip = false;
  }

  function onBack()
  {
    if ( !seen_skip )
    {
      var num_items = SBGetNumPlaylistItems();
      var cur_index = SBGetCurrentPlaylistIndex();
      var index = -1;
      if ( playerControls_repeat.GetIntValue() == 1 )
      {
        index = cur_index;
      }
      else if ( cur_index != -1 )
      {
        index = ( ( cur_index - 1 ) + num_items ) % num_items;
      }
      if ( index != -1 )
      {
        SBPlayPlaylistIndex( index );
      }
    }
  }
  function onFwdDown()
  {
    seen_skip = false;
    if ( gPPS.getPlaying() )
    {
      skip = true;
      skip_direction = 1;
      if ( ! skip_interval )
      {
        skip_interval = setInterval( "SBSkipLoop()", 1000 );
      }
    }
  }
  function onFwdUp()
  {
    skip = false;
  }
  function onFwd()
  {
    if ( !seen_skip )
    {
      var num_items = SBGetNumPlaylistItems();
      var cur_index = SBGetCurrentPlaylistIndex();
      var index = -1;
      if ( cur_index != -1 )
      {
        if ( playerControls_repeat.GetIntValue() == 1 )
        {
          index = cur_index;
        }
        else if ( playerControls_shuffle.GetIntValue() )
        {
          var rand = num_items * Math.random();
          index = parseInt( rand );
        }
        else    
        {
          index = ( cur_index + 1 ) % num_items;
        }
        
        SBPlayPlaylistIndex( index );
      }
    }
  }
  function SBSkipLoop( )
  {
    if ( skip )
    {
      seen_skip = true;
      gPPS.setPosition( gPPS.getPosition() + ( 15000 * skip_direction ) );
    }
    else
    {
      clearInterval( skip_interval );
      skip_interval = null;
    }
  }

  // Shuffle state 
  function onShuffle()
  {
    playerControls_shuffle.SetValue( ( playerControls_shuffle.GetIntValue() + 1 ) % 2 );
  }

  function SBGetNumPlaylistItems()
  {
    var retval = 0;
    if ( playerControls_playingRef.GetValue().length )
    {
      var source = new sbIPlaylistsource();
      retval = source.GetRefRowCount( playerControls_playingRef.GetValue() );
    }
    else if ( thePlaylistTree )
    {
      // OPTIONAL CONNECTION
      retval = thePlaylistTree.view.rowCount;
    }
    return retval;  
  }

  function SBGetCurrentPlaylistIndex()
  {
    var retval = -1;
    if ( playerControls_playingRef.GetValue().length > 0 )
    {
      var source = new sbIPlaylistsource();
      var ref = playerControls_playingRef.GetValue();
      
      retval = source.GetRefRowByColumnValue( ref, kURL, playerControls_playURL.GetValue() );
    }
    return retval;
  }

  // Repeat state
  function onRepeat()
  {
    // Rob decided to change the order.  Woo.
    var value = 0;
    switch ( playerControls_repeat.GetIntValue() )
    {
      case 0:
        value = 2;
        break;
      case 1:
        value = 0;
        break;
      case 2:
        value = 1;
        break;
    }
    playerControls_repeat.SetValue( value );
  }

  function SBPlayPlaylistIndex( index, playlist )
  {
    try
    {
      var ref = "";
      if ( playlist == null )
      {
        ref = playerControls_playingRef.GetValue();
      }
      else
      {
        ref = playlist.ref;
      }
      
      var play_url = "";
      var play_name = "";
      var play_artist = "";
      var play_album = "";

      if ( ref.length )
      {
        var source = new sbIPlaylistsource();
        
        // Use it to get the important information.
        play_url = source.GetRefRowCellByColumn( ref, index, kURL );
        play_name = source.GetRefRowCellByColumn( ref, index, kName );
        if ( !play_name )
        {
          play_name = "";
        }
        play_artist = source.GetRefRowCellByColumn( ref, index, kArtist );
        if ( !play_artist )
        {
          play_artist = "";
        }
        play_album = source.GetRefRowCellByColumn( ref, index, kAlbum );
        if ( !play_album )
        {
          play_album = "";
        }
      }

      // And if it's good, launch it.
      if ( play_url && play_url.length > 0 )
      {
        playerControls_playURL.SetValue( play_url );
        if ( play_name.length > 0 )
        {
          playerControls_titleText.SetValue( play_name );
          playerControls_artistText.SetValue( play_artist );
          playerControls_albumText.SetValue( play_album );
        }
        else
        {
          playerControls_titleText.SetValue( playerControls_playURL.GetValue() );
          playerControls_artistText.SetValue( "" );
          playerControls_albumText.SetValue( "" );
        }
        // Now this guy takes over and plays what needs to be played.
        gPPS.playRef( ref, index );
        // Hide the intro box and show the normal faceplate box
        SBDataSetValue( "faceplate.state", 1 );
        SBSyncPlaylistIndex();
      }
    }
    catch ( err )
    {
      alert( err );
    }
  }

  function SBSyncPlaylistIndex()
  {
    try 
    {
      // OPTIONAL CONNECTION
      var tree = thePlaylistTree;
      if ( !tree )
      {
        // EVEN MORE OPTIONAL CONNECTION
        if (theWebPlaylist) tree = theWebPlaylist.tree;
      }
      if ( tree )
      {
        if ( ! tree.view.rowCount )
        {
          // Keep trying.
          setTimeout( SBSyncPlaylistIndex, 250 );
        }
        else
        {
          // Only if we're looking at the same playlist from which we are playing
          if ( playerControls_playlistRef.GetValue() == playerControls_playingRef.GetValue() )
          {
            var index = SBGetCurrentPlaylistIndex();
            tree.view.selection.clearSelection();
            tree.view.selection.currentIndex = index;
            if ( index > -1 )
            {
              tree.view.selection.select( index );
              tree.treeBoxObject.ensureRowIsVisible( index );
            }
          }
        }
      }
    }
    catch (err)
    {
      // The XBL object has not yet been fully created (grrrr...), so keep trying.
      setTimeout( SBSyncPlaylistIndex, 250 );
    }
  }

  function onPause()
  {
    
    if ( playerControls_seenPlaying.GetIntValue() )
    {
      gPPS.pause();

      // Humm ... I wonder ...
      // if (document.restartOnPlaybackEnd) restartApp();
    }
  }

  function onTotalDown()
  {
    var len = gPPS.getLength();
    var pos = gPPS.getPosition();
    if ( len > 0 )
    {
      playerControls_showRemaining.SetValue(!playerControls_showRemaining.GetIntValue());
      SBSetTimeText( len, pos );
    }
    // If you try to toggle it while it is zero, you lose the state.
    else
    {
      playerControls_showRemaining.SetValue(false);
    }
  }
  
}
catch ( err )
{
  alert( err );
}
