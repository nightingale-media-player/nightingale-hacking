/**
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
 
/**
 * \file sbPLSPlaylistHandler.js
 */

function sbPLSPlaylistHandler() {
  this._originalURI = null;
}

// sbIPlaylistReader
sbPLSPlaylistHandler.prototype.__defineGetter__("originalURI",
function()
{
  return this._originalURI;
});

sbPLSPlaylistHandler.prototype.__defineSetter__("originalURI",
function(value)
{
  this._originalURI = value;
});

sbPLSPlaylistHandler.prototype.read =
function(aFile, aMediaList, aReplace)
{
  var playlistItems = {};

  // Match any line that starts with File, Title, or Length and then a number,
  // an equals sign, and the rest of the line.
  var re = new RegExp("^(File|Title|Length)(\\d+)=(.+)$");

  var self = this;
  SB_ProcessFile(aFile, function(aLine) {

    var a = aLine.match(re);
    if (a) {
      var kind = a[1];
      var num = parseInt(a[2], 10);
      var value = a[3];
      if (!isNaN(num) && value != "") {

        var item = playlistItems[num];
        if (!item) {
          item = {};
          playlistItems[num] = item;
        }

        switch(kind) {
          case "File":
            var uri = SB_ResolveURI(value, this._originalURI);
            if (uri)
              item.uri = uri;
          break;
          case "Title":
            if (value != "")
              item.title = value;
          break;
          case "Length":
            var length = parseInt(value, 10);
            if (!isNaN(length) && length >= 0)
              item.length = length;
          break;
        }
      }
    }

    // Ignore all other lines

  }, this);

  // Copy the items we found in the playlist into an array for sorting
  var itemList = [];
  for (var i in playlistItems) {
    itemList.push({ index: i, data: playlistItems[i]});
  }

  // Sort the items by index
  itemList.sort(function(a, b) { a.index - b.index });

  var toAdd = [];
  itemList.forEach(function(e) {
    var data = e.data;
    if (data.uri) {
      var item = { uri: data.uri, properties: {} };
      toAdd.push(item);
      if (data.title)
        item.properties[SB_NS + "title"] = data.title;
      if (data.length)
        item.properties[SB_NS + "duration"] = data.length * 1000000;
    }
  });

  SB_AddItems(toAdd, aMediaList, aReplace);
}

sbPLSPlaylistHandler.prototype.vote =
function(aURL)
{
  return 10000;
}

sbPLSPlaylistHandler.prototype.name =
function()
{
  return "Songbird PLS Reader";
}

sbPLSPlaylistHandler.prototype.description =
function()
{
  return "Loads PLS playlists from remote and local locations.";
}

sbPLSPlaylistHandler.prototype.supportedMIMETypes =
function(aMIMECount, aMIMETypes)
{
  var mimeTypes = ["audio/x-scpls"];
  aMIMECount.value = mimeTypes.length;
  return mimeTypes;
}

sbPLSPlaylistHandler.prototype.supportedFileExtensions =
function(aExtCount, aExts)
{
  var exts = ["pls"];
  aExtCount.value = exts.length;
  return exts;
}

sbPLSPlaylistHandler.prototype.QueryInterface =
function(iid)
{
  if (!iid.equals(Ci.sbIPlaylistReader) &&
      !iid.equals(Ci.nsISupports))
    throw Cr.NS_ERROR_NO_INTERFACE;
  return this;
}

