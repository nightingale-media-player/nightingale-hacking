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
//  * add onSwitchCompleted, change onSwitchRequested to allow feedback
//  * Explore skin/layout versioning issues?
// 
 
const CONTRACTID = "@songbirdnest.com/songbird/feathersmanager;1";
const CLASSNAME = "Songbird Feathers Manager Service Interface";
const CID = Components.ID("{99f24350-a67f-11db-befa-0800200c9a66}");
const IID = Components.interfaces.sbIFeathersManager;


const RDFURI_ADDON_ROOT               = "urn:songbird:addon:root" 
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";

// Fallback layouts/skin, used by previousSkinName and previousLayoutURL
// Changes to the shipped feathers must be reflected here
// and in test_feathersManager.js
const DEFAULT_MAIN_LAYOUT_URL         = "chrome://songbird/content/feathers/basic-layouts/xul/mainwin.xul";
const DEFAULT_SECONDARY_LAYOUT_URL    = "chrome://songbird/content/feathers/basic-layouts/xul/miniplayer.xul";
const DEFAULT_SKIN_NAME               = "rubberducky/0.2";

const WINDOWTYPE_SONGBIRD_PLAYER      = "Songbird:Main";
const WINDOWTYPE_SONGBIRD_CORE        = "Songbird:Core";




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
  requiredProperties: [ "name", "internalName" ],
  optionalProperties: [ ],
  
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
  requiredProperties: [ "name", "url" ],
  optionalProperties: [ ],
  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbILayoutDescription)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


/**
 * Static function that verifies the contents of the given description
 *
 * Example:
 *   try {
 *     LayoutDescription.verify(layout);
 *   } catch (e) {
 *     reportError(e);
 *   }
 *
 */
LayoutDescription.verify = SkinDescription.verify = function( description ) 
{
  for (var i = 0; i < this.prototype.requiredProperties.length; i++) {
    var property = this.prototype.requiredProperties[i];
    if (! (typeof(description[property]) == 'string'
             && description[property].length > 0)) 
    {
      throw("Invalid description. '" + property + "' is a required property.");
    }
  }
}






/**
 * /class AddonMetadataReader
 * Responsible for reading addon metadata and performing 
 * registration with FeathersManager
 */
