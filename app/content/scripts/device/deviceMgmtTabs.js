/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

/**
* \file  DeviceMgmtTabs.js
* \brief Javascript source for the device sync tabs widget.
*/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device sync tabs widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device sync tabs defs.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/SBUtils.jsm");

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceMgmtTabs = {
  //
  // Device Sync Tabs object fields.
  //
  //   _widget                  Device sync tabs widget.
  //   _deviceID                Device ID.
  //   _device                  sbIDevice object.
  //

  _widget: null,
  _deviceID: null,
  _device: null,

  /**
   * \brief Initialize the device sync tabs services for the device sync tabs
   *        widget specified by aWidget.
   *
   * \param aWidget             Device sync tabs widget.
   */

  initialize: function DeviceMgmtTabs_initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;

    var toolsBox = this._getElement("device_tools");
    var settingsBox = this._getElement("device_summary_settings");
    if (toolsBox) 
      toolsBox.addEventListener("DOMAttrModified", this.tabListener, false);
    if (settingsBox)
      settingsBox.addEventListener("DOMAttrModified", this.tabListener, false);

    this._update();

    // Add device listener
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);
    }
  },

  tabListener: function DeviceMgmtTabs_tabDisableListener(event) {
    var target = event.target;
    var id = target.getAttribute("sbid");
    if (event.attrName != "disabledTab")
      return;
    if (event.newValue == "true") {
      if (id == "device_tools") {
        DeviceMgmtTabs._widget.setAttribute("hideToolsTab", "true");
      } else if (id == "device_summary_settings") {
        DeviceMgmtTabs._widget.setAttribute("hideSettingsTab", "true");
      }
    } else {
      if (id == "device_tools") {
        DeviceMgmtTabs._widget.removeAttribute("hideToolsTab");
      } else if (id == "device_summary_settings") {
        DeviceMgmtTabs._widget.removeAttribute("hideSettingsTab");
      }
    }
  },

  _update: function DeviceMgmtTabs_update() {
    // Check what content this device supports then hide tabs for unsupported
    // content types.
    var sbIDC = Ci.sbIDeviceCapabilities;
    // Check Audio
    if (!this._deviceSupportsContent(sbIDC.CONTENT_AUDIO,
                                     sbIDC.FUNCTION_AUDIO_PLAYBACK)) {
      this._toggleTab("music", true);
    }

    // Check Video
    if (!this._deviceSupportsContent(sbIDC.CONTENT_VIDEO,
                                     sbIDC.FUNCTION_VIDEO_PLAYBACK)) {
      this._toggleTab("video", true);
    }

    // Check Images
    if (!this._deviceSupportsContent(sbIDC.CONTENT_IMAGE,
                                     sbIDC.FUNCTION_IMAGE_DISPLAY)) {
      this._toggleTab("image", true);
    }
  },

  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DeviceMgmtTabs_finalize() {
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    var toolsBox = this._getElement("device_tools");
    var settingsBox = this._getElement("device_summary_settings");
    if (toolsBox)
      toolsBox.removeEventListener("DOMAttrModified", this.tabListener, false);
    if (settingsBox)
      settingsBox.removeEventListener("DOMAttrModified",
                                      this.tabListener,
                                      false);

    this._device = null;
    this._deviceID = null;
    this._widget = null;
  },

  onDeviceEvent: function DeviceMgmtTabs_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_INSERTED:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_REMOVED:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END:
        this._update();
        break;
    }
  },

  /**
   * \brief Check if the device has the requested function capabilities.
   *
   * \param aFunctionType            Function type we wish to test for.
   * \sa sbIDeviceCapabilities.idl
   */

  _checkFunctionSupport:
    function DeviceMgmtTabs__checkFunctionSupport(aFunctionType) {
    try {
      var functionTypes = this._device
                              .capabilities
                              .getSupportedFunctionTypes({});
      for (var i in functionTypes) {
        if (functionTypes[i] == aFunctionType) {
          return true;
        }
      }
    }
    catch (err) {
      var strFunctionType;
      switch (aFunctionType) {
        case Ci.sbIDeviceCapabilities.FUNCTION_DEVICE:
          strFunctionType = "Device";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_AUDIO_PLAYBACK:
          strFunctionType = "Audio Playback";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_AUDIO_CAPTURE:
          strFunctionType = "Audio Capture";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_IMAGE_DISPLAY:
          strFunctionType = "Image Display";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_IMAGE_CAPTURE:
          strFunctionType = "Image Capture";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_VIDEO_PLAYBACK:
          strFunctionType = "Video Playback";
          break;
        case Ci.sbIDeviceCapabilities.FUNCTION_VIDEO_CAPTURE:
          strFunctionType = "Video Capture";
          break;
        default:
          strFunctionType = "Unknown";
          break;
      }
      Cu.reportError("Unable to determine if device supports function type: "+
                     strFunctionType + " [" + err + "]");
    }

    return false;
  },

  /**
   * \brief Check if the device has the requested content capabilities in the
   * requested function area.
   *
   * \param aContentType             Content type we wish to test for.
   * \param aFunctionType            Functional area to test in.
   * \sa sbIDeviceCapabilities.idl
   */

  _deviceSupportsContent:
    function DeviceMgmtTabs__deviceSupportsContent(aContentType,
                                                   aFunctionType) {
    // First check if the device supports the function
    if (!this._checkFunctionSupport(aFunctionType)) {
      return false;
    }
    
    // lone> the next statement is not good, instead all relevant device
    // interfaces should report image support as needed: some devices currently
    // do not report image support even though they do support images, and some
    // devices will never report support because the api (ie, MSC) doesn't allow
    // us to ask; any MSC device has potential support for anything whatsoever,
    // since it appears as a USB drive. maybe it only means than MSC should
    // always support everything, but why not MTP then ? In fact, it's an open
    // question as to whether we should use this function to determine tab
    // visibility at all because even if a device does not support images (or
    // videos), you could still want to use it as syncable portable storage for
    // the files. The same argument can be made of tabs which are always visible
    // (ie, music): think of a USB key that plugs into a car stereo, it'll never
    // be reported as supporting music, yet it's just the portable storage used
    // by the user to play on a device that does support playback, so it's a
    // good thing we show the tab even though the physical device reports no
    // audio support.
    var sbIDC = Ci.sbIDeviceCapabilities;
    if (aContentType == sbIDC.CONTENT_IMAGE)
      return true;

    // Now check if the device supports the content type in that function.
    if (aFunctionType >= 0) {
      try {
        var contentTypes = this._device
                               .capabilities
                               .getSupportedContentTypes(aFunctionType, {});
        for (var i in contentTypes) {
          if (contentTypes[i] == aContentType) {
            return true;
          }
        }
      }
      catch (err) {
        var strContentType;
        switch (aContentType) {
          case sbIDC.CONTENT_AUDIO:
            strContentType = "Audio";
            break;
          case sbIDC.CONTENT_VIDEO:
            strContentType = "Video";
            break;
          case sbIDC.CONTENT_IMAGE:
            strContentType = "Image";
            break;
          default:
            strContentType = "Unknown";
            break;
        }
        Cu.reportError("Unable to determine if device supports content: "+
                       strContentType + " [" + err + "]");
      }
    }

    return false;
  },

  /**
   * \brief Hide or show a tab that should not be visible due to the device not
   * supporting them.
   *
   * \param aTabType              reference name of tab to hide.
   *    these are currenly "music","video","tools","settings"
   * \param shouldHide            should we hide this tab.
   */

  _toggleTab: function DeviceMgmtTabs__toggleTab(aTabType, shouldHide) {
    var tab = this._getElement("device_management_" + aTabType + "_sync_tab");

    if (shouldHide) {
      tab.setAttribute("hidden", true);
    }
    else {
      tab.removeAttribute("hidden");
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device sync tabs XUL services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function DeviceMgmtTabs__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
};
