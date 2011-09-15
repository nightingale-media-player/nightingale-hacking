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

EXPORTED_SYMBOLS = [ "sbCDDeviceUtils" ];

Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

var sbCDDeviceUtils = {
  
  /**
   * Start a CD Lookup Process for the device.
   * \param aDevice - device to perform lookup on.
   */
  doCDLookUp: function(aDevice) {
    // Invoke the CD Lookup again
    var bag = Cc["@mozilla.org/hash-property-bag;1"]
                .createInstance(Ci.nsIPropertyBag2);
    aDevice.submitRequest(Ci.sbICDDeviceEvent.REQUEST_CDLOOKUP, bag);
  },
  
  /**
   * Rip any selected tracks from the cd device to the main library.
   * \param aDevice - device to rip tracks from.
   */
  doCDRip: function(aDevice) {
    var deviceLibrary = this._getDeviceLibrary(aDevice);
    try {
      // Get all the selected tracks from the device library
      var addItems = deviceLibrary.getItemsByProperty(SBProperties.shouldRip,
                                                      "1");
  
      //reset the rip status of the items to be ripped
      var ripItemsEnum = addItems.enumerate();
      while (ripItemsEnum.hasMoreElements()) {
        var ripItem = ripItemsEnum.getNext();
        ripItem.setProperty(SBProperties.cdRipStatus, null);
      }

      // Then add them to the main library using addSome(enumerator)
      LibraryUtils.mainLibrary.addSome(addItems.enumerate());
    } catch (err) {
      Cu.reportError("Error occurred when trying to rip tracks from CD: " + err);
    }
  },
  
  /**
   * Get the devices library (defaults to first library)
   * \param aDevice - device to get library from
   * \param aIndex - Library index to retrieve
   */
  _getDeviceLibrary: function _getDeviceLibrary(aDevice, aIndex) {
    if (typeof(aIndex) == "undefined") {
      aIndex = 0;
    }
    
    // Get the libraries for device
    var libraries = aDevice.content.libraries;
    if (libraries.length < 1) {
      // Oh no, we have no libraries
      Cu.reportError("Device " + aDevice.id + " has no libraries!");
      return null;
    }

    // Get the requested library
    var deviceLibrary = libraries.queryElementAt(aIndex, Ci.sbIMediaList);
    if (!deviceLibrary) {
      Cu.reportError("Unable to get library " + aIndex + " for device: " +
                     aDevice.id);
      return null;
    }
    
    return deviceLibrary;
  }
};
