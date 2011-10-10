/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test file
 */

var gFileMetadataService;
var count = 0;
var gServer;

function runTest () {
  var NUM_JOBS = 10;
  var NUM_ITEMADD_LOOPS = 2;

  var port = getTestServerPortNumber();

  gServer = Cc["@mozilla.org/server/jshttp;1"].createInstance(Ci.nsIHttpServer);

  gServer.start(port);
  gServer.registerDirectory("/", getFile("."));

  var prefix = "http://localhost:" + port + "/files/";

  var library = createNewLibrary( "test_metadatajob" );

  var urls = [
    "file:///media/sdb1/steve/old/steve/fakemp3s/Sigur%20R%C3%B3s/%C3%81g%C3%A6tis%20Byrjun/1%20Intro.mp3",
    "file:///media/sdb1/steve/old/steve/fakemp3s/Sigur%20R%C3%B3s/%C3%81g%C3%A6tis%20Byrjun/2%20Svefn-G-Englar.mp3",
    "file:///media/sdb1/steve/old/steve/fakemp3s/Sigur%20R%C3%B3s/%C3%81g%C3%A6tis%20Byrjun/3%20Star%C3%A1lfur.mp3",
    "file:///media/sdb1/steve/old/steve/fakemp3s/Sigur%20R%C3%B3s/%C3%81g%C3%A6tis%20Byrjun/4%20Flugufrelsarinn.mp3",
    "file:///media/sdb1/steve/old/steve/fakemp3s/Sigur%20R%C3%B3s/%C3%81g%C3%A6tis%20Byrjun/5%20Ny%20Batteri.mp3",
    prefix + "MP3_ID3v1.mp3",
    prefix + "MP3_ID3v23.mp3",
    prefix + "FLAC.flac"
  ];

  gFileMetadataService = Components.classes["@getnightingale.com/Nightingale/FileMetadataService;1"]
                                   .getService(Components.interfaces.sbIFileMetadataService);

  for (var i = 0; i < NUM_JOBS; i++) {
    var a = Components.classes["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                      .createInstance(Components.interfaces.nsIMutableArray);
    for (var k = 0; k < NUM_ITEMADD_LOOPS; k++) {
      for (var j = 0; j < urls.length; j++) {
        a.appendElement(library.createMediaItem(newURI(urls[j]), null, true), false);
      }
    }

    var job = gFileMetadataService.read(a);

    // Set an observer to know when we complete
    job.addJobProgressListener( onComplete );
    count++;
    log("length = " + a.length + " count = " + count);
  }

  testPending();

}

function onComplete(job) {
  if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
    return;
  }

  job.removeJobProgressListener(onComplete);

  log("done = " + count);
  count--;
  if (count == 0) {
    gServer.stop();
    testFinished(); // Complete the testing
  }
}

function getFile(fileName) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  file.append("testharness");
  file.append("metadatamanager");
  file.append(fileName);
  return file;
}
