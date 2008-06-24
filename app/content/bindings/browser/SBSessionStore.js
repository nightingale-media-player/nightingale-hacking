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

const PREF_TAB_STATE = "songbird.browser.tab_state";
const PREF_FIRSTRUN = "songbird.firstrun.tabs.restore";
const PREF_FIRSTRUN_URL = "songbird.url.firstrunpage";

__defineGetter__("_tabState", function() {
  if (Application.prefs.has(PREF_TAB_STATE)) {
    // XXX Mook: EWW, JSON me
    return eval(Application.prefs.getValue(PREF_TAB_STATE, "null"));
  } else {
    return null;
  }
});
__defineSetter__("_tabState", function(aValue) {
  Application.prefs.setValue(PREF_TAB_STATE, aValue.toSource());
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
    if ( !Application.prefs.getValue(PREF_FIRSTRUN, false) ) {
      // If we have never run the app before, load this keen stuff!@
      var homePageURL = aTabBrowser.homePage;
      var firstrunURL = Application.prefs.getValue(PREF_FIRSTRUN_URL, "about:blank");
      aTabBrowser.loadURI(homePageURL, null, null, null, '_top');
      aTabBrowser.loadURI(firstrunURL, null, null, null, '_blank');
      Application.prefs.setValue(PREF_FIRSTRUN, true);
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
      var tabs = _tabState;
      if (tabs) {
        var location = "_top";
        var tab;              
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
            
            var list = LibraryUtils.getMediaListByGUID(tab.libraryGUID,
                                                       tab.listGUID);
            aTabBrowser.loadMediaList(list, null, location, null, url);
            
          // Otherwise just reload the URL
          } else {
            if (isInvalidChromeURL(tab)) {
              // we don't want to restore invalid chrome URLs
              continue;
            }
            aTabBrowser.loadURI(tab, null, null, null, location);
          }
  
          // Load the first url into the current tab and subsequent 
          // urls into new tabs 
          location = "_blank";
        }
      }
    }
    this.tabStateRestored = true;
  },
  
  tabStateRestored: false
};
