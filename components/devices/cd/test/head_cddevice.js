/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//------------------------------------------------------------------------------
//
// Mock CD device test
//
//------------------------------------------------------------------------------

/**
 * List of callbacks to call on test complete
 */
var gTailCallback = [];

/**
 * set the CD lookup provider to be the test provider
 * and make sure we don't accidentally go over the internet to look for data
 */
(function head_ForceLookupProvider() {
  const K_PREF = "metadata.lookup.default_provider";
  const Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);

  var providerValue = Application.prefs.get(K_PREF);
  Application.prefs.setValue(K_PREF, "TestProvider");
  gTailCallback.push(function(){
    if (providerValue && providerValue.value) {
      Application.prefs.setValue(K_PREF, providerValue.value);
    } else {
      pref = Application.prefs.get(K_PREF);
      if (pref) {
        pref.reset();
      }
    }
  });
})();

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
  var tocClass = Cc["@getnightingale.com/Nightingale/MockCDTOC;1"];
  var toc = tocClass.createInstance(Ci.sbIMockCDTOC);
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

function sbMakeInstantJunk()
{
  var toc = newTOC(1, 1, 131072);
  toc.addTocEntry(32, 32, 0, Ci.sbICDTOCEntry.TRACKMODE_AUDIO);
  return toc;
}

function sbCreateDevice(deviceName, toc)
{
  var deviceController = Cc["@getnightingale.com/Nightingale/CDDeviceController;1"]
                           .createInstance(Ci.sbIDeviceController);
  var cdDevice = Cc["@getnightingale.com/Nightingale/MockCDDevice;1"]
                   .createInstance(Ci.sbIMockCDDevice)
                   .QueryInterface(Ci.sbICDDevice);

  cdDevice.initialize(deviceName, 
                      true, 
                      false, 
                      true, 
                      Ci.sbIDeviceController.AUDIO_DISC_TYPE,
                      false,
                      null);
  if (toc) {
    cdDevice.setDiscTOC(toc);
  }
  var deviceParams = new Object;
  deviceParams["sbICDDevice"] = cdDevice;
  var sbDevice = deviceController.createDevice(createPropertyBag(deviceParams));
  sbDevice.connect();
  return sbDevice;
}
