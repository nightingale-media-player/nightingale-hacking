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

/**
 * \file dndDefaultHandler.js
 * \internal
 */

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;

var firstDrop;
var importingDrop = false;

// Module specific global for auto-init/deinit support
var dndDefaultHandler_module = {};
dndDefaultHandler_module.init_once = 0;
dndDefaultHandler_module.deinit_once = 0;

dndDefaultHandler_module.onDragOver = function(event) { nsDragAndDrop.dragOver( event, SBDropObserver ) }
dndDefaultHandler_module.onDragDrop = function(event) { nsDragAndDrop.drop( event, SBDropObserver ) }
dndDefaultHandler_module.onLoad = function(event)
{
  if (dndDefaultHandler_module.init_once++) { dump("WARNING: dndDefaultHandler_module double init!!\n"); return; }
  
  document.addEventListener( "dragover", dndDefaultHandler_module.onDragOver, true );
  document.addEventListener( "dragdrop", dndDefaultHandler_module.onDragDrop, true );
}

dndDefaultHandler_module.onUnload = function()
{
  if (dndDefaultHandler_module.deinit_once++) { dump("WARNING: dndDefaultHandler_module double deinit!!\n"); return; }
  document.removeEventListener( "dragover", dndDefaultHandler_module.onDragOver, true );
  document.removeEventListener( "dragdrop", dndDefaultHandler_module.onDragDrop, true );
  window.removeEventListener("load", dndDefaultHandler_module.onLoad, false);
  window.removeEventListener("unload", dndDefaultHandler_module.onUnload, false);
}
// Auto-init/deinit registration
window.addEventListener("load", dndDefaultHandler_module.onLoad, false);
window.addEventListener("unload", dndDefaultHandler_module.onUnload, false);

var SBDropObserver = 
{
  canHandleMultipleItems: true,
  libraryServicePane: null,
  
  getSupportedFlavours : function () 
  {
    if(!libraryServicePane) {
      var libraryServicePane = Components.classes['@songbirdnest.com/servicepane/library;1']
                              .getService(Components.interfaces.sbILibraryServicePaneService);
    }
    
    var flavours = new FlavourSet();
    flavours.appendFlavour("application/x-moz-file","nsIFile");
    flavours.appendFlavour("text/x-moz-url");
    flavours.appendFlavour("text/unicode");
    flavours.appendFlavour("text/plain");
    return flavours;
  },
  
  onDragOver: function ( evt, flavour, session )
  {
    //Figure out if we got dropped onto a playlist or library or just a generic element.  
    if(!libraryServicePane) {
      var libraryServicePane = Components.classes['@songbirdnest.com/servicepane/library;1']
                              .getService(Components.interfaces.sbILibraryServicePaneService);
    }

    if ( evt.target.tagName == "servicepane" ) {
      var node = null;
      try {
        node = evt.target.mTreePane.computeEventPosition(evt);
      }
      catch(e) {
        node = null;
      }
      
      if(node && 
         node.node instanceof Ci.sbIServicePaneNode) {
        var libraryResource = libraryServicePane.getLibraryResourceForNode(node.node);
        
        if(libraryResource &&
           SBIsWebLibrary(libraryResource)) {
          throw Cr.NS_ERROR_INVALID_ARG;
        }
      }
    }
    
    if ( evt.target.tagName == "playlist" ) {
      var playlist = evt.target.QueryInterface(Ci.sbIPlaylistWidget);
      
      if(playlist.wrappedJSObject) {
        playlist = playlist.wrappedJSObject;
      }

      if(playlist.mediaList &&
         SBIsWebLibrary(playlist.mediaList)) {
        throw Cr.NS_ERROR_INVALID_ARG;
      }
    }
    
    return false;
  },
  
  onDrop: function ( evt, dropdata, session )
  {
    dropFiles(evt, dropdata, session, null, null);
  }
};

var _insert_index = -1;
var _library_resource = null;
var _drop_filelist = null;

