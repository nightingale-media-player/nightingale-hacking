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

var gRemoteAPIPane = {

  _exceptionsParams: {
    // provides initialization data to the permissions dialog
    metadata: { blockVisible: true, sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "rapi.metadata" },
    controls: { blockVisible: true, sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "rapi.controls" },
    binding:  { blockVisible: true, sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "rapi.binding"   }
  },

  _showExceptions: function (aPermissionType)
  {
    // get ref to the properties file string bundle
    var bundlePreferences = document.getElementById("bundlePreferences");

    // set up a parmater object to pass to permission window
    var params = this._exceptionsParams[aPermissionType];

    // Pull the title and text for the particular permission from the properties file
    params.windowTitle = bundlePreferences.getString("rapi." + aPermissionType + ".permissions_title");
    params.introText = bundlePreferences.getString("rapi." + aPermissionType + ".permissions_text");

    // open the permission window to set allow/disallow/session permissions
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "",
                                        params);
  },

  showMetadataWhitelist: function ()
  {
    this._showExceptions("metadata");
  },

  showControlsWhitelist: function ()
  {
    this._showExceptions("controls");
  },

  showBindingWhitelist: function ()
  {
    this._showExceptions("binding");
  },

};

