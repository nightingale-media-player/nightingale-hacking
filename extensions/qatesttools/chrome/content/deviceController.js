/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;

const CONNECT_BUTTON_LABEL    = "Add and Connect Mock Device";
const DISCONNECT_BUTTON_LABEL = "Remove and Disconnect Mock Device";

const BUSY_BUTTON_LABEL = "Make Mock Device Busy";
const IDLE_BUTTON_LABEL = "Make Mock Device Idle";

const CFU_FAIL_LABEL    = "Enable Fail on CheckForUpdatedFirmware";
const CFU_SUCCESS_LABEL = "Disable Fail on CheckForUpdatedFirmware";

//const DOWNLOAD_FAIL_LABEL     = "Enable Fail on DownloadNewFirmware";
//const DOWNLOAD_SUCCESS_LABEL  = "Disable Fail on DownloadNewFirmware";

const WRITE_FAIL_LABEL      = "Enable Fail on WritingNewFirmware";
const WRITE_SUCCESS_LABEL   = "Disable Fail on WritingNewFirmware";

const INSTALL_FAIL_LABEL    = "Enable Fail on InstallNewFirmware";
const INSTALL_SUCCESS_LABEL = "Disable Fail on InstallNewFirmware";

const RECOVERY_ENABLE_LABEL   = "Enable RecoveryModeRequired on Device";
const RECOVERY_DISABLE_LABEL  = "Disable RecoveryModeRequired on Device";

const MOCK_DEVICE_ID = Components.ID("{3572E6FC-4954-4458-AFE7-0D0A65BF5F55}");

const PREF_MOCK_DEVICE_CFU_FAIL      = "testing.firmware.cfu.fail";
const PREF_MOCK_DEVICE_DOWNLOAD_FAIL = "testing.firmware.download.fail";
const PREF_MOCK_DEVICE_WRITE_FAIL    = "testing.firmware.write.fail";
const PREF_MOCK_DEVICE_INSTALL_FAIL  = "testing.firmware.update.fail";
const PREF_MOCK_DEVICE_BUSY          = "testing.busy";

const PREF_MOCK_DEVICE_NEED_RECOVERY = "testing.firmware.needRecoveryMode";

