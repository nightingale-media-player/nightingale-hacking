/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
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
// Formal constructors to get interfaces - all creations in js should use these unless
//   getting the object as a service, then you should use the proper getService call.
// alpha-sorted!!
//

const sbIDatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");
const sbIDynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
const sbIMediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
const sbIMediaScan = new Components.Constructor("@songbirdnest.com/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbirdnest.com/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");
const sbIPlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");
const sbIPlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
const sbIPlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");
const sbISimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");

// XXXredfive - this goes in the sbWindowUtils.js file when I get around to making it.
var gPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
var gConsole = Components.classes["@mozilla.org/consoleservice;1"]
               .getService(Components.interfaces.nsIConsoleService);
// log to JS console AND to the command line (in case of crashes)
function SB_LOG (scopeStr, msg) {
  msg = msg ? msg : ""; 
  //This works, but adds everything as an Error
  //Components.utils.reportError( scopeStr + " : " + msg);
  gConsole.logStringMessage( scopeStr + " : " + msg );
  dump( scopeStr + " : " + msg + "\n");
}



//
// SBBindInterface - take an object and make it a wrapper of the given type.
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