function dropFiles(evt, dropdata, session, targetdb, targetpl) {
  var dataList = dropdata.dataList;
  var dataListLength = dataList.length;
  var lcase = 0;

  //Figure out if we got dropped onto a playlist or library or just a generic element.  
  var libraryServicePane = Components.classes['@songbirdnest.com/servicepane/library;1']
                          .getService(Components.interfaces.sbILibraryServicePaneService);

  var libraryResource = null;

  if ( evt.target.tagName == "servicepane" ) {
    var node = null;
    try {
      node = evt.target.mTreePane.computeEventPosition(evt);
    }
    catch(e) {
      node = null;
    }
    
    if(node && 
       node.node instanceof Ci.sbIServicePaneNode) {
      libraryResource = libraryServicePane.getLibraryResourceForNode(node.node);
    }
  }
  
  if ( evt.target.tagName == "playlist" ) {
    var playlist = evt.target;
  
    if(playlist.wrappedJSObject) {
      playlist = playlist.wrappedJSObject;
    }

    libraryResource = playlist.mediaList;
    
    //Figure out which row we are over so we can insert at that position later.
    var tree = playlist.tree;

    if(tree.wrappedJSObject) {
      tree = tree.wrappedJSObject;
    }

    // what cell are we over?
    var row = { }, col = { }, child = { };
    tree.treeBoxObject.getCellAt(evt.pageX, evt.pageY, row, col, child);
    
    if(row.value != -1) {
      _insert_index = row.value;
    }
  }
  
  _library_resource = libraryResource;
  
  if (getPlatformString() == "Windows_NT") lcase = 1;
  
  if (!_drop_filelist) _drop_filelist = Array();

  for (var i = 0; i < dataListLength; i++) 
  {
    var item = dataList[i].first;
    var prettyName;
    var rawData = item.data;
    
    if (item.flavour.contentType == "application/x-moz-file")
    {
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
      var fileHandler = ioService.getProtocolHandler("file")
                        .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
      rawData = fileHandler.getURLSpecFromFile(rawData);
    } 
    else
    {
      if (rawData.toLowerCase().indexOf("http://") < 0) continue;
    }

    var separator = rawData.indexOf("\n");
    if (separator != -1) 
    {
      prettyName = rawData.substr(separator+1);
      rawData = rawData.substr(0,separator);
    }
    
    if (lcase) rawData = rawData.toLowerCase();
    _drop_filelist.push(rawData);
  }
  
  if (!importingDrop) {
    firstDrop = true;
    importingDrop = true;
    setTimeout( SBDropped, 10 ); // Next frame
    importingDrop = false;
  }
}

function SBDropped() 
{
  var url;
  try {
    if (_drop_filelist.length > 0) {
      url = _drop_filelist[0];
      _drop_filelist.splice(0, 1);
      if (url != "") {
        var ios = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
        var uri = ios.newURI(url, null, null);
        theDropPath = uri.spec;
        var fileUrl;
        try {
          fileUrl = uri.QueryInterface(Components.interfaces.nsIFileURL);
        } catch (e) { 
          fileUrl = null; 
        }
        if (fileUrl) {
          theDropIsDir = fileUrl.file.isDirectory();
          if(theDropIsDir) {
            theDropPath = fileUrl.file.path;
          }
        }
        else {
          theDropIsDir = false;
        }
      }
      SBDroppedEntry();
      setTimeout( SBDropped, 10 ); // Next frame
    }
  } catch (e) {
    dump(e);
    setTimeout( SBDropped, 10 ); // Try next frame
  }
}

var theDropPath = "";
var theDropIsDir = false;

function SBDroppedEntry()
{
  try {    
    if ( theDropIsDir ) {
      theFileScanIsOpen.boolValue = true;
      
      // otherwise, fire off the media scan page.
      var media_scan_data = new Object();

      media_scan_data.target_db =  null;
      media_scan_data.target_pl = null;
      media_scan_data.target_pl_row = -1;
      
      if(_library_resource instanceof Ci.sbILibrary &&
         !SBIsWebLibrary(_library_resource.guid)) {
        media_scan_data.target_db = _library_resource.guid;
      }
      else if(_library_resource instanceof Ci.sbIOrderableMediaList) {
        media_scan_data.target_pl = _library_resource;
        media_scan_data.target_pl_row = _insert_index;
      }
      
      media_scan_data.URL = theDropPath;
      media_scan_data.retval = "";

      // Open the modal dialog
      SBOpenModalDialog( "chrome://songbird/content/xul/mediaScan.xul", "media_scan", "chrome,centerscreen", media_scan_data ); 
      
      theFileScanIsOpen.boolValue = false;
      _insert_index = -1;
    } 
    else if (gPPS.isMediaURL( theDropPath )) {
      if(firstDrop) {
        firstDrop = false;

        //Import the track into the main library.
        var item = SBImportURLIntoMainLibrary(theDropPath);
        
        dump(_library_resource + "\n");
        for(var prop in _library_resource) {
          try {
            dump(prop + ":" + _library_resource[prop] + "\n");
          }catch(e) {}
        }
        
        if(_library_resource) {
          if(_library_resource.type == "simple" &&
             !SBIsWebLibrary(_library_resource)) {
            _library_resource.add(item);
          }
          
          SBDisplayViewForListAndPlayItem(_library_resource, item);
        }
        else {
          var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                 .getService(Ci.sbILibraryManager);

          SBDisplayViewForListAndPlayItem(libraryManager.mainLibrary, item);
        }
      }
    } else if ( isXPI( theDropPath ) ) {
      installXPI(theDropPath);
    }
  } catch(e) { alert( "SBDroppedEntry\n\n" + e ); listProperties( e ); }
}
