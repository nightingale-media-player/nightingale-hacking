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

/**
 * \file  albumArtwork.js
 * \brief Javascript source for the album artwork preferences UI.
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

Fetchers = {

  /**
   * \brief doPaneLoad - We need to load up the fetchers and watch for the
   * pref dialog for when it closes due to cancel or apply.
   */
  doPaneLoad: function() {
    var self = this;

    function forceCheck(event) {
      const instantApply =
        Application.prefs.getValue("browser.preferences.instantApply", true);
      if (event.type == "dialogcancel" && !instantApply) {
        // cancel dialog, don't do anything
        return;
      }

      // We need to manually save all the fetcher data since it is complicated
      // preferences.
      self.applyFetcherPreferences();
    }

    window.addEventListener('dialogaccept', forceCheck, false);
    window.addEventListener('dialogcancel', forceCheck, false);

    // Set up the fetcher data from preferences
    this.resetFetcherPreferences();
  },

  /**
   * \brief moveFetcher - This is a helper function to move the fetcher up or
   * down in priority in the list.
   */
  moveFetcher: function(offset) {
    var fetcherBox = document.getElementById("fetcherListBox");
    var currentFetcher = fetcherBox.getSelectedItem(0);
    if (currentFetcher) {
      var fetcherIndex = fetcherBox.getIndexOfItem(currentFetcher);
      var fLabel = currentFetcher.getAttribute("label");
      var fValue = currentFetcher.value;
      var fEnabled = currentFetcher.checked;
      if ( (fetcherIndex > 0) || (offset > 0) ) {
        fetcherBox.removeItemAt(fetcherIndex);
        if (fetcherIndex <= fetcherBox.getRowCount()) {
          newFetcher = fetcherBox.insertItemAt((fetcherIndex + offset),
                                               fLabel,
                                               fValue);
        } else {
          newFetcher = fetcherBox.appendItem(fLabel, fValue);
        }
        newFetcher.setAttribute("type", "checkbox");
        newFetcher.setAttribute("checked", fEnabled);
        fetcherBox.selectItem(newFetcher);
      }
    }
  },

  /**
   * \brief getFetchers - This will get all of the fetchers that are available.
   * The sbIAlbumArtService.getFetcherList currently will only get either
   * REMOTE or LOCAL fetchers but not both.
   * Remove when the TYPE_ALL in the sbIAlbumArtService.getFetcherList works.
   * (bug 16751)
   */
  getFetchers: function() {
    var fetcherList = [];
    // Get the category manager.
    var categoryManager = Cc["@mozilla.org/categorymanager;1"]
                            .getService(Ci.nsICategoryManager);
    // Get the enumeration of album art fetchers.
    var enumerator = categoryManager.enumerateCategory("songbird-album-art-fetcher");
    while (enumerator.hasMoreElements()) {
      var catItem = enumerator.getNext();
      var categoryName = catItem.QueryInterface(Ci.nsISupportsCString).toString();
      var fetcherCid = categoryManager.getCategoryEntry("songbird-album-art-fetcher",
                                                        categoryName);

      var cFetcher = Cc[fetcherCid].createInstance(Ci.sbIAlbumArtFetcher);
      fetcherList.push( { fetcherCID: fetcherCid,
                          fetcherPriority: cFetcher.priority});
    }

    function fetcherPriorityCompare(a, b) {
      if (a.fetcherPriority == -1) {
        return 1;
      }
      if (b.fetcherPriority == -1) {
        return -1;
      }
      return (a.fetcherPriority - b.fetcherPriority);
    }
    fetcherList.sort(fetcherPriorityCompare);

    var outList = [];
    for (var i = 0; i < fetcherList.length; i++) {
      outList.push(fetcherList[i].fetcherCID);
    }
    return outList;
  },

  /**
   * \brief resetFetcherPreferences - This will setup the preferences for all
   * of the fetchers. It will clear the current settings first.
   */
  resetFetcherPreferences: function() {
    var fetcherBox = document.getElementById("fetcherListBox");

    // Remove any existing ones
    while(fetcherBox.getRowCount() > 0) {
      fetcherbox.removeItemAt(0);
    }
    /**
     * The sbIAlbumArtService.getFetcherList is broken :( it doesn't process the
     * TYPE_ALL flag even though it accepts it (As of 1.2.0).
     * (again, bug 16751)
     */
    var albumArtService = Cc["@songbirdnest.com/Songbird/album-art-service;1"]
                            .createInstance(Ci.sbIAlbumArtService);
    var fetchList = albumArtService.getFetcherList(
                                            Ci.sbIAlbumArtFetcherSet.TYPE_ALL);
    var fetchers;
    if (fetchList.length > 0) {
      fetchers = ArrayConverter.JSArray(fetchList);
    } else {
      fetchers = this.getFetchers();
    }

    for (var fIndex = 0; fIndex < fetchers.length; fIndex++) {
      // Add a list item entry for each one
      var fetcherCid = fetchers[fIndex];
      var cFetcher = Cc[fetcherCid].createInstance(Ci.sbIAlbumArtFetcher);

      // File and Metadata look stupid right now :P
      // TODO: file bug for localization of these two and remove once done.
      var fLabel;
      var bundlePreferences =
         document.getElementById("bundleSongbirdPreferences");

      if (cFetcher.name.match(/^file$/)) {
        fLabel = bundlePreferences.getString("albumartprefs.fetcher.local");
      } else if (cFetcher.name.match(/^metadata$/)) {
        fLabel = bundlePreferences.getString("albumartprefs.fetcher.metadata");
      } else {
        fLabel = cFetcher.name + " (" + cFetcher.description + ")";
      }
      var newFetcher = fetcherBox.appendItem(fLabel, fetcherCid);
      newFetcher.setAttribute("type", "checkbox");
      newFetcher.setAttribute("checked", cFetcher.isEnabled);
    }
  },

  /**
   * \brief applyFetcherPreferences - This will apply all the preferences for
   * the fetchers (priorities, enabled, etc).
   */
  applyFetcherPreferences: function() {
    var fetcherBox = document.getElementById("fetcherListBox");

    var fetcherCount = fetcherBox.getRowCount();
    var fPref = "songbird.albumart.";

    for (var fIndex = 0; fIndex < fetcherCount; fIndex++) {
      var fetcherItem = fetcherBox.getItemAtIndex(fIndex);
      if (fetcherItem) {
        var fetcherCid = fetcherItem.value;
        var cFetcher = Cc[fetcherCid].createInstance(Ci.sbIAlbumArtFetcher);
        cFetcher.priority = fIndex;
        cFetcher.isEnabled = fetcherItem.checked;
      }
    }

    return true;
  }
};
