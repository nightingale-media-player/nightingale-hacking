/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

function newTOC(aFirstTrack, aLastTrack, aLead)
{
  var toc = Cc["@songbirdnest.com/Songbird/MockCDTOC;1"]
             .createInstance(Ci.sbIMockCDTOC);
  toc.initialize(aFirstTrack, aLastTrack, aLead);
 
  return toc;
}

function sbMakeMidnightRock()
{
  var toc = newTOC(1, 15, 285675);

  // tracks:
  toc.addTocEntry(32, 309, 0, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(23260, 231, 1, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(40612, 242, 2, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(58770, 191, 3, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(73145, 310, 4, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(96415, 290, 5, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(118232, 301, 6, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(140867, 259, 7, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(160322, 316, 8, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(184085, 222, 9, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(200777, 236, 10, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(218535, 185, 11, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(232437, 211, 12, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(248320, 184, 13, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(262145, 313, 14, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  
  return toc;
}

//------------------------------------------------------------------------------
//
// Mock CD TOC "...Baby One More Time" by Britney Spears
//
//------------------------------------------------------------------------------

function sbMakeBabyOneMoreTime()
{
  var toc = newTOC(1, 12, 260335);

  toc.addTocEntry(0, 211, 0, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(15847, 200, 1, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(30859, 246, 2, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(49320, 202, 3, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(64479, 245, 4, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(82865, 312, 5, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(106307, 234, 6, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(123929, 243, 7, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(142217, 216, 8, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(158447, 223, 9, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(175179, 223, 10, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(203309, 760, 11, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);

  return toc;
}

//------------------------------------------------------------------------------
//
// Mock CD TOC "All That You Can't Leave Behind" by U2
//
//------------------------------------------------------------------------------

function sbMakeAllThatYouCantLeaveBehind()
{
  var toc = newTOC(1, 11, 225562);

  toc.addTocEntry(150, 248, 0, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(18843, 272, 1, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(39601, 227, 2, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(56966, 296, 3, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(79487, 267, 4, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(99796, 219, 5, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(116534, 226, 6, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(133832, 288, 7, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(155768, 258, 8, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(175400, 330, 9, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(200468, 331, 10, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);

  return toc;
}

//------------------------------------------------------------------------------
//
// Mock CD TOC "Incredibad" by The Lonely Island
//
//------------------------------------------------------------------------------

function sbMakeIncredibad()
{
  var toc = newTOC(1, 19, 190565);

  toc.addTocEntry(150, 76, 0, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(5896, 155, 1, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(17528, 151, 2, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(28879, 156, 3, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(40599, 126, 4, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(50106, 139, 5, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(60584, 64, 6, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(65394, 193, 7, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(79870, 34, 8, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(82446, 106, 9, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(90457, 123, 10, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(99748, 193, 11, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(114258, 126, 12, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(123750, 161, 13, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(135829, 65, 14, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(140754, 167, 15, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(153283, 175, 16, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(166425, 149, 17, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  toc.addTocEntry(177440, 179, 18, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);

  return toc;
}

//------------------------------------------------------------------------------
//
// Mock CD service implementation.
//
//------------------------------------------------------------------------------

function sbMockCDService()
{
  this._mListeners = [];
}

sbMockCDService.prototype =
{
  _mDevices:    null,
  _mListeners:  null,

  //
  // sbICDDeviceService:
  //
  get weight()
  {
    // return a weight of 0 effectively making the mock CD service selected
    // only if no other service wants to assume responsibility.
    return 0;
  },

  get nbDevices()
  {
    this._initDevices();
    return this._mDevices.length;
  },

  getDevice: function sbMockCDDevice_getDevice(aDeviceIndex)
  {
    this._initDevices();
    return this._mDevices[aDeviceIndex];
  },
  
  getDeviceFromIdentifier:
            function sbMockCDService_getDeviceFromIdentifier(aDeviceIdentifier)
  {
    var foundDevice = null;
    
    this._initDevices();
    for (var i = 0; i < this._mDevices.length; i++) {
      var curDeviceIdentifier = this._mDevices[i].identifier;
      if (curDeviceIdentifier == aDeviceIdentifier) {
        foundDevice = this._mDevices[i];
        break;
      }
    }
    
    return foundDevice;
  },

  getCDDevices: function sbMockCDService_getCDDevics()
  {
    this._initDevices();
    return ArrayConverter.nsIArray(this._mDevices).enumerate();
  },

  registerListener: function sbMockCDDevice_registerListener(aListener)
  {
    aListener.QueryInterface(Ci.sbICDDeviceListener);
    this._mListeners.push(aListener);
  },

  removeListener: function sbMockCDDevice_removeListener(aListener)
  {
    var index = this._mListeners.indexOf(aListener);
    if (index > -1) {
      this._mListeners.splice(index, 1);
    }
  },

  //
  // sbIMockCDDeviceController
  //
  insertMedia: function sbMockCDService_insertMedia(aCDDevice, aMediaDisc)
  {
    var curCDDevice = this._findDevice(aCDDevice);
    if (!curCDDevice || curCDDevice.isDiskInserted) {
      return;
    }

    // Determine which TOC to insert into the device.
    var mockMediaTOC;
    switch (aMediaDisc) {
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK:
        mockMediaTOC = sbMakeMidnightRock();
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_BABY_ONE_MORE_TIME:
        mockMediaTOC = sbMakeBabyOneMoreTime();
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_U2:
        mockMediaTOC = sbMakeAllThatYouCantLeaveBehind();
        break;
      case Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_INCREDIBAD:
        mockMediaTOC = sbMakeIncredibad();
        break;
    }

    curCDDevice.setDiscTOC(mockMediaTOC);
    this._onMediaInserted(curCDDevice);
  },

  ejectMedia: function sbMockCDService_ejectMedia(aCDDevice)
  {
    var curCDDevice = this._findDevice(aCDDevice);

    if (!curCDDevice || !curCDDevice.isDiscInserted) {
      return;
    }

    curCDDevice.eject();
    this._onMediaEjected(curCDDevice);
  },

  notifyEject: function sbMockCDService_notifyEject(aCDDevice)
  {
    var curCDDevice = this._findDevice(aCDDevice);
    if (!curCDDevice) {
      return;
    }

    this._onMediaEjected(curCDDevice);
  },

  //
  // nsIObserver
  //

  observe: function sbMockCDService_observe(aSubject, aTopic, aData)
  {
    // clear the devices
    this._mDevices = null;

    // unobserve
    var obs = Cc["@mozilla.org/observer-service;1"]
                .getService(Ci.nsIObserverService);
    obs.removeObserver(this, aTopic);
  },

  //
  // internal methods
  //

  /**
   * Make sure the devices are initialized
   */
  _initDevices: function sbMockCDService_init()
  {
    if (this._mDevices) {
      // already initialized
      return;
    }

    this._mDevices = [];

    // Push in two mock cd drive devices.
    var device = Cc["@songbirdnest.com/Songbird/MockCDDevice;1"]
                     .createInstance(Ci.sbIMockCDDevice);
    device.initialize("Songbird MockCD Device 8000",
                      true,
                      false,
                      false,
                      Ci.sbIDeviceController.AUDIO_DISC_TYPE,
                      false,
                      this);
    this._mDevices.push(device.QueryInterface(Ci.sbICDDevice));

    device = Cc["@songbirdnest.com/Songbird/MockCDDevice;1"]
                     .createInstance(Ci.sbIMockCDDevice);
    device.initialize("Songbird MockCD Device 7000",
                      true,
                      false,
                      false,
                      Ci.sbIDeviceController.AUDIO_DISC_TYPE,
                      false,
                      this);
    this._mDevices.push(device.QueryInterface(Ci.sbICDDevice));

    var obs = Cc["@mozilla.org/observer-service;1"]
                .getService(Ci.nsIObserverService);
    obs.addObserver(this, "quit-application", false);
  },

  _findDevice: function sbMockCDService_findDevice(aCDDevice)
  {
    // Return the local copy of our Mock CD Device that we have stashed in
    // |_mDevices|.
    this._initDevices();
    var foundDevice = null;
    for (var i = 0; i < this._mDevices.length && foundDevice == null; i++) {
      if (this._mDevices[i].name == aCDDevice.name) {
        foundDevice = this._mDevices[i];
      }
    }

    return foundDevice;
  },

  _onMediaEjected: function sbMockCDService_onMediaEjected(aDevice)
  {
    this._mListeners.forEach(
      function(aListener) {
        try {
          aListener.onMediaEjected(aDevice);
        }
        catch (e) {
          Cu.reportError(e);
        }
      });
  },

  _onMediaInserted: function sbMockCDService_onMediaInserted(aDevice)
  {
    this._mListeners.forEach(
      function(aListener) {
        try {
          aListener.onMediaInserted(aDevice);
        }
        catch (e) {
          Cu.reportError(e);
        }
      });
  },

  classDescription: "Songbird Mock CD Device Service",
  classID: Components.ID("{C701A410-7F71-41E9-A07A-C765A4F04D41}"),
  contractID: "@songbirdnest.com/device/cd/mock-cddevice-service;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDDeviceService,
                                         Ci.sbIMockCDDeviceController,
                                         Ci.nsIObserver])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMockCDService],
      // our post-register function to register ourselves with the
      // category manager
      function (aCompMgr, aFileSpec, aLocation) {
        XPCOMUtils.categoryManager.addCategoryEntry(
          "cdrip-engine",
          "Mock CD Device",
          sbMockCDService.prototype.contractID,
          true,
          true);
      }
  );
}

