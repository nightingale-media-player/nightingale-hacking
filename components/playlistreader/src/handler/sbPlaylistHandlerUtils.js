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
 * \file sbPlaylistHandlerUtils.js
 */

const SB_NS = "http://songbirdnest.com/data/1.0#";

function SB_ProcessFile(aFile, aCallback, aThis) {

  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aFile, 0x01, 0444, 0);
  istream.QueryInterface(Ci.nsILineInputStream);

  var line = {}, hasmore;
  do {
    hasmore = istream.readLine(line);
    aCallback.apply(aThis, [line.value]);
  } while(hasmore);

  istream.close();
}

function SB_ResolveURI(aStringURL, aBaseURI)
{
  var isURI = false;

  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);

  // If there is no base URI or the base URI is a file, try the string as a
  // file
  if (aBaseURI == null || aBaseURI.scheme == "file") {
    try {
      var file = Cc["@mozilla.org/file/local;1"]
                   .createInstance(Ci.nsILocalFile);
      file.initWithPath(aStringURL);

      var uri = ios.newFileURI(file);
      return uri;
    }
    catch(e) {
      // If the base URI is a local file, try to use it to resolve the local
      // file path
      // XXXsteve: this does not work since setRelativeDescriptor does not know
      // if the leaf of the base path is a directory or a file.
/*
      if (aBaseURI && aBaseURI.scheme == "file") {
        try {
          var baseFile = aBaseURI.QueryInterface(Ci.nsIFileURL).file;
          var file = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
          file.setRelativeDescriptor(baseFile, aStringURL);
          var uri = ios.newFileURI(file);
          return uri;
        }
        catch(e) {
          // fall through
        }
      }
*/
    }
  }

  // Ok, it is not a local file.  Try creating a new URI with the base URI
  try {
    var uri = ios.newURI(aStringURL, null, aBaseURI);
    return uri;
  }
  catch(e) {
    // fall through
  }

  // Couldn't resolve it, return null for failure
  return null;
}

function SB_GetFirstItemByContentUrl(aMediaList, aURI) {

  var listener = {
    item: null,
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.item = item;
      return false;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  aMediaList.enumerateItemsByProperty(SB_NS + "contentUrl",
                                      aURI.spec,
                                      listener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  return listener.item;
}

