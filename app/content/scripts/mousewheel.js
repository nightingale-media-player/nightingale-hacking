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
//  Mouse Wheel Support
//

try 
{
  function SBInitMouseWheel()
  {
    window.addEventListener("DOMMouseScroll", onMouseWheel, false);
  }

  function onMouseWheel(event)
  {
    try
    {
      var node = event.originalTarget;
      while (node != document && node != null)
      {
        // if your object implements an event on the wheel,
        // but is not one of these, you should prevent the 
        // event from bubbling
        if (node.tagName == "tree") return;
        if (node.tagName == "xul:tree") return;
        if (node.tagName == "listbox") return;
        if (node.tagName == "xul:listbox") return;
        if (node.tagName == "browser") return;
        if (node.tagName == "xul:browser") return;
        node = node.parentNode;
      }

      if (node == null)
      {
        // could not walk up to the window before hitting a document, 
        // we're inside a sub document. the event will continue bubbling, 
        // and we'll catch it on the next pass
        return;
      }
    
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);

      // walked up to the window
      var s = PPS.getVolume();
      var v = parseInt(s)+((event.detail > 0) ? -8 : 8);
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      PPS.setVolume(v);
      if (v != 0) SBDataSetValue("faceplate.volume.last", v);
    }
    catch (err)
    {
      alert("onMouseWheel - " + err);
    }
  }
}
catch (err)
{
  alert("mousewheel.js - " + err);
}
