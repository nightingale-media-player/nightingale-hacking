/* vim: set sw=2 : */
/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

  Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

  var conf = Cc["@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/Device/GStreamer;1"]
               .createInstance(Ci.sbITranscodingConfigurator);
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
  
  const K_PRIORITY_MAP = {"-1":   350,
                          "0":    350,
                          "0.25": 1925,
                          "0.5":  3500,
                          "0.75": 4000,
                          "1":    4500,
                          "2":    4500};
  const K_BITRATE_MAP = {"-1":   32000,
                         "0":    32000,
                         "0.25": 80000,
                         "0.5":  128000,
                         "0.75": 224000,
                         "1":    320000,
                         "2":    320000};
  const K_BPP_MAP = {"-1":   0.03,
                     "0":    0.03,
                     "0.25": 0.0975,
                     "0.5":  0.1650,
                     "0.75": 0.2325,
                     "1":    0.3,
                     "2":    0.3};

  for (var i in K_PRIORITY_MAP) {
    assertEqual(K_PRIORITY_MAP[i], profile.getEncoderProfilePriority(i));
  }
  for (var i in K_BITRATE_MAP) {
    assertEqual(K_BITRATE_MAP[i], profile.getAudioBitrate(i));
  }
  for (var i in K_BPP_MAP) {
    assertEqual(K_BPP_MAP[i].toFixed(5),
                profile.getVideoBitsPerPixel(i).toFixed(5));
  }
}
