/* vim: set sw=2 : miv*/
/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

/**
 * \brief Test that the audio processing API acts as expected on some short
 *        sample files.
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  this.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  this.Ci = Components.interfaces;

var TEST_FILES = newAppRelativeFile("testharness/gstreamer/files");

function checkFormat(listener, processor, test, eventDetails) {
  var format = eventDetails.QueryInterface(Ci.sbIMediaFormatAudio);

  assertTrue(format.sampleRate == test.expectedRate,
      "Expected sample rate "+test.expectedRate+
          " differed from actual sample rate "+format.sampleRate);
 
  assertTrue(format.channels == test.expectedChannels,
      "Expected channel count "+test.expectedChannels+
          " differed from actual channel count "+format.channels);

  var actualFormat;
  if (format.audioType == "audio/x-int")
    actualFormat = Ci.sbIMediacoreAudioProcessor.FORMAT_INT16;
  else
    actualFormat = Ci.sbIMediacoreAudioProcessor.FORMAT_FLOAT;

  assertTrue(
      test.expectedFormat == 0 || actualFormat == test.expectedFormat,
      "Expected format "+test.expectedFormat+
          " differed from actual format "+actualFormat);

  return true;
};

function pauseTest(listener, processor, test, eventDetails) {
  processor.suspend();

  // Unpause in a bit. No events or data should arrive in the middle.
  doTimeout(200, function() {
      processor.resume();

      listener.nextSequence();
    });

  // Don't automatically continue to the next item - we do that once our
  // timeout expires. This ensures that we check that no additional data is
  // arriving before we restart the processor.
  return false;
};

function startAgain(listener, processor, test, eventDetails) {
  var startFailed = false;
  try {
    processor.start();
  } catch (e) {
    startFailed = true;
  }

  assertTrue(startFailed, "Calling start() again did not fail");

  return true;
};

function stopProcessing(listener, processor, test, eventDetails) {
  processor.stop();

  return true;
};

function pauseMomentarily(listener, processor, test, unused) {
  processor.suspend();

  // Unpause as soon as possible.
  doTimeout(0, function() {
    processor.resume();
  });
};

const K_TEST_CASES = [
  {
    description: "simple unconstrained decode",
    filename: "simple.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    expectedRate:        44100,
    expectedChannels:    2,
    expectedBlockSize:   0, // Will vary, we don't care.
    expectedFormat:      Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.
 
    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: checkFormat },
      {expected: "samples", sampleCount: 20480, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "constrained decode to mono",
    filename: "simple.ogg",

    constraintRate:      22050,
    constraintChannels:  1,
    constraintBlockSize: 2048,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_FLOAT,

    expectedRate:        22050,
    expectedChannels:    1,
    expectedBlockSize:   2048,
    expectedFormat:      Ci.sbIMediacoreAudioProcessor.FORMAT_FLOAT,
 
    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: checkFormat },
      {expected: "samples", sampleCount: 5120, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "unconstrained 5.1 channel vorbis",
    filename: "surround51.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY,

    expectedRate:        44100,
    expectedChannels:    2, // Note that despite not setting a constraint, we
                            // expect 2 channels out of a 5.1 file here - more
                            // than 2 channels aren't supported.
    expectedBlockSize:   0,
    expectedFormat:      Ci.sbIMediacoreAudioProcessor.FORMAT_ANY,
 
    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: checkFormat },
      {expected: "samples", sampleCount: 20480, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "pause/unpause testing",
    filename: "simple.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 1000,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    expectedRate:        44100,
    expectedChannels:    2,
    expectedBlockSize:   1000,
    expectedFormat:      Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.
 
    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: checkFormat },
      {expected: "samples", sampleCount: 10000, action: pauseTest, blockAction: null },
      {expected: "samples", sampleCount: 10480, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "pause/unpause every block testing",
    filename: "simple.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 1000,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    expectedRate:        44100,
    expectedChannels:    2,
    expectedBlockSize:   1000,
    expectedFormat:      Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.
 
    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: checkFormat },
      {expected: "samples", sampleCount: 20480, action: null, blockAction: pauseMomentarily },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "multiple calls to start",
    filename: "simple.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: startAgain },
      {expected: "samples", sampleCount: 20480, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  },
  {
    description: "stop in the middle",
    filename: "simple.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 1000,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: startAgain },
      {expected: "samples", sampleCount: 12000, action: stopProcessing, blockAction: null }
    ]
  },
  {
    description: "video only file",
    filename: "video.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_ERROR, action: null },
    ]
  },
  {
    description: "file that doesn't exist",
    filename: "nothere.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY, // We don't care.

    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_ERROR, action: null },
    ]
  },
  {
    description: "gap test (corrupt data in middle of file)",
    filename: "corrupt-in-middle.ogg",

    constraintRate:      0,
    constraintChannels:  0,
    constraintBlockSize: 0,
    constraintFormat:    Ci.sbIMediacoreAudioProcessor.FORMAT_ANY,

    sequence: [
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_START, action: null},
      {expected: "samples", sampleCount: 21184, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_GAP, action: null },
      // Note: gap handling in GStreamer's vorbisdec element changed upstream.
      // After the next update to GStreamer, we'll probably need to fix this
      // value up to be 58688.
      {expected: "samples", sampleCount: 59712, action: null, blockAction: null },
      {expected: "event", eventType: Ci.sbIMediacoreAudioProcessorListener.EVENT_EOS, action: null }
    ]
  }

];

function runAudioProcessingTest(library, test)
{
  var processor = Cc["@songbirdnest.com/Songbird/Mediacore/GStreamer/AudioProcessor;1"]
                    .createInstance(Ci.sbIMediacoreAudioProcessor);
  assertTrue(processor, "failed to create processor");

  var listener = {
    samplesCounted : 0,

    seqIdx : 0,
    
    onIntegerAudioDecoded : function(timestamp, numSamples, sampleData) {
      this.onAudio(timestamp, numSamples);
    },

    onFloatAudioDecoded : function(timestamp, numSamples, sampleData) {
      this.onAudio(timestamp, numSamples);
    },

    onAudio : function(timestamp, numSamples) {
      assertTrue(this.seqIdx < test.sequence.length, "Audio after end!");
      var expectingSamples = test.sequence[this.seqIdx].expected == "samples";
      assertTrue(expectingSamples,
          "Got audio samples when not expecting any");
      assertTrue(test.constraintBlockSize == 0 ||
                 numSamples <= test.constraintBlockSize,
          "Received incorrect block size");

      var numSamplesExpected =  test.sequence[this.seqIdx].sampleCount;

      this.samplesCounted += numSamples;

      assertTrue(this.samplesCounted <= numSamplesExpected,
          "Received more samples than expected");

      if (test.sequence[this.seqIdx].blockAction != null)
      {
        test.sequence[this.seqIdx].blockAction(this, processor, test, null);
      }

      if (this.samplesCounted == numSamplesExpected) {
        this.samplesCounted = 0;
        this.nextAction(null);
      }
    },
    
    onEvent : function(eventType, details) {
      assertTrue(this.seqIdx < test.sequence.length, "Event after end!");
      var expectingEvent = test.sequence[this.seqIdx].expected == "event";
      assertTrue(expectingEvent,
          "Got event of type "+eventType+" when not expecting an event");

      var expectedEventType = test.sequence[this.seqIdx].eventType;
      assertEqual(expectedEventType, eventType);

      this.nextAction(details);
    },

    nextAction: function(data) {
      // Do next action, if any.
      var continueToNextSequence = true;
      if (test.sequence[this.seqIdx].action != null)
        continueToNextSequence = test.sequence[this.seqIdx].action(
                this, processor, test, data);

      if (continueToNextSequence)
        this.nextSequence();
    },

    nextSequence: function() {
      this.seqIdx++;
      // Check if we're done now.
      if (this.seqIdx == test.sequence.length) {
        processor.stop();
        // Clear processor object to avoid leaks.
        processor = null;
        testFinished();
      }
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacoreAudioProcessorListener])
  };

  processor.init(listener);

  processor.constraintSampleRate = test.constraintRate;
  processor.constraintChannelCount = test.constraintChannels;
  processor.constraintBlockSize = test.constraintBlockSize;
  processor.constraintAudioFormat = test.constraintFormat;

  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);

  // Set up a media item for test.filename
  var file = TEST_FILES.clone();
  file.append(test.filename);

  var fileURI = ioService.newFileURI(file, false);
  var mediaItem = library.createMediaItem(fileURI);
  processor.start(mediaItem);

  // Now we wait!
  testPending();
}


function runTest() {
  // Set up a test library to use for the media items we process
  var testlib = createLibrary("test_audio_processing");

  for each (var testcase in K_TEST_CASES) {
    log("Checking testcase [" + testcase.description + "]");

    runAudioProcessingTest(testlib, testcase);
  }

  return;
}
