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
  var test_value = item[ attrib ];
  if ( test_value.spec ) { // for nsURI return values, you must compare spec?
    log( "!!!!!!! item." + attrib + " = " + test_value.spec );
    assertEqual( test_value.spec, value );
  } else {
    log( "!!!!!!! item." + attrib + " = " + test_value );
    assertEqual( test_value, value );
  }
}

function testAvailable( item, available, completion ) {
  // Make an observer.
  var is_available_observer = {
    _available: available,
    _completion: completion,
    observe: function( aSubject, aTopic, aData ) { 
      assertEqual( aData, this._available );
      this._completion();
    },
    QueryInterface: function(iid) {
      if (!iid.equals(Components.interfaces.nsIObserver) && 
          !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  }
  item.testIsAvailable( is_available_observer );
  
  // How do I wait the tests to get back my observer call?
}

function runTest () {

  var databaseGUID = "test_localdatabaselibrary";
  var testlib = createLibrary(databaseGUID);
  
  // Get an item
  var item = testlib.getMediaItem("3E586C1A-AD99-11DB-9321-C22AB7121F49");
  
  // Basic tests on retrieving its info...
  testGet( item, "library", testlib );  
  testGet( item, "isMutable", true );  
//  testGet( item, "originLibrary", testlib );  // -- Not to be implemented??

  // These are obviously testing incorrect values, I'll fix them before I commit.
  testGet( item, "mediaCreated", 1169855962000 );
  testGet( item, "mediaUpdated", 1169855962000 );  
  testGet( item, "contentSrc", "file:///home/steve/Hells%20Bells.mp3" );  
  testGet( item, "contentLength", 0x21C );  
  testGet( item, "contentType", "audio/mpeg" );  
  
/* -- Not yet implemented.
  // How can I make sure this is always available?
  // Use an HTTP resource that we control?
  testAvailable( item, true ); 
*/

/* -- SetProperty is not yet implemented by aus, so these don't work.

  // Slightly more complicated tests for setting its info
  testSet( item, "mediaCreated", 0xbaadf00d );
  testSet( item, "mediaUpdated", 0xbaadf00d );
  testSet( item, "contentSrc", "file://poo" );
  testSet( item, "contentLength", 0xbaadf00d );
  testSet( item, "contentType", "x-media/x-poo" );
*/
  
/* -- Not yet implemented.
  // Now it should no longer be available
  testAvailable( item, false ); 
*/

/* -- Not yet implemented.

  var inputStream = item.openInputStream( 0 );
  assertNotEqual(inputStream, null);
  // ??? Then what?

  var ouputStream = item.openOuputStream( 0 );
  assertNotEqual(ouputStream, null);
  // ??? Then what?
  
*/
}

