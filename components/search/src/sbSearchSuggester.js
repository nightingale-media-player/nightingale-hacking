/**
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
 * \file sbSearchSuggester.js
 * Provides autocomplete suggestions based on player state/context
 * Originally based on the Mozilla nsSearchSuggestions.js implementation
 */ 



const BROWSER_SUGGEST_PREF = "browser.search.suggest.enabled";
const XPCOM_SHUTDOWN_TOPIC              = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbirdnest.com/Songbird/DataRemote;1";
const sbIDataRemote            = Components.interfaces.sbIDataRemote;


const SEARCH_SUGGEST_CONTRACTID =
  "@mozilla.org/autocomplete/search;1?name=songbird-autocomplete";
const SEARCH_SUGGEST_CLASSNAME = "Songbird Search Suggestions";
const SEARCH_SUGGEST_CLASSID =
  Components.ID("{0be64502-ee00-11db-8314-0800200c9a66}");

const SEARCH_BUNDLE = "chrome://songbird/locale/songbird.properties";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;


/**
 * AutoCompleteResult contains the results returned by the Suggest
 * service - it implements nsIAutoCompleteResult and is used by the auto-
 * complete controller to populate the front end.
 * @constructor
 */
function AutoCompleteResult(searchString,
                                   defaultIndex,
                                   errorDescription,
                                   results,
                                   comments) {
  this._searchString = searchString;
  this._defaultIndex = defaultIndex;
  this._errorDescription = errorDescription;
  this._results = results;
  this._comments = comments;
}
AutoCompleteResult.prototype = {
  /**
   * The user's query string
   * @private
   */
  _searchString: "",

  /**
   * The default item that should be entered if none is selected
   * @private
   */
  _defaultIndex: 0,

  /**
   * The reason the search failed
   * @private
   */
  _errorDescription: "",

  /**
   * The list of words returned by the Suggest Service
   * @private
   */
  _results: [],

  /**
   * The list of Comments (number of results - or page titles) returned by the
   * Suggest Service.
   * @private
   */
  _comments: [],


  /**
   * @return the user's query string
   */
  get searchString() {
    return this._searchString;
  },

  /**
   * @return the result code of this result object, either:
   *         RESULT_IGNORED   (invalid searchString)
   *         RESULT_FAILURE   (failure)
   *         RESULT_NOMATCH   (no matches found)
   *         RESULT_SUCCESS   (matches found)
   */
  get searchResult() {
    if (this._results.length > 0) {
      return Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
    } else {
      Ci.nsIAutoCompleteResult.RESULT_NOMATCH;
    }
  },

  /**
   * @return the default item that should be entered if none is selected
   */
  get defaultIndex() {
    return this._defaultIndex;
  },

  /**
   * @return the reason the search failed
   */
  get errorDescription() {
    return this._errorDescription;
  },

  /**
   * @return the number of results
   */
  get matchCount() {
    return this._results.length;
  },

  /**
   * Retrieves a result
   * @param  index    the index of the result requested
   * @return          the result at the specified index
   */
  getValueAt: function(index) {
    return this._results[index];
  },

  /**
   * Retrieves a comment (metadata instance)
   * @param  index    the index of the comment requested
   * @return          the comment at the specified index
   */
  getCommentAt: function(index) {
    return this._comments[index];
  },

  /**
   * Retrieves a style hint specific to a particular index.
   * @param  index    the index of the style hint requested
   * @return          the style hint at the specified index
   */
  getStyleAt: function(index) {
    if (!this._comments[index])
      return null;  // not a category label, so no special styling

    if (index == 0)
      return "suggestfirst";  // category label on first line of results

    return "suggesthint";   // category label on any other line of results
  },

  /** 
   * Retrieves an image url. 
   * @param  index    the index of the image url requested 
   * @return          the image url at the specified index 
   */ 
  getImageAt: function(index) { 
    return ""; 
  },
     
  /**
   * Removes a result from the resultset
   * @param  index    the index of the result to remove
   */
  removeValueAt: function(index, removeFromDatabase) {
    this._results.splice(index, 1);
    this._comments.splice(index, 1);
  },

  /**
   * Part of nsISupports implementation.
   * @param   iid     requested interface identifier
   * @return  this object (XPConnect handles the magic of telling the caller that
   *                       we're the type it requested)
   */
  QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsIAutoCompleteResult) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};






