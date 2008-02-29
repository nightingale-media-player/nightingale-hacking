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
 
/**
 * \file sbFeathersManager.js
 * \brief Coordinates the loading of feathers (combination of skin and XUL window layout)
 */ 
 
 
//
// TODO:
//  * Explore skin/layout versioning issues?
// 
 
const Ci = Components.interfaces;
const Cc = Components.classes; 

const CONTRACTID = "@songbirdnest.com/songbird/feathersmanager;1";
const CLASSNAME = "Songbird Feathers Manager Service Interface";
const CID = Components.ID("{99f24350-a67f-11db-befa-0800200c9a66}");
const IID = Ci.sbIFeathersManager;


const RDFURI_ADDON_ROOT               = "urn:songbird:addon:root" 
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";

// Fallback layouts/skin, used by previousSkinName and previousLayoutURL
// Changes to the shipped feathers must be reflected here
// and in test_feathersManager.js
const DEFAULT_MAIN_LAYOUT_URL         = "chrome://songbird/content/feathers/basic-layouts/xul/mainplayer.xul";
const DEFAULT_SECONDARY_LAYOUT_URL    = "chrome://songbird/content/feathers/basic-layouts/xul/miniplayer.xul";
const DEFAULT_SKIN_NAME               = "rubberducky/0.2";

const WINDOWTYPE_SONGBIRD_PLAYER      = "Songbird:Main";
const WINDOWTYPE_SONGBIRD_CORE        = "Songbird:Core";

