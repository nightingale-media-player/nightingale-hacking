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
 * \brief Hack script to load a songbird library from a preformatted text file.
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

// Not part of the automated test suite.  Only useful for 
// manual testing.

//  makeLibrary(1000);
//  makeLibrary(5000);
//  makeLibrary(10000);
//  makeLibrary(25000);
//  makeLibrary(50000);
//  makeLibrary(100000);
}

function makeLibrary(aLength) {

  var library = createLibrary("test_" + aLength);
  library.clear();

  var file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath("/builds/songbird/trunk/big.txt");

  var data = "";
  var fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  var sstream = Cc["@mozilla.org/scriptableinputstream;1"]
                  .createInstance(Ci.nsIScriptableInputStream);
  fstream.init(file, -1, 0, 0);
  sstream.init(fstream);

  var str = sstream.read(4096);
  while (str.length > 0) {
    data += str;
    str = sstream.read(4096);
  }

  sstream.close();
  fstream.close();

  var uris       = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  var properties = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);

  var a = data.split("\n");
  for (var i = 0; i < a.length && i < aLength; i++) {

    var pa = SBProperties.createArray();
    var b = a[i].split("\t");

    var artist = b[1];
    if (artist && artist.length) {
      pa.appendProperty(SBProperties.artistName, artist);
    }

    var album  = (b[3] == "" ? "" : b[3] + " ") +  b[2];
    if (album && album.length) {
      pa.appendProperty(SBProperties.albumName, album);
    }

    var genre = b[4];
    if (genre && genre.length) {
      pa.appendProperty(SBProperties.genre, genre);
    }

    var year = b[5].length > 3 ? parseInt(b[5].substr(0, 4)) : null;
    if (year && !isNaN(year)) {
      try {
        pa.appendProperty(SBProperties.year, year);
      }
      catch(e) {
        // don't care
      }
    }

    var track  = (b[8] == "" ? "" : b[8] + " ") +  b[6];
    if (track.length) {
      pa.appendProperty(SBProperties.trackName, track);
    }

    var trackNumber = parseInt(b[7]);
    if (!isNaN(trackNumber)) {
      pa.appendProperty(SBProperties.trackNumber, trackNumber);
    }

    var length;
    var c = b[9].split(":");
    if (c.length == 2) {
      length = (c[0] * 60) + c[1];
      pa.appendProperty(SBProperties.duration, length);
    }

    var uri = newURI("file:///foo/" + escape(artist) + "-" +
                     escape(album) + "-" + escape(track) + ".mp3");
    uris.appendElement(uri, false);
    properties.appendElement(pa, false);

/*
    dump("album: '" + album + "'\n");
    dump("genre: '" + genre + "'\n");
    dump("year: '" + year + "'\n");
    dump("track: '" + track + "'\n");
    dump("trackNumber: '" + trackNumber + "'\n");
    dump("legth: '" + length + "'\n");
*/

    if (i > 0 && (i + 1) % 1000 == 0) {
      library.batchCreateMediaItems(uris, properties, true);
      dump("loading " + i + " a.length " + a.length + " length + " + aLength + "\n");
      var uris       = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
      var properties = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);

    }

  }

 // library.batchCreateMediaItems(uris, properties, true);

}

function createLibrary(databaseGuid, databaseLocation) {

  var directory;
  if (databaseLocation) {
    directory = databaseLocation.QueryInterface(Ci.nsIFileURL).file;
  }
  else {
    directory = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).
                get("ProfD", Ci.nsIFile);
    directory.append("db");
  }

  var file = directory.clone();
  file.append(databaseGuid + ".db");

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .getService(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(hashBag);
  library.clear();
  return library;
}
