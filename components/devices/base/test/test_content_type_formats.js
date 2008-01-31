/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

/**
 * \brief Content Type Formats test
 */

Components.utils.import("resource://app/components/ArrayConverter.jsm");

/* return a new random FourCC. well, smells like one anyway. */
function generateFourCC() {
  // note that we generate a random 32-bit *unsigned* integer.
  return (Math.random() * (-1 >>> 0)) << 0 >>> 0;
}

function CompareResults(aJSArray, aEnumerator, aMessage) {
  var result = ArrayConverter.JSArray(aEnumerator);
  assertEqual(aJSArray.length, result.length, aMessage);
  
  const nsISupportsPRUint32 = Components.interfaces.nsISupportsPRUint32;
  
  for (var i = 0; i < result.length; ++i) {
    var data = result[i].QueryInterface(nsISupportsPRUint32).data;
    assertEqual(aJSArray[i], data, aMessage + "[" + i + "]");
  }
}

function runTest () {
  var formats = Components.classes["@songbirdnest.com/Songbird/Device/ContentTypeFormat;1"]
                          .createInstance(Components.interfaces.sbIContentTypeFormat);
  assertTrue(formats, "Failed to create content type format component");
  
  var FCCContainer = generateFourCC();
  var FCCEncode = [], FCCDecode = [];
  for (var i = 0; i < 10; ++i) {
    FCCEncode.push(generateFourCC());
    FCCDecode.push(generateFourCC());
  }
  
  formats.init(FCCContainer, FCCEncode, FCCEncode.length, FCCDecode, FCCDecode.length);

  try {
    formats.init(FCCContainer, FCCEncode, FCCEncode.length, FCCDecode, FCCDecode.length);
    fail("re-initialization of content type format did not throw");
  } catch (ex) {
    /* expected to throw */
  }

  assertEqual(formats.containerFormat, FCCContainer);
  CompareResults(FCCEncode, formats.encodingFormats, "encodingFormats");
  CompareResults(FCCDecode, formats.decodingFormats, "decodingFormats");
}