/**
 * Implements nsIAutoCompleteSearch to provide suggestions based 
 * on Songbird's state.
 *
 * To access this suggester set autocompletesearch="songbird-autocomplete"
 * on an autocomplete textbox.  See the search.xml binding for details.
 *
 * @constructor
 */
function SearchSuggester() {
  this._addObservers();
  this._loadSuggestPref();
  this._loadDataRemotes();
}
SearchSuggester.prototype = {

  /**
   * this._strings is the string bundle for message internationalization.
   */
  get _strings() {
    if (!this.__strings) {
      var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService);

      this.__strings = sbs.createBundle(SEARCH_BUNDLE);
    }
    return this.__strings;
  },
  __strings: null,

  /**
   * Search suggestions will be shown if this._suggestEnabled is true.
   */
  _loadSuggestPref: function SAC_loadSuggestPref() {
    var prefService = Cc["@mozilla.org/preferences-service;1"].
                      getService(Ci.nsIPrefBranch);
    this._suggestEnabled = prefService.getBoolPref(BROWSER_SUGGEST_PREF);
  },
  _suggestEnabled: null,

  // Metadata for the current playing track in Songbird
  _metadataTitle:  null,
  _metadataArtist: null,
  _metadataAlbum:  null,
  
  /**
   * Load metadata dataremotes
   */
  _loadDataRemotes: function SAC_loadDataRemotes() {
  
    // TODO: Is this an issue?  Should I be waiting for profile load before doing this?
    var createDataRemote = new Components.Constructor( 
                SONGBIRD_DATAREMOTE_CONTRACTID, sbIDataRemote, "init");
    this._metadataTitle   = createDataRemote("metadata.title", null);
    this._metadataArtist  = createDataRemote("metadata.artist", null);
    this._metadataAlbum   = createDataRemote("metadata.album", null);
  },  
  
  /**
   * Let go of metadata dataremotes just in case
   */
  _releaseDataRemotes: function() {
    this._metadataTitle.unbind();
    this._metadataArtist.unbind();
    this._metadataAlbum.unbind();
  },  


  /**
   * The object implementing nsIAutoCompleteObserver that we notify when
   * we have found results
   * @private
   */
  _listener: null,


  /**
   * Notifies the front end of new results.
   * @param searchString  the user's query string
   * @param results       an array of results to the search
   * @param comments      an array of metadata corresponding to the results
   * @private
   */
  onSearchResult: function(searchString, results, comments) {
    if (this._listener) {
      var result = new AutoCompleteResult(
          searchString,
          0,
          "",
          results,
          comments);

      this._listener.onSearchResult(this, result);

      // Null out listener to make sure we don't notify it twice, in case our
      // timer callback still hasn't run.
      this._listener = null;
    }
  },


  /**
   * Initiates the search result gathering process. Part of
   * nsIAutoCompleteSearch implementation.
   *
   * @param searchString    the user's query string
   * @param searchParam     unused, "an extra parameter"; even though
   *                        this parameter and the next are unused, pass
   *                        them through in case the form history
   *                        service wants them
   * @param previousResult  unused, a client-cached store of the previous
   *                        generated resultset for faster searching.
   * @param listener        object implementing nsIAutoCompleteObserver which
   *                        we notify when results are ready.
   */
  startSearch: function(searchString, searchParam, previousResult, listener) {
    this.stopSearch();
    
    var searchService = Cc["@mozilla.org/browser/search-service;1"].
                        getService(Ci.nsIBrowserSearchService);

    // If there's an existing request, stop it
    this.stopSearch();

    this._listener = listener;

    var results = [];

    var engine = searchService.currentEngine;

    // Normally we would do something asynchronous, but since 
    // for now all we're returning is dataremote values, we
    // might as well just do it immediately.
    
    // If there is no search query then get some default suggestions
    if (searchString == "") {
      results = this._getPlayerContextSuggestions();    
    }
    
    // TODO Add a localized comment
    var comments = [];
    for (var i = 0; i < results.length; i++) {
      comments.push("");
    }

    this.onSearchResult(searchString, results, comments);    
  },

  /**
   * Ends the search result gathering process. Part of nsIAutoCompleteSearch
   * implementation.
   */
  stopSearch: function() {
    // Nothing to do since we return our searches immediately.
  },


  /**
   * Get a list of suggestions to display regardless of the search query
   */
  _getPlayerContextSuggestions: function() {
    var results = [];
    
    // TODO: Do not return metadata unless playing or paused!
    // Currently returns metadata from the previous session.
    
    if (this._metadataTitle.stringValue != "") {
      results.push(this._metadataTitle.stringValue);
    }
    if (this._metadataAlbum.stringValue != "") {
      results.push(this._metadataAlbum.stringValue);
    }
    if (this._metadataArtist.stringValue != "") {
      results.push(this._metadataArtist.stringValue);
    }
    return results;
  },


  /**
   * nsIObserver
   */
  observe: function SAC_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        this._loadSuggestPref();
        break;
      case XPCOM_SHUTDOWN_TOPIC:
        this.stopSearch();
        this._removeObservers();
        this._releaseDataRemotes();
        break;
    }
  },

  _addObservers: function SAC_addObservers() {
    var prefService2 = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch2);
    prefService2.addObserver(BROWSER_SUGGEST_PREF, this, false);

    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.addObserver(this, XPCOM_SHUTDOWN_TOPIC, false);
  },

  _removeObservers: function SAC_removeObservers() {
    var prefService2 = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch2);
    prefService2.removeObserver(BROWSER_SUGGEST_PREF, this);

    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.removeObserver(this, XPCOM_SHUTDOWN_TOPIC);
  },

  /**
   * Part of nsISupports implementation.
   * @param   iid     requested interface identifier
   * @return  this object (XPConnect handles the magic of telling the caller that
   *                       we're the type it requested)
   */
  QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsIAutoCompleteSearch) &&
        !iid.equals(Ci.nsIObserver) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};



