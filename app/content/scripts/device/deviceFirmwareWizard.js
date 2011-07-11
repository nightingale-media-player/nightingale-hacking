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
* \file  deviceFirmwareWizard.js
* \brief Javascript source for the device firmware wizard dialog.
*/

Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

var deviceFirmwareWizard = {
  _device: null,
  _deviceProperties: null,
  _deviceFirmwareUpdater: null,
  _isDefaultDevice: false,
  _wizardElem: null,
  _wizardMode: "update",
  
  _currentMode: null,
  _currentOperation: null,
  _firmwareUpdate: null,
  
  _waitingForDeviceReconnect: false,
  _initialized: false,
  
  _repairDescriptionNode: null,
  _activeDataRemote: "firmware.wizard.active",

  get wizardElem() {
    if (!this._wizardElem)
      this._wizardElem = document.getElementById("device_firmware_wizard");
    return this._wizardElem;
  },

  doLoad: function deviceFirmwareWizard_doLoad() {
    this._initialize();
  },


  doUnload: function deviceFirmwareWizard_doUnload() {
    this._finalize();
  },

  doFinish: function deviceFirmwareWizard_doFinish() {
    // Looks like we were actually busy and we need to retry.
    if(this._currentOperation == "busy") {
      var self = this;
      setTimeout(function() { self.wizardElem.goTo("device_firmware_wizard_download_page"); }, 0);
      return false;
    }

    this._deviceFirmwareUpdater.finalizeUpdate(this._device);
    
    var self = window;
    setTimeout(function() { self.close(); }, 0);

    return true;
  },

  doCancel: function deviceFirmwareWizard_doCancel() {
    // If we are in the process of installing the firmware we have
    // to disable the cancel operation on the wizard.
    if(this._currentOperation == "install") {
      return false;
    }

    try {
      if(this._currentOperation != "complete") {
        this._deviceFirmwareUpdater.cancel(this._device);
      }
    } catch (e) {
      // Ignore any exceptions thrown so we continue to close the window
    }
    
    return true;
  },
  
  doClose: function deviceFirmwareWizard_doClose() {
    return this.doCancel();
  },

  doPageShow: function deviceFirmwareWizard_doPageShow() {
    if(!this._initialized)
      return;

    var currentPage = this.wizardElem.currentPage;
    var self = this;
    
    switch(currentPage.id) {
      case "device_firmware_check": {
        let self = this;
        
        this._currentOperation = "checkforupdate";
        if(this._isDefaultDevice) {
          this._deviceFirmwareUpdater.checkForUpdate(this._device, 
                                                     this._defaultDeviceVID,
                                                     this._defaultDevicePID,
                                                     this);
        }
        else {
          this._deviceFirmwareUpdater.checkForUpdate(this._device, 0, 0, this);
        }
      }
      break;
      
      case "device_firwmare_check_error": {
        this._currentOperation = "checkforupdate_error";
        
        let okButton = this.wizardElem.getButton("next");
        okButton.label = SBString("window.ok.label");
        okButton.accessKey = SBString("window.ok.accessKey");
        
        let supportLink = 
          document.getElementById("device_firmware_check_error_link");
        let handler = this._deviceFirmwareUpdater.getActiveHandler(this._device);
        
        let supportLinkValue = handler.customerSupportLocation.spec;
        supportLink.value = supportLinkValue;
        supportLink.href = supportLinkValue;
      }
      break;
      
      case "device_needs_recovery_mode": {
        this._currentOperation = "needsrecoverymode";
        
        let cancelButton = this.wizardElem.getButton("cancel");
        cancelButton.disabled = false;
        
        let deviceManager = 
          Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
            .getService(Ci.sbIDeviceManager2);
        
        deviceManager.addEventListener(this);

        // Handle default device
        let handler = null;        
        if(this._isDefaultDevice) {
          // Get a firmware handler for the selected device and hint it to
          // expect a recovery mode connection
          handler = this._deviceFirmwareUpdater.getHandler(
                                                  null,
                                                  this._defaultDeviceVID,
                                                  this._defaultDevicePID);
          handler.initiateRecoveryModeSwitch(this._defaultDeviceVID,
                                             this._defaultDevicePID);
          let recoveryInstructions = 
            SBFormattedString("device.firmware.wizard.recovery_mode.instructions", 
                              [this._defaultDeviceName, 
                               this._defaultDeviceKey, 
                               this._defaultDeviceKey]);

          // Hack up the style so that it doesn't look so damn ugly.
          recoveryInstructions = "<html><body><p style=\"font-family: sans-serif; font-size: 12px\">" + 
                                 recoveryInstructions + 
                                 "</p></body></html>";

          let browser = document.getElementById("device_firmware_wizard_recovery_mode_browser");
          let dataURI = "data:text/html;charset=UTF-8," + recoveryInstructions;
          browser.setAttribute("src", dataURI);
          
          let label = document.getElementById("device_firmware_wizard_recovery_mode_label");
          label.value = SBFormattedString("device.firmware.wizard.recovery_mode.connected",
                                        [this._defaultDeviceName]);
          return;
        }
        
        // Not default device, proceed as normal.
        try {
          handler = this._deviceFirmwareUpdater.getActiveHandler(this._device);
        }
        catch(e) {
          if(e.result == Cr.NS_ERROR_NOT_AVAILABLE &&
             this._wizardMode == "repair") {
            let listener = {
              device: self._device,
              onDeviceEvent: function(aEvent) {
                // Even if there's an error we should have the information
                // necessary to continue. All firmware handlers are required
                // to provide a basic set of reset instructions and a basic
                // firmware for recovery.
                if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_END ||
                   aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_ERROR) {
                  let handler = self._deviceFirmwareUpdater.getActiveHandler(this.device);

                  // Hint the handler to expect a recovery mode connection
                  handler.initiateRecoveryModeSwitch();

                  let label = document.getElementById("device_firmware_wizard_recovery_mode_label");
                  label.value = SBFormattedString("device.firmware.wizard.recovery_mode.connected",
                                                  [self._deviceProperties.modelNumber]);
                                                  
                  if(handler.recoveryModeKeyCombination) {
                    let recoveryInstructions = 
                      SBFormattedString("device.firmware.wizard.recovery_mode.instructions", 
                                        [self._deviceProperties.modelNumber, 
                                         handler.recoveryModeKeyCombination, 
                                         handler.recoveryModeKeyCombination]);

                    // Hack up the style so that it doesn't look so damn ugly.
                    recoveryInstructions = "<html><body><p style=\"font-family: sans-serif; font-size: 12px\">" + 
                                           recoveryInstructions + 
                                           "</p></body></html>";

                    let browser = document.getElementById("device_firmware_wizard_recovery_mode_browser");
                    let dataURI = "data:text/html;charset=UTF-8," + recoveryInstructions;
                    browser.setAttribute("src", dataURI);
                  }
                  else if(handler.resetInstructionsLocation) {
                    let browser = document.getElementById("device_firmware_wizard_recovery_mode_browser");
                    browser.setAttribute("src", handler.resetInstructionsLocation.spec);
                  }
                }
              }
            };
            
            this._deviceFirmwareUpdater.checkForUpdate(this._device, 0, 0, listener);
            this._deviceFirmwareUpdater.requireRecovery(this._device);
            
            return;
          }
        }

        this._deviceFirmwareUpdater.requireRecovery(this._device);
        
        // Hint the handler to expect a recovery mode connection
        handler.initiateRecoveryModeSwitch();

        let label = document.getElementById("device_firmware_wizard_recovery_mode_label");
        label.value = SBFormattedString("device.firmware.wizard.recovery_mode.connected",
                                        [this._deviceProperties.modelNumber]);
                                        
        if(handler.recoveryModeKeyCombination) {
          let recoveryInstructions = 
            SBFormattedString("device.firmware.wizard.recovery_mode.instructions", 
                              [this._deviceProperties.modelNumber, 
                               handler.recoveryModeKeyCombination, 
                               handler.recoveryModeKeyCombination]);

          // Hack up the style so that it doesn't look so damn ugly.
          recoveryInstructions = "<html><body><p style=\"font-family: sans-serif; font-size: 12px\">" + 
                                 recoveryInstructions + 
                                 "</p></body></html>";

          let browser = document.getElementById("device_firmware_wizard_recovery_mode_browser");
          let dataURI = "data:text/html;charset=UTF-8," + recoveryInstructions;
          browser.setAttribute("src", dataURI);
        }
        else if(handler.resetInstructionsLocation) {
          let browser = document.getElementById("device_firmware_wizard_recovery_mode_browser");
          browser.setAttribute("src", handler.resetInstructionsLocation.spec);
        }        
      }
      break;
      
      case "device_firmware_download": {
        if(this._device.isBusy) {
          this._currentOperation = "busy";
          setTimeout(function() { 
            self.wizardElem.goTo("device_firmware_busy_device_page");
            }, 0);
        }
        else {
          let descElem = 
            document.getElementById("device_firmware_download_no_disconnect_desc");
          let descTextNode = 
            document.createTextNode(SBString("device.firmware.wizard.no_disconnect_warn"));
          
          if(descElem.firstChild && 
             descElem.firstChild.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
            descElem.removeChild(descElem.firstChild);
          }
          
          descElem.appendChild(descTextNode);
          
          this._currentOperation = "download";
          this._deviceFirmwareUpdater.downloadUpdate(this._device, false, this);
        }
      }
      break;
      
      case "device_firmware_download_error": {
        this._currentOperation = "download_error";
        
        let okButton = this.wizardElem.getButton("next");
        okButton.label = SBString("window.ok.label");
        okButton.accessKey = SBString("window.ok.accessKey");
        
        let supportLink = 
          document.getElementById("device_firmware_download_error_link");
        let handler = this._deviceFirmwareUpdater.getActiveHandler(this._device);
        
        let supportLinkValue = handler.customerSupportLocation.spec;
        supportLink.value = supportLinkValue;
        supportLink.href = supportLinkValue;
      }
      break;
      
      case "device_firmware_repair": {
        this._currentOperation = "confirmrepair";
        
        let restoreButton = this.wizardElem.getButton("next");
        restoreButton.label = SBString("device.firmware.repair.button");
        restoreButton.accessKey = null;
        
        let descElem = document.getElementById("device_firmware_repair_description");
        if(this._repairDescriptionNode) {
          descElem.removeChild(this._repairDescriptionNode);
          this._repairDescriptionNode = null;
        }

        let deviceName = this._isDefaultDevice ? 
                           this._defaultDeviceName : this._deviceProperties.friendlyName;
        let descString = SBFormattedString("device.firmware.repair.description",
                                           [deviceName]);
        this._repairDescriptionNode = document.createTextNode(descString);
        descElem.appendChild(this._repairDescriptionNode);
      }
      break;
      
      case "device_firmware_install": {
        this._currentOperation = "install";
        
        let cancelButton = this.wizardElem.getButton("cancel");
        cancelButton.disabled = true;
        
        let descElem = document.getElementById("device_firmware_install_no_disconnect_desc");
        if(descElem.firstChild && 
           descElem.firstChild.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
          descElem.removeChild(descElem.firstChild);
        }
        
        if(this._wizardMode == "repair") {
          let label = document.getElementById("device_firmware_wizard_install_title");
          label.value = SBString("device.firmware.repair.inprocess");
          
          let textNode = 
            document.createTextNode(SBString("device.firmware.repair.no_disconnect_warning"));
          descElem.appendChild(textNode);
          
          try {
            if(this._isDefaultDevice) {
              this._deviceFirmwareUpdater.recoveryUpdate(this._device, 
                                                         this._firmwareUpdate, 
                                                         this._defaultDeviceVID, 
                                                         this._defaultDevicePID, 
                                                         this);
            }
            else {
              this._deviceFirmwareUpdater.recoveryUpdate(this._device, 
                                                         this._firmwareUpdate, 
                                                         0, 
                                                         0, 
                                                         this);
            }
          }
          catch(e) {
            this.wizardElem.goTo("device_firmware_install_error_page");
          }
        }
        else {
          let textNode = 
            document.createTextNode(SBString("device.firmware.wizard.no_disconnect_warn"));
          descElem.appendChild(textNode);
          
          // We have to do this to ensure that the device event is propagated
          // to everyone before we actually start the update. Yes this is lame.
          // Yes, it would be nice to do it a different way.
          setTimeout(function() {
              self._deviceFirmwareUpdater.applyUpdate(self._device, self._firmwareUpdate, self);
            }, 0);
        }
      }
      break;
      
      case "device_firmware_install_error": {
        this._currentOperation = "install_error";
        
        let okButton = this.wizardElem.getButton("next");
        okButton.label = SBString("window.ok.label");
        okButton.accessKey = SBString("window.ok.accessKey");
        
        let supportLink = 
          document.getElementById("device_firmware_install_error_link");
        let handler = this._deviceFirmwareUpdater.getActiveHandler(this._device);
        
        let supportLinkValue = null;
        
        try {
          supportLinkValue = handler.customerSupportLocation.spec;
        } catch(e) {}
        
        supportLink.value = supportLinkValue;
        supportLink.href = supportLinkValue;
        
        let descElem = 
          document.getElementById("device_firmware_install_error_description");
          
        if(descElem.firstChild && 
           descElem.firstChild.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
          descElem.removeChild(descElem.firstChild);
        }
        
        let text = null;        
        if(this._wizardMode == "repair") {
          text = SBString("device.firmware.repair.error.description");
        }
        else {
          text = SBString("device.firmware.wizard.install.error.desc");
        }
        descElem.appendChild(document.createTextNode(text));
      }
      break;
      
      case "device_firmware_update_complete": {
        this._currentOperation = "complete";
        
        let desc = 
          document.getElementById("device_firmware_update_complete_description");
        let descStr = "";
        
        if(this._wizardMode == "repair") {
          descStr = SBString("device.firmware.repair.complete.desc");
          
          // Automatically close the wizard when in repair mode.
          this.doFinish();
        }
        else {
          descStr = SBString("device.firmware.wizard.complete.desc");
        }
        
        let descTextNode = document.createTextNode(descStr);
        desc.appendChild(descTextNode);
      }
      break;
      
      case "device_firmware_busy_device": {
        let retryButton = this.wizardElem.getButton("next");
        retryButton.label = 
          SBString("device.firmware.wizard.retry.button");
        retryButton.accessKey = null;
      }
      break;
      
      default:
        throw new Error("not reached");
    }
  },
  
  doBack: function deviceFirmwareWizard_onBack(aEvent) {
    this._deviceFirmwareUpdater.cancel(this._device);

    var self = window;
    setTimeout(function() { self.close(); }, 0);
    
    return false;
  },
  
  doNext: function deviceFirmwareWizard_onNext(aEvent) {
    if(this._currentOperation) {
      let isError = this._currentOperation.substring(-5, 5) == "error";
      if(isError || this._currentOperation == "uptodate") {
        if(isError) {
          // Error, cancel the operation
          this._deviceFirmwareUpdater.cancel(this._device);
        }
        
        try {
          this._deviceFirmwareUpdater.finalizeUpdate(this._device);
        } catch(e) { Cu.reportError(e); }

        window.close();
        
        return false;
      }
      else if(this._currentOperation == "checkforupdate") {
        // Check if the handler needs to be in recovery mode, if so
        // go to the recovery mode page to show the instructions and
        // wait for the device to be reconnected in recovery mode.
        let handler = 
          this._deviceFirmwareUpdater.getActiveHandler(this._device);
          
        if(handler.needsRecoveryMode) {
          this._currentOperation = "needsrecoverymode";
          
          let self = this;  
          setTimeout(function() {
            self.wizardElem.goTo("device_needs_recovery_mode_page");
            }, 0);
          return false;
        }
      }
      else if(this._currentOperation == "confirmrepair") {
        let self = this;
        setTimeout(function() {
          self.wizardElem.goTo("device_firmware_wizard_download_page");
          }, 0);
        return false;
      }
    }
   
    return true;
  },
  
  doExtra1: function deviceFirmwareWizard_onExtra1(aEvent) {
    return true;
  },
  
  doExtra2: function deviceFirmwareWizard_onExtra2(aEvent) {
    return true;
  },
  
  onDeviceEvent: function deviceFirmwareWizard_onDeviceEvent(aEvent) {
    this._handleDeviceEvent(aEvent);
  },

  update: function deviceFirmwareWizard_update() {
  },

  _initialize: function deviceFirmwareWizard__initialize() {
    if (this._initialized)
      return;

    var dialogPB = 
      window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);

    try {
      var wizardMode = dialogPB.GetString(0).split("=");
      this._wizardMode = wizardMode[1];
    } catch(e) {
      this._wizardMode = "update";
    }
    
    try {
      var defaultDevice = dialogPB.GetString(1).split("=");
      this._isDefaultDevice = (defaultDevice[1] == "true");
    }
    catch(e) {
      this._isDefaultDevice = false;
    }
    
    if(!this._isDefaultDevice) {
      this._device = dialogPB.objects.queryElementAt(0, Ci.sbIDevice);
    }
    else {
      this._defaultDeviceName = dialogPB.GetString(2);
      this._defaultDeviceKey = dialogPB.GetString(3);
      
      var deviceVIDPID = dialogPB.GetString(4).split("-");
      this._defaultDeviceVID = deviceVIDPID[0];
      this._defaultDevicePID = deviceVIDPID[1];
    }
    
    if(!this._isDefaultDevice) {
      this._deviceProperties = this._device.properties;
    }
    
    this._deviceFirmwareUpdater = 
      Cc["@songbirdnest.com/Songbird/Device/Firmware/Updater;1"]
        .getService(Ci.sbIDeviceFirmwareUpdater);

    this._wizardElem = document.getElementById("device_firmware_wizard");
    this._domEventListenerSet = new DOMEventListenerSet();

    var browserBox = document.getElementById("device_firmware_wizard_release_notes_box");
    this._domEventListenerSet.add(browserBox, "collapse", this._handleBrowserCollapse, true, false);

    this._wizardElem.canRewind = true;

    this._initialized = true;
    SBDataSetBoolValue(this._activeDataRemote, true);
    
    // in repair mode and default device mode, skip check for update and 
    // download firmware.
    var self = this;
    
    // first check for default device mode as it needs to bypass the use
    // of the firmware handler.
    if(this._wizardMode == "repair" && this._isDefaultDevice) {
      // repair mode + default device means we go straight to displaying
      // the instructions to put the device in recovery mode.
      this._wizardElem.title = SBString("device.firmware.repair.title");
      return this._wizardElem.goTo("device_needs_recovery_mode_page");
    }
    
    // not default device, proceed as normal.
    if(this._wizardMode == "repair") {
      this._wizardElem.title = SBString("device.firmware.repair.title");
      let handler = this._deviceFirmwareUpdater.getHandler(this._device, 0, 0);
      handler.bind(this._device, null);
      let recoveryMode = handler.recoveryMode;
      
      handler.unbind();
      handler = null;
      
      if(recoveryMode) {
        setTimeout(function() {
            self._wizardElem.goTo("device_firmware_wizard_check_page");
          }, 0);
      }
      else {
        // device needs to be switched to recovery mode
        setTimeout(function() {
            self._wizardElem.goTo("device_needs_recovery_mode_page");
          }, 0);
    }
    }
    else {
    setTimeout(function() {
        self._wizardElem.goTo("device_firmware_wizard_check_page");
      }, 0);
    }
  },


  _finalize: function deviceFirmwareWizard__finalize() {
    this._deviceFirmwareUpdater.finalizeUpdate(this._device);
    
    let deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    deviceManager.removeEventListener(this);
  
    this._device = null;
    this._deviceFirmwareUpdater = null;
    
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;
    
    SBDataSetBoolValue(this._activeDataRemote, false);
  },
  
  _handleBrowserCollapse: function deviceFirmwareWizard__handleBrowserCollapse(aEvent) {
    const BROWSER_VISIBLE_INNER_HEIGHT = 360;
    
    if(aEvent.detail == true) {
      window.innerHeight = BROWSER_VISIBLE_INNER_HEIGHT;
    }
  },

  _handleDeviceEvent: function deviceFirmwareWizard__handleDeviceEvent(aEvent) {
    switch(this._currentOperation) {
      case "checkforupdate":
        this._handleCheckForUpdate(aEvent);
      break;
      
      case "download":
        this._handleDownloadFirmware(aEvent);
      break;
      
      case "install":
        this._handleApplyUpdate(aEvent);
      break;
      
      case "needsrecoverymode":
        this._handleNeedsRecoveryMode(aEvent);
      break;
    }
  },
  
  _handleCheckForUpdate: function deviceFirmwareWizard__handleCheckForUpdate(aEvent) {
    if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_END) {
      var progressDeck = document.getElementById("device_firmware_wizard_check_deck");
      if(aEvent.data == true) {
        
        // If we are in repair mode, we skip to the repair confirmation page
        if(this._wizardMode == "repair") {
          var self = this;
          setTimeout(function() {
            self._wizardElem.goTo("device_firmware_wizard_repair_page");
          }, 0);
          return;
        }
        
        var newVerDesc = 
          document.getElementById("device_firmware_wizard_check_newver_description");
        
        var handler = this._deviceFirmwareUpdater.getActiveHandler(this._device);
                
        var text = SBFormattedString("device.firmware.wizard.check.newver.description", 
                                     [handler.latestFirmwareReadableVersion]);
                                     
        newVerDesc.appendChild(document.createTextNode(text));
        
        var remindMeLaterButton = this.wizardElem.getButton("back");
        var installNewFirmwareButton = this.wizardElem.getButton("next");
        
        remindMeLaterButton.label = 
          SBString("device.firmware.wizard.check.remind_me_later.label");
        remindMeLaterButton.accessKey = null;
        remindMeLaterButton.disabled = false;
        
        installNewFirmwareButton.label = 
          SBString("device.firmware.wizard.check.install.label");
        installNewFirmwareButton.accessKey = null;
        
        this.wizardElem.currentPage.setAttribute("showback", "true");
        this.wizardElem.currentPage.setAttribute("shownext", "true");
                  
        var browser = document.getElementById("device_firmware_wizard_release_notes_browser");
        
        var releaseNotesLocation = null;
        try { 
          releaseNotesLocation = handler.releaseNotesLocation;
        }
        catch(e) {}
        
        var spec = releaseNotesLocation ? releaseNotesLocation.spec : "about:blank";
        browser.setAttribute("src", spec);
        
        progressDeck.selectedPanel = 
          document.getElementById("device_firmware_wizard_check_new_box");

        var browserBox =
          document.getElementById("device_firmware_wizard_release_notes_box");
        browserBox.open = true;
          
        // force canRewind since we need to be able to use the back button
        // as the remind me later button.
        this._wizardElem.canRewind = true;
      }
      else {
        let okButton = this.wizardElem.getButton("next");
        okButton.label = SBString("window.ok.label");
        okButton.accessKey = SBString("window.ok.accessKey");
        
        this.wizardElem.currentPage.setAttribute("hidecancel", "true");
        this.wizardElem.currentPage.setAttribute("shownext", "true");
        
        progressDeck.selectedPanel = 
          document.getElementById("device_firmware_wizard_check_already_box");
         
        this._currentOperation = "uptodate";
      }      
    }
    else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_ERROR) {
      this.wizardElem.goTo("device_firmware_wizard_check_error_page");
    }
  },
  
  _handleDownloadFirmware: function deviceFirmwareWizard__handleDownloadFirmware(aEvent) {
    var progressMeter = 
      document.getElementById("device_firmware_wizard_download_progress");
    switch(aEvent.type) {
      // Good events
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_PROGRESS:
        progressMeter.value = aEvent.data;
      break;
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_END:
        this._firmwareUpdate = aEvent.data.QueryInterface(Ci.sbIDeviceFirmwareUpdate);
        this.wizardElem.goTo("device_firmware_wizard_install_page");
      break;
      
      // Error Events
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_ERROR:
        this.wizardElem.goTo("device_firmware_wizard_download_error_page");
      break;
    }
  },
  
  _handleApplyUpdate: function deviceFirmwareWizard__handleApplyUpdate(aEvent) {
    var progressMeter = 
      document.getElementById("device_firmware_wizard_install_progress");
      
    switch(aEvent.type) {
      // Good events
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_START:
        progressMeter.mode = "undetermined";
      break;
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_PROGRESS:
        if(progressMeter.mode != "determined") {
          progressMeter.mode = "determined";
        }
        progressMeter.value = aEvent.data;
      break;
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_END:
        if(progressMeter.mode != "determined") {
          progressMeter.mode = "determined";
        }
        progressMeter.value = 100;
      break;
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_UPDATE_END:
        this.wizardElem.goTo("device_firmware_wizard_complete_page");  
      break;
      
      // Special error that indicates that the firmware image we're
      // attempting to put on the device actually requires the device to
      // be in recovery mode.
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_NEEDREC_ERROR:
        let handler = 
          this._deviceFirmwareUpdater.getActiveHandler(this._device);
          
        if(handler.needsRecoveryMode) {
          this._currentOperation = "needsrecoverymode";
          
          let self = this;  
          setTimeout(function() {
            self.wizardElem.goTo("device_needs_recovery_mode_page");
            }, 0);
        }
      break;
      
      // Error events
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_ERROR:
      case Ci.sbIDeviceEvent.EVENT_FIRMWARE_UPDATE_ERROR:
        this.wizardElem.goTo("device_firmware_install_error_page");
      break;
    }
  },
  
  _handleNeedsRecoveryMode: function deviceFirmwareWizard__handleNeedsRecoveryMode(aEvent) {
    switch(aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
        if(!this._waitingForDeviceReconnect && !this._isDefaultDevice) {
          return;
        }
        
        this._device = aEvent.data.QueryInterface(Ci.sbIDevice);
        
        let criticalFailure = false;
        let continueSuccess = false;
        
        // Not default device, proceed as normal.
        if(!this._isDefaultDevice) {
          try {
            continueSuccess = 
              this._deviceFirmwareUpdater.continueUpdate(this._device, this);
          }
          catch(e) {
            criticalFailure = true;
            Cu.reportError(e);
          }
        }
        else {
          // Simply continue if we're dealing with the default device.
          continueSuccess = true;
        }

        if(continueSuccess || criticalFailure) {
          this._waitingForDeviceReconnect = false;
        
          let deviceManager = 
            Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
              .getService(Ci.sbIDeviceManager2);
      
          deviceManager.removeEventListener(this);

          if(criticalFailure) {
            // We're not recovering from this one, go to the error page.
            this.wizardElem.goTo("device_firmware_install_error_page");
          }
          else {
            if(this._wizardMode == "repair") {
              // Repair mode needs to ask for confirmation first.
              this.wizardElem.goTo("device_firmware_wizard_check_page");
            }
            else {
              // Business as usual, download the new firmware and proceed
              // with the installation.
              this.wizardElem.goTo("device_firmware_wizard_download_page");
            }
          }
        }
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED: {
        this._waitingForDeviceReconnect = true;

        let label = document.getElementById("device_firmware_wizard_recovery_mode_label");
        if(this._isDefaultDevice) {
          label.value = SBFormattedString("device.firmware.wizard.recovery_mode.disconnected",
                                          [this._defaultDeviceName]);
        }
        else {
          label.value = SBFormattedString("device.firmware.wizard.recovery_mode.disconnected",
                                          [this._deviceProperties.modelNumber]);
        }
      }
      break;
    } 
  }
};
