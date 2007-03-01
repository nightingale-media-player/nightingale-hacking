// JScript source code
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

// This is a temporary file to house methods that need to roll into
// our new Currenttrack XBL object that we'll be building for 0.3

var theCurrentTrackInterval = null;
function onCurrentTrack()
{
  if ( theCurrentTrackInterval )
  {
    clearInterval( theCurrentTrackInterval );
  }

  var guid;
  var table;
  var ref = SBDataGetStringValue("playing.ref");
  if (ref != "") {
    source_ref = ref;
    var source = new sbIPlaylistsource();
    guid = source.getRefGUID( ref );
    table = source.getRefTable( ref );
  } else {
    source_ref = "NC:songbird_library";
    guid = "songbird";
    table = "library";
  }
  
  if ( thePlaylistTree ) {
    var curplaylist = document.__CURRENTPLAYLIST__;
    if (curplaylist && curplaylist.ref == source_ref) {
      curplaylist.syncPlaylistIndex(true);
      return;
    }
  }
  
  if (guid == "songbird" && table == "library") { 
    gServicePane.loadURL( "chrome://songbird/content/xul/playlist_test.xul?library");
  }
  else 
  {
    gServicePane.loadURL( "chrome://songbird/content/xul/playlist_test.xul?" + table+ "," + guid);
  } 
  theCurrentTrackInterval = setInterval( onCurrentTrack, 500 );
}


