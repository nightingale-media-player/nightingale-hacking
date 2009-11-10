/**
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
 */
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/RDFHelper.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

function MediaPageManager() {
}

MediaPageManager.prototype = {
  classDescription: "Songbird MediaPage Manager",
  classID:          Components.ID("{e63463d0-357c-4035-af33-db670ee1b7f2}"),
  contractID:       "@songbirdnest.com/Songbird/MediaPageManager;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIMediaPageManager]),
  
  _pageInfoArray: [],
  
  // PageInfo objects for the fallback mediapages.  Set by _registerDefaults.
  _defaultPlaylistPage:  null,
  _defaultFilteredPlaylistPage: null,
  
  
  registerPage: function(aName, aURL, aMatchInterface) {
  
    // Make sure we don't already have a page with
    // the given url
    var pageInfo;
    
    for each (pageInfo in this._pageInfoArray) {
      if (pageInfo.contentUrl == aURL) {
        throw new Error("Page URL already registered: " + aURL);
      }
    }
    
    // Make a PageInfo object
    pageInfo = {
      get contentTitle() { 
        return aName;
      },
      get contentUrl() {
        return aURL;
      },
      get matchInterface() {
        return aMatchInterface;
      },
      QueryInterface: function(iid) {
        if (!iid.equals(Ci.sbIMediaPageInfo) &&
            !iid.equals(Ci.nsISupports))
          throw Cr.NS_ERROR_NO_INTERFACE;
        return this;
      }
    };
    
    // Store the page into our page array
    this._pageInfoArray.push(pageInfo);
    
    // And return it to the caller so he may use that to unregister
    return pageInfo;
  },
  
  unregisterPage: function(aPageInfo) {

    // If unregistering the default pages, must remove shortcut pointers
    if (this._defaultFilteredPlaylistPage && 
        this._defaultFilteredPlaylistPage.contentUrl == aPageInfo.contentUrl) {
      this._defaultFilteredPlaylistPage = null;
    } else if (this._defaultPlaylistPage && 
      this._defaultPlaylistPage.contentUrl == aPageInfo.contentUrl) {
      this._defaultPlaylistPage = null;
    }
  
    // Search the array for the matching page
    for (var i in this._pageInfoArray) {
      if (aPageInfo.contentUrl == this._pageInfoArray[i].contentUrl) {
        // found, remove it and stop
        this._pageInfoArray.splice(i, 1);
        return;
      }
    }
    // Page not found, throw!
    throw new Error("Page " + aPageInfo.contentTitle + " not found in unregisterPage");
  },
  
  getAvailablePages: function(aList, aConstraint) {
    this._ensureMediaPageRegistration();
    // If no list is provided, return the entire set
    if (!aList) {
      return ArrayConverter.enumerator(this._pageInfoArray); 
    }
    // Otherwise, make a list of what matches the list
    var tempArray = [];
    for (var i in this._pageInfoArray) {
      var pageInfo = this._pageInfoArray[i];
      if (pageInfo.matchInterface.match(aList, aConstraint)) {
        tempArray.push(pageInfo);
      }
    }
    
    // ... and return that.
    return ArrayConverter.enumerator(tempArray); 
  },
  
  getPage: function(aList, aConstraint) {
    this._ensureMediaPageRegistration();

    // use the outermost list
    aList = this._getOutermostList(aList);
    
    // Read the saved state
    var remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                 .createInstance(Ci.sbIDataRemote);
    remote.init("mediapages." + aList.guid, null);
    var savedPageURL = remote.stringValue;
    if (savedPageURL && savedPageURL != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, aConstraint, savedPageURL);
      if (pageInfo) return pageInfo;
    }
    
    // Read the list's default
    var defaultPageURL = aList.getProperty(SBProperties.defaultMediaPageURL);
    if (defaultPageURL && defaultPageURL != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, aConstraint, defaultPageURL);
      if (pageInfo) return pageInfo;
    }
    
    // No saved state and no default, this is either the first time this list
    // is shown, or its previous saved/default page isn't valid anymore, so
    // pick a new one
    
    // Hardcoded first run logic:
    // Libraries get filter lists, playlists do not.
    if (aList instanceof Ci.sbILibrary && this._defaultFilteredPlaylistPage) {
      return this._defaultFilteredPlaylistPage;
    } else if (this._defaultPlaylistPage)  {
      return this._defaultPlaylistPage;
    } else {
      // No hardcoded defaults.  Look for anything
      // that matches.
      for (var i in this._pageInfoArray) {
        var pageInfo = this._pageInfoArray[i];
        if (pageInfo.matchInterface.match(aList, aConstraint)) {
          return pageInfo;
        }
      }
    }
    
    // Oh crap.
    throw new Error("MediaPageManager unable to determine a page for " + aList.guid);

    // keep js happy ?
    return null;
  },
  
  setPage: function(aList, aPageInfo) {
    // use the outermost list
    aList = this._getOutermostList(aList);

    // Save the state
    var remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                 .createInstance(Ci.sbIDataRemote);
    remote.init("mediapages." + aList.guid, null);
    remote.stringValue = aPageInfo.contentUrl;
  },

  // internal, get the outermost list for a given list. for instance, when
  // given a smart medialist's storage list, this returns the original
  // smart medialist.
  _getOutermostList: function(aList) {
    var outerGuid = aList.getProperty(SBProperties.outerGUID);
    if (outerGuid)
      aList = aList.library.getMediaItem(outerGuid);
    return aList;
  },
  
  // internal. checks that a url is registered in the list of pages, and that 
  // its matching test succeeds for a given list
  _checkPageForList: function(aList, aConstraint, aUrl) {
    for (var i in this._pageInfoArray) {
      var pageInfo = this._pageInfoArray[i];
      if (pageInfo.contentUrl != aUrl) continue;
      if (!pageInfo.matchInterface.match(aList, aConstraint)) continue;
      return pageInfo;
    }
    return null;
  },

  _ensureMediaPageRegistration: function() {
    if(this._registrationComplete) { return };
    
    this._registerDefaults();
    MediaPageMetadataReader.loadMetadata(this);
    
    this._registrationComplete = true;
  },
  
  _registerDefaults: function() {
    var playlistString = "mediapages.playlistpage";
    var filteredPlaylistString = "mediapages.filteredplaylistpage";
    try {
      var stringBundleService =
          Components.classes["@mozilla.org/intl/stringbundle;1"]
                    .getService(Components.interfaces.nsIStringBundleService);
      var stringBundle = stringBundleService.createBundle(
           "chrome://songbird/locale/songbird.properties" );
      playlistString = stringBundle.GetStringFromName(playlistString);
      filteredPlaylistString = stringBundle.GetStringFromName(
                  filteredPlaylistString);
      stringBundleService = null;
      stringBundle = null;
    } catch (e) {
      Component.utils.reportError("MediaPageManager: Couldn't localize default media page name.\n")
    }

    // the default page matches everything    
    var matchAll = {match: function() true};

    // Register the playlist with filters
    this._defaultFilteredPlaylistPage =
        this.registerPage( filteredPlaylistString,
        "chrome://songbird/content/mediapages/filtersPage.xul", 
        matchAll);

    // And the playlist without filters
    this._defaultPlaylistPage = 
        this.registerPage( playlistString,
        "chrome://songbird/content/mediapages/playlistPage.xul",
        matchAll);
  },
  
}; // MediaPageManager.prototype



