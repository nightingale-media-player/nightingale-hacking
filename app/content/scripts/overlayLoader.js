// JScript source code
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
 * Manages runtime overlay loading.
 * NOTE: This is a temporary hack. Fix overlays to work with XBL ASAP.
 * TODO: Document side effects.
 */
var OverlayLoader = {

  // Flag to indicate that loadOverlayList has been called
  // and has not yet observed the last overlay finish loading 
  loadInProgress: false,
  
  // Flag to indicate that the whole overlay loading is already done
  loadCompleted: false,

  /**
   * Initiates dynamic loading of all overlays appropriate to
   * this window.  This includes overlays targeted at
   * windowtype:* and windowtype:[current windowtype attribute]
   *
   * Note: This function is called on window load. See bottom
   * of this file.
   */
  loadRuntimeOverlays: function loadRuntimeOverlays() {
    // Get overlays that are intended for all windows
    var overlays = this.getOverlaysForTarget("windowtype:*"); 
     
    // If this window has a windowtype attribute, then also get all overlays 
    // specifically targeting this window type
    var windowType = document.documentElement.getAttribute("windowtype");
    if (windowType != null && windowType != "") {
      overlays = overlays.concat(this.getOverlaysForTarget("windowtype:" + windowType)); 
    }
    
    // If there are overlays to load, get started
    if (overlays.length > 0) {
      this.loadOverlayList(overlays);
    } else {
      // Or send the event because there was nothing to do
      this.sendOverlayEvent();
    }
  },


  /**
   * Send the event to let everyone know we're done
   */
  sendOverlayEvent: function sendOverlayEvent() {
    var e = document.createEvent("Events");
    e.initEvent("sb-overlay-load", false, true);
    document.dispatchEvent(e);
    this.loadCompleted = true;
  },

  /**
   * Sequentially loads the given list of string URLs as overlays
   */
  loadOverlayList: function loadOverlayList(overlays) {

    dump("\n\nOverlayLoader.loadOverlayList()\n\t+");
    dump(overlays.join("\n\t+") + "\n\n");
    
    // Check simultaneous load flag
    if (this.loadInProgress) {
      dump("\n\nOverlayLoader.loadOverlayList() already in progress!");
      return;
    }
    this.loadInProgress = true;

    // Disable the normal loadOverlay function while we are loading 
    // in order to avoid BMO 330458
    document._loadOverlayInUse = document.loadOverlay;
    document.loadOverlay = function() { 
      var caller = "unknown";
      try { 
        caller = document.loadOverlay.caller.name;
      } catch (e) {};
      dump("\n\n\n\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      dump("WARNING:  Someone is attempting to use document.loadOverlay while\n");
      dump("          loadPlayerOverlays() is in progress.  This cannot be allowed\n");
      dump("          as simultaneous loadOverlay calls will clobber each other.\n");
      dump("          For more information see Mozilla Bug 330458.\n\n");
      dump("          Possible culprit: " + caller +  "()\n");
      dump("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n\n\n\n");
    }

   
    // Create an observer that will notice xul-overlay-merged events
    // and load the next overlay in the list
    var observer = {
      observe: function () 
      {
        // If there are overlays in our list, load the next one
        if (overlays.length > 0) {
          var overlay = overlays.pop();
          dump("\nOverlayLoader.loadPlayerOverlays() loading overlay " + overlay + "\n");
          // Pass ourselves in as the observer, so that we will know
          // when it is safe to load the next overlay
          document._loadOverlayInUse(overlay, observer);
        } 
        // Otherwise, we're finished, so restore the normal loadOverlay
        else {
          dump("\nOverlayLoader.loadPlayerOverlays() finished loading overlays\n");
          document.loadOverlay = document._loadOverlayInUse;
          OverlayLoader.loadInProgress = false;
          
          // Send out an event to let people know it's safe to go modal.
          setTimeout( OverlayLoader.sendOverlayEvent, 0 );
        }
      }
    }
    
    // If we haven't finished loading in 3 seconds, something has
    // then gone terribly wrong.  Set a timeout that will warn
    // the user if the load does not happen.  *sigh*
    var checkCompleted = function() {
      if (OverlayLoader.loadInProgress) {
        alert("An extension overlay has failed to load.\n\n" +
              "Known causes include: \n" +
              "  - A malformed overlay file\n" +
              "  - An overlay which is itself the target of an overlay\n\n" +
              "Open the Add-Ons window and disable extensions one by one until\n" +
              "you have isolated which extension is causing the problem.\n"+
              "Please report this error to the extension author, or\n" +
              "http://bugzilla.songbirdnest.com.");

        // Then send the event that we're done              
        OverlayLoader.sendOverlayEvent();
      }
    }
    setTimeout(checkCompleted, 3000);
    
    // Start loading the overlays
    observer.observe();
  },
  
  
  /**
   * Given the string URL for a XUL document, returns all
   * overlays targetting that document as an array of string URLs
   */
  getOverlaysForTarget: function getOverlaysForTarget(targetURL) {
   
    var targetURI = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService)
                              .newURI(targetURL, null, null);
     
    var uriList = [];

    // Helper to add string URIs to our array using a given
    // nsISimpleEnumerator of nsIURIs
    var addURIs = function(enumerator) {
      while (enumerator.hasMoreElements()) {
        uriList.push( enumerator.getNext()
                                .QueryInterface(Components.interfaces.nsIURI)
                                .spec );
      }
    }

    // Ask the chrome registry for all overlays that should be applied to this identifier
    var chromeRegistry = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                                    .getService(Components.interfaces.nsIXULOverlayProvider);
    addURIs(chromeRegistry.getXULOverlays(targetURI));
    addURIs(chromeRegistry.getStyleOverlays(targetURI));

    return uriList;
  }
} // End of OverlayLoader



// Start pulling in the dynamic overlays as soon as the window loads
window.addEventListener("load", function() { OverlayLoader.loadRuntimeOverlays();}, false);


