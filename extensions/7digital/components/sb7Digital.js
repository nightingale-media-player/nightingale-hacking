/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the 'GPL').
//
// Software distributed under the License is distributed
// on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, either
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

// Constants for convience
if (typeof(Cc) == 'undefined')
  var Cc = Components.classes;
if (typeof(Ci) == 'undefined')
  var Ci = Components.interfaces;
if (typeof(Cu) == 'undefined')
  var Cu = Components.utils;
if (typeof(Cr) == 'undefined')
  var Cr = Components.results;

// Imports to help with some common tasks
Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import('resource://app/jsmodules/sbLibraryUtils.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

// FUEL makes us happier...
var Application = Components.classes["@mozilla.org/fuel/application;1"]
    .getService(Components.interfaces.fuelIApplication);

// Some prefs to use
const PREF_CONFIGURED = 'extensions.7digital.configured';
// What's my GUID
const EXTENSION_GUID = '7digital@songbirdnest.com';
// Node IDs for the service pane
const NODE_STORES = 'SB:Stores';
const NODE_7DIGITAL = 'SB:7Digital';
// Where's our stringbundle?
const STRINGBUNDLE = 'chrome://7digital/locale/7digital.properties';
// Where's that store?
const STORE_URL = 'http://songbird.7digital.com/songbird/';
// What's the search engine's name?
const SEARCHENGINE_NAME = '7digital'; // Note - must match ShortName in:
const SEARCHENGINE_URL = 'chrome://7digital/content/7digital-search.xml';
// What's the permission for?
const PERMISSION_SCOPE = 'http://songbird.7digital.com/';
// What's the service pane's namespace
const SERVICEPANE_NS = 'http://songbirdnest.com/rdf/servicepane#'

function sb7Digital() { }
sb7Digital.prototype = {
  // XPCOM stuff
  className: 'sb7Digital',
  classDescription: '7digital Music Store Integration',
  classID: Components.ID('{2ceaef85-303f-4e6d-af40-b449f4922693}'),
  contractID: '@songbirdnest.com/7digital;1',
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  // Observer stuff
  _observerTopics: [
    'quit-application-granted', // the application is shutting down
    'em-action-requested',      // the extension manager is doing something
  ],
  _observerService: null,

  // Should I stay or should I go now?
  _uninstall: false,

  // Handy services
  _permissionManager: null,
  _searchService: null,
  _servicePaneService: null,
  _ioService: null,
  
  // What whitelists do we do?
  _permissions: ['rapi.playback_control', 'rapi.playback_read', 
      'rapi.library_read', 'rapi.library_write'],
}

sb7Digital.prototype.initialize = 
function sb7Digital_initialize() {
  // Listen to a bunch of observer topics
  for (var i=0; i<this._observerTopics.length; i++) {
    this._observerService.addObserver(this, this._observerTopics[i], false);
  }

  // here's some service we may well use later...
  this._permissionManager = Cc["@mozilla.org/permissionmanager;1"]
    .getService(Ci.nsIPermissionManager);
  this._searchService = Cc['@mozilla.org/browser/search-service;1']
    .getService(Ci.nsIBrowserSearchService);
  this._servicePaneService = Cc['@songbirdnest.com/servicepane/service;1']
    .getService(Ci.sbIServicePaneService);
  this._servicePaneService.init();
  this._ioService = Cc['@mozilla.org/network/io-service;1']
    .getService(Ci.nsIIOService);

  // If we need to install ourselves, then let's do that
  if (!Application.prefs.getValue(PREF_CONFIGURED, false)) {
    this.install();
  }
}
// nsIObserver
sb7Digital.prototype.observe =
function sb7Digital_observe(subject, topic, data) {
  if (topic == 'app-startup') {
    this._observerService = Cc['@mozilla.org/observer-service;1']
        .getService(Ci.nsIObserverService);
    this._observerService.addObserver(this, 'profile-after-change', false);
  } else if (topic == 'profile-after-change') {
    this._observerService.removeObserver(this, 'profile-after-change', false);
    this.initialize();
  } else if (topic == 'quit-application-granted') {
    // the application is shutting down
    if (this._uninstall) {
      this.uninstall();
      this._uninstall = false;
    }
  } else if (topic == 'em-action-requested') {
    // the extension manager is doing something
    subject.QueryInterface(Components.interfaces.nsIUpdateItem);
    if (subject.id != EXTENSION_GUID) {
      // if they're talking about someone else, we don't care
      return;
    }
    if (data == 'item-uninstalled' || data == 'item-disabled') {
      this._uninstall = true;
    } else if (data == 'item-cancel-action') {
      this._uninstall = false;
    }
  }
}

// install 7digital music store integration
sb7Digital.prototype.install =
function sb7Digital_install() {
  // install ourselves into the service pane
  try {
    // find the 7digital node
    var myNode = this._servicePaneService.getNode(NODE_7DIGITAL);
    if (!myNode) {
      // can't find my node, let's make it
      // but first we need to get the store node
      var storesNode = this._servicePaneService.getNode(NODE_STORES);
      if (!storesNode) {
        // what? no store node?
        storesNode = this._servicePaneService.addNode(NODE_STORES, 
            this._servicePaneService.root, true);
        storesNode.properties = 'folder stores';
        storesNode.editable = false;
        storesNode.name = '&servicepane.stores';
        storesNode.stringbundle = STRINGBUNDLE;
        storesNode.setAttributeNS(SERVICEPANE_NS, 'Weight', 2);
        this._servicePaneService.sortNode(storesNode);
      }

      myNode = this._servicePaneService.addNode(NODE_7DIGITAL, storesNode, 
          false);
      myNode.url = STORE_URL;
      myNode.image = 'chrome://7digital/skin/servicepane-icon.png';
      myNode.properties = '7digital';
      myNode.name = '&servicepane.7digital';
      myNode.stringbundle = STRINGBUNDLE;
    }

    // make the 7digital node visible
    myNode.hidden = false;

    // find the store node
    var storesNode = this._servicePaneService.getNode(NODE_STORES);
    // show it
    storesNode.hidden = false;
    
  } catch (e) {
    Cu.reportError(e);
  }

  // install our search plugin
  try {

    // if we don't already have the search engine...
    if (!this._searchService.getEngineByName(SEARCHENGINE_NAME)) {
      this._searchService.addEngine(SEARCHENGINE_URL,
          Ci.nsISearchEngine.DATA_XML, 
          'chrome://7digital/skin/servicepane-icon.png', false);
    }
  } catch (e) {
    Cu.reportError(e);
  }

  // add whitelist entries
  try {
    var scope = this._ioService.newURI(PERMISSION_SCOPE, null, null);
    for (var i = 0; i < this._permissions.length; i++) {
      if (!this._permissionManager.testExactPermission(scope, 
            this._permissions[i])) {
        this._permissionManager.add(scope, this._permissions[i], 
            Ci.nsIPermissionManager.ALLOW_ACTION);
      }
    }
  } catch (e) {
    Cu.reportError(e);
  }
  Application.prefs.setValue(PREF_CONFIGURED, true);
}

// uninstall 7digital music store integration
sb7Digital.prototype.uninstall =
function sb7Digital_uninstall() {
  // remove ourselves from the service pane
  try {
    // find the 7digital node
    var myNode = this._servicePaneService.getNode(NODE_7DIGITAL);

    // try to remove the 7digital node
    if (myNode) {
      myNode.parentNode.removeChild(myNode);
    }

    // find the stores node
    var storesNode = this._servicePaneService.getNode(NODE_STORES);
    // kill it!
    this._servicePaneService.root.removeChild(storesNode);
  } catch (e) {
    Cu.reportError(e);
  }

  // uninstall our search plugin
  try {

    // look for the engine...
    var engine = this._searchService.getEngineByName(SEARCHENGINE_NAME);
    if (engine) {
      this._searchService.removeEngine(engine);
    }
  } catch (e) {
    Cu.reportError(e);
  }

  // remove whitelist entries
  try {
    var scope = this._ioService.newURI(PERMISSION_SCOPE, null, null);
    for (var i = 0; i < this._permissions.length; i++) {
      if (this._permissionManager.testExactPermission(scope, 
            this._permissions[i])) {
        this._permissionManager.remove(scope.host, this._permissions[i]);
      }
    }
  } catch (e) {
    Cu.reportError(e);
  }
  Application.prefs.setValue(PREF_CONFIGURED, false);
}

// XPCOM NSGetModule implementation
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sb7Digital],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry('app-startup',
      sb7Digital.prototype.classDescription,
      sb7Digital.prototype.contractID, true, true);
  });
}

