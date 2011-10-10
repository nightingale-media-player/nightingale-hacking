/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \brief Test the metadata lookup manager with the test provider
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

var gMLM;
var gMockSvc;
var gMockController;

var testListener = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.sbICDDeviceListener,
    Ci.sbIJobProgressListener
  ]),

  // sbICDDeviceListener
  onMediaInserted: function(cdDevice) {
    assertEqual(cdDevice.isDiscInserted, true,
                "Expected CD device to be inserted");

    // get the test provider
    var provider = gMLM.getProvider("TestProvider");

    // lookup the disc
    var job = provider.queryDisc(cdDevice.discTOC);
    // if the job was already finished, then just trigger the listener
    // otherwise add the listener and wait
    if (job.status == Ci.sbIJobProgress.STATUS_SUCCEEDED)
      this.onJobProgress(job);
    else
      job.addJobProgressListener(this);
  },

  onDeviceRemoved: function(cdDevice) {},
  onMediaEjected: function(cdDevice) {},

  // sbIJobProgressListener
  onJobProgress: function(job) {
    if (job.status != Ci.sbIJobProgress.STATUS_SUCCEEDED)
      return;

    // The check below does an implicit QueryInterface call, it is necessary
    // before accessing sbIMetadataLookupJob properties
    assertEqual(job instanceof Ci.sbIMetadataLookupJob, true,
                "Expected sbIMetadataLookupJob instance. Got: " + job);

    // output the # of results found
    var numResults = job.mlNumResults;
    assertEqual(numResults > 0, true,
        "Expected at least 1 result.  Got: " + numResults);

    log("Got " + numResults + " results");
    // enumerate the various results
    var results = job.getMetadataResults();
    while (results.hasMoreElements()) {
      var a = results.getNext().QueryInterface(Ci.sbIMetadataAlbumDetail);
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
         Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK);

    job.removeJobProgressListener(this);
    gMockSvc.removeListener(this);
    testFinished();
  }
}

function runTest () {
  // Get the media lookup manager and mock CD service/controller
  gMLM = Cc["@getnightingale.com/Nightingale/MetadataLookup/manager;1"]
           .getService(Ci.sbIMetadataLookupManager);
  gMockSvc = Cc["@getnightingale.com/device/cd/mock-cddevice-service;1"]
               .getService(Ci.sbICDDeviceService);
  gMockController = Cc["@getnightingale.com/device/cd/mock-cddevice-service;1"]
                      .getService(Ci.sbIMockCDDeviceController);

  gMockSvc.registerListener(testListener);
  var cd0 = gMockSvc.getDevice(0);

  testListener.device = cd0;
  // insert "All That You Can't Leave Behind"
  gMockController.insertMedia(cd0,
         Ci.sbIMockCDDeviceController.MOCK_MEDIA_DISC_MIDNIGHT_ROCK);

  testPending();
}

