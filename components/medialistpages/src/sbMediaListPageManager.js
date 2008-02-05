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
  
  registerPage: function(aName, aURL, aMatchInterface) {
  
    // Make a unique identifier
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
    var aUUID = aUUIDGenerator.generateUUID();

    // Make a pageInfo object
    var pageInfo = {
      get guid() {
        return aUUID;
      },
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
    // Search the array for the matching page
    for (var i in this._pageInfoArray) {
      if (aPageInfo.guid == this._pageInfoArray[i].guid) {
        // found, remove it and stop
        this._pageInfoArray.splice(i, 1);
        return;
      }
    }
    // Page not found, throw!
    throw new Error("Page " + aPageInfo.guid + " not found in unregisterPage");
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
    remote.init("medialistpage." + aList.guid, null);
    var page_saved_url = remote.stringValue;
    if (page_saved_url && page_saved_url != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, page_saved_url);
      if (pageInfo) return pageInfo;
    }
    
    // Read the list's default
    var page_default_url = aList.getProperty(SBProperties.defaultMediaListPageURL);
    if (page_default_url && page_default_url != "") {
      // Check that the saved url is still registered 
      // and still supports this list
      var pageInfo = this._checkPageForList(aList, page_default_url);
      if (pageInfo) return pageInfo;
    }
    
    // No saved state and no default, this is either the first time this list
    // is shown, or its previous saved/default page isn't valid anymore, so
    // pick a new one
    
    for (var i in this._pageInfoArray) {
      var pageInfo = this._pageInfoArray[i];
      if (pageInfo.matchInterface.match(aList)) {
        return pageInfo;
      }
    }
    
    // nothing matched ! what the hell ? at least one of the songbird pages
    // should support everything, so something is very very wrong here.
    throw new Error("No page found that matches list " + aList.guid);

    // keep js happy ?
    return null;
  },
  
  setPage: function(aList, aPageInfo) {
    // Save the sate
    var remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                 .createInstance(Ci.sbIDataRemote);
    remote.init("medialistpage." + aList.guid, null);
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
    var string = "mediapages.standardview";
    try {
      var stringBundleService =
          Components.classes["@mozilla.org/intl/stringbundle;1"]
                    .getService(Components.interfaces.nsIStringBundleService);
      var stringBundle = stringBundleService.createBundle(
           "chrome://songbird/locale/songbird.properties" );
      string = stringBundle.GetStringFromName(string);
    } catch (e) {
      Component.utils.reportError("L10N: Couldn't localize default media page name.\n")
    }
    
    this.registerPage( string,
                       "chrome://songbird/content/xul/sbLibraryPage.xul",
                       {match: function(mediaList) { return(true); }} 
                       // the default page matches everything
                     );
  },
  
} // MediaListPageManager.prototype

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

