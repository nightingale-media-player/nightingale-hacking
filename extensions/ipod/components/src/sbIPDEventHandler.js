/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

/**
 * \file  sbIPDEventHandler.js
 * \brief Javascript source for the iPod device event handler component.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device event handler component.
//
//   This component listens for and handle iPod device specific events.  Event
// handling includes presenting the user with dialogs.
//   This component must be instantiated as a service once the device manager is
// ready.  This may be done from the iPod device object initialization.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// iPod device event handler imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// iPod device event handler defs.
//
//------------------------------------------------------------------------------

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;


//------------------------------------------------------------------------------
//
// iPod device event handler configuration.
//
//------------------------------------------------------------------------------

//
// className                    Name of component class.
// cid                          Component CID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// categoryList                 List of component categories.
//
// localeBundlePath             Path to locale string bundle.
//

var sbIPDEventHandlerCfg= {
  className: "Songbird iPod Device Event Handler Service",
  cid: Components.ID("{adb505cb-75d9-4526-84df-5b69d6c571e9}"),
  contractID: "@songbirdnest.com/Songbird/IPDEventHandler;1",
  ifList: [ Ci.nsIObserver, Ci.sbIDeviceEventListener ],
  categoryList: [],

  localeBundlePath: "chrome://ipod/locale/IPodDevice.properties"
};


//------------------------------------------------------------------------------
//
// iPod device event handler services.
//
//   These services handle iPod device specific events.
//   Processing of some events require the opening of dialog windows.  While a
// dialog window is open, device events may still be sent, causing these
// services to be re-entered.  In order to ensure processing of events in the
// proper order, an event queue is used internally.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device event handler object.
 */

function sbIPDEventHandler()
{
  // Initialize the services.
  this._initialize();
}

/* Set the constructor. */
sbIPDEventHandler.prototype.constructor = sbIPDEventHandler;

