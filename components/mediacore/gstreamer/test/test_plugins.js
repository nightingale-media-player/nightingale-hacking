/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2008 POTI, Inc.
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

function runTest() {

  var gst = Cc["@songbirdnest.com/Songbird/Mediacore/GStreamer/Service;1"]
              .getService(Ci.sbIGStreamerService);

  var list = [];

  var handler = {
    beginInspect: function() {
    },
    endInspect: function() {
    },
    beginPluginInfo: function(aName, aDescription, aFilename, aVersion, aLicense, aSource, aPackage, aOrigin) {
      list.push(aName);
    },
    endPluginInfo: function() {
    },
    beginFactoryInfo: function(aLongName, aClass, aDescription, aAuthor, aRankName, aRank) {
    },
    endFactoryInfo: function() {
    },
    beginPadTemplateInfo: function(aName, aDirection, aPresence, aCodecDescription) {
    },
    endPadTemplateInfo: function() {
    }
  };

  gst.inspect(handler);

  log(list.length + " plugins found");

  var platform = getPlatform();

  assertContains(list, ["staticelements", "ogg", "vorbis", "mozilla"]);

  switch (platform) {
    case "Windows_NT":
      assertContains(list, ["directsound", "dshowsinkwrapper"]);
      break;
    case "Darwin":
      assertContains(list, ["osxaudio", "osxvideo"]);
      break;
    case "Linux":
      assertContains(list, ["alsa", "ximagesink", "xvimagesink"]);
      break;
    default:
      log("Unknown platform " + platform);
  }
}

function assertContains(a, b) {
  for (var i = 0; i < b.length; i++) {
    assertTrue(a.indexOf(b[i]) >= 0, "Plugin not found: " + b[i]);
  }
}

function assertDoesNotContain(a, b) {
  for (var i = 0; i < b.length; i++) {
    assertFalse(a.indexOf(b[i]) >= 0, "Plugin unexpectedly found: "+b[i]);
  }
}

