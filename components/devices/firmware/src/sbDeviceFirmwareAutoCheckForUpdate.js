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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBTimer.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const NS_QUIT_APPLICATION_GRANTED_TOPIC = "quit-application-granted";
const NS_TIMER_CALLBACK_TOPIC           = "timer-callback";
const SB_FINAL_UI_STARTUP_TOPIC         = "final-ui-startup";
const SB_TIMER_MANAGER_PREFIX           = "songbird-device-firmware-update-";

const FIRMWARE_WIZARD_ACTIVE_DATAREMOTE = "firmware.wizard.active";

function DEBUG(msg) {
  return;

  function repr(x) {
    if (typeof(x) == "undefined") {
      return 'undefined';
    } else if (x == null) {
      return 'null';
    } else if (typeof x == 'function') {
      return x.name+'(...)';
    } else if (typeof x == 'string') {
      return x.toSource().match(/^\(new String\((.*)\)\)$/)[1];
    } else if (typeof x == 'number') {
      return x.toSource().match(/^\(new Number\((.*)\)\)$/)[1];
    } else if (typeof x == 'object' && x instanceof Array) {
      var value = '';
      for (var i=0; i<x.length; i++) {
        if (i) value = value + ', ';
        value = value + repr(x[i]);
      }
      return '['+value+']';
    } else if (x instanceof Ci.nsISupports) {
      return x.toString();
    } else {
      return x.toSource();
    }
  }
  dump('sbDeviceFirmwareAutocheckForUpdate:: '+DEBUG.caller.name);
  if (typeof(msg) == "undefined") {
    // when nothing is passed in, print the arguments
    dump('(');
    for (var i=0; i<DEBUG.caller.length; i++) {
      if (i) dump(', ');
      dump(repr(DEBUG.caller.arguments[i]));
    }
    dump(')');
  } else {
    dump(': ');
    if (typeof msg != 'object' || msg instanceof Array) {
      dump(repr(msg));
    } else {
      dump(msg.toSource());
    }
  }
  dump('\n');
}


function sbDeviceFirmwareAutoCheckForUpdate() {
  DEBUG();

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, SB_FINAL_UI_STARTUP_TOPIC, false);
}

/** a cached reference to the device firmware update service */
sbDeviceFirmwareAutoCheckForUpdate.prototype._deviceFirmwareUpdater = null;
/** a cached reference to the device manager service */
sbDeviceFirmwareAutoCheckForUpdate.prototype._deviceManager = null;
/** a set where the keys are the ids of devices that have registered timers */
sbDeviceFirmwareAutoCheckForUpdate.prototype._registeredDevices = {};
/** the queue of devices currently needing a firmware update check */
sbDeviceFirmwareAutoCheckForUpdate.prototype._queue = [];
/** the device currenly being checked for a firmware update */
sbDeviceFirmwareAutoCheckForUpdate.prototype._queueItem = null;
/** whether the current item in the queue has been successfully checked */
sbDeviceFirmwareAutoCheckForUpdate.prototype._queueItemSuccess = false;
/** timer used to manage firmware update queue */
sbDeviceFirmwareAutoCheckForUpdate.prototype._timer = null;
/** the timer manager service (long-running timer service) */
sbDeviceFirmwareAutoCheckForUpdate.prototype._timerManager = null;

sbDeviceFirmwareAutoCheckForUpdate.prototype.classDescription =
    'Songbird Device Firmware Auto Check For Update';
sbDeviceFirmwareAutoCheckForUpdate.prototype.classID =
    Components.ID("{2137a87f-2ade-448b-a093-bad4f6649fa3}");
sbDeviceFirmwareAutoCheckForUpdate.prototype.contractID =
    '@songbirdnest.com/Songbird/Device/Firmware/AutoCheckForUpdate;1';
sbDeviceFirmwareAutoCheckForUpdate.prototype.flags = Ci.nsIClassInfo.SINGLETON;
sbDeviceFirmwareAutoCheckForUpdate.prototype.interfaces =
    [Ci.nsISupports, Ci.nsIClassInfo, Ci.nsIObserver, Ci.sbIDeviceEventListener];
sbDeviceFirmwareAutoCheckForUpdate.prototype.getHelperForLanguage = Function();
sbDeviceFirmwareAutoCheckForUpdate.prototype.getInterfaces =
function sbDeviceFirmwareAutoCheckForUpdate_getInterfaces(count) {
  count.value = this.interfaces.length;
  return this.interfaces;
}
sbDeviceFirmwareAutoCheckForUpdate.prototype.QueryInterface =
    XPCOMUtils.generateQI(sbDeviceFirmwareAutoCheckForUpdate.prototype.interfaces);

