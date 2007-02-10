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
// our Playlist XBL object that we'll be updating for 0.3

function onPlaylistKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 13: // Return
      SBPlayPlaylistIndex( thePlaylistTree.currentIndex );
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
  document.getElementById( 'frame_servicetree' ).launchServiceURL( 'chrome://songbird/content/xul/playlist_test.xul?library' ); 
}

function onPlaylistFilterChange() {
  if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
}

function onPlaylistDblClick( evt )
{
  if ( typeof( thePlaylistTree ) == 'undefined' )
  {
    alert( "DOM?" );
    return;
  }
  var obj = {}, row = {}, col = {}; 
  thePlaylistTree.treeBoxObject.getCellAt( evt.clientX, evt.clientY, row, col, obj );
  // If the "obj" has a value, it is a cell?
  if ( obj.value )
  {
    if ( thePlaylistTree.currentIndex != -1 )
    {
      SBPlayPlaylistIndex( thePlaylistTree.currentIndex );
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

var theCurrentlyEditingPlaylist = null;
function onPlaylistEdit( evt )
{
  var playlist = evt.target;
  if ( playlist.wrappedJSObject )
    playlist = playlist.wrappedJSObject;
  theCurrentlyEditingPlaylist = playlist;
  setTimeout(doPlaylistEdit, 0);
}

function doPlaylistEdit()
{
  try
  {
    // Make sure it's something with a uuid column.
    var filter = "uuid";
    var filter_column = theCurrentlyEditingPlaylist.tree.columns ? theCurrentlyEditingPlaylist.tree.columns[filter] : filter;
    var filter_value = theCurrentlyEditingPlaylist.tree.view.getCellText( theCurrentlyEditingPlaylist.tree.currentIndex, filter_column );
    if ( !filter_value )
    {
      return;
    }
    
    // We want to resize the edit box to the size of the cell.
    var out_x = {}, out_y = {}, out_w = {}, out_h = {}; 
    theCurrentlyEditingPlaylist.tree.treeBoxObject.getCoordsForCellItem( theCurrentlyEditingPlaylist.edit_row, theCurrentlyEditingPlaylist.edit_col, "cell",
                                                        out_x , out_y , out_w , out_h );
                           
    var cell_text = theCurrentlyEditingPlaylist.tree.view.getCellText( theCurrentlyEditingPlaylist.edit_row, theCurrentlyEditingPlaylist.edit_col );
    
    // Then pop the edit box to the bounds of the cell.
    var theMainPane = document.getElementById( "frame_main_pane" );
    var theEditPopup = document.getElementById( "playlist_edit_popup" );
    var theEditBox = document.getElementById( "playlist_edit" );
    var extra_x = 5; // Why do I have to give it extra?  What am I calculating wrong?
    var extra_y = 21; // Why do I have to give it extra?  What am I calculating wrong?
    var less_w  = 6;
    var less_h  = 0;
    var pos_x = extra_x + theCurrentlyEditingPlaylist.tree.boxObject.screenX + out_x.value;
    var pos_y = extra_y + theCurrentlyEditingPlaylist.tree.boxObject.screenY + out_y.value;
    theEditBox.setAttribute( "hidden", "false" );
    theEditPopup.showPopup( theMainPane, pos_x, pos_y, "context" );
    theEditPopup.sizeTo( out_w.value - less_w, out_h.value - less_h ); // increase the width to the size of the cell.
    theEditBox.value = cell_text;
    theEditBox.focus();
    theEditBox.select();
    isPlaylistEditShowing = true;
    theCurrentlyEditingPlaylist.theCurrentlyEditedUUID = filter_value;
    theCurrentlyEditingPlaylist.theCurrentlyEditedOldValue = cell_text;
  }
  catch ( err )
  {
    alert( err );
  }
}

function onPlaylistEditChange( evt )
{
  try
  {
    if (isPlaylistEditShowing) {
      var theEditBox = document.getElementById( "playlist_edit" );
      
      var filter_value = theCurrentlyEditingPlaylist.theCurrentlyEditedUUID;
      
      var the_table_column = theCurrentlyEditingPlaylist.edit_col.id;
      var the_new_value = theEditBox.value
      if (theCurrentlyEditingPlaylist.theCurrentlyEditedOldValue != the_new_value) {
      
        var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
        var aMediaLibrary = Components.classes["@songbirdnest.com/Songbird/MediaLibrary;1"].createInstance(Components.interfaces.sbIMediaLibrary);
        
        if ( ! aDBQuery || ! aMediaLibrary)
          return;
        
        aDBQuery.setAsyncQuery(true);
        aDBQuery.setDatabaseGUID(theCurrentlyEditingPlaylist.guid);
        aMediaLibrary.setQueryObject(aDBQuery);
        
        aMediaLibrary.setValueByGUID(filter_value, the_table_column, the_new_value, false);
        
        //var table = "library" // hmm... // theCurrentlyEditingPlaylist.table;
        //var q = 'update ' + table + ' set ' + the_table_column + '="' + the_new_value + '" where ' + filter + '="' + filter_value + '"';
        //aDBQuery.addQuery( q );
        
        //var ret = aDBQuery.execute();
      }    
      HidePlaylistEdit();
    }
  }
  catch ( err )
  {
    alert( err );
  }
}

function onPlaylistEditKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 27: // Esc
      HidePlaylistEdit();
      break;
    case 13: // Return
      onPlaylistEditChange( evt );
      break;
  }
}

var isPlaylistEditShowing = false;
function HidePlaylistEdit()
{
  try
  {
    if ( isPlaylistEditShowing )
    {
      var theEditBox = document.getElementById( "playlist_edit" );
      theEditBox.setAttribute( "hidden", "true" );
      var theEditPopup = document.getElementById( "playlist_edit_popup" );
      theEditPopup.hidePopup();
      isPlaylistEditShowing = false;        
      theCurrentlyEditingPlaylist = null;
    }
  }
  catch ( err )
  {
    alert( err );
  }
}

// Menubar handling
function onPlaylistContextMenu( evt )
{
  try
  {
    // hacky for now
    var playlist = theLibraryPlaylist;
    if ( !playlist )
    {
      playlist = theWebPlaylist;
    }
    
    // All we do up here, now, is dispatch the search items
    onSearchTerm( playlist.context_item, playlist.context_term );
  }
  catch ( err )
  {
    alert( err );
  }
}

function onSearchTerm( target, in_term )
{
  var search_url = "";
  if ( target && in_term && in_term.length )
  {
//    var term = '"' + in_term + '"';
    var term = in_term;
    var v = target.getAttribute( "id" );
    switch ( v )
    {
      case "search.popup.songbird":
        onSearchEditIdle();
      break;
      case "search.popup.creativecommons":
        search_url = "http://search.creativecommons.com/?q=" + term;
      break;
      case "search.popup.mefeedia":
        search_url = "http://www.mefeedia.com/search.php?object=feed&q=" + term;
      break;
      case "search.popup.google":
        search_url = "http://www.google.com/musicsearch?q=" + term + "&sa=Search";
      break;
      case "search.popup.wiki":
        search_url = "http://en.wikipedia.org/wiki/Special:Search?search=" + term;
      break;
      case "search.popup.yahoo":
        search_url = "http://audio.search.yahoo.com/search/audio?ei=UTF-8&fr=sfp&p=" + term;
      break;
      case "search.popup.emusic":
        search_url = "http://www.emusic.com/search.html?mode=x&QT=" + term + "&fref=150554";
      break;
      case "search.popup.insound":
        search_url = "http://search.insound.com/search/searchmain.jsp?searchby=meta&query=" + term + "&fromindex=1&submit.x=0&submit.y=0";
      break;
      case "search.popup.odeo":
        search_url = "http://odeo.com/search/query/?q=" + term + "&Search.x=0&Search.y=0";
      break;
      case "search.popup.shoutcast":
        search_url = "http://www.shoutcast.com/directory/?s=" + term;
      break;
      case "search.popup.radiotime":
        search_url = "http://radiotime.com/Search.aspx?query=" + term;
      break;        
      case "search.popup.elbows":
        search_url = "http://elbo.ws/mp3s/" + term;
      break;
      case "search.popup.dogpile":
        search_url = "http://www.dogpile.com/info.dogpl/search/redir.htm?r_fcid=414&r_fcp=top&advanced=1&top=1&nde=1&qcat=audio&q_all=" + term + "&q_phrase=&q_any=&q_not=&duration=long&ffmt=mp3&tviewby=1&qk=40&adultfilter=none&bottomadvancedsubmit=Go+Fetch%21";
      break;
      case "lyrics.popup.google":
        search_url = "http://www.google.com/search?q=lyrics " + term + "&sa=Search&client=pub-4053348708517670&forid=1&ie=ISO-8859-1&oe=ISO-8859-1&hl=en&GALT:#333333;GL:1;DIV:#37352E;VLC:000000;AH:center;BGC:C6B396;LBGC:8E866F;ALC:000000;LC:000000;T:44423A;GFNT:663333;GIMP:663333;FORID:1;";
      break;
    }
  }
  if ( search_url.length )
  {
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchURL( search_url );
  }
}

