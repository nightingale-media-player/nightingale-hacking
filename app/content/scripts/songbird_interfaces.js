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
// Formal constructors to get interfaces because I'm lazy.
//

const sbIDatabaseQuery = new Components.Constructor("@songbird.org/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");

const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");

const sbIMediaLibrary = new Components.Constructor("@songbird.org/Songbird/MediaLibrary;1", "sbIMediaLibrary");

const sbIPlaylistManager = new Components.Constructor("@songbird.org/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbird.org/Songbird/Playlist;1", "sbIPlaylist");
const sbISimplePlaylist = new Components.Constructor("@songbird.org/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbIDynamicPlaylist = new Components.Constructor("@songbird.org/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbird.org/Songbird/SmartPlaylist;1", "sbISmartPlaylist");

const sbIPlaylistReaderManager = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistReaderListener = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");

const sbIMediaScan = new Components.Constructor("@songbird.org/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbird.org/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");

//
// SBBindInterface - take an object and make it a wrapper of the given type. (doesn't crash)
//
function SBBindInterface( obj, contract_id, cid, as_service )
{
  try
  {
    if ( !obj )
    {
      var new_obj = {};
      obj = new_obj;
    }
    // First, get the factory by contract ID
    obj.m_Instance = Components.classes[ contract_id ];
    if ( obj.m_Instance )
    {
      // Massage it into an actual object of our type
      if ( as_service )
        obj.m_Instance = obj.m_Instance.getService();
      else
        obj.m_Instance = obj.m_Instance.createInstance();
      obj.m_Instance = obj.m_Instance.QueryInterface( cid );

      // Then attach the methods
      if ( obj.m_Instance )
      {
        for ( var i in obj.m_Instance )
        {
          obj[ i ] = obj.m_Instance[ i ];
        }
      }
      else
      {
        alert( "Load Failed - " + contract_id );
      }
    }
    else
    {
      alert( "Load Failed - " + contract_id );
    }
  }
  catch ( err )
  {
    alert( err );
  }
  return obj;
}
