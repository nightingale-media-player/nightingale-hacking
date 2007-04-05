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
var OUTPUT_URL = "file:///C:/test_mediaitem.out";
var INPUT_URL_EXISTS = "http://stashbox.org/796/zadornov.mp3";
var INPUT_URL_NOT_EXIST = "http://stashbox.org/poo/poo.mp3";


/**
 * Makes a new URI from a url string
 */
function newURI(aURLString)
{
  // Must be a string here
  if (!(aURLString &&
       (aURLString instanceof String) || typeof(aURLString) == "string"))
    throw Components.results.NS_ERROR_INVALID_ARG;
  
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);
  
  try {
    return ioService.newURI(aURLString, null, null);
  }
  catch (e) { }
  
  return null;
}

function createItem( library, url ) {
  return library.createMediaItem(newURI(url));
}

function testGet( item, attrib, value ) {
  var test_value = item[ attrib ];
  if ( test_value.spec ) { // for nsURI return values, you must compare spec?
    assertEqual( test_value.spec, value );
  } else {
    assertEqual( test_value, value );
  }
}

function testSet( item, attrib, value ) {
  item[ attrib ] = value;
  testGet( item, attrib, value );
}

function testAvailable( library, url, available, completion ) {
  var item = createItem( library, url );

  // Make an observer to receive results of the availability test.
  // This one is complicated because I want to chain tests.
  var is_available_observer = {
    _item: item,
    _available: available,
    _completion: completion,
    observe: function( aSubject, aTopic, aData ) {
      if ( aTopic == "available" ) {
        log( "(sbIMediaItem::TestIsAvailable) COMPLETE " + this._item.contentSrc.spec + ": " + aTopic + " == " + aData );
        assertEqual( aData, this._available );
        if ( this._completion != null )
          try {
            this._completion();
          } catch ( e ) {
            assertEqual( e, null ); // Fail the tests on a throw.
          }
        testFinished();
      }
    },
    QueryInterface: function(iid) {
      if (!iid.equals(Components.interfaces.nsIObserver) && 
          !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  }
  
  // Start the test, tell the testharness to wait for us.
  item.testIsAvailable( is_available_observer );
  testPending();
  log( "(sbIMediaItem::TestIsAvailable) START    " + url );
}

function runTest () {

  var databaseGUID = "test_localdatabaselibrary";
  var testlib = createLibrary(databaseGUID);
  
  // Get an item
  var item = testlib.getMediaItem("3E586C1A-AD99-11DB-9321-C22AB7121F49");
  
  // Basic tests on retrieving its info...
  testGet( item, "library", testlib );  
  testGet( item, "isMutable", true );  
  testGet( item, "mediaCreated", 1169855962000 );
  testGet( item, "mediaUpdated", 1169855962000 );  
  testGet( item, "contentSrc", "file:///home/steve/Hells%20Bells.mp3" );  
  testGet( item, "contentLength", 0x21C );  
  testGet( item, "contentType", "audio/mpeg" );  
/* -- SetProperty is not yet implemented by aus, so these don't work.
  // Slightly more complicated tests for setting its info
  testSet( item, "mediaCreated", 0x1337baadf00d );
  testSet( item, "mediaUpdated", 0x1337baadf00d );
  testSet( item, "contentSrc", "file://poo" );
  testSet( item, "contentLength", 0xbaadf00d );
  testSet( item, "contentType", "x-media/x-poo" );
*/
  
  var inputItem = testlib.createMediaItem( newURI(INPUT_URL_EXISTS) );
/* -- Implemented, not yet written to test
  var inputChannel = inputItem.openInputStreamAsync();
  assertNotEqual(inputChannel, null);
  // ??? Then what?
  var inputStream = inputItem.openInputStream();
  assertNotEqual(inputStream, null);
  // ??? Then what?
*/
  
  var outputItem = testlib.createMediaItem( newURI(OUTPUT_URL) );
  var outputStream = outputItem.openOutputStream();
  assertNotEqual(outputStream, null);
  // ??? Then what?

  // Async tests of availability for a (supposedly!) known url.
  testAvailable( testlib, INPUT_URL_EXISTS, "true", 
    function() {
      testAvailable( testlib, INPUT_URL_NOT_EXIST, "false" /* , 
        function() {
          testAvailable( testlib, "file:///c:/boot.ini", "true" ); // Works on Win32
        } */
      );
    }
  ); 
}
