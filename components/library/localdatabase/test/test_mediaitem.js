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
 
var URL_AVAILABLE = "http://download.songbirdnest.com/extensions/test/test1.mp3";
var URL_NOT_AVAILABLE = "http://download.songbirdnest.com/extensions/poo/poo.mp3";
var TEST_STRING = "The quick brown fox jumps over the lazy dog.  Bitches."



function read_stream(stream, count) {
  /* assume stream has non-ASCII data */
  var wrapper =
      Cc["@mozilla.org/binaryinputstream;1"].
      createInstance(Ci.nsIBinaryInputStream);
  wrapper.setInputStream(stream);
  /* JS methods can be called with a maximum of 65535 arguments, and input
     streams don't have to return all the data they make .available() when
     asked to .read() that number of bytes. */
  var data = [];
  while (count > 0) {
    var bytes = wrapper.readByteArray(Math.min(65535, count));
    data.push(String.fromCharCode.apply(null, bytes));
    count -= bytes.length;
    if (bytes.length == 0)
      do_throw("Nothing read from input stream!");
  }
  return data.join('');
}

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

function newFileURI(file)
{
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);
  try {
    return ioService.newFileURI(file);
  }
  catch (e) { }
  
  return null;
}

function newTempURI( name ) {
  var file = Components.classes["@mozilla.org/file/directory_service;1"]
                      .getService(Components.interfaces.nsIProperties)
                      .get("TmpD", Components.interfaces.nsIFile);
  file.append(name);
  file.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0664);
  var retval = newFileURI(file);
  log( "newTempURI = " + retval.spec );
  return retval;
}

function createItem( library, url ) {
  return library.createMediaItem(newURI(url));
}

function testGet( item, attrib, value ) {
  var test_value = item[ attrib ];
  // for nsURI return values, you must compare spec?
  if ( test_value.spec )
    test_value = test_value.spec;
  if ( value.spec )
    value = value.spec;
  assertEqual( test_value, value );
}

function testSet( item, attrib, value ) {
  log("testSet: " + attrib + " -> " + value);
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

function testIsAvailable( ioItem ) {
  var databaseGUID = "test_localdatabaselibrary";
  var testlib = createLibrary(databaseGUID);
  // Async tests of availability for a (supposedly!) known url.
  testAvailable( testlib, ioItem.contentSrc.spec, "true", 
    function() {
      testAvailable( testlib, URL_NOT_AVAILABLE, "false", 
        function() {
          testAvailable( testlib, URL_AVAILABLE, "true" );
        }
      );
    }
  ); 
}

function testAsyncRead(ioItem) {
  var listener = {
    _item: ioItem,
    
    /**
    * See nsIStreamListener.idl
    */
    onDataAvailable: function LLL_onDataAvailable(request, context, inputStream, 
                                                  sourceOffset, count) {
      // Once we have enough bytes, read it.                                                  
      if ((count + sourceOffset) >= TEST_STRING.length) {
        this._value = read_stream(inputStream, (count + sourceOffset));
      }
    },
    
    /**
    * See nsIRequestObserver.idl
    */
    onStartRequest: function LLL_onStartRequest(request, context) {
    },
    
    /**
    * See nsIRequestObserver.idl
    */
    onStopRequest: function LLL_onStopReqeust(request, context, status) {
      // Test this and go on to the next tests.
      assertEqual( this._value, TEST_STRING );
      testIsAvailable( this._item );
      testFinished();
    },
    /**
    * See nsISupports.idl
    */
    QueryInterface: function LLL_QueryInterface(iid) {
      if (iid.equals(Ci.nsIStreamListener) ||
          iid.equals(Ci.nsIRequestObserver)||
          iid.equals(Ci.nsISupports))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  }
  var inputChannel = ioItem.openInputStreamAsync(listener, null);
  assertNotEqual(inputChannel, null);
  listener._inputChannel = inputChannel;
  testPending();
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

  // Slightly more complicated tests for setting its info
  testSet( item, "mediaCreated", 0x1337baadf00d );
  testSet( item, "mediaUpdated", 0x1337baadf00d );
  var newSrc = newURI("file:///poo/MyTrack.mp3");
  testSet( item, "contentSrc", newSrc);
  testSet( item, "contentLength", 0xbaadf00d );
  testSet( item, "contentType", "x-media/x-poo" );
  
  item.setProperty("newpropertyneverseenever", "valuevalue");
  
  // Open up a temp file through this interface
  var ioItem = testlib.createMediaItem( newTempURI("test_mediaitem") );
  assertNotEqual(ioItem, null);
  var outputStream = ioItem.openOutputStream();
  assertNotEqual(outputStream, null);
  
  // Write some junk to it and close.
  var count = outputStream.write( TEST_STRING, TEST_STRING.length );
  assertEqual(count, TEST_STRING.length);
  outputStream.flush();
  outputStream.close();
  
  // Reopen it for input, and see if we read the same junk back.
  var inputStream = ioItem.openInputStream().QueryInterface(Components.interfaces.nsILineInputStream);
  assertNotEqual(inputStream, null);
  var out = {};
  inputStream.readLine( out );
  assertEqual(out.value, TEST_STRING);
  inputStream.close();
  
  // Async test, pauses the test system.
  testAsyncRead(ioItem);
  // This passes control to testIsAvailable() when it completes.
}

