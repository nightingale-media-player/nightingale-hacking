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



//
// Module specific auto-init/deinit support
//
var dragWindow = {};
dragWindow.init_once = 0;
dragWindow.deinit_once = 0;
dragWindow.onLoad = function()
{
  if (dragWindow.init_once++) { dump("WARNING: dragWindow double init!!\n"); return; }
  // Module Init
  document.addEventListener("draggesture", onBkgDown, false);
}
dragWindow.onUnload = function()
{
  if (dragWindow.deinit_once++) { dump("WARNING: dragWindow double deinit!!\n"); return; }
  window.removeEventListener("load", dragWindow.onLoad, false);
  window.removeEventListener("unload", dragWindow.onUnload, false);
  // Module Deinit
  document.removeEventListener("draggesture", onBkgDown, false);
}
window.addEventListener("load", dragWindow.onLoad, false);
window.addEventListener("unload", dragWindow.onUnload, false);

//Necessary when WindowDragger is not available on the current platform.
var trackerBkg = false;
var offsetScrX = 0;
var offsetScrY = 0;

var windowDragCallback = {
  onWindowDragComplete: function () {
    onWindowDragComplete();
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowDraggerCallback) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
  
};

// The background image allows us to move the window around the screen
function onBkgDown( theEvent, popup ) 
{
  if (maximized) 
    return;
  
  if ( (theEvent.target.getAttribute("drag_window") == "true") ||
       ( (document.documentElement.getAttribute("spacers_drag_window") == "true") &&
         (theEvent.target.nodeName == "spacer") ) )
  {
    try
    {
      var windowDragger = Components.classes["@songbirdnest.com/Songbird/WindowDragger;1"];
      if (windowDragger) {
        var service = windowDragger.getService(Components.interfaces.sbIWindowDragger);
        if (service) {
          var dockDistance = (window.dockDistance ? window.dockDistance : 0);
          service.beginWindowDrag(dockDistance, windowDragCallback); // automatically ends
        }
      }
      else {
        trackerBkg = true;
        offsetScrX = document.documentElement.boxObject.screenX - theEvent.screenX;
        offsetScrY = document.documentElement.boxObject.screenY - theEvent.screenY;
        // ScreenY is reported incorrectly on osx for non-popup windows without title bars.
        if ( ( popup != true ) && (navigator.userAgent.indexOf("Mac OS X") != -1) ) {
          // TODO: This will be incorrect in the jumptofile dialog, as it is loaded as a popup.
          // How do I know from this scope whether the window is loaded as popup?
          offsetScrY -= 22; 
        }
        document.addEventListener( "mousemove", onBkgMove, true );
      }
    }
    catch(err) {
      dump("Error. Songbird.js::onBkDown() \n" + err + "\n");
    }
  }
/*  
  else
    alert( theEvent.target.nodeName + " - " + theEvent.target.id + " - " + theEvent.target.getAttribute("drag_window") );
*/    
}

function onBkgMove( theEvent ) 
{
  if ( trackerBkg )
  {
    document.defaultView.moveTo( offsetScrX + theEvent.screenX, offsetScrY + theEvent.screenY );
  }
}

function onBkgUp( ) 
{
  if ( trackerBkg )
  {
    trackerBkg = false;
    onWindowDragComplete();
    document.removeEventListener( "mousemove", onBkgMove, true );
  }
}
