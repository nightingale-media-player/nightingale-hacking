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
 * \file searchHandler.js
 * \brief Search Handler object.
 * \internal
 */

Components.utils.import("resource://app/jsmodules/DebugUtils.jsm");

const LOG = DebugUtils.generateLogFunction("searchHandler", 2);

// Alias identifying the Songbird search engine
const SEARCHENGINE_ALIAS_SONGBIRD = "songbird-internal-search";

/**
 * \brief Songbird Search Handler.
 *
 * Songbird Search Handler
 * Responsible for:
 *   - Switching between standard web search mode and Songbird's internal
 *     "Live Search" mode based on the state of the browser
 *   - Detecting search capabilities embedded in web pages
 *   - Launching the "get more search plugins" page
 *   - Responding to search events
 *
 * This object is based on the Firefox BrowserSearch object.
 * See http://lxr.mozilla.org/seamonkey/source/browser/base/content/browser.js
 * \internal
 */
const gSearchHandler = {


  /**
   * Register search handler listeners
   */
  init: function SearchHandler_init() {

    // If there is no gBrowser, then there is nothing
    // for us to do.
    if (typeof gBrowser == 'undefined') {
      return;
    }

    // Listen for browser links in order to detect embedded search engines
    gBrowser.addEventListener("DOMLinkAdded",
                              function (event) { gSearchHandler.onLinkAdded(event); },
                              false);

    // Listen for tab change events
    gBrowser.addEventListener('TabContentChange',
                              function (event) { gSearchHandler.onTabChanged(event); },
                              false);
    gBrowser.addEventListener('TabPropertyChange',
                              function (event) { gSearchHandler.onTabChanged(event); },
                              false);

    // Listen for search events
    document.addEventListener("search",
                              function (event) { gSearchHandler.onSearchEvent(event); },
                              true);
                              
    this.registerSearchEngineHandler(SEARCHENGINE_ALIAS_SONGBIRD,true);
  },


  /**
   * Uninitialize
   */
  uninit: function SearchHandler_uninit() {
    // Hmm, nothing to do?
    //this.removeSearchEngineHandler("songbird-internal-search");
  },


  /////////////////////////////////////////////////////////////////////////////
  // Private Variables ////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////


  // Used to save the state of the web search box
  // when switching to the Songbird search engine
  _previousSearchEngine: null,
  _previousSearch: "",



  /////////////////////////////////////////////////////////////////////////////
  // Event Listeners  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /**
   * A new <link> tag has been discovered - check to see if it advertises
   * an OpenSearch engine.
   */
  onLinkAdded: function SearchHandler_onLinkAdded(event) {
    // XXX this event listener can/should probably be combined with the onLinkAdded
    // listener in tabBrowser.xml.  See comments in FeedHandler.onLinkAdded().
    const target = event.target;
    var etype = target.type;
    const searchRelRegex = /(^|\s)search($|\s)/i;
    const searchHrefRegex = /^(https?|ftp):\/\//i;

    if (!etype)
      return;

    // Mozilla Bug 349431: If the engine has no suggested title, ignore it rather
    // than trying to find an alternative.
    if (!target.title)
      return;

    if (etype == "application/opensearchdescription+xml" &&
        searchRelRegex.test(target.rel) && searchHrefRegex.test(target.href))
    {
      const targetDoc = target.ownerDocument;
      // Set the attribute of the (first) search-engine button.
      var searchButton = document.getAnonymousElementByAttribute(this.getSearchBar(),
                                  "anonid", "searchbar-engine-button");
      if (searchButton) {
        var browser = gBrowser.getBrowserForDocument(targetDoc);
         // Append the URI and an appropriate title to the browser data.
        var iconURL = null;
        if (gBrowser.shouldLoadFavIcon(browser.currentURI)) {
          var faviconService = Cc["@mozilla.org/browser/favicon-service;1"]
                                 .getService(Ci.nsIFaviconService);
          try {
            iconURL = faviconService.getFaviconForPage(
                        browser.contentDocument.documentURIObject).spec;

            // Favicon URI's are prepended with "moz-anno:favicon:".
            iconURL = iconURL.replace(/^moz-anno:favicon:/, '');
          }
          catch(e) {
            if (Components.lastResult != Cr.NS_ERROR_NOT_AVAILABLE)
              Cu.reportError(e);

            //Default to favicon.ico if no favicon is available.
            iconURL = browser.currentURI.prePath + "/favicon.ico";
          }

        }

        var hidden = false;
        // If this engine (identified by title) is already in the list, add it
        // to the list of hidden engines rather than to the main list.
        // XXX This will need to be changed when engines are identified by URL;
        // see bug 335102.
         var searchService = Cc["@mozilla.org/browser/search-service;1"]
                               .getService(Ci.nsIBrowserSearchService);
        if (searchService.getEngineByName(target.title))
          hidden = true;

        var engines = [];
        if (hidden) {
          if (browser.hiddenEngines)
            engines = browser.hiddenEngines;
        }
        else {
          if (browser.engines)
            engines = browser.engines;
        }

        engines.push({ uri: target.href,
                       title: target.title,
                       icon: iconURL });

        if (hidden) {
          browser.hiddenEngines = engines;
        }
        else {
          browser.engines = engines;
          if (browser == gBrowser || browser == gBrowser.mCurrentBrowser)
            this.updateSearchButton();
        }
      }
    }
  },



  /**
   * Called when a "search" event is received.
   * Search events are expected to come from elements
   * with .value and .currentEngine properties.
   */
  onSearchEvent: function SearchHandler_onSearchEvent( evt )
  {
    var searchBar = this.getSearchBar();
    if (searchBar == null) {
      throw("gSearchHandler: Could not process search event. " +
            "Target did not have a currentEngine property.");
    }

    var currentEngine = searchBar.currentEngine;
    // If this engine is an internal one, do the search internally.
    if (this._internalEngines[currentEngine.alias])
    {
      // Empty search text means to disable the search filter. Still necessary
      // to dispatch search.

      // Special case for our internal search. Other people can add their
      // own listeners as well.
      var contractID =
        "@songbirdnest.com/Songbird/" + currentEngine.alias + ";1";
      if (contractID in Cc) {
        var searchEngine = Cc[contractID].getService(Ci.sbISearchEngine);
        searchEngine.doSearch(window, searchBar.value);

        return;
      }
    }

    // No need to dispatch web search if the search text is empty.
    if (searchBar.value == "")
        return;

    // null parameter below specifies HTML response for search
    var submission = currentEngine.getSubmission(searchBar.value, null);

    // TODO: Some logic to determine where this opens?
    var where = "current";

    openUILinkIn(submission.uri.spec, where, null, submission.postData);
  },


  onTabChanged: function SearchHandler_onTabChanged(event) {
    // Update search button to reflect available engines.
    // Note: In firefox this is called by browser.js asyncUpdateUI()
    BrowserSearch.updateSearchButton();

    // Update mode depending on location
    // (Library vs Website)
    BrowserSearch.updateSearchMode();
  },




  /////////////////////////////////////////////////////////////////////////////
  // FireFox Search Engine Detection //////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////



  /**
   * Update the browser UI to show whether or not additional engines are
   * available when a page is loaded or the user switches tabs to a page that
   * has search engines.
   */
  updateSearchButton: function SearchHandler_updateSearchButton() {
    var searchButton = document.getAnonymousElementByAttribute(this.getSearchBar(),
                                "anonid", "searchbar-engine-button");
    if (!searchButton)
      return;
    var engines = gBrowser.mCurrentBrowser.engines;
    if (!engines || engines.length == 0) {
      if (searchButton.hasAttribute("addengines"))
        searchButton.removeAttribute("addengines");
    }
    else {
      searchButton.setAttribute("addengines", "true");
    }
  },

  /**
   * Gives focus to the search bar, if it is present on the toolbar, or loads
   * the default engine's search form otherwise. For Mac, opens a new window
   * or focuses an existing window, if necessary.
   */
  webSearch: function SearchHandler_webSearch() {
    var searchBar = this.getSearchBar();
    if (searchBar) {
      searchBar.select();
      searchBar.focus();
    } else {
      var ss = Cc["@mozilla.org/browser/search-service;1"].
               getService(Ci.nsIBrowserSearchService);
      var searchForm = ss.defaultEngine.searchForm;
      loadURI(searchForm, null, null, false);
    }
  },

  /**
   * Loads a search results page, given a set of search terms. Uses the current
   * engine if the search bar is visible, or the default engine otherwise.
   *
   * @param searchText
   *        The search terms to use for the search.
   *
   * @param useNewTab
   *        Boolean indicating whether or not the search should load in a new
   *        tab.
   */
  loadSearch: function SearchHandler_loadSearch(searchText, useNewTab) {
    var ss = Cc["@mozilla.org/browser/search-service;1"].
             getService(Ci.nsIBrowserSearchService);
    var engine;

    // If the search bar is visible, use the current engine, otherwise, fall
    // back to the default engine.
    if (this.getSearchBar())
      engine = ss.currentEngine;
    else
      engine = ss.defaultEngine;

    var submission = engine.getSubmission(searchText, null); // HTML response

    // getSubmission can return null if the engine doesn't have a URL
    // with a text/html response type.  This is unlikely (since
    // SearchService._addEngineToStore() should fail for such an engine),
    // but let's be on the safe side.
    if (!submission)
      return;

    if (useNewTab) {
      window.gBrowser.loadOneTab(submission.uri.spec, null, null,
                              submission.postData, null, false);
    } else
      loadURI(submission.uri.spec, null, submission.postData, false);
  },

  /**
   * Returns the search bar element if it is present in the toolbar and not
   * hidden, null otherwise.
   */
  getSearchBar: function SearchHandler_getSearchBar() {
    // Look for a searchbar element
    var elements = document.getElementsByTagName("searchbar");
    if (elements && elements.length > 0) {
       return elements[0];
    }
    return null;
  },

  /**
   * Returns all the Songbird search engines
   */
  getSongbirdSearchEngines: function SearchHandler_getSongbirdSearchEngines() {
    var songbirdEngines = this.getSearchBar().searchService.getEngines({});
    if (!songbirdEngines) {
      LOG("\n\nCould not find any search engines for Songbird.\n");
    }
    return songbirdEngines;
  },

  /**
   * Returns the Songbird internal search engine
   */
  getSongbirdSearchEngine:
    function SearchHandler_getSongbirdSearchEngine(aAlias) {
    if (!aAlias)
      aAlias = SEARCHENGINE_ALIAS_SONGBIRD;
    var songbirdEngine = this.getSearchBar()
                             .searchService
                             .getEngineByAlias(aAlias);
    if (!songbirdEngine) {
      LOG("\n\nThe Songbird search engine with alias \"" + aAlias +
          "\" could not be found.\n");
    }
    return songbirdEngine;
  },

  loadAddEngines: function SearchHandler_loadAddEngines() {
    // Hardcode Songbird to load the page in a tab
    var where = "tab";
    var regionBundle = document.getElementById("bundle_browser_region");

    var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
    var searchEnginesURL = formatter.formatURLPref("browser.search.searchEnginesURL");

    openUILinkIn(searchEnginesURL, where);
  },


  /////////////////////////////////////////////////////////////////////////////
  // Songbird Search Mode Support /////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////


  /**
   * Update the state of the search box based on the current
   * browser location
   */
  updateSearchMode: function SearchHandler_updateSearchMode() {

    // Do nothing until the browser is finished loading
    // This avoids annoying flickering.
    if (gBrowser.loading) {
      return;
    }

    // If a media page is open in the current tab,
    // then we will need to restore the search filter state
    if (this._isMediaTabOrMediaPageShowing())
    {
      this._switchToInternalSearch();
    }
    // Must be showing a regular page.
    // May need to deactivate the songbird search.
    else
    {
      this._switchToWebSearch();
    }
  },


  /**
   * Set up the search box to act as a filter for the current media page
   * Note that some final setup cannot be completed until the media page
   * fully loads.
   */
  _switchToInternalSearch: function SearchHandler__switchToInternalSearch() {
    var searchBar = this.getSearchBar();
    var currentEngine = searchBar.currentEngine;

    // Save the previous web search engine, used when switch to web search
    if (!this._internalEngines[currentEngine.alias])
    {
      this._previousSearchEngine = currentEngine;
      this._previousSearch = searchBar.value;
    }

    // Get the corresponding search engine for the service pane node.
    var alias = null;
    if (gServicePane) {
      // Get the current active node.
      var node = gServicePane.activeNode;
      if (node) {
        alias = "songbird-" + node.searchtype + "-search";
      }
    }
    var engine = this.getSongbirdSearchEngine(alias);

    var liveSearchEnabled = false;
    // Live search is disabled for search engines who don't support livesearch
    if (this._internalEngines[alias].liveSearch) {
      liveSearchEnabled =
        Application.prefs.getValue("songbird.livesearch.enabled", true);
    }

    // Set live search mode for the songbird search engine
    searchBar.setLiveSearchMode(engine, liveSearchEnabled);

    // Make sure the songbird search is selected
    if (currentEngine != engine) {
      // Switch to the songbird internal search engine...
      // but first remove any query text so as not to cause
      // the engine to immediately submit the query
      searchBar.value = "";
      searchBar.currentEngine = engine;
    }

    // Set the query to match the state of the media page
    this._syncSearchBarToMediaPage(alias);

    searchBar.updateDisplay();
  },


  /**
   * Restore web search mode
   */
  _switchToWebSearch: function SearchHandler__switchToWebSearch() {
    var searchBar = this.getSearchBar()
    var currentEngine = searchBar.currentEngine;

    // If the songbird engine is in live search mode then
    // turn that feature off and restore the default
    // display text.
    if (searchBar.isInLiveSearchMode(currentEngine)) {
      searchBar.setLiveSearchMode(currentEngine, false);
      searchBar.setEngineDisplayText(currentEngine, null);
    }

    // If this engine has not been registered as internal,
    // we need to restore the engine active prior to us.
    if (this._internalEngines[currentEngine.alias])
    {
      // If there is a previous search engine, switch to it...
      // but first remove any query text so as not to cause
      // the engine to immediately submit the query
      if (this._previousSearchEngine)
      {
        searchBar.value = "";
        searchBar.currentEngine = this._previousSearchEngine;

        // Restore the old query
        searchBar.value = this._previousSearch;
      }
    }

    // Sync the search bar contents
    BrowserSearch._syncSearchBarToMediaPage();

    searchBar.updateDisplay();
  },


  /**
   * Return true if the active tab is the media tab or the active tab is
   * displaying a songbird media page.
   * Note: The media page may not be initialized.
   */
  _isMediaTabOrMediaPageShowing:
  function SearchHandler__isMediaTabOrMediaPageShowing() {
    return (gBrowser.mediaTab == gBrowser.selectedTab ||
            gBrowser.currentMediaPage != null);
  },


  /**
   * Get the real target of this event
   * (hack through binding and browser layers)
   */
  _getSearchEventTarget: function SearchHandler__getSearchEventTarget(evt)  {
    var target;

    // If normal search event
    if (evt.target && evt.target.currentEngine) {
      target = evt.target;
    // If search target is within a binding
    } else if (evt.originalTarget && evt.originalTarget.currentEngine) {
      target = evt.originalTarget;
    // If search target is within a browser document
    } else if (evt.target && evt.target.wrappedJSObject &&
              evt.target.wrappedJSObject.currentEngine)
    {
      target = evt.target.wrappedJSObject;
    // If search target is within a browser document AND a binding
    } else if (evt.originalTarget && evt.originalTarget.wrappedJSObject &&
              evt.originalTarget.wrappedJSObject.currentEngine)
    {
      target = evt.originalTarget.wrappedJSObject;
    // Else I'm out of ideas...
    } else {
      dump("\ngSearchHandler: Error! Search event received from" +
            " a target with no currentEngine!\n");
      try { dump("\ttarget " + evt.target.tagName + "\n"); } catch (e) {};
      try { dump("\toriginalTarget " + evt.originalTarget.tagName + "\n"); } catch (e) {};
    }
    return target;
  },


  /**
   * Attempt to ask the current media page what search it is currently using.
   * Returns "" if the media page is unavailable.
   */
  _getMediaPageSearch: function SearchHandler__getMediaPageSearch() {
    // Get the currently displayed sbIMediaListView
    var mediaListView = this._getCurrentMediaListView();

    // XXXsteve We need a better way to discover the actual search terms
    // rather than reverse engineer it from the search constaint
    if (mediaListView && mediaListView.searchConstraint) {
      var search = mediaListView.searchConstraint;
      var terms = [];
      var groupCount = search.groupCount;
      for (var i = 0; i < groupCount; i++) {
        var group = search.getGroup(i);
        var property = group.properties.getNext();
        terms.push(group.getValues(property).getNext());
      }
      return terms.join(" ");
    }
    return "";
  },


  /**
   * Attempt to set a search filter on the media page in the current tab.
   * Returns true on success.
   */
  _setMediaPageSearch: function SearchHandler__setMediaPageSearch(query) {
    // Get the currently displayed sbIMediaListView
    var mediaListView = this._getCurrentMediaListView();

    /* we need an sbIMediaListView with a cascadeFilterSet */
    if (!mediaListView || !mediaListView.cascadeFilterSet) {
      Cu.reportError("Error: no cascade filter set!");
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

    if (query == "" || query == null) {
      filters.set(searchIndex, [], 0);
    } else {

      var stringTransform =
        Cc["@songbirdnest.com/Songbird/Intl/StringTransform;1"]
          .createInstance(Ci.sbIStringTransform);

      query = stringTransform.normalizeString("",
                     Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE,
                     query);

      var valArray = query.split(" ");

      filters.set(searchIndex, valArray, valArray.length);
    }
  },


  /**
   * Try to get a human readable name for the media page
   * in the current tab
   */
  _getMediaPageDisplayName: function SearchHandler__getMediaPageDisplayName() {

    // Get the currently displayed sbIMediaListView
    var mediaListView = this._getCurrentMediaListView();

    // Return the mediaListView's mediaList's name
    return mediaListView?mediaListView.mediaList.name:"";
  },


  /**
   * Get the media list view that is currently showing in the media page
   * in the browser
   */
  _getCurrentMediaListView: function SearchHandler__getCurrentMediaListView() {
    if (gBrowser.currentMediaPage && gBrowser.currentMediaPage.mediaListView) {
      return gBrowser.currentMediaPage.mediaListView;
    } else {
      return null;
    }
  },


  /**
   * Update the Songbird search to reflect
   * the state of the current media list view
   */
  _syncSearchBarToMediaPage:
    function SearchHandler__syncSearchBarToMediaPage(aAlias) {
    // if we are not currently showing a view (iem we're showing a web site)
    // then do not change anything, we want the content of the search bar to
    // persist
    var mediaListView = this._getCurrentMediaListView();
    if (!mediaListView) return;

    // Get the search box element
    var searchBar = this.getSearchBar();
    if (searchBar == null) {
      return;
    }

    // Change the text displayed on empty query to reflect
    // the current search context.
    var mediaPageName = this._getMediaPageDisplayName();
    if (mediaPageName != "") {
      var engine = this.getSongbirdSearchEngine(aAlias);
      searchBar.setEngineDisplayText(engine, mediaPageName);
    }

    // Find out what search is filtering this medialist
    var queryString = this._getMediaPageSearch();
    searchBar.value = queryString;
  },
  
  registerSearchEngineHandler :
    function SearchHandler_registerSearchEngineHandler(searchEngineAlias, liveSearch) {
        var engine = this.getSongbirdSearchEngine(searchEngineAlias);
        
        // Only continue if the engine isn't yet registered and exists
        if(!this._internalEngines[searchEngineAlias]&&engine) {
            // default liveSearch to false
            if(liveSearch === undefined)
                liveSearch = false;
                
            var hidden = engine.hidden;
            // If the engine is hidden, make it visible
            if(hidden)
                engine.hidden = false;
                
            this._internalEngines[searchEngineAlias] = {'liveSearch':liveSearch,'wasHidden':hidden};
            return true;
        }
        LOG("\n\nCouldn't register a search engine handler for \"" + searchEngineAlias +
            "\".There either is already a handler for the search engine with this alias or there is no engine with this alias registered.\n");
        return false;
    },
    
  removeSearchEngineHandler :
    function SearchHandler_removeSearchEngineHandler(searchEngineAlias) {
        var engine = this.getSongbirdSearchEngine(searchEngineAlias);
        if(this._internalEngines[searchEngineAlias]) {
            // reset the hidden property to the default
            if(this._internalEngines[searchEngineAlias].wasHidden)
                engine.hidden = false;
                
            delete this._internalEngines[searchEngineAlias];
        }
        LOG("\n\nThere is no internal search engine for the alias \""
            + searchEngineAlias + "\" registered.\n");
    },
    
    _internalEngines: {}
}  // End of gSearchHandler


// Also expose the search handler as "BrowserSearch" for
// compatibility with FireFox
const BrowserSearch = gSearchHandler;


// Initialize the search handler on load
window.addEventListener("load",
  function() {
    gSearchHandler.init();
  },
  false);

// Shutdown the search handler on unload
window.addEventListener("unload",
  function() {
    gSearchHandler.uninit();
  },
  false);
  
// global search engine rerouting
const registerSearchEngineHandler = gSearchHandler.registerSearchEngineHandler(searchEngineAlias,liveSearch);
const removeSearchEngineHandler = gSearchHandler.removeSearchEngineHandler(searchEngineAlias);
