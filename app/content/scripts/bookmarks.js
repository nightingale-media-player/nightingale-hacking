/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
  extensionsFolderNode : null,

  init : function() {
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    jsLoader.loadSubScript( "chrome://songbird/content/scripts/messagebox.js", this );
    this.loadBookmarks();
    this.installProxy();
    this.rebuildBookmarks();
  },

  /**
  * Replaces the datasource in the service tree and service menu with a filtering 
  * proxy that can add additional nodes as required for demos.
  */
  installProxy : function () {

    this.serviceTree = document.getElementById('frame_service_tree');
    this.serviceMenu = document.getElementById('services-popup');
    
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
    
    this.serviceMenu.database.RemoveDataSource(current);
    this.serviceMenu.database.AddDataSource(this.serviceSourceProxy);
  },
  
  
  
  rebuildBookmarks : function() {
    if (this.serviceSourceProxy && this.serviceTree) {
      this.serviceSourceProxy.setAdditionalNodes(this.bookmark_nodes);
      this.serviceSourceProxy.enableFiltering(true);
      this.serviceTree.builder.rebuild(); 
      this.serviceMenu.builder.rebuild(); 
    }
  },
  
  saveBookmarks : function() {
    // Don't save the Extension node by filtering it out before the stringify
    var filteredNodes = {};
    filteredNodes.children = this.bookmark_nodes.children.filter(function(o) {
      return o != this.extensionsFolderNode;
    }, this);
    SBDataSetStringValue("bookmarks.serializedTree", JSON.stringify(filteredNodes));
  },
  
  loadBookmarks : function() {
    var thechildren = Array();

    var done = SBDataGetBoolValue("bookmarks.prepopulated");
    if (!done) {
      this.bookmark_nodes = this._getDefaultNodes();    
      this.saveBookmarks();
      SBDataSetBoolValue("bookmarks.prepopulated", true);
      // HACK to force the Bookmarks tree entry open on first run
      SBDataSetBoolValue("servicetree_opened_frame_servicetree_Bookmarks", true);
    } 
    else
    {
      var serializedTree = SBDataGetStringValue("bookmarks.serializedTree");
      this.bookmark_nodes = JSON.parse(serializedTree);
    }
    this.appendExtensionBookmarks();

    return this.bookmark_nodes;
  },
  
  appendExtensionBookmarks : function() {
    var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                        .getService(Components.interfaces.nsIPrefService);

    // Scan this branch for extension bookmarks
    var branch = prefService.getBranch("bookmarks.extensions");
    if(branch) {
      var extensions = {};
      var count = { value : 0 };
      var children = branch.getChildList("", count);
      var hasExtensionBookmarks = false;
      for(var i = 0; i < count.value; i++) {
        var a = children[i].split(".", 2);
        if(a.length == 2) {
          var name = "." + a[1];
          if(!extensions[name]) {
            try {
              var extension = {};
              extension.icon       = branch.getCharPref(name + ".icon");
              extension.label      = branch.getCharPref(name + ".label");
              extension.url        = branch.getCharPref(name + ".url");
              extension.properties = "bookmark";
              extension.children   = [];
              extensions[name] = extension;
              hasExtensionBookmarks = true;
            }
            catch(e) {
              // Extension bookmark pref was invalid, just skip
            }
          }
        }
      }

      if(hasExtensionBookmarks) {
        this.extensionsFolderNode = {
          label: "Extensions",
          icon: "chrome://songbird/skin/default/icon_folder.png",
          url: "",
          properties: "bookmark",
          children: []
        };
        this.bookmark_nodes.children.push(this.extensionsFolderNode);

        for(var extension in extensions) {
          this.extensionsFolderNode.children.push(extensions[extension]);
        }
      }

    }
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
    var bmStrings = sb.createBundle("chrome://songbird/locale/songbird.properties");
    try {
      defstr = bmStrings.GetStringFromName(strid);
    } catch (e) {dump(e);}
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
                children[n].icon = "chrome://songbird/skin/serviceicons/default.ico";
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

          if (this.popup) { service.removeChild(this.popup); this.popup = null; }
          this.popup = document.createElement("popup");
          this.popup.setAttribute("id", "bookmark_menu");
          this.popup.setAttribute("oncommand", "onBookmarkCommand(event.target);");
          var edititem = document.createElement("menuitem");
          edititem.setAttribute("id", "bookmark.edit");
          edititem.setAttribute("label", this.getString("bookmarks.menu.edit", "Edit"));
          edititem.setAttribute("rdfid", element.getAttribute( "id" ));
          var removeitem = document.createElement("menuitem");
          removeitem.setAttribute("id", "bookmark.remove");
          removeitem.setAttribute("label", this.getString("bookmarks.menu.remove", "Remove"));
          removeitem.setAttribute("rdfid", element.getAttribute( "id" ));
          this.popup.appendChild(removeitem);
          this.popup.appendChild(edititem);
          service.appendChild(this.popup);
          this.popup.showPopup( service.tree, theEvent.screenX + 5, theEvent.screenY, "context", null, null, null );
          theEvent.preventBubble();
          theEvent.preventDefault();
          return;
        }
      }
    }
    service.onServiceTreeContext(theEvent);
    theEvent.preventBubble();
    theEvent.preventDefault();
  },
  
  onEditCommand : function (target) {
    var id = target.getAttribute("rdfid");
    var node = this.serviceSourceProxy.getNodeByResourceId(id);
    
    if (node != null) {
      var data = {};
      data.in_url = node.url;
      data.in_label = node.label;
      data.in_icon= node.icon;
      SBOpenModalDialog( "chrome://songbird/content/xul/editbookmark.xul", "edit_bookmark", "chrome,modal=yes, centerscreen", data );
      if (data.result) {
        var newurl = data.out_url;
        var newicon = data.out_icon;
        var newlabel = data.out_label;
        node.url = newurl;
        node.label = newlabel;
        node.icon = newicon;
        this.saveBookmarks();
        this.rebuildBookmarks();
      } 
    }
  },
  
  onRemoveCommand : function (target) {
    var id = target.getAttribute("rdfid");
    var node = this.serviceSourceProxy.getNodeByResourceId(id);
    //dump("\n\nid is: " + id + " resource is " + node + "\n\n");    
    
    // Find index of item in node tree.
    // This could be better.. but it works.
    var i = 0;
    var j = -1;
    var found = false;
    for (i = 0; i < this.bookmark_nodes.children.length; i++) {
      if (this.bookmark_nodes.children[i] == node) {
        j = -1;
        found = true;
        break;
      }
      for (j = 0; j < this.bookmark_nodes.children[i].children.length; j++) {
        if (this.bookmark_nodes.children[i].children[j] == node) {
          found = true;
          break;
        }
      }
      if (found) break;
    }

    
    if (!found) {
      dump("!!! Bookmarks onRemoveCommand failed to find " + id + "\n");
      return;
    }

    dump("\nbookmarks.js: onRemoveCommand found item at " + i + ", " + j + "\n");

    // Remove the node..
    if (i > 0 && j == -1) {
      var children = this.bookmark_nodes.children;
      children.splice(i, 1);
    } else if (i >= 0 && j != -1) {
      var children = this.bookmark_nodes.children[i].children;
      children.splice(j, 1);
    }
        
    this.saveBookmarks();
    this.rebuildBookmarks();
  },
  
  _getDefaultNodes : function() {

    var nodes = {
          children: [
              { /////// BOOKMARKS
                  label: "&servicesource.bookmarks",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  children: [   
                  { 
                      label: "Podbop",
                      icon: "http://podbop.org/favicon.ico",
                      url: "http://podbop.org/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Scissorkick",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.scissorkick.com/",
                      properties: "bookmark",
                      children: []
                  },                                    
                  { 
                      label: "Fluxblog",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://fluxblog.org",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Swedelife",
                      icon: "http://www.swedelife.com/favicon.ico",
                      url: "http://www.swedelife.com/",
                      properties: "bookmark",
                      children: []
                  },                                    
                  { 
                      label: "La Blogothèque",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.blogotheque.net/mp3/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Banana Nutrament",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://banananutrament.blogspot.com/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Beggars Group",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.beggars.com/us/audio_video/index.html",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Up Records",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.uprecords.com/mp3s/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "MonkeyFilter Wiki",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://wiki.monkeyfilter.com/index.php?title=MP3_Blog_Listing",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Songbirdnest",
                      icon: "chrome://songbird/skin/default/logo_16.png",
                      url: "http://songbirdnest.com/",
                      properties: "bookmark",
                      children: []
                  }
/*              ,                   
                  { 
                      label: "Autodownload",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "chrome://songbird/content/xul/remote_download.xul",
                      properties: "bookmark",
                      children: []
                }
*/              
                  ] // end of bookmark children
            },
              { /////// SEARCHES
                  label: "&servicesource.searches",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  properties: "bookmark",
                  children: [
                  { 
                      label: "Google",
                      icon: "chrome://songbird/skin/serviceicons/google.ico",
                      url: "http://www.google.com/musicsearch?q=",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Yahoo!",
                      icon: "chrome://songbird/skin/serviceicons/yahoo.ico",
                      properties: "bookmark",
                      url: "http://audio.search.yahoo.com/",
                      children: []
                  },  
                  { 
                      label: "Creative Commons",
                      icon: "http://creativecommons.org/favicon.ico",
                      url: "http://search.creativecommons.org/",
                      properties: "bookmark",
                      children: []
                  }         
                  ] // end of searches children
            },      
              { /////// MUSIC STORES
                  label: "&servicesource.music_stores",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  properties: "bookmark",
                  url: "",
                  children: [
                  { 
                      label: "Connect",
                      icon: "chrome://songbird/skin/serviceicons/connect.ico",
                      url: "http://musicstore.connect.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Amazon",
                      icon: "chrome://songbird/skin/serviceicons/amazon.ico",
                      url: "http://www.faser.net/mab/chrome/content/mab.xul",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Audible",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://audible.com/",
                      properties: "bookmark",
                      children: []
                  },                                    
                  { 
                      label: "Beatport",
                      icon: "http://www.beatport.com/favicon.ico",
                      url: "http://beatport.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "MP3Tunes",
                      icon: "http://www.mp3tunes.com/favicon.ico",
                      url: "http://www.mp3tunes.com/store.php",
                      properties: "bookmark",
                      children: []
                },
                  { 
                      label: "eMusic",
                      icon: "chrome://songbird/skin/serviceicons/emusic.ico",
                      url: "http://emusic.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "CD Baby",
                      icon: "chrome://songbird/skin/serviceicons/cdbaby.ico",
                      url: "http://cdbaby.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Selectric Records",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.selectricrecords.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Arts and Crafts Records",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.galleryac.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Calabash Music",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",                      
                      url: "http://calabashmusic.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Project Opus",
                      icon: "http://projectopus.com/favicon.ico",
                      url: "http://projectopus.com/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Birdman Records",
                      icon: "http://www.birdmanrecords.com/favicon.ico",
                      url: "http://www.birdmanrecords.com/",
                      properties: "bookmark",
                      children: []
                  }                                                                                                                                           
                  ] // end of music stores children
            },              
              { /////// PODCASTS
                  label: "&servicesource.podcasts",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  properties: "bookmark",
                  children: [
                  { 
                      label: "Odeo Featured",
                      icon: "chrome://songbird/skin/serviceicons/odeo.ico",
                      url: "http://odeo.com/listen/featured/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Podemus",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://podemus.fr/",
                      properties: "bookmark",
                      children: []
                  }         
                  ] // end of podcasts children
            },  
              { /////// RADIO
                  label: "&servicesource.radio",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  properties: "bookmark",
                  children: [
                  { 
                      label: "soma fm",
                      icon: "http://somafm.com/favicon.ico",
                      url: "http://somafm.com/",
                      properties: "bookmark",
                      children: []
                  },   
                  { 
                      label: "95.7 Max FM",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://957maxfm.com/stream.cfm",
                      properties: "bookmark",
                      children: []
                  }     
                  ] // end of radio children
            },  
              { /////// NETWORK SERVICES
                  label: "&servicesource.net.services",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  properties: "bookmark",
                  children: [
                  { 
                      label: "Ninjam",
                      icon: "chrome://songbird/skin/serviceicons/ninjam_gui_win.ico",
                      url: "http://ninjam.com/samples.php",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "last.fm",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://last.fm/group/Songbird",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "AllMusic",
                      icon: "http://www.allmusic.com/favicon.ico",
                      url: "http://www.allmusic.com/",
                      properties: "bookmark",
                      children: []
                  },                                    
                  { 
                      label: "MP3Tunes Locker",
                      icon: "http://mp3tunes.com/favicon.ico",
                      url: "http://www.mp3tunes.com/locker/",
                      properties: "bookmark",
                      children: []
                  },  
                  { 
                      label: "Streampad",
                      icon: "chrome://songbird/skin/serviceicons/default.ico",
                      url: "http://www.streampad.com/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Tables Turned",
                      icon: "http://tablesturned.com/favicon.ico",
                      url: "http://tablesturned.com/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Music Strands",
                      icon: "http://musicstrands.com/favicon.ico",
                      url: "http://musicstrands.com/",
                      properties: "bookmark",
                      children: []
                  },
                  { 
                      label: "Jamendo",
                      icon: "http://jamendo.com/favicon.ico",
                      url: "http://jamendo.com/",
                      properties: "bookmark",
                      children: []
                  }                                                                 
                  ] // end of net services children
              },                          
              { /////// NETWORK DEVICES
                  label: "&servicesource.net.devices",
                  icon: "chrome://songbird/skin/default/icon_folder.png",
                  url: "",
                  properties: "bookmark",
                  children: [
                    { 
                      label: "SlimDevices",
                      icon: "http://slimdevices.com/favicon.ico",
                      url: "http://www.slimdevices.com/",
                      properties: "bookmark",
                      children: []
                    },  
                    { 
                      label: "Sonos",
                      icon: "http://www.sonos.com/favicon.ico",
                      url: "http://www.sonos.com/",
                      properties: "bookmark",
                      children: []
                    }         
                  ] // end of devices children
              }
          ]
    };
    return nodes;
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
