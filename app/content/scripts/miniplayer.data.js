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
//  This file binds UI objects from the Miniplayer.xul (currently rmp_demo.xul)
//  to data remotes that will automatically update them to reflect the proper value.
//

try
{
  function onSBMiniplayerDataLoad()
  {
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
      MiniplayerAdd( SBDataBindElementAttribute( "playlist.shuffle", "rmp_demo_btn_shuf", "hidden", true ) );
      MiniplayerAdd( SBDataBindElementAttribute( "playlist.shuffle", "rmp_demo_btn_shuf_on", "hidden", true, true ) );

      MiniplayerAdd( SBDataBindElementAttribute( "playlist.repeat", "rmp_demo_btn_rep", "hidden", true, false, "parseInt( value ) != 0" ) );
      MiniplayerAdd( SBDataBindElementAttribute( "playlist.repeat", "rmp_demo_btn_rep1", "hidden", true, false, "parseInt( value ) != 1" ) );
      MiniplayerAdd( SBDataBindElementAttribute( "playlist.repeat", "rmp_demo_btn_repa", "hidden", true, false, "parseInt( value ) != 2" ) );
      
      // Metadata Items    
      
      MiniplayerAdd( SBDataBindElementProperty ( "metadata.title",  "rmp_demo_text_title", "value" ) );
      
//      MiniplayerAdd( SBDataBindElementProperty ( "metadata.artist", "rmp_demo_text_artist", "value" ) );
//      MiniplayerAdd( SBDataBindElementProperty ( "metadata.album", "rmp_demo_text_album", "value" ) );
      
      MiniplayerAdd( SBDataBindElementAttribute( "metadata.position.str", "rmp_demo_text_time_elapsed", "value" ) );
      MiniplayerAdd( SBDataBindElementAttribute( "metadata.length.str", "rmp_demo_text_time_total", "value" ) );
      
      MiniplayerAdd( SBDataBindElementProperty ( "metadata.position", "songbird_seekbar", "value", false, false, "parseInt( value )" ) );
      MiniplayerAdd( SBDataBindElementProperty ( "metadata.length", "songbird_seekbar", "maxpos" ) );

      MiniplayerAdd( SBDataBindElementProperty( "faceplate.volume", "songbird_volume", "value", false, false) );
      MiniplayerAdd( SBDataBindElementAttribute( "faceplate.mute", "mute_on", "hidden", true, true ) );
      MiniplayerAdd( SBDataBindElementAttribute( "faceplate.mute", "mute_off", "hidden", true ) );
      

      // Faceplate Items    
      MiniplayerAdd( SBDataBindElementAttribute( "faceplate.play", "rmp_demo_btn_pause", "hidden", true ) );
      MiniplayerAdd( SBDataBindElementAttribute( "faceplate.play", "rmp_demo_btn_play", "hidden", true, true ) );
      
      // Complex Data Items
//      onSBMiniplayerComplexLoad();
    }
    catch ( err )
    {
      alert( err );
    }
  }
  
  // The bind function returns a remote, 
  // we keep it here so there's always a ref
  // and we stay bound till we unload.
  var MiniplayerDataRemotes = new Array();
  function MiniplayerAdd( remote )
  {
    if ( remote )
    {
      MiniplayerDataRemotes.push( remote );
    }
  }

  function onSBMiniplayerDataUnload()
  {
    try
    {
//      alert( "unload" );
      // Unbind the observers!
      for ( var i = 0; i < MiniplayerDataRemotes.length; i++ )
      {
        MiniplayerDataRemotes[ i ].Unbind();
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
  
  var MiniplayerArtistRemote = null;
  var MiniplayerAlbumRemote = null;
  function onSBArtistAlbumChanged( value )
  {
    // (we get called before they're fully bound)
    if ( MiniplayerArtistRemote && MiniplayerAlbumRemote )
    {
      var artist = MiniplayerArtistRemote.GetValue();
      var album = MiniplayerAlbumRemote.GetValue();
      var theAASlash = document.getElementById( "rmp_demo_text_slash" );
      var theAABox = document.getElementById( "rmp_demo_text_artistalbum" );
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
        theAABox.setAttribute( "hidden", "true" );
      }
    }
  }

  function onSBMiniplayerComplexLoad()
  {
    try
    {
      // Artist/Album Box Complex -- two data items for one callback.
      MiniplayerArtistRemote = new sbIDataRemote( "metadata.artist" );
      MiniplayerArtistRemote.BindCallbackFunction( onSBArtistAlbumChanged );
      MiniplayerAdd( MiniplayerArtistRemote );
      MiniplayerAlbumRemote = new sbIDataRemote( "metadata.album" );
      MiniplayerAlbumRemote.BindCallbackFunction( onSBArtistAlbumChanged );
      MiniplayerAdd( MiniplayerAlbumRemote );
      
    }
    catch ( err )
    {
      alert( err );
    }
  }
  // END
}
catch ( err )
{
  alert( err );
}