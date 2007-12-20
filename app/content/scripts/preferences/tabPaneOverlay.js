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

var SongbirdTabPaneOverlay = {
  /**
   * This function sets up our UI in the main preferences dialog.
   */
   onPaneLoad: function(event) {
     // Don't actually load until the pane has been loaded (see the comments
     // near the matching addEventListener call at the end of this code).
     if (event.target.getAttribute("id") != "paneTabs") {
       return;
     }

    window.removeEventListener('paneload',
                               SongbirdTabPaneOverlay.onPaneLoad,
                               false);

    const warnOpenMany = document.getElementById("warnOpenMany");
    warnOpenMany.setAttribute("hidden", "true");
  }
};

// Don't forget to load! Can't use the standard load event because the
// individual preference panes are loaded asynchronously via
// document.loadOverlay (see preferences.xml) and the load event may fire before
// our target pane has been integrated.
window.addEventListener('paneload',
                        SongbirdTabPaneOverlay.onPaneLoad,
                        false);
