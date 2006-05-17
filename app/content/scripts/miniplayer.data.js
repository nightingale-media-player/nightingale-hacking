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
      //onSBMiniplayerComplexLoad();
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
        MiniplayerDataRemotes[ i ].unbind();
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
      var artist = MiniplayerArtistRemote.getValue();
      var album = MiniplayerAlbumRemote.getValue();
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
      MiniplayerArtistRemote.bindCallbackFunction( onSBArtistAlbumChanged );
      MiniplayerAdd( MiniplayerArtistRemote );
      MiniplayerAlbumRemote = new sbIDataRemote( "metadata.album" );
      MiniplayerAlbumRemote.bindCallbackFunction( onSBArtistAlbumChanged );
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