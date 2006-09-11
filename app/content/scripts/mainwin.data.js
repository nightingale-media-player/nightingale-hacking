/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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
//  This file binds UI objects from the mainwin.xul
//  to data remotes that will automatically update them to reflect the proper value.
//

try
{
  function onSBMainwinDataLoad()
  {
    //SB_LOG("onSBMainwinDataLoad", "");
    try
    {
      // Do the magic binding stuff here.
      //
      // SBDataBindElementProperty Param List
      //  1 - The data ID to bind upon
      //  2 - The element ID to be bound
      //  3 - The name of the property or attribute to change
      //  4 - Optionally assign the data as BOOLEAN data (true/false props, "true"/"false" attrs)
      //  5 - Optionally assign the data as a boolean NOT of the value
      //  6 - Optionally apply an eval string where `value = eval( "eval_string" );`

      // Playlist Items
      MainwinAdd( SBDataBindElementAttribute( "playlist.shuffle", "control.shuf", "checked", true ) );

      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.repx", "checked", true, true, "parseInt( value ) != 0" ) );
      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.rep1", "checked", true, true, "parseInt( value ) != 1" ) );
      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.repa", "checked", true, true, "parseInt( value ) != 2" ) );      
     
      // Faceplate Items    
      MainwinAdd( SBDataBindElementAttribute( "faceplate.state", "intro_box", "hidden", true ) );
      MainwinAdd( SBDataBindElementAttribute( "faceplate.state", "dashboard_box", "hidden", true, true ) );

     
      MainwinAdd( SBDataBindElementProperty ( "faceplate.search.reset", "search_widget", "reset" ) );
      
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "spinner_stopped", "hidden", true ) );
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "spinner_spin", "hidden", true, true ) );
      
      MainwinAdd( SBDataBindElementProperty ( "faceplate.loading", "status_progress", "mode", false, false, "if ( value == '1' ) value = 'undetermined'; else value = ''; value;" ) );
      
      // Browser Items
      MainwinAdd( SBDataBindElementAttribute( "browser.cangoback", "browser_back", "disabled", true, true ) );
      MainwinAdd( SBDataBindElementAttribute( "browser.cangofwd", "browser_fwd", "disabled", true, true ) );
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "browser_stop", "disabled", true, true ) );
      //MainwinAdd( SBDataBindElementAttribute( "browser.canplaylist", "browser_playlist", "disabled", true, true ) );
      //MainwinAdd( SBDataBindElementAttribute( "browser.hasdownload", "browser_download", "disabled", true, true ) );
      MainwinAdd( SBDataBindElementProperty ( "browser.url.text", "browser_url", "value" ) );
      MainwinAdd( SBDataBindElementProperty ( "browser.url.image", "browser_url_image", "src" ) );
      MainwinAdd( SBDataBindElementProperty ( "browser.playlist.show", "playlist_web", "hidden", true, true ) );
      MainwinAdd( SBDataBindElementProperty ( "browser.playlist.show", "playlist_web_split", "hidden", true, true ) );
      
      // Options
//      MainwinAdd( SBDataBindElementAttribute( "option.htmlbar", "file.htmlbar", "checked", true ) );

      // Complex Data Items
      onSBMainwinComplexLoad();
      
      // Events
      document.addEventListener( "search", onSBMainwinSearchEvent, true ); // this didn't work when we put it directly on the widget?
      
