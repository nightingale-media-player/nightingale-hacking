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

//------------------------------------------------------------------------------
//
// Mock CD TOC Entry implementation
//
//------------------------------------------------------------------------------

function sbMockCDTOCEntry(aFrameOffset, aLength, aTrackNumber)
{
  this.frameOffset = aFrameOffset;
  this.length = aLength;
  this.trackNumber = aTrackNumber;
}

sbMockCDTOCEntry.prototype =
{
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOCEntry])
};

//------------------------------------------------------------------------------
//
// Mock CD TOC Base Item
//
//------------------------------------------------------------------------------

function sbMockCDTOCBase()
{
}

sbMockCDTOCBase.prototype =
{
  _tracks:             null,
  _firstTrackIndex:    0,
  _lastTrackIndex:     0,
  _leadOutTrackOffset: 0,

  get status()
  {
    return Ci.sbICDTOC.STATUS_OK;
  },

  get firstTrackIndex()
  {
    return this._firstTrackIndex;
  },

  get lastTrackIndex()
  {
    return this._lastTrackIndex;
  },

  get leadOutTrackOffset()
  {
    return this._leadOutTrackOffset;
  },

  get tracks()
  {
    return ArrayConverter.nsIArray(this._tracks);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOC])
};

//------------------------------------------------------------------------------
//
// Mock CD TOC "Midnight Rock" by Various Artists
//
//------------------------------------------------------------------------------

function sbMockCDTOC_MidnightRock()
{
  // Call super constructor
  sbMockCDTOCBase.call(this);

  this._firstTrackIndex    = 1;
  this._lastTrackIndex     = 15;
  this._leadOutTrackOffset = 285675;

  // tracks:
  this._tracks = [];
  this._tracks.push(new sbMockCDTOCEntry(32, 309, 0));
  this._tracks.push(new sbMockCDTOCEntry(23260, 231, 0));
  this._tracks.push(new sbMockCDTOCEntry(40612, 242, 0));
  this._tracks.push(new sbMockCDTOCEntry(58770, 191, 0));
  this._tracks.push(new sbMockCDTOCEntry(73145, 310, 0));
  this._tracks.push(new sbMockCDTOCEntry(96415, 290, 0));
  this._tracks.push(new sbMockCDTOCEntry(118232, 301, 0));
  this._tracks.push(new sbMockCDTOCEntry(140867, 259, 0));
  this._tracks.push(new sbMockCDTOCEntry(160322, 316, 0));
  this._tracks.push(new sbMockCDTOCEntry(184085, 222, 0));
  this._tracks.push(new sbMockCDTOCEntry(200777, 236, 0));
  this._tracks.push(new sbMockCDTOCEntry(218535, 185, 0));
  this._tracks.push(new sbMockCDTOCEntry(232437, 211, 0));
  this._tracks.push(new sbMockCDTOCEntry(248320, 184, 0));
  this._tracks.push(new sbMockCDTOCEntry(262145, 313, 0));
}

sbMockCDTOC_MidnightRock.prototype =
{
  __proto__: sbMockCDTOCBase.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOC])
};

var gMidnightRockTOC = new sbMockCDTOC_MidnightRock();

//------------------------------------------------------------------------------
//
// Mock CD TOC "Burnshit Here" by Brian Glaze
//
//------------------------------------------------------------------------------

function sbMockCDTOC_BabyOneMoreTime()
{
  // Call super constructor
  sbMockCDTOCBase.call(this);

  this._firstTrackIndex    = 1;
  this._lastTrackIndex     = 12;
  this._leadOutTrackOffset = 260335;

  // tracks:
  this._tracks = [];

  this._tracks.push(new sbMockCDTOCEntry(0, 211, 1));
  this._tracks.push(new sbMockCDTOCEntry(15847, 200, 2));
  this._tracks.push(new sbMockCDTOCEntry(30859, 246, 3));
  this._tracks.push(new sbMockCDTOCEntry(49320, 202, 4));
  this._tracks.push(new sbMockCDTOCEntry(64479, 245, 5));
  this._tracks.push(new sbMockCDTOCEntry(82865, 312, 6));
  this._tracks.push(new sbMockCDTOCEntry(106307, 234, 7));
  this._tracks.push(new sbMockCDTOCEntry(123929, 243, 8));
  this._tracks.push(new sbMockCDTOCEntry(142217, 216, 9));
  this._tracks.push(new sbMockCDTOCEntry(158447, 223, 10));
  this._tracks.push(new sbMockCDTOCEntry(175179, 223, 11));
  this._tracks.push(new sbMockCDTOCEntry(203309, 760, 12));
}

sbMockCDTOC_BabyOneMoreTime.prototype =
{
  __proto__: sbMockCDTOCBase.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOC])
};

var gBabyOneMoreTimeTOC = new sbMockCDTOC_BabyOneMoreTime();