/**
 * MediaPageMetadataReader
 * Reads the Add-on Metadata RDF datasource for Media Page declarations.
 */
var MediaPageMetadataReader = {
  loadMetadata: function(manager) {
    this._manager = manager;
    
    var addons = RDFHelper.help(
      "rdf:addon-metadata",
      "urn:songbird:addon:root",
      RDFHelper.DEFAULT_RDF_NAMESPACES
    );
    
    for (var i = 0; i < addons.length; i++) {
      // skip addons with no panes.
      if (!addons[i].mediaPage) 
        continue;
      try {
        var pages = addons[i].mediaPage;
        for (var j = 0; j < pages.length; j++) {
          this._registerMediaPage(addons[i], pages[j]) 
        }
      } catch (e) {
        this._reportErrors("", [  "An error occurred while processing " +
                  "extension " + addons[i].Value + ".  Exception: " + e  ]);
      }
    }
  },
  
  /**
   * Extract pane metadata and register it with the manager.
   */
  _registerMediaPage: function _registerMediaPage(addon, page) {
    // create and validate our page info
    var errorList = [];
    var warningList = [];
    
    var info = {};
    for (property in page) {
      if (page[property])
       info[property] = page[property][0];
    }
    
    this._validateProperties(info, errorList, warningList);
    
    // create a match function
    var matchFunction;
    if (page.match) {
      var matchList = this._createMatchList(page, warningList);
      matchFunction = this._createMatchFunction(matchList);
    }
    else {
      matchFunction = this._createMatchAllFunction();
    }
    
    // If errors were encountered, then do not submit
    if (warningList.length > 0){
      this._reportErrors(
          "Warning: " + addon.Value + " install.rdf loading media page: " , warningList);
    } 
    if (errorList.length > 0) {
      this._reportErrors(
          "ERROR: " + addon.Value + " install.rdf IGNORED media page: ", errorList);
      return;
    }
    
    // Submit description
    this._manager.registerPage( info.contentTitle,
                                info.contentUrl,
                                {match: matchFunction}
                               );
    
    //dump("MediaPageMetadataReader: registered pane " + info.contentTitle
    //     + " at " + info.contentUrl + " from addon " + addon.Value + " \n");
  },
  
  _validateProperties: function(info, errorList, warningList) {
    var requiredProperties = ["contentTitle", "contentUrl"];
    var optionalProperties = ["match"];
    
    // check for required properties
    for (var p in requiredProperties) { 
      if (!info[requiredProperties[p]]) {
        errorList.push("Missing required property " + requiredProperties[p] + ".\n")
      }
    }
    
    // check for unused RDF nodes and warn about them.
    var template = {};
    for(var i in requiredProperties) {
      template[requiredProperties[i]] = "required";
    }
    for(var i in optionalProperties) {
      template[optionalProperties[i]] = "optional";
    }
    
    for (var p in info) {
      if (!template[p]) {
        warningList.push("Unrecognized property " + p + ".\n")
      }
    }
  },
  
  _createMatchList: function(page, warningList) {
    // create a set of property-comparison objects
    // one for each <match>. a mediapage will work for medialists which match
    // all the properties in a hash.
    var matchList = [];
    
    for (var i = 0; i < page.match.length; i++) {
      var fields = page.match[i].split(/\s+/);
      var properties = {}
      for(var f in fields) {
        var key; var value;
        [key, value] = fields[f].split(":");
        if(!value) { value = key; key = "type" };
        
        // TODO: quoted string values ala name:"My Funky List"
        // TODO: check if the key is actually a legal key-type and warn if not
        
        if(properties[key]) {
           warningList.push("Attempting to match two values for "
                            +key+": "+properties[key]+" and "+value+".");
        }
        
        properties[key] = value;
      }
      
      matchList.push(properties); 
    }
    
    return matchList;
  },
  
  _createMatchFunction: function(matchList) {
    // create a match function that uses the match options
    var matchFunction = function(mediaList, aConstraint) {
      // just in case someone's passing in bad values
      if(!mediaList) {
        return false;
      }

      // check each <match/>'s values
      // if any one set works, this is a good media page for the list
      for (var m in matchList) {
        let match = matchList[m];

        // first, see if we just want to opt out of this match
        // our definition of an opt-out-able list is:
        // one that is so unspecific as to only target by "type"
        // or to target everything. (see below)
        // (this is a bit natty)
        var numProperties = 0;
        for (var i in match) {
          numProperties++
        }
        // if we *only* target the "type" of the list
        // and the list wants to opt out
        if (numProperties == 1 && match["type"]) {
          if (mediaList.getProperty(SBProperties.onlyCustomMediaPages) == "1") {
            return false;
          }
        }

        var thisListMatches = true;
        for (var i in match) {
          switch(i) {
            case "contentType":
              // this is set on the constraint instead
              if (aConstraint) {
                for (let group in ArrayConverter.JSEnum(aConstraint.groups)) {
                  if (!(group instanceof Ci.sbILibraryConstraintGroup)) {
                    continue;
                  }
                  if (!group.hasProperty(SBProperties.contentType)) {
                    continue;
                  }
                  let contentTypes =
                    ArrayConverter.JSArray(group.getValues(SBProperties.contentType));
                  if (contentTypes.indexOf(match[i]) == -1) {
                    thisListMatches = false;
                    break;
                  }
                }
              }
              break;
            default:
              if (i in mediaList) {
                thisListMatches &= (match[i] == mediaList[i]);
              }
              else {
                // Use getProperty notation if the desired field is not
                // specified as a true JS property on the object.
                // TODO: This should be improved.
                thisListMatches &= (match[i] == mediaList.getProperty(SBProperties[i]));
              }
          }
          if (!thisListMatches) {
            break;
          }
        }
        if (thisListMatches) {
          return true;
        }
      }

      // arriving here means none of the <match>
      // elements fit the medialist passed in
      return false;
    }

    return matchFunction;
  },
  
  _createMatchAllFunction: function() {
    var matchFunction = function(mediaList) {
      // opt-out lists will also exclude completely generic pages
      // this detail must be clearly communicated to the MP dev'rs!
      return(mediaList && mediaList.getProperty(SBProperties.onlyCustomMediaPages) != "1");
    };
    return matchFunction;
  },
  
  /**
   * \brief Dump a list of errors to the console and jsconsole
   *
   * \param contextMessage Additional prefix to use before every line
   * \param errorList Array of error messages
   */
  _reportErrors: function _reportErrors(contextMessage, errorList) {
    var consoleService = Components.classes["@mozilla.org/consoleservice;1"].
         getService(Components.interfaces.nsIConsoleService);
    for (var i = 0; i  < errorList.length; i++) {
      Components.utils.reportError("MediaPage Addon Metadata: " + contextMessage + errorList[i]);
    }
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([MediaPageManager]);
}

