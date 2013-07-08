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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var errorDialog = null;

// Array to put errors into while the dialog is being opened
var errorQueue = null;

/******************************************************************************
 * http://bugzilla.songbirdnest.com/show_bug.cgi?id=17812
 * Responsible for passing mediacore errors to the error dialog and making sure
 * only one instance of the dialog is open at any time.
 *****************************************************************************/
function ErrorHandler() {  
}

ErrorHandler.prototype = {
  classDescription: "Songbird Mediacore error handler",
  classID:          Components.ID("{8bb6de60-a11b-11de-8a39-0800200c9a66}"),
  contractID:       "@songbirdnest.com/Songbird/MediacoreErrorHandler;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIMediacoreErrorHandler]),

  /**
   * \brief Called to process a mediacore error, usually will display a
   *        message to the user.
   * \param aError Error to be communicated to the user.
   */
  processError: function ErrorHandler_processError(aError) {
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    if (Application.prefs.getValue("songbird.mediacore.error.dontshowme",
                                   false)) {
      return;
    }

    if (errorDialog && !errorDialog.closed) {
      if (errorQueue)
        errorQueue.push(aError);
      else
        errorDialog.addError(aError);
    }
    else {
      var windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                            .getService(Ci.nsIWindowWatcher);
      errorDialog = windowWatcher.openWindow(windowWatcher.activeWindow,
        "chrome://songbird/content/xul/mediacore/mediacoreErrorDialog.xul",
        null,
        "centerscreen,chrome,resizable,titlebar",
        aError);

      // Queue up further errors until the dialog finishes loading
      errorQueue = [];
      errorDialog.addEventListener("load", function() {
        for each (let error in errorQueue)
          errorDialog.addError(error);
        errorQueue = null;
      }, false);
    }
  }
}

var NSGetModule = XPCOMUtils.generateNSGetFactory([ErrorHandler])
