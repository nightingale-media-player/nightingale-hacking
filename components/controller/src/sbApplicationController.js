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
 
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const WINDOWTYPE_SONGBIRD_PLAYER      = "Songbird:Main";
const WINDOWTYPE_SONGBIRD_CORE        = "Songbird:Core";

/**
 * \class ApplicationController
 * \brief Service representing the global instance of the Songbird application.
 * \sa sbIApplicationController.idl
 */
function ApplicationController() {
}
ApplicationController.prototype = {
  QueryInterface          : XPCOMUtils.generateQI(
      [Ci.sbIApplicationController, Ci.nsIClassInfo]),
  classDescription        : 'Songbird Root Application Service Implementation',
  classID                 : Components.ID("{8492c5a0-ab8e-11dd-ad8b-0800200c9a66}"),
  contractID              : "@songbirdnest.com/Songbird/ApplicationController;1",
  flags                   : Ci.nsIClassInfo.MAIN_THREAD_ONLY,
  implementationLanguage  : Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage    : function(aLanguage) { return null; },
  getInterfaces : function(count) {
    var interfaces = [Ci.sbIApplicationController,
                      Ci.nsIClassInfo,
                      Ci.nsISupports
                     ];
    count.value = interfaces.length;
    return interfaces;
  },

  /**
   * Get the current active window, regardless of category.
   */
  get activeWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Ci.nsIWindowMediator);
    return windowMediator.getMostRecentWindow(null);
  },
  
  /**
   * Get the current active window of type Songbird:Main.
   */
  get activeMainWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Ci.nsIWindowMediator);
    return windowMediator.getMostRecentWindow(WINDOWTYPE_SONGBIRD_PLAYER);
  },

  /**
   * Play something using the UI for context
   */
  playDefault: function ApplicationController_playDefault() {
    var mm = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
               .getService(Ci.sbIMediacoreManager);
                 
    // If paused, just continue
    if (mm.status.state == Ci.sbIMediacoreStatus.STATUS_PAUSED) {
      mm.playbackControl.play();
      return;
    }
    
    var handled = false;
    
    // Try to get the front-most window to start
    // playback, based on whatever context it has
    var window = this.activeWindow;
    if (window) {
      if (this._sendEventToWindow("Play", window))
        return;
    }
        
    // If that didn't work, then try the main songbird window
    // (if different)
    var mainWindow = this.activeMainWindow;
    if (mainWindow && mainWindow != window) {
      if (this._sendEventToWindow("Play", mainWindow)) 
        return;
    }
    
    // Fallback to a stashed view, or the main library
    
    // When the tabbrowser is unloaded, the currently active view is
    // set on the sequencer, just in case it is of some use
    // to the next window to load.
    var view = mm.sequencer.view;

    // Still no view, fetch main library.
    if(!view) {
      if (typeof(LibraryUtils) == "undefined") {
        Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
      }
      view = LibraryUtils.createStandardMediaListView(LibraryUtils.mainLibrary);
    }

    var index = view.selection.currentIndex;
      
    // If same view as current view on sequencer and nothing
    // selected in the view, use sequencer view position.
    if((index == -1) && (mm.sequencer.view == view)) {
      index = mm.sequencer.viewPosition;
    }

    mm.sequencer.playView(view,  
                           Math.max(index, 
                                    Ci.sbIMediacoreSequencer.AUTO_PICK_INDEX)); 
  },
  
  /** 
   * Helper to dispatch and event to a window. Returns true if
   * the event was handled.
   */
  _sendEventToWindow: 
  function ApplicationController__sendEventToWindow(aEventName, aWindow) {
    var event = aWindow.document.createEvent("Events");
    event.initEvent(aEventName, true, true);
    return !aWindow.dispatchEvent(event);
  }
}


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([ApplicationController]);
}