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


// TODO: This entire file should be going away!


// This is a temporary file to house methods that need to roll into
// our Playlist XBL object that we'll be updating for 0.3

function onPlaylistKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 13: // Return
      SBPlayPlaylistIndex(gBrowser.playlistTree.currentIndex );
      break;
  }
}

// Yo, play something, bitch.
function onPlaylistPlay( evt )
{
  var target = evt.target;
  if ( target.wrappedJSObject )
  {
    target = target.wrappedJSObject;
  }
  SBPlayPlaylistIndex( target.tree.currentIndex, target );
}

function onPlaylistBurnToCD( evt )
{
  var target = evt.target;
  if ( target.wrappedJSObject )
  {
    target = target.wrappedJSObject;
  }
  
  playlist = target;
  playlistTree = target.tree;
  
	var filterCol = "uuid";
	var filterVals = new Array();

	var columnObj = playlistTree.columns.getNamedColumn(filterCol);
	var rangeCount = playlistTree.view.selection.getRangeCount();
	for (var i=0; i < rangeCount; i++) 
	{
		var start = {};
		var end = {};
		playlistTree.view.selection.getRangeAt( i, start, end );
		for( var c = start.value; c <= end.value; c++ )
		{
			if (c >= playlistTree.view.rowCount) 
			{
			continue; 
			}
	        
			var value = playlistTree.view.getCellText(c, columnObj);
			filterVals.push(value);
		}
	}
	
    onAddToCDBurn( playlist.guid, playlist.table, filterCol, filterVals.length, filterVals );
}

function onPlaylistNoPlaylist() 
{ 
  gBrowser.loadURI( 'chrome://songbird/content/xul/playlist_test.xul?library' ); 
}

function onPlaylistFilterChange() {
  if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
}

function onPlaylistDblClick( evt )
{
  if ( typeof(gBrowser.playlistTree) == 'undefined' )
  {
    alert( "DOM?" );
    return;
  }
  var obj = {}, row = {}, col = {}; 
  gBrowser.playlistTree.treeBoxObject.getCellAt( evt.clientX, evt.clientY, row, col, obj );
  // If the "obj" has a value, it is a cell?
  if ( obj.value )
  {
    if ( gBrowser.playlistTree.currentIndex != -1 )
    {
      SBPlayPlaylistIndex( gBrowser.playlistTree.currentIndex );
    }
  }
}

// Sigh.  The track editor assumes this document object will be populated with
// special info.  So we have to pop the track editor from this DOM, not XBL.
function onPlaylistEditor( evt )
{
  var playlist = evt.target;
  if ( playlist.wrappedJSObject )
    playlist = playlist.wrappedJSObject;
  
  if (playlist && playlist.guid == "songbird") 
    SBTrackEditorOpen();
}





