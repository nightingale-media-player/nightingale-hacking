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

//------------------------------------------------------------------------------
//
// Mock CD device test
//
//------------------------------------------------------------------------------

function createPropertyBag(aParams) {
  var bag = Cc["@mozilla.org/hash-property-bag;1"]
              .createInstance(Ci.nsIWritablePropertyBag);
  for (var name in aParams) {
    bag.setProperty(name, aParams[name]);
  }
  return bag;
}

function newTOC(aFirstTrack, aLastTrack, aLead)
{
  var tocClass = Cc["@songbirdnest.com/Songbird/MockCDTOC;1"];
  var toc = tocClass.createInstance(Ci.sbIMockCDTOC);
  toc.initialize(aFirstTrack, aLastTrack, aLead);
 
  return toc;
}

function sbMakeMidnightRock()
{
  var toc = newTOC(1, 15, 285675);

  // tracks:
  toc.addTocEntry(32, 309, 0);
  toc.addTocEntry(23260, 231, 0);
  toc.addTocEntry(40612, 242, 0);
  toc.addTocEntry(58770, 191, 0);
  toc.addTocEntry(73145, 310, 0);
  toc.addTocEntry(96415, 290, 0);
  toc.addTocEntry(118232, 301, 0);
  toc.addTocEntry(140867, 259, 0);
  toc.addTocEntry(160322, 316, 0);
  toc.addTocEntry(184085, 222, 0);
  toc.addTocEntry(200777, 236, 0);
  toc.addTocEntry(218535, 185, 0);
  toc.addTocEntry(232437, 211, 0);
  toc.addTocEntry(248320, 184, 0);
  toc.addTocEntry(262145, 313, 0);
  
  return toc;
}

function sbCreateDevice(deviceName, toc)
{
  var deviceController = Cc["@songbirdnest.com/Songbird/CDDeviceController;1"]
                           .createInstance(Ci.sbIDeviceController);
  var cdDevice = Cc["@songbirdnest.com/Songbird/MockCDDevice;1"]
                   .createInstance(Ci.sbIMockCDDevice);

  cdDevice.initialize(deviceName, 
                      true, 
                      false, 
                      true, 
                      Ci.sbIDeviceController.AUDIO_DISC_TYPE,
                      false,
                      deviceController.sbCDMockDeviceController);
  if (toc) {
    cdDevice.sbICDDevice.discTOC = toc;
  }
  var deviceParams = { sbICDDevice : cdDevice };
  var sbDevice = deviceController.createDevice(createPropertyBag(deviceParams));
  sbDevice.connect();
  return sbDevice;
}