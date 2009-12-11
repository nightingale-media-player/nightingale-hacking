/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
  _errorContainer: null,
  _errorList: null,
  _cancelButton: null,


  /**
   * Called when the dialog loads
   */
  onLoad: function JobProgressDialog_onLoad() {
    // The first window argument can either be a |nsIDialogParamBlock| or a 
    // |nsIArray| object (i.e. from |WindowUtils.openDialog()|).
    var argArray = null;
    if (window.arguments[0] instanceof Ci.nsIDialogParamBlock) {
      argArray = window.arguments[0].objects;
    }
    else if (window.arguments[0] instanceof Ci.nsIArray) {
      argArray = window.arguments[0];
    }
    else {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    this._job = argArray.queryElementAt(0, Ci.sbIJobProgress);
    
    if (this._job.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      window.close();
      return;
    }

    this._job.addJobProgressListener(this);

    this._description = document.getElementById("jobprogress_status_desc");
    this._progressMeter = document.getElementById("jobprogress_progressmeter");
    this._errorContainer = document.getElementById("jobprogress_error_box");
    this._errorList = document.getElementById("jobprogress_error_list");
    
    // Show cancel if allowed
    if (this._job instanceof Ci.sbIJobCancelable) {
      document.documentElement.buttons = "cancel";
      this._cancelButton = document.documentElement.getButton("cancel");
    }
    
    // Initialize the UI
    this.onJobProgress();
  },  

  /**
   * Called when the dialog is closed
   */
  onUnLoad: function JobProgressDialog_onUnLoad() {
    try { 
      this._job.removeJobProgressListener(this);
    } catch (e) {  
      //  Failing to remove is fine, as the job may have cleared listeners 
    }
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
    this._updateProgressUI();
    
    if (this._job.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      window.close();
    } else if (this._job.status == Ci.sbIJobProgress.STATUS_FAILED) {
      
      // If there is a main error summary, show the error messages
      if (this._job.statusText) {
        document.documentElement.buttons = "accept";
        this._showErrors(); 
      } else {
        // Otherwise just close
        window.close();
      }
    } else if (this._job.blocked) {
      // Close window to prevent blocking the UI.
      //XXXeps should only do this if dialog is modal.
      window.close();
    }
  },
  
  /**
   * Update the UI elements with the state of the job
   */
  _updateProgressUI: function JobProgressDialog__updateUI() {
    this._formatDescription(this._description, this._job);
    this._setTitle(this._job.titleText);
    
    if (this._cancelButton) {
      var cancelable = this._job.canCancel;
      if (this._cancelButton.disabled == cancelable) {
        this._cancelButton.disabled = !cancelable;
      }
    }
    
    if (this._job.total <= 0) {
      this._progressMeter.mode = "undetermined";
    } else {
      this._progressMeter.mode = "determined";
      
      // Songbird has a special modified progress meter.
      // We allow max > 100, but there  are some crazy bugs 
      // if max goes too high.
      if (this._job.total < 10000) {
        this._progressMeter.max = this._job.total;
        this._progressMeter.value = this._job.progress;
      } else {
        this._progressMeter.max = 10000;
        this._progressMeter.value = Math.round((this._job.progress / this._job.total) * 10000);
      }
    }
    
    // If the content is too big for the window (or the window has just loaded)
    // then resize the window.   
    // This madness is needed because calling sizeToContent twice with no
    // change in dimensions results 1px of bonus width. 
    // See https://bugzilla.mozilla.org/show_bug.cgi?id=230959
    if (window.innerHeight < 10 ||
        window.innerHeight != document.documentElement.boxObject.height) {
      setTimeout(function() { window.sizeToContent(); }, 50);
    }
  },
  
  /**
   * Hide the progress bar and display a list of error messages
   */
  _showErrors: function () {
    this._progressMeter.hidden = true;
    this._errorContainer.hidden = false;
    
    var messages = this._job.getErrorMessages();
    var message;
    while (messages.hasMore()) {
      message = unescape(messages.getNext());
      var item = this._errorList.appendItem(message);
      item.setAttribute("crop", "center");
      item.setAttribute("tooltiptext", message);
    }
    
    if (this._errorList.hasAttribute("maxrows")) {
      rows = parseInt(this._errorList.getAttribute("maxrows"));
    }
    else {
      rows = 4;
    }
    this._errorList.setAttribute("rows", Math.min(rows, this._job.errorCount));
  },
  
  /**
   * Workaround to set the title of a super special Songbird dialog
   */
  _setTitle: function JobProgressDialog__setTitle(aTitle) {
    if (document.title == aTitle) {
      return;
    }

    document.title = aTitle;

    // If this dialog is going to be shown on Mac (i.e. sheets) and the current
    // skin is running showChrome=true, use the inline progress title.
    var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                    .getService(Components.interfaces.nsIPropertyBag2);
    var platform = sysInfo.getProperty("name");
    var isPlucked = Application.prefs.getValue("songbird.accessibility.enabled", false); 
    var titleBox = document.getElementById("jobprogress_title_box");
    if (platform == "Darwin" && isPlucked && window.opener) {
      var macSheetTitle = document.getElementById("jobprogress_title_desc");
      if (macSheetTitle) {
        macSheetTitle.value = aTitle;
      }
    }
    else {
      titleBox.hidden = true;
      var windowTitle = document.getElementById('dialog-titlebar');
      if (windowTitle) {
        windowTitle.title = aTitle;
      }
    }
  },
  
  /**
   * Localized strings cannot have line breaks. As a result, we have to 
   * jump through hoops and convert \n to <br/>.  Bleh.
   */
  _formatDescription: function(aDescription, aJob) {
    var crop = false;
    if (aJob instanceof Components.interfaces.sbIJobProgressUI) {
      if (aJob.crop) {
        aDescription.setAttribute("crop", aJob.crop);
        crop = true;
      } else {
        aDescription.removeAttribute("crop");
      }
    }
    var lines = aJob.statusText.split("\n");
    if (lines.length > 1) {
      aDescription.textContent = "";
      for (var i = 0; i < lines.length; i++) {
        aDescription.appendChild(document.createTextNode(lines[i]));
        aDescription.appendChild(document.createElementNS("http://www.w3.org/1999/xhtml","html:br"));
      }
    } else {
      if (crop) {
        aDescription.value = lines[0];
      } else {
        aDescription.textContent = lines[0];
      }
    }
  }
}

