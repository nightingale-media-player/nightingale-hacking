/* vim: set sw=2 : */
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

function runTest() {

  Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

  var conf = Cc["@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/Device/GStreamer;1"]
               .createInstance(Ci.sbPIGstTranscodingConfigurator);
  var profiles = ArrayConverter.JSArray(conf.availableProfiles);
  var testProfile = null;
  for each (var profile in profiles) {
    profile.QueryInterface(Ci.sbITranscodeProfile);
    log(profile.id + " = " + profile.description);
    if (profile.id == "072b5104-1dd2-11b2-9313-e80ca3194e4e") {
      // this is the ogg/theora/vorbis test profile
      testProfile = profile;
    }
  }

  profile = testProfile.QueryInterface(Ci.sbITranscodeEncoderProfile);
  
  const K_PRIORITY_MAP = {"-1":   100,
                          "0":    100,
                          "0.25": 550,
                          "0.5":  1000,
                          "0.75": 1250,
                          "1":    1500,
                          "2":    1500};
  const K_BITRATE_MAP = {"-1":   32000,
                         "0":    32000,
                         "0.25": 80000,
                         "0.5":  128000,
                         "0.75": 224000,
                         "1":    320000,
                         "2":    320000};
  const K_BPP_MAP = {"-1":   0.2,
                     "0":    0.2,
                     "0.25": 0.4,
                     "0.5":  0.6,
                     "0.75": 0.8,
                     "1":    1.0,
                     "2":    1.0};

  for (var i in K_PRIORITY_MAP) {
    assertEqual(K_PRIORITY_MAP[i], profile.getPriority(i));
  }
  for (var i in K_BITRATE_MAP) {
    assertEqual(K_BITRATE_MAP[i], profile.getAudioBitrate(i));
  }
  for (var i in K_BPP_MAP) {
    assertEqual(K_BPP_MAP[i].toFixed(5),
                profile.getVideoBitsPerPixel(i).toFixed(5));
  }
}
