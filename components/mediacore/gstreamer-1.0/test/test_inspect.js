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

function runTest() {

  var gst = Cc["@songbirdnest.com/Songbird/Mediacore/GStreamer/Service;1"]
              .getService(Ci.sbIGStreamerService);

  var count = 0;

  var handler = {
    beginInspect: function() {
      log("beginInspect");
    },
    endInspect: function() {
      log("endInspect");
    },
    beginPluginInfo: function(aName, aDescription, aFilename, aVersion, aLicense, aSource, aPackage, aOrigin) {
      log(aName);
      count++;
    },
    endPluginInfo: function() {
    }
  };

  gst.inspect(handler);

  log(count + " plugins found");
}
