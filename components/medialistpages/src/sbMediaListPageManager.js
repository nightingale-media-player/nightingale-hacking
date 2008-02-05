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
Cu.import("resource://app/components/ArrayConverter.jsm");
Cu.import("resource://app/components/RDFHelper.jsm");
Cu.import("resource://app/components/sbProperties.jsm");

function MediaListPageManager() {
}

MediaListPageManager.prototype = {
  classDescription: "Songbird MediaListPage Manager",
  classID:          Components.ID("{e63463d0-357c-4035-af33-db670ee1b7f2}"),
  contractID:       "@songbirdnest.com/Songbird/MediaListPageManager;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIMediaListPageManager]),
  
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
        if (!iid.equals(Ci.sbIMediaListPageInfo) &&
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
  
  getAvailablePages: function(aList) {
    this._ensureMediaPageRegistration();
    
    // If no list is provided, return the entire set
    if (!aList) {
      return ArrayConverter.enumerator(this._pageInfoArray); 
    }
    
    // Otherwise, make a list of what matches the list
    var tempArray = [];
    for (var i in this._pageInfoArray) {
      var pageInfo = this._pageInfoArray[i];
      if (pageInfo.matchInterface.match(aList)) {
        tempArray.push(pageInfo);
      }
    }
    
    // ... and return that.
    return ArrayConverter.enumerator(tempArray); 
  },
  
  getPage: function(aList) {
    this._ensureMediaPageRegistration();
    
    // Read the saved state
    var remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                 .createInstance(Ci.sbIDataRemote);
    remote.init("mediapages." + aList.guid, null);
    var savedPageURL = remote.stringValue;
    if (savedPageURL && savedPageURL != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, savedPageURL);
      if (pageInfo) return pageInfo;
    }
    
    // Read the list's default
    var defaultPageURL = aList.getProperty(SBProperties.defaultMediaListPageURL);
    if (defaultPageURL && defaultPageURL != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, defaultPageURL);
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
        if (pageInfo.matchInterface.match(aList)) {
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
    // Save the state
    var remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                 .createInstance(Ci.sbIDataRemote);
    remote.init("mediapages." + aList.guid, null);
    remote.stringValue = aPageInfo.contentUrl;
  },
  
  // internal. checks that a url is registered in the list of pages, and that 
  // its matching test succeeds for a given list
  _checkPageForList: function(aList, aUrl) {
    for (var i in this._pageInfoArray) {
      var pageInfo = this._pageInfoArray[i];
      if (pageInfo.contentUrl != aUrl) continue;
      if (!pageInfo.matchInterface.match(aList)) continue;
      return pageInfo;
    }
    return null;
  },

  _ensureMediaPageRegistration: function() {
    if(this._registrationComplete) { return };
    
    this._registerDefaults();
    MediaListPageMetadataReader.loadMetadata(this);
    
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
    var matchAll = {match: function(mediaList) { return(true); }};

    // Register the playlist with filters
    this._defaultFilteredPlaylistPage =
        this.registerPage( filteredPlaylistString,
        "chrome://songbird/content/xul/mediapages/playlistPage.xul?useFilters=true", 
        matchAll);    

    // And the playlist without filters
    this._defaultPlaylistPage = 
        this.registerPage( playlistString,
        "chrome://songbird/content/xul/mediapages/playlistPage.xul",
        matchAll);                       
  },
  
} // MediaListPageManager.prototype



/**
 * MediaListPageMetadataReader
 * Reads the Add-on Metadata RDF datasource for Media Page declarations.
 */
var MediaListPageMetadataReader = {
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
    // create and validate our pane info
    var info = {};
    for (property in page) {
      if (page[property])
       info[property] = page[property][0];
    }
    requiredProperties = ["contentTitle", "contentUrl"];
    optionalProperties = ["match"];
    
    var errorList = [];
    var warningList = [];
    
    for (p in requiredProperties) { 
      if (!info[requiredProperties[p]]) {
        errorList.push("Missing required property " + requiredProperties[p] + ".\n")
      }
    }
    for (p in info) {
      if (!requiredProperties[p] && !optionalProperties[p]) {
        //TODO: warningList.push("Unrecognized property " + p + ".\n")
      }
    } 
    
    // If errors were encountered, then do not submit
    if (warningList.length > 0){
      this._reportErrors(
          "Warning media page addon in the install.rdf of extension " +
          addon.Value + " reported these warning(s):\n", warningList);
    } 
    if (errorList.length > 0) {
      this._reportErrors(
          "Ignoring media page addon in the install.rdf of extension " +
          addon.Value + " due to these error(s):\n", errorList);
      return;
    }
    
    // TODO: write the parser for the info.match() part
    
    // Submit description
    this._manager.registerPage( info.contentTitle,
                                info.contentUrl,
                                {match: function(mediaList) { return(true); }}
                               );
    
    //dump("MediaPageMetadataReader: registered pane " + info.contentTitle
    //     + " at " + info.contentUrl + " from addon " + addon.Value + " \n");
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
      consoleService.logStringMessage("Media Page Metadata Reader: " 
                                       + contextMessage + errorList[i]);
      dump("MediaPageMetadataReader: " + contextMessage + errorList[i] + "\n");
    }
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([MediaListPageManager]);
}

