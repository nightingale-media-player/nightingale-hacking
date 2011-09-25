// vim: set sw=2 :miv
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//

/**
 * BrowserDOMWindow
 *
 * nsIBrowserDOMWindow implementation for nightingale; used in sbTabBrowser ctor
 *
 * This currently will open everything in new tabs.
 */

const EXPORTED_SYMBOLS = ["BrowserDOMWindow"];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function BrowserDOMWindow(aTabBrowser) {
  this._tabBrowser = aTabBrowser;
  this.wrappedJSObject = this;
}

BrowserDOMWindow.prototype = {
  _tabBrowser: null,
  
  openURI: function BrowserDOMWindow_openURI(aURI, aOpener, aWhere, aContext) {
    // XXX Mook: for now, just always open in new tab (ignore aWhere)
    if (aURI instanceof Components.interfaces.nsIURI)
      aURI = aURI.spec;
    // XXX Mook: if the tab isn't loaded in the background,
    // onLocationChange freaks out; so we focus it later if needed
    var newTab = this._tabBrowser.addTab("about:blank", null, null, null, true, false);
    var newWindow = this._tabBrowser
                        .getBrowserForTab(newTab)
                        .docShell
                        .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                        .getInterface(Components.interfaces.nsIDOMWindow);
    try {
      var loadflags = (aContext == Components.interfaces.nsIBrowserDOMWindow.OPEN_EXTERNAL) ?
                      Components.interfaces.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                      Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
      if (aOpener) {
        var referrer =
                Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService)
                          .newURI(aOpener.location, null, null);
      }
      if (!aURI)
        aURI = "about:blank";
      newWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
               .getInterface(Components.interfaces.nsIWebNavigation)
               .loadURI(aURI, loadflags, referrer, null, null);
    } catch(e) {
      // silently eat the error
    }
    var loadInBackground = this._tabBrowser
                               .mPrefs
                               .getBoolPref("browser.tabs.loadDivertedInBackground");
    if (!loadInBackground) {
      this._tabBrowser.selectedTab = newTab;
    }
    return newWindow;
  },
  
  isTabContentWindow: function BrowserDOMWindow_isTabContentWindow(aWindow) {
    for (var ctr = 0; ctr < this._tabBrowser.browsers.length; ctr++)
      if (this._tabBrowser.browsers.item(ctr).contentWindow == aWindow)
        return true;
    return false;
  },
  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIBrowserDOMWindow])
};
