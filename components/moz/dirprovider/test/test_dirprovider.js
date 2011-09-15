/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

/**
 * \brief test the directory service provider
 */
function runTest() {
  var dirService = Cc["@mozilla.org/file/directory_service;1"]
                     .getService(Ci.nsIProperties);
  var dirProvider = Cc["@songbirdnest.com/moz/directory/provider;1"]
                     .getService(Ci.nsIDirectoryServiceProvider);

  // Test that our directory service provider returns valid directories
  // in all cases - both directly and through directory service.
  var testDirs = ["CmDocs",
                  "CmPics",
                  "CmMusic",
                  "CmVideo",
                  "CmPics",
                  "Docs",
                  "Pics",
                  "Music",
                  "Video",
                  "DiscBurning"];

  for each (let dir in testDirs) {
    // We cannot know whether the result is correct but test at least that
    // it is a directory.
    let file1 = dirService.get(dir, Ci.nsIFile);
    assertTrue(file1, "Non-null file returned by directory service");
    assertTrue(file1.exists(), "Existing file returned");
    assertTrue(file1.isDirectory(), "Directory returned");

    // Make sure that directory service uses our provider to resolve the
    // directories (or at least produces identical results)
    let file2 = dirProvider.getFile(dir, {});
    assertTrue(file2, "Non-null file returned by directory provider");
    assertTrue(file2.equals(file1), "Our provider returns the same file as directory service");
  }

  let hadException = false;
  try {
    dirProvider.getFile("FooBarDummy", {});
  }
  catch (e) {
    hadException = true;
  }
  assertTrue(hadException, "Exception thrown when trying to request unknown file");
}
