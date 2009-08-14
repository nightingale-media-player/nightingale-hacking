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

function sbMockCDTOC_BurnshitHere()
{
  // Call super constructor
  sbMockCDTOCBase.call(this);

  this._firstTrackIndex    = 1;
  this._lastTrackIndex     = 11;
  this._leadOutTrackOffset = 160183;

  // tracks:
  this._tracks = [];

  // FreeDB value:
  // http://www.freedb.org/freedb/rock/9908570b
  this._tracks.push(new sbMockCDTOCEntry(150, 204, 0));
  this._tracks.push(new sbMockCDTOCEntry(15497, 221, 1));
  this._tracks.push(new sbMockCDTOCEntry(32131, 163, 2));
  this._tracks.push(new sbMockCDTOCEntry(44400, 164, 3));
  this._tracks.push(new sbMockCDTOCEntry(56734, 141, 4));
  this._tracks.push(new sbMockCDTOCEntry(67365, 176, 5));
  this._tracks.push(new sbMockCDTOCEntry(80595, 249, 6));
  this._tracks.push(new sbMockCDTOCEntry(99276, 222, 7));
  this._tracks.push(new sbMockCDTOCEntry(115938, 218, 8));
  this._tracks.push(new sbMockCDTOCEntry(132320, 168, 9));
  this._tracks.push(new sbMockCDTOCEntry(144978, 204, 10));

  /*
  // CD Value
  this._tracks.push(new sbMockCDTOCEntry(0, 204, 1));
  this._tracks.push(new sbMockCDTOCEntry(15347, 221, 2));
  this._tracks.push(new sbMockCDTOCEntry(31981, 163, 3));
  this._tracks.push(new sbMockCDTOCEntry(44250, 164, 4));
  this._tracks.push(new sbMockCDTOCEntry(56584, 141, 5));
  this._tracks.push(new sbMockCDTOCEntry(67215, 176, 6));
  this._tracks.push(new sbMockCDTOCEntry(80445, 249, 7));
  this._tracks.push(new sbMockCDTOCEntry(99126, 222, 8));
  this._tracks.push(new sbMockCDTOCEntry(115788, 218, 9));
  this._tracks.push(new sbMockCDTOCEntry(132170, 168, 10));
  this._tracks.push(new sbMockCDTOCEntry(144828, 204, 11));
  */
}

sbMockCDTOC_BurnshitHere.prototype =
{
  __proto__: sbMockCDTOCBase.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDTOC])
};

var gBurnshitHereTOC = new sbMockCDTOC_BurnshitHere();

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

    //
    // XXXkreeger - Probably need to invoke listeners here, definately a hole
    //              in the current IDL format.
    //  @see bug 17454.
    //
  },

  _setDiscTOC: function sbMockCDDevice_setDiscTOC(aDiscTOC)
  {
    this._mDiscTOC = aDiscTOC;
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

    curCDDevice._mIsDiskInserted = true;

    // Set the media disc TOC.
    var mockMediaTOC = 
      aMediaDisc == Ci.sbICDMockDeviceController.MOCK_MEDIA_DISC1 ?
                      gMidnightRockTOC : gBurnshitHereTOC;
    curCDDevice._setDiscTOC(mockMediaTOC);

    // Notify the listeners that media was inserted.
    this._mListeners.forEach(
      function(aListener) {
        try {
          aListener.handleEvent(Ci.sbICDDeviceListener.DEVICE_ADDED,
                                curCDDevice.QueryInterface(Ci.sbICDDevice));
        }
        catch (e) {
          Cu.reportError(e);
        }
      }
    );
  },

  ejectMedia: function sbMockCDService_ejectMedia(aCDDevice, aMediaDisc)
  {
    var curCDDevice = this._findDevice(aCDDevice);
    if (!curCDDevice || !curCDDevice.isDiskInserted) {
      return;
    }

    curCDDevice._mIsDiskInserted = false;

    // Notify the listeners that media was inserted.
    this._mListeners.forEach(
      function(aListener) {
        try {
          aListener.handleEvent(Ci.sbICDDeviceListener.DEVICE_REMOVED,
                                curCDDevice.QueryInterface(Ci.sbICDDevice));
        }
        catch (e) {
          Cu.reportError(e);
        }
      }
    );
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

  classDescription: "Songbird Mock CD Device Service",
  classID: Components.ID("{C701A410-7F71-41E9-A07A-C765A4F04D41}"),
  contractID: "@songbirdnest.com/device/cd/mock-cddevice-service;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.sbICDDeviceService,
                                         Ci.sbICDMockDeviceController])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMockCDService]);
}

