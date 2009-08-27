/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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
 * \brief Test the metadata lookup manager and FreeDB provider
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

var gMLM;
var gMockSvc;
var gMockController;

var testListener = {
  // sbICDDeviceListener
  onMediaInserted: function(cdDevice) {
    assertEqual(cdDevice.isDiscInserted, true,
                "Expected CD device to be inserted");

    // get the FreeDB provider
    var provider = gMLM.getProvider("TestProvider");

    // lookup the disc
    var job = provider.queryDisc(cdDevice.discTOC);
    // if the job was already finished, then just trigger the listener
    // otherwise add the listener and wait
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED)
      this.onJobProgress(job);
    else
      job.addJobProgressListener(this);
  },

  // sbIJobProgressListener
  onJobProgress: function(job) {
    if (job.status != Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED)
      return;

    // output the # of results found
    var numResults = job.mlNumResults;
    assertEqual(numResults > 0, true,
        "Expected at least 1 result.  Got: " + numResults);

    log("Got " + numResults + " results");
    // enumerate the various results
    var enum = job.getMetadataResults();
    while (enum.hasMoreElements()) {
      var a = enum.getNext().QueryInterface(
                      Components.interfaces.sbIMetadataAlbumDetail);
      log("This album has " + a.tracks.length + " tracks");
      var albumName = a.properties.getPropertyValue(SBProperties.albumName);
      assertEqual(albumName, "Midnight Rock Disc 1",
                  "Expected album name to be 'Midnight Rock Disc 1', actual " +
                  "was: " + albumName);
      var artistName = a.properties.getPropertyValue(SBProperties.artistName);
      assertEqual(artistName, "Various",
                  "Expected artist name to be 'Various', actual was: " +
                  artistName);
      assertEqual(a.tracks.length, 15, "Expected 15 tracks");
    }

    // eject the disc
    gMockController.ejectMedia(testListener.device,
         Components.interfaces.sbICDMockDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK);

    testFinished();
  }
}

function runTest () {
  // Get the media lookup manager and mock CD service/controller
  gMLM = Components.classes["@songbirdnest.com/Songbird/MetadataLookup/manager;1"].getService(Components.interfaces.sbIMetadataLookupManager);
  gMockSvc = Components.classes["@songbirdnest.com/device/cd/mock-cddevice-service;1"].getService(Components.interfaces.sbICDDeviceService);
  gMockController = Components.classes["@songbirdnest.com/device/cd/mock-cddevice-service;1"].getService(Components.interfaces.sbIMockCDDeviceController);

  gMockSvc.registerListener(testListener);
  var cd0 = gMockSvc.getDevice(0);

  testListener.device = cd0;
  // insert "All That You Can't Leave Behind"
  gMockController.insertMedia(cd0,
         Components.interfaces.sbICDMockDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK);

  testPending();
}

