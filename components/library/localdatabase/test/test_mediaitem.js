/*
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
 * \brief Test file
 */

function testSet( item, attrib, value ) {
  item[ attrib ] = value;
  var test_value = item[ attrib ];
  assertEqual( test_value, value );
}

function runTest () {

  var databaseGUID = "test_localdatabaselibrary";
  var testlib = createLibrary(databaseGUID);
  
  // Get an item
  var item = testlib.getMediaItem("3E586C1A-AD99-11DB-9321-C22AB7121F49");
  
  // Basic tests on retrieving its info...
  var library = item.library;
  assertEqual(library, testlib);
  
  var isMutable = item.isMutable;
  assertEqual(isMutable, true);

/* -- Not yet implemented.
  
  var originLibrary = item.originLibrary();
  assertEqual(originLibrary, originLibrary);  // !!!!!!!!!! - Not a valid test!
  
  var mediaCreated = item.mediaCreated;
  assertEqual(mediaCreated, mediaCreated);  // !!!!!!!!!! - Not a valid test!
  
  var mediaUpdated = item.mediaUpdated;
  assertEqual(mediaUpdated, mediaUpdated);  // !!!!!!!!!! - Not a valid test!
  
  var contentSrc = item.contentSrc;
  assertEqual(contentSrc, contentSrc);  // !!!!!!!!!! - Not a valid test!
  
  var contentLength = item.contentLength;
  assertEqual(contentLength, contentLength);  // !!!!!!!!!! - Not a valid test!
  
  var contentType = item.contentType;
  assertEqual(contentType, contentType);  // !!!!!!!!!! - Not a valid test!
  
*/

  // Slightly more complicated tests for setting its info
/* -- Not yet implemented.

  testSet( item, "mediaCreated", 0xbaadf00d );

  testSet( item, "mediaUpdated", 0xbaadf00d );

  testSet( item, "contentSrc", "file://poo" );

  testSet( item, "contentLength", 0xbaadf00d );

  testSet( item, "contentType", "x-media/x-poo" );
  
*/
  
  // I don't know that I really want to test file I/O on these items....
/* -- Not yet implemented.

  var inputStream = item.openInputStream( 0 );
  assertNotEqual(inputStream, null);

  var ouputStream = item.openOuputStream( 0 );
  assertNotEqual(inputStream, null);
  
*/


  // What other tests do I need to implement?  I assume I don't need to test "toString" ?

}

