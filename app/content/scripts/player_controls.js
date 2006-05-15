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

  var playerControls_playingRef = new sbIDataRemote( "playing.ref" );
  var playerControls_playlistRef = new sbIDataRemote( "playlist.ref" );
  var playerControls_playlistIndex = new sbIDataRemote( "playlist.index" );

  var playerControls_titleText = new sbIDataRemote( "metadata.title" );
  var playerControls_artistText = new sbIDataRemote( "metadata.artist" );
  var playerControls_albumText = new sbIDataRemote( "metadata.album" );
  var playerControls_playbackURL = new sbIDataRemote( "metadata.url" );

  var playerControls_statusText = new sbIDataRemote( "faceplate.status.text" );
  var playerControls_statusStyle = new sbIDataRemote( "faceplate.status.style" );
  
  // Called by each faceplate
  function SBInitPlayerControls()
  {
  }

  // Initialize default values (called only by the core window, not by faceplates)
  
  function SBSetPlayerControlsDefaults()
  {
    playerControls_playButton.setValue( 1 ); // Start on.
    playerControls_playingRef.setValue( "" );
    playerControls_playURL.setValue( "" ); 
    playerControls_statusText.setValue( "" );
    playerControls_statusStyle.setValue( "" );
    playerControls_playbackURL.setValue( "" );
  }

  function onPlay()
  {
    if ( playerControls_seenPlaying.getIntValue() && gPPS.getPaused() )
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
    sbIDataRemote( "faceplate.state" ).setValue( 1 );
    
    // No current url?! naughty naughty!
    if ( playerControls_playURL.getValue().length == 0 )
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
    gPPS.playUrl( playerControls_playURL.getValue() );
    // Tell __EVERYBODY__
    playerControls_playbackURL.setValue( playerControls_playURL.getValue() )
    // Set our timeout tracking variable
    playerControls_seenPlaying.setValue(false);
    
    // Assign to the display if that is desired
    if ( setdisplay )
    {
      theTitleText.setValue( ConvertUrlToDisplayName( playerControls_playURL.getValue() ) );
      theAlbumText.setValue( "" );
      theArtistText.setValue( "" );
    }
    // If we've never seen it before, put it in the library
    SBAddUrlToDatabase( playerControls_playURL.getValue() );
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

  function log(str)
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage( str );
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
          return false;
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

  function SBGetNumPlaylistItems()
  {
    var retval = 0;
    if ( playerControls_playingRef.getValue().length )
    {
      var source = new sbIPlaylistsource();
      retval = source.GetRefRowCount( playerControls_playingRef.getValue() );
    }
    else if ( thePlaylistTree )
    {
      // OPTIONAL CONNECTION
      retval = thePlaylistTree.view.rowCount;
    }
    return retval;  
  }

  function SBPlayPlaylistIndex( index, playlist )
  {
    try
    {
      var ref = "";
      if ( playlist == null )
      {
        ref = playerControls_playingRef.getValue();
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
        playerControls_playURL.setValue( play_url );
        if ( play_name.length > 0 )
        {
          playerControls_titleText.setValue( play_name );
          playerControls_artistText.setValue( play_artist );
          playerControls_albumText.setValue( play_album );
        }
        else
        {
          playerControls_titleText.setValue( playerControls_playURL.getValue() );
          playerControls_artistText.setValue( "" );
          playerControls_albumText.setValue( "" );
        }
        // Now this guy takes over and plays what needs to be played.
        gPPS.playRef( ref, index );
        // Hide the intro box and show the normal faceplate box
        SBDataSetValue( "faceplate.state", 1 );
      }
    }
    catch ( err )
    {
      alert( err );
    }
  }

  function onPause()
  {
    
    if ( playerControls_seenPlaying.getIntValue() )
    {
      gPPS.pause();
    }
  }
}
catch ( err )
{
  alert( "player_controls.js\n" + err );
}

