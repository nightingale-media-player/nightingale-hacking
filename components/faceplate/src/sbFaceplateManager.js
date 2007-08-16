/** vim: ts=2 sw=2 expandtab
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
 * \file sbFaceplateManager.js
 * \brief Manages the lifecycle of faceplate pane bindings
 */


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;

const URL_BINDING_DEFAULT_PANE = "chrome://songbird/content/bindings/facePlate.xml#default-pane";
const URL_BINDING_DASHBOARD_PANE = "chrome://songbird/content/bindings/facePlate.xml#playback-pane"; 

const DATAREMOTE_PLAYBACK = "faceplate.playing";



/**
 * \class ArrayEnumerator
 * \brief Wraps a js array in an nsISimpleEnumerator
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

  /**
   *  Gah. nsISimpleEnumerator uses hasMoreElements, but 
   *  nsIStringEnumerator uses hasMore.  
   */
  hasMoreElements: function() {
    if (this.index < this.data.length)
      return true;
    else
      return false;
  },
  hasMore: function () {
    return this.hasMoreElements();
  },
  
  QueryInterface: function(iid)
  {
    if (!iid.equals(Ci.nsISimpleEnumerator) &&
        !iid.equals(Ci.nsIStringEnumerator) &&        
        !iid.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}






/**
 * \class FaceplatePane
 * \brief An implementation of nsIFaceplatePane.  Acts as a single
 *        representation the many instances of a faceplate pane.
 * \sa sbIFaceplatePane 
 */
function FaceplatePane(aID, aName, aBindingURL) 
{
  this._id = aID;
  this._name = aName;
  this._bindingURL = aBindingURL;
  this._data = {};
  this._observers = [];
}
FaceplatePane.prototype = {

  _id: null,
  _name: null,
  _bindingURL: null,

  _data: null,
  
  _observers: null,
  
  get id() { return this._id; },
  get name() { return this._name; },
  get bindingURL() { return this._bindingURL; }, 
  
  /**
   * \sa sbIFaceplatePane
   */
  
  setData: function FaceplatePane_setData(aKey, aValue) {
    if (!aKey) {
      throw Components.results.NS_ERROR_INVALID_ARG;      
    }
    
    this._data[aKey] = aValue;
    this._notify(aKey);
  },

  getData: function FaceplatePane_getData(aKey) {
    return this._data[aKey];
  },

  getKeys: function FaceplatePane_getKeys() {
    // Copy all the data keys into an array, and then return an enumerator
    return new ArrayEnumerator( [key for (key in this._data)] );     
  },
  
  addObserver: function FaceplatePane_addObserver(aObserver) {
    if (! (aObserver instanceof Ci.nsIObserver)) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._observers.indexOf(aObserver) == -1) {
      this._observers.push(aObserver);
    }
  },
  
  removeObserver: function FaceplatePane_removeObserver(aObserver) {
    var index = this._observers.indexOf(aObserver);
    if (index > -1) {
      this._observers.splice(index,1);
    }     
  },
  
  _notify: function FaceplatePane_notify(topic) {
    var thisPane = this.QueryInterface(Ci.sbIFaceplatePane);
    this._observers.forEach( function (observer) {
      observer.observe(thisPane, topic, null);
    });    
  },
  
  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.sbIFaceplatePane) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}






/**
 * \class FaceplateManager
 * \brief Manages the lifecycle of faceplate panes
 * \sa sbIFaceplateManager
 */
