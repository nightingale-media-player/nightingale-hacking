/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */
 
/**
 * \file sbFeathersManager.js
 * \brief Coordinates the loading of feathers (combination of skin and XUL window layout)
 */ 
 
 
//
// TODO:
//  * Explore skin/layout versioning issues?
// 
 
Components.utils.import("resource://app/jsmodules/ObserverUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes; 
const Cr = Components.results;
const Cu = Components.utils;

const CONTRACTID = "@songbirdnest.com/songbird/feathersmanager;1";
const CLASSNAME = "Songbird Feathers Manager Service Interface";
const CID = Components.ID("{99f24350-a67f-11db-befa-0800200c9a66}");
const IID = Ci.sbIFeathersManager;


const RDFURI_ADDON_ROOT               = "urn:songbird:addon:root" 
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";

const CHROME_PREFIX = "chrome://"

// This DataRemote is required to indicate to the feather manager
// that it is currently running in test mode. 
const DATAREMOTE_TESTMODE = "__testmode__";

//
// Default feather/layout/skin preference prefs.
//
// * PREF_DEFAULT_MAIN_LAYOUT
//     - Default main layout to start with. This is typically the "main player"
//       layout. This layout will be used when the player window is opened for
//       the first time.
//
// * PREF_DEFAULT_SECONDARY_LAYOUT
//     - The default secondary layout. This layout is typically an alternative 
//       view to the main layout defined in |PREF_DEFAULT_MAIN_LAYOUT| - such
//       as a "mini-player" view.
//     - NOTE: This is an optional pref.
//
// * PREF_DEFAULT_SKIN_LOCALNAME
//     - The local name of the default feather. This is defined in the install.rdf
//       of the shipped feather, in the |<songbird:internalName/>| tag.
//
// * PREF_DEFAULT_FEATHER_ID
//     - The ID of the default feathers extension. This is defined in the
//       install.rdf of the shipped feather, in the <em:id/> tag.
//
//
// NOTE: Changes to the default layout/skin/feather will be automatically picked up
//       in the feathers unit test.
//
const PREF_DEFAULT_MAIN_LAYOUT       = "songbird.feathers.default_main_layout";
const PREF_DEFAULT_SECONDARY_LAYOUT  = "songbird.feathers.default_secondary_layout";
const PREF_DEFAULT_SKIN_INTERNALNAME = "songbird.feathers.default_skin_internalname";
const PREF_DEFAULT_FEATHER_ID        = "songbird.feathers.default_feather_id";

// Pref to ensure sanity tests are run the first time the feathers manager
// has started on a profile.
const PREF_FEATHERS_MANAGER_HAS_STARTED  = "songbird.feathersmanager.hasStarted";

const WINDOWTYPE_SONGBIRD_PLAYER      = "Songbird:Main";
const WINDOWTYPE_SONGBIRD_CORE        = "Songbird:Core";

Cu.import("resource://app/jsmodules/RDFHelper.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/PlatformUtils.jsm");

/**
 * sbISkinDescription
 */
function SkinDescription() {};
SkinDescription.prototype = {
  // TODO Expand?
  requiredProperties: [ "internalName" ],
  optionalProperties: [ "name" ],
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
    dump("AddonMetadataReader::loadMetadata\n");

    this._manager = manager;

    var addons = RDFHelper.help("rdf:addon-metadata",
                                "urn:songbird:addon:root",
                                RDFHelper.DEFAULT_RDF_NAMESPACES);
    dump("AddonMetadataReader::loadMetadata -- addons.length = "+addons.length+"\n");

    for (var i = 0; i < addons.length; i++) {
      dump("AddonMetadataReader::loadMetadata -- in loop: i = "+i+"\n");
      // first a little workaround to for backwards compatibility 
      // with the now obsolete <feathers> element
      // TODO: remove this when we stop supporting 0.4 feathers
      var feathersHub = addons[i];
      if (feathersHub.feathers) {
        Components.utils.reportError("Feathers Metadata Reader: The <feathers/> element in " +
                       "install.rdf is deprecated and will go away in a future version.");
        feathersHub = feathersHub.feathers[0];
      }

      dump("AddonMetadataReader::loadMetadata -- about to check feathersHub.skin\n");
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
    dump("AddonMetadataReader::_registerSkin(addon = "+addon+", skin = "+skin+")\n");
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
      if(skin[prop] && skin[prop][0]) {
        description[prop] = skin[prop][0];
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

    // Don't register skins that have no name, but throw up a friendly
    // informative message so users don't see gross red error messages
    // and misinterpret them.
    if (!skin["name"]) {
      var consoleSvc = Cc["@mozilla.org/consoleservice;1"]
                         .getService(Ci.nsIConsoleService);
      consoleSvc.logStringMessage(
          "Feathers Metadata Reader: Skipping registration of Feather with " +
          "skin internal name: '" + skin["internalName"] + "' due to " +
          "undefined or blank skin name.");
      return;
    }
    
    // Submit description
    this._manager.registerSkin(description);
    //debug("AddonMetadataReader: registered skin " + description.internalName
    //        + " from addon " + addon.Value + " \n");

    if (skin.compatibleLayout) {
      var compatibleLayouts = skin.compatibleLayout;
      var hasRegisteredDefault = false;
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

        var showChrome = (PlatformUtils.platformString == "Darwin");
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

        // If this is the first element in the RDF - or the layout is set to
        // be the default layout, let's assign these attributes here.
        if ((i == 0) || 
            (compatibleLayout.isDefault && 
            compatibleLayout.isDefault[0] == "true")) 
        {
          this._manager.setDefaultLayout(layoutUrl, description.internalName);
          
          if (hasRegisteredDefault) {
            Components.utils.reportError(
              "A default layout has already been assigned for " + 
              description.internalName
            );
          }
          
          hasRegisteredDefault = true;
        }
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

    // Resolve any localised layout names to their actual strings
    if (description.name.substr(0,CHROME_PREFIX.length) == CHROME_PREFIX)
    {
      var name = SBString("feathers.name.unnamed");
      var split = description.name.split("#", 2);
      if (split.length == 2) {
        var bundle = new SBStringBundle(split[0]);
        name = bundle.get(split[1], name);
      }
      description.name = name;
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
        if (compatibleSkin.internalName && 
           compatibleSkin.internalName[0].length != 0) 
        {
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
        var onTop = false;
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

  this._observerSet = new ObserverSet();

  // We need to init at final UI startup after the Extension Manager has checked
  // for feathers (required for NO_EM_RESTART support).
  this._observerSet.add(this, "final-ui-startup", false, true);

  // We need to unhook things on shutdown
  this._observerSet.add(this, "quit-application", false, true);
  
  this._skins = {};
  this._layouts = {};
  this._skinDefaults = {};
  this._mappings = {};
  this._listeners = [];
};
FeathersManager.prototype = {
  classDescription:  CLASSNAME,
  classID:           CID,
  contractID:        CONTRACTID,
  _xpcom_categories:
  [{
    category: "app-startup",
    entry: "feathers-manager",
    value: "service," + CONTRACTID,
    service: true
  }],
  constructor: FeathersManager,
  
  _observerSet: null,

  _layoutDataRemote: null,
  _skinDataRemote: null,

  _previousLayoutDataRemote: null,
  _previousSkinDataRemote: null,
  
  _showChromeDataRemote: null,

  _switching: false,

  // Ignore autoswitching when the feathers manager is run for the first time.
  _ignoreAutoswitch: false,

  // feather, layout, and skin name defaults
  _defaultLayoutURL: "",
  _defaultSecondaryLayoutURL: "",
  _defaultSkinName: "",
  
  // Hash of skin descriptions keyed by internalName (e.g. classic/1.0)
  _skins: null,
  
  // Hash of layout descriptions keyed by URL
  _layouts: null,
  
  // Hash of default layouts for skins.
  _skinDefaults: null,
                                                                  
  
  
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

  // nsIURI, our agent sheet
  _agentSheetURI: null,
  

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
    // before reading the prefs, make sure they're stabilized; see bug
    // 14504 for details.
    Cc["@mozilla.org/browser/browserglue;1"]
      .getService(Ci.nsIObserver)
      .observe(null, "prefservice:after-app-defaults", null);
    
    var AppPrefs = Cc["@mozilla.org/fuel/application;1"]
                     .getService(Ci.fuelIApplication).prefs;

    // If the safe-mode dialog was requested to disable all addons, our
    // basic layouts and default skin have been disabled too. We need to 
    // check if that's the case, and reenable them if needed
    var defaultFeather = AppPrefs.getValue(PREF_DEFAULT_FEATHER_ID, "");
    this._ensureAddOnEnabled(defaultFeather);

    // Read in defaults
    this._defaultLayoutURL = AppPrefs.getValue(PREF_DEFAULT_MAIN_LAYOUT, "");
    this._defaultSecondaryLayoutURL = 
      AppPrefs.getValue(PREF_DEFAULT_SECONDARY_LAYOUT, "");
    this._defaultSkinName = AppPrefs.getValue(PREF_DEFAULT_SKIN_INTERNALNAME, "");
    
    if (!AppPrefs.has(PREF_FEATHERS_MANAGER_HAS_STARTED)) {
      // Ignore the autoswitch detecting for the first run, this prevents
      // jumping between two shipped feathers.
      this._ignoreAutoswitch = true;

      // Now the feather manager has started at least once.
      AppPrefs.setValue(PREF_FEATHERS_MANAGER_HAS_STARTED, true);
    }


    // Register our agent sheet for form styling
    this._agentSheetURI = Cc["@mozilla.org/network/io-service;1"]
                            .getService(Ci.nsIIOService)
                            .newURI("chrome://songbird/skin/formsImport.css",
                                    null, null);
    var styleSheetService = Cc["@mozilla.org/content/style-sheet-service;1"]
                              .getService(Ci.nsIStyleSheetService);
    styleSheetService.loadAndRegisterSheet(this._agentSheetURI,
                                           styleSheetService.AGENT_SHEET);

    // Make dataremotes to persist feathers settings
    var createDataRemote =  new Components.Constructor(
                  "@songbirdnest.com/Songbird/DataRemote;1",
                  Ci.sbIDataRemote, "init");

    this._layoutDataRemote = createDataRemote("feathers.selectedLayout", null);
    this._skinDataRemote = createDataRemote("selectedSkin", "general.skins.");
    
    this._previousLayoutDataRemote = createDataRemote("feathers.previousLayout", null);
    this._previousSkinDataRemote = createDataRemote("feathers.previousSkin", null);

    // Check to make sure we have a skin set; if not, then set the current
    // skin to be the default skin.  If we don't have a default skin, then
    // we'll really fubar'd.  (bug 20528)
    if (!this.currentSkinName) {
      this._skinDataRemote.stringValue = this._defaultSkinName;
    }
    // TODO: Rename accessibility.enabled?
    this._showChromeDataRemote = createDataRemote("accessibility.enabled", null);

    // Load the feathers metadata
    var metadataReader = new AddonMetadataReader();
    metadataReader.loadMetadata(this);
    
    // If no layout url has been specified, set to default
    if (this._layoutDataRemote.stringValue == "") {
      this._layoutDataRemote.stringValue = this._defaultLayoutURL;
    }

    // Ensure chrome enabled is set appropriately
    var showChrome = this.isChromeEnabled(this.currentLayoutURL,
                                          this.currentSkinName);
    this._setChromeEnabled(showChrome);
  },
  
  /**
   * Called on xpcom-shutdown
   */
  _deinit: function deinit() {
    this._observerSet.removeAll();
    this._observerSet = null;
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
    return this._skinDataRemote.stringValue;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get currentLayoutURL() {
    return this._layoutDataRemote.stringValue;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get previousSkinName() {
    // Test to make sure the previous skin exists
    var skin = this.getSkinDescription(this._previousSkinDataRemote.stringValue);
    
    // If the skin exists, then return the skin name
    if (skin) {
      return skin.internalName;
    }
    
    // Otherwise, return the default skin
    return this._defaultSkinName;
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  get previousLayoutURL() {
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
    var layoutURL = this._defaultLayoutURL;
    if (this.currentLayoutURL == layoutURL) {
      layoutURL = this._defaultSecondaryLayoutURL;
    }
    
    return layoutURL;    
  },
 
 
  /**
   * \sa sbIFeathersManager
   */ 
  get skinCount() {
    return this._skinCount;
  },
  
  /**
   * \sa sbIFeathersManager
   */  
  get layoutCount() {
    return this._layoutCount;
  },


  /**
   * \sa sbIFeathersManager
   */
  getSkinDescriptions: function getSkinDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return ArrayConverter.enumerator( [this._skins[key] for (key in this._skins)] );
  },

  /**
   * \sa sbIFeathersManager
   */
  getLayoutDescriptions: function getLayoutDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return ArrayConverter.enumerator( [this._layouts[key] for (key in this._layouts)] );
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
    dump("sbFeathersManager::getSkinDescription(internalName = "+internalName+")\n");
    return this._skins[internalName];
  },
  
  
  /**
   * \sa sbIFeathersManager
   */  
  registerLayout: function registerLayout(layoutDesc) {
    LayoutDescription.verify(layoutDesc);

    if (!(layoutDesc.url in this._layouts)) {
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
    if (layoutDesc.url in this._layouts) {
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
    dump("sbFeathersManager::getLayoutDescription(url = "+url+")\n");
    return (url in this._layouts ? this._layouts[url] : null);
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

    // check if this layout/skin combination has already been seen, 
    // if it hasn't then we want to switch to it in openPlayerWindow,
    // so remember it
    var branch = this.getFeatherPrefBranch(layoutURL, internalName);
    var seen = false;
    try {
      seen = branch.getBoolPref("seen");
    } catch (e) { }
    if (!seen) {
      branch.setBoolPref("seen", true);
      if (!this._autoswitch && !this._ignoreAutoswitch) {
        this._autoswitch = {};
        this._autoswitch.skin = internalName;
        this._autoswitch.layoutURL = layoutURL;
      }
    }
    
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
  setDefaultLayout: function setDefaultLayout(aLayoutURL, aInternalName) {
    if (!(typeof(aLayoutURL) == "string" && 
          typeof(aInternalName) == "string")) 
    {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    
    this._skinDefaults[aInternalName] = aLayoutURL;
    this._onUpdate();  // notify observers
  },
  
  /**
   * \sa sbIFeathersManager
   */
  getDefaultLayout: function getDefaultLayout(aInternalName) {
    if (!typeof(aInternalName) == "string") {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    
    var defaultLayoutURL = this._skinDefaults[aInternalName];
    
    // If a default URL isn't registered, just use the first compatible 
    // layout registered for the skin identifier.
    if (!defaultLayoutURL) {
      for (var curLayoutURL in this._mappings) {
        if (aInternalName in this._mappings[curLayoutURL]) {
          defaultLayoutURL = curLayoutURL;
          break;
        }
      }
    }
    
    // Something is terribly wrong - no layouts are registered for this skin
    if (!defaultLayoutURL) {
      throw Components.results.NS_ERROR_FAILURE;
    }
    
    return defaultLayoutURL;
  },
    
  /**
   * \sa sbIFeathersManager
   */
  isChromeEnabled: function isChromeEnabled(layoutURL, internalName) {
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
    if (this._mappings[layoutURL]) {
      if (this._mappings[layoutURL][internalName]) {
        return this._mappings[layoutURL][internalName].onTop == true;
      }
    }
   
    return false; 
  },


  isOnTop: function isOnTop(layoutURL, internalName) {
    if (!this.canOnTop(layoutURL, internalName)) {
      return false;
    }

    var prefBranch = this.getFeatherPrefBranch(layoutURL, null);
    if (prefBranch.prefHasUserValue('on_top')) {
      return prefBranch.getBoolPref('on_top');
    }
    
    return false;
  },


  setOnTop: function setOnTop(layoutURL, internalName, onTop) {
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
    return ArrayConverter.enumerator( skins );
  },
  
  
  /**
   * \sa sbIFeathersManager
   */
  getLayoutsForSkin: function getLayoutsForSkin(internalName) {
    return ArrayConverter.enumerator( this._getLayoutsArrayForSkin(internalName) );
  },


  /**
   * \sa sbIFeathersManager
   */
  switchFeathers: function switchFeathers(layoutURL, internalName) {
    // don't allow this call if we're already switching
    if (this._switching) {
      return;
    }

    dump("sbFeathersManager::switchFeathers(layoutURL = "+layoutURL+", internalName = "+internalName+")\n");
    var layoutDescription = this.getLayoutDescription(layoutURL);
    var skinDescription = this.getSkinDescription(internalName);
    dump("sbFeathersManager::switchFeathers -- layoutDescription = "+layoutDescription+"\n");
    dump("sbFeathersManager::switchFeathers -- skinDescription = "+skinDescription+"\n");

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

    // Make sure the application doesn't exit because we closed the last window
    var appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);
    appStartup.enterLastWindowClosingSurvivalArea();

    try {
      // close the player window *before* changing the skin
      // otherwise Gecko tries to load an image that will go away right after and crashes
      // (songbird bug 3965)
      this._closePlayerWindow(internalName == this.currentSkinName);
      
      var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      var callback = new FeathersManager_switchFeathers_callback(this, layoutURL, internalName);
      this._switching = true;
      timer.initWithCallback(callback, 0, Ci.nsITimer.TYPE_ONE_SHOT);
    } catch (e) {
      // If we failed make sure to allow the application to quit again
      appStartup.exitLastWindowClosingSurvivalArea();
      throw e;
    }
  },
  
  /**
   * \sa sbIFeathersManager
   */
  switchToNextLayout: function switchToNextLayout() {
    var curSkinName = this.currentSkinName;
    var curLayoutURL = this.currentLayoutURL;
    
    // Find the next layout (if one exists):
    var nextLayout;
    var layouts = this._getLayoutsArrayForSkin(curSkinName);
    for (var i = 0; i < layouts.length; i++) {
      if (layouts[i].url == curLayoutURL) {
        if (i >= layouts.length - 1) {
          nextLayout = layouts[0];
        } 
        else {
          nextLayout = layouts[i+1];
        }
      }
    }
    
    if (nextLayout != null && nextLayout.url != curLayoutURL) {
      this.switchFeathers(nextLayout.url, curSkinName);
    }
  },
  
  
  /**
   * \sa sbIFeathersManager
   * Relaunch the main window
   */
  openPlayerWindow: function openPlayerWindow() {
    dump("sbFeathersManager::openPlayerWindow\n");

    // First, check if we should auto switch to a new skin/layout
    // (but only if we're not already in the middle of a switch)
    if (this._autoswitch && !this._switching) {
      this._layoutDataRemote.stringValue = this._autoswitch.layoutURL;
      this._skinDataRemote.stringValue = this._autoswitch.skin;
      this._autoswitch = null;
    } 
    
    // Check to make sure the current feathers are valid
    var layoutDescription = this.getLayoutDescription(this.currentLayoutURL);
    var skinDescription = this.getSkinDescription(this.currentSkinName);
    if (layoutDescription == null || skinDescription == null) {
      // The current feathers are invalid. Switch to the defaults.
      this.switchFeathers(this._defaultLayoutURL, this._defaultSkinName);
      return;
    }

    var currentLayoutURL = this.currentLayoutURL;
    var currentSkinName = this.currentSkinName;

    // check if we're in safe mode
    var app = Cc["@mozilla.org/xre/app-info;1"]
                .getService(Ci.nsIXULRuntime);
    if (app.inSafeMode) {
      // in safe mode, force using default layout/skin
      // (but do not persist this choice)
      currentLayoutURL = this._defaultLayoutURL;
      currentSkinName = this._defaultSkinName;
    }

    // Check to see if we are in test mode, if so, we don't actually
    // want to open the window as it will break the testing we're 
    // attempting to do.    
    if(SBDataGetBoolValue(DATAREMOTE_TESTMODE)) {
      // Indicate to the console and jsconsole that we are test mode.
      dump("FeathersManager.openPlayerWindow: In Test Mode, no window will be open!\n");
      Cu.reportError("FeathersManager.openPlayerWindow: In Test Mode, no window will be open\n");
      return;
    }

    // Determine window features.  If chrome is enabled, make resizable.
    // Otherwise remove the titlebar.
    var chromeFeatures = "chrome,modal=no,resizable=yes,toolbar=yes,popup=no";
    
    // on windows and mac, centerscreen gets overriden by persisted position.
    // not so for linux.
    var runtimeInfo = Components.classes["@mozilla.org/xre/runtime;1"]
                                .getService(Components.interfaces.nsIXULRuntime);
    switch (runtimeInfo.OS) {
      case "WINNT":
      case "Darwin":
        chromeFeatures += ",centerscreen";
    }
    
    var showChrome = this.isChromeEnabled(currentLayoutURL, currentSkinName);
    if (showChrome) {
       chromeFeatures += ",titlebar=yes";
    } else {
       chromeFeatures += ",titlebar=no";
    }
    
    // Set the global chrome (window border and title) flag
    this._setChromeEnabled(showChrome);

    // Open the new player window
    var windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                          .getService(Ci.nsIWindowWatcher);

    var newMainWin = windowWatcher.openWindow(null,
                                              currentLayoutURL,
                                              "",
                                              chromeFeatures,
                                              null);
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
   * Get an array of the layouts for the current skin.
   */
  _getLayoutsArrayForSkin: function _getLayoutsArrayForSkin(internalName) {
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
    
    return layouts;
  },


  /**
   * Close all player windows (except the plugin host)
   *
   * @param aLayoutSwitchOnly if true, we will not close windows that wish to
   *                          not be closed on layout changes
   * 
   */
  _closePlayerWindow: function _closePlayerWindow(aLayoutSwitchOnly) {
    // Check to see if we are in test mode, if so, we don't actually
    // want to open the window as it will break the testing we're 
    // attempting to do.    
    if(SBDataGetBoolValue(DATAREMOTE_TESTMODE)) {
      // Indicate to the console and jsconsole that we are test mode.
      dump("FeathersManager.openPlayerWindow: In Test Mode\n");
      Cu.reportError("FeathersManager.openPlayerWindow: In Test Mode\n");
      return;
    }

    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                                   .getService(Ci.nsIWindowMediator);

    // Close all open windows other than the core, dominspector, and venkman.
    // This is needed in order to reset window chrome settings.
    var playerWindows = windowMediator.getEnumerator(null);
    while (playerWindows.hasMoreElements()) {
      var window = playerWindows.getNext();
      if (!window) {
        continue;
      }

      // Don't close DOMi or other debug windows... that's just annoying
      try {
        let docElement = window.document.documentElement;
        // Go by the window ID instead of URL for these, in case we end up with
        // things that try to override them (e.g. Error2)
        switch (docElement.getAttribute("id")) {
          case "JSConsoleWindow":
          case "winInspectorMain":
          case "venkman-window":
            // don't close these
            continue;
        }
        if (aLayoutSwitchOnly) {
          // this is a layout switch, the skin will be the same;
          // check for opt-in attribute and skip those
          if (docElement.hasAttribute("sb-no-close-on-layout-switch")) {
            // don't close this either
            continue;
          }
        }
      } catch (e) {
        /* ignore any errors - assume they're closable */
      }

      window.close();
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
      Cu.reportError("FeathersManager._setChromeEnabled: Error setting " + 
                     "defaultChromeFeatures pref!\n" + e);
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

  /**
   * Called by the observer service. Looks for XRE shutdown messages 
   */
  observe: function(subject, topic, data) {
    var os      = Cc["@mozilla.org/observer-service;1"]
                      .getService(Ci.nsIObserverService);
    switch (topic) {
    case "quit-application":
      this._deinit();
      break;

    case "final-ui-startup":
      this._init();
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
    AddonManager.getAddonByID(id, function(addon) {
      if (addon.userDisabled) {
        addon.userDisabled = false;
      }
    });
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([IID, Components.interfaces.nsIObserver]),
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
    try {
      // Set new values
      this.feathersManager._layoutDataRemote.stringValue = this.layoutURL;
      this.feathersManager._skinDataRemote.stringValue = this.internalName;

      // Flush all chrome caches
      Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService)
        .notifyObservers(null, "chrome-flush-caches", null);

      // Reload our agent sheet
      var styleSheetService = Cc["@mozilla.org/content/style-sheet-service;1"]
                                .getService(Ci.nsIStyleSheetService);
      styleSheetService.unregisterSheet(this.feathersManager._agentSheetURI,
                                        styleSheetService.AGENT_SHEET);
      styleSheetService.loadAndRegisterSheet(this.feathersManager._agentSheetURI,
                                             styleSheetService.AGENT_SHEET);
  
      this.feathersManager.openPlayerWindow();
      this.feathersManager._switching = false;
      this.feathersManager._onSelectComplete();
      this.feathersManager = null;
    } finally {
      // After the callback the application should always be able to quit again
      var appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Ci.nsIAppStartup);
      appStartup.exitLastWindowClosingSurvivalArea();
    }
  }
}; // FeathersManager_switchFeathers_callback.prototype





/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */

var NSGetFactory = XPCOMUtils.generateNSGetFactory([FeathersManager]);

