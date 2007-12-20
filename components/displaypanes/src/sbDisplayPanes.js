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
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/components/ArrayConverter.jsm");

const SONGBIRD_DISPLAYPANE_MANAGER_IID = Components.interfaces.sbIDisplayPaneManager;

const RDFURI_ADDON_ROOT               = "urn:songbird:addon:root" 
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";


function SB_NewDataRemote(a,b) {
  return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                    "sbIDataRemote", "init"))(a,b);
}




/**
 * Debug helper that serializes an RDF datasource to the console
 */
function dumpDS(prefix, ds) {
  var outputStream = {
    data: "",
    close : function(){},
    flush : function(){},
    write : function (buffer,count){
      this.data += buffer;
      return count;
    },
    writeFrom : function (stream,count){},
    isNonBlocking: false
  }

  var serializer = Components.classes["@mozilla.org/rdf/xml-serializer;1"]
                           .createInstance(Components.interfaces.nsIRDFXMLSerializer);
  serializer.init(ds);

  serializer.QueryInterface(Components.interfaces.nsIRDFXMLSource);
  serializer.Serialize(outputStream);
  
  outputStream.data.split('\n').forEach( function(line) {
    dump(prefix + line + "\n");
  });
}

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
  
  updateContentInfo: function(aNewTitle, aNewIcon) {
    this.contentTitle = aNewTitle;
    this.contentIcon = aNewIcon;
  },
  
  verify: function() {
    for (var i = 0; i < this.requiredProperties.length; i++) {
      var property = this.requiredProperties[i];
      if (! (typeof(this[property]) == 'string'
               && this[property].length > 0)) 
      {
        throw("Invalid description. '" + property + "' is a required property.");
      }
    }
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
  this._RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                        .getService(Components.interfaces.nsIRDFService);
  this._datasource = this._RDF.GetDataSourceBlocking("rdf:addon-metadata");
  this._manager = Components.classes["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                            .getService(SONGBIRD_DISPLAYPANE_MANAGER_IID);
    
  this._resources = {
    root: this._RDF.GetResource(RDFURI_ADDON_ROOT),
    // Helper to convert a string array into 
    // RDF resources in this object
    addSongbirdResources: function(list){
      for (var i = 0; i < list.length; i++) {
        this[list[i]] = this._RDF.GetResource(PREFIX_NS_SONGBIRD + list[i]);
      }
    },
    _RDF: this._RDF
  };
  
  // Make RDF resources for all properties expected
  // in the displayPanes portion of an install.rdf     
  this._resources.addSongbirdResources(PaneInfo.prototype.requiredProperties);
  this._resources.addSongbirdResources(PaneInfo.prototype.optionalProperties);
  this._resources.addSongbirdResources(
     [ "displayPane",
       "displayPanes" ] );
}
DisplayPaneMetadataReader.prototype = {

  _RDF: null,
  
  _manager: null,

  // Addon metadata rdf datasource
  _datasource: null,

  // Hash of addon metadata RDF resources
  _resources: null,

  /**
   * Populate DisplayPaneManager using addon metadata
   */
  loadPanes: function loadPanes() {
    //debug("DisplayPaneMetadataReader: loadPanes\n");
    
    // Get addon list
    var containerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"]
                                   .getService(Components.interfaces.nsIRDFContainerUtils);
    var container = containerUtils.MakeSeq(this._datasource, this._resources.root);
    var addons = container.GetElements();
    
    // Search all addons for displayPanes metadata
    while (addons.hasMoreElements()) {
      var addon = addons.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      //debug("DisplayPaneMetadataReader.displayPanes: - processing " + addon.Value + "\n");
      try {
      
        if (this._datasource.hasArcOut(addon, this._resources.displayPanes)) {
          var displayPanesTarget = this._datasource.GetTarget(addon, this._resources.displayPanes, true)
                                       .QueryInterface(Components.interfaces.nsIRDFResource);
          
          // Process all pane metadata
          var items = this._datasource.GetTargets(displayPanesTarget, this._resources.displayPane, true)
          while (items.hasMoreElements()) {
            var item = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
            this._processDisplayPane(addon, item);
          }
        }
        
      } catch (e) {
        this._reportErrors("", [  "An error occurred while processing " +
                    "extension " + addon.Value + ".  Exception: " + e  ]);
      }
    }
  },
  
  
  /**
   * Extract pane metadata
   */
  _processDisplayPane: function _processDisplayPane(addon, pane) {
    var info = new PaneInfo();
    
    // Array of error messages
    var errorList = [];
    
    // Fill the description object
    this._populateDescription(pane, info, errorList);

    try {
      info.verify();
      info.defaultWidth = parseInt(info.defaultWidth);
      info.defaultHeight = parseInt(info.defaultHeight);
      info.showOnInstall = info.showOnInstall == "true";
    } catch (e) {
      errorList.push(e.toString());
    }
    
    // If errors were encountered, then do not submit 
    // to the Display Pane Manager
    if (errorList.length > 0) {
      this._reportErrors(
          "Ignoring display pane addon in the install.rdf of extension " +
          addon.Value + ". Message: ", errorList);
      return;
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
   * \brief Populate a description object by looking up requiredProperties and
   *        optionalProperties in a the given rdf source.
   *
   * \param source RDF resource from which to obtain property values
   * \param description Object with requiredProperties and optionalProperties arrays
   * \param errorList An array to which error messages should be added
   */
  _populateDescription: function _populateDescription(source, description, errorList) {

    for (var i = 0; i < description.requiredProperties.length; i++) {
      this._requireProperty(source, description, 
                description.requiredProperties[i], errorList);
    }
    for (var i = 0; i < description.optionalProperties.length; i++) {
      this._copyProperty(source, description, 
                description.optionalProperties[i], errorList);
    }
  },
  
  
  /**
   * \brief Attempts to copy a property from an RDFResource into a
   *        container object and reports an error on failure.
   *
   * \param source RDF resource from which to obtain the value
   * \param description Container object to receive the value
   * \param property String property to be copied
   * \param errorList An array to which error messages should be added
   */
  _requireProperty: function _requireProperty(source, description, property, errorList) {
    this._copyProperty(source, description, property);
    if (description[property] == undefined) 
    {
      errorList.push(
        property + " is a required property."
      );
    }
  },
 
  /**
   * \brief Copies a property from an RDFResource into a container object.
   *
   * \param source RDF resource from which to obtain the value
   * \param description Container object to receive the value
   * \param property String property to be copied
   */
  _copyProperty: function _copyProperty(source, description, property) {
    description[property] = this._getProperty(source, property);
  },


  /**
   * \brief Copies a property from an RDFResource into a container object.
   *
   * \param source RDF resource from which to obtain the value
   * \param description Container object to receive the value
   * \param property String property to be copied
   */  
  _getProperty: function _getProperty(source, property) {
    //debug("DisplayPaneMetadataReader._getProperty " + source.Value + " " + property + "\n");
    var target = this._datasource.GetTarget(source, this._resources[property], true);
    if ( target instanceof Components.interfaces.nsIRDFInt
         || target instanceof Components.interfaces.nsIRDFLiteral 
         || target instanceof Components.interfaces.nsIRDFResource )
    {
      return target.Value;
    }
    return undefined;
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

DisplayPaneManager.prototype.constructor = DisplayPaneManager;

DisplayPaneManager.prototype = {
  classDescription: "Songbird Display Pane Manager Service Interface",
  classID:          Components.ID("{6aef120f-d7ad-414d-a93d-3ac945e64301}"),
  contractID:       "@songbirdnest.com/Songbird/DisplayPane/Manager;1",

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
  get contentList() {
    this.ensureAddonMetadataLoaded();
    return ArrayConverter.enumerator(this._contentList);
  },
  
  /**
   * \see sbIDisplayPaneManager
   */
  get instantiatorsList() {
    for (var i = this._instantiatorsList.length - 1; i >= 0; --i) {
      if (!(this._instantiatorsList[i] instanceof
            Components.interfaces.sbIDisplayPaneInstantiator)) {
        Components.utils.reportError("Warning: found bad instantiator; "+
                                     "possibly via removal from DOM");
        this._instantiatorsList.splice(i, 1);
      }
    }
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

    info.updateContentInfo(aNewContentTitle, aNewContentIcon);
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
    XPCOMUtils.generateQI([SONGBIRD_DISPLAYPANE_MANAGER_IID])
}; // DisplayPaneManager.prototype

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DisplayPaneManager]);
}

