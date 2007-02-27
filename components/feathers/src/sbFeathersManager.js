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
 * \file sbFeathersManager.js
 * \brief Coordinates the loading of feathers (combination of skin and XUL window layout)
 */ 
 
 
//
// TODO:
//  * Add doxygen comments
//  * Make skin switching actually do something
//  * Add function arg assertions (verify description contents)
//  * add onSwitchCompleted, change onSwitchRequested to allow feedback
//  * Make a AddonMetadataReader base class?  With what functionality?
//  * Explore skin/layout versioning issues?
//  * Make metadata reader a startup component
// 
 
const CONTRACTID = "@songbirdnest.com/songbird/feathersmanager;1";
const CLASSNAME = "Songbird Feathers Manager Service Interface";
const CID = Components.ID("{99f24350-a67f-11db-befa-0800200c9a66}");
const IID = Components.interfaces.sbIFeathersManager;


const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root" // TODO Rename
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";



/**
 * /class ArrayEnumerator
 * /brief Converts a js array into an nsISimpleEnumerator
 */
function ArrayEnumerator(array) 
{
  this.data = array;
}
ArrayEnumerator.prototype = {

  index: 0,
  
  getNext: function() {
    return this.data[this.index++];
  },
  
  hasMoreElements: function() {            
    if (this.index < this.data.length)
      return true;
    else
      return false;
  },
  
  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.nsISimpleEnumerator) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
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
 * sbISkinDescription
 */
function SkinDescription() {};
SkinDescription.prototype = {
  // TODO Expand?
  requiredProperties: [ "name", "internalName", "description", "creator" ],
  optionalProperties: [ "previewImageURL" ],
  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbISkinDescription)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

/**
 * sbILayoutDescription
 */
function LayoutDescription() {};
LayoutDescription.prototype = {
  // TODO Expand?
  requiredProperties: [ "name", "url", "description", "creator" ],
  optionalProperties: [ ],

  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbILayoutDescription)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


/**
 * Responsible for reading addon metadata and performing 
 * registration with FeathersManager
 */
