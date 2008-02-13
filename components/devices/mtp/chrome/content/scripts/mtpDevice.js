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

Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
  
var mtpCore = {
  _deviceEventListener: null,
  _deviceManager:       null,
  _observerSvc:         null,
  _promptSvc:           null,
  _stringBundle:        null,
  _stringBundleSvc:     null,
  
  // ************************************
  // nsIObserver implementation
  // ************************************
  observe: function mtpCore_observe(aSubject, 
                                    aTopic, 
                                    aData) {
    switch (aTopic) {
      case this._cfg.appQuitTopic :
        this._shutdown();
      break;
    }
  },
  
  // ************************************
  // Internal methods
  // ************************************
  _initialize: function mtpCore_initialize() {
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    this._observerSvc.addObserver(this, "quit-application", false);

    this._deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceManager2);
    
    var deviceEventListener = {
      mtpCore: this,
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        this.mtpCore._processDeviceEvent(aDeviceEvent);
      }
    };

    this._deviceEventListener = deviceEventListener;
    this._deviceManager.addEventListener(deviceEventListener);
    
    this._promptSvc = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);
                        
    this._stringBundleSvc = Cc["@mozilla.org/intl/stringbundle;1"]
                              .getService(Ci.nsIStringBundleService);
    this._stringBundle = 
      this._stringBundleSvc
          .createBundle( "chrome://songbird/locale/songbird.properties" );
    
  },
  
  _shutdown: function mtpCore_shutdown() {
    this._observerSvc.removeObserver(this, "quit-application");
    
    this._deviceManagerSvc.removeEventListener(this._deviceEventListener);
    this._deviceEventListener = null;

    this._observerSvc = null;
    this._deviceManager = null;
    this._promptSvc = null;
    
    this._stringBundle = null;
    this._stringBundleSvc = null;
  },
  
  _processDeviceEvent: function mtpCore_processDeviceEvent(aDeviceEvent) {
    
    // For now, we only process events coming from sbIDevice.
    if(!(aDeviceEvent.origin instanceof Ci.sbIDevice))
      return;
    
    var applicationName = this._stringBundle.getStringFromName("brandShortName");

    var errorTitle = "";
    var errorMsg = "";
    
    var device = aDeviceEvent.origin.QueryInterface(Ci.sbIDevice);
    
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Ci.nsIWindowMediator);
    var mainWindow = windowMediator.getMostRecentWindow("Songbird:Main");
    
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ACCESS_DENIED: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.access_denied.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.access_denied.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_ENOUGH_FREESPACE: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.not_enough_freespace.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.not_enough_freespace.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_AVAILABLE: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.not_available.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.not_avaialble.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
    }
  },
};

mtpCore._initialize();