var DeviceDialogController =
{
  _device: null,
  _deviceId: null,

  // XUL Elements
  _buttonMockDeviceConnect:   null,
  _buttonMockDeviceCfu: null,
  _buttonMockDeviceDownload: null,
  _buttonMockDeviceWrite: null,
  _buttonMockDeviceInstall: null,
  _buttonMockDeviceBusy: null,

  onDialogLoad: function()
  {
    this._buttonMockDeviceConnect = 
      document.getElementById("mock-device-connect-button");

    this._buttonMockDeviceCfu = 
      document.getElementById("mock-device-cfu-button");
    
    /*  
    this._buttonMockDeviceDownload = 
      document.getElementById("mock-device-download-button");
    */

    this._buttonMockDeviceWrite = 
      document.getElementById("mock-device-write-button");
      
    this._buttonMockDeviceInstall = 
      document.getElementById("mock-device-install-button");

    this._buttonMockDeviceRecovery = 
      document.getElementById("mock-device-recovery-button");

    this._buttonMockDeviceBusy = 
      document.getElementById("mock-device-busy-button");

    this._update();
  },
  
  onMockDeviceCheckForUpdateButton: function() 
  {
    if(!this._device) 
      return;
      
    var cfuFail = 
      this._device.getPreference(PREF_MOCK_DEVICE_CFU_FAIL);
    
    if(cfuFail) {
      this._device.setPreference(PREF_MOCK_DEVICE_CFU_FAIL, false);
      this._buttonMockDeviceCfu.label = CFU_FAIL_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_CFU_FAIL, true);
      this._buttonMockDeviceCfu.label = CFU_SUCCESS_LABEL;
    }
  },
  
  onMockDeviceDownloadButton: function()
  {
    if(!this._device) 
      return;
      
    var downloadFail = 
      this._device.getPreference(PREF_MOCK_DEVICE_DOWNLOAD_FAIL);
    
    if(downloadFail) {
      this._device.setPreference(PREF_MOCK_DEVICE_DOWNLOAD_FAIL, false);
      this._buttonMockDeviceDownload.label = DOWNLOAD_FAIL_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_DOWNLOAD_FAIL, true);
      this._buttonMockDeviceDownload.label = DOWNLOAD_SUCCESS_LABEL;
    }
  },
  
  onMockDeviceWriteButton: function()
  {
    if(!this._device)
      return;
      
    var writeFail = 
      this._device.getPreference(PREF_MOCK_DEVICE_WRITE_FAIL);
    
    if(writeFail) {
      this._device.setPreference(PREF_MOCK_DEVICE_WRITE_FAIL, false);
      this._buttonMockDeviceWrite.label = WRITE_FAIL_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_WRITE_FAIL, true);
      this._buttonMockDeviceWrite.label = WRITE_SUCCESS_LABEL;
    }
  },

  onMockDeviceInstallButton: function()
  {
    if(!this._device)
      return;
      
    var installFail = 
      this._device.getPreference(PREF_MOCK_DEVICE_INSTALL_FAIL);
    
    if(installFail) {
      this._device.setPreference(PREF_MOCK_DEVICE_INSTALL_FAIL, false);
      this._buttonMockDeviceInstall.label = INSTALL_FAIL_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_INSTALL_FAIL, true);
      this._buttonMockDeviceInstall.label = INSTALL_SUCCESS_LABEL;
    }
  },

  onMockDeviceRecoveryButton: function()
  {
    if(!this._device)
      return;
      
    var recoveryMode = 
      this._device.getPreference(PREF_MOCK_DEVICE_NEED_RECOVERY);
    
    if(recoveryMode) {
      this._device.setPreference(PREF_MOCK_DEVICE_NEED_RECOVERY, false);
      this._buttonMockDeviceRecovery.label = RECOVERY_ENABLE_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_NEED_RECOVERY, true);
      this._buttonMockDeviceRecovery.label = RECOVERY_DISABLE_LABEL;
    }
  },
  
  onMockDeviceBusyButton: function()
  {
    if(!this._device)
      return;
      
    var deviceBusy = 
      this._device.getPreference(PREF_MOCK_DEVICE_BUSY);
    
    if(deviceBusy) {
      this._device.setPreference(PREF_MOCK_DEVICE_BUSY, false);
      this._buttonMockDeviceBusy.label = BUSY_BUTTON_LABEL;
    }
    else {
      this._device.setPreference(PREF_MOCK_DEVICE_BUSY, true);
      this._buttonMockDeviceBusy.label = IDLE_BUTTON_LABEL;
    }
  },
  
  onMockDeviceConnectButton: function() 
  {
    if(this._device == null) {
      this._createMockDevice();
    }
    else { 
      this._destroyMockDevice();
    }
  },

  _update: function()
  {
    var device = null;
    var devMan = this._getDevMan();
    
    try {
      device = devMan.getDevice(MOCK_DEVICE_ID);
    } catch(e) {}
    
    if(device) {
      this._device = device;
      this._deviceId = device.id;

      this._buttonMockDeviceConnect.label = DISCONNECT_BUTTON_LABEL;
      
      var cfuFail = device.getPreference(PREF_MOCK_DEVICE_CFU_FAIL);
      if(cfuFail) {
        this._buttonMockDeviceCfu.label = CFU_SUCCESS_LABEL;
      }
      
      /*
      var downloadFail = device.getPreference(PREF_MOCK_DEVICE_DOWNLOAD_FAIL);
      if(downloadFail) {
        this._buttonMockDeviceDownload.label = DOWNLOAD_SUCCESS_LABEL;
      }
      else {
        this._buttonMockDeviceDownload.label = DOWNLOAD_FAIL_LABEL;
      }
      */
      
      var writeFail = device.getPreference(PREF_MOCK_DEVICE_WRITE_FAIL);
      if(writeFail) {
        this._buttonMockDeviceWrite.label = WRITE_SUCCESS_LABEL;
      }
      else {
        this._buttonMockDeviceWrite.label = WRITE_FAIL_LABEL;
      }
      
      var installFail = device.getPreference(PREF_MOCK_DEVICE_INSTALL_FAIL);
      if(installFail) {
        this._buttonMockDeviceInstall.label = INSTALL_SUCCESS_LABEL;
      }
      else {
        this._buttonMockDeviceInstall.label = INSTALL_FAIL_LABEL;
      }
      
      var recoveryMode = device.getPreference(PREF_MOCK_DEVICE_NEED_RECOVERY);
      if(recoveryMode) {
        this._buttonMockDeviceRecovery.label = RECOVERY_DISABLE_LABEL;
      }
      else {
        this._buttonMockDeviceRecovery.label = RECOVERY_ENABLE_LABEL;
      }
      
      var deviceBusy = device.getPreference(PREF_MOCK_DEVICE_BUSY);
      if(deviceBusy) {
        this._buttonMockDeviceBusy.label = IDLE_BUTTON_LABEL;
      }
    }
    else {
      this._buttonMockDeviceCfu.disabled = true;
      //this._buttonMockDeviceDownload.disabled = true;
      this._buttonMockDeviceWrite.disabled = true;
      this._buttonMockDeviceInstall.disabled = true;
      this._buttonMockDeviceRecovery.disabled = true;
      this._buttonMockDeviceBusy.disabled = true;
    }
  },
  
  _createMockDevice: function() 
  {
    var device = 
      Cc["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
        .createInstance(Ci.sbIDevice); 
        
    var devMan = 
      Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
        .getService(Ci.sbIDeviceManager2); 
    devMan.registerDevice(device); 
    
    var data = 
      Cc["@songbirdnest.com/Songbird/Variant;1"]
        .createInstance(Ci.nsIWritableVariant); 
    data.setAsISupports(device.QueryInterface(Components.interfaces.nsISupports)); 
    
    var ev = 
      devMan.createEvent(Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED, device); 
    devMan.dispatchEvent(ev); 
    
    device.setPreference("testing.busy", false); 
    
    this._device = device;
    this._deviceId = device.id;
    
    this._buttonMockDeviceConnect.label = DISCONNECT_BUTTON_LABEL;
    
    this._buttonMockDeviceCfu.disabled = false;
    //this._buttonMockDeviceDownload.disabled = false;
    this._buttonMockDeviceWrite.disabled = false;
    this._buttonMockDeviceInstall.disabled = false;
    this._buttonMockDeviceRecovery.disabled = false;
    this._buttonMockDeviceBusy.disabled = false;
  },
  
  _destroyMockDevice: function()
  {
    var devMan = this._getDevMan(); 

    var data = 
      Cc["@songbirdnest.com/Songbird/Variant;1"]
        .createInstance(Ci.nsIWritableVariant); 
    data.setAsISupports(this._device.QueryInterface(Ci.nsISupports));

    var ev = 
      devMan.createEvent(Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED, this._device); 
    devMan.dispatchEvent(ev);
    
    devMan.unregisterDevice(this._device);
    
    this._device = null;        
    this._deviceId = null;
    
    this._buttonMockDeviceConnect.label = CONNECT_BUTTON_LABEL;
    
    this._buttonMockDeviceCfu.disabled = true;
    //this._buttonMockDeviceDownload.disabled = true;
    this._buttonMockDeviceWrite.disabled = true;
    this._buttonMockDeviceInstall.disabled = true;
    this._buttonMockDeviceRecovery.disabled = true;
    this._buttonMockDeviceBusy.disabled = true;
  },

  _getDevMan: function()
  {
    var devMan = 
      Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
        .getService(Ci.sbIDeviceManager2);
    return devMan;
  }
};

