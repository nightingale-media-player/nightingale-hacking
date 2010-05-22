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
 * \param aState the stage in configurating (not done, determined output, or
 *               fully configurated.
 */
function testHasConfigurated(configurator, aState) {

  const EXPECTED_VALUES = [
    {
      // not initialized
      "muxer" : new Error(),
      "videoEncoder" : new Error(),
      "videoFormat": new Error(),
      "videoEncoderProperties": new Error(),
      "audioEncoder": new Error(),
      "audioFormat": new Error(),
      "audioEncoderProperties": new Error()
    },
    {
      // determined output format, but not configurated
      "muxer" : "oggmux",
      "videoEncoder" : "theoraenc",
      "videoFormat": new Error(),
      "videoEncoderProperties": new Error(),
      "audioEncoder": "vorbisenc",
      "audioFormat": new Error(),
      "audioEncoderProperties": new Error()
    },
    {
      // configurated
      "muxer" : "oggmux",
      "videoEncoder" : "theoraenc",
      "videoFormat": {videoWidth: 1280,
                      videoHeight: 720},
      // Mock device supports a bitrate of up to 4 * 1024 * 1024; this is high
      // enough resolution to get limited by that, so use that.
      "videoEncoderProperties": {bitrate: 4194},
      "audioEncoder": "vorbisenc",
      "audioFormat": {sampleRate: 44100,
                      channels: 1},
      "audioEncoderProperties": {"max-bitrate": 250000}
    },
  ];

  var expected = EXPECTED_VALUES[aState];
  for (let prop in expected) {
    if (expected[prop] instanceof Error) {
      try {
        configurator[prop];
        doFail("attempt to get property " + prop + " unexpectedly succeeded; " +
               "got value: " + configurator[prop]);
      } catch (err) {
        // error expected
      }
    }
    else if (typeof(expected[prop]) == "string") {
      // not an error; look at the value (which may throw), and make sure
      // it's equal.
      assertEqual(configurator[prop], expected[prop],
                  "error in stage " + aState + ": " +
                  "attribute " + prop + " not equal");
    }
    else {
      for (let subprop in expected[prop]) {
        let result = null;
        if (configurator[prop] instanceof Ci.nsIPropertyBag2) {
          assertTrue(configurator[prop].hasKey(subprop),
                     "error in stage " + aState + ": " +
                     "property " + subprop + " of " +
                     "attribute " + prop + " missing");
          result = configurator[prop].getProperty(subprop);
        } else {
          result = configurator[prop][subprop];
        }
        assertEqual(result, expected[prop][subprop],
                    "error in stage " + aState + ": " +
                    "property " + subprop + " of " +
                    "attribute " + prop + " not equal");
      }
      // not implemented
    }
  }
}

function runTest() {
  // Set up a test library with a test item to use for the configurator.
  var testlib = createLibrary("test_transcodeconfigurator");
  var testFile = newAppRelativeFile(gTestFileLocation);
  testFile.append("transcode_configurator_test.mkv");
  var localTestFileURI = newFileURI(testFile);
  assertTrue(localTestFileURI, "failed to get configurator test media file");
  var testItem = testlib.createMediaItem(localTestFileURI, null, true);
  assertTrue(testItem, "failed to create media item");

  // Create a new configurator to test with.
  var configurator =
    Cc["@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/Device/GStreamer;1"]
      .createInstance(Ci.sbIDeviceTranscodingConfigurator);
  assertTrue(configurator, "failed to create configurator");
  // need to set an input URI so the error handling can report something;
  // the value actually used here isn't important.
  configurator.inputUri = newURI("data:text/plain,does_not_exist");

  // First test to make sure functions that need configurate called first throw
  // the NS_ERROR_NOT_INITIALIZED.
  testHasConfigurated(configurator, 0);

  // Test that determineOutputFormat fails due to lack of a device
  try {
    configurator.determineOutputType();
    doFail("configurator determineOutputType successful with no device set");
  } catch (err) {
    // expected fail
  }
  
  // Test the configurator by setting a device on it
  var device = Cc["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
                  .createInstance(Ci.sbIDevice);
  configurator.device = device;
  configurator.determineOutputType();
  testHasConfigurated(configurator, 1);

  // try to set the device again; this should fail
  try {
    configurator.device = null;
    doFail("set device succeeded after determining output type");
  } catch (err) {
    // expected failure
  }
  
  // Try to configurate before setting the input format.
  try {
    configurator.configurate();
    doFail("Configurate successfully called with out an input");
  }
  catch (err) {
    // All good
  }

  // We have to inspect a mediaItem in order to get a mediaFormat
  var mediaInspector =
    Cc["@songbirdnest.com/Songbird/Mediacore/mediainspector;1"]
      .createInstance(Ci.sbIMediaInspector);
  var inputFormat = mediaInspector.inspectMedia(testItem);
  assertTrue(inputFormat, "failed to inspect configurator test file");

  // Set the input format, this should not throw NS_ERROR_ALREADY_INITIALIZED
  configurator.inputFormat = inputFormat;
  assertEqual(configurator.inputFormat, inputFormat);

  configurator.configurate();

  // test to make sure we get back expected values.
  testHasConfigurated(configurator, 2);

  // Try to set again and this should throw NS_ERROR_ALREADY_INITIALIZED
  try {
    configurator.inputFormat = inputFormat;
    doFail("Setting inputFormat twice worked when it should not have");
  }
  catch (err) {
    // All good
  }

  return;
}
