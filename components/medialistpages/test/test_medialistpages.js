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
 * \brief Medialistpage Unit Test File
 */

Components.utils.import("resource://app/components/sbProperties.jsm");
 
var pageMgr = null;

function runTest () {

  log("Testing MediaListPageManager Service:");

  pageMgr = Components.classes["@songbirdnest.com/Songbird/MediaListPageManager;1"]
                      .getService(Components.interfaces.sbIMediaListPageManager);

  testMediaListPageManager();

  log("OK");
}

const DEFAULTPAGE1 = "chrome://songbird/content/xul/mediapages/playlistPage.xul?useFilters=true"
const DEFAULTPAGE2 = "chrome://songbird/content/xul/mediapages/playlistPage.xul"
const EXTENSIONPAGE = "chrome://songbird-test-media-page/content/testMediaPage.xul"
const BADURL = "http://non/registered/url.xul";
const URL1 = "http://fake/url1.xul";
const URL2 = "http://fake/url2.xul";
const URL3 = "http://fake/url3.xul";

function testMediaListPageManager() {
  if (!pageMgr) {
    fail("Manager service could not be retrieved!");
  }

  // Create our temp library
  var library1 = createLibrary("test_medialistpages", null);
  library1.clear();
  
  // Create list of type "simple"
  var list1 = library1.createMediaList("simple");

  // Verify that the built-in pages have been registered
  // and that giving a list that has no default returns the global default 
  // page
  var pageInfo = pageMgr.getPage(library1);
  assertEqual(
    pageInfo.contentUrl, DEFAULTPAGE1, 
    "built-in page not found (" + pageInfo.contentUrl + ")\n"
  );
  var defaultInfo1 = pageInfo;
  pageInfo = pageMgr.getPage(list1);
  assertEqual(
    pageInfo.contentUrl, DEFAULTPAGE2, 
    "built-in page not found (" + pageInfo.contentUrl + ")\n"
  );
  
  // Set a default for the list
  list1.setProperty(SBProperties.defaultMediaListPageURL, BADURL);
  
  // Verify that giving a list that has an obsolete default returns the 
  // global default page
  pageInfo = pageMgr.getPage(list1);
  assertEqual(
    pageInfo.contentUrl, DEFAULTPAGE2,
    "setting a default page to a BADURL should give DEFAULT"
  );
  var defaultInfo2 = pageInfo;
  
  // test install.rdf loaded page from the test extension
  var rdfInfo = {};
  var pages = pageMgr.getAvailablePages();
  while (pages.hasMoreElements()) {
    var pageInfo = pages.getNext();
    pageInfo.QueryInterface(Components.interfaces.sbIMediaListPageInfo);
    if (pageInfo.contentUrl == EXTENSIONPAGE) {
      rdfInfo = pageInfo;
    }
  }
  assertEqual(rdfInfo.contentUrl, EXTENSIONPAGE,
    "the extension page should be registered among the available pages"
  );
  
  // unregister default pages and verify that nothing remains
  pageMgr.unregisterPage(defaultInfo1);
  pageMgr.unregisterPage(defaultInfo2);
  pageMgr.unregisterPage(rdfInfo);
  var enumerator = pageMgr.getAvailablePages();
  assertEqual(enumerator.hasMoreElements(), false,
    "after unregistering the one built-in page and the extension, the list should be empty"
  );

  // Tabula rasa
  library1.clear();
  list1 = library1.createMediaList("simple");

  // Set up a match for all simple lists
  var page_simpletype = pageMgr.registerPage("MatchSimpleType", 
                        URL2, 
                        function(aList) { return (aList.type == "simple"); } );

  // Verify that matching works :
  // Step 1, only a type match is registered, must match
  pageInfo = pageMgr.getPage(list1);
  assertEqual(pageInfo.contentUrl, URL2);

  // Set up a match for all lists again
  var page_matchall = pageMgr.registerPage("MatchAll", 
                                       URL1, 
                                       function(aList) { return true; } );
  
  // Verify that matching works:
  // Step 2, type(*) + global
  pageInfo = pageMgr.getPage(list1);
  assertEqual(pageInfo.contentUrl, URL2);
 
  // Create a list of type "smart"
  var list2 = library1.createMediaList("smart");
  
  // Verify that matching works:
  // Step 3, non matching type + global(*)
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL1);
  
  // Set up a match for all smart lists
  var page_smarttype = pageMgr.registerPage("MatchSmartType", 
                       URL3, 
                       function(aList) { return (aList.type == "smart"); } );

  // Verify that matching works:
  // Step 4, type + global(*) + type
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL1);
  
  // Set the default for the smart list to the smart page
  list2.setProperty(SBProperties.defaultMediaListPageURL, URL3);
  
  // Verify that matching works:
  // Step 5, bypass search, use list default, check that its match works
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL3);

  // Set the default for the smart list to the simple page (non-matching page 
  // on purpose)
  list2.setProperty(SBProperties.defaultMediaListPageURL, URL2);

  // Verify that matching works:
  // Step 6, check that list default is dropped when it does not match
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL1);

  // Tabula rasa
  library1.clear();
  list1 = library1.createMediaList("simple");
  list2 = library1.createMediaList("smart");

  // Save page_smarttype as user state for list2
  pageMgr.setPage(list2, page_smarttype);

  // Verify that matching works:
  // Step 7, bypass search, use saved state, check that its match works
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL3);

  // Save page_simpletype as user state for list2 (non-matching page on purpose)
  pageMgr.setPage(list2, page_simpletype);
  
  // Verify that matching works:
  // Step 8, check that saved state is dropped when it does not match
  pageInfo = pageMgr.getPage(list2);
  assertEqual(pageInfo.contentUrl, URL1);
  
  // Verify that the internal page list really looks like what we 
  // expected all along
  enumerator = pageMgr.getAvailablePages();
  // 0 elements ? no !
  assertEqual(enumerator.hasMoreElements(), true);
  // element 0 is type match = simple
  assertEqual(enumerator.getNext().contentUrl, URL2);
  // 1 element ? no !
  assertEqual(enumerator.hasMoreElements(), true);
  // element 1 is global match
  assertEqual(enumerator.getNext().contentUrl, URL1);
  // 2 elements ? no !
  assertEqual(enumerator.hasMoreElements(), true);
  // element 2 is type match = smart
  assertEqual(enumerator.getNext().contentUrl, URL3);
  // any more element ? no !
  assertEqual(enumerator.hasMoreElements(), false);

  // Now that we checked matching priorities based on registration order,
  // check that multiple matches work when listing pages
  
  // This should return the simple type match and the global match
  enumerator = pageMgr.getAvailablePages(list1);
  // perform list check
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL2);
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL1);
  assertEqual(enumerator.hasMoreElements(), false);

  // This should return the global match and the smart type match
  enumerator = pageMgr.getAvailablePages(list2);
  // perform list check
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL1);
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL3);
  assertEqual(enumerator.hasMoreElements(), false);

  // Verify that unregistering pages works with multiple pages
  
  // Step 1, remove 2nd of 3 items
  pageMgr.unregisterPage(page_matchall);
  // perform list check
  enumerator = pageMgr.getAvailablePages();
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL2);
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL3);
  assertEqual(enumerator.hasMoreElements(), false);

  // Step 2, remove 2nd of 2 items
  pageMgr.unregisterPage(page_smarttype);
  // perform list check again
  enumerator = pageMgr.getAvailablePages();
  assertEqual(enumerator.hasMoreElements(), true);
  assertEqual(enumerator.getNext().contentUrl, URL2);
  assertEqual(enumerator.hasMoreElements(), false);

  // Step 3, remove last item
  pageMgr.unregisterPage(page_simpletype);
  // perform list check again
  enumerator = pageMgr.getAvailablePages();
  assertEqual(enumerator.hasMoreElements(), false);

  library1.clear();
  
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
  try {
    library.clear();
  }
  catch(e) {
  }

  return library;
}
