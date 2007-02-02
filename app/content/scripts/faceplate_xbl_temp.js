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
// our Faceplate XBL object that we'll be updating for 0.3

function initFaceplateButton() {
  var button = document.getElementById("songbird_btn_next");
  button.addEventListener("mousedown", buildFaceplateMenuItems, true);
}

function shutdownFaceplateButton() {
  var button = document.getElementById("songbird_btn_next");
  button.removeEventListener("mousedown", buildFaceplateMenuItems, true);
}

function buildFaceplateMenuItems() {
  var button = document.getElementById("songbird_btn_next");
  while (button.firstChild) button.removeChild(button.firstChild);
  var faceplate = document.getElementById("main_faceplate");
  for (var i=0;i<faceplate.getNumPanes();i++) {
    var name = faceplate.getPaneName(i);
    var item = document.createElement("menuitem");
    item.setAttribute("id", faceplate.getPaneId(i));
    item.setAttribute("label", name);
    item.setAttribute("checked", (faceplate.getCurPane() == i) ? "true" : "false");
    item.setAttribute("type", "checkbox");
    button.appendChild(item);
  }
}

function onFaceplateMenu(element) {
  var faceplate = document.getElementById("main_faceplate");
  var id = element.getAttribute("id");
  if (id == "songbird_btn_next") {
    faceplate.switchToNextPane();
  } else {
    var index = faceplate.getPaneIndex(id);
    faceplate.switchToPane(index);
  }
}

