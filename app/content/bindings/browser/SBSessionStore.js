// vim: set sw=2 :miv
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const Application = Cc["@mozilla.org/fuel/application;1"].getService();
const JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

const PREF_TAB_STATE = "songbird.browser.tab_state";
const PREF_FIRSTRUN = "songbird.firstrun.tabs.restore";
const PREF_FIRSTRUN_URL = "songbird.url.firstrunpage";
const PLACEHOLDER_URL = "chrome://songbird/content/mediapages/firstrun.xul";
// this pref marks the current session as being firstrun.  Used to cooperate
// with things like file scan.
const PREF_FIRSTRUN_SESSION = "songbird.firstrun.is_session";

function LOG(str) {
  // var environment = Cc["@mozilla.org/process/environment;1"]
  //                     .createInstance(Ci.nsIEnvironment);
  // var level = ("," + environment.get("NSPR_LOG_MODULES") + ",")
  //             .match(/,(?:sbSessionStore|all):(\d+),/);
  // if (!level || level[1] < 3) {
  //   // don't log
  //   return;
  // }
  // var file = (new Error).stack.split("\n").reverse()[1];
  // dump(file + "" + str + "\n");

  // dump("\n\n\n");
  // dump(str);
  // dump("\n\n\n");
  // var consoleService = Cc['@mozilla.org/consoleservice;1']
  //                        .getService(Ci.nsIConsoleService);
  // consoleService.logStringMessage(str);
}

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
            var view = tab.mediaPage.mediaListView;
            var mediaList = view.mediaList;
            var data = {
              listGUID: mediaList.guid,
              libraryGUID: mediaList.library.guid,
              pageURL: tab.linkedBrowser.currentURI.spec,
              isOnlyView: tab.mediaPage.isOnlyView
            };
            if (view instanceof Ci.sbIFilterableMediaListView) {
              // Clear the cascade filter set so that the filter constraints
              // only include the original constraints on the view, not those
              // imposed by the CFS. We do this on a copy of the view to avoid
              // changing the state of the original.
              var cloneView = view.clone();
              cloneView.cascadeFilterSet.clearAll();

              data.constraints = [];
              for (var group in ArrayConverter.JSEnum(cloneView.filterConstraint.groups)) {
                if (!(group instanceof Ci.sbILibraryConstraintGroup)) {
                  // this shouldn't happen... but let's be nice and not die
                  continue;
                }
                let propArray = [];
                for (let prop in ArrayConverter.JSEnum(group.properties)) {
                  propArray.push([prop,
                                  ArrayConverter.JSArray(group.getValues(prop))]);
                }
                data.constraints.push(propArray);
              }
              if (!data.constraints.length) {
                // no constraints; this shouldn't ever happen either (we should
                // have the standard not hidden one), but let's not die
                delete data.constraints;
              }
            }
            urls.push(data);
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

    _tabState = {
                  selectedTabIndex: aTabBrowser.tabContainer.selectedIndex,
                  urlList: urls
                };
  },

  restoreTabState: function restoreTabState(aTabBrowser)
  {
    var tabObject = _tabState;
    var tabs = [];
    var selectedIndex = 0, selectedTab;

    LOG("restoring tab state");

    if (Application.prefs.has(PREF_FIRSTRUN_SESSION))
      Application.prefs.get(PREF_FIRSTRUN_SESSION).reset();

    if (tabObject && "urlList" in tabObject) {
      // v2 of the tab state object (with selectedTabIndex)
      tabs = tabObject.urlList;
      selectedIndex = tabObject.selectedTabIndex || 0;
    } else {
      // v1 of the tab state object (no selected tab index, only urls)
      tabs = tabObject;
    }

    var isFirstTab = true;
    if ( !tabs || !tabs.length ) {
      if (!Application.prefs.getValue(PREF_FIRSTRUN, false)) {
        LOG("no saved tabs, first run - using defaults");
        // First run, load the dummy page in the first tab, and the welcome
        // page in the second.  The dummy page will get replaced in mainWinInit.js
        // when media scan is done / skipped.
        aTabBrowser.loadURI(PLACEHOLDER_URL, null, null, null, '_media');
        isFirstTab = false;

        var loadMLInBackground =
          Application.prefs.getValue("songbird.firstrun.load_ml_in_background",
                                     false);
        var firstrunURL = Application.prefs.getValue(PREF_FIRSTRUN_URL,
                                                     "about:blank");
        // If the pref to load the medialist in the background is true, then
        // we want to load the firstrun page in the foreground
        selectedTab = aTabBrowser.loadOneTab(firstrunURL, null, null, null,
                                             !loadMLInBackground);

        Application.prefs.setValue(PREF_FIRSTRUN, true);
        Application.prefs.setValue(PREF_FIRSTRUN_SESSION, true);
      }
    } else {
      LOG("saved tabs found: " + uneval(tabs));

      // check if this is an invalid chrome url
      var chromeReg = Cc['@mozilla.org/chrome/chrome-registry;1']
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
          uri = chromeReg.convertChromeURL(uri);
        } catch (e) {
          // an exception here means that this chrome URL isn't valid
          return true;
        }
        // if the scheme *is* chrome, then something's wrong
        // (recursive chrome mapping)
        if (uri.scheme == 'chrome') {
          return true;
        }
        // ok - things look fine
        return false;
      }

      // Otherwise, just restore whatever was there, previously.
      var tab, location;
      for (var i = 0; i < tabs.length; i++) {
        tab = tabs[i];
        var newTab = null;

        var url = (tab.pageURL ? tab.pageURL : tab);
        if (isInvalidChromeURL(url)) {
          // we don't want to restore invalid chrome URLs
          continue;
        }
        if (url == "about:blank") {
          // skip restoring blank tabs
          continue;
        }

        // If the tab had a media page, restore it by reloading
        // the media list
        if (tab.listGUID) {

          // HACK! Add a random param to the querystring in order to avoid
          // the XUL cache.  This is a work around for the following bugs:
          //
          // Bug 7896   - Media pages do not initialize when loaded from tab restore
          // BMO 420815 - XUL Cache interferes with onload when loading multiple
          //              instances of the same XUL file
          if (url.indexOf("&bypassXULCache") == -1) {
            url += "&bypassXULCache="+ Math.random();
          }

          if (isFirstTab) {
            // restore the first tab into the media tab, if available
            location = "_media";
          } else {
            location = "_blank";
          }

          try {
            var list = LibraryUtils.getMediaListByGUID(tab.libraryGUID,
                                                       tab.listGUID);
          }
          catch (e if e.result == Components.results.NS_ERROR_NOT_AVAILABLE) {
            // not available, just go to the library.
            var list = LibraryUtils.mainLibrary;
          }

          let view = LibraryUtils.createStandardMediaListView(list);
          if ("constraints" in tab) {
            view.filterConstraint = LibraryUtils.createConstraint(tab.constraints);
          }
          newTab = aTabBrowser.loadMediaList(list,
                                             null,
                                             location,
                                             view,
                                             url,
                                             tab.isOnlyView);

        // Otherwise just reload the URL
        } else {
          LOG("loading plain url: " + url);
          if (isFirstTab) {
            if (aTabBrowser.mediaTab) {
              // let the placeholder URL load in the media tab (again).
              if ((PLACEHOLDER_URL == url) ||
                  LibraryUtils.isMediaTabURL(url, aTabBrowser))
              {
                LOG("this is the placeholder tab or media url");
                // this is the first tab, and is a media-ish url
                location = "_media";
              } else {
                LOG(<>not a media-esque tab ({firstrunURL} vs {url}: {(firstrunURL == url)})</>);
                // this is the first tab, but this is unsuitable for the media tab
                location = "_top";
                // load the library page to fill up the media tab
                libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                           .getService(Ci.sbILibraryManager);
                mainLib = libMgr.mainLibrary;
                newTab = aTabBrowser.loadMediaList(mainLib);
              }
            } else {
              // no media tab, but this is the first tab
              LOG("no media tab found, just use first tab");
              location = "_top";
            }
          } else {
            LOG("not first tab, always _blank");
            location = "_blank";
          }

          // don't load device pages for a device that isn't mounted
          var uri = ios.newURI(url, null, null);
          if (uri.scheme == 'chrome' && (uri.path.indexOf("?device-id") >= 0)) {
            var deviceId = uri.path.match(/\?device-id=\{([0-9a-z\-]+)\}/);
            if (deviceId) {
              deviceId = deviceId[1];

              var deviceMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                              .getService(Ci.sbIDeviceManager2);
              try {
                deviceMgr.getDevice(Components.ID(deviceId));
                // It's a valid device, so go ahead and open up the chrome page
                newTab = aTabBrowser.loadURI(url, null, null, null, location);
              } catch (e) {
                // This is an invalid device, just load the main library view.
                var libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                               .getService(Ci.sbILibraryManager);
                var mainLib = libMgr.mainLibrary;
                aTabBrowser.loadMediaList(mainLib);
              }
            }
          } else {
            // It's not a device page, so go ahead and load it like normal
            newTab = aTabBrowser.loadURI(url, null, null, null, location);
          }
        }

        // Load the first url into the current tab and subsequent
        // urls into new tabs
        isFirstTab = false;

        if (i == selectedIndex) {
          // note that this might not match the actual tab index, if there are
          // any saved tabs that are now at an invalid chrome URL.  That's fine,
          // because we just want to selected the same content, not the index.
          selectedTab = newTab;
        }
      }
    }

    // Let's just go to the main library when:
    // - there was only one tab and that tab is an invalid chrome url
    //   (isInvalidChromeURL says true)
    // - or no saved tabs, not first run, and tab state pref is missing/corrupt.
    if (isFirstTab) {
      var libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                     .getService(Ci.sbILibraryManager);
      var mainLib = libMgr.mainLibrary;
      aTabBrowser.loadMediaList(mainLib);
    }

    // Select the selected tab from the previous session (or the first one if
    // we don't know which one that is)
    // use delayedSelectTab because sbTabBrowser::loadURI uses a timeout to
    // force the new tab to be selected :(
    if (!selectedTab && aTabBrowser.mediaTab) {
      selectedTab = aTabBrowser.mediaTab;
    }
    aTabBrowser.delayedSelectTab(selectedTab);

    this.tabStateRestored = true;

    // tell the tab browser we switched tabs so it can update state correctly
    var selectEvent = document.createEvent("Events");
    selectEvent.initEvent("select", true, true);
    aTabBrowser.tabStrip.dispatchEvent(selectEvent);
  },

  tabStateRestored: false
};
