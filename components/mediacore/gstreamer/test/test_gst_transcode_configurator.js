/* vim: set sw=2 : miv*/
/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://songbirdnest.com
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
 * \brief Test that the configurator gives expected results when deciding on
 * the output formats.  This doesn't operate on real files, just feeds in the
 * fake inputs and hopes it comes out right.
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

/**
 * \brief Create a property bag, and do a shallow copy of another property bag
 * if given
 */
function CopyProperties(aPropertyBag) {
  var newProps = Cc["@songbirdnest.com/moz/xpcom/sbpropertybag;1"]
                  .createInstance(Ci.nsIWritablePropertyBag2);
  if (aPropertyBag) {
    // stuff the property bag into a SIP to make sure instanceof works correctly
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = aPropertyBag;
    if (sip.data instanceof Ci.nsIPropertyBag) {
      for each (var prop in ArrayConverter.JSEnum(sip.data.enumerator)) {
        newProps.setProperty(prop.name, prop.value);
      }
    }
    else {
      // not a property bag; assume this is a simple JS object
      for (var prop in aPropertyBag) {
        newProps.setProperty(prop, aPropertyBag[prop]);
      }
    }
  }
  return newProps;
}

/**
 * \brief Shorthand for creating a fraction
 */
function F(aNumerator, aDenominator) {
  return {
    numerator: aNumerator,
    denominator: aDenominator,
    QueryInterface: XPCOMUtils.generateQI([Ci.sbIDevCapFraction])
  };
}

/**
 * \brief Make a sbIDevCapRange
 * The input can either be an array (in which case it's used as explicit values)
 * or an object with "min", "step", and "max" properties.
 */
function sbDevCapRange(aInput) {
  var range = Cc["@songbirdnest.com/Songbird/Device/sbrange;1"]
                .createInstance(Ci.sbIDevCapRange);
  if (aInput === null) {
    // possibly no size ranges, or something
    return null;
  }
  if (aInput instanceof Array) {
    for each (let val in aInput) {
      range.AddValue(val);
    }
  }
  else {
    range.Initialize(aInput.min, aInput.max, aInput.step);
  }
  return range;
}

/*
 * The default input specifications
 */
const K_DEFAULT_INPUT = {
  inputUri: "x-transcode-test:default-input",
  container: {
    containerType: "container/default",
    properties: CopyProperties(null)
  },
  videoStream: {
    videoType: "video/default",
    videoWidth: 640,
    videoHeight: 480,
    PAR: F(1, 1),
    frameRate: F(30000, 1001),
    bitRate: 500000,
    getVideoPAR: function videoFormat_getVideoPAR(aNum, aDenom) {
      aNum.value = this.PAR.numerator;
      aDenom.value = this.PAR.denominator;
    },
    getVideoFrameRate: function videoFormat_getVideoFrameRate(aNum, aDenom) {
      aNum.value = this.frameRate.numerator;
      aDenom.value = this.frameRate.denominator;
    },
    properties: CopyProperties(null)
  },
  audioStream: {
    audioType: "audio/default",
    bitRate: 128000,
    sampleRate: 44100,
    channels: 2,
    properties: CopyProperties(null)
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaFormat])
};

/*
 * The default output specifications (device caps)
 */
const K_DEFAULT_CAPS = {
  type: "video",
  containerType: "application/ogg",
  video: {
    type: "video/x-theora",
    explicitSizes: [{width: 320, height: 240}],
    widths: {min: 16, step: 16, max: 320},
    heights: {min: 16, step: 16, max: 240},
    PARs: [F(1, 1)],
    frameRates: [F(15, 1), F(30000, 1001)],
    bitRates: {min: 0, step:1, max: 4000000}
  },
  audio: {
    type: "audio/x-vorbis",
    bitRates: {min: 0, step: 1, max: 400000},
    sampleRates: [44100],
    channels: [1, 2]
  }
};