var gModule = {
  /**
   * Registers all the components supplied by this module. Part of nsIModule
   * implementation.
   * @param componentManager  the XPCOM component manager
   * @param location          the location of the module on disk
   * @param loaderString      opaque loader specific string
   * @param type              loader type being used to load this module
   */
  registerSelf: function(componentManager, location, loaderString, type) {
    if (this._firstTime) {
      this._firstTime = false;
      throw Cr.NS_ERROR_FACTORY_REGISTER_AGAIN;
    }
    componentManager =
      componentManager.QueryInterface(Ci.nsIComponentRegistrar);

    for (var key in this.objects) {
      var obj = this.objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               location, loaderString, type);
    }
  },

  /**
   * Retrieves a Factory for the given ClassID. Part of nsIModule
   * implementation.
   * @param componentManager  the XPCOM component manager
   * @param cid               the ClassID of the object for which a factory
   *                          has been requested
   * @param iid               the IID of the interface requested
   */
  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this.objects) {
      if (cid.equals(this.objects[key].CID))
        return this.objects[key].factory;
    }

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  /**
   * Create a Factory object that can construct an instance of an object.
   * @param constructor   the constructor used to create the object
   * @private
   */
  _makeFactory: function(constructor) {
    function createInstance(outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return (new constructor()).QueryInterface(iid);
    }
    return { createInstance: createInstance };
  },

  /**
   * Determines whether or not this module can be unloaded.
   * @return returning true indicates that this module can be unloaded.
   */
  canUnload: function(componentManager) {
    return true;
  }
};

/**
 * Entry point for registering the components supplied by this JavaScript
 * module.
 * @param componentManager  the XPCOM component manager
 * @param location          the location of this module on disk
 */
function NSGetModule(componentManager, location) {
  // Metadata about the objects this module can construct
  gModule.objects = {
    search: {
      CID: SEARCH_SUGGEST_CLASSID,
      contractID: SEARCH_SUGGEST_CONTRACTID,
      className: SEARCH_SUGGEST_CLASSNAME,
      factory: gModule._makeFactory(SearchSuggester)
    },
  };
  return gModule;
}
