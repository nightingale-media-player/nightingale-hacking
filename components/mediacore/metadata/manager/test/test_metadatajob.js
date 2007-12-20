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
 * \brief Test file
 */

// The local and remote urls point to the same file, to be tested by the given matrix
var gLocalFiles = [
  "testharness/metadatamanager/test1.mp3",
  "testharness/metadatamanager/test2.mp3",
  "testharness/metadatamanager/test3.mp3"
];
var gLocalMediaItems = [];

var PORT_NUMBER = getTestServerPortNumber();

var gRemoteUrls = [
  <>http://localhost:{PORT_NUMBER}/test1.mp3</>,
  <>http://localhost:{PORT_NUMBER}/test2.mp3</>,
  <>http://localhost:{PORT_NUMBER}/test3.mp3</>
];
var gRemoteMediaItems = [];

var gTestMatrix = [
  [ "http://songbirdnest.com/data/1.0#artistName", "Chrysostomos" ],  
  [ "http://songbirdnest.com/data/1.0#trackName", "Trisagion" ],                
  [ "http://songbirdnest.com/data/1.0#albumName", "The Singing Dictionary" ]        
];

var gTestMetadataJobManager = null;
var gTestMetadataJob = null;

var gTestInterval = null;

var gNumTestItems = gLocalFiles.length;

var gServer;


function runTest () {
  var gTestLibrary = createNewLibrary( "test_metadatajob" );
  var gTestMediaItems = Components.classes["@mozilla.org/array;1"]
                                  .createInstance(Components.interfaces.nsIMutableArray);

  gServer = Cc["@mozilla.org/server/jshttp;1"]
              .createInstance(Ci.nsIHttpServer);

  gServer.start(PORT_NUMBER);
  var file = newAppRelativeFile(gLocalFiles[0]).parent;
  gServer.registerDirectory("/", file);

  // Make sure you didn't screw up your data entry.
  assertEqual( gNumTestItems, gRemoteUrls.length );
  assertEqual( gNumTestItems, gTestMatrix.length );

  for ( var i = 0; i < gNumTestItems; i++ )
  {
    // Add gLocalFiles to it
    var localPath = newAppRelativeFile( gLocalFiles[ i ] );
    assertNotEqual( localPath, null );
    var localPathURI = newFileURI( localPath );
    assertNotEqual( localPathURI, null );
    var localPathMI = gTestLibrary.createMediaItem( localPathURI );
    assertNotEqual( localPathMI, null );
    gLocalMediaItems.push( localPathMI );
    gTestMediaItems.appendElement( localPathMI, false );
/*    
*/    
    // Add gRemoteUrls to it
    var remotePathURI = newURI( gRemoteUrls[ i ] );
    assertNotEqual( remotePathURI, null );
    var remotePathMI = gTestLibrary.createMediaItem( remotePathURI );
    assertNotEqual( remotePathMI, null );
    gRemoteMediaItems.push( remotePathMI );
    gTestMediaItems.appendElement( remotePathMI, false );
  }
  // Request metadata for both local and remote urls at the same time.  Woo!
  gTestMetadataJobManager = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                                .getService(Components.interfaces.sbIMetadataJobManager);
  gTestMetadataJob = gTestMetadataJobManager.newJob( gTestMediaItems, 5 );                                

  var gTestObserver = new MetadataJobObserver(onComplete);
  // Set an observer to know when we complete
  gTestMetadataJob.setObserver( gTestObserver );
  testPending();
}

function MetadataJobObserver(completeFunc) {
  this._completeFunc = completeFunc;
}

MetadataJobObserver.prototype = {
  observe: function(aSubject, aTopic, aData)
  {
    this._completeFunc.call(this, aSubject, aTopic, aData);
  }
}

function onComplete(aSubject, aTopic, aData) {
  gTestMetadataJob.removeObserver();

  // Are you really complete?
  assertEqual( aTopic, "complete" );
  assertEqual( aData, gTestMetadataJob.tableName );
  assertTrue( gTestMetadataJob.completed );
  
  // Debug output.  Output everything before testing anything so we can see
  // the full set of data instead of quitting on the first error.
  for ( var i = 0; i < gNumTestItems; i++ )
  {
    var property = gTestMatrix[ i ][ 0 ];
    var value = gTestMatrix[ i ][ 1 ];
    var local = null, remote = null;
    try {
      local = gLocalMediaItems[ i ].getProperty( property );
    } catch (e) { log( e ); }
    try {
      remote = gRemoteMediaItems[ i ].getProperty( property );
    } catch (e) { log( e ); }
    log( property + " -- test:" + value + " ?= local:" + local + " ?= remote:" + remote );
  }
  
  // Compare the values from the items to the expected results in gTestMatrix
  for ( var i = 0; i < gNumTestItems; i++ )
  {
    var property = gTestMatrix[ i ][ 0 ];
    var value = gTestMatrix[ i ][ 1 ];
    var local = null, remote = null;
    try {
      local = gLocalMediaItems[ i ].getProperty( property );
    } catch (e) {}
    try {
      remote = gRemoteMediaItems[ i ].getProperty( property );
    } catch (e) {}
    assertEqual( value, local );
    assertEqual( value, remote );
  }

  // So testing is complete
  gTestMetadataJobManager = null;
  gTestMetadataJob = null;
  
  gServer.stop();
  
  testFinished(); // Complete the testing
}