/** nsIObserver */
sbDeviceFirmwareAutoCheckForUpdate.prototype.observe =
function sbDeviceFirmwareAutoCheckForUpdate_observe(subject, topic, data) {
  DEBUG();

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);

  if (topic == SB_FINAL_UI_STARTUP_TOPIC) {
    obs.removeObserver(this, SB_FINAL_UI_STARTUP_TOPIC);
    obs.addObserver(this, NS_QUIT_APPLICATION_GRANTED_TOPIC, false);

    this._deviceFirmwareUpdater = 
      Cc['@songbirdnest.com/Songbird/Device/Firmware/Updater;1']
        .getService(Ci.sbIDeviceFirmwareUpdater);
        
    this._deviceManager =
      Cc['@songbirdnest.com/Songbird/DeviceManager;2']
        .getService(Ci.sbIDeviceManager2);
        
    this._deviceManager.addEventListener(this);
    
  } else if (topic == NS_QUIT_APPLICATION_GRANTED_TOPIC) {
    // cleanup
    obs.removeObserver(this, NS_QUIT_APPLICATION_GRANTED_TOPIC);

    if(this._deviceManager) {
      this._deviceManager.removeEventListener(this);
    }
    
    if (this._timer) {
      this._clearTimer();
    }

    if (this._timerManager) {
      for (let id in this._registeredDevices) {
        this._unregisterTimer(id);
      }
    }

    if (this._queueItem && this._deviceFirmwareUpdater) {
      this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
    }

  } else if (topic == NS_TIMER_CALLBACK_TOPIC) {
    DEBUG(this._queue.length + " items, top is " + this._queueItem);
    if(this._queue.length &&
       !this._queueItem) {
      // Queue has items in it and we're not currently processing an item.
      // Grab first device in the queue.
      let device = this._queue[0];
      this._queueItem = device;
      // Check for update
      if (device.getPreference("firmware.update.enabled")) {
        this._deviceFirmwareUpdater.checkForUpdate(device, this);
      }
    }
    else if(this._queueItem && 
            this._queueItemSuccess) {
      // If the device is busy, wait for the next timer callback.
      if(this._queueItem.isBusy) {
        return;
      }
      
      this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
      
      let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Ci.nsIWindowMediator);
      let parent = wm.getMostRecentWindow("Songbird:Main");

      WindowUtils.openModalDialog
        (parent,
         "chrome://songbird/content/xul/device/deviceFirmwareWizard.xul",
         "device_firmware_dialog",
         "",
         ["", "defaultDevice=false", this._queueItem ],
         null);

      this._queue.splice(0, 1);
      this._queueItem = null;
      this._queueItemSuccess = false;
    }
    else if(!this._queue.length &&
            this._timer) {
      // Queue is empty, clear timer.
      this._timer.cancel();
      this._timer = null;
    }
  }
}

sbDeviceFirmwareAutoCheckForUpdate.prototype.notify =
function sbDeviceFirmwareAutoCheckForUpdate_notify(aDevice) {
  DEBUG();
  if (!this._timer) {
    // set up the timer
    this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    this._timer.init(this, 5000, Ci.nsITimer.TYPE_REPEATING_SLACK);
  }
  if (this._queue.indexOf(aDevice) < 0) {
    this._queue.push(aDevice);
  }
}