//      SBInitPlayerLoop();
    }
    catch ( err )
    {
      alert( err );
    }
  }
      
  // The bind function returns a remote, 
  // we keep it here so there's always a ref
  // and we stay bound till we unload.
  var MainwinDataRemotes = new Array();
  function MainwinAdd( remote )
  {
    if ( remote )
    {
      MainwinDataRemotes.push( remote );
    }
  }

  function onSBMainwinDataUnload()
  {
    //SB_LOG("onSBMainwinDataUnload", "");
    try
    {
      // Unbind the observers!
      for ( var i = 0; i < MainwinDataRemotes.length; i++ )
      {
        MainwinDataRemotes[ i ].unbind();
        MainwinDataRemotes[ i ] = null;
      }
    }  
    catch ( err )
    {
      alert( err );
    }
  }
  
  //
  // Complex Implementations (can't be handled by a simple eval)
  //
  
  var MainwinArtistRemote = null;
  var MainwinAlbumRemote = null;
  function onSBArtistAlbumChanged()
  {
    // (we get called before they are fully bound)
    if ( MainwinArtistRemote && MainwinAlbumRemote )
    {
      var artist = MainwinArtistRemote.stringValue;
      var album = MainwinAlbumRemote.stringValue;
      var theAASlash = document.getElementById( "songbird_text_slash" );
      var theAABox = document.getElementById( "songbird_text_artistalbum" );
      if ( album.length || artist.length )
      {
        if ( album.length && artist.length )
        {
          theAASlash.setAttribute( "hidden", "false" );    
        }
        else
        {
          theAASlash.setAttribute( "hidden", "true" );    
        }
        theAABox.setAttribute( "hidden", "false" );
      }
      else
      {
        theAASlash.setAttribute( "hidden", "true" );
        // theAABox.setAttribute( "hidden", "true" ); // use this to center the seekbar & title
        theAABox.setAttribute( "hidden", "false" ); // use this to keep the seekbar in the same place
      }
    }
  }

  // event handler for data remotes
  const on_artist_album_changed = {
    observe: function ( aSubject, aTopic, aData ) { onSBArtistAlbumChanged(); }
  };

  function onSBMainwinComplexLoad()
  {
    try
    {
      // Title/<slash>/Album Box Complex -- two data items for one callback.

      // Create and bind the data remotes
      MainwinArtistRemote = SB_NewDataRemote( "metadata.title", null ); // changed to title cuz we like to be odd.
      MainwinAlbumRemote = SB_NewDataRemote( "metadata.album", null );
      MainwinArtistRemote.bindObserver( on_artist_album_changed, true );
      MainwinAlbumRemote.bindObserver( on_artist_album_changed, true );

      // add both to the array to be unbound on shutdown
      MainwinAdd( MainwinArtistRemote );
      MainwinAdd( MainwinAlbumRemote );
      
    }
    catch ( err )
    {
      alert( err );
    }
  }

  var theLastSearchEventTarget = null;
  var theSearchEventInterval = null;
  function onSBMainwinSearchEvent( evt )
  {
    if ( theSearchEventInterval )
    {
      clearInterval( theSearchEventInterval );
      theSearchEventInterval = null;
    }
    var widget = null;
    if ( theLastSearchEventTarget )
    {
      widget = theLastSearchEventTarget;
    }
    else if ( evt )
    {
      widget = evt.target;
    }
    if ( widget )
    {
      if ( widget.is_songbird )
      {
        if ( !thePlaylistTree && !theLastSearchEventTarget )
        {
          var theServiceTree = document.getElementById( 'frame_servicetree' );
          if (theServiceTree) theServiceTree.launchServiceURL( "chrome://songbird/content/xul/playlist_test.xul?library" );
        }
        
        if ( thePlaylistRef.stringValue.length )
        {
          // Feed the new filter into the list.
          var source = new sbIPlaylistsource();
          // Wait until it is done executing
          if ( ! source.isQueryExecuting( thePlaylistRef.stringValue ) )
          {
            // ...before attempting to override.
            source.setSearchString( thePlaylistRef.stringValue, widget.list.label );
            theLastSearchEventTarget = null;
            if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
            return;
          }
        }
        
        // If we get here, we need to wait for the tree to appear
        // and finish its query.
        theLastSearchEventTarget = widget;
        theSearchEventInterval = setInterval( onSBMainwinSearchEvent, 1500 );
      }
      else
      {
        onSearchTerm( evt.target.cur_service, evt.target.list.value );
      }
    }
  }
  
  // END
}
catch ( err )
{
  alert( err );
}

