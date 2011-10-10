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
  var library = createLibrary("test_playlistwriter", null, false);
  library.clear();

  var playlistWriter = Cc["@getnightingale.com/Nightingale/PlaylistWriter/M3U;1"]
                         .createInstance(Ci.sbIPlaylistWriter);

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

  var playlistFile = testDir.clone();
  playlistFile.append("playlist1.m3u");

  playlistWriter.write(playlistFile, testMediaList, "");
  assertFilesEqual(playlistFile, getFile("test.m3u"));
}

