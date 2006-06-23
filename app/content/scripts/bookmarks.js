/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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


function changeBookmarkButton( ) {
  var button = getBookmarkButton();
  if (button) button.setAttribute("oncommand", "onBookmarkClicked();");
}

function changeServiceContext( ) {
  var service = getServiceObject();
  if (service) {
    //alert(service.tree);
    service.tree.setAttribute("oncontextmenu", "onBookmarkServiceContextMenu(event);");
  }
}

function getBookmarkButton() {
  return document.getElementById("browser_bookmark");
}

function getBrowserObject() {
  return document.getElementById("frame_main_pane");
}

function getServiceObject() {
  return document.getElementById("frame_servicetree");
}

var bmManager = {

  serviceSourceProxy : null, 
  serviceTree : null, 
  bookmark_nodes : null,

  init : function() {
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    jsLoader.loadSubScript( "chrome://songbird/content/scripts/sbIDataRemote.js", this );
    jsLoader.loadSubScript( "chrome://songbird/content/scripts/messagebox.js", this );
    this.loadBookmarks();
    this.installProxy();
    this.rebuildBookmarks();
  },

  installProxy : function () {
    /**
    * Replaces the datasource in the service tree with a filtering proxy that can add
    * additional nodes as required for demos.
    */
	  this.serviceTree = document.getElementById('frame_service_tree');
    
	  var sourceEnumerator = this.serviceTree.database.GetDataSources();
	  var serviceSource = null;
	  var current = null;
    
	  // Find the Servicesource
	  while(sourceEnumerator.hasMoreElements()) {
		  current =  sourceEnumerator.getNext();
		  try {
			  this.serviceSource = current.QueryInterface(Components.interfaces.sbIServicesource);
			  break;
		  } catch (err) {}
    	
	  }

	  this.serviceTree.database.RemoveDataSource(current);
	  this.serviceSourceProxy = new ServicesourceProxy();
	  this.serviceTree.database.AddDataSource(this.serviceSourceProxy);
  },
  
  rebuildBookmarks : function() {
		if (this.serviceSourceProxy && this.serviceTree) {
  		this.serviceSourceProxy.setAdditionalNodes(this.bookmark_nodes);
	  	this.serviceSourceProxy.enableFiltering(true);
		  this.serviceTree.builder.rebuild(); 
		}
  },
  
  saveBookmarks : function() {
    var children = this.bookmark_nodes.children[0].children;
    
    SBDataSetValue("bookmarks.count", children.length);
    for (var i=0;i<children.length;i++) {
      var item = children[i];
      var root = "bookmarks.bookmark"+i;
      SBDataSetValue(root+".label", item.label);
      SBDataSetValue(root+".url", item.url);
      SBDataSetValue(root+".icon", item.icon);
    }
  },
  
  loadBookmarks : function() {
    var thechildren = Array();

    var done = SBDataGetIntValue("bookmarks.prepopulate");
    if (!done) {
      SBDataSetValue("bookmarks.count", 7);
      SBDataSetValue("bookmarks.bookmark0.label", "Pitchfork");
      SBDataSetValue("bookmarks.bookmark0.url", "http://www.pitchforkmedia.com/page/downloads/");
      SBDataSetValue("bookmarks.bookmark0.icon", "http://www.pitchforkmedia.com/favicon.ico");
      SBDataSetValue("bookmarks.bookmark1.label", "Podbop");
      SBDataSetValue("bookmarks.bookmark1.url", "http://podbop.org/");
      SBDataSetValue("bookmarks.bookmark1.icon", "http://podbop.org/favicon.ico");
      SBDataSetValue("bookmarks.bookmark2.label", "Swedelife");
      SBDataSetValue("bookmarks.bookmark2.url", "http://www.swedelife.com/");
      SBDataSetValue("bookmarks.bookmark2.icon", "http://www.swedelife.com/favicon.ico");
      SBDataSetValue("bookmarks.bookmark3.label", "La Blogotheque");
      SBDataSetValue("bookmarks.bookmark3.url", "http://www.blogotheque.net/mp3/");
      SBDataSetValue("bookmarks.bookmark3.icon", "chrome://songbird/skin/serviceicons/pogues.ico");
      SBDataSetValue("bookmarks.bookmark4.label", "Medicine");
      SBDataSetValue("bookmarks.bookmark4.url", "http://takeyourmedicinemp3.blogspot.com/");
      SBDataSetValue("bookmarks.bookmark4.icon", "chrome://songbird/skin/serviceicons/pogues.ico");
      SBDataSetValue("bookmarks.bookmark5.label", "OpenBSD");
      SBDataSetValue("bookmarks.bookmark5.url", "http://openbsd.mirrors.tds.net/pub/OpenBSD/songs/");
      SBDataSetValue("bookmarks.bookmark5.icon", "chrome://songbird/skin/serviceicons/pogues.ico");
      SBDataSetValue("bookmarks.bookmark6.label", "Songbirdnest");
      SBDataSetValue("bookmarks.bookmark6.url", "http://songbirdnest.com/");
      SBDataSetValue("bookmarks.bookmark6.icon", "chrome://songbird/skin/default/logo_16.png");
      SBDataSetValue("bookmarks.prepopulate", 1);
    }

    var n = SBDataGetIntValue("bookmarks.count");
    for (var i=0;i<n;i++) {
      var root = "bookmarks.bookmark"+i;
      var child = { label: SBDataGetValue(root+".label"),
                    icon: SBDataGetValue(root+".icon"),
  				          properties: "bookmark",
						        url: SBDataGetValue(root+".url")
		  };
		  thechildren.push(child);
    }

    this.bookmark_nodes = {
		  children: [
			  { 
				  label: "Bookmarks",
				  icon: "chrome://songbird/skin/default/icon_folder.png",
				  url: "",
				  children: null
		    }
		  ]
		};
		this.bookmark_nodes.children[0].children = thechildren;
	  return this.bookmark_nodes;
  },
  
  findBookmark : function(url) {
    var children = this.bookmark_nodes.children[0].children;
    for (var i=0;i<children.length;i++) {
      var child = children[i];
      if (child.url == url) return i;
    }
    return -1;
  },
  
  getString : function(strid, defstr) {
    var sb = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var bmStrings = sb.createBundle("chrome://songbird/locales/songbird.properties");
    try {
      defstr = bmStrings.GetStringFromName(strid);
    } catch (e) {}
    return defstr;
  },
  
  addBookmark : function() {
		var browser = getBrowserObject();
		if (browser) {
      var theurl = browser.currentURI.spec;
      var n = this.findBookmark(theurl);
      if (n == -1) {
        if (SBGetServiceFromUrl(theurl, true)) {
          sbMessageBox(this.getString("bookmarks.addmsg.title", "Bookmark"), this.getString("bookmarks.addmsg.internal", "This page is already available from the service tree"), false);
          return;
        }
        var thelabel = browser.contentDocument.title;
        if (thelabel == "") thelabel = theurl;
        var theicon = "http://" + browser.currentURI.hostPort + "/favicon.ico";
        var child = { label: thelabel,
                      icon: theicon,
						          url: theurl,
						          properties: "bookmark"
		    };
        this.bookmark_nodes.children[0].children.push(child);
        this.saveBookmarks();
        this.rebuildBookmarks();
        var observer = {
          url : "",
          manager : null,
          onStartRequest : function(aRequest,aContext) {
          },
          onStopRequest : function(aRequest, aContext, aStatusCode) {
            if (aStatusCode != 0) {
              var n = this.manager.findBookmark(this.url);
              if (n >= 0) {
                var children = this.manager.bookmark_nodes.children[0].children;
                children[n].icon = "chrome://bookmarks/skin/icon_bookmark.ico";
                this.manager.saveBookmarks();
                this.manager.rebuildBookmarks();
              }
            }
          }
        };
        observer.manager = this;
        observer.url = theurl;
        var checker = Components.classes["@mozilla.org/network/urichecker;1"].createInstance(Components.interfaces.nsIURIChecker);
        var uri = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURI);
        uri.spec = theicon;
        checker.init(uri);
        checker.asyncCheck(observer, null);
      } else {
        // tell user it already exists
        sbMessageBox(this.getString("bookmarks.addmsg.title", "Bookmark"), this.getString("bookmarks.addmsg.exists", "This bookmark already exists"), false);
      }
		}
  },
  
  onServiceMenu : function( theEvent ) {
    var service = getServiceObject();

    // First, get the row clicked.
    var obj = {}; 
    var row = {};
    var col = {};
    service.tree.treeBoxObject.getCellAt( theEvent.clientX, theEvent.clientY, row, col, obj );

    row = row.value;    

    if ( row >= 0 ) {
      // Find the selected element
      var element = service.tree.contentView.getItemAtIndex( row );
      var properties = element.getAttribute( "properties" ).split(" ");
      if ( properties.length > 0 ) {
        // The first property is the type.  Later strings are specific to the type.
        if ( properties[ 0 ] == "bookmark" ) {
          var children = this.bookmark_nodes.children[0].children;
          var element = service.tree.contentView.getItemAtIndex( row );
          var url = element.getAttribute( "url" );
          var n = this.findBookmark(url);
          if (n >= 0) {
            if (this.popup) { service.removeChild(this.popup); this.popup = null; }
            this.popup = document.createElement("popup");
            this.popup.setAttribute("id", "bookmark_menu");
            this.popup.setAttribute("oncommand", "onBookmarkCommand(event.target);");
            var edititem = document.createElement("menuitem");
            edititem.setAttribute("id", "bookmark.edit");
            edititem.setAttribute("label", this.getString("bookmarks.menu.edit", "Edit"));
            edititem.setAttribute("url", children[n].url);
            var removeitem = document.createElement("menuitem");
            removeitem.setAttribute("id", "bookmark.remove");
            removeitem.setAttribute("label", this.getString("bookmarks.menu.remove", "Remove"));
            removeitem.setAttribute("url", children[n].url);
            this.popup.appendChild(removeitem);
            this.popup.appendChild(edititem);
            service.appendChild(this.popup);
            this.popup.showPopup( service.tree, theEvent.screenX, theEvent.screenY, "context", null, null, null );
            theEvent.preventBubble();
            theEvent.preventDefault();
            return;
          }
        }
      }
    }
    service.onServiceTreeContext(theEvent);
    theEvent.preventBubble();
    theEvent.preventDefault();
  },
  
  onEditCommand : function (target) {
    var url = target.getAttribute("url");
    var n = this.findBookmark(url);
    if (n >= 0) {
      var children = this.bookmark_nodes.children[0].children;
      var data = {};
      data.in_url = children[n].url;
      data.in_label = children[n].label;
      data.in_icon= children[n].icon;
      SBOpenModalDialog( "chrome://songbird/content/xul/editbookmark.xul", "edit_bookmark", "chrome,modal=yes, centerscreen", data );
      if (data.result) {
        var newurl = data.out_url;
        var newicon = data.out_icon;
        var newlabel = data.out_label;
        children[n].url = newurl;
        children[n].label = newlabel;
        children[n].icon = newicon;
        this.saveBookmarks();
        this.rebuildBookmarks();
      } 
    }
  },
  
  onRemoveCommand : function (target) {
    var url = target.getAttribute("url");
    var n = this.findBookmark(url);
    if (n >= 0) {
      var children = this.bookmark_nodes.children[0].children;
      children.splice(n, 1);
      this.saveBookmarks();
      this.rebuildBookmarks();
    }    
  }
};

window.addEventListener("load", function(e) { installBookmarksRdfProxy(e); }, false); 

function installBookmarksRdfProxy() {
  bmManager.init();
  changeServiceContext();
  changeBookmarkButton();
}

function onBookmarkClicked() {
  bmManager.addBookmark();
}

function onBookmarkServiceContextMenu(evt) {
  bmManager.onServiceMenu(evt);
}

function onBookmarkCommand(target) {
  switch (target.getAttribute("id")) {
    case "bookmark.edit":
      bmManager.onEditCommand(target);
      break;
    case "bookmark.remove":
      bmManager.onRemoveCommand(target);
      break;
  }
}