function AddonMetadataReader() {
  //debug("AddonMetadataReader: ctor\n");
  this._RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                        .getService(Components.interfaces.nsIRDFService);
  this._datasource = this._RDF.GetDataSourceBlocking("rdf:addon-metadata");
  this._feathersManager = Components.classes[CONTRACTID].getService(IID);
    
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
    //debug("AddonMetadataReader: loadFeathers\n");
    
    // Get addon list
    var containerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"]
                           .getService(Components.interfaces.nsIRDFContainerUtils);
    var container = containerUtils.MakeSeq(this._datasource, this._resources.root);
    var addons = container.GetElements();
    
    // Search all addons for feathers metadata
    while (addons.hasMoreElements()) {
      var addon = addons.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      //debug("AddonMetadataReader.loadFeathers: - processing " + addon.Value + "\n");
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
    
    try {
      SkinDescription.verify(description);
    } catch (e) {
      errorList.push(e.toString());
    }
    
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
    //debug("AddonMetadataReader: registered skin " + description.internalName
    //        + " from addon " + addon.Value + " \n");
    
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
    
    try {
      LayoutDescription.verify(description);
    } catch (e) {
      errorList.push(e.toString());
    }
    
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
    //debug("AddonMetadataReader: registered layout " + description.name +
    //     " from addon " + addon.Value + "\n");    
    
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
               description.url,
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
    //debug("AddonMetadataReader._getProperty " + source.Value + " " + property + "\n");
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
    
    //debug("AddonMetadataReader: found " + identifiers.length + " " 
    //      + resourceName + " rule(s) for addon " 
    //      + addon.Value + "\n");

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
 *
 * \sa sbIFeathersManager
 */
function FeathersManager() {

  var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  // We need to unhook things on shutdown
  os.addObserver(this, "quit-application", false);
  
  this._skins = {};
  this._layouts = {};
  this._mappings = {};
  this._listeners = [];
};
FeathersManager.prototype = {
  constructor: FeathersManager,
  
  _layoutDataRemote: null,
  _skinDataRemote: null,

  _previousLayoutDataRemote: null,
  _previousSkinDataRemote: null,
  
  _showChromeDataRemote: null,

  
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
  
  _initialized: false,
  

  /**
   * Initializes dataremotes and triggers the AddonMetadataReader
   * to explore installed extensions and register any feathers.
   *
   * Note that this function is not run until a get method is called
   * on the feathers manager.  This is to defer loading the metadata
   * as long as possible and avoid impacting startup time.
   * 
   */
  _init: function init() {
  
    // If already initialized do nothing
    if (this._initialized) {
      return;
    }
    
    // Make dataremotes to persist feathers settings
    var createDataRemote =  new Components.Constructor(
                  "@songbirdnest.com/Songbird/DataRemote;1",
                  Components.interfaces.sbIDataRemote, "init");

    this._layoutDataRemote = createDataRemote("feathers.selectedLayout", null);
    this._skinDataRemote = createDataRemote("selectedSkin", "general.skins.");
    
    this._previousLayoutDataRemote = createDataRemote("feathers.previousLayout", null);
    this._previousSkinDataRemote = createDataRemote("feathers.previousSkin", null);
    
    // TODO: Rename accessibility.enabled?
    this._showChromeDataRemote = createDataRemote("accessibility.enabled", null);
    
    // Load the feathers metadata
    var metadataReader = new AddonMetadataReader();
    metadataReader.loadFeathers();
    
    // If no layout url has been specified, set to default
    if (this._layoutDataRemote.stringValue == "") {
      this._layoutDataRemote.stringValue = DEFAULT_MAIN_LAYOUT_URL;
    }
    
    this._initialized = true;
  },
  
  /**
   * Called on xpcom-shutdown
   */
  _deinit: function deinit() {
    this._skins = null;
    this._layouts = null;
    this._mappings = null;
    this._listeners = null;
    this._layoutDataRemote = null;
    this._skinDataRemote = null;
    this._previousLayoutDataRemote = null;
    this._previousSkinDataRemote = null;
    this._showChromeDataRemote = null;
  },
    
  /**
   * \sa sbIFeathersManager
   */
  get currentSkinName() {
    this._init();
    return this._skinDataRemote.stringValue;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get currentLayoutURL() {
    this._init();
    return this._layoutDataRemote.stringValue;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get previousSkinName() {
    this._init();
    
    // Test to make sure the previous skin exists
    var skin = this.getSkinDescription(this._previousSkinDataRemote.stringValue);
    
    // If the skin exists, then return the skin name
    if (skin) {
      return skin.internalName;
    }
    
    // Otherwise, return the default skin
    return DEFAULT_SKIN_NAME;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get previousLayoutURL() {
    this._init();
    
    // Test to make sure the previous layout exists
    var layout = this.getLayoutDescription(this._previousLayoutDataRemote.stringValue);
    
    // If the layout exists, then return the url/identifier
    if (layout) {
      return layout.url;
    }
    
    // Otherwise, return the default 
    
    // Use the main default unless it is currently
    // active. This way if the user reverts for the
    // first time they will end up in the miniplayer.
    var layoutURL = DEFAULT_MAIN_LAYOUT_URL;
    if (this.currentLayoutURL == layoutURL) {
      layoutURL = DEFAULT_SECONDARY_LAYOUT_URL;
    }
    
    return layoutURL;    
  },
 
 
  /**
   * \sa sbIFeathersManager
   */ 
  get skinCount() {
    this._init();
    return this._skinCount;
  },
  
  /**
   * \sa sbIFeathersManager
   */  
  get layoutCount() {
    this._init();
    return this._layoutCount;
  },


  /**
   * \sa sbIFeathersManager
   */
  getSkinDescriptions: function getSkinDescriptions() {
    this._init();      
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._skins[key] for (key in this._skins)] );
  },

  /**
   * \sa sbIFeathersManager
   */
  getLayoutDescriptions: function getLayoutDescriptions() {
    this._init();        
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._layouts[key] for (key in this._layouts)] );
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  registerSkin: function registerSkin(skinDesc) {
    SkinDescription.verify(skinDesc);
    
    if (this._skins[skinDesc.internalName] == null) {
      this._skinCount++;
    }
    this._skins[skinDesc.internalName] = skinDesc;
    
    // Notify observers
    this._onUpdate();
  },

  /**
   * \sa sbIFeathersManager
   */
  unregisterSkin: function unregisterSkin(skinDesc) {
    if (this._skins[skinDesc.internalName]) {
      delete this._skins[skinDesc.internalName];
      this._skinCount--;
      
      // Notify observers
      this._onUpdate();
    }
  },

  /**
   * \sa sbIFeathersManager
   */
  getSkinDescription: function getSkinDescription(internalName) {
    this._init();
    return this._skins[internalName];
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  registerLayout: function registerLayout(layoutDesc) {
    LayoutDescription.verify(layoutDesc);
   
    if (this._layouts[layoutDesc.url] == null) {
      this._layoutCount++;
    }
    this._layouts[layoutDesc.url] = layoutDesc;
    
    // Notify observers
    this._onUpdate();
  },

  /**
   * \sa sbIFeathersManager
   */
  unregisterLayout: function unregisterLayout(layoutDesc) {
    if (this._layouts[layoutDesc.url]) {
      delete this._layouts[layoutDesc.url];
      this._layoutCount--;
      
      // Notify observers
      this._onUpdate();  
    }  
  },
    
  /**
   * \sa sbIFeathersManager
   */    
  getLayoutDescription: function getLayoutDescription(url) {
    this._init();
    return this._layouts[url];
  }, 

  
  /**
   * \sa sbIFeathersManager
   */
  assertCompatibility: 
  function assertCompatibility(layoutURL, internalName, showChrome) {
    if (! (typeof(layoutURL) == "string" && typeof(internalName) == 'string')) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._mappings[layoutURL] == null) {
      this._mappings[layoutURL] = {};
    }
    this._mappings[layoutURL][internalName] = showChrome;
    
    // Notify observers
    this._onUpdate();
  },

  /**
   * \sa sbIFeathersManager
   */
  unassertCompatibility: function unassertCompatibility(layoutURL, internalName) {
    if (this._mappings[layoutURL]) {
      delete this._mappings[layoutURL][internalName];
      
      // Notify observers
      this._onUpdate();
    }  
  },
  

  /**
   * \sa sbIFeathersManager
   */
  isChromeEnabled: function isChromeEnabled(layoutURL, internalName) {
    this._init();
    if (this._mappings[layoutURL]) {
      return this._mappings[layoutURL][internalName] == true;
    }
   
    return false; 
  },


  /**
   * \sa sbIFeathersManager
   */
  getSkinsForLayout: function getSkinsForLayout(layoutURL) {
    this._init();

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


  /**
   * \sa sbIFeathersManager
   */
  switchFeathers: function switchFeathers(layoutURL, internalName) {
    this._init();

    layoutDescription = this.getLayoutDescription(layoutURL);
    skinDescription = this.getSkinDescription(internalName);
    
    // Make sure we know about the requested skin and layout
    if (layoutDescription == null || skinDescription == null) {
      throw new Components.Exception("Unknown layout/skin passed to switchFeathers");
    }
    
    // Check compatibility.
    // True/false refer to the showChrome value, so check for undefined
    // to determine compatibility.
    if (this._mappings[layoutURL][internalName] === undefined) {
      throw new Components.Exception("Skin [" + internalName + "] and Layout [" + layoutURL +
            " are not compatible");
    } 
    
    // If the given pair is currently active, then do nothing
    if (layoutURL == this.currentLayoutURL 
        && internalName == this.currentSkinName)
    {
      debug("FeathersManager.switchFeathers: already active: " + layoutURL + ", " + internalName + "\n");
      return;
    }
    
    
    // Notify that a select is about to occur
    this._onSelect(layoutDescription, skinDescription);
    
    // Remember the current feathers so that we can revert later if needed
    this._previousLayoutDataRemote.stringValue = this.currentLayoutURL;
    this._previousSkinDataRemote.stringValue = this.currentSkinName;
        
    // Set new values
    this._layoutDataRemote.stringValue = layoutURL;
    this._skinDataRemote.stringValue = internalName;

    this._flushXULPrototypeCache();

    this._reloadPlayerWindow();
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  addListener: function addListener(listener) {
    if (! (listener instanceof Components.interfaces.sbIFeathersManagerListener))
    {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    this._listeners.push(listener);
  },
  
  /**
   * \sa sbIFeathersManager
   */  
  removeListener: function removeListener(listener) {
    var index = this._listeners.indexOf(listener);
    if (index > -1) {
      this._listeners.splice(index,1);
    }
  },


  /**
   * Close all player windows (except the plugin host)
   * and then relaunch the main window
   */
  _reloadPlayerWindow: function _reloadPlayerWindow() {

    var windowMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                                   .getService(Components.interfaces.nsIWindowMediator);

    // The core window (plugin host) is the only window which cannot be shut down
    var coreWindow = windowMediator.getMostRecentWindow(WINDOWTYPE_SONGBIRD_CORE);  
    
    // If no core window exists, then we are probably in test mode.
    // Therefore do nothing.
    if (coreWindow == null) {
      dump("FeathersManager._reloadPlayerWindow: unable to find window of type Songbird:Core. Test mode?\n");
      return;
    }

    // Close all open windows other than the core, dominspector, and venkman.
    // This is needed in order to reset window chrome settings.
    var playerWindows = windowMediator.getEnumerator(null);
    while (playerWindows.hasMoreElements()) {
      var window = playerWindows.getNext();
      
      if (window != coreWindow) {
        
        // Don't close domi or other debug windows... that's just annoying
        var isDebugWindow = false;
        try {    
          var windowElement = window.document.documentElement;
          var windowID = windowElement.getAttribute("id");
          if (windowID == "JSConsoleWindow" || 
              windowID == "winInspectorMain" || 
              windowID == "venkman-window") 
          {
            isDebugWindow = true;
          }
        } catch (e) {}
        
        if (!isDebugWindow) {
          // Ask nicely.  The window should be able to cancel the onunload if
          // it chooses.
          window.close();
        }
      }
    }
            
    // Determine window features.  If chrome is enabled, make resizable.
    // Otherwise remove the titlebar.
    var chromeFeatures = "chrome,modal=no,toolbar=yes,popup=no";    
    var showChrome = this.isChromeEnabled(this.currentLayoutURL, this.currentSkinName);
    if (showChrome) {
       chromeFeatures += ",resizable=yes";
    } else {
       chromeFeatures += ",titlebar=no";
    }
    
    // Set the global chrome (window border and title) flag
    this._setChromeEnabled(showChrome);
    
    // Open the new player window
    var newMainWin = coreWindow.open(this.currentLayoutURL, "", chromeFeatures);
    newMainWin.focus();
  },

      
  /**
   * Indicates to the rest of the system whether or not to 
   * enable titlebars when opening windows
   */
  _setChromeEnabled: function _setChromeEnabled(enabled) {

    // Set the global chrome (window border and title) flag
    this._showChromeDataRemote.boolValue = enabled;

    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);

    // Set the flags used to open the core window on startup.
    // Do a replacement in order to preserve whatever other features 
    // were specified.
    try {
      var titlebarRegEx = /(titlebar=)(no|yes)/;
      var replacement = (enabled) ? "$1yes" : "$1no";
      var defaultChromeFeatures = prefs.getCharPref("toolkit.defaultChromeFeatures");
      prefs.setCharPref("toolkit.defaultChromeFeatures",
              defaultChromeFeatures.replace(titlebarRegEx, replacement));
    } catch (e) {
      dump("\nFeathersManager._setChromeEnabled: Error setting defaultChromeFeatures pref! " +
            e.toString + "\n");
    }
  },      
      

  /**
   * Broadcasts an update event to all registered listeners
   */
  _onUpdate: function onUpdate() {
    this._listeners.forEach( function (listener) {
      listener.onFeathersUpdate();
    });
  },


  /**
   * Broadcasts an select (feathers switch) event to all registered listeners
   */
  _onSelect: function onSelect(layoutDesc, skinDesc) {
    // Verify args
    layoutDesc = layoutDesc.QueryInterface(Components.interfaces.sbILayoutDescription);
    skinDesc = skinDesc.QueryInterface(Components.interfaces.sbISkinDescription);
    
    // Broadcast notification
    this._listeners.forEach( function (listener) {
      listener.onFeathersSelectRequest(layoutDesc, skinDesc);
    });
  },

  _flushXULPrototypeCache: function flushXULPrototypeCache() {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
    var disabled = false;
    var userPref = false;

    try {
      disabled = prefs.getBoolPref("nglayout.debug.disable_xul_cache");
      userPref = true;
    }
    catch(e) {
    }

    if (!disabled) {
      prefs.setBoolPref("nglayout.debug.disable_xul_cache", true);
      prefs.setBoolPref("nglayout.debug.disable_xul_cache", false);
      if (!userPref) {
        prefs.clearUserPref("nglayout.debug.disable_xul_cache");
      }
    }
  },

  /**
   * Called by the observer service. Looks for XRE shutdown messages 
   */
  observe: function(subject, topic, data) {
    var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
    switch (topic) {
    case "quit-application":
      os.removeObserver(this, "quit-application");
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

    componentManager.registerFactoryLocation(CID, CLASSNAME, CONTRACTID,
                                               fileSpec, location, type);
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (cid.equals(CID)) {
       return { 
          createInstance: function(outer, iid) {
            if (outer != null)
              throw Components.results.NS_ERROR_NO_AGGREGATION;
              
            // Make the feathers manager  
            return (new FeathersManager()).QueryInterface(iid);;
          }
       };
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule


