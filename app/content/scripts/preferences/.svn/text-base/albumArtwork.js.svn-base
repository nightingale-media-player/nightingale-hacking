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

var Fetchers = {

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
   * \brief resetFetcherPreferences - This will setup the preferences for all
   * of the fetchers. It will clear the current settings first.
   */
  resetFetcherPreferences: function() {
    var fetcherBox = document.getElementById("fetcherListBox");

    // Remove any existing ones
    while(fetcherBox.getRowCount() > 0) {
      fetcherBox.removeItemAt(0);
    }

    var albumArtService = Cc["@songbirdnest.com/Songbird/album-art-service;1"]
                            .getService(Ci.sbIAlbumArtService);
    var fetchList = albumArtService.getFetcherList(
                                            Ci.sbIAlbumArtFetcherSet.TYPE_ALL,
                                            true);

    for (var fIndex = 0; fIndex < fetchList.length; fIndex++) {
      // Add a list item entry for each one
      try {
        var fetcherCid = fetchList.queryElementAt(fIndex, Ci.nsIVariant);
        var cFetcher = Cc[fetcherCid].createInstance(Ci.sbIAlbumArtFetcher);
        var fLabel = cFetcher.name + " (" + cFetcher.description + ")";
        var newFetcher = fetcherBox.appendItem(fLabel, fetcherCid);
        newFetcher.setAttribute("type", "checkbox");
        newFetcher.setAttribute("checked", cFetcher.isEnabled);
      } catch (err) {
        Cu.reportError("Unable to find fetcher: " + err);
      }
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
        try {
          var fetcherCid = fetcherItem.value;
          var cFetcher = Cc[fetcherCid].createInstance(Ci.sbIAlbumArtFetcher);
          cFetcher.priority = fIndex;
          cFetcher.isEnabled = fetcherItem.checked;
        } catch (err) {
          Cu.reportError("Unable to update fetcher: " + err);
        }
      }
    }

    return true;
  }
};
