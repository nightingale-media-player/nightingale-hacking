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

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// @brief Unit test helper service for setting the URL's of the mock firmware
//        resource file locations.
//
//------------------------------------------------------------------------------

function sbMockDeviceFirmwareHandlerURLService()
{
}

sbMockDeviceFirmwareHandlerURLService.prototype =
{
  // sbPIMockFirmwareHandlerURLService
  firmwareURL:     "",
  resetURL:        "",
  releaseNotesURL: "",
  supportURL:     "",
  registerURL:     "",

  // XPCOM Stuff:
  classDescription: "Songbird Mock Firmware Handler URL Service",
  classID:          Components.ID("{c0a0ebce-1dd1-11b2-afed-a5eeb1302690}"),
  contractID:       "@songbirdnest.com/mock-firmware-url-handler;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbPIMockFirmwareHandlerURLService]),
};

//------------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbMockDeviceFirmwareHandlerURLService]);
