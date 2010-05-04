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

/**
 * This is a helper class for devices
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

function BaseDeviceHelper() {}

BaseDeviceHelper.prototype = {

/*
  boolean hasSpaceForWrite(in unsigned long long aSpaceNeeded,
                           in sbIDeviceLibrary aLibrary,
                           [optional] in sbIDevice aDevice,
                           [optional] out unsigned long long aSpaceRemaining);
 */
  hasSpaceForWrite: function BaseDeviceHelper_hasSpaceForWrite(
    aSpaceNeeded, aLibrary, aDevice, aSpaceRemaining)
  {
    var device = aDevice;
    if (!device) {
      // no device supplied, need to find it
      const deviceMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceRegistrar);
      var foundLib = false;
      for (device in ArrayConverter.JSArray(deviceMgr.devices)) {
        for (var library in ArrayConverter.JSArray(device.content.libraries)) {
          if (library.equals(aLibrary)) {
            foundLib = true;
            break;
          }
        }
        if (foundLib)
          break;
      }
      if (!foundLib) {
        throw Components.Exception("Failed to find device for library");
      }
    }

    // figure out the space remaining
    var spaceRemaining =
      device.properties
            .properties
            .getPropertyAsInt64("http://songbirdnest.com/device/1.0#freeSpace");
    if (aSpaceRemaining) {
      aSpaceRemaining.value = spaceRemaining;
    }

    // if we have enough space, no need to look at anything else
    if (aSpaceNeeded < spaceRemaining)
      return true;

    // need to ask the user
    var messageKeyPrefix = this._getMessageKeyPrefix(aLibrary);

    storageConverter =
      Cc["@songbirdnest.com/Songbird/Properties/UnitConverter/Storage;1"]
        .createInstance(Ci.sbIPropertyUnitConverter);
    var messageParams = [
      device.name,
      storageConverter.autoFormat(aSpaceNeeded, -1, 1),
      storageConverter.autoFormat(spaceRemaining, -1, 1)
    ];
    var message = SBFormattedString(messageKeyPrefix + ".message",
                                    messageParams);

    const prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                       .getService(Ci.sbIPrompter);
    var neverPromptAgain = { value: false };
    var abortRequest = prompter.confirmEx(null, /* parent */
                                          SBString(messageKeyPrefix + ".title"),
                                          message,
                                          Ci.nsIPromptService.STD_YES_NO_BUTTONS,
                                          null, null, null, /* button text */
                                          null, /* checkbox message TODO */
                                          neverPromptAgain);
    return (!abortRequest);
  },

/*
  boolean queryUserSpaceExceeded(in nsIDOMWindow       aParent,
                                 in sbIDevice          aDevice,
                                 in sbIDeviceLibrary   aLibrary,
                                 in unsigned long long aSpaceNeeded,
                                 in unsigned long long aSpaceAvailable);
 */
  queryUserSpaceExceeded: function BaseDeviceHelper_queryUserSpaceExceeded
                                     (aParent,
                                      aDevice,
                                      aLibrary,
                                      aSpaceNeeded,
                                      aSpaceAvailable,
                                      aAbort)
  {
    var messageKeyPrefix = this._getMessageKeyPrefix(aLibrary);

    storageConverter =
      Cc["@songbirdnest.com/Songbird/Properties/UnitConverter/Storage;1"]
        .createInstance(Ci.sbIPropertyUnitConverter);
    var message = SBFormattedString
                    (messageKeyPrefix + ".message",
                     [ aDevice.name,
                       storageConverter.autoFormat(aSpaceNeeded, -1, 1),
                       storageConverter.autoFormat(aSpaceAvailable, -1, 1) ]);

    var buttonFlags = (Ci.nsIPromptService.BUTTON_POS_0 *
                       Ci.nsIPromptService.BUTTON_TITLE_IS_STRING) +
                      (Ci.nsIPromptService.BUTTON_POS_1 *
                       Ci.nsIPromptService.BUTTON_TITLE_IS_STRING);
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .getService(Ci.sbIPrompter);
    var neverPromptAgain = { value: false };
    var abortRequest = prompter.confirmEx
                         (aParent,
                          SBString(messageKeyPrefix + ".title"),
                          message,
                          buttonFlags,
                          SBString(messageKeyPrefix + ".confirm_button"),
                          SBString(messageKeyPrefix + ".cancel_button"),
                          null,
                          null,
                          neverPromptAgain);

    return !abortRequest;
  },

  _getMessageKeyPrefix:
    function BaseDeviceHelper__getMessageKeyPrefix(aLibrary) {
    var messageKeyPrefix = "device.error.not_enough_freespace.prompt.";
    if ((aLibrary.mgmtType & Ci.sbIDeviceLibrary.MGMT_TYPE_ALL_MASK) > 0) {
        messageKeyPrefix += "sync";
    }
    else if ((aLibrary.mgmtType & Ci.sbIDeviceLibrary.MGMT_TYPE_PLAYLISTS_MASK) > 0){
        messageKeyPrefix += "sync_playlists";
    }
    else {
        messageKeyPrefix += "manual";
    }

    return messageKeyPrefix;
  },

  contractID: "@songbirdnest.com/Songbird/Device/Base/Helper;1",
  classDescription: "Helper component for device implementations",
  classID: Components.ID("{ebe6e08a-0604-44fd-a3d7-2be556b96b24}"),
  QueryInterface: XPCOMUtils.generateQI([
    Components.interfaces.sbIDeviceHelper
    ])
};

NSGetModule = XPCOMUtils.generateNSGetModule([BaseDeviceHelper], null, null);
