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

var AddonsPrefPane = {

  load: function load() {
    // We need to wait until the IFrame loads completely
    var addonsIFrame = document.getElementById("addonsFrame");
    addonsIFrame.addEventListener("load", this.onEMLoad, true);
  },
  
  onEMLoad: function onEMLoad() {
    // Kill the resizer
    var addonsIFrame = document.getElementById("addonsFrame");
    var addonsDoc = addonsIFrame.contentDocument;
    var resizerBox = addonsDoc.getElementById("resizerBox");
    if (resizerBox)
      resizerBox.style.display = "none";
  }
};
