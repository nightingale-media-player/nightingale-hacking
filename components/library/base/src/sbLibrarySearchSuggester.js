/**
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
 */
 
/**
 * \file sbLibrarySearchSuggester.js
 * Provides autocomplete suggestions based on distinct values for a property
 * Originally based on the Mozilla nsSearchSuggestions.js implementation.
 *
 * The format of the searchparam attribute is the following:
 *
 *  property;libraryguid;defaultvalues;unit
 *
 *  - 'property' is a property id, such as http://getnightingale.com/data/1.0#artistName
 *  - 'libraryguid' is the guid of a library from which to get distinct values,
 *    or no value to get from all libraries
 *  - 'defaultvalues' is a comma separated list of additional default values
 *    which are always matched against this input even if they are not part of
 *    the distinct values set
 *  - 'unit' is the unit into which the distinct values should be converted to
 *    before being matched against the input and inserted in the suggestion
 *    result set.
 */ 

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@mozilla.org/autocomplete/search;1?name=library-distinct-properties";
const DESCRIPTION = "Nightingale Library Search Suggestions";
const CID = Components.ID("{1ed101bc-a11c-4e03-83af-514672bd3a70}");

const XPCOM_SHUTDOWN_TOPIC              = "xpcom-shutdown";


/**
 * Map of properties to hard-coded default values
 */
var gDefaultValues = {};
gDefaultValues["audio"] = [
  "Alternative", "Blues/R&B", "Books&Spoken", "Children's Music",
  "Classical", "Comedy", "Country", "Dance", "Easy Listening", "World",
  "Electronic", "Folk", "Hip Hop/Rap", "Holiday", "House", "Industrial",
  "Jazz", "New Age", "Nerdcore", "Podcast", "Pop", "Reggae", "Religious",
  "Rock", "Science", "Soundtrack", "Techno", "Trance", "Unclassifiable",
];

gDefaultValues["video"] = [
  "Children's", "Comedy", "Drama", "Entertainment", "Healthcare & Fitness",
  "Travel", "Unclassifiable",
];

/**
 * AutoCompleteResult contains the results returned by the Suggest
 * service - it implements nsIAutoCompleteResult and is used by the auto-
 * complete controller to populate the front end.
 * @constructor
 */
function AutoCompleteResult(searchString,
                                   defaultIndex,
                                   errorDescription,
                                   results) {
  this._searchString = searchString;
  this._defaultIndex = defaultIndex;
  this._errorDescription = errorDescription;
  this._results = results;
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
      return Ci.nsIAutoCompleteResult.RESULT_NOMATCH;
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
    return "";
  },

  /**
   * Retrieves a style hint specific to a particular index.
   * @param  index    the index of the style hint requested
   * @return          the style hint at the specified index
   */
  getStyleAt: function(index) {
    if (!this._results[index])
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
 * on Nightingale's state.
 *
 * To access this suggester set autocompletesearch="library-distinct-properties"
 * on an autocomplete textbox.  See the search.xml binding for details.
 *
 * @constructor
 */
function LibrarySearchSuggester() {
    var os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);
    os.addObserver(this, XPCOM_SHUTDOWN_TOPIC, false);
}

