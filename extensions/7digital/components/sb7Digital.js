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
Cu.import('resource://app/jsmodules/StringUtils.jsm');

// FUEL makes us happier...
var Application = Components.classes["@mozilla.org/fuel/application;1"]
    .getService(Components.interfaces.fuelIApplication);

// Some prefs to use
const PREF_CONFIGURED = 'extensions.7digital.configured';
// What's my GUID
const EXTENSION_GUID = '7digital@songbirdnest.com';
// Node IDs for the service pane
const NODE_SERVICES = 'SB:Services';
const NODE_7DIGITAL = 'SB:7Digital';
// Where's our stringbundle?
const STRINGBUNDLE = 'chrome://7digital/locale/7digital.properties';
// Where's that store?
const STORE_URL = 'http://www.7digital.com/songbird/';
// What's the search engine's name?
const SEARCHENGINE_NAME = '7digital'; // Note - must match ShortName in:
const SEARCHENGINE_URL = 'chrome://7digital/content/7digital-search.xml';
// What's the permission for?
const PERMISSION_SCOPE = 'http://7digital.com/';
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
    'songbird-main-window-presented' // app is done starting up the ui
  ],
  _observerService: null,

  // Should I stay or should I go now?
  _uninstall: false,

  // Handy services
  _permissionManager: null,
  _searchService: null,
  _servicePaneService: null,
  _ioService: null,

  // a repeating timer that we use for setting our search engine to default
  _firstRunTimer: null,
  
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
  this._ioService = Cc['@mozilla.org/network/io-service;1']
    .getService(Ci.nsIIOService);
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
  } else if (topic == 'timer-callback' && subject == this._firstRunTimer) {
    var searchEngine = this._searchService.getEngineByName(SEARCHENGINE_NAME);
    if (!searchEngine) {
      return;
    }
    searchEngine.hidden = false;
    this._searchService.moveEngine(searchEngine, 1);
    //gSearchHandler._previousSearchEngine = searchEngine;

    this._firstRunTimer.cancel();
    this._firstRunTimer = null;
  } else if (topic == 'songbird-main-window-presented') {
             /* && !Application.prefs.getValue(PREF_CONFIGURED, false)) { */
    // Run this everytime since it involves servicepane node migration
    this.install();
  }
}

// install 7digital music store integration
sb7Digital.prototype.install =
function sb7Digital_install() {
  this._servicePaneService.init();
  // install ourselves into the service pane
  try {
    // find the store node
    var servicesNode = this._servicePaneService.getNode(NODE_SERVICES);
    if (!servicesNode) {
      servicesNode = this._servicePaneService.addNode(NODE_SERVICES, 
          this._servicePaneService.root, true);
      servicesNode.properties = 'folder services';
      servicesNode.editable = false;
      servicesNode.name = SBString("servicesource.services");
      servicesNode.setAttributeNS(SERVICEPANE_NS, 'Weight', 1);
      this._servicePaneService.sortNode(servicesNode);
    }

    // find the 7digital node
    var myNode = this._servicePaneService.getNode(NODE_7DIGITAL);
    
    // for migration purposes, 7Digital used to be under the store node
    // let's assert that it's under the servicesnode
    if (myNode && myNode.parentNode &&
        myNode.parentNode.name != SBString("servicesource.services"))
    {
      var parentNode = myNode.parentNode;

      // it's not under the services node, delete it
      this._servicePaneService.removeNode(myNode);

      // set it to null so we'll recreate it in the next step
      myNode = null;

      // see if the parent node is empty, if it is, then remove it
      var enum = parentNode.childNodes;
      if (!enum.hasMoreElements()) {
        this._servicePaneService.removeNode(parentNode);
      } else {
        var visible = false;
        while (enum.hasMoreElements()) {
          var node = enum.getNext();
          if (!node.hidden) {
            visible = true;
            break;
          }
        }
        parentNode.hidden = !visible;
      }
    }

    if (!myNode) {
      // can't find my node, let's make it
      myNode = this._servicePaneService.addNode(NODE_7DIGITAL, servicesNode, 
          false);
      myNode.url = STORE_URL;
      myNode.image = 'chrome://7digital/skin/servicepane-icon.png';
      myNode.properties = '7digital';
      myNode.name = '&servicepane.7digital';
      myNode.stringbundle = STRINGBUNDLE;
    }

    // make the 7digital node visible
    myNode.hidden = false;

    // make the services node visible
    servicesNode.hidden = false;
  } catch (e) {
    Cu.reportError(e);
  }

  // install our search plugin
  try {

    // if we don't already have the search engine...
    if (!this._searchService.getEngineByName(SEARCHENGINE_NAME)) {
      // install the engine
      this._searchService.addEngine(SEARCHENGINE_URL,
          Ci.nsISearchEngine.DATA_XML, 
          'chrome://7digital/skin/servicepane-icon.png', false);
      // start a timer to try to set the engine as the default
      this._firstRunTimer = Cc['@mozilla.org/timer;1']
        .createInstance(Ci.nsITimer);
      this._firstRunTimer.init(this, 250, Ci.nsITimer.TYPE_REPEATING_SLACK);
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
  this._servicePaneService.init();
  // remove ourselves from the service pane
  try {
    // find the 7digital node
    var myNode = this._servicePaneService.getNode(NODE_7DIGITAL);

    // try to remove the 7digital node
    if (myNode) {
      myNode.parentNode.removeChild(myNode);
    }

    // find the services node, if it has any visible children then leave
    // it visible - otherwise hide it
    var servicesNode = this._servicePaneService.getNode(NODE_SERVICES);
    var visible = false;
    var enum = servicesNode.childNodes;
    while (enum.hasMoreElements()) {
      var node = enum.getNext();
      if (!node.hidden) {
        visible = true;
        break;
      }
    }
    servicesNode.hidden = !visible;
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

