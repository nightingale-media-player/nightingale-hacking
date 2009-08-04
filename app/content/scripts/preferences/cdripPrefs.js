/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
// \file cdripPrefs.js
// \brief Javascript source for the CD Rip preferences UI.
//

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

//------------------------------------------------------------------------------
//
// CD Rip preference pane UI controller.
//
//------------------------------------------------------------------------------

var CDRipPrefsUI =
{
  initialize: function CDRipPrefsUI_initialize() {
    //
    // XXXkreeger:
    // Nothing to do until transcoding profiles are available (for populating
    // the format and quality popups).
    //
  }
};

//------------------------------------------------------------------------------
//
// CD Rip preference pane services.
//
//------------------------------------------------------------------------------

var CDRipPrefsPane =
{
  doPaneLoad: function CDRipPrefsPane_doPaneLoad() {
    CDRipPrefsUI.initialize();
  }
}
