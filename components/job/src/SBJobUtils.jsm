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
 
EXPORTED_SYMBOLS = [ "SBJobUtils" ];

// Amount of time to wait before launching a progress dialog
const PROGRESS_DIALOG_DELAY = 1000;

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

Cu.import("resource://app/jsmodules/WindowUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");


/**
 * A collection of functions for manipulating sbIJob* interfaces
 */
var SBJobUtils = {
 
  /**
   * \brief Display a progress dialog for an object implementing sbIJobProgress.
   *
   * If the job has not completed in a given amount of time, display
   * a modal progress dialog. 
   *
   * \param aJobProgress    sbIJobProgress interface to be displayed.
   * \param aWindow         Parent window for the dialog. Can be null.
   */
  showProgressDialog: function(aJobProgress, aWindow) {
    if (!(aJobProgress instanceof Ci.sbIJobProgress)) {
      throw new Error("showProgressDialog requires an object implementing sbIJobProgress");
    }

    function showDialog() {
      timer.cancel();
      timer = null;
      
      // If the job is already complete, skip the dialog
      if (aJobProgress.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
        return;
      }
     
      // Parent to the main window by default
      if (!aWindow || aWindow.closed) {
        var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Components.interfaces.nsIWindowMediator);
        aWindow = wm.getMostRecentWindow("Songbird:Main");
      }
      
      // Otherwise, launch the dialog  
      WindowUtils.openModalDialog(
         aWindow,
         "chrome://songbird/content/xul/jobProgress.xul",
         "job_progress_dialog",
         "",
         [ aJobProgress ],
         null,
         null);
    } 
   
    // Wait a bit before launching the dialog.  If the job is already
    // complete don't bother launching it.
    // The timer will maintain a ref to this closure during the delay, and vice versa.
    var timer = new SBTimer(showDialog, PROGRESS_DIALOG_DELAY, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  
}
