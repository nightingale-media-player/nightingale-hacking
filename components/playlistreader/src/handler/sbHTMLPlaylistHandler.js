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
 * \file sbHTMLPlaylistHandler.js
 */
function sbHTMLPlaylistHandler() {
  this._originalURI = null;
}

// sbIPlaylistReader
sbHTMLPlaylistHandler.prototype.__defineGetter__("originalURI",
function()
{
  return this._originalURI;
});

sbHTMLPlaylistHandler.prototype.__defineSetter__("originalURI",
function(value)
{
  this._originalURI = value;
});

sbHTMLPlaylistHandler.prototype.read =
function(aFile, aMediaList, aReplace)
{
  var toAdd = [];

  var pps = Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
              .getService(Ci.sbIPlaylistPlayback);

  // A fairly permissive regular expression to match href attributes.  It will
  // match href values that are surrounded by single or double quotes, or no
  // quotes at all.  It is also permissive about whitespace around the equals
  // sign.
  //
  // The regular expression explained:
  //   match "href" then 0 or more characters of whitespace
  //   match "=" then 0 or more characters of whitespace
  //   capture an optional single or double quote
  //   non-greedy capture of everything up to...
  //   something that matches the first capture or a close angle bracket
  //
  // Note that the match we want shows up in the second capture
  var re = new RegExp("href\\s*=\\s*(['\"])?(.*?)(?:\\1|>)", "ig");

  var self = this;
  SB_ProcessFile(aFile, function(aLine) {

    var a = re.exec(aLine);
    while (a && a.length == 3) {
      var href = a[2];
      if (href && pps.isMediaURL(href)) {
        var newUri = SB_ResolveURI(href, this._originalURI);
        if (newUri) {
          var item = { uri: newUri, properties: {}};
          toAdd.push(item);
        }
      }

      a = re.exec(aLine);
    }

  }, this);

  SB_AddItems(toAdd, aMediaList, aReplace);

}

sbHTMLPlaylistHandler.prototype.vote =
function(aURL)
{
  return 10000;
}

sbHTMLPlaylistHandler.prototype.name =
function()
{
  return "Songbird HTML Reader";
}

sbHTMLPlaylistHandler.prototype.description =
function()
{
  return "Scrape HREFs from HTML files.";
}

sbHTMLPlaylistHandler.prototype.supportedMIMETypes =
function(aMIMECount)
{
  var mimeTypes = ["text/html"];
  aMIMECount.value = mimeTypes.length;
  return mimeTypes;
}

sbHTMLPlaylistHandler.prototype.supportedFileExtensions =
function(aExtCount)
{
  var exts = ["html", "htm", "php", "php3", ""];
  aExtCount.value = exts.length;
  return exts;
}

sbHTMLPlaylistHandler.prototype.QueryInterface =
function(iid)
{
  if (!iid.equals(Ci.sbIPlaylistReader) &&
      !iid.equals(Ci.nsISupports))
    throw Cr.NS_ERROR_NO_INTERFACE;
  return this;
}

