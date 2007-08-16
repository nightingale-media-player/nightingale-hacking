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
// our Servicetree XBL object when it is cleaned up for 0.3



function onServiceTreeCommand( theEvent )
{
  if ( theEvent && theEvent.target )
  {
    // These attribs get set when the menu is popped up on a playlist.
    var label = theEvent.target.parentNode.getAttribute( "label" );
    var guid = theEvent.target.parentNode.getAttribute( "sb_guid" );
    var table = theEvent.target.parentNode.getAttribute( "sb_table" );
    var type = theEvent.target.parentNode.getAttribute( "sb_type" );
    var base_type = theEvent.target.parentNode.getAttribute( "sb_base_type" );
    switch ( theEvent.target.id )
    {
      case "service_popup_new":
        SBNewPlaylist();
      break;
      case "service_popup_new_smart":
        SBNewSmartPlaylist();
      break;
      case "service_popup_new_remote":
        SBSubscribe(null, null);
      break;
      
      case "playlist_context_smartedit":
        if ( base_type == "smart" )
        {
          if ( guid && guid.length > 0 && table && table.length > 0 )
          {
            SBNewSmartPlaylist( guid, table );
          }
        }
        if ( base_type == "dynamic" )
        {
          if ( guid && guid.length > 0 && table && table.length > 0 )
          {
            SBSubscribe( type, guid, table, label );
          }
        }
      break;
      
      case "playlist_context_rename":
        if ( guid && guid.length > 0 && table && table.length > 0 )
        {
          onServiceEdit();
        }
      break;
      
      case "playlist_context_remove":
        if ( guid && guid.length > 0 && table && table.length > 0 )
        {
  alert("XXXX - migrate 'onServiceTreeCommand()' to new API");  
  /*
          // Assume we'll need this...
          const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
          var aPlaylistManager = new PlaylistManager();
          aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);
          var aDBQuery = new sbIDatabaseQuery();
          
          aDBQuery.setAsyncQuery(false);
          aDBQuery.setDatabaseGUID(guid);

          switch ( base_type )
          {
            case "simple":
              aPlaylistManager.deleteSimplePlaylist(table, aDBQuery);
              break;
            case "dynamic":
              aPlaylistManager.deleteDynamicPlaylist(table, aDBQuery);
              break;
            case "smart":
              aPlaylistManager.deleteSmartPlaylist(table, aDBQuery);
              break;
            default:
              aPlaylistManager.deletePlaylist(table, aDBQuery);
              break;
          }
  */      
        }
      break;
    }
  }
}

var theServiceTreeScanItems = new Array();
var theServiceTreeScanCount = 0;
function SBScanServiceTreeNewEntryEditable()
{
  try
  {
    var theServiceTree = document.getElementById( "servicepane" );
    theServiceTreeScanItems.length = 0;
    theServiceTreeScanCount = 0;
    
    // Go get all the current service tree urls.
    var theServiceTree_tree = theServiceTree.tree;
    for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
    {
      theServiceTreeScanItems.push( theServiceTree_tree.contentView.getItemAtIndex( i ).getAttribute( "url" ) );
    }
  } 
  catch ( e ) 
  {
    dump("ERROR - Service tree stuff isn't hooked up!\n" + e );
  }
}

function SBScanServiceTreeNewEntryStart()
{
  setTimeout( SBScanServiceTreeNewEntryCallback, 500 );
}

