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


/* this code should probably be merged into onBrowserBookmark in browser_xbl_temp */
var bmManager = {
  svc: Components.classes['@songbirdnest.com/servicepane/bookmarks;1']
                         .getService(Components.interfaces.sbIBookmarks),
  addBookmark : function bmManager_addBookmark() {
    var browser = window.gBrowser;
    if (browser) {
      var theurl = browser.currentURI.spec;
      
      // First we check if the currentURI matches what is in the url bar
      var locationBar = document.getElementById("location_bar");
      if (locationBar.value != theurl) {
        theurl = locationBar.value;
      }

      var thelabel = browser.contentDocument.title;
      if (thelabel == "") thelabel = theurl;
      
      this.addBookmarkForPage(theurl, thelabel);
    }
  },
  
  addBookmarkForPage: function bmManager_addBookmarkForPage(aLocation, aTitle) {
    if (!(aLocation instanceof Components.interfaces.nsIURI)) {
      // if we didn't get a uri, assume we got a url

      var uri = (Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService)
                           .newURI(aLocation, null, null));
      aLocation = uri;
    }
    
    var theurl = aLocation.spec;

    if (!this.svc.bookmarkExists(theurl)) {
      var theicon = "http://" + aLocation.hostPort + "/favicon.ico";
      var faviconService = Components.classes["@mozilla.org/browser/favicon-service;1"]
                           .getService(Components.interfaces.nsIFaviconService);
        
      try {
        theicon = faviconService.getFaviconForPage(aLocation).spec;
        
        // Favicon URI's are prepended with "moz-anno:favicon:".
        if(theicon.indexOf("moz-anno:favicon:") == 0) {
          theicon = theicon.substr(17);
        }
      }
      catch(e) {
        if (Components.lastResult != Components.results.NS_ERROR_NOT_AVAILABLE)
          Components.utils.reportError(e);
      }

      // XXX: The bookmark service should eventually get the favicon from the favicon service instead
      // of simply saving the URI to the favicon. :(
      this.svc.addBookmark(theurl, aTitle, theicon);
    } else {
      // tell user it already exists
      gPrompt.alert( window, 
                    SBString( "bookmarks.addmsg.title", "Bookmark" ),
                    SBString( "bookmarks.addmsg.msg", "This bookmark already exists" ) );
    }

  }
};
