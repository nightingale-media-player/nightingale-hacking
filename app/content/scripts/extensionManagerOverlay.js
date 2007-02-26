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

function fixResizers() {
  window.removeEventListener("load", fixResizers, false);

  // Resizers are broken in release mode for some reason... Worthy of further
  // investigation. Do this for now, but remove once bug 2324 is resolved.
  var resizersList = document.getElementsByTagName("resizer");
  var resizerCount = resizersList.length;
  for (var index = 0; index < resizerCount; index++) {
    var resizer = resizersList.item(index);
    if (resizer)
      resizer.dir = resizer.dir;
  }
}

function fixExtensionsLink() {
  var getMore = document.getElementById("getMore");
  if (getMore) {
    getMore.setAttribute("onclick",
                         "openURLSB(this.getAttribute('getMoreURL'));");
  }
}

function openURLSB(url) {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
  var mainWin = wm.getMostRecentWindow("Songbird:Main");
  if (mainWin) {
    var st = mainWin.document.getElementById("frame_servicetree");
    if (st) {
      st.launchServiceURL(url);
      mainWin.focus();
      return;
    }
  }

  openURL(url);
}

window.addEventListener("load", fixResizers, false);
window.addEventListener("load", fixExtensionsLink, false);

