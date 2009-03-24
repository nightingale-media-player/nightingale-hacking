/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \file  sbSeeqPod.js
 * \brief Javascript source for the SeeqPod services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// SeeqPod services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// SeeqPod imported services.
//
//------------------------------------------------------------------------------

// Songbird services.
Components.utils.import("resource://app/jsmodules/ObserverUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

// Mozilla services.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// SeeqPod services defs.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;


//------------------------------------------------------------------------------
//
// SeeqPod services configuration.
//
//------------------------------------------------------------------------------

//
// className                    Name of component class.
// cid                          Component CID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// categoryList                 List of component categories.
//
// profileVersion               Extension profile version.
//

var sbSeeqPodCfg = {
  className: "Songbird SeeqPod Service",
  cid: Components.ID("{4bba7bc9-e4c3-404a-a931-8ec16bb23592}"),
  contractID: "@songbirdnest.com/Songbird/SeeqPod;1",
  ifList: [ Ci.nsIObserver ],

  profileVersion: 2
};

sbSeeqPodCfg.categoryList = [
  {
    category: "app-startup",
    entry:    sbSeeqPodCfg.className,
    value:    "service," + sbSeeqPodCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// SeeqPod services.
//
//------------------------------------------------------------------------------

/**
 * Construct a SeeqPod services object.
 */

function sbSeeqPod() {
}

// Define the object.
sbSeeqPod.prototype = {
  // Set the constructor.
  constructor: sbSeeqPod,

  //
  // SeeqPod services fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _isInitialized           True if the SeeqPod services have been
  //                            initialized.
  //   _prefsAvailable          True if preferences are available.
  //   _sbLibMgrAvailable       True if the Songbird library manager is
  //                            available.
  //   _uninstall               If true, uninstall services on shut down.
  //   _observerSet             Set of observers.
  //

  classDescription: sbSeeqPodCfg.className,
  classID: sbSeeqPodCfg.cid,
  contractID: sbSeeqPodCfg.contractID,
  _xpcom_categories: sbSeeqPodCfg.categoryList,

  _cfg: sbSeeqPodCfg,
  _isInitialized: false,
  _prefsAvailable: false,
  _sbLibMgrAvailable: false,
  _finalUIStartupObserved: false,
  _uninstall: false,
  _observerSet: null,


  //----------------------------------------------------------------------------
  //
  // SeeqPod nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbSeeqPod_observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        this._handleAppStartup();
        break;

      case "profile-after-change" :
        this._handleProfileAfterChange();
        break;

      case "songbird-library-manager-ready" :
        this._handleSBLibMgrAvailable();
        break;

      case "final-ui-startup" :
        this._handleFinalUIStartup();
        break;

      case "browser-search-engine-modified" :
        this._handleBrowserSearchEngineModified(aSubject, aTopic, aData);
        break;

      case "em-action-requested" :
        this._handleEMActionRequested(aSubject, aTopic, aData);
        break;

      case "quit-application-granted" :
        this._handleAppQuit();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // SeeqPod nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbSeeqPodCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // SeeqPod event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle application startup events.
   */

  _handleAppStartup: function sbSeeqPod__handleAppStartup() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle profile after change events.
   */

  _handleProfileAfterChange: function sbSeeqPod__handleProfileAfterChange() {
    // Preferences are now available.
    this._prefsAvailable = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle Songbird library manager available events.
   */

  _handleSBLibMgrAvailable: function sbSeeqPod__handleSBMgrAvailable() {
    // The Songbird library manager is now available.
    this._sbLibMgrAvailable = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle final UI startup events.
   */

  _handleFinalUIStartup: function sbSeeqPod__handleFinalUIStartup() {
    // The final UI startup has been observed.
    this._finalUIStartupObserved = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle the browser search engine modified event specified by aSubject,
   * aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  _handleBrowserSearchEngineModified:
    function sbSeeqPod__handleBrowserSearchEngineModified(aSubject,
                                                          aTopic,
                                                          aData) {
    // If the SeeqPod search engine has been added, show it and insert it into
    // its proper location.
    var engine = aSubject.QueryInterface(Ci.nsISearchEngine);
    if ((aData == "engine-added") &&
        (engine.alias == "songbird-seeqpod-search")) {
      // Stop observing event.
      this._observerSet.remove(this._browserSearchEngineModifiedObserver);
      this._browserSearchEngineModifiedObserver = null;

      // Show the SeeqPod search engine and insert it into position.
      var search = Cc['@mozilla.org/browser/search-service;1']
                     .getService(Ci.nsIBrowserSearchService);
      engine.hidden = false;
      this._insertSearchEngine(engine);
    }
  },


  /**
   * Handle the extension manager action requested event specified by aSubject,
   * aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  _handleEMActionRequested:
    function sbSeeqPod__handleEMActionRequested(aSubject, aTopic, aData) {
    // Extension has been flagged to be uninstalled.
    aSubject.QueryInterface(Components.interfaces.nsIUpdateItem);
    if (aSubject.id == "{ab4bf7ab-0100-b748-a82c-a0a467773cca}") {
      if (aData == "item-uninstalled" || aData == "item-disabled") {
        this._uninstall = true;
      } else if (aData == "item-cancel-action") {
        this._uninstall = false;
      }
    }
  },


  /**
   * Handle application quit events.
   */

  _handleAppQuit: function sbSeeqPod__handleAppQuit() {
    // Uninstall services if marked to do so.
    if (this._uninstall)
      this._uninstallServices();

    // Finalize the services.
    this._finalize();
  },


  //----------------------------------------------------------------------------
  //
  // Internal SeeqPod services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbSeeqPod__initialize() {
    // Do nothing if already initialized.
    if (this._isInitialized)
      return;

    // Set up observers.
    if (!this._observerSet) {
      this._observerSet = new ObserverSet();
      this._observerSet.add(this, "profile-after-change", false, true);
      this._observerSet.add(this,
                            "songbird-library-manager-ready",
                            false,
                            true);
      this._observerSet.add(this, "final-ui-startup", false, true);
      this._observerSet.add(this, "em-action-requested", false, false);
      this._observerSet.add(this, "quit-application-granted", false, false);
    }

    // The service pane service requires the preferences and the Songbird
    // library manager.  In addition, the SeeqPod search engine must perform
    // shutdown duties even during an extension manager restart (e.g., if the
    // extension is no longer compatible), so it should do as little as possible
    // before the final UI startup.  Quit notifications are not sent unless a
    // final UI startup is sent.
    if (!this._finalUIStartupObserved ||
        !this._prefsAvailable ||
        !this._sbLibMgrAvailable) {
      return;
    }

    // Perform extra actions the first time the extension is run, or on upgrade.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var firstRunPref = Application.prefs.get("extensions.seeqpod.firstrun");
    firstRunPref = firstRunPref ? firstRunPref.value : true;
    var profileVersionPref =
          Application.prefs.get("extensions.seeqpod.profileversion");
    profileVersionPref = profileVersionPref ? profileVersionPref.value : 0;
    if (firstRunPref || (profileVersionPref < this._cfg.profileVersion)) {
      Application.prefs.setValue("extensions.seeqpod.firstrun", false);
      Application.prefs.setValue("extensions.seeqpod.profileversion",
                                 this._cfg.profileVersion);
      this._firstRunSetup();
    }

    // Show all of the SeeqPod service pane nodes.
    ServicePane.showNodes();

    // Show the SeeqPod search engine.
    var search = Cc['@mozilla.org/browser/search-service;1']
                   .getService(Ci.nsIBrowserSearchService);
    var searchEngine = search.getEngineByAlias("songbird-seeqpod-search");
    if (searchEngine)
      searchEngine.hidden = false;

    // If the SeeqPod search engine is the selected engine in prefs, set it as
    // the current engine now that it's showing.
    var selectedEngineName =
          Application.prefs.get("browser.search.selectedEngine").value;
    if (selectedEngineName == "SeeqPod") {
      var selectedEngine = search.getEngineByName(selectedEngineName);
      if (selectedEngine)
        search.currentEngine = selectedEngine;
    }

    // Initialization complete.
    this._isInitialized = true;
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbSeeqPod__finalize() {
    // Remove observers.
    if (this._observerSet) {
      this._observerSet.removeAll();
      this._observerSet = null;
    }

    // Hide SeeqPod items so that they're not visible if the SeeqPod extension
    // doesn't run on the next startup.  This can happen if Songbird is upgraded
    // and the SeeqPod extension is no longer compatible.

    // Hide all service pane nodes.
    ServicePane.hideAllNodes();

    // Hide the search engine.
    var search = Cc['@mozilla.org/browser/search-service;1']
                   .getService(Ci.nsIBrowserSearchService);
    var searchEngine = search.getEngineByAlias("songbird-seeqpod-search");
    if (searchEngine)
      searchEngine.hidden = true;
  },


  /**
   * Perform extra setup the first time the extension is run
   */

  _firstRunSetup: function sbSeeqPod__firstRunSetup() {
    // Add and remove service pane nodes.
    ServicePane.configure();

    // Update search engine.
    var search = Cc["@mozilla.org/browser/search-service;1"]
                   .getService(Ci.nsIBrowserSearchService);

    // Remove the old engine if it's there.
    var searchEngine = search.getEngineByName('SeeqPod');
    if (searchEngine) {
      search.removeEngine(searchEngine);
    }
    var searchEngine = search.getEngineByName('Seeqpod');
    if (searchEngine) {
      search.removeEngine(searchEngine);
    }

    // Observe browser search engine modified events to post-process engine
    // after it's added.
    this._browserSearchEngineModifiedObserver =
           this._observerSet.add(this,
                                 "browser-search-engine-modified",
                                 false,
                                 true);

    // Add the new engine.
    search.addEngine("chrome://seeqpod/content/seeqpod-search.xml",
                     Ci.nsISearchEngine.DATA_XML,
                     "http://www.seeqpod.com/favicon.ico",
                     false);
  },


  /**
   * Insert the SeeqPod search engine specified by aEngine into the proper
   * location.  The engine is inserted as the first non-internal search engine.
   *
   * \param aEngine             SeeqPod engine to insert.
   */

  _insertSearchEngine: function sbSeeqPod__insertSearchEngine(aEngine) {
    // Get the list of all search engines.
    var search = Cc['@mozilla.org/browser/search-service;1']
                   .getService(Ci.nsIBrowserSearchService);
    var engineList = search.getEngines({});

    // Search for the first non-internal search engine as the insert point.
    var insertPointEngine = null;
    for (var i = 0; i < engineList.length; i++) {
      // Get the next engine.
      var engine = engineList[i];

      // Check if the engine is not an internal Songbird engine.
      var tagList = engine.tags.split(" ");
      if (tagList.indexOf("songbird:internal") < 0) {
        insertPointEngine = engine;
        break;
      }
    }

    // If the specified engine is already at the insertion point, do nothing.
    if (!insertPointEngine || (insertPointEngine == aEngine))
      return;

    // Engines can only be moved before other visible engines.  Save the hidden
    // property of the insert point engine and ensure it's visible.
    var savedHidden = insertPointEngine.hidden;
    if (insertPointEngine.hidden)
      insertPointEngine.hidden = false;

    // Get the visible index of the insert point engine.
    var visibleEngineList = search.getVisibleEngines({});
    var insertPointVisibleIndex = visibleEngineList.indexOf(insertPointEngine);

    // Move the engine into place.
    if (insertPointVisibleIndex >= 0)
      search.moveEngine(aEngine, insertPointVisibleIndex);

    // Restore hidden state of the insert point engine.
    if (savedHidden)
      insertPointEngine.hidden = true;
  },


  /**
   * Uninstall the SeeqPod services.
   */

  _uninstallServices: function sbSeeqPod__uninstallServices() {
    // Clean up the service pane.
    ServicePane.removeAllNodes();

    // Remove the search engine.
    try {
      var search = Cc['@mozilla.org/browser/search-service;1']
                     .getService(Ci.nsIBrowserSearchService)
      var searchEngine = search.getEngineByAlias("songbird-seeqpod-search");
      if (searchEngine)
        search.removeEngine(searchEngine);
    } catch (e) { }

    // Clear the first-run pref.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    try {
      Application.prefs.get("extensions.seeqpod.firstrun").reset();
    } catch(e) { }
  }
}


//------------------------------------------------------------------------------
//
// SeeqPod component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbSeeqPod]);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Service pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

if (typeof ServicePane == 'undefined') {
    var ServicePane = {};
}

/**
 * Provides service pane manipulation methods
 */
ServicePane = {
    /* What URL does our node link to? */
    url: 'chrome://seeqpod/content/search.xul',
    /* What's our icon? */
    icon: 'http://seeqpod.com/favicon.ico',
    /* What's our service pane node id? */
    nodeId: 'SB:SeeqPod',
    /**
     * Our folder name as it appears in the service pane
     */
    _folderName: function() {
      var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                                  .getService(Ci.nsIStringBundleService);
      var locale =
            stringBundleService
              .createBundle("chrome://seeqpod/locale/overlay.properties");
      return SBString("servicePaneFolderLabel", null, locale);
    },

    /**
     * Find and return our folder node from the service pane
     */
    _findFolderNode: function(node) {
        if (node.isContainer && node.name != null && node.name == this._folderName()) {
            return node;
        }

        if (node.isContainer) {
            var children = node.childNodes;
            while (children.hasMoreElements()) {
                var child = children.getNext().QueryInterface(Components.interfaces.sbIServicePaneNode);
                var result = this._findFolderNode(child);
                if (result != null) {
                    return result;
                }
            }
        }
        return null;
    },

    /**
     * Find and return our folder node from the service pane (if exists)
     */
    findFolderNode: function() {
        var servicePane = Cc['@songbirdnest.com/servicepane/service;1']
                            .getService(Ci.sbIServicePaneService);
        return this._findFolderNode(servicePane.root);
    },

    /**
     * Add the main node and remove legacy nodes in the service pane
     */
    configure: function() {
        var servicePane = Cc['@songbirdnest.com/servicepane/service;1']
                            .getService(Ci.sbIServicePaneService);
        servicePane.init();

        // Walk nodes to see if a "Search" folder already exists
        var servicePaneFolder = this.findFolderNode();
        if (servicePaneFolder != null) {
            // if it does, we want to remove it.
            servicePaneFolder.parentNode.removeChild(servicePaneFolder);
        }

        // Look for our node
        var node = servicePane.getNode(this.nodeId);
        if (!node) {
            // no node? we should add one
            node = servicePane.addNode(this.nodeId, servicePane.root, false);
            // let's decorate it all pretty-like
            node.hidden = false;
            node.editable = false;
            node.name = '&servicepane.seeqpodsearch';
            node.stringbundle = 'chrome://seeqpod/locale/overlay.properties';
            node.image = this.icon;
            // point it at the right place
            node.url = this.url;
            // sort it correctly
            node.setAttributeNS('http://songbirdnest.com/rdf/servicepane#',
                                'Weight', 2);
            servicePane.sortNode(node);
        }

        servicePane.save();
    },

    /**
     * Show the folder and all children nodes in the service pane
     */
    showNodes: function() {
        try {
            var servicePane = Cc['@songbirdnest.com/servicepane/service;1']
                                .getService(Ci.sbIServicePaneService);
            servicePane.init();
            var bookmark = servicePane.getNode(this.nodeId);
            if (bookmark) {
                bookmark.hidden = false;
            }

            servicePane.save();
        } catch(e) { }
    },

    /**
     * Hide the folder and all children nodes in the service pane
     */
    hideAllNodes: function() {
        try {
            var servicePane = Cc['@songbirdnest.com/servicepane/service;1']
                                .getService(Ci.sbIServicePaneService);
            servicePane.init();
            var bookmark = servicePane.getNode(this.nodeId);
            if (bookmark) {
                bookmark.hidden = true;
            }

            servicePane.save();
        } catch(e) { }
    },

    /**
     * Remove the folder and all children node from the service pane
     */
    removeAllNodes: function() {
        try {
            var servicePane = Cc['@songbirdnest.com/servicepane/service;1']
                                .getService(Ci.sbIServicePaneService);
            servicePane.init();
            var bookmark = servicePane.getNode(this.nodeId);
            if (bookmark) {
                servicePane.root.removeChild(bookmark);
            }

            servicePane.save();
        } catch(e) { }
    }
}