Components.utils.import("resource://app/jsmodules/RDFHelper.jsm");

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
    if (!iid.equals(Ci.nsISimpleEnumerator) &&
        !iid.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
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
    if (!iid.equals(Ci.sbISkinDescription))
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
    if (!iid.equals(Ci.sbILayoutDescription))
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
function AddonMetadataReader() {};

AddonMetadataReader.prototype = {
  _manager: null,

  /**
   * Populate FeathersManager using addon metadata
   */
  loadMetadata: function(manager) {
    //debug("AddonMetadataReader: loadMetadata\n");
    this._manager = manager;
    
    var addons = RDFHelper.help(
      "rdf:addon-metadata",
      "urn:songbird:addon:root",
      RDFHelper.DEFAULT_RDF_NAMESPACES
    );
    
    for (var i = 0; i < addons.length; i++) {
      // first a little workaround to for backwards compatibility 
      // with the now obsolete <feathers> element
      // TODO: remove this when we stop supporting 0.4 feathers
      var feathersHub = addons[i];
      if (feathersHub.feathers) {
        Components.utils.reportError("Feathers Metadata Reader: The <feathers/> element in " +
                       "install.rdf is deprecated and will go away in a future version.");
        feathersHub = feathersHub.feathers[0];
      }
      
      if (feathersHub.skin) {
        var skins = feathersHub.skin;
        for (var j = 0; j < skins.length; j++) {
          try {
            this._registerSkin(addons[i], skins[j]);
          }
          catch (e) {
            this._reportErrors("", [  "An error occurred while processing " +
                  "extension " + addons[i].Value + ".  Exception: " + e  ]);
          }
        }
      }
      
      if (feathersHub.layout) {
        var layouts = feathersHub.layout;
        for (var j = 0; j < layouts.length; j++) {
          try {
            this._registerLayout(addons[i], layouts[j]);
          }
          catch (e) {
            this._reportErrors("", [  "An error occurred while processing " +
                  "extension " + addons[i].Value + ".  Exception: " + e  ]);
          }
        }
      }
    }
  },
  
  
  /**
   * Extract skin metadata
   */
  _registerSkin: function _registerSkin(addon, skin) {
    var description = new SkinDescription();
    
    // Array of error messages
    var errorList = [];
    
    // NB: there is a "verify" function as well, 
    //     but here we build and verify at the same time
    for each (var prop in SkinDescription.prototype.requiredProperties) {
      if(skin[prop][0]) {
        description[prop] = skin[prop][0];
      }
      else {
        errorList.push("Missing required <"+prop+"> element.");
      }
    }

    for each (var prop in SkinDescription.prototype.optionalProperties) {
      if(layout[prop][0]) {
        description[prop] = layout[prop][0];
      }
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
    this._manager.registerSkin(description);
    //debug("AddonMetadataReader: registered skin " + description.internalName
    //        + " from addon " + addon.Value + " \n");

    if (skin.compatibleLayout) {
      var compatibleLayouts = skin.compatibleLayout;
      for (var i = 0; i < compatibleLayouts.length; i++) {
        var compatibleLayout = compatibleLayouts[i];

        var layoutUrl;
        // TODO: FIXME! this is inconsistent with other capitalizations!!
        if(compatibleLayout.layoutURL && compatibleLayout.layoutURL[0].length != 0) {
          layoutUrl  = compatibleLayout.layoutURL[0];
        }
        else {
          errorList.push("layoutUrl was missing or incorrect.");
          continue;
        }
  
        var showChrome = false;
        if (compatibleLayout.showChrome && 
            compatibleLayout.showChrome[0] == "true") {
          showChrome = true;
        }
        var onTop = false
        if (compatibleLayout.onTop && 
            compatibleLayout.onTop[0] == "true") {
          onTop = true;
        }
  
        this._manager.assertCompatibility(
          layoutUrl, 
          description.internalName, 
          showChrome, 
          onTop
        );
      }
      
      if (errorList.length > 0) {
        this._reportErrors(
            "Ignoring a <compatibleLayout> in the install.rdf of extension " +
            addon.Value + ". Message: ", errorList);
        return;
      }
    }
  },

  /**
   * Extract layout metadata
   */
  _registerLayout: function _processLayout(addon, layout) {
    var description = new LayoutDescription();

    // Array of error messages
    var errorList = [];
    
    // NB: there is a "verify" function as well, 
    //     but here we build and verify at the same time
    for each (var prop in LayoutDescription.prototype.requiredProperties) {
      if(layout[prop][0]) {
        description[prop] = layout[prop][0];
      }
      else {
        errorList.push("Missing required <"+prop+"> element.");
      }
    }

    for each (var prop in LayoutDescription.prototype.optionalProperties) {
      if(layout[prop][0]) {
        description[prop] = layout[prop][0];
      }
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
    this._manager.registerLayout(description);
    //debug("AddonMetadataReader: registered layout " + description.name +
    //     " from addon " + addon.Value + "\n");    

    // TODO: should we error out here if there are errors already?
    if (layout.compatibleSkin) {
      var compatibleSkins = layout.compatibleSkin;
      for (var i = 0; i < compatibleSkins.length; i++) {
        var compatibleSkin = compatibleSkins[i];
  
        var internalName;
        if(compatibleSkin.internalName && compatibleSkin.internalName[0].length != 0) {
          internalName  = compatibleSkin.internalName[0];
        }
        else {
          errorList.push("internalName was missing or incorrect.");
          continue;
        }
  
        var showChrome = false;
        if (compatibleSkin.showChrome && 
            compatibleSkin.showChrome[0] == "true") {
          showChrome = true;
        }
        var onTop = false
        if (compatibleSkin.onTop && 
            compatibleSkin.onTop[0] == "true") {
          onTop = true;
        }
  
        this._manager.assertCompatibility(
          description.url, 
          internalName, 
          showChrome, 
          onTop
        );
      }
    }

    // Report errors
    if (errorList.length > 0) {
      this._reportErrors(
          "Error finding compatibility information for layout " +
          description.name + " in the install.rdf " +
          "of extension " + addon.Value + ". Message: ", errorList);
    }
  },
  
  /**
   * \brief Dump a list of errors to the console and jsconsole
   *
   * \param contextMessage Additional prefix to use before every line
   * \param errorList Array of error messages
   */
  _reportErrors: function _reportErrors(contextMessage, errorList) {
    var consoleService = Cc["@mozilla.org/consoleservice;1"].
         getService(Ci.nsIConsoleService);
    for (var i = 0; i  < errorList.length; i++) {
      Components.utils.reportError("Feathers Metadata Reader: " 
                                       + contextMessage + errorList[i]);
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

  var os      = Cc["@mozilla.org/observer-service;1"]
                      .getService(Ci.nsIObserverService);
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

  _switching: false,

  
  // Hash of skin descriptions keyed by internalName (e.g. classic/1.0)
  _skins: null,
  
  // Hash of layout descriptions keyed by URL
  _layouts: null,
  
  
  // Hash of layout URL to hash of compatible skin internalNames, pointing to 
  // {showChrome,onTop} objects.  
  //
  // eg
  // {  
  //     mainwin.xul: {
  //       blueskin: {showChrome:true, onTop:false},
  //       redskin: {showChrome:false, onTop:true},
  //     }
  // }
  //
  // Compatibility is determined by whether or not a internalName
  // key is *defined* in the hash, not the actual value it points to.
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


    // If the safe-mode dialog was requested to disable all addons, our
    // basic layouts and default skin have been disabled too. We need to 
    // check if that's the case, and reenable them if needed
    this._ensureAddOnEnabled("basic-layouts@songbirdnest.com");
    this._ensureAddOnEnabled("rubberducky@songbirdnest.com");
    
    // Make dataremotes to persist feathers settings
    var createDataRemote =  new Components.Constructor(
                  "@songbirdnest.com/Songbird/DataRemote;1",
                  Ci.sbIDataRemote, "init");

    this._layoutDataRemote = createDataRemote("feathers.selectedLayout", null);
    this._skinDataRemote = createDataRemote("selectedSkin", "general.skins.");
    
    this._previousLayoutDataRemote = createDataRemote("feathers.previousLayout", null);
    this._previousSkinDataRemote = createDataRemote("feathers.previousSkin", null);
    
    // TODO: Rename accessibility.enabled?
    this._showChromeDataRemote = createDataRemote("accessibility.enabled", null);
    
    // Load the feathers metadata
    var metadataReader = new AddonMetadataReader();
    metadataReader.loadMetadata(this);
    
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
  function assertCompatibility(layoutURL, internalName, aShowChrome, aOnTop) {
    if (! (typeof(layoutURL) == "string" && typeof(internalName) == 'string')) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._mappings[layoutURL] == null) {
      this._mappings[layoutURL] = {};
    }
    this._mappings[layoutURL][internalName] = {showChrome: aShowChrome, onTop: aOnTop};
    
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
    
    // TEMP fix for the Mac to enable the titlebar on the main window.
    // See Bug 4363
    var sysInfo = Cc["@mozilla.org/system-info;1"]
                            .getService(Ci.nsIPropertyBag2);
    var platform = sysInfo.getProperty("name");
    
    if (this._mappings[layoutURL]) {
      if (this._mappings[layoutURL][internalName]) {
        return this._mappings[layoutURL][internalName].showChrome == true;
      }
    }
   
    return false; 
  },


  getFeatherPrefBranch: function getFeatherPrefBranch (layoutURL, internalName) {
    var prefs = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefService);

    // a really simple url escaping algorithm
    // turn all non-alphanumeric characters into:
    //   "_" upper case hex charactre code "_"
    function escape_url(url) {
      return url.replace(/[^a-zA-Z0-9]/g, 
          function(c) { 
            return '_'+(c.charCodeAt(0).toString(16)).toUpperCase()+'_'; });
    }

    var branchName = 'songbird.feather.' +
      (internalName?internalName:'null') + '.' +
      (layoutURL?escape_url(layoutURL):'null') + '.';

    return prefs.getBranch(branchName);
  },


  canOnTop: function canOnTop(layoutURL, internalName) {
    this._init();
    
    if (this._mappings[layoutURL]) {
      if (this._mappings[layoutURL][internalName]) {
        return this._mappings[layoutURL][internalName].onTop == true;
      }
    }
   
    return false; 
  },


  isOnTop: function isOnTop(layoutURL, internalName) {
    this._init();

    if (!this.canOnTop(layoutURL, internalName)) {
      return false;
    }

    var prefBranch = this.getFeatherPrefBranch(layoutURL, null);
    if (prefBranch.prefHasUserValue('on_top')) {
      return prefBranch.getBoolPref('on_top');
    }
    
    return true;
  },


  setOnTop: function setOnTop(layoutURL, internalName, onTop) {
    this._init();
    
    if (!this.canOnTop(layoutURL, internalName)) {
      return false;
    }

    var prefBranch = this.getFeatherPrefBranch(layoutURL, null);
    prefBranch.setBoolPref('on_top', onTop);

    return;
  },


  /* FIXME: add the ability to observe onTop state */


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
  getLayoutsForSkin: function getLayoutsForSkin(internalName) {
    this._init();

    var layouts = [];
    
    // Find skin descriptions that are compatible with the given layout.
    for (var layout in this._mappings) {
      if (internalName in this._mappings[layout]) {
        var desc = this.getLayoutDescription(layout);
        if (desc) {
          layouts.push(desc);
        }      
      }
    }
      
    return new ArrayEnumerator( layouts );
  },


  /**
   * \sa sbIFeathersManager
   */
  switchFeathers: function switchFeathers(layoutURL, internalName) {
    // don't allow this call if we're already switching
    if (this._switching) {
      return;
    }

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
    
    // Notify that a select is about to occur
    this._onSelect(layoutDescription, skinDescription);
    
    // Remember the current feathers so that we can revert later if needed
    this._previousLayoutDataRemote.stringValue = this.currentLayoutURL;
    this._previousSkinDataRemote.stringValue = this.currentSkinName;

    // close the player window *before* changing the skin
    // otherwise Gecko tries to load an image that will go away right after and crashes
    // (songbird bug 3965)
    this._closePlayerWindow();
    
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    var callback = new FeathersManager_switchFeathers_callback(this, layoutURL, internalName);
    this._switching = true;
    timer.initWithCallback(callback, 0, Ci.nsITimer.TYPE_ONE_SHOT);
  },
  
  
  /**
   * \sa sbIFeathersManager
   * Relaunch the main window
   */
  openPlayerWindow: function openPlayerWindow() {
    
    // First, check to make sure the current
    // feathers are valid
    var layoutDescription = this.getLayoutDescription(this.currentLayoutURL);
    var skinDescription = this.getSkinDescription(this.currentSkinName);
    if (layoutDescription == null || skinDescription == null) {
      // The current feathers are invalid. Switch to the defaults.
      this.switchFeathers(DEFAULT_MAIN_LAYOUT_URL, DEFAULT_SKIN_NAME);
      return;
    }
    
    
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                                   .getService(Ci.nsIWindowMediator);

    // The core window (plugin host) is the only window which cannot be shut down
    var coreWindow = windowMediator.getMostRecentWindow(WINDOWTYPE_SONGBIRD_CORE);  

    // If no core window exists, then we are probably in test mode.
    // Therefore do nothing.
    if (coreWindow == null) {
      dump("FeathersManager.openPlayerWindow: unable to find window of type Songbird:Core. Test mode?\n");
      return;
    }

    // Determine window features.  If chrome is enabled, make resizable.
    // Otherwise remove the titlebar.
    var chromeFeatures = "chrome,modal=no,resizable=yes,centerscreen,toolbar=yes,popup=no";
    var showChrome = this.isChromeEnabled(this.currentLayoutURL, this.currentSkinName);
    if (showChrome) {
       chromeFeatures += ",titlebar=yes";
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
   * \sa sbIFeathersManager
   */  
  addListener: function addListener(listener) {
    if (! (listener instanceof Ci.sbIFeathersManagerListener))
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
   */
  _closePlayerWindow: function _closePlayerWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                                   .getService(Ci.nsIWindowMediator);

    // The core window (plugin host) is the only window which cannot be shut down
    var coreWindow = windowMediator.getMostRecentWindow(WINDOWTYPE_SONGBIRD_CORE);  
    
    // If no core window exists, then we are probably in test mode.
    // Therefore do nothing.
    if (coreWindow == null) {
      dump("FeathersManager._closePlayerWindow: unable to find window of type Songbird:Core. Test mode?\n");
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
  },

      
  /**
   * Indicates to the rest of the system whether or not to 
   * enable titlebars when opening windows
   */
  _setChromeEnabled: function _setChromeEnabled(enabled) {

    // Set the global chrome (window border and title) flag
    this._showChromeDataRemote.boolValue = enabled;

    var prefs = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefBranch);

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
      Components.utils.reportError("FeathersManager._setChromeEnabled: Error setting " + 
                                   "defaultChromeFeatures pref! " + e.toString);
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
    layoutDesc = layoutDesc.QueryInterface(Ci.sbILayoutDescription);
    skinDesc = skinDesc.QueryInterface(Ci.sbISkinDescription);
    
    // Broadcast notification
    this._listeners.forEach( function (listener) {
      listener.onFeathersSelectRequest(layoutDesc, skinDesc);
    });
  },

  _onSelectComplete: function onSelectComplete() {
    var layoutDescription = this.getLayoutDescription(this.currentLayoutURL);
    var skinDescription = this.getSkinDescription(this.currentSkinName);

    // Broadcast notification
    this._listeners.forEach( function (listener) {
      listener.onFeathersSelectComplete(layoutDescription, skinDescription);
    });
  },

  _flushXULPrototypeCache: function flushXULPrototypeCache() {
    var prefs = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefBranch);
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
    var os      = Cc["@mozilla.org/observer-service;1"]
                      .getService(Ci.nsIObserverService);
    switch (topic) {
    case "quit-application":
      os.removeObserver(this, "quit-application");
      this._deinit();
      break;
    }
  },

  /**
   * Check if an addon is disabled, and if so, re-enables it.
   * Note that this only checks for the userDisabled flag,
   * not appDisabled, so addons that have been disabled because
   * of a compatibility or security issue remain disabled.
   */
  _ensureAddOnEnabled: function(id) {
    const nsIUpdateItem = Ci.nsIUpdateItem;
    var em = Cc["@mozilla.org/extensions/manager;1"]
               .getService(Ci.nsIExtensionManager);
    var ds = em.datasource; 
    var rdf = Cc["@mozilla.org/rdf/rdf-service;1"]
                .getService(Ci.nsIRDFService);

    var resource = rdf.GetResource("urn:mozilla:item:" + id); 

    var property = rdf.GetResource("http://www.mozilla.org/2004/em-rdf#userDisabled");
    var target = ds.GetTarget(resource, property, true);

    function getData(literalOrResource) {
      if (literalOrResource instanceof Ci.nsIRDFLiteral ||
          literalOrResource instanceof Ci.nsIRDFResource ||
          literalOrResource instanceof Ci.nsIRDFInt)
        return literalOrResource.Value;
      return undefined;
    }

    var userDisabled = getData(target);
      
    if (userDisabled == "true") {
      em.enableItem(id); 
    }
  }, 

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(IID) &&
        !iid.equals(Ci.nsIObserver) && 
        !iid.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // FeathersManager.prototype

/**
 * Callback helper for FeathersManager::switchFeathers
 * This is needed to make sure the window is really closed before we switch skins
 */
function FeathersManager_switchFeathers_callback(aFeathersManager,
                                                 aLayoutURL,
                                                 aInternalName) {
  this.feathersManager = aFeathersManager;
  this.layoutURL = aLayoutURL;
  this.internalName = aInternalName;
}

FeathersManager_switchFeathers_callback.prototype = {
  /**
   * \sa nsITimerCallback
   */
  notify: function FeathersManager_switchFeathers_callback_notify() {
    // Set new values
    this.feathersManager._layoutDataRemote.stringValue = this.layoutURL;
    this.feathersManager._skinDataRemote.stringValue = this.internalName;

    this.feathersManager._flushXULPrototypeCache();
    this.feathersManager.openPlayerWindow();
    this.feathersManager._switching = false;
    this.feathersManager._onSelectComplete();
    this.feathersManager = null;
  }
}; // FeathersManager_switchFeathers_callback.prototype





/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Ci.nsIComponentRegistrar);

    componentManager.registerFactoryLocation(CID, CLASSNAME, CONTRACTID,
                                               fileSpec, location, type);
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
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