/*
 * The default output values
 */
const K_DEFAULT_OUTPUT = {
  muxer: "oggmux",
  fileExtension: "ogg",
  videoEncoder: "theoraenc",
  audioEncoder: "vorbisenc",
  videoFormat: {
    width: 320,
    height: 240,
    PAR: F(1, 1),
    frameRate: F(30000, 1001),
    properties: {}
  },
  audioFormat: {
    sampleRate: 44100,
    channels: 2,
    properties: {}
  }
};

function checkBitrate(aCaps, aBitrate, aMin, aMax)
{
  assertTrue(aBitrate >= aCaps.video.bitRates.min / 1000);
  assertTrue(aBitrate <= aCaps.video.bitRates.max / 1000);
  assertTrue(aBitrate >= aMin, "Bitrate too low");
  assertTrue(aBitrate <= aMax, "Bitrate too high");
  return true;
}

const K_TEST_CASES = [
  { description: "default",
    output: {
      videoFormat: {
        properties: {
          bitrate: function(bitrate) {
            return checkBitrate(this.caps, bitrate, 600, 700);
          }
        }
      },
      audioFormat: {
        properties: {
          "max-bitrate": 128000
        }
      }
    }
  },
  { description: "scale down with empty areas",
    input: {
      videoStream: {
        videoWidth: 1280,
        videoHeight: 720
      }
    },
    output: {
      videoFormat: {
        width: 320,
        height: 192, // 320 / 16:9 = 180, rounded up to nearest multiple of 16
        properties: {
          bitrate: function(bitrate) {
            return checkBitrate(this.caps, bitrate, 500, 600);
          }
        }
      }
    }
  },
  { description: "scale down with padding",
    input: {
      videoStream: {
        videoWidth: 1280,
        videoHeight: 720
      }
    },
    caps: {
      video: {
        // We still use explicitSizes here (forcing 320x240)
        widths: null,
        heights: null
      }
    },
    output: {
      videoFormat: {
        width: 320,
        height: 240
      }
    }
  },
  { description: "scale up with empty areas",
    input: {
      videoStream: {
        videoWidth: 16,
        videoHeight: 16
      }
    },
    caps: {
      video: {
        widths: {min: 320, max: 1024, step: 16},
        heights: {min: 240, max: 1024, step: 16}
      }
    },
    output: {
      videoFormat: {
        width: 320,
        height: 320
      }
    }
  },
  { description: "scale up with padding",
    input: {
      videoStream: {
        videoWidth: 16,
        videoHeight: 16
      }
    },
    caps: {
      video: {
        widths: null,
        heights: null
      }
    },
    output: {
      videoFormat: {
        width: 320,
        height: 240
      }
    }
  },
  { description: "reduce fps",
    input: {
      videoStream: {
        frameRate: F(24000, 1001)
      }
    },
    caps: {
      video: {
        frameRates: [F(15, 1), F(60000, 1001)]
      }
    },
    output: {
      videoFormat: {
        frameRate: F(15, 1)
      }
    }
  },
  { description: "increase fps",
    input: {
      videoStream: {
        frameRate: F(24000, 1001)
      }
    },
    caps: {
      video: {
        frameRates: [F(15, 1), F(30000, 1001)]
      }
    },
    output: {
      videoFormat: {
        frameRate: F(30000, 1001)
      }
    }
  },
  { description: "no change",
    input: {
      videoStream: {
        videoWidth: 800,
        videoHeight: 600,
        PAR: F(1, 1),
        frameRate: F(24000, 1001)
      }
    },
    caps: {
      video: {
        widths: {min: 640, step: 1, max: 1024},
        heights: {min: 480, step: 1, max: 768},
        PARs: [F(1, 1), F(2, 1), F(3, 2)],
        frameRates: {min: F(0, 1), max: F(30000, 1001)},
        bitRates: {min: 1, step: 1, max: 2000000}
      }
    },
    output: {
      videoFormat: {
        width: 800,
        height: 600,
        PAR: F(1, 1),
        frameRate: F(24000, 1001),
        properties: {
          bitrate: function(bitrate) {
            // Max bitrate is 2000000; ensure we capped it to this for this
            // high-res test.
            return checkBitrate(this.caps, bitrate, 2000, 2000);
          }
        }
      }
    }
  },
  { description: "favour size over quality",
    input: {
      videoStream: {
        videoWidth: 1920,
        videoHeight: 816,
        PAR: F(1, 1),
        frameRate: F(24000, 1001)
      }
    },
    caps: {
      video: {
        widths: {min: 1, max: 720, step: 1},
        heights: {min: 1, max: 576, step: 1},
        bitRates: {min: 0, max: 1536000, step: 1},
        frameRates: {min: F(0, 1), max: F(30000, 1001)}
      }
    },
    output: {
      videoFormat: {
        width: 720,
        height: 306,
        frameRate: F(24000, 1001)
      }
    }
  }
];