/** sbIDeviceEventListener */
sbDeviceFirmwareAutoCheckForUpdate.prototype.onDeviceEvent =
function sbDeviceFirmwareAutoCheckForUpdate_onDeviceEvent(aEvent) {
  //DEBUG();
  var device = null;

  switch(aEvent.type) {
    case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
      device = aEvent.data.QueryInterface(Ci.sbIDevice);
      if (device.getPreference("firmware.update.enabled")) {
        this._registerTimer(device);
      }
      
      // if we have a firmware handler for this device, we'll
      // also check to see if it's recovery mode and ask the 
      // user if they wish to repair it if it is.
      if (this._deviceFirmwareUpdater.hasHandler(device) &&
          !SBDataGetBoolValue(FIRMWARE_WIZARD_ACTIVE_DATAREMOTE)) {
        var handler = this._deviceFirmwareUpdater.getHandler(device);
        handler.bind(device, null);
        
        var recoveryMode = handler.recoveryMode;
        handler.unbind();
        
        // Also check to make sure we're not currently updating
        // other firmware, if so, don't do anything for this device
        if (recoveryMode) {
          var self = this;
          SBUtils.deferFunction(
            function() { 
              self._promptForRepair(device); 
            });
        }
      }
    }
    break;

    case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED: {
      // Device removed, if in queue, remove it, also finalize any update attempt.
      device = aEvent.data.QueryInterface(Ci.sbIDevice);
      this._unregisterTimer(device);
      let index = this._queue.indexOf(device);
      if (index < 0) {
        break;
      }
      this._queue.splice(index, 1);
      if(this._queueItem == device) {
        // currently attempting this item
        this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
        this._queueItem = null;
        this._queueItemSuccess = false;
      }
    }
    break;

    case Ci.sbIDeviceEvent.EVENT_DEVICE_PREFS_CHANGED: {
      device = aEvent.origin.QueryInterface(Ci.sbIDevice);
      if (device.getPreference("firmware.update.enabled")) {
        this._registerTimer(device);
      } else {
        this._unregisterTimer(device);
      }
    }
    break;

    case Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_END: {
      this._queueItemSuccess = aEvent.data;
    }
    break;
    
    case Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_ERROR: {
      // Remove from queue
      device = aEvent.origin.QueryInterface(Ci.sbIDevice);
      let index = this._queue.indexOf(device);
      if(index < 0) {
        break;
      }
      this._queue.splice(index, 1);
      
      if(this._queueItem == device) {
        this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
        this._queueItem = null;
        this._queueItemSuccess = false;
      }
    }
    break;
  }
}

/**
 * Registers a device with the timer manager
 */
sbDeviceFirmwareAutoCheckForUpdate.prototype._registerTimer =
function sbDeviceFirmwareAutoCheckForUpdate__registerTimer(aDevice) {
  DEBUG();
  if (!this._timerManager) {
    this._timerManager = Cc['@mozilla.org/updates/timer-manager;1']
                           .getService(Ci.nsIUpdateTimerManager);
  }
  let self = this;
  let callback = function(aTimer) {
    self.notify(aDevice);
  };
  let interval = aDevice.getPreference("firmware.update.interval");
  if (!interval) {
    interval = 60 * 60 * 24 * 7; // default to 7 days
  }
  this._timerManager.registerTimer(SB_TIMER_MANAGER_PREFIX + aDevice.id,
                                   callback,
                                   interval);
  this._registeredDevices[aDevice.id] = true;
}
/**
 * Unegisters a with the timer manager for the given device
 */
sbDeviceFirmwareAutoCheckForUpdate.prototype._unregisterTimer =
function sbDeviceFirmwareAutoCheckForUpdate__unregisterTimer(aDevice) {
  DEBUG();
  let id = aDevice;
  if ("id" in aDevice) {
    id = aDevice.id;
  }
  if (!this._registeredDevices[id]) {
    // this device was never registerd. probably means update was disabled.
    return;
  }
  delete this._registeredDevices[id];
  // we can't actually unregister timers; instead, we can register it for
  // a really long time (here, 136 years) and hope it doesn't get called
  if (this._timerManager) {
    this._timerManager.registerTimer(SB_TIMER_MANAGER_PREFIX + id,
                                     null,
                                     -1);
  }
}

sbDeviceFirmwareAutoCheckForUpdate.prototype._promptForRepair = 
function sbDeviceFirmwareAutoCheckForUpdate__promptForRepair(aDevice) {
  var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                         .getService(Ci.nsIWindowMediator);
  var songbirdWindow = windowMediator.getMostRecentWindow("Songbird:Main");  

  var prompter = Cc['@songbirdnest.com/Songbird/Prompter;1']
                   .getService(Components.interfaces.sbIPrompter);
  var confirmed = 
    prompter.confirm(songbirdWindow,
                     SBString('device.firmware.corrupt.title'),
                     SBFormattedString('device.firmware.corrupt.message',
                     [aDevice.name]));
  if (confirmed) {
    WindowUtils.openModalDialog
                (songbirdWindow,
                 "chrome://songbird/content/xul/device/deviceFirmwareWizard.xul",
                 "device_firmware_dialog",
                 "",
                 [ "mode=repair", "defaultDevice=false", aDevice ],
                 null);  
  }
}

var NSGetModule = XPCOMUtils.generateNSGetModule(
  [
    sbDeviceFirmwareAutoCheckForUpdate,
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      sbDeviceFirmwareAutoCheckForUpdate.prototype.classDescription,
      "service," + sbDeviceFirmwareAutoCheckForUpdate.prototype.contractID,
      true,
      true);
  }
);