function SBScanServiceTreeNewEntryCallback()
{
  try
  {
    var theServiceTree = document.getElementById( "sevicepane" );
    
    if ( ++theServiceTreeScanCount > 10 )
    {
      return; // don't loop more than 1 second.
    }
    
    // Go through all the current service tree items.
    var done = false;
    var theServiceTree_tree = theServiceTree.tree;
    for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
    {
      var found = false;
      var url = theServiceTree_tree.contentView.getItemAtIndex( i ).getAttribute( "url" );
      // Match them against the scan items
      for ( var j = 0; j < theServiceTreeScanItems.length; j++ )
      {
        if ( url == theServiceTreeScanItems[ j ] )
        {
          found = true;
          break;
        }
      }
      // Right now, only songbird playlists are editable.
      if ( ( ! found ) && ( url.indexOf( ",songbird" ) != -1 ) )
      {
  /*    
        // This must be the new one?
        theServiceTree_tree.view.selection.currentIndex = i;
        
        // HACK: flag to prevent the empty playlist from launching on select
        theServiceTree_tree.newPlaylistCreated = true;
        theServiceTree_tree.view.selection.select( i );
        theServiceTree_tree.newPlaylistCreated = false;
  */      
        onServiceEdit(i);
        done = true;
        break;
      }
    }
    if ( ! done )
    {
      setTimeout( SBScanServiceTreeNewEntryCallback, 100 );
    }
  } 
  catch ( e ) 
  {
    dump("ERROR - Service tree stuff isn't hooked up!\n" + e );
  }
}

function onServiceEdit( index )
{
  try
  {
    // in case onpopuphiding was not called (ie, ctrl-n + ctrl-n)
    if (isServiceEditShowing) {
      // validate changes in the previous popup, if any
      onServiceEditChange();
      // reset, this is important
      isServiceEditShowing = false; 
    }
    var theServiceTree = document.getElementById( "servicepane" );
    if ( theServiceTree)
    {
      var theServiceTree_tree = theServiceTree.tree;
      if ( index == null ) // Optionally specify which index to edit.
        index = theServiceTree_tree.currentIndex;
      if ( theServiceTree_tree && ( index > -1 ) )
      {
        var column = theServiceTree_tree.columns["frame_service_tree_label"];
        var cell_text = theServiceTree_tree.view.getCellText( index, column );
        var cell_url =  theServiceTree_tree.view.getCellText( index, theServiceTree_tree.columns["url"] );

        // This is nuts!
        var text_x = {}, text_y = {}, text_w = {}, text_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "text", text_x, text_y , text_w , text_h );
        var cell_x = {}, cell_y = {}, cell_w = {}, cell_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "cell", cell_x, cell_y , cell_w , cell_h );
        var image_x = {}, image_y = {}, image_w = {}, image_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "image", image_x, image_y , image_w , image_h );
        var twisty_x = {}, twisty_y = {}, twisty_w = {}, twisty_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "twisty", twisty_x, twisty_y , twisty_w , twisty_h );
          
        var out_x = {}, out_y = {}, out_w = {}, out_h = {};
        out_x = text_x;
        out_y = cell_y;
        out_w.value = cell_w.value - twisty_w.value - image_w.value;
        out_h = cell_h;
        
        // Then pop the edit box to the bounds of the cell.
        var theEditPopup = document.getElementById( "service_edit_popup" );
        var theEditBox = document.getElementById( "service_edit" );
        var extra_x = 3; // Why do I have to give it extra?  What am I calculating wrong?
        var extra_y = 7; // Why do I have to give it extra?  What am I calculating wrong?
        if (getPlatformString() == "Darwin")
          extra_y -= 1;  // And an extra pixel on the mac.  Great.
        var less_w  = 5;
        var less_h  = -2;
        var pos_x = extra_x + theServiceTree_tree.boxObject.screenX + out_x.value;
        var pos_y = extra_y + theServiceTree_tree.boxObject.screenY + out_y.value;
        theEditBox.setAttribute( "hidden", "false" );
        theEditPopup.showPopup( theServiceTree_tree, pos_x, pos_y, "popup" );
        theEditPopup.sizeTo( out_w.value - less_w, out_h.value - less_h ); // increase the width to the size of the cell.
        theEditBox.value = cell_text;
        theEditBox.index = index;
        theEditBox.url = cell_url;
        theEditBox.focus();
        theEditBox.select();
        isServiceEditShowing = true;
      }
    }
  }
  catch ( err )
  {
    alert( "onServiceEdit - " + err );
  }
}