/**
 * Convert suported PARs / frame rates into an nsIArray
 * The input can either be an array (in which case it's not a range),
 * or an object with "min" and "max" properties, each of which is a fraction
 * (in which case it is a range)
 */
function fromFractionRange(aFractions) {
  return ArrayConverter.nsIArray((aFractions instanceof Array) ?
                                 aFractions :
                                 [aFractions.min, aFractions.max]);
}

function runTest() {
  // Set up a test library to use for the configurator.
  var testlib = createLibrary("test_gst_configurator");

  for each (var testcase in K_TEST_CASES) {
    log("Checking testcase [" + testcase.description + "]");
    // Create a new configurator to test with.
    var configurator =
      Cc["@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/Device/GStreamer;1"]
        .createInstance(Ci.sbIDeviceTranscodingConfigurator);
    assertTrue(configurator, "failed to create configurator");

    // fix up the test case to inherit from the default
    if (!testcase.hasOwnProperty("input")) {
      testcase.input = {};
    }
    testcase.input.__proto__ = K_DEFAULT_INPUT;
    if (!testcase.hasOwnProperty("caps")) {
      testcase.caps = {};
    }
    testcase.caps.__proto__ = K_DEFAULT_CAPS;
    if (!testcase.hasOwnProperty("output")) {
      testcase.output = {};
    }
    testcase.output.__proto__ = K_DEFAULT_OUTPUT;
    for each (let prop in ["container", "videoStream", "audioStream"]) {
      if (testcase.input.hasOwnProperty(prop)) {
        testcase.input[prop].__proto__ = K_DEFAULT_INPUT[prop];
      }
    }
    for each (let prop in ["video", "audio"]) {
      if (testcase.caps.hasOwnProperty(prop)) {
        testcase.caps[prop].__proto__ = K_DEFAULT_CAPS[prop];
      }
    }
    for each (let prop in ["videoFormat", "audioFormat"]) {
      if (testcase.output.hasOwnProperty(prop)) {
        testcase.output[prop].__proto__ = K_DEFAULT_OUTPUT[prop];
      }
    }

    // need to set an input URI so the error handling can report something;
    // the value actually used here isn't important.
    configurator.inputUri = newURI(testcase.input.inputUri);

    // the input format interface is simple enough to use the JS object directly
    configurator.inputFormat = testcase.input;

    // the device caps is not so simple; we need to construct something more
    // complex to appropriately fake all the interfaces
    var videoCaps = Cc["@songbirdnest.com/Songbird/Device/sbdevcapvideostream;1"]
                      .createInstance(Ci.sbIDevCapVideoStream);
    var videoSizes = [{width: x.width, height: x.height,
                       QueryInterface: XPCOMUtils.generateQI([Ci.sbIImageSize])}
                      for each (x in testcase.caps.video.explicitSizes)];
    videoCaps.initialize(testcase.caps.video.type,
                         ArrayConverter.nsIArray(videoSizes),
                         sbDevCapRange(testcase.caps.video.widths),
                         sbDevCapRange(testcase.caps.video.heights),
                         fromFractionRange(testcase.caps.video.PARs),
                         !(testcase.caps.video.PARs instanceof Array),
                         fromFractionRange(testcase.caps.video.frameRates),
                         !(testcase.caps.video.frameRates instanceof Array),
                         sbDevCapRange(testcase.caps.video.bitRates));

    var audioCaps = Cc["@songbirdnest.com/Songbird/Device/sbdevcapaudiostream;1"]
                      .createInstance(Ci.sbIDevCapAudioStream);
    audioCaps.initialize(testcase.caps.audio.type,
                         sbDevCapRange(testcase.caps.audio.bitRates),
                         sbDevCapRange(testcase.caps.audio.sampleRates),
                         sbDevCapRange(testcase.caps.audio.channels));

    var formatType = Cc["@songbirdnest.com/Songbird/Device/sbvideoformattype;1"]
                       .createInstance(Ci.sbIVideoFormatType);
    formatType.initialize(testcase.caps.containerType,
                          videoCaps,
                          audioCaps);

    var caps = Cc["@songbirdnest.com/Songbird/Device/DeviceCapabilities;1"]
                 .createInstance(Ci.sbIDeviceCapabilities);
    caps.init();
    caps.setFunctionTypes([caps.FUNCTION_VIDEO_PLAYBACK], 1);
    caps.addContentTypes(caps.FUNCTION_VIDEO_PLAYBACK, [caps.CONTENT_VIDEO], 1);
    caps.addMimeTypes(caps.CONTENT_VIDEO, [testcase.caps.containerType], 1);
    caps.AddFormatType(caps.CONTENT_VIDEO,
                       testcase.caps.containerType,
                       formatType);
    caps.configureDone();

    // Test the configurator by setting a device on it
    configurator.device = #1= {
      capabilities: caps,
      getPreference: function device_getPreference(aPrefName) {
        switch (aPrefName) {
          case "transcode.quality.video":
            return testcase.hasOwnProperty("quality") ? testcase.quality : 0.5;
        }
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
      },
      wrappedJSObject: #1#
    };
    configurator.determineOutputType();

    //////////
    // actually run the test
    //////////

    // configurate
    configurator.configurate();
    
    // remove things from the device, to get rid of the closure that will end up
    // leaking this whole test.  However, we can't just ask the configurator to
    // null out the device, because that's not supported.
    for (let prop in configurator.device.wrappedJSObject) {
      if (prop != "wrappedJSObject") {
        delete configurator.device.wrappedJSObject[prop];
      }
    }
    delete configurator.device.wrappedJSObject.wrappedJSObject;

    // check the muxer
    if (testcase.output.muxer != null) {
      assertTrue(configurator.useMuxer,
                 "expected configurator to use a muxer");
      assertEqual(testcase.output.muxer,
                  configurator.muxer,
                  "expected muxer is not configured muxer");
    }
    else {
      assertFalse(configurator.useMuxer,
                  "expected configurator to not use a muxer");
    }
    
    // check the file extension
    assertEqual(testcase.output.fileExtension,
                configurator.fileExtension,
                "expected file extension is not configured file extension");
    
    // check the video encoder
    if (testcase.output.videoEncoder != null) {
      assertEqual(testcase.output.videoEncoder,
                  configurator.videoEncoder,
                  "expected video encoder is not configured video encoder");
      assertTrue(configurator.useVideoEncoder,
                 "expected configurator to use a video encoder");

      let PARnum = {}, PARdenom = {}, FPSnum = {}, FPSdenom = {};
      configurator.videoFormat.getVideoPAR(PARnum, PARdenom);
      configurator.videoFormat.getVideoFrameRate(FPSnum, FPSdenom);

      // dump out the configurated video format for easier debugging
      let props = [];
      let propEnum = configurator.videoEncoderProperties.enumerator;
      for each (let prop in ArrayConverter.JSEnum(propEnum)) {
        if (prop instanceof Ci.nsIProperty) {
          props.push(prop.name + ": " + prop.value);
        }
      }
      log("Configurated video format: " +
          configurator.videoFormat.videoWidth +
          "x" + configurator.videoFormat.videoHeight +
          " PAR " + PARnum.value + ":" + PARdenom.value +
          " FPS " + FPSnum.value + ":" + FPSdenom.value +
          " props: {" + props.join(", ") + "}");

      // check the video format
      assertEqual(testcase.output.videoFormat.width,
                  configurator.videoFormat.videoWidth,
                  "video format width unexpected");
      assertEqual(testcase.output.videoFormat.height,
                  configurator.videoFormat.videoHeight,
                  "video format height unexpected");
      assertEqual(testcase.output.videoFormat.PAR.numerator,
                  PARnum.value,
                  "video format PAR numerator unexpected");
      assertEqual(testcase.output.videoFormat.PAR.denominator,
                  PARdenom.value,
                  "video format PAR denominator unexpected");
      assertEqual(testcase.output.videoFormat.frameRate.numerator,
                  FPSnum.value,
                  "video format frame rate numerator unexpected");
      assertEqual(testcase.output.videoFormat.frameRate.denominator,
                  FPSdenom.value,
                  "video format frame rate denominator unexpected");

      assertTrue(configurator.videoEncoderProperties instanceof Ci.nsIPropertyBag2);
      for (let prop in testcase.output.videoFormat.properties) {
        if (testcase.output.videoFormat.properties[prop] instanceof Function) {
          let f = testcase.output.videoFormat.properties[prop];
          let val = configurator.videoEncoderProperties.get(prop);
          assertTrue(f.call(testcase, val));
        }
        else {
          assertEqual(testcase.output.videoFormat.properties[prop],
                      configurator.videoEncoderProperties.get(prop),
                      "video property " + prop + " mismatch");
        }
      }
    }
    else {
      assertFalse(configurator.useVideoEncoder,
                  "expected configurator to not use a video encoder");
    }

    // check the audio encoder
    if (testcase.output.audioEncoder != null) {
      assertEqual(testcase.output.audioEncoder,
                  configurator.audioEncoder,
                  "expected audio encoder is not configured audio encoder");
      assertTrue(configurator.useAudioEncoder,
                 "expected configurator to use an audio encoder");

      // dump out the configurated audio format for easier debugging
      let props = [];
      let propEnum = configurator.audioEncoderProperties.enumerator;
      for each (let prop in ArrayConverter.JSEnum(propEnum)) {
        if (prop instanceof Ci.nsIProperty) {
          props.push(prop.name + ": " + prop.value);
        }
      }
      log("Configurated audio format: " +
          configurator.audioFormat.sampleRate + "Hz " +
          configurator.audioFormat.channels + "ch" +
          " props: {" + props.join(", ") + "}");

      // check the audio format
      assertEqual(testcase.output.audioFormat.sampleRate,
                  configurator.audioFormat.sampleRate,
                  "audio format sample rate unexpected");
      assertEqual(testcase.output.audioFormat.channels,
                  configurator.audioFormat.channels,
                  "audio format channels unexpected");

      assertTrue(configurator.audioEncoderProperties instanceof Ci.nsIPropertyBag2);
      for (let prop in testcase.output.audioFormat.properties) {
        assertEqual(testcase.output.audioFormat.properties[prop],
                    configurator.audioEncoderProperties.get(prop),
                    "video property " + prop + " mismatch");
      }
    }
    else {
      assertFalse(configurator.useAudioEncoder,
                  "expected configurator to not use an audio encoder");
    }
  }

  return;
}
