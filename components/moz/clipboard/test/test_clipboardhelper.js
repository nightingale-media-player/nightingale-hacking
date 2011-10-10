/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief test the clipboard helper
 */
var gFileLocation = "testharness/clipboardhelper/files/";

function runTest() {
  var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation), "_temp_reading_files");

  var sbClipboard = Cc["@getnightingale.com/moz/clipboard/helper;1"]
                      .createInstance(Ci.sbIClipboardHelper);


  // First we try to copy an image to the clipboard
  var imageFile = testFolder.clone();
  imageFile.append("test.png");
  
  var mimeType = Cc["@mozilla.org/mime;1"]
                   .getService(Ci.nsIMIMEService)
                   .getTypeFromFile(imageFile);
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  inputStream.init(imageFile, 0x01, 0600, 0);
  var imageSize = inputStream.available();
  // use a binary input stream to grab the bytes.
  var bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);
  var imageData= bis.readByteArray(imageSize);
  try {
    sbClipboard.pasteImageToClipboard(mimeType, imageData, imageSize);
  } catch (err) {
    log("Failed: " + err);
  }
/*
  // Then we try to read it from the clipboard
  mimeType = {};
  imageSize = {};
  imageData = sbClipboard.copyImageFromClipboard(mimeType, imageSize);
  assertNotEqual(imageSize.value, 0);
  assertNotEqual(mimeType.value, "");
  assertNotEqual(imageData.length, 0);*/
}
