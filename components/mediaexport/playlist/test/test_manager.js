/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

/**
 * \brief Test file
 */

function runTest () {

  var library = createLibrary("test_playlistwritermanager");

  var manager = Cc["@getnightingale.com/Nightingale/PlaylistWriterManager;1"]
                  .getService(Ci.sbIPlaylistWriterManager);
  var platform = getPlatform();

  // Test mime types
  var mimeTypesCount = {};
  var mimeTypes = manager.supportedMIMETypes(mimeTypesCount);
  var expected = ["audio/mpegurl", "audio/x-mpegurl"];

  for (var i in expected) {
    assertTrue(
      mimeTypes.some( function(a) { return a == expected[i]}),
      "Could not find support for mimetype " + expected[i]);
  }

  for (var i in mimeTypes) {
    assertTrue(
      expected.some( function(a) { return a == mimeTypes[i]}),
      "Found unexpected support for mimetype " + mimeTypes[i]);
  }

  // Test extensions
  var extCount = {};
  var exts = manager.supportedFileExtensions(extCount);
  expected = ["m3u"];

  for (var i in expected) {
    assertTrue(
      exts.some( function(a) { return a == expected[i]}),
      "Could not find support for mimetype " + expected[i]);
  }

  for (var i in exts) {
    assertTrue(
      expected.some( function(a) { return a == exts[i]}),
      "Found unexpected support for extension " + exts[i]);
  }


  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  var temporaryFileService =
        Cc["@getnightingale.com/Nightingale/TemporaryFileService;1"]
          .getService(Ci.sbITemporaryFileService);
  var testDir = temporaryFileService.createFile(Ci.nsIFile.DIRECTORY_TYPE);

  var testMediaList = library.createMediaList("simple");

  var item1File = testDir.clone();
  item1File.append("item1.mp3");
  var item1 = library.createMediaItem(ioService.newFileURI(item1File));
  testMediaList.add(item1);

  var item2File = testDir.clone();
  item2File.append("item2Dir");
  item2File.append("item2.mp3");
  var item2 = library.createMediaItem(ioService.newFileURI(item2File));
  testMediaList.add(item2);

  // Test write with extension
  var playlistFile1 = testDir.clone();
  playlistFile1.append("playlist1.m3u");
  manager.write(playlistFile1, testMediaList, null);
  assertFilesEqual(playlistFile1, getFile("test.m3u"));

  // Test write with content type
  var playlistFile2 = testDir.clone();
  playlistFile2.append("playlist2");
  manager.write(playlistFile2, testMediaList, "audio/mpegurl");
  assertFilesEqual(playlistFile2, getFile("test.m3u"));
}

function getFileUri(aFile) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  return ioService.newFileURI(aFile);
}

