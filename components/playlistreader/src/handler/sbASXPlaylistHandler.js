/**
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
 * \file sbASXPlaylistHandler.js
 */

function sbASXPlaylistHandler() {
  this._originalURI = null;
}

// sbIPlaylistReader
sbASXPlaylistHandler.prototype.__defineGetter__("originalURI",
function()
{
  return this._originalURI;
});

sbASXPlaylistHandler.prototype.__defineSetter__("originalURI",
function(value)
{
  this._originalURI = value;
});

sbASXPlaylistHandler.prototype.read =
function(aFile, aMediaList, aReplace)
{
  var domParser = Cc["@mozilla.org/xmlextras/domparser;1"]
                    .createInstance(Ci.nsIDOMParser);
  var fileStream = Cc["@mozilla.org/network/file-input-stream;1"]
                    .createInstance(Ci.nsIFileInputStream);

  var itemList = [];

  fileStream.init(aFile, PR_RDONLY, PR_FLAGS_DEFAULT, 0);
  
  var doc = domParser.parseFromStream(fileStream, 
                                      null, 
                                      fileStream.available(),
                                      "text/xml");
  fileStream.close();

  doc = doc.documentElement;  

  var entries = doc.getElementsByTagName("ENTRY");
  
  for(var i = 0; i < entries.length; ++i) {
    
    var item = {};  
    item.uri = [];
      
    var children = entries.item(i).childNodes;
        
    for(var j = 0; j < children.length; ++j) {
    
      var child = children.item(j);
      
      switch(child.nodeName) {
      
        case "TITLE":
          var title = child.firstChild.nodeValue;
          
          if (title) {
            item.title = title;
          }
        break;
        
        case "REF":
          var href = child.getAttribute("href");
          var uri = SB_ResolveURI(href, this._originalURI);
          
          if (uri) {
            item.uri.push(uri);
          }
        break;
                
      }
      
    }
    
    itemList.push(item);
  }
  
  var toAdd = [];
  itemList.forEach(function(e) {
    for(var i = 0; i < e.uri.length; ++i) {
      var item = { uri: e.uri[i], properties: {} };
      toAdd.push(item);
      if (e.title)
        item.properties[SBProperties.trackName] = e.title;
    }
  });

  SB_AddItems(toAdd, aMediaList, aReplace);
}

sbASXPlaylistHandler.prototype.vote =
function(aURL)
{
  return 10000;
}

sbASXPlaylistHandler.prototype.name =
function()
{
  return "Songbird ASX Reader";
}

sbASXPlaylistHandler.prototype.description =
function()
{
  return "Loads ASX playlists from remote and local locations.";
}

sbASXPlaylistHandler.prototype.supportedMIMETypes =
function(aMIMECount, aMIMETypes)
{
  var mimeTypes = ["video/x-ms-asf"];
  aMIMECount.value = mimeTypes.length;
  return mimeTypes;
}

sbASXPlaylistHandler.prototype.supportedFileExtensions =
function(aExtCount, aExts)
{
  var exts = ["asx"];
  aExtCount.value = exts.length;
  return exts;
}

sbASXPlaylistHandler.prototype.QueryInterface =
function(iid)
{
  if (!iid.equals(Ci.sbIPlaylistReader) &&
      !iid.equals(Ci.nsISupports))
    throw Cr.NS_ERROR_NO_INTERFACE;
  return this;
}
