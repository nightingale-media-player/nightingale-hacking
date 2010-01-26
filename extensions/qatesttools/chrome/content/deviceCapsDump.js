/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;


var DialogController =
{
  _output: "",
  
  onDialogLoad: function()
  {
    this._clearDeviceMenupopup();

    // Fill out the devices menupopup.
    var menupopup = document.getElementById("devices-menupopup");
    var deviceRegistrar = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceRegistrar);
    var devicesArray = deviceRegistrar.devices;
    for (var i = 0; i < devicesArray.length; i++) {
      var curDevice = devicesArray.queryElementAt(i, Ci.sbIDevice);

      var deviceNode = document.createElement("menuitem");
      deviceNode.setAttribute("label", curDevice.name);
      deviceNode.setAttribute("device_id", curDevice.id);

      menupopup.appendChild(deviceNode);
    }

    var self = this;
    this._commandHandler = function() {
      self._onDeviceSelected();
    };
    menupopup.addEventListener("command", self._commandHandler, false);
  },

  _onDeviceSelected: function()
  {
    var selectedItem =
      document.getElementById("devices-menulist").selectedItem;

    // Same device selected, continue on.
    var deviceID = selectedItem.getAttribute("device_id");
    if (deviceID == this._curSelectedDeviceId) {
      return;
    }

    // Lookup the device in the device registrar.
    var deviceRegistrar = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceRegistrar);
    var device = deviceRegistrar.getDevice(Components.ID(deviceID));
    if (!device) {
      // Device no longer exists.
      return;
    }

    this._output = "";
    this._clearDescField();

    var caps = device.capabilities;
    var retFormats = caps.getSupportedFunctionTypes({});

    var formatIndex;
    for (formatIndex in retFormats) {
      var contentArray =
        caps.getSupportedContentTypes(retFormats[formatIndex], {});

      for (var contentCount = 0;
           contentCount < contentArray.length;
           contentCount++)
      {
        var formatArray =
          caps.getSupportedFormats(contentArray[contentCount], {});

        for (var formatIndex = 0;
             formatIndex < formatArray.length;
             formatIndex++)
        {
          try {
            var format = caps.getFormatType(contentArray[contentCount],
                                            formatArray[formatIndex]);
            
            // Pass on logging depending on what type the format is.
            if (format instanceof Ci.sbIAudioFormatType) {
              format.QueryInterface(Ci.sbIAudioFormatType);
              this._logAudioFormatType(format, formatArray[formatIndex]);
            }
            else if (format instanceof Ci.sbIVideoFormatType) {
              format.QueryInterface(Ci.sbIVideoFormatType);
              this._logVideoFormatType(format, formatArray[formatIndex]);
            }
            else if (format instanceof Ci.sbIImageFormatType) {
              format.QueryInterface(Ci.sbIImageFormatType);
              this._logImageFormatType(format, formatArray[formatIndex]);
            }
          }
          catch (e) {
            Cu.reportError("Hrm... couldn't get the format type?");
          }
        }
      }
    }

    document.getElementById("devicecaps-desc").appendChild(
      document.createTextNode(this._output));
  },

  _logAudioFormatType: function(aAudioFormat, aFormat)
  {
    var codec = aAudioFormat.containerFormat;
    var containerFormat = aAudioFormat.containerFormat;
    var bitrates = aAudioFormat.supportedBitrates;
    var sampleRates = aAudioFormat.supportedSampleRates;
    var channels = aAudioFormat.supportedChannels;
    
    this._output +=
      " *** AUDIO FORMAT TYPE '" + aFormat + "' ***\n" +
      " * codec: " + codec + "\n" +
      " * container format: " + containerFormat + "\n" +
      " * bitrates: \n" + this._logRange(bitrates, true) +
      " * samplerates: \n" + this._logRange(sampleRates, false) +
      " * channels: \n" + this._logRange(channels, true) + "\n";
  },

  _logVideoFormatType: function(aVideoFormat, aFormat)
  {
    var containerType = aVideoFormat.containerType;
    
    var videoStreamType = aVideoFormat.videoStream.type;
    var videoStreamSizes = aVideoFormat.videoStream.supportedExplicitSizes;
    var videoStreamWidths = aVideoFormat.videoStream.supportedWidths;
    var videoStreamHeights = aVideoFormat.videoStream.supportedHeights;
    var videoStreamBitrates = aVideoFormat.videoStream.supportedBitRates;
    var videoStreamPars = aVideoFormat.videoStream.getSupportedVideoPARs({});
    var videoStreamFrameRates =
      aVideoFormat.videoStream.getSupportedFrameRates({});

    var audioStreamType = aVideoFormat.audioStream.type;
    var audioStreamBitrates = aVideoFormat.audioStream.supportedBitRates;
    var audioStreamSampleRates = aVideoFormat.audioStream.supportedSampleRates;
    var audioStreamChannels = aVideoFormat.audioStream.supportedChannels;

    this._output +=
      " *** VIDEO FORMAT TYPE '" + aFormat + "' ***\n" +
      " * video format container type: " + containerType + "\n" +
      " * videostream type: " + videoStreamType + "\n" +
      " * videostream supported widths: \n" +
          this._logRange(videoStreamWidths, true) +
      " * videostream supported heights: \n" +
          this._logRange(videoStreamHeights, true) +
      " * videostream bitrates: \n" +
          this._logRange(videoStreamBitrates, true) +
      " * videostream PARs: \n" + this._logArray(videoStreamPars) + 
      " * videostream frame rates: \n" + this._logArray(videoStreamFrameRates) + 
      " * audiostream type: " + audioStreamType + "\n" +
      " * audiostream bitrates: \n" +
          this._logRange(audioStreamBitrates, true) +
      " * audiostream samplerates: \n" +
          this._logRange(audioStreamSampleRates, false) +
      " * audiostream channels: \n" +
          this._logRange(audioStreamChannels, true) + "\n";
  },

  _logImageFormatType: function(aImageFormat, aFormat)
  {
    var imgFormat = aImageFormat.imageFormat;
    var widths = aImageFormat.supportedWidths;
    var heights = aImageFormat.supportedHeights;
    var sizes = aImageFormat.supportedExplicitSizes;

    this._output +=
      " *** IMAGE FORMAT TYPE '" + aFormat + "' ***\n" +
      " * image format: " + imgFormat + "\n" +
      " * image widths: \n" + this._logRange(widths, true) +
      " * image heights: \n" + this._logRange(heights, true) +
      " * image supported sizes: \n" + this._logSizesArray(sizes) + "\n";
  },

  _logRange: function(aRange, aIsMinMax)
  {
    var output = "";
    
    if (aRange) {
      if (aIsMinMax) {
        output += "   - min: " + aRange.min + "\n" +
                  "   - max: " + aRange.max + "\n" +
                  "   - step: " + aRange.step + "\n";
      }
      else {
        for (var i = 0; i < aRange.valueCount; i++) {
          output += "   - " + aRange.GetValue(i) + "\n";
        }
      }
    }

    return output;
  },

  _logSizesArray: function(aSizesArray)
  {
    var output = "";
    
    if (aSizesArray) {
      for (var i = 0; i < aSizesArray.length; i++) {
        var imageSize = aSizesArray.queryElementAt(i, Ci.sbIImageSize);
        output += "   - " + imageSize.width +
          " x " + imageSize.height + "\n";
      }
    }

    return output;
  },

  _logArray: function(aArray)
  {
    var output = "";

    if (aArray) {
      for (var i = 0; i < aArray.length; i++) {
        output += "   - " + aArray[i] + "\n";
      }
    }

    return output;
  },

  _clearDeviceMenupopup: function() 
  {
    var menupopup = document.getElementById("devices-menupopup");
    while (menupopup.firstChild) {
      menupopup.removeChild(menupopup.firstChild);
    }
  },

  _clearDescField: function()
  {
    var desc = document.getElementById("devicecaps-desc");
    while (desc.firstChild) {
      desc.removeChild(desc.firstChild);
    }
  },
};

