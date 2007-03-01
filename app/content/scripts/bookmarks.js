/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
  addBookmark : function() {
    var browser = document.getElementById("frame_main_pane");
    if (browser) {
      var theurl = browser.currentURI.spec;
      
      if (!this.svc.bookmarkExists(theurl)) {
        var thelabel = browser.contentDocument.title;
        if (thelabel == "") thelabel = theurl;
        /* FIXME: get the favicon from the link rel in the document */
        var theicon = "http://" + browser.currentURI.hostPort + "/favicon.ico";
        this.svc.addBookmark(theurl, thelabel, theicon);
      } else {
        // tell user it already exists
        gPrompt.alert( window, 
                      SBString( "bookmarks.addmsg.title", "Bookmark" ),
                      SBString( "bookmarks.addmsg.msg", "This bookmark already exists" ) );
      }
    }
  }
};
