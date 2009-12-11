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


var gTestFileLocation = "testharness/transcodeservice/files/";

/**
 * \brief Test to see if the attribute and functions in
 *  sbITranscodingConfigurator throw when Configurate has not been run.
 * \param configurator is the sbITranscodingConfigurator to use
 * \param hasRunConfigurate indicates if Configurate has run on the configurator
 *  object yet.
 * TODO: Add expected return values for each of the attributes when the
 * Configurate function is implemented.
 */
function testHasConfigurated(configurator, hasRunConfigurate) {

  function testAttribute(attributeName, testFunction) {
    log("Testing " + attributeName + " - " + hasRunConfigurate);
    try {
      var testAttributeValue = configurator[attributeName];
      if (!hasRunConfigurate) {
        // This should not be reached since the call would throw
        doFail("Attribute \"" + attributeName + "\" should throw since" +
               "Configurate was not called yet.");
      }
      else if (!testFunction(testAttributeValue)) {
        // We got something different than expected so fail
        doFail("Attribute \"" + attributeName + "\" returned" +
               " \"" + testAttributeValue + "\" instead of the expected value" +
               " \"" + expectedValue + "\"");
      }
    }
    catch (err) {
      if (hasRunConfigurate) {
        // The call should not have failed since Configurate ran
        doFail("Attribute \"" + attributeName + "\" threw when it was not" +
               "expected to.");
      }
    }
  }

  // TODO: Change these functions to actually compare expected values.
  testAttribute("muxer", function (value) { return true; });
  testAttribute("videoEncoder", function (value) { return true; });
  testAttribute("videoFormat", function (value) { return true; });
  testAttribute("videoEncoderProperties", function (value) { true; });
  testAttribute("audioEncoder", function (value) { true; });
  testAttribute("audioFormat", function (value) { true; });
  testAttribute("audioEncoderProperties", function (value) { true; });
}

function runTest() {
  // Set up a test library with a test item to use for the configurator.
  var testlib = createLibrary("test_transcodeconfigurator");
  var testFile = newAppRelativeFile(gTestFileLocation);
  testFile.append("transcode_configurator_test.mp3");
  var localTestFileURI = newFileURI(testFile);
  assertNotEqual(localTestFileURI, null);
  var testItem = testlib.createMediaItem(localTestFileURI, null, true);
  assertNotEqual(testItem, null);

  // Create a new configurator to test with.
  var configurator =
    Cc["@songbirdnest.com/Songbird/Mediacore/TranscodingConfigurator;1"]
      .createInstance(Ci.sbITranscodingConfigurator);
  assertNotEqual(configurator, null);

  // First test to make sure functions that need configurate called first throw
  // the NS_ERROR_NOT_INITIALIZED.
  testHasConfigurated(configurator, false);

  // Try to configurate before setting the input format.
  try {
    configurator.configurate();
    doFail("Configurate successfully called with out an input");
  }
  catch (err) {
    // All good
  }

  // We have to inspect a mediaItem in order to get a mediaFormat
/* TODO: Uncomment when inspectMedia is implemented.
  var mediaInspector =
    Cc["@songbirdnest.com/Songbird/Mediacore/mediainspector;1"]
      .createInstance(Ci.sbIMediaInspector);
  var inputFormat = mediaInspector.inspectMedia(testItem);
  assertNotEqual(inputFormat, null);

  // Set the input format, this should not throw NS_ERROR_ALREADY_INITIALIZED
  configurator.inputFormat = inputFormat;
  assertEqual(configurator.inputFormat, inputFormat);
*/
  // TODO: Remove the try/catch when configurate is implemented. See Bug 19145
  try {
    configurator.configurate();
  } catch (err) { }
/* TODO: Uncomment when inspectMedia is implemented.
  // Try to set again and this should throw NS_ERROR_ALREADY_INITIALIZED
  try {
    configurator.inputFormat = inputFormat;
    assertNotEqual(true,
                   "Setting inputFormat twice worked when it should not have");
  }
  catch (err) {
    // All good
  }
*/

  // Now we test to make sure we get back expected values.
  // TODO: Uncomment once configurate is implemented. See Bug 19145
  //testHasConfigurated(configurator, true);

  return;
}