function FaceplateManager() {

  var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  
  // We want to wait till profile-after-change to initialize
  os.addObserver(this, 'profile-after-change', false);

  // We need to unhook things on shutdown
  os.addObserver(this, "quit-application", false);
  
  this._listeners = [];
  this._panes = {};

};
FaceplateManager.prototype = {
  constructor: FaceplateManager,

  _listeners: null,
  _paneCount: 0,
  
  // Map of pane IDs to sbIFaceplatePanes
  _panes: null,
  
  /**
   * Initialize the faceplate manager. Called after profile load.
   * Sets up the intro pane and playback dashboard pane.
   */
  _init: function init() {

    // Come up with some localized names for the default panes.
    var strings = Cc["@mozilla.org/intl/stringbundle;1"]
                  .getService(Ci.nsIStringBundleService)
                  .createBundle("chrome://songbird/locale/songbird.properties"); 
    var getString = function(aStringId, aDefault) {
      try {
        return strings.GetStringFromName(aStringId);
      } catch (e) {
        return aDefault;
      }
    }
    var introName = getString("faceplate.pane.intro.name", "Intro");
    var dashboardName = getString("faceplate.pane.dashboard.name", "Dashboard");
    
    // Create the panes
    this.createPane("songbird-intro", introName, URL_BINDING_DEFAULT_PANE);
    this.createPane("songbird-dashboard", dashboardName, URL_BINDING_DASHBOARD_PANE);
    
    // Set up a dataremote to show the dashboard on first playback.
    // XXX: This needs to change. There shouldn't be an faceplate dataremotes.
    // The PlaylistPlayback service should not know about faceplates.
    var createDataRemote =  new Components.Constructor(
                                   "@songbirdnest.com/Songbird/DataRemote;1",
                                   Components.interfaces.sbIDataRemote, "init");
    
    this._playbackDataRemote = createDataRemote(DATAREMOTE_PLAYBACK, null);
    this._playbackDataRemote.bindObserver(this, true);
  },
  
  /**
   * Called on xpcom-shutdown
   */
  _deinit: function deinit() {
    this._listeners = null;
    this._panes = null;
    if (this._playbackDataRemote) {
      this._playbackDataRemote.unbind();
      this._playbackDataRemote = null;
    }
  },
 
  
  /**
   * \sa sbIFaceplateManager
   */  
  
  get paneCount() {
    return this._paneCount;
  },
    
  createPane:  function FaceplateManager_createPane(aID, aName, aBindingURL) {
    if (!aID || !aName || !aBindingURL) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._panes[aID]) {
      throw Components.results.NS_ERROR_FAILURE;
    }
    
    var pane = new FaceplatePane(aID, aName, aBindingURL);
    this._panes[aID] = pane;
    this._paneCount++;
    
    // Announce this faceplate to the world.  All listening faceplate
    // widgets should receive this message and then instantiate
    // the requested binding
    this._onCreate(pane);
    
    return pane;
  },
    
  showPane: function FaceplateManager_showPane(aPane) {
    if (!aPane || !aPane.id || !this._panes[aPane.id]) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    this._onShow(this._panes[aPane.id]);
  },
  
  destroyPane: function FaceplateManager_destroyPane(aPane) {
    if (!aPane || !aPane.id || !this._panes[aPane.id]) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    var pane = this._panes[aPane.id];
    delete this._panes[pane.id];
    this._paneCount--;
    this._onDestroy(pane);
  },
  
  getPane: function FaceplateManager_getPane(aID) {
    return this._panes[aID];
  },
  
  getPanes: function FaceplateManager_getPanes() {
    // Copy all the faceplates into an array, and then return an enumerator
    return new ArrayEnumerator( [this._panes[key] for (key in this._panes)] ); 
  },
  
  addListener: function FaceplateManager_addListener(aListener) {
    if (! (aListener instanceof Ci.sbIFaceplateManagerListener)) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._listeners.indexOf(aListener) == -1) {
      this._listeners.push(aListener);
    }
  },

  removeListener: function FaceplateManager_removeListener(aListener) {
    var index = this._listeners.indexOf(aListener);
    if (index > -1) {
      this._listeners.splice(index,1);
    }    
  },
    
   
  /**
   * Broadcasts a notification to create the given faceplate pane.
   */
  _onCreate: function FaceplateManager__onCreate(aFaceplatePane) {
    aFaceplatePane = aFaceplatePane.QueryInterface(Ci.sbIFaceplatePane);
    this._listeners.forEach( function (listener) {
      listener.onCreatePane(aFaceplatePane);
    });
  },

  /**
   * Broadcasts a notification to show the given faceplate pane.
   */
  _onShow: function FaceplateManager__onShow(aFaceplatePane) {
    aFaceplatePane = aFaceplatePane.QueryInterface(Ci.sbIFaceplatePane);    
    this._listeners.forEach( function (listener) {
      listener.onShowPane(aFaceplatePane);
    });
  },
  
  /**
   * Broadcasts a notification to destory the given faceplate.
   */
  _onDestroy: function FaceplateManager__onDestroy(aFaceplatePane) {
    aFaceplatePane = aFaceplatePane.QueryInterface(Ci.sbIFaceplatePane);    
    this._listeners.forEach( function (listener) {
      listener.onDestroyPane(aFaceplatePane);
    });
  },
  
  
  /**
   * Cause the playback dashboard pane to show in all 
   * faceplates.  Called by a dataremote when playback 
   * starts for the first time.
   */  
  _showDashboardPane: function FaceplateManager__showDashboardPane() {
    var pane = this.getPane("songbird-dashboard");
    if (pane) {
      this.showPane(pane);
    } else {
      dump("FaceplateManager__showDashboardPane: dashboard not found\n");
    }
  },
  
  
  /**
   * Called by dataremotes and the observer service.
   */
  observe: function(subject, topic, data) {
    var os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
    switch (topic) {
    case "profile-after-change":
      os.removeObserver(this, "profile-after-change");
      this._init();
      break;
    case "quit-application":
      os.removeObserver(this, "quit-application");
      this._deinit();
      break;
    // When playback begins for the first time, jump
    // to the playback dashboard
    case DATAREMOTE_PLAYBACK:
      this._playbackDataRemote.unbind();
      this._playbackDataRemote = null;
      this._showDashboardPane();
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbIFaceplateManager) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // FaceplateManager.prototype






/**
 * \brief XPCOM initialization code
 */
function makeGetModule(CONSTRUCTOR, CID, CLASSNAME, CONTRACTID, CATEGORIES) {
  return function (comMgr, fileSpec) {
    return {
      registerSelf : function (compMgr, fileSpec, location, type) {
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(CID,
                        CLASSNAME,
                        CONTRACTID,
                        fileSpec,
                        location,
                        type);
        if (CATEGORIES && CATEGORIES.length) {
          var catman =  Cc["@mozilla.org/categorymanager;1"]
              .getService(Ci.nsICategoryManager);
          for (var i=0; i<CATEGORIES.length; i++) {
            var e = CATEGORIES[i];
            catman.addCategoryEntry(e.category, e.entry, e.value, 
              true, true);
          }
        }
      },

      getClassObject : function (compMgr, cid, iid) {
        if (!cid.equals(CID)) {
          throw Cr.NS_ERROR_NO_INTERFACE;
        }

        if (!iid.equals(Ci.nsIFactory)) {
          throw Cr.NS_ERROR_NOT_IMPLEMENTED;
        }

        return this._factory;
      },

      _factory : {
        createInstance : function (outer, iid) {
          if (outer != null) {
            throw Cr.NS_ERROR_NO_AGGREGATION;
          }
          return (new CONSTRUCTOR()).QueryInterface(iid);
        }
      },

      unregisterSelf : function (compMgr, location, type) {
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        compMgr.unregisterFactoryLocation(CID, location);
        if (CATEGORIES && CATEGORIES.length) {
          var catman =  Cc["@mozilla.org/categorymanager;1"]
              .getService(Ci.nsICategoryManager);
          for (var i=0; i<CATEGORIES.length; i++) {
            var e = CATEGORIES[i];
            catman.deleteCategoryEntry(e.category, e.entry, true);
          }
        }
      },

      canUnload : function (compMgr) {
        return true;
      },

      QueryInterface : function (iid) {
        if ( !iid.equals(Ci.nsIModule) ||
             !iid.equals(Ci.nsISupports) )
          throw Cr.NS_ERROR_NO_INTERFACE;
        return this;
      }

    };
  }
}

var NSGetModule = makeGetModule (
  FaceplateManager,
  Components.ID("{eb5c665a-bfe2-49f0-a747-cd3554e55606}"),
  "Songbird Faceplate Pane Manager Service",
  "@songbirdnest.com/faceplate/manager;1",
  [{
    category: 'app-startup',
    entry: 'faceplate-pane-manager',
    value: 'service,@songbirdnest.com/faceplate/manager;1'
  }]);
