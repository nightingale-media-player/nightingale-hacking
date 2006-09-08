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
// Drag and Drop 
//

var sbIPlaylistDragObserver = { 
  onDragStart: function (evt,transferData,action) {
    var treechildren = evt.target;
    var node = treechildren;
    while (node != null && node.tagName != "playlist") { node = node.parentNode; }
    var playlist = node;
    var dndsourceindex = playlist.getDnDSourceIndex();
    transferData.data=new TransferData();
    transferData.data.addDataForFlavour("songbird/playlist_selection",dndsourceindex);
  }
};

var sbIServiceDropObserver = {
  
  m_serviceTree: null,
  m_curSelectedTarget: -1,
  m_curRealSelection: -1,
  
  getSupportedFlavours : function () {
    var flavours = new FlavourSet();
    flavours.appendFlavour("songbird/playlist_selection");
    return flavours;
  },
  
  onDragOver: function (evt,flavour,session){ },
  
  onDragExit: function (evt,session) { 
  },
  
  onDrop: function (evt,dropdata,session){
    /*var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage("drop");*/
    //var urlcolumn = this.m_serviceTree.columns ? this.m_serviceTree.columns["url"] : "url";
    //var tree_url = this.m_serviceTree.view.getCellText( this.m_curSelectedTarget, urlcolumn );

    var element = this.m_serviceTree.contentView.getItemAtIndex( this.m_curSelectedTarget );
    var properties = element.getAttribute( "properties" ).split(" ");
    
    var dest_playlist_name;
    var dest_playlist_service;

    if (properties.length > 1)
    {
      dest_playlist_name = properties[1];
      dest_playlist_service = properties[2];
    }
    else
    {
      dest_playlist_name = null;
      dest_playlist_service = "songbird";
    }

    switch (dropdata.flavour.contentType)
    {
      case "songbird/playlist_selection":
      {
        var source_playlist = sbDnDSourceTracker.getDnDSource(dropdata.data);
        if (source_playlist != null) 
        {
          source_playlist.addToPlaylistOrLibrary(dest_playlist_name);
        }
      }
      break;
    }
  },
  
  canDrop: function(evt, session) {
    if (this.m_curSelectedTarget == -1) return false; // not an item


    var flavourSet = this.getSupportedFlavours();
    var transferData = nsTransferable.get(flavourSet, nsDragAndDrop.getDragData, true);
    var dropdata = transferData.first.first;
    switch (dropdata.flavour.contentType)
    {
      case "songbird/playlist_selection":
      {
        var source_playlist = sbDnDSourceTracker.getDnDSource(dropdata.data);

        var element = this.m_serviceTree.contentView.getItemAtIndex( this.m_curSelectedTarget );
        var properties = element.getAttribute( "properties" ).split(" ");
        
        if (properties.length < 5) {
          // hack for library, fix
          var urlcolumn = this.m_serviceTree.columns ? this.m_serviceTree.columns["url"] : "url";
          var tree_url = this.m_serviceTree.view.getCellText( this.m_curSelectedTarget, urlcolumn );
          if (tree_url.indexOf("playlist_test.xul") == -1) return false;
        } else {
          if (properties[4] == "dynamic") return false; // dynamic playlists should only contain items from their dynamic source
        }
        
        var dest_playlist_name;
        var dest_playlist_service;

        if (properties.length > 1)
        {
          dest_playlist_name = properties[1];
          dest_playlist_service = properties[2];
        }
        else
        {
          dest_playlist_name = "library";
          dest_playlist_service = "songbird";
        }
        
        // if dragging on the source playlist, deny drop
        if (source_playlist.guid == dest_playlist_service && source_playlist.table == dest_playlist_name) return false;
        
        // if dragging from a playlist onto the library it is associated with, deny drop since the tracks are already there
        if (source_playlist.guid == dest_playlist_service && dest_playlist_name == "library") return false;
        
        // for now, we can only drop on playlists from the songbird service, this will change
        if (dest_playlist_service != "songbird") return false;
        
        return true;
      }
    }
    
    return false;
  },

  setServiceTree: function(tree) { 
    this.m_serviceTree = tree; 
    this.m_curRealSelection = tree.currentIndex;  
  },
  
  highlightTarget: function(x, y) {
    if (this.m_curSelectedTarget != -1) this.unHighlightPreviousTarget();
    var treebox = this.m_serviceTree.treeBoxObject;
    var row = {};
    var col = {};
    var childpe = {};
    treebox.getCellAt(x, y, row, col, childpe);
    if (row.value == -1) return;
    this.m_curSelectedTarget = row.value;
    this.m_serviceTree.setAttribute("seltype", "multiple");
    if (this.m_curSelectedTarget != this.m_curRealSelection) this.m_serviceTree.view.selection.toggleSelect(this.m_curSelectedTarget);
    this.m_serviceTree.currentIndex = this.m_curRealSelection;
    this.m_serviceTree.setAttribute("seltype", "single");
  },
  
  unHighlightPreviousTarget: function() {
    if (this.m_curSelectedTarget == this.m_curRealSelection || this.m_curSelectedTarget == -1) return;
    this.m_serviceTree.setAttribute("seltype", "multiple");
    this.m_serviceTree.view.selection.toggleSelect(this.m_curSelectedTarget);
    this.m_serviceTree.currentIndex = this.m_curRealSelection;
    this.m_serviceTree.setAttribute("seltype", "single");
    this.m_curSelectedTarget = -1;
  }
};


function onServiceTreeDragEnter(event)
{
  sbIServiceDropObserver.setServiceTree(event.currentTarget);
}

function onServiceTreeDragOver(event)
{
  sbIServiceDropObserver.highlightTarget(event.clientX, event.clientY);
  nsDragAndDrop.dragOver(event, sbIServiceDropObserver);
}

function onServiceTreeDragExit(event)
{
  sbIServiceDropObserver.unHighlightPreviousTarget(event.clientX, event.clientY);
  nsDragAndDrop.dragExit(event, sbIServiceDropObserver);
}

function onServiceTreeDragDrop(event)
{
  nsDragAndDrop.drop(event, sbIServiceDropObserver);
}
