// vim: set sw=2 :miv
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

/**
 * SBTabProgressListener
 *
 * The tab progress implementation for <sb-tabbrowser>
 * used to set various data remotes on location change (such as whether the back
 * button can be enabled).
 * In addition, it triggers notifyTabContentChange() (for TabContentChange) and
 * session store.
 */

const EXPORTED_SYMBOLS = ["SBTabProgressListener"];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

function SBTabProgressListener(aTabBrowser ) {
  this._tabBrowser = aTabBrowser;
}

// create an object which implements as much of nsIWebProgressListener as we're using
SBTabProgressListener.prototype = {
  _tabBrowser: null,

  onLocationChange: function SBTabProgressListener_onLocationChange(aWebProgress, aRequest, aLocation) {
    try {
      // for some reason we need to do this dance to get scrollbars to
      // show up
      const nsIScrollable = Ci.nsIScrollable;
      var scrollable = this._tabBrowser
                           .selectedBrowser
                           .webNavigation
                           .QueryInterface(nsIScrollable);
      scrollable.setDefaultScrollbarPreferences(nsIScrollable.ScrollOrientation_Y,
                                                nsIScrollable.Scrollbar_Auto);
      scrollable.setDefaultScrollbarPreferences(nsIScrollable.ScrollOrientation_X,
                                               nsIScrollable.Scrollbar_Auto);

      if (this._tabBrowser.selectedBrowser.webNavigation.sessionHistory) {
        SBDataSetBoolValue('browser.cangoback', this._tabBrowser.canGoBack);
        SBDataSetBoolValue('browser.cangofwd', this._tabBrowser.canGoForward);
      }

      // Nothing in the status text
      SBDataSetStringValue( "faceplate.status.text", "");

      if (!aLocation) {
        // If we do not have a location, make a new uri that points to
        // about:blank. Do not assign a simple string, or subsequent usage
        // of .scheme and .spec will fail
        var ioService =
          Components.classes["@mozilla.org/network/io-service;1"]
                    .getService(Components.interfaces.nsIIOService);
        
        aLocation = ioService.newURI("about:blank", null, null);
      }

      SBDataSetBoolValue('browser.cansubscription',
                         this._tabBrowser.selectedTab.outerPlaylistShowing);

      var mediaTab = this._tabBrowser.mediaTab;
      SBDataSetBoolValue("browser.in_media_page", 
                         mediaTab && this._tabBrowser.selectedTab == mediaTab);

      // Let listeners know that the tab location has changed
      this._tabBrowser.notifyTabContentChange();

      // If we're in the media tab and NOT a media list view, then we're some
      // sort of arbitrary XUL page, so hide the #nav-bar
      if (this._tabBrowser.selectedTab == mediaTab &&
          !this._tabBrowser.currentMediaListView)
      {
        document.getElementById("nav-bar").setAttribute("collapsed", "true");
      }
      else
      {
        document.getElementById("nav-bar").removeAttribute("collapsed");
      }
    }
    catch ( err )
    {
      Components.utils.reportError( "SBTabProgressListener::onLocationChange\n\n" + err );
    }
  },

  onStateChange: function SBTabProgressListener_onStateChange(aWebProgress, aRequest, aState, aStatus) {
    const nsIWebProgressListener =
      Ci.nsIWebProgressListener;
    
    // if this state change isn't on the top-level window, ignore it
    if (aWebProgress.DOMWindow != aWebProgress.DOMWindow.parent ||
        !(aState & nsIWebProgressListener.STATE_IS_WINDOW)) {
      return;
    }

    if (aState & nsIWebProgressListener.STATE_START) {
      // Start the spinner if necessary
      this._tabBrowser.loading = true;
    }
    else if (aState & nsIWebProgressListener.STATE_STOP) {
      // Stop the spinner if necessary
      this._tabBrowser.loading = false;

      // Let listeners know that the tab has finished loading
      // but only if aStatus == 0, an nsresult representing whether the request 
      // finished or was cancelled...
      if (aStatus == 0) {
        this._tabBrowser.notifyTabContentChange();
      }

      // Save the tab state after every page load, just in case we crash
      if ("_sessionStore" in this._tabBrowser) {
        if (this._tabBrowser._sessionStore.tabStateRestored) {
          this._tabBrowser._sessionStore.saveTabState(this._tabBrowser);
        }
      }
    }
  },
  
  
  /* these are not called; refer to the sb-tabbrowser constructor where it's registered */
  onProgressChange: function() { /* do nothing */ },
  onStatusChange: function() { /* do nothing */ },
  onSecurityChange: function() { /* do nothing */ },
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIWebProgressListener])
};
