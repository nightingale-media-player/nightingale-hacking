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
// sbIPlaylist Object
//


const SONGBIRD_PLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/Playlist;1";
const SONGBIRD_PLAYLIST_CLASSNAME = "Songbird Playlist Interface"
const SONGBIRD_PLAYLIST_CID = Components.ID("{c3c120f4-5ab6-4402-8cab-9b8f1d788769}");
const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

function CPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
  this.m_strPlaylistTableName = "playlist_list";
}

CPlaylist.prototype = new CPlaylistBase();
CPlaylist.prototype.constructor = CPlaylist;

/* the CPlaylist class def */
CPlaylist.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsISupports) &&
      !iid.equals(SONGBIRD_PLAYLIST_IID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};