sbIPDEventHandler.prototype = {
  //
  // iPod device event handler services fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _locale                  Locale string bundle.
  //   _observerSvc             Observer service object.
  //   _devMgr                  Device manager object.
  //   _devInfoList             List of device information.
  //   _eventQueue              Queue of received device events.
  //   _eventQueueBusy          True if busy processing device event queue.
  //

  classDescription: sbIPDEventHandlerCfg.className,
  classID: sbIPDEventHandlerCfg.cid,
  contractID: sbIPDEventHandlerCfg.contractID,
  _xpcom_categories: sbIPDEventHandlerCfg.categoryList,

  _cfg: sbIPDEventHandlerCfg,
  _locale: null,
  _observerSvc: null,
  _devMgr: null,
  _devInfoList: null,
  _eventQueue: [],
  _eventQueueBusy: false,


  //----------------------------------------------------------------------------
  //
  // iPod device event handler sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * aEvent - information about the event
   */

  onDeviceEvent: function sbIPDEventHandler_onDeviceEvent(aEvent) {
    try { this._onDeviceEvent(aEvent); }
    catch (ex) { dump("onDeviceEvent exception: " + ex +
                      " at " + ex.fileName +
                      ", line " + ex.lineNumber + "\n"); }
  },

  _onDeviceEvent: function sbIPDEventHandler__onDeviceEvent(aEvent) {
    // Enqueue the device event and process the event queue.
    this._eventQueue.push(aEvent);
    this._processEventQueue();
  },


  //----------------------------------------------------------------------------
  //
  // iPod device event handler nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbIPDEventHandler_observe(aSubject, aTopic, aData) {
    try { this._observe(aSubject, aTopic, aData); }
    catch (ex) { dump("observe exception: " + ex + "\n"); }
  },

  _observe: function sbIPDEventHandler__observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "quit-application" :
        this._handleAppQuit();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // iPod device event handler nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbIPDEventHandlerCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // iPod device event handler event services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle application quit events.
   */

  _handleAppQuit: function sbIPDEventHandler__handleAppQuit() {
    // Finalize the services.
    this._finalize();
  },


  /**
   * Process the device events in the device event queue.
   */

  _processEventQueue: function sbIPDEventHandler__processEventQueue() {
    // Do nothing if already busy processing event queue.
    if (this._eventQueueBusy)
      return;

    // Process events in the queue.
    this._eventQueueBusy = true;
    while (this._eventQueue.length > 0) {
      var event = this._eventQueue.shift();
      this._handleEvent(event);
    }
    this._eventQueueBusy = false;
  },


  /**
   * Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  _handleEvent: function sbIPDEventHandler__handleEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED :
        this._addDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        this._removeDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIIPDDeviceEvent.EVENT_IPOD_FAIRPLAY_NOT_AUTHORIZED :
        this._handleFairPlayNotAuthorized(aEvent);
        break;

      case Ci.sbIIPDDeviceEvent.EVENT_IPOD_NOT_INITIALIZED:
        this._handleIPodNotInitialized(aEvent);
        break;

      case Ci.sbIIPDDeviceEvent.EVENT_IPOD_UNSUPPORTED_FILE_SYSTEM:
        this._handleIPodUnsupportedFileSystem(aEvent);
        break;

      case Ci.sbIIPDDeviceEvent.EVENT_IPOD_HFSPLUS_READ_ONLY:
        this._handleIPodHFSPlusReadOnly(aEvent);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END :
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_WRITE_END :
        this._processEventInfo(aEvent.origin.QueryInterface(Ci.sbIDevice));
        break;

      default :
        break;
    }
  },


  /**
   * Handle the FairPlay not authorized event specified by aEvent.
   *
   * \param aEvent              FairPlay not authorized event.
   */

  _handleFairPlayNotAuthorized:
    function sbIPDEventHandler__handleFairPlayNotAuthorized(aEvent) {
    // Get the event info.
    var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
    var fairPlayEvent = aEvent.QueryInterface(Ci.sbIIPDFairPlayEvent);

    // Get the device info.  Do nothing more if no device info.
    var devInfo = this._devInfoList[device.id];
    if (!devInfo)
      return;
    if (!devInfo.fairPlayNotAuthorizedInfoList)
      devInfo.fairPlayNotAuthorizedInfoList = [];
    fairPlayNotAuthorizedInfoList = devInfo.fairPlayNotAuthorizedInfoList;

    // Accumulate the event information for later processing.
    var fairPlayNotAuthorizedInfo =
          {
            userID: fairPlayEvent.userID,
            accountName: fairPlayEvent.accountName,
            userName: fairPlayEvent.userName,
            mediaItem: fairPlayEvent.mediaItem
          };
    fairPlayNotAuthorizedInfoList.push(fairPlayNotAuthorizedInfo);
  },


  /**
   * Handle the iPod not initialized event, specified by aEvent
   *
   * \param aEvent              Event
   */

  _handleIPodNotInitialized:
    function sbIPDEventHandler__handleIPodNotInitialized(aEvent) {
    function promptForDeviceName(aWindow) {
      var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
        .getService(Ci.nsIStringBundleService)
        .createBundle('chrome://ipod/locale/IPodDevice.properties');

      var deviceName = { value:
        stringBundle.GetStringFromName('initialize.default_device_name') };

      var promptService = Cc["@songbirdnest.com/Songbird/Prompter;1"]
        .getService(Ci.nsIPromptService);
      var accept = promptService.prompt(aWindow,
          stringBundle.GetStringFromName('initialize.title'),
          stringBundle.GetStringFromName('initialize.prompt'),
          deviceName, null, {});
      if (accept) {
        aEvent.data.QueryInterface(Ci.sbIDevice).properties.friendlyName
          = deviceName.value;
      }
    }
    var sbWindowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
      .getService(Ci.sbIWindowWatcher);
    sbWindowWatcher.callWithWindow("Songbird:Main", promptForDeviceName, false);
  },


  /**
   * Handle the unsupported file system event specified by aEvent.
   *
   * \param aEvent              Unsupported file system event.
   */

  _handleIPodUnsupportedFileSystem:
    function sbIPDEventHandler__handleIPodUnsupportedFileSystem(aEvent) {
    // Set up the dialog parameters.
    var title = SBString("ipod.dialog.unsupported_file_system.title",
                         null,
                         this._locale);
    var msg = SBString("ipod.dialog.unsupported_file_system.msg",
                       null,
                       this._locale);

    // Present the user with a dialog using the main Songbird window as the
    // parent.
    var dialogFunc = function(aWindow) {
      var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                       .getService(Ci.nsIPromptService);
      prompter.alert(aWindow, title, msg);
    };
    var sbWindowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                            .getService(Ci.sbIWindowWatcher);
    sbWindowWatcher.callWithWindow("Songbird:Main", dialogFunc, false);
  },


  /**
   * Handle the read only hfsplus event specified by aEvent.
   *
   * \param aEvent              Unsupported file system event.
   */

  _handleIPodHFSPlusReadOnly:
    function sbIPDEventHandler__handleIPodUnsupportedFileSystem(aEvent) {
    // Set up the dialog parameters.
    var title = SBString("ipod.dialog.hfsplus_read_only.title",
                         null,
                         this._locale);
    var msg = SBString("ipod.dialog.hfsplus_read_only.msg",
                       null,
                       this._locale);

    // Present the user with a dialog using the main Songbird window as the
    // parent.
    var dialogFunc = function(aWindow) {
      var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                       .getService(Ci.nsIPromptService);
      prompter.alert(aWindow, title, msg);
    };
    var sbWindowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                            .getService(Ci.sbIWindowWatcher);
    sbWindowWatcher.callWithWindow("Songbird:Main", dialogFunc, false);
  },


  /**
   * Process the accumulated event information for the device specified by
   * aDevice.
   *
   * \param aDevice             Device for which to process event information.
   */

  _processEventInfo: function sbIPDEventHandler__processEventInfo(aDevice) {
    // Process accumulated event info.
    this._processFairPlayNotAuthorizedEventInfo(aDevice);
  },


  /**
   * Process the accumulated FairPlay not authorized event information for the
   * device specified by aDevice.
   *
   * \param aDevice             Device for which to process event information.
   */

  _processFairPlayNotAuthorizedEventInfo:
    function sbIPDEventHandler__processFairPlayNotAuthorizedEventInfo(aDevice) {
    // Get the device info.  Do nothing more if no device info.
    var devInfo = this._devInfoList[aDevice.id];
    if (!devInfo)
      return;

    // Get the FairPlay not authorized event info.  Do nothing more if not
    // present.
    fairPlayNotAuthorizedInfoList = devInfo.fairPlayNotAuthorizedInfoList;
    if (!fairPlayNotAuthorizedInfoList)
      return;

    // Collect the FairPlay not authorized event track info.
    var trackInfoList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                          .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < fairPlayNotAuthorizedInfoList.length; i++) {
      // Get the FairPlay not authorized info.
      var fairPlayNotAuthorizedInfo = fairPlayNotAuthorizedInfoList[i];
      var trackContentURL = fairPlayNotAuthorizedInfo
                              .mediaItem.contentSrc.QueryInterface(Ci.nsIURL);

      // Produce the track info string.
      var trackInfo = Cc["@mozilla.org/supports-string;1"]
                        .createInstance(Ci.nsISupportsString);
      trackInfo.data = fairPlayNotAuthorizedInfo.accountName;
      trackInfo.data += ": " + decodeURIComponent(trackContentURL.fileName);

      // Add the track info to the track info list.
      trackInfoList.appendElement(trackInfo, false);
    }

    // Clear the FairPlay not authorized event info.
    devInfo.fairPlayNotAuthorizedInfoList = null;

    // Set up the dialog parameters.
    var title = SBString("ipod.dialog.fair_play_not_authorized_transfer.title",
                         null,
                         this._locale);
    var errorMsg =
          SBFormattedString("ipod.dialog.fair_play_not_authorized_transfer.msg",
                            [ aDevice.properties.friendlyName ],
                            null,
                            this._locale);
    var listLabel =
          SBFormattedCountString
            ("ipod.dialog.fair_play_not_authorized_transfer.list_label",
             fairPlayNotAuthorizedInfoList.length,
             null,
             null,
             this._locale);

    // Present the user with a dialog using the main Songbird window as the
    // parent.
    var dialogFunc = function(aWindow) {
      WindowUtils.openModalDialog
        (aWindow,
         "chrome://songbird/content/xul/device/deviceErrorDialog.xul",
         "device_error_dialog",
         "",
         [ "windowTitle=" + title +
             ",listLabel=" + listLabel +
             ",errorMsg=" + errorMsg,
           aDevice,
           trackInfoList ],
         null,
         null);
    };
    var sbWindowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                            .getService(Ci.sbIWindowWatcher);
    sbWindowWatcher.callWithWindow("Songbird:Main", dialogFunc, false);
  },


  //----------------------------------------------------------------------------
  //
  // iPod device event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbIPDEventHandler__initialize() {
    // Initialize the device info list.
    this._devInfoList = {};

    // Get the locale string bundle.
    var stringBundleSvc = Cc["@mozilla.org/intl/stringbundle;1"]
                            .getService(Ci.nsIStringBundleService);
    this._locale = stringBundleSvc.createBundle(this._cfg.localeBundlePath);

    // Set up observers.
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    this._observerSvc.addObserver(this, "quit-application", false);

    // Get the device manager.
    this._devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                     .getService(Ci.sbIDeviceManager2);

    // Add device event listener.
    this._devMgr.addEventListener(this);

    // Add all connected devices.
    this._addAllConnectedDevices();
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbIPDEventHandler__finalize() {
    // Remove observers.
    try {
      this._observerSvc.removeObserver(this, "quit-application");
    } catch(ex) {}

    // Remove device event listener.
    this._devMgr.removeEventListener(this);

    // Remove all devices.
    this._removeAllDevices();

    // Clear the device info list.
    this._devInfoList = null;

    // Clear object references.
    this._observerSvc = null;
    this._devMgr = null;
  },


  /**
   * Add the device specified by aDevice.
   *
   * \param aDevice             The device object.
   */

  _addDevice: function sbIPDEventHandler__addDevice(aDevice) {
    // Do nothing if device has already been added.
    var deviceID = aDevice.id;
    if (this._devInfoList[deviceID])
      return;

    // Do nothing if device is not an iPod.
    if (aDevice.parameters.getProperty("DeviceType") != "iPod")
      return;

    // Do nothing if device is not connected.
    if (!aDevice.connected)
      return;

    // Create a device info list entry.
    this._devInfoList[deviceID] = { device: aDevice };
  },


  /**
   * Remove the device specified by aDevice.
   *
   * \param aDevice             The device object.
   */

  _removeDevice: function sbIPDEventHandler__removeDevice(aDevice) {
    // Get the device info.  Do nothing if no device info available.
    var deviceID = aDevice.id;
    var devInfo = this._devInfoList[deviceID];
    if (!devInfo)
      return;

    // Remove device info list entry.
    delete this._devInfoList[deviceID];
  },


  /**
   * Add all connected devices.
   */

  _addAllConnectedDevices: function
                             sbIPDEventHandler__addAllConnectedDevices() {
    var deviceList = ArrayConverter.JSArray(this._devMgr.devices);
    for each (device in deviceList) {
      this._addDevice(device.QueryInterface(Ci.sbIDevice));
    }
  },


  /**
   * Remove all devices.
   */

  _removeAllDevices: function sbIPDEventHandler__removeAllDevices() {
    // Remove all devices.
    for (var deviceID in this._devInfoList)
      this._removeDevice(this._devInfoList[deviceID].device);
  }
};


//------------------------------------------------------------------------------
//
// iPod device event handler component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbIPDEventHandler]);
}

