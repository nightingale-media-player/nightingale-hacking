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

const CHROME_PREFIX = "chrome://"

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/RDFHelper.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

/**
 * sbIContentPaneInfo
 */
 
function PaneInfo() {};

PaneInfo.prototype = {
  requiredProperties: [ "contentUrl", 
                        "contentTitle",
                        "contentIcon",
                        "suggestedContentGroups",
                        "defaultWidth", 
                        "defaultHeight" ],
  optionalProperties: [ "showOnInstall" ],
    
  verify: function() {
    var errorList = [];
    for (var i = 0; i < this.requiredProperties.length; i++) {
      var property = this.requiredProperties[i];
      if (! (typeof(this[property]) == 'string'
               && this[property].length > 0)) 
      {
        errorList.push("Invalid description. '" + property + "' is a required property.");
      }
    }
    try {
      this.defaultWidth = parseInt(this.defaultWidth);
      this.defaultHeight = parseInt(this.defaultHeight);
      this.showOnInstall = this.showOnInstall == "true";
    } catch (e) {
      errorList.push(e.toString());
    }
    return(errorList);
  },
  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbIDisplayPaneContentInfo)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

/**
 * /class DisplayPaneMetadataReader
 * Responsible for reading addon metadata and performing 
 * registration with DisplayPaneManager
 */