function getItemByUrl( tree, url )
{
  var retval = null;

  for ( var i = 0; i < tree.view.rowCount; i++ )
  {
    var cell_url =  tree.view.getCellText( i, tree.columns["url"] );
    
    if ( cell_url == url )
    {
      retval = tree.contentView.getItemAtIndex( i );
      break;
    }
  }
  
  return retval;
}

function onServiceEditChange( )
{
  try
  {
    var theServiceTree = document.getElementById( "servicepane" );
    if ( theServiceTree)
    {
      var theServiceTree_tree = theServiceTree.tree;
      if ( theServiceTree_tree )
      {
        var theEditBox = document.getElementById( "service_edit" );

        var element = getItemByUrl( theServiceTree_tree, theEditBox.url );
        
        if ( element == null ) 
          return; // ??
        
        var properties = element.getAttribute( "properties" ).split(" ");
        if ( properties.length >= 5 && properties[0] == "playlist" )
        {
          var table = properties[ 1 ];
          var guid = properties[ 2 ];
          var type = properties[ 3 ];
          var base_type = properties[ 4 ];
          
          const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
          var aPlaylistManager = (new PlaylistManager()).QueryInterface(Components.interfaces.sbIPlaylistManager);
          var aDBQuery = new sbIDatabaseQuery();
          
          aDBQuery.setAsyncQuery(false);
          aDBQuery.setDatabaseGUID(guid);
          
          var playlist = null;
          
          // How do I edit a table's readable name?  I have to know what type it is?  Ugh.
          switch ( base_type )
          {
            case "simple":  playlist = aPlaylistManager.getSimplePlaylist(table, aDBQuery);  break;
            case "dynamic": playlist = aPlaylistManager.getDynamicPlaylist(table, aDBQuery); break;
            case "smart":   playlist = aPlaylistManager.getSmartPlaylist(table, aDBQuery);   break;
            default:        playlist = aPlaylistManager.getPlaylist(table, aDBQuery);        break;
          }
          
          if(playlist)
          {
            var theEditBox = document.getElementById( "service_edit" );
            var strReadableName = theEditBox.value;
            playlist.setReadableName(strReadableName);
          }
        }      
        HideServiceEdit( null );
      }
    }
  }
  catch ( err )
  {
    alert( "onServiceEditChange\n\n" + err );
  }
}

var hasServiceEditBeenModified = false;
function onServiceEditKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 27: // Esc
      HideServiceEdit( null );
      break;
    case 13: // Return
      onServiceEditChange();
      break;
    default:
      hasServiceEditBeenModified = true;
  }
}

function onServiceEditHide( evt ) {
  isServiceEditShowing = false;
}

var isServiceEditShowing = false;
function HideServiceEdit( evt )
{
  try
  {
    if (hasServiceEditBeenModified && evt)
    {
      hasServiceEditBeenModified = false;
      onServiceEditChange();
    }

    if ( isServiceEditShowing )
    {
      var theEditBox = document.getElementById( "service_edit" );
      theEditBox.setAttribute( "hidden", "true" );
      var theEditPopup = document.getElementById( "service_edit_popup" );
      theEditPopup.hidePopup();
      isServiceEditShowing = false;
      
    }
  }
  catch ( err )
  {
    alert( "HideServiceEdit\n\n" + err );
  }
}

function SBGetUrlFromService( service )
{
  retval = service;
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";
        
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_url = theServiceTree_tree.view.getCellText( i, urlcolumn );
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          if ( tree_label == service )
          {
            retval = tree_url;
            break;
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBGetUrlFromService - " + err )
  }
  return retval;
}

function SBTabcompleteService( service )
{
  retval = service;
  var service_lc = service.toLowerCase();
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";

        var found_one = false;      
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          var label_lc = tree_label.toLowerCase();
          
          // If we are the beginning of the label string
          if ( label_lc.indexOf( service_lc ) == 0 )
          {
            if ( found_one )
            {
              retval = service; // only find ONE!
              break;
            }
            else
            {
              found_one = true; // only find ONE!
              retval = tree_label;
            }
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBTabcompleteService - " + err )
  }
  return retval;
}