LibrarySearchSuggester.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,

  /**
   * The object implementing nsIAutoCompleteObserver that we notify when
   * we have found results
   * @private
   */
  _listener: null,
  _libraryManager: null,
  _lastSearch: null,
  _timer: null,
  _distinctValues: null,
  _cacheParam: null,


  /**
   * Notifies the front end of new results.
   * @param searchString  the user's query string
   * @param results       an array of results to the search
   * @private
   */
  onSearchResult: function(searchString, results) {
    if (this._listener) {
      var result = new AutoCompleteResult(
          searchString,
          0,
          "",
          results);

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
   * @param searchParam     the search parameter
   * @param previousResult  unused, a client-cached store of the previous
   *                        generated resultset for faster searching.
   * @param listener        object implementing nsIAutoCompleteObserver which
   *                        we notify when results are ready.
   */
  startSearch: function(searchString, searchParam, previousResult, listener) {

    // If no property was specified, we can't perform a search, abort now
    if (!searchParam ||
        searchParam == "") {
      // notify empty result, probably not needed but hey, why not.
      this.onSearchResult(searchString, []);
      return;
    }
      
    // If there's an existing request, stop it
    this.stopSearch();

    // remember the listener
    this._listener = listener;
    
    // if we do not yet have the distinct values, or if they
    // have been invalidated, get them again now.
    if (!this._distinctValues ||
        this._cacheParam != searchParam) {
      
      // remember current search param
      this._cacheParam = searchParam;
      
      // discard previous results no matter what,
      // we want to use the new data
      previousResult = null;
      
      // start anew
      this._distinctValues = {};

      // get the library manager if needed
      if (!this._libraryManager) {
        this._libraryManager = 
          Cc["@getnightingale.com/Nightingale/library/Manager;1"]
            .getService(Ci.sbILibraryManager);
      }

      // parse search parameters
      var params = searchParam.split(";");

      var properties = params[0].split("$");
      this._prop = properties[0];
      this._type = properties[1] || "audio";

      var guid = params[1];
      var additionalValues = params[2];
      this._conversionUnit = params[3];

      if (this._type != "video") {
        // Record distinct values for a library
        function getDistinctValues(aLibrary, prop, obj) {
          if (!aLibrary) 
            return;
          var values = aLibrary.getDistinctValuesForProperty(prop);
          while (values.hasMore()) { 
            // is there a way to assert a key without doing an assignment ?
            obj._distinctValues[values.getNext()] = true;
          }
        }

        // If we have a guid in the params, get the distinct values
        // from a library with that guid, otherwise, get them from
        // all libraries
        if (guid && guid.length > 0) {
          getDistinctValues(this._libraryManager.getLibrary(guid),
                            this._prop, this);
        } else {
          var libs = this._libraryManager.getLibraries();
          while (libs.hasMoreElements()) {
            getDistinctValues(libs.getNext(), this._prop, this);
          }
        }
      }

      // If we have additional values, add them to the 
      // distinct values array
      if (additionalValues && additionalValues.length > 0) {
        var values = additionalValues.split(",");
        for each (var value in values) {
          this._distinctValues[value] = true;
        }
      }
      
      // Add hardcoded values, if any
      this._addDefaultDistinctValues();
      
      // set this cache to expire in 5s

      if (!this._timer)
        this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

      this._timer.cancel();
      this._timer.initWithCallback(this, 5000, this._timer.TYPE_ONE_SHOT);
    }

    // our search is case insensitive
    var search = searchString.toLowerCase();

    // matching function
    function startsWith(aString, aPartial) {
      return (!aPartial ||
               aPartial == "" ||
               aString.toLowerCase().slice(0, aPartial.length) == aPartial);
    }

    var results = [];
    
    // if this is a narrowing down of the previous search,
    // use the previousResults array, otherwise,
    // use the full distinctValues array
    
    if (previousResult &&
        startsWith(search, this._lastSearch)) {
      for (var i = 0 ; i < previousResult.matchCount ; i++) {
        var value = previousResult.getValueAt(i);
        if (startsWith(value, search))
          results.push(value);
      }
    } else {

      var converter = null;
      if (this._conversionUnit && this._conversionUnit != "") {
        var propertyManager = 
          Cc["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
            .getService(Ci.sbIPropertyManager);
        var info = propertyManager.getPropertyInfo(this._prop);
        converter = info.unitConverter;
      }

      for (var value in this._distinctValues) {
        if (converter) {
          value = converter.convert(value, 
                                    converter.nativeUnitId, 
                                    this._conversionUnit, 
                                    -1, -1 /* no min/max decimals */);
        }
        if (startsWith(value, search))
          results.push(value);
      }
    }
    
    // remember the last search string, to see if 
    // we can use previousResult next time.
    this._lastSearch = search;
    
    // Notify the listener that we got results
    this.onSearchResult(searchString, results);    
  },
  
  // one shot timer notification method
  notify: function(timer) {
    this._lastSearch = null;
    this._distinctValues = null;
  },

  /**
   * Ends the search result gathering process. Part of nsIAutoCompleteSearch
   * implementation.
   */
  stopSearch: function() {
    // Nothing to do since we return our searches immediately.
  },

  /**
   * Add hardcoded default values for the current property
   */
  _addDefaultDistinctValues: function() {
    var defaults = gDefaultValues[this._type];
    if (defaults) {
      for each (var value in defaults) {
        this._distinctValues[value] = true;
      }
    }
  },

  /**
   * nsIObserver
   */
  observe: function SAC_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case XPCOM_SHUTDOWN_TOPIC:
        this.stopSearch();
        this._libraryManager = null;
        if (this._timer) {
          this._timer.cancel();
          this._timer = null;
        }
        var os = Cc["@mozilla.org/observer-service;1"]
                   .getService(Ci.nsIObserverService);
        os.removeObserver(this, XPCOM_SHUTDOWN_TOPIC);
        break;
    }
  },

  /**
   * Part of nsISupports implementation.
   * @param   iid     requested interface identifier
   * @return  this object (XPConnect handles the magic of telling the caller that
   *                       we're the type it requested)
   */
  QueryInterface:
    XPCOMUtils.generateQI([Ci.nsIAutoCompleteSearch,
                           Ci.nsIObserver])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([LibrarySearchSuggester]);
}
