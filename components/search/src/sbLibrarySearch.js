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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/DebugUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const LOG = DebugUtils.generateLogFunction("sbLibrarySearch", 2);

function sbLibrarySearch()
{
}

sbLibrarySearch.prototype = {
  classDescription: "Songbird Internal Library Search Service",
  classID:          Components.ID("{13d82f60-a625-11df-981c-0800200c9a66}"),
  contractID:       "@songbirdnest.com/Songbird/songbird-internal-search;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbISearchEngine]),

  /**
   * Return true if the active tab is displaying a songbird media page.
   * Note: The media page may not be initialized.
   */
  _isMediaPageShowing: function sbLibrarySearch__isMediaPageShowing(aBrowser) {
    return aBrowser.currentMediaPage != null;
  },

  /**
   * Get the media list view that is currently showing in the media page
   * in the browser
   */
  _getCurrentMediaListView:
    function sbLibrarySearch__getCurrentMediaListView(aBrowser) {
    if (aBrowser.currentMediaPage && aBrowser.currentMediaPage.mediaListView) {
      return aBrowser.currentMediaPage.mediaListView;
    } else {
      return null;
    }
  },

  /**
   * Attempt to set a search filter on the media page in the current tab.
   * Returns true on success.
   */
  _setMediaPageSearch:
    function sbLibrarySearch__setMediaPageSearch(aBrowser, aQuery) {
    // Get the currently displayed sbIMediaListView
    var mediaListView = this._getCurrentMediaListView(aBrowser);

    // we need an sbIMediaListView with a cascadeFilterSet
    if (!mediaListView || !mediaListView.cascadeFilterSet) {
      LOG("no cascade filter set!");
      return;
    }

    // Attempt to set the search filter on the media list view
    var filters = mediaListView.cascadeFilterSet;

    var searchIndex = -1;
    for (let i = 0; i < filters.length; ++i) {
      if (filters.isSearch(i)) {
        searchIndex = i;
        break;
      }
    }
    if (searchIndex < 0) {
      searchIndex = filters.appendSearch(["*"], 1);
    }

    if (aQuery == "" || aQuery == null) {
      filters.set(searchIndex, [], 0);
    } else {
      var stringTransform =
        Cc["@songbirdnest.com/Songbird/Intl/StringTransform;1"]
          .createInstance(Ci.sbIStringTransform);

      aQuery = stringTransform.normalizeString("",
                      Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE,
                      aQuery);

      var valArray = aQuery.split(" ");

      filters.set(searchIndex, valArray, valArray.length);
    }
  },

  doSearch: function sbLibrarySearch_doSearch(aWindow, aQuery) {
    var browser = aWindow.gBrowser;

    // If there is no gBrowser, then there is nothing for us to do.
    if (!browser)
      return;

    // trim the leading and trailing whitespace from the search string
    var query = aQuery.trim();

    if (!this._isMediaPageShowing(browser)) {
      // create a view into the main library with the requested search
      var library = LibraryUtils.mainLibrary;
      var view = LibraryUtils.createStandardMediaListView(library, query);

      // load that view
      browser.loadMediaList(library, null, null, view);
    } else {
    // If we are showing a media page, then just set the query directly
      this._setMediaPageSearch(browser, query);
    }
  }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLibrarySearch]);
}
