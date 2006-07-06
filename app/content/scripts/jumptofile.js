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
// Jump To File
//

try
{
  // called by windows that want to support the J hotkey
  function initJumpToFileHotkey()
  {
    document.addEventListener("keypress", onKeyPress, false);
  }
   
  function resetJumpToFileHotkey()
  {
    document.removeEventListener("keypress", onKeyPress, false);
  }
  
  function closeJumpTo() {
    if (document.__JUMPTO__) {
      document.__JUMPTO__.defaultView.close();
    }
  }
  
  function onJumpToFileKey(evt) {
    if (!document.__JUMPTO__)
      window.openDialog( "chrome://songbird/content/xul/jumptofile.xul", "jump_to_file", "chrome,modal=no, popup=yes", document );
  }

  function onKeyPress(evt) {
    if (evt.charCode == 106 && evt.ctrlKey && !evt.altKey) {
      evt.preventBubble();
      onJumpToFileKey();
    }
  }

  var SBEmptyPlaylistCommands = 
  {
    m_Playlist: null,

    getNumCommands: function()
    {
      return 0;
    },

    getCommandId: function( index )
    {
      return -1;
    },

    getCommandText: function( index )
    {
      return "";
    },

    getCommandFlex: function( index )
    {
      return 0;
    },

    getCommandToolTipText: function( index )
    {
      return "";
    },

    getCommandEnabled: function( index )
    {
      return 0;
    },

    onCommand: function( event )
    {
    },
    
    // The object registered with the sbIPlaylistSource interface acts 
    // as a template for instances bound to specific playlist elements
    duplicate: function()
    {
      var obj = {};
      for ( var i in this )
      {
        obj[ i ] = this[ i ];
      }
      return obj;
    },
    
    setPlaylist: function( playlist )
    {
      this.m_Playlist = playlist;
    },
    
    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
  }
 
  var jumpto_ref;
  var source_ref;
  var editIdleInterval;
  
  function onLoadJumpToFile() {
    onWindowLoadSize();
    window.arguments[0].__JUMPTO__ = document;
    var guid;
    var table;
    var ref = SBDataGetStringValue("playing.ref");
    if (ref != "") {
      source_ref = ref;
      var source = new sbIPlaylistsource();
      guid = source.getRefGUID( ref );
      table = source.getRefTable( ref );
    } else {
      var pl = window.arguments[0].__CURRENTPLAYLIST__;
      if (!pl) pl = window.arguments[0].__CURRENTPLAYLIST__;
      if (pl) {
        source_ref = pl.ref;
        guid = pl.guid;
        table = pl.table
      } else {
        source_ref = "NC:songbird_library";
        guid = "songbird";
        table = "library";
      }
    }
    var playlist = document.getElementById("jumpto.playlist");
    playlist.tree.setAttribute("seltype", "single");
    playlist.forcedcommands = SBEmptyPlaylistCommands;
    playlist.bind(guid, table, null, null, null, null, "jumpto");
    jumpto_ref = playlist.ref;
    var textbox = document.getElementById("jumpto.textbox");
    window.focus();
    textbox.focus();
    playlist.addEventListener("playlist-play", onJumpToPlay, false);
    playlist.addEventListener("playlist-esc", onExit, false);
    _applyFilter();
  }
  
  // for the playlist 
  function onListKeypress(evt) {
    try
    {
      switch ( evt.keyCode )
      {
        case 27: // Esc
          // close the window          
          onExit();
          break;
      }
    }
    catch( err )
    {
      alert( err )
    }
  }
  
  // for the edit box
  function onFilterKeypress(evt) {
    try
    {
      switch ( evt.keyCode )
      {
        case 27: // Esc
          // close the window          
          onExit();
          break;
        case 13: // Return
          onFilterEditEnter();
          // reselect
          break;      
        default:
          if ( editIdleInterval )
          {
            clearInterval( editIdleInterval );
          }
          editIdleInterval = setInterval( onFilterEditIdle, 1000 );
          break;      
      }
    }
    catch( err )
    {
      alert( err )
    }
  }
  
  function onFilterEditIdle(evt) {
    if ( editIdleInterval )
    {
      clearInterval( editIdleInterval );
    }
    _applyFilter();
  }

  function onFilterEditEnter(evt) {
    _applyFilter();
    var playlist = document.getElementById("jumpto.playlist");
    // change focus to playlist
    playlist.tree.focus();
    if (playlist.tree.view.rowCount > 0) {
      playlist.tree.view.selection.clearSelection();
      playlist.tree.view.selection.rangedSelect(0, 0, true);
    } 
  }
  
  function _applyFilter() {
    var filter = document.getElementById("jumpto.textbox").value;
    // Feed the new filter into the list.
    var source = new sbIPlaylistsource();
    // Wait until it is done executing
    if ( ! source.isQueryExecuting( jumpto_ref ) )
    {
      // ...before attempting to override.
      source.setSearchString( jumpto_ref, filter );
    }
  }
  
  function onJumpToPlay(event) {
    var playlist = document.getElementById("jumpto.playlist");
    var first=0;
    var rangeCount = playlist.tree.view.selection.getRangeCount();
    if (rangeCount > 0)
    {
      var start = {};
      var end = {};
      playlist.tree.view.selection.getRangeAt( 0, start, end );
      first = start.value;
    }
    var idcolumn = playlist.tree.columns.getNamedColumn("id"); 
    var rowid=0;
    if (idcolumn != null) 
    {
      rowid = playlist.tree.view.getCellText( first, idcolumn );
    }
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.playRefByID(source_ref, rowid);
    onExit();
  }

  function onUnloadJumpToFile() {
    var playlist = document.getElementById("jumpto.playlist");
    playlist.removeEventListener("playlist-esc", onExit, false);
    playlist.removeEventListener("playlist-play", onJumpToPlay, false);
    window.arguments[0].__JUMPTO__ = null;
  }

}
catch ( err )
{
  alert( err );
}