function AddonMetadataReader() {
  debug("AddonMetadataReader: ctor\n");
  this._RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                        .getService(Components.interfaces.nsIRDFService);                             
  this._datasource = this._RDF.GetDataSourceBlocking("rdf:addon-metadata");
  this._feathersManager = Components.classes[CONTRACTID].getService(IID);
    
  this._resources = {
    root: this._RDF.GetResource(RDFURI_ITEM_ROOT),
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
  // in the feathers portion of an install.rdf      
  this._resources.addSongbirdResources(SkinDescription.prototype.requiredProperties);
  this._resources.addSongbirdResources(SkinDescription.prototype.optionalProperties);
  this._resources.addSongbirdResources(LayoutDescription.prototype.requiredProperties);
  this._resources.addSongbirdResources(LayoutDescription.prototype.optionalProperties);
  this._resources.addSongbirdResources(
     [ "compatibleSkin", 
       "compatibleLayout", 
       "showChrome",
       "feathers",
       "skin",
       "layout",
       "layoutURL" ] );

}
AddonMetadataReader.prototype = {

  _RDF: null,
  
  _feathersManager: null,

  // Addon metadata rdf datasource
  _datasource: null,

  // Hash of addon metadata RDF resources
  _resources: null,

  /**
   * Populate FeathersManager using addon metadata
   */
  loadFeathers: function loadFeathers() {
    debug("AddonMetadataReader: loadFeathers\n");
    
    // Get addon list
    var containerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"]
                           .getService(Components.interfaces.nsIRDFContainerUtils);
    var container = containerUtils.MakeSeq(this._datasource, this._resources.root);
    var addons = container.GetElements();
    
    // Search all addons for feathers metadata
    while (addons.hasMoreElements()) {
      var addon = addons.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      debug("AddonMetadataReader.loadFeathers: - processing " + addon.Value + "\n");
      try {
      
        if (this._datasource.hasArcOut(addon, this._resources.feathers)) {
          var feathersTarget = this._datasource.GetTarget(addon, this._resources.feathers, true)
                                   .QueryInterface(Components.interfaces.nsIRDFResource);
          
          // Process all skin metadata
          var items = this._datasource.GetTargets(feathersTarget, this._resources.skin, true)
          while (items.hasMoreElements()) {
            var item = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
            this._processSkin(addon, item);
          }

          // Process all layout metadata
          var items = this._datasource.GetTargets(feathersTarget, this._resources.layout, true)
          while (items.hasMoreElements()) {
            var item = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
            this._processLayout(addon, item);
          }
        }
        
      } catch (e) {
        this._reportErrors("", [  "An error occurred while processing " +
                    "extension " + addon.Value + ".  Exception: " + e  ]);
      }
    }
  },
  
  
  /**
   * Extract skin metadata
   */
  _processSkin: function _processSkin(addon, skin) {
    var description = new SkinDescription();
    
    // Array of error messages
    var errorList = [];
    
    // Fill the description object
    this._populateDescription(skin, description, errorList);
    
    // If errors were encountered, then do not submit 
    // to the Feathers Manager
    if (errorList.length > 0) {
      this._reportErrors(
          "Ignoring skin addon in the install.rdf of extension " +
          addon.Value + ". Message: ", errorList);
      return;
    }
    
    // Submit description
    this._feathersManager.registerSkin(description);
    debug("AddonMetadataReader: registered skin " + description.internalName
            + " from addon " + addon.Value + " \n");
    
    // Get compatibility information
    var identifiers, showChromeInstructions;
    [identifiers, showChromeInstructions] =
        this._getCompatibility(addon, skin, "compatibleLayout", "layoutURL", errorList);

    // Report errors
    if (errorList.length > 0) {
      this._reportErrors(
          "Error finding compatibility information for skin " +
          description.name + " in the install.rdf " +
          "of extension " + addon.Value + ". Message: ", errorList);
    }
     
    // Assert compatibility    
    for (var i = 0; i < identifiers.length; i++) {
      this._feathersManager.assertCompatibility(
               identifiers[i],
               description.internalName, 
               showChromeInstructions[i]);
    }
  },


  /**
   * Extract layout metadata
   */
  _processLayout: function _processLayout(addon, layout) {
    var description = new LayoutDescription();
    
    // Array of error messages
    var errorList = [];
    
    // Fill the description object
    this._populateDescription(layout, description, errorList);
    
    // If errors were encountered, then do not submit 
    // to the Feathers Manager
    if (errorList.length > 0) {
      this._reportErrors(
          "Ignoring layout addon in the install.rdf of extension " +
          addon.Value + ". Message: ", errorList);
      return;
    }
    
    // Submit description
    this._feathersManager.registerLayout(description);
    debug("AddonMetadataReader: registered layout addon " + addon.Value + "\n");    
    
    // Get compatibility information
    var identifiers, showChromeInstructions;
    [identifiers, showChromeInstructions] =
        this._getCompatibility(addon, layout, "compatibleSkin", "internalName", errorList);

    // Report errors
    if (errorList.length > 0) {
      this._reportErrors(
          "Error finding compatibility information for layout " +
          description.name + " in the install.rdf " +
          "of extension " + addon.Value + ". Message: ", errorList);
    }
    
    // Assert compatibility      
    for (var i = 0; i < identifiers.length; i++) {
      this._feathersManager.assertCompatibility(
               description.layoutURL,
               identifiers[i], 
               showChromeInstructions[i]);
    }
  
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
    debug("AddonMetadataReader._getProperty " + source.Value + " " + property + "\n");
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
   * \brief Extracts compatibility information for a feathers item into 
   *        an array of identifiers and matching array of booleans 
   *        indicating whether chrome should be shown
   *
   * \param addon RDF resource from the addon description
   * \param source RDF resource for the feathers item
   * \param resourceName Container object to receive the value
   * \param idProperty String property to be copied
   * \param errorList An array to which error messages should be added
   * \return [array of identifiers, array of showChrome bools]
   */
  _getCompatibility: function _getCompatibility(addon, source, resourceName, idProperty, errorList) {
    // List of internalName or layoutURL identifiers
    var identifiers = [];
    // Matching list of showChrome hints
    var showChromeInstructions = [];
    
    // Look at all compatibility rules of type resourceName
    var targets = this._datasource.GetTargets(source, this._resources[resourceName], true);
    while (targets.hasMoreElements()) {
      var target = targets.getNext();
      
      // Target must be a resource / contain a description
      if (! (target instanceof Components.interfaces.nsIRDFResource)) {
        
        errorList.push("The install.rdf for " + addon.Value 
              + " is providing an incomplete " + resourceName 
              + " section.");
              
        // Skip this section
        continue;        
      }
      target = target.QueryInterface(Components.interfaces.nsIRDFResource);
      
      // Get the identifier for the compatible feather item
      var id = this._getProperty(target, idProperty);
      
      // Must provide an identifier
      if (id == undefined) {
        errorList.push("Extension " + addon.Value 
              + " must provide " + idProperty 
              + " in " + resourceName + " descriptions");
              
        // Skip this rule since it is incomplete
        continue;
      }
      
      // Should chrome be shown with this id?
      var showChrome = this._getProperty(target, "showChrome") == "true";
      
      // Store compatibility rule
      identifiers.push(id);
      showChromeInstructions.push(showChrome);
    }
    
    debug("AddonMetadataReader: found " + identifiers.length + " " 
          + resourceName + " rule(s) for addon " 
          + addon.Value + "\n");

    return [identifiers, showChromeInstructions];
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
      consoleService.logStringMessage("Feathers Metadata Reader: " 
                                       + contextMessage + errorList[i]);
      dump("FeathersMetadataReader: " + contextMessage + errorList[i] + "\n");
    }
  }
}








/**
 * /class FeathersManager
 * /brief Coordinates the loading of feathers
 *
 * Acts as a registry for skins and layout (known as feathers)
 * and manages compatibility and selection.
 */
function FeathersManager() {

  var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  // We should wait until the profile has been loaded to start
  os.addObserver(this, "profile-after-change", false);
  // We need to unhook things on shutdown
  os.addObserver(this, "xpcom-shutdown", false);
  
  this._skins = {};
  this._layouts = {};
  this._mappings = {};
  this._listeners = [];
};
FeathersManager.prototype = {
  constructor: FeathersManager,
  
  _currentSkinName: null,
  _currentLayoutURL: null,
  
  // Hash of skin descriptions keyed by internalName (e.g. classic/1.0)
  _skins: null,
  
  // Hash of layout descriptions keyed by URL
  _layouts: null,
  
  
  // Hash of layout URL to hash of compatible skin internalNames, pointing to 
  // showChrome value.  
  //
  // eg
  // {  
  //     mainwin.xul: {
  //       blueskin: true,
  //       redskin: false,
  //     }
  // }
  //
  // Compatibility is determined by whether or not a internalName
  // key is *defined* in the hash, not the actual value it points to.
  // In the above example false means "don't show chrome"
  _mappings: null,
  
  
  // Array of sbIFeathersChangeListeners
  _listeners: null,

  _layoutCount: 0,
  _skinCount: 0,
  

  _init: function init() {
    debug("FeathersManager: _init\n");
    // Hmm, don't care?
    
    // TODO: move outside of FM?
    var metadataReader = new AddonMetadataReader();
    metadataReader.loadFeathers();
  },
  
  _deinit: function deinit() {
    debug("FeathersManager: _deinit\n");
    this._skins = null;
    this._layouts = null;
    this._mappings = null;
    this._listeners = null;
  },
  
  
  get currentSkinName() {
    return this._currentSkinName;
  },
  
  get currentLayoutURL() {
    return this._currentLayoutURL;
  },
  
  get skinCount() {
    return this._skinCount;
  },
  
  get layoutCount() {
    return this._layoutCount;
  },

  getSkinDescriptions: function getSkinDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._skins[key] for (key in this._skins)] );
  },

  getLayoutDescriptions: function getLayoutDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._layouts[key] for (key in this._layouts)] );
  },
  
  
  
  registerSkin: function registerSkin(skinDesc) {
    // TODO: Verify desc
    
    if (this._skins[skinDesc.internalName] == null) {
      this._skinCount++;
    }
    this._skins[skinDesc.internalName] = skinDesc;
    
    // Notify observers
    this._onUpdate();
  },

  unregisterSkin: function unregisterSkin(skinDesc) {
    if (this._skins[skinDesc.internalName]) {
      delete this._skins[skinDesc.internalName];
      this._skinCount--;
      
      // Notify observers
      this._onUpdate();
    }
  },

  getSkinDescription: function getSkinDescription(internalName) {
    return this._skins[internalName];
  },
  
  
  
  registerLayout: function registerLayout(layoutDesc) {
    // TODO: Verify desc
   
    if (this._layouts[layoutDesc.url] == null) {
      this._layoutCount++;
    }
    this._layouts[layoutDesc.url] = layoutDesc;
    
    // Notify observers
    this._onUpdate();
  },

  unregisterLayout: function unregisterLayout(layoutDesc) {
    if (this._layouts[layoutDesc.url]) {
      delete this._layouts[layoutDesc.url];
      this._layoutCount--;
      
      // Notify observers
      this._onUpdate();  
    }  
  },
    
  getLayoutDescription: function getLayoutDescription(url) {
    return this._layouts[url];
  }, 
  


  assertCompatibility: 
  function assertCompatibility(layoutURL, internalName, showChrome) {
    // TODO verify

    if (this._mappings[layoutURL] == null) {
      this._mappings[layoutURL] = {};
    }
    this._mappings[layoutURL][internalName] = showChrome;
    
    // Notify observers
    this._onUpdate();    
  },

  unassertCompatibility: function unassertCompatibility(layoutURL, internalName) {
    if (this._mappings[layoutURL]) {
      delete this._mappings[layoutURL][internalName];
      
      // Notify observers
      this._onUpdate();  
    }  
  },
  


  isChromeEnabled: function isChromeEnabled(layoutURL, internalName) {
    if (this._mappings[layoutURL]) {
      return this._mappings[layoutURL][internalName] == true;
    }
   
    return false; 
  },



  getSkinsForLayout: function getSkinsForLayout(layoutURL) {
    var skins = [];
    
    // Find skin descriptions that are compatible with the given layout.
    if (this._mappings[layoutURL]) {
      for (internalName in this._mappings[layoutURL]) {
        var desc = this.getSkinDescription(internalName);
        if (desc) {
          skins.push(desc);
        }
      }
    }   
    return new ArrayEnumerator( skins );
  },



  switchFeathers: function switchFeathers(layoutURL, internalName) {
    layoutDescription = this.getLayoutDescription(layoutURL);
    skinDescription = this.getSkinDescription(internalName);
    
    // Make sure we know about the requested skin and layout
    if (layoutDescription == null || skinDescription == null) {
      throw("Unknown layout/skin passed to switchFeathers");
    }
    
    // Check compatibility.
    // True/false refer to the showChrome value, so check for undefined
    // to determine compatibility.
    if (this._mappings[layoutURL][internalName] === undefined) {
      throw("Skin [" + internalName + "] and Layout [" + layoutURL +
            " are not compatible");
    } 
    
    // Notify that a select is about to occur
    this._onSelect(layoutDescription, skinDescription);
    
    // TODO!
    // Can we get away with killing the main window, or do we have to completely restart?
    // Modify dataremotes and keep a copy of the previous values.
  },



  addListener: function addListener(listener) {
    this._listeners.push(listener);
  },
  
  
  removeListener: function removeListener(listener) {
    var index = this._listeners.indexOf(listener);
    if (index > -1) {
      this._listeners.splice(index,1);
    }
  },


  _onUpdate: function onUpdate() {
    this._listeners.forEach( function (listener) {
      listener.onFeathersUpdate();
    });
  },


  _onSelect: function onSelect(layoutDesc, skinDesc) {
    // Verify args
    layoutDesc = layoutDesc.QueryInterface(Components.interfaces.sbILayoutDescription);
    skinDesc = skinDesc.QueryInterface(Components.interfaces.sbISkinDescription);
    
    // Broadcast notification
    this._listeners.forEach( function (listener) {
      listener.onFeathersSelectRequest(layoutDesc, skinDesc);
    });
  },

      

  // watch for XRE startup and shutdown messages 
  observe: function(subject, topic, data) {
    var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
    switch (topic) {
    case "profile-after-change":
      os.removeObserver(this, "profile-after-change");
      
      // Preferences are initialized, ready to start the service
      this._init();
      break;
    case "xpcom-shutdown":
      os.removeObserver(this, "xpcom-shutdown");
      this._deinit();
      break;
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // FeathersManager.prototype





/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this._objects) {
      var obj = this._objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Components.interfaces.nsICategoryManager);
    categoryManager.addCategoryEntry("app-startup", this._objects.feathersmanager.className,
                                    "service," + this._objects.feathersmanager.contractID, 
                                    true, true, null);
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  _makeFactory: #1= function(ctor) {
    function ci(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new ctor()).QueryInterface(iid);
    } 
    return { createInstance: ci };
  },
  
  _objects: {
    // The FeathersManager Component
    feathersmanager:     { CID        : CID,
                           contractID : CONTRACTID,
                           className  : CLASSNAME,
                           factory    : #1#(FeathersManager)
                         },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule


