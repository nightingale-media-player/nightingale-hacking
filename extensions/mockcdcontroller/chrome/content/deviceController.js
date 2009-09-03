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

const CONNECT_BUTTON_LABEL = "Add and Connect Mock Device";
const DISCONNECT_BUTTON_LABEL = "Remove and Disconnect Mock Device";

const MOCK_DEVICE_ID = Components.ID("{3572E6FC-4954-4458-AFE7-0D0A65BF5F55}");

var DialogController =
{
  _device: null,
  _deviceId: null,

  // XUL Elements
  _buttonMockDeviceConnect:   null,

  onDialogLoad: function()
  {
    this._buttonMockDeviceConnect = 
      document.getElementById("mock-device-connect-button");

    this._update();
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
  },

  _getDevMan: function()
  {
    var devMan = 
      Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
        .getService(Ci.sbIDeviceManager2);
    return devMan;
  }
};

