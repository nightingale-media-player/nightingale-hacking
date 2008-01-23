/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/

EXPORTED_SYMBOLS = ["TimeFormatter"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

var TimeFormatter = {
  _cfg: {
    localeBundlePath: "chrome://songbird/locale/songbird.properties"
  },


  /*
   * formatSingle
   *
   *   ==> aTime                Time in seconds to format.
   *
   *   <==                      Formatted time string with single unit.
   *
   *   This function returns a formatted version of the time value specified in
   * seconds by aTime.  The formatted string contains a single unit of seconds,
   * minutes, or hours.
   *
   * Example:
   *
   *   formatSingle(32) returns "32 seconds"
   *   formatSingle(66) returns "1.1 minutes"
   */

  formatSingle: function(aTime) {
    /* Don't allow negative times. */
    if (aTime < 0) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    /* Get the format time value and unit. */
    var formatTime;
    var formatTimeUnit;
    if (aTime >= 3600) {
      formatTime = (aTime * 10) / 3600;
      formatTime = Math.floor(formatTime);
      formatTime = formatTime / 10;
      if (formatTime != 1)
        formatTimeUnit = "timeformatter.hours";
      else
        formatTimeUnit = "timeformatter.hour";
    } else if (aTime >= 60) {
      formatTime = (aTime * 10) / 60;
      formatTime = Math.floor(formatTime);
      formatTime = formatTime / 10;
      if (formatTime != 1)
        formatTimeUnit = "timeformatter.minutes";
      else
        formatTimeUnit = "timeformatter.minute";
    } else {
      formatTime = Math.floor(aTime);
      if (formatTime != 1)
        formatTimeUnit = "timeformatter.seconds";
      else
        formatTimeUnit = "timeformatter.second";
    }

    /* Produce the formatted time string. */
    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                                      .getService(Ci.nsIStringBundleService);
    var bundle = stringBundleService.createBundle(this._cfg.localeBundlePath);
    formatTime = bundle.formatStringFromName(formatTimeUnit, [ formatTime ], 1);

    return formatTime;
  }
}

