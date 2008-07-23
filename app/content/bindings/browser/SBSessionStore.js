// vim: set sw=2 :miv
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

/**
 * Session Store
 *
 * This is loaded into the tabbrowser as an object to provide session restore
 * services (remember which tabs are open to which locations, and restore them
 * on restart).  It is not expected to be used separately from the tabbrowser.
 *
 * Currently it depends on tabbrowser internals.
 */

const EXPORTED_SYMBOLS = ["SBSessionStore"];

if ("undefined" == typeof(XPCOMUtils))
  Components.utils.import("resource://app/jsmodules/XPCOMUtils.jsm");
if ("undefined" == typeof(LibraryUtils))
  Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const Application = Cc["@mozilla.org/fuel/application;1"].getService();
const JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

const PREF_TAB_STATE = "songbird.browser.tab_state";
const PREF_FIRSTRUN = "songbird.firstrun.tabs.restore";
const PREF_FIRSTRUN_URL = "songbird.url.firstrunpage";

__defineGetter__("_tabState", function() {
  var state = Application.prefs.getValue(PREF_TAB_STATE, null);
  if (state === null)
    return null;
  try {
    return JSON.decode(state);
  } catch(e) {
    Components.utils.reportError("Error restoring tab state: invalid JSON\n" +
                                 state);
    return null;
  }
});

__defineSetter__("_tabState", function(aValue) {
  Application.prefs.setValue(PREF_TAB_STATE, JSON.encode(aValue));
});

var SBSessionStore = {
  saveTabState: function saveTabState(aTabBrowser)
  {
    var tab = aTabBrowser.mTabs[0];
    var urls = [];
    while (tab) {
      try {
        // For media pages, store the list GUIDs
        // so that the page can be restored correctly
        if (tab.mediaPage) {
          if (tab.mediaPage.mediaListView) {
            var mediaList = tab.mediaPage.mediaListView.mediaList;
            urls.push({
                listGUID: mediaList.guid,
                libraryGUID: mediaList.library.guid,
                pageURL: tab.linkedBrowser.currentURI.spec
              });
          }
        // For all other pages, just keep the URI
        } else {
          urls.push(tab.linkedBrowser.currentURI.spec);
        }
      } catch (e) { 
        Components.utils.reportError(e);
      }
      tab = tab.nextSibling;
    }
    
    _tabState = urls;
  },
  
  restoreTabState: function restoreTabState(aTabBrowser)
  {
    var tabs = _tabState;
    
    if ( !tabs ) {
      // either first run, or pref missing/corrupt.
      // let's just go to the main library
      var libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                     .getService(Ci.sbILibraryManager);
      var mainLib = libMgr.mainLibrary;
      aTabBrowser.loadMediaList(mainLib);
      
      if (!Application.prefs.getValue(PREF_FIRSTRUN, false)) {
        // Also load the first-run page in the background.
        var firstrunURL = Application.prefs.getValue(PREF_FIRSTRUN_URL, "about:blank");
        aTabBrowser.loadOneTab(firstrunURL, null, null, null, true);
        Application.prefs.setValue(PREF_FIRSTRUN, true);
      }
    } else {
  
      // check if this is an invalid chrome url
      var cr = Cc['@mozilla.org/chrome/chrome-registry;1']
          .getService(Ci.nsIChromeRegistry);
      var ios = Cc["@mozilla.org/network/io-service;1"]
          .getService(Ci.nsIIOService);
      function isInvalidChromeURL(url) {
        var uri;
        // parse the URL
        try {
          uri = ios.newURI(url, null, null);
        } catch (e) {
          // can't parse the url, that will be handled elsewhere
          return false;
        }
        // if it's not chrome then we don't care
        if (uri.scheme != 'chrome') {
          return false;
        }
        // resolve the chrome url with the registry
        try {
          uri = cr.convertChromeURL(uri);
        } catch (e) {
          // an exception here means that this chrome URL isn't valid
          return true;
        }
        // if the scheme *is* chrome, then something's wrong
        if (uri.scheme == 'chrome') {
          return true;
        }
        // ok - things look fine
        return false;
      }
  
      // Otherwise, just restore whatever was there, previously.
      var isFirstTab = true;
      var tab, location;
      for (var i=0; i<tabs.length; i++) {
        tab = tabs[i];
        // If the tab had a media page, restore it by reloading
        // the media list
        if (tab.listGUID) {

          // HACK! Add a random param to the querystring in order to avoid
          // the XUL cache.  This is a work around for the following bugs:
          //
          // Bug 7896   - Media pages do not initialize when loaded from tab restore
          // BMO 420815 - XUL Cache interferes with onload when loading multiple 
          //              instances of the same XUL file
          var url = tab.pageURL;
          if (isInvalidChromeURL(url)) {
            // we don't want to restore invalid chrome URLs
            continue;
          }
          if (url.indexOf("&bypassXULCache") == -1) {
            url += "&bypassXULCache="+ Math.random();
          }
          
          if (isFirstTab) {
            // restore the first tab into the media tab, if available
            location = "_media";
          } else {
            location = "_blank";
          }
          
          var list = LibraryUtils.getMediaListByGUID(tab.libraryGUID,
                                                     tab.listGUID);
          aTabBrowser.loadMediaList(list, null, location, null, url);
          
        // Otherwise just reload the URL
        } else {
          if (isInvalidChromeURL(tab)) {
            // we don't want to restore invalid chrome URLs
            continue;
          }
          
          if (isFirstTab) {
            // let the first run URL load in the media tab (again).
            var firstrunURL = Application.prefs.getValue(PREF_FIRSTRUN_URL, null);
            if ((firstrunURL == tab) ||
                (gServicePane && gServicePane.mTreePane.isMediaTabURL(tab))) {
              location = "_media";
            } else {
              location = "_top";
            }
          } else {
            location = "_blank";
          }
          aTabBrowser.loadURI(tab, null, null, null, location);
        }

        // Load the first url into the current tab and subsequent 
        // urls into new tabs 
        isFirstTab = false;
      }
    }
    this.tabStateRestored = true;
    
    // tell the tab browser we switched tabs so it can update state correctly
    var selectEvent = document.createEvent("Events");
    selectEvent.initEvent("select", true, true);
    aTabBrowser.tabStrip.dispatchEvent(selectEvent);
  },
  
  tabStateRestored: false
};