//------------------------------------------------------------------------------
//
// Mock CD TOC "All That You Can't Leave Behind" by U2
//
//------------------------------------------------------------------------------

function sbMockCDTOC_AllThatYouCantLeaveBehind()
{
  // Call super constructor
  sbMockCDTOCBase.call(this);

  this._firstTrackIndex    = 1;
  this._lastTrackIndex     = 11;
  this._leadOutTrackOffset = 225562;

  // tracks:
  this._tracks = [];
  this._tracks.push(new sbMockCDTOCEntry(150, 248, 1));
  this._tracks.push(new sbMockCDTOCEntry(18843, 272, 2));
  this._tracks.push(new sbMockCDTOCEntry(39601, 227, 3));
  this._tracks.push(new sbMockCDTOCEntry(56966, 296, 4));
  this._tracks.push(new sbMockCDTOCEntry(79487, 267, 5));
  this._tracks.push(new sbMockCDTOCEntry(99796, 219, 6));
  this._tracks.push(new sbMockCDTOCEntry(116534, 226, 7));
  this._tracks.push(new sbMockCDTOCEntry(133832, 288, 8));
  this._tracks.push(new sbMockCDTOCEntry(155768, 258, 9));
  this._tracks.push(new sbMockCDTOCEntry(175400, 330, 10));
  this._tracks.push(new sbMockCDTOCEntry(200468, 331, 11));
}

sbMockCDTOC_AllThatYouCantLeaveBehind.prototype =
{
  __proto__: sbMockCDTOCBase.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOC])
};

var gAllThatYouCantLeaveBehindTOC = new sbMockCDTOC_AllThatYouCantLeaveBehind();

//------------------------------------------------------------------------------
//
// Mock CD device implementation.
//
//------------------------------------------------------------------------------

function sbMockCDDevice(aDeviceName)
{
  this._mName = aDeviceName;
}

sbMockCDDevice.prototype =
{
  _mIsDiskInserted: false,
  _mName:           "",
  _mDiscTOC:        null,

  get name()
  {
    return this._mName;
  },

  get readable()
  {
    return true;
  },

  get writeable()
  {
    return true;
  },

  get isDiskInserted()
  {
    return this._mIsDiskInserted;
  },

  get discTOC()
  {
    if (!this._mIsDiskInserted) {
      return null;
    }

    return this._mDiscTOC;
  },

  get diskType()
  {
    return Ci.sbICDDevice.AUDIO_DISC_TYPE;
  },

  eject: function sbMockCDDevice_eject()
  {
    this._mIsDiskInserted = false;
    this._mDiscTOC = null;

    this._mParentService._onMediaEjected(this);
  },

  _insertDiscTOC: function sbMockCDDevice_insertDiscTOC(aDiscTOC)
  {
    this._mDiscTOC = aDiscTOC;
    this._mIsDiskInserted = true;

    this._mParentService._onMediaInserted(this);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDDevice])
};

//------------------------------------------------------------------------------
//
// Mock CD service implementation.
//
//------------------------------------------------------------------------------

function sbMockCDService()
{
  this._mDevices = [];
  this._mListeners = [];

  this._mDevices.push(new sbMockCDDevice("Songbird MockCD Device 8000"));
  this._mDevices.push(new sbMockCDDevice("Songbird MockCD Device 7000"));
}

sbMockCDService.prototype =
{
  _mDevices:    null,
  _mListeners:  null,

  //
  // sbICDDeviceService:
  //
  get nbDevices()
  {
    return this._mDevices.length;
  },

  getDevice: function sbMockCDDevice_getDevice(aDeviceIndex)
  {
    return this._mDevices[aDeviceIndex];
  },

  getCDDevices: function sbMockCDService_getCDDevics()
  {
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
  // sbICDMockDeviceController
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
      case Ci.sbICDMockDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK:
        mockMediaTOC = gMidnightRockTOC;
        break;

      case Ci.sbICDMockDeviceController.MOCK_MEDIA_DISC2:
        mockMediaTOC = gBabyOneMoreTimeTOC;
        break;

      case Ci.sbICDMockDeviceController.MOCK_MEDIA_DISC_U2:
        mockMediaTOC = gAllThatYouCantLeaveBehindTOC;
        break;
    }

    curCDDevice._insertDiscTOC(mockMediaTOC);
  },

  ejectMedia: function sbMockCDService_ejectMedia(aCDDevice, aMediaDisc)
  {
    var curCDDevice = this._findDevice(aCDDevice);
    if (!curCDDevice || !curCDDevice.isDiskInserted) {
      return;
    }

    curCDDevice.eject();
  },

  _findDevice: function sbMockCDService_findDevice(aCDDevice)
  {
    // Return the local copy of our Mock CD Device that we have stashed in
    // |_mDevices|.
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
                                         Ci.sbICDMockDeviceController])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMockCDService]);
}

