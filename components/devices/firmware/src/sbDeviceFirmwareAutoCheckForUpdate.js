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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const NS_QUIT_APPLICATION_GRANTED_TOPIC = "quit-application-granted";
const NS_TIMER_CALLBACK_TOPIC           = "timer-callback";
const SB_FINAL_UI_STARTUP_TOPIC         = "final-ui-startup";

function DEBUG(msg) {
  return;

  function repr(x) {
    if (x == undefined) {
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
  dump('DEBUG '+DEBUG.caller.name);
  if (msg == undefined) {
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

sbDeviceFirmwareAutoCheckForUpdate.prototype._deviceFirmwareUpdater = null;
sbDeviceFirmwareAutoCheckForUpdate.prototype._deviceManager = null;
sbDeviceFirmwareAutoCheckForUpdate.prototype._queue = [];
sbDeviceFirmwareAutoCheckForUpdate.prototype._queueItem = null;
sbDeviceFirmwareAutoCheckForUpdate.prototype._queueItemSuccess = false;
sbDeviceFirmwareAutoCheckForUpdate.prototype._timer = null;

sbDeviceFirmwareAutoCheckForUpdate.prototype.classDescription =
    'Songbird Device Firmware Auto Check For Update';
sbDeviceFirmwareAutoCheckForUpdate.prototype.classID =
    Components.ID("{2137a87f-2ade-448b-a093-bad4f6649fa3}");
sbDeviceFirmwareAutoCheckForUpdate.prototype.contractID = '@songbirdnest.com/Songbird/Device/Firmware/AutoCheckForUpdate;1';
sbDeviceFirmwareAutoCheckForUpdate.prototype.flags = Ci.nsIClassInfo.SINGLETON;
sbDeviceFirmwareAutoCheckForUpdate.prototype.interfaces =
    [Ci.nsISupports, Ci.nsIClassInfo, Ci.nsIObserver, Ci.sbIDeviceEventListener];
sbDeviceFirmwareAutoCheckForUpdate.prototype.getHelperForLanguage = function(x) { return null; }
sbDeviceFirmwareAutoCheckForUpdate.prototype.getInterfaces =
function sbDeviceFirmwareAutoCheckForUpdate_getInterfaces(count, array) {
  array.value = this.interfaces;
  count.value = array.value.length;
}
sbDeviceFirmwareAutoCheckForUpdate.prototype.QueryInterface =
    XPCOMUtils.generateQI(sbDeviceFirmwareAutoCheckForUpdate.prototype.interfaces);

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
    obs.removeObserver(this, NS_QUIT_APPLICATION_GRANTED_TOPIC);

    if(this._deviceManager) {
      this._deviceManager.removeEventListener(this);
    }
    
    if (this._timer) {
      this._clearTimer();
    }
    
  } else if (topic == NS_TIMER_CALLBACK_TOPIC) {
    if(this._queue.length &&
       !this._queueItem) {
      // Queue has items in it and we're not currently processing an item.
      // Grab first device in the queue.
      let device = this._queue[0];
      this._queueItem = device;
      // Check for update
      this._deviceFirmwareUpdater.checkForUpdate(device, this);
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
         ["", this._queueItem ],
         null);

      this._queue.splice(0, 1);
      this._queueItem = null;
      this._queueItemSuccess = false;
    }
    else if(!this._queue.length &&
            this._timer){
      // Queue is empty, clear timer.
      this._clearTimer();
    }
  }
}

sbDeviceFirmwareAutoCheckForUpdate.prototype.onDeviceEvent =
function sbDeviceFirmwareAutoCheckForUpdate_onDeviceEvent(aEvent) {
  //DEBUG();
  var device = null;
  
  switch(aEvent.type) {
    case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
      device = aEvent.data.QueryInterface(Ci.sbIDevice);
      this._queue.push(device);
      this._setUpTimer();
    }
    break;
    
    case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED: {
      // Device removed, if in queue, remove it, also finalize any update attempt.
      device = aEvent.data.QueryInterface(Ci.sbIDevice);
      let index = this._queue.indexOf(device);
      if(index > -1) {
        this._queue.splice(index, 1);
      }
      if(this._queueItem) {
        this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
        this._queueItem = null;
        this._queueItemSuccess = false;
      }   
    }
    break;
    
    case Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_END: {
      this._queueItemSuccess = aEvent.data;
    }
    break;
    
    case Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_ERROR: {
      // Remove from queue
      this._queue.splice(0, 1);
      this._deviceFirmwareUpdater.finalizeUpdate(this._queueItem);
      this._queueItem = null;
      this._queueItemSuccess = false;
    }
    break;
  }
}

sbDeviceFirmwareAutoCheckForUpdate.prototype._setUpTimer =
function sbDeviceFirmwareAutoCheckForUpdate__setUpTimer() {
  DEBUG();
  if (!this._timer) {
    // set up the timer
    this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    this._timer.init(this, 5000, Ci.nsITimer.TYPE_REPEATING_SLACK);
  }
}
sbDeviceFirmwareAutoCheckForUpdate.prototype._clearTimer =
function sbDeviceFirmwareAutoCheckForUpdate__clearTimer() {
  DEBUG();
  if (this._timer) {
    this._timer.cancel();
    this._timer = null;
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
