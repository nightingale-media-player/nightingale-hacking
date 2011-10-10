/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

// Timeout a job after 30s
const JOB_TIMEOUT = 30;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");

function sbMLJob() {
  SBJobUtils.JobBase.call(this);

  // where we'll be storing our results
  this._mlResults = [];

  // our timeout counter
  this._mlJobTimeoutTimer = Cc["@mozilla.org/timer;1"]
                              .createInstance(Ci.nsITimer);
}

sbMLJob.prototype = {
  __proto__: SBJobUtils.JobBase.prototype,

  classDescription : 'Nightingale Metadata Lookup Job Implementation',
  classID          : Components.ID("e4fd9496-1dd1-11b2-93ca-d33e8cc507ba"),
  contractID       : "@getnightingale.com/Nightingale/MetadataLookup/job;1",
  QueryInterface   : XPCOMUtils.generateQI(
      [Ci.sbIMetadataLookupJob, Ci.sbIJobProgress, Ci.sbIJobCancelable, 
       Ci.sbIJobProgressListener, Ci.nsIClassInfo, Ci.nsITimerCallback]),

  _mlNumResults        : 0,
  _mlJobType           : Ci.sbIMetadataLookupJob.JOB_DISC_LOOKUP,
  _mlJobTimedOut       : false,

  /** sbIMetadataLookupJob **/
  get mlJobType() {
    return this._mlJobType;
  },

  get mlNumResults() {
    return this._mlNumResults;
  },

  init: function(jobType, status) {
    this._mlJobType = jobType;
    this._status = status;

    // start our timer
    this._mlJobTimeoutTimer.initWithCallback(this, JOB_TIMEOUT*1000,
                                             Ci.nsITimer.TYPE_ONE_SHOT);
  },

  appendResult: function(album) {
    this._mlResults.push(album);
    this._mlNumResults++;
  },

  changeStatus: function(status) {
    if (this._mlJobTimedOut) {
      Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService)
         .logStringMessage("Attempted to change status on an " +
                           "already timed out lookup job.");
      return;
    }
    this._status = status;
    this.notifyJobProgressListeners();
    if (this._mlJobTimeoutTimer)
      this._mlJobTimeoutTimer.cancel();
  },

  getMetadataResults: function() {
    return ArrayConverter.enumerator(this._mlResults);
  },

  /** nsITimerCallback **/
  notify: function(timer) {
    // only timeout if we're still running
    if (this.status == Ci.sbIMetadataLookupJob.STATUS_RUNNING) {
      // set our status to indicate we've failed
      this.changeStatus(Ci.sbIMetadataLookupJob.STATUS_FAILED);
      this._mlJobTimedOut = true;
    }
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMLJob]);
}
