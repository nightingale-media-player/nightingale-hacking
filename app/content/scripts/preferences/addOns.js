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

  _addonsDoc: null,

  _hideResizer: function() {
    const resizerBox = this._addonsDoc.getElementById("resizerBox");
    resizerBox.setAttribute("hidden", "true");
  },

  _hidePlugins: function() {
    const viewGroup = this._addonsDoc.getElementById("viewGroup");
    if (viewGroup.getAttribute("last-selected") != "extensions") {
      // Tell the EM's Startup() function to select the extensions view.
      viewGroup.setAttribute("last-selected", "extensions");
    }

    // Hide the plugins radio option.
    const pluginRadio = this._addonsDoc.getElementById("plugins-view");
    pluginRadio.setAttribute("hidden", "true");
  },

  load: function load() {
    const self = AddonsPrefPane;

    // We need to wait until the IFrame loads completely
    const addonsIFrame = document.getElementById("addonsFrame");
    addonsIFrame.addEventListener("load", self.onEMLoad, true);
  },

  onEMLoad: function onEMLoad() {
    const self = AddonsPrefPane;

    const addonsIFrame = document.getElementById("addonsFrame");
    addonsIFrame.removeEventListener("load", self.onEMLoad, true);

    self._addonsDoc = addonsIFrame.contentDocument;

    self._hideResizer();
    self._hidePlugins();
  }
};
