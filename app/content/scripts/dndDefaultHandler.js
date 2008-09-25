// JScript source code
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

if (typeof(ExternalDropHandler) == "undefined")
  Components.utils.import("resource://app/jsmodules/DropHelper.jsm");


// Module specific global for auto-init/deinit support
var dndDefaultHandler_module = {};
dndDefaultHandler_module.init_once = 0;
dndDefaultHandler_module.deinit_once = 0;

dndDefaultHandler_module.onDragOver = function(event) { nsDragAndDrop.dragOver( event, SBDropObserver ) }
dndDefaultHandler_module.onDragDrop = function(event) { nsDragAndDrop.drop( event, SBDropObserver ) }
dndDefaultHandler_module.onLoad = function(event)
{
  if (dndDefaultHandler_module.init_once++) { dump("WARNING: dndDefaultHandler_module double init!!\n"); return; }
  
  document.addEventListener( "dragover", dndDefaultHandler_module.onDragOver, false );
  document.addEventListener( "dragdrop", dndDefaultHandler_module.onDragDrop, false );
}

dndDefaultHandler_module.onUnload = function()
{
  if (dndDefaultHandler_module.deinit_once++) { dump("WARNING: dndDefaultHandler_module double deinit!!\n"); return; }
  document.removeEventListener( "dragover", dndDefaultHandler_module.onDragOver, false );
  document.removeEventListener( "dragdrop", dndDefaultHandler_module.onDragDrop, false );
  window.removeEventListener("load", dndDefaultHandler_module.onLoad, false);
  window.removeEventListener("unload", dndDefaultHandler_module.onUnload, false);
}
// Auto-init/deinit registration
window.addEventListener("load", dndDefaultHandler_module.onLoad, false);
window.addEventListener("unload", dndDefaultHandler_module.onUnload, false);

var SBDropObserver = 
{
  canHandleMultipleItems: true,
 
  getSupportedFlavours : function () 
  {
    var flavours = new FlavourSet();
    ExternalDropHandler.addFlavours(flavours);
    return flavours;
  },
  
  onDragOver: function ( evt, flavour, session ) {},
  onDrop: function ( evt, dropdata, session )
  {
    var dropHandlerListener = {
      observer: SBDropObserver,
      onDropComplete: function(aTargetList,
                               aImportedInLibrary,
                               aDuplicates,
                               aInsertedInMediaList,
                               aOtherDropsHandled) {
        // show the standard report on the status bar
        return true;
      },
      onFirstMediaItem: function(aTargetList, aFirstMediaItem) {
        this.observer.playFirstDrop(aFirstMediaItem);
      },
    };
    ExternalDropHandler.drop(window, session, dropHandlerListener);
  },
  
  playFirstDrop: function(firstDrop) {
    var view = LibraryUtils.createStandardMediaListView(LibraryUtils.mainLibrary);

    var index = view.getIndexForItem(firstDrop);
    
    // If we have a browser, try to show the view
    if (window.gBrowser) {
      gBrowser.showIndexInView(view, index);
    }
    
    // Play the item
    gMM.sequencer.playView(view, index);
  }
};

