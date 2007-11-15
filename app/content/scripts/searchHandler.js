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

/**
 * \file searchHandler.js
 * \brief Search Handler object.
 * \internal
 */
 
// Searches using engines tagged with "songbird:internal" are not
// sent to the browser
const SEARCHENGINE_TAG_INTERNAL = "songbird:internal";

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
   
    // Listen for browser links in order to detect embedded search engines
    gBrowser.addEventListener("DOMLinkAdded", 
                              function (event) { gSearchHandler.onLinkAdded(event); }, 
                              false);
    
    // Listen for page loads in order to update the search box content
    gBrowser.addEventListener("load", 
                              function (event) { gSearchHandler.onBrowserLoad(event); }, 
                              false);                              
                                                        
    // Listen for location and state changes in order to update the
    // selected search engine and list of available engines
    var nsIWebProgress = Components.interfaces.nsIWebProgress;
    gBrowser.addProgressListener(this.webProgressListener,
            nsIWebProgress.NOTIFY_LOCATION | nsIWebProgress.NOTIFY_STATE_ALL);

    // Listen for search events
    document.addEventListener("search", 
                              function (event) { gSearchHandler.onSearchEvent(event); }, 
                              true);        

    // Show the songbird search engine
    // (All songbird: tagged engines are hidden on startup)
    var songbirdSearch = this.getSongbirdSearchEngine();
    songbirdSearch.hidden = false;
  },
  

  /**
   * Uninitialize
   */
  uninit: function SearchHandler_uninit() {
    // Hmm, nothing to do?   
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
        if (gBrowser.shouldLoadFavIcon(browser.currentURI))
          iconURL = browser.currentURI.prePath + "/favicon.ico";

        var hidden = false;
        // If this engine (identified by title) is already in the list, add it
        // to the list of hidden engines rather than to the main list.
        // XXX This will need to be changed when engines are identified by URL;
        // see bug 335102.
         var searchService =
            Components.classes["@mozilla.org/browser/search-service;1"]
                      .getService(Components.interfaces.nsIBrowserSearchService);
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
   * Called whenever something loads in gBrowser.
   * Used to detect when a playlist has fully loaded.
   */
  onBrowserLoad: function SearchHandler_onBrowserLoad(evt) {
    // If a media list is open in the current tab,
    // then we may need to update the contents of the search box
    var playlist = this._getCurrentPlaylist();
    if (playlist && playlist.mediaListView) 
    {
      this._syncSearchBarToPlaylist();
    }
  },


  /**
   * Called when a "search" event is received. 
   * Search events are expected to come from elements
   * with .value and .currentEngine properties.
   */  
  onSearchEvent: function SearchHandler_onSearchEvent( evt )
  {
    // Find the search widget responsible for this event
    var widget = this._getSearchEventTarget(evt);
    if (widget == null) {
      throw("gSearchHandler: Could not process search event. " +
            "Target did not have a currentEngine property.");
    }

    // If this engine has not been tagged as internal
    // then dispatch the search normally
    if ( widget.currentEngine.tags.indexOf(SEARCHENGINE_TAG_INTERNAL) == -1 ) 
    { 
      // null parameter below specifies HTML response for search
      var submission = widget.currentEngine.getSubmission(widget.value, null);

      // TODO: Some logic to determine where this opens? 
      var where = "current";
      
      openUILinkIn(submission.uri.spec, where, null, submission.postData);             
    }
    // Must be an internal search. 
    else 
    {
      // Is this our internal search?  If not, do nothing.  
      // Other people can add their own listeners.
      if ( widget.currentEngine.alias == SEARCHENGINE_ALIAS_SONGBIRD )
      {
        this._doSongbirdSearch(widget.value);
      } 
    }
  },


  /**
   * nsIWebProgressListener to monitor browser changes 
   * and notify gSearchHandler
   */  
  webProgressListener:
  {
  
    onLocationChange: function SearchHandler_onLocationChange(aWebProgress, aRequest, aLocation) 
    {
    
      // Update search button to reflect available engines.
      // Note: In firefox this is called by browser.js asyncUpdateUI()
      BrowserSearch.updateSearchButton();
      
      // Update mode depending on location
      // (Library vs Website)
      BrowserSearch.updateSearchMode();
    },

    onStateChange: function SearchHandler_onStateChange(aWebProgress, aRequest, aStateFlags, aStatus){     
      // Figure out if this is a new network request
      var networkStartingFlags = Ci.nsIWebProgressListener.STATE_START 
                                 | Ci.nsIWebProgressListener.STATE_IS_NETWORK;        
      var isStarting = (networkStartingFlags & aStateFlags) == networkStartingFlags;
     
      // When loading new page, reset search engine list 
      if (isStarting && aRequest && aWebProgress.DOMWindow == content) {
        BrowserSearch.resetFoundEngineList();        
      }
    },
    
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress,
                               aMaxSelfProgress, aCurTotalProgress,
                               aMaxTotalProgress) {},
    onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage) {},
    onSecurityChange: function(aWebProgress, aRequest, state) {},

    QueryInterface: function SearchHandler_QueryInterface(aIID)
    {
      if (aIID.equals(Components.interfaces.nsIWebProgressListener)
          || aIID.equals(Components.interfaces.nsISupports)
          || aIID.equals(Components.interfaces.nsISupportsWeakReference))
      {
          return this;
      }
      throw Components.results.NS_NOINTERFACE;
    }
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
    if (elements && elements.length > 0 && isElementVisible(elements[0])) {
       return elements[0];
    } 
    return null;
  },
  
  /**
   * Returns the Songbird internal search engine
   */
  getSongbirdSearchEngine: function SearchHandler_getSongbirdSearchEngine() {
    var songbirdEngine = this.getSearchBar()
                             .searchService
                             .getEngineByAlias(SEARCHENGINE_ALIAS_SONGBIRD);
    if (!songbirdEngine) {
      dump("\n\nError: The Songbird internal search engine could not be found. \n");
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


  /**
   * Get rid of the engine lists on the current browser.
   * Note: In Firefox this is done by nsBrowserStatusHandler.onStateChange
   */
  resetFoundEngineList: function SearchHandler_resetFoundEngineList() {
    
    // Clear out engine list
    gBrowser.mCurrentBrowser.engines = null;  
    
    // Why don't we have to clear hiddenEngines? 
  },
  



  /////////////////////////////////////////////////////////////////////////////
  // Songbird Search Mode Support /////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  
  /**
   * Update the state of the search box based on the current
   * browser location
   */
  updateSearchMode: function SearchHandler_updateSearchMode() {
 
    // If a media list is open in the current tab,
    // then we will need to restore the search filter state
    if (this._isPlaylistShowing()) 
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
   * Set up the search box to act as a filter for the current media list
   * Note that some final setup cannot be completed until the playlist
   * fully loads.  See also this.onBrowserLoad.
   */
  _switchToInternalSearch: function SearchHandler__switchToInternalSearch() {
    var searchBar = this.getSearchBar();
    var songbirdEngine = this.getSongbirdSearchEngine();
     
    // Unless we are already in songbird live search mode,
    // remember the current search settings so we can 
    // restore them when the user returns to a standard 
    // web page
    if (!searchBar.isInLiveSearchMode(songbirdEngine)
        && searchBar.currentEngine != songbirdEngine) 
    {
      this._previousSearchEngine = searchBar.currentEngine;
      this._previousSearch =  searchBar.value;
    }
    
    // Activate live search mode for the songbird search engine
    searchBar.setLiveSearchMode(songbirdEngine, true);
    
    // Make sure the songbird search is selected
    if (searchBar.currentEngine != songbirdEngine) { 
      // Switch to the songbird search engine...
      // but first remove any query text so as not to cause 
      // the engine to immediately submit the query
      searchBar.value = "";
      searchBar.currentEngine = songbirdEngine;
    }

    // Set the query to match the state of the media list
    this._syncSearchBarToPlaylist();
  },
  
  
  /**
   * Restore web search mode
   */
  _switchToWebSearch: function SearchHandler__switchToWebSearch() {
    var searchBar = this.getSearchBar()
    var songbirdEngine = this.getSongbirdSearchEngine();
     
    // If the songbird engine is in live search mode then 
    // turn that feature off and restore the default
    // display text.
    if (searchBar.isInLiveSearchMode(songbirdEngine)) 
    {
      searchBar.setLiveSearchMode(songbirdEngine, false);
      searchBar.setEngineDisplayText(songbirdEngine, null);
      
      // If songbird is also the active search engine, then 
      // we need to restore the engine active prior to us
      // entering songbird live search mode
      if (searchBar.currentEngine == songbirdEngine)
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
          
          this._previousSearchEngine = null;
          this._previousSearch = "";
        }
        // If there is no previous engine, it would have been due to the user
        // manually choosing the internal engine.  Don't switch.
      }
    }
  },  
    
  
  /**
   * Return true if the active tab is displaying a songbird media list.
   * Note: The playlist may not be initialized.
   */
  _isPlaylistShowing: function SearchHandler__isPlaylistShowing() {
    if (!gBrowser.mCurrentBrowser.currentURI) {
      // about:blank pages have no playlist (about:blank causes currentURI = null)
      return false;
    }
    
    var url = gBrowser.mCurrentBrowser.currentURI.spec;
    return url.indexOf("sbLibraryPage") >= 0;
  },  
   
   
  /**
   * If there is a media list in the current tab, set the current search
   * to the given query.  If not, then open the default library
   * with the given query.
   */
  _doSongbirdSearch: function SearchHandler__doSongbirdSearch(query) {    
    // If we aren't showing a playlist, then load the library
    if ( !this._isPlaylistShowing() )
    {
      // Load the main library
      var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                     .getService(Components.interfaces.sbILibraryManager);
      // TODO make a const or helper function?
      var url = "chrome://songbird/content/xul/sbLibraryPage.xul?library," +
                libraryManager.mainLibrary.guid + 
                "&search=" + 
                escape(query);
      gBrowser.loadURI(url);
    }
    // If we are showing a media list, then just set the query directly 
    else 
    {
      this._setPlaylistSearch(query);
    }
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
   * Attempt to ask the current playlist what
   * search it is currently using.
   * Returns "" if the playlist is unavailable.
   */
  _getPlaylistSearch: function SearchHandler__getPlaylistSearch() {

    // Get the playlist element from within the current tab      
    var playlist = this._getCurrentPlaylist();
    if (playlist == null) {
      return "";
    }
    
    return playlist.searchFilter;
  },


  /**
   * Attempt to set a search filter on the playlist 
   * in the current tab.
   * Returns true on success.
   */
  _setPlaylistSearch: function SearchHandler__setPlaylistSearch(query) {

    // Get the playlist element from within the current tab      
    var playlist = this._getCurrentPlaylist();
    if (playlist == null) {
      dump("SearchHandler__setPlaylistSearch: NO PLAYLIST!\n");
      return false;
    }
    
    var success = false;
    
    try { 
      playlist.searchFilter = query;
      success = true;

      // TODO Remove this legacy functionality.  The jump-to should be self contained.
      if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
    } catch(e) {
      dump("SearchHandler__setPlaylistSearch: Unable to set the search query.\n");
      dump(e + "\n");
    }
    return success;
  },

  
  /**
   * Try to get a human readable name for the playlist 
   * in the current tab
   */
  _getPlaylistDisplayName: function SearchHandler__getPlaylistDisplayName() {
  
    // Get the playlist element from within the current tab      
    var playlist = this._getCurrentPlaylist();

    // Attempt to get the name of the playlist
    if (playlist) {
      return playlist.displayName;
    } 
    return "";
  },  
    

  /**
   * Get the playlist that is currently showing in the browser
   */
  _getCurrentPlaylist: function SearchHandler__getCurrentPlaylist() {
    return gBrowser.currentPlaylist;
  },  
  
  
  /**
   * Update the Songbird search to reflect
   * the state of the current playlist
   */
  _syncSearchBarToPlaylist: function SearchHandler__syncSearchBarToPlaylist() {
    
    // Get the search box element
    var searchBar = this.getSearchBar();
    if (searchBar == null) {
      return;
    }    
      
    // Change the text displayed on empty query to reflect
    // the current search context.
    var playlistName = this._getPlaylistDisplayName();
    if (playlistName != "") {
      var songbirdEngine = this.getSongbirdSearchEngine();
      searchBar.setEngineDisplayText(songbirdEngine, playlistName);
    }

    // Find out what search is filtering this medialist
    var queryString = this._getPlaylistSearch();    
    searchBar.value = queryString;
  }
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

                
