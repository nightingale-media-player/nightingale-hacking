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

/**
 * \file  DebugUtils.jsm
 * \brief Javascript module providing some debug utility functions.
 */

EXPORTED_SYMBOLS = ["DebugUtils"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

var DebugUtils = {
  /**
   * \brief Returns the value of NSPR_LOG_MODULES environment variable.
   */
  get logModules() {
    let environment = Cc["@mozilla.org/process/environment;1"]
                        .createInstance(Ci.nsIEnvironment);
    let result = environment.get("NSPR_LOG_MODULES");

    // Memoize the result
    this.__defineGetter__("logModules", function() result);
    return this.logModules;
  },

  /**
   * \brief Generates a log function for a module.
   *
   * Call with your module name. This will only return a logging function if
   * your module is listed in the NSPR_LOG_MODULES environment variable with
   * a log level of at least 3. Otherwise it will return a no-op function.
   * Note that log output will still go to stderr, not to NSPR_LOG_FILE as one
   * might expect.
   */
  generateLogFunction: function DebugUtils_generateLogFunction(module) {
    let doLog = false;
    for each (let entry in this.logModules.split(/,/)) {
      if (/(.*):(\d+)$/.test(entry) &&
          (RegExp.$1 == module || RegExp.$1 == "all") &&
          parseInt(RegExp.$2) >= 3) {
        doLog = true;
      }
    }

    if (doLog) {
      return function LOG(msg) {
        dump(module + ": " + msg + "\n");
      };
    }
    else {
      return function LOG(msg) {};
    }
  }
};
