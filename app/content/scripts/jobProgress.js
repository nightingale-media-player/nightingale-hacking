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

/**
 * \file jobProgress.js
 * \brief Controller for jobProgress.xul.  Responsible for displaying the state
 *        of an object implementing sbIJobProgress.
 * \sa SBJobUtils.jsm
 */
 
var JobProgressDialog = {

  // The sbIJobProgress interface to track
  _job: null,

  // UI elements
  _label: null,
  _progressMeter: null,


  /**
   * Called when the dialog loads
   */
  onLoad: function JobProgressDialog_onLoad() {

    var dialogPB = window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);
    this._job = dialogPB.objects.queryElementAt(0, Ci.sbIJobProgress);
    
    if (this._job.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      window.close();
      return;
    }

    this._job.addJobProgressListener(this);

    this._label = document.getElementById("jobprogress_status_label");
    this._progressMeter = document.getElementById("jobprogress_progressmeter");    
    this._updateUI();
    
    // Show cancel if allowed
    if (this._job instanceof Ci.sbIJobCancelable) {
      document.documentElement.buttons = "cancel";
    }    
  },

  /**
   * Called when the dialog is closed
   */
  onUnLoad: function JobProgressDialog_onUnLoad() {
    this._job.removeJobProgressListener(this);
    this._job = null;
  },
  

  /**
   * Called when the job is canceled by the user
   */
  onCancel: function JobProgressDialog_onUnLoad() {
    this._job.cancel();
    window.close();
  },
  
  /**
   * Called periodically by the monitored sbIJobProgress
   */
  onJobProgress: function JobProgressDialog_onJobProgress(aJob) {
    this._updateUI();
    
    if (this._job.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      window.close();
    } else if (this._job.status == Ci.sbIJobProgress.STATUS_FAILED) {
      // TODO
    }
  },
  
  /**
   * Update the UI elements with the state of the job
   */
  _updateUI: function JobProgressDialog__updateUI() {
    this._label.value = this._job.statusText;
    this._setTitle(this._job.titleText);
    
    if (this._job.total <= 0) {
      this._progressMeter.mode = "undetermined";
    } else {
      this._progressMeter.mode = "determined";
      var progress = Math.round((this._job.progress / this._job.total) * 100);
      this._progressMeter.value = new String(progress);
    }
  },
  
  /**
   * Workaround to set the title of a super special Songbird dialog
   */
  _setTitle: function JobProgressDialog__setTitle(aTitle) {
    document.title = aTitle;
    var windowTitle = document.getElementById('dialog-titlebar');
    if (windowTitle) {
      windowTitle.title = aTitle;
    }
  }
}