function DisplayPaneMetadataReader() {
  //debug("DisplayPaneMetadataReader: ctor\n");
  this._manager = Components.classes["@getnightingale.com/Nightingale/DisplayPane/Manager;1"]
                            .getService(Components.interfaces.sbIDisplayPaneManager);
}
DisplayPaneMetadataReader.prototype = {
  _manager: null,

  /**
   * Populate DisplayPaneManager using addon metadata
   */
  loadPanes: function loadPanes() {
    //debug("DisplayPaneMetadataReader: loadPanes\n");
    var addons = RDFHelper.help("rdf:addon-metadata", "urn:nightingale:addon:root", RDFHelper.DEFAULT_RDF_NAMESPACES);
    
    for (var i = 0; i < addons.length; i++) {
      // skip addons with no panes.
      var panes;
      if (addons[i].displayPanes) {
        // TODO: remove this some time post 0.5 and before 1.0
        Components.utils.reportError(
          "DisplayPanes: Use of the <displayPanes> element in install.rdf " +
          "is deprecated. Remove that element and leave the contents as-is."
        );
        panes = addons[i].displayPanes[0].displayPane;
      }
      else {
        panes = addons[i].displayPane;
      }
      
      if (!panes) {
        // no display panes in this addon.
        continue;
      }
      
      try {
        for (var j = 0; j < panes.length; j++) {
          this._registerDisplayPane(addons[i], panes[j]) 
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
  _registerDisplayPane: function _registerDisplayPane(addon, pane) {
    // create and validate our pane info
    var info = new PaneInfo();
    for (property in pane) {
      if (pane[property])
       info[property] = pane[property][0];
    }
    var errorList = info.verify();
    
    // If errors were encountered, then do not submit 
    // to the Display Pane Manager
    if (errorList.length > 0) {
      this._reportErrors(
          "Ignoring display pane addon in the install.rdf of extension " +
          addon.Value + " due to these error(s):\n", errorList);
      return;
    }

    // Resolve any localised display pane contentTitle to their actual strings
    if (info.contentTitle.substr(0,CHROME_PREFIX.length) == CHROME_PREFIX)
    {
      var contentTitle = SBString("displaypanes.contenttitle.unnamed");
      var split = info.contentTitle.split("#", 2);
      if (split.length == 2) {
        var bundle = new SBStringBundle(split[0]);
        contentTitle = bundle.get(split[1], contentTitle);
      }
      info.contentTitle = contentTitle;
    }     
    // Submit description
    this._manager.registerContent( info.contentUrl, 
                                   info.contentTitle,
                                   info.contentIcon,
                                   info.defaultWidth,
                                   info.defaultHeight,
                                   info.suggestedContentGroups,
                                   info.showOnInstall );
    
    //debug("DisplayPaneMetadataReader: registered pane " + info.contentTitle
    //        + " from addon " + addon.Value + " \n");
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
      consoleService.logStringMessage("Display Pane Metadata Reader: " 
                                       + contextMessage + errorList[i]);
      dump("DisplayPaneMetadataReader: " + contextMessage + errorList[i] + "\n");
    }
  }
}

/**
 * /class DisplayPaneManager
 * /brief Coordinates display pane content
 *
 * Acts as a registry for display panes and available content.
 *
 * \sa sbIDisplayPaneManager
 */
function DisplayPaneManager() {
}

DisplayPaneManager.prototype = {
  classDescription: "Nightingale Display Pane Manager Service Interface",
  classID:          Components.ID("{6aef120f-d7ad-414d-a93d-3ac945e64301}"),
  contractID:       "@getnightingale.com/Nightingale/DisplayPane/Manager;1",
  constructor:      DisplayPaneManager,

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },

  _contentList: [],
  _instantiatorsList: [],
  _delayedInstantiations: [],
  _listenersList: [],
  
  _addonMetadataLoaded: false,

  _getString: function(aName, aDefault) {
    if (!this._stringbundle) {
      var src = "chrome://branding/locale/brand.properties";
      var stringBundleService = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                  .getService(Components.interfaces.nsIStringBundleService);
      this._stringbundle = stringBundleService.createBundle(src);
    }
        
    try {
      return this._stringbundle.GetStringFromName(aName);
    }
    catch(e) {
      return aDefault;
    }

  },
  
  _defaultPaneInfo: null,
  
  _cleanupInstantiatorsList: function DPM_cleanupInstantiatorsList() {
    for (var i = this._instantiatorsList.length - 1; i >= 0; --i) {
      if (!(this._instantiatorsList[i] instanceof
            Components.interfaces.sbIDisplayPaneInstantiator)) {
        Components.utils.reportError("Warning: found bad instantiator; "+
                                     "possibly via removal from DOM");
        this._instantiatorsList.splice(i, 1);
      }
    }
  },
  
  get defaultPaneInfo() {
    if (!this._defaultPaneInfo) {
      var paneInfo = {
        contentUrl: this._getString("displaypane.default.url",  "chrome://nightingale/content/xul/defaultDisplayPane.xul"),
        contentTitle: null, // automatically set by host depending on where the default pane is instantiated
        contentIcon: this._getString("displaypane.default.icon",  "chrome://branding/content/branding.ico"),
        defaultWidth: this._getString("displaypane.default.width",  "150"),
        defaultHeight: this._getString("displaypane.default.height",  "75"),
        suggestedContentGroups: this._getString("displaypane.default.groups",  "")
      };
      this._defaultPaneInfo = paneInfo;
    }
    return this._defaultPaneInfo;
  },

  /**
   * Make sure that we've read display pane registration
   * metadata from all extension install.rdf files.
   */
  ensureAddonMetadataLoaded: function() {
    if (this._addonMetadataLoaded) {
      return;
    }
    this._addonMetadataLoaded = true;
    
    // Load the addon metadata
    var metadataReader = new DisplayPaneMetadataReader();
    metadataReader.loadPanes();
  },
  

  /**
   * given a list of pane parameters, return a new sbIDisplayPaneContentInfo
   */
  makePaneInfo: function(aContentUrl,
                         aContentTitle,
                         aContentIcon,
                         aSuggestedContentGroups,
                         aDefaultWidth,
                         aDefaultHeight) {
    var paneInfo = new PaneInfo();
    paneInfo.contentUrl = aContentUrl;
    paneInfo.contentTitle = aContentTitle;
    paneInfo.contentIcon = aContentIcon;
    paneInfo.suggestedContentGroups = aSuggestedContentGroups;
    paneInfo.defaultWidth = aDefaultWidth;
    paneInfo.defaultHeight = aDefaultHeight;
    
    return paneInfo;
  },

  /**
   * \see sbIDisplayPaneManager
   */
  getPaneInfo: function(aContentUrl) {
    this.ensureAddonMetadataLoaded();
  
    for each (var pane in this._contentList) {
      ////debug("PANE: " + pane.contentTitle  + " XXX " + aContentUrl + "\n\n");
      if (pane.contentUrl == aContentUrl) 
        return pane;
    }
    return null;
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  getInstantiatorForWindow:
  function sbDisplayPaneMgr_getInstantiatorForWindow(aWindow) {
    this._cleanupInstantiatorsList();
    for each (var instantiator in this._instantiatorsList) {
      if (instantiator.contentWindow === aWindow) {
        return instantiator;
      }
    }
    return null;
  },

  /**
   * \see sbIDisplayPaneManager
   */
  get contentList() {
    this.ensureAddonMetadataLoaded();
    return ArrayConverter.enumerator(this._contentList);
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  get instantiatorsList() {
    this._cleanupInstantiatorsList();
    return ArrayConverter.enumerator(this._instantiatorsList);
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  registerContent: function(aContentUrl,
                            aContentTitle,
                            aContentIcon,
                            aDefaultWidth,
                            aDefaultHeight,
                            aSuggestedContentGroups,
                            aAutoShow) {
                            
    ////debug("REGISTER: " + aContentUrl + "\n");
    
    var info = this.getPaneInfo(aContentUrl);
    if (info) {
      throw Components.results.NS_ERROR_ALREADY_INITIALIZED;
    }
    info = this.makePaneInfo(aContentUrl,
                             aContentTitle,
                             aContentIcon,
                             aSuggestedContentGroups,
                             aDefaultWidth,
                             aDefaultHeight);
    this._contentList.push(info);
    for each (var listener in this._listenersList) {
      listener.onRegisterContent(info);
    }
    // if we have never seen this pane, show it in its prefered group
    var SB_NewDataRemote = Components.Constructor("@getnightingale.com/Nightingale/DataRemote;1",
                                                  "sbIDataRemote",
                                                  "init");
    var known = SB_NewDataRemote("displaypane.known." + aContentUrl, null);
    if (!known.boolValue) {
      if (aAutoShow) {
        if (!this.tryInstantiation(info)) {
          this._delayedInstantiations.push(info);
        }
      }
      // remember we've seen this pane, let the pane hosts reload on their own if they need to
      known.boolValue = true;
    }
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  unregisterContent: function(aContentUrl) {
    for (var contentIndex = 0; contentIndex < this._contentList.length; contentIndex++) {
      if (this._contentList[contentIndex].contentUrl != aContentUrl) {
        continue;
      }

      // any instantiator currently hosting this url should be emptied
      for each (var instantiator in this._instantiatorsList) {
        if (instantiator.contentUrl == aContentUrl) {
          instantiator.hide();
        }
      }
      // also remove it from the delayed instantiation list
      for (instantiatorIndex = this._delayedInstantiations.length - 1; instantiatorIndex >= 0; --instantiatorIndex) {
        if (this._delayedInstantiations[instantiatorIndex].contentUrl == aContentUrl) {
          this._delayedInstantiations.splice(instantiatorIndex, 1);
        }
      }

      var [info] = this._contentList.splice(contentIndex, 1);
      
      for each (var listener in this._listenersList) {
        listener.onUnregisterContent(info);
      }
      return;
    }
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  registerInstantiator: function(aInstantiator) {
    this.ensureAddonMetadataLoaded();
    
    if (this._instantiatorsList.indexOf(aInstantiator) > -1) {
      Components.utils.reportError("Attempt to re-register instantiator ignored\n" +
                                   (new Error()).stack);
      return;
    }
    this._instantiatorsList.push(aInstantiator);
    for each (var listener in this._listenersList) {
      listener.onRegisterInstantiator(aInstantiator);
    }
    this.processDelayedInstantiations();
  },

  /**
   * \see sbIDisplayPaneManager
   */
  unregisterInstantiator: function(aInstantiator) {
    var index = this._instantiatorsList.indexOf(aInstantiator);
    if (index < 0) {
      // not found
      return;
    }
    this._instantiatorsList.splice(index, 1);
    for each (var listener in this._listenersList) {
      listener.onUnregisterInstantiator(aInstantiator);
    }
  },
  
  /**
   * given a content group list (from a sbIDisplayPaneContentInfo),
   * return the the first instantiator that matches the earliest possible
   * content group
   */
  getFirstInstantiatorForGroupList: function(aContentGroupList) {
    var groups = aContentGroupList.toUpperCase().split(";");
    for each (var group in groups) {
      for each (var instantiator in this._instantiatorsList) {
        if (instantiator.contentGroup.toUpperCase() == group) {
          return instantiator;
        }
      }
    }
    return null;
  },
  
  processDelayedInstantiations: function() {
    var table = [];
    for each (var info in this._delayedInstantiations) {
      if (!this.isValidPane(info) || this.tryInstantiation(info)) {
        continue;
      }
      table.push(info);
    }
    this._delayedInstantiations = table;
  },
  
  tryInstantiation: function(info) {
    var instantiator = this.getFirstInstantiatorForGroupList(info.suggestedContentGroups);
    if (instantiator) {
      instantiator.loadContent(info);
      return true;
    }
    return false;
  },
  
  isValidPane: function(aPane) {
    this.ensureAddonMetadataLoaded();
    for each (var pane in this._contentList) {
      if (pane == aPane) return true;
    }
    return false;
  },

  showPane: function(aContentUrl) {
    for each (var instantiator in this._instantiatorsList) {
      if (instantiator.contentUrl == aContentUrl) {
        // we already have a pane with this content
        instantiator.collapsed = false;
        return;
      }
    }
    var info = this.getPaneInfo(aContentUrl);
    if (info) {
      if (!this.tryInstantiation(info)) {
        this._delayedInstantiations.push(info);
      }
    } else {
      throw new Error("Content URL was not found in list of registered panes");
    }
  },
  
  addListener: function(aListener) {
    this._listenersList.push(aListener);
  },
  
  removeListener: function(aListener) {
    var index = this._listenersList.indexOf(aListener);
    if (index > -1)
      this._listenersList.splice(index, 1);
  },
  
  updateContentInfo: function(aContentUrl, aNewContentTitle, aNewContentIcon) {
    var info = this.getPaneInfo(aContentUrl);
    if (!info) {
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }

    info.contentTitle = aNewContentTitle;
    info.contentIcon  = aNewContentIcon;
    
    // change the live title for every instance of this content
    for each (var instantiator in this._instantiatorsList) {
      if (instantiator.contentUrl == aContentUrl) {
        instantiator.contentTitle = aNewContentTitle;
        instantiator.contentIcon = aNewContentIcon;
      }
    }
    for each (var listener in this._listenersList) {
      listener.onPaneInfoChanged(info);
    }
  },

  /**
   * \see nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([Components.interfaces.sbIDisplayPaneManager])
}; // DisplayPaneManager.prototype

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DisplayPaneManager]);
}

