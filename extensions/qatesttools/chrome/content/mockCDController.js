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

const INSERT_LABEL = "Insert Into Drive ";
const EJECT_LABEL  = "Eject Drive ";


var DialogController =
{
  // sbICDDevice instances
  _device1: null,
  _device2: null,

  // XUL Elements
  _buttonDevice1:   null,
  _buttonDevice2:   null,
  _albumRadioGroup: null,

  onDialogLoad: function()
  {
    var mockCDService =
      Cc["@songbirdnest.com/device/cd/mock-cddevice-service;1"]
        .getService(Ci.sbICDDeviceService);

    this._device1 = mockCDService.getDevice(0);
    this._device2 = mockCDService.getDevice(1);

    mockCDService.registerListener(this);

    this._buttonDevice1 = document.getElementById("mockcd-drive1-button");
    this._buttonDevice2 = document.getElementById("mockcd-drive2-button");
    this._albumRadioGroup = document.getElementById("album-radiogroup");

    this._update();
  },

  onDriveAction: function(aDeviceNum)
  {
    var device = (aDeviceNum == 1 ? this._device1 : this._device2);
    this._handleDriveAction(device);
  },

  _handleDriveAction: function(aDevice)
  {
    var mockCDService =
      Cc["@songbirdnest.com/device/cd/mock-cddevice-service;1"]
        .getService(Ci.sbICDDeviceService)
        .QueryInterface(Ci.sbIMockCDDeviceController);

    if (aDevice.isDiscInserted) {
      mockCDService.ejectMedia(aDevice);
    }
    else {
      // Insert the disc TOC based on the value of the selected radio item
      var mediaIndex = parseInt(this._albumRadioGroup.selectedItem.value);
      mockCDService.insertMedia(aDevice, mediaIndex); 
    }
  },

  _update: function()
  {
    // Update button labels
    if (this._device1.isDiscInserted) {
      this._buttonDevice1.setAttribute("label", EJECT_LABEL + "1");
    }
    else {
      this._buttonDevice1.setAttribute("label", INSERT_LABEL + "1");
    }
    if (this._device2.isDiscInserted) {
      this._buttonDevice2.setAttribute("label", EJECT_LABEL + "2");
    }
    else {
      this._buttonDevice2.setAttribute("label", INSERT_LABEL + "2");
    }
  },

  // sbICDDeviceListener
  onDeviceRemoved: function(aCDDevice)
  {
  },

  onMediaInserted: function(aCDDevice)
  {
    dump("\n\n\n   onMediaInserted: " + aCDDevice.name + "\n\n\n");
    this._update();
  },

  onMediaEjected: function(aCDDevice)
  {
    dump("\n\n\n   onMediaEjected: " + aCDDevice.name + "\n\n\n");
    this._update();
  },
};

