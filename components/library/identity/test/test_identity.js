/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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
 * \brief Identity Service Unit Test File
 */

// ============================================================================
// GLOBALS & CONSTS
// ============================================================================

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

const SEPARATOR = '|';
const CONTENT_TYPES = ['audio', 'video'];

var gTestLibrary = null;
var gIdentityService = null;
var gTestData = {
  "NoTags": { trackName: "",
              artistName: "",
              albumName: "",
              genre: "",
              producerName: "",
              audioMediaItem: null,
              videoMediaItem: null,
              audioExpectedIdentity: null,
              videoExpectedIdentity: null},
  "OnlyName": { trackName: "OnlyAName",
                artistName: "",
                albumName: "",
                genre: "",
                producerName: "",
                audioMediaItem: null,
                videoMediaItem: null,
                audioExpectedIdentity: "edW9P0emhROCprpy6YdiXg==",
                videoExpectedIdentity: "8DHNDFkX6afo03hmmInR6w==" },
  "Tagged": { trackName: "A fully tagged track",
              artistName: "with an artist name",
              albumName: "and an album name",
              genre: "and even a genre",
              producerName: "some producer",
              audioMediaItem: null,
              videoMediaItem: null,
              audioExpectedIdentity: "4C9SB69gL6pB/NZ/2UQbsw==",
              videoExpectedIdentity: "gPL0UOMSMv08yHcuLA+peg==" },
  "ForeignChar": { trackName: "太陽",
                   artistName: "理多",
                   albumName: "moment 〜もぉめんと〜",
                   genre: "Game",
                   producerName: "まちあわせ",
                   audioMediaItem: null,
                   videoMediaItem: null,
                   audioExpectedIdentity: "1giivr/B90byHVMftVlFmQ==",
                   videoExpectedIdentity: "Ki4O6mqLlA+cNCHutxjVOw=="}
}

// ============================================================================
// ENTRY POINT
// ============================================================================

function runTest () {
  log("Testing Identity Service:");

  // Setup our globals
  gIdentityService = Cc["@songbirdnest.com/Songbird/IdentityService;1"]
                       .getService(Ci.sbIIdentityService);
  gTestLibrary = createLibrary("test-identity", null, false);

  /* Create audio and video mediaItems for our test data and attach those
   * mediaitems to our test data objects */
  for (var dataName in gTestData) {
    var currData = gTestData[dataName];

    CONTENT_TYPES.forEach( function (contentType) {
      var props =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
      props.appendProperty(SBProperties.trackName, currData.trackName);
      props.appendProperty(SBProperties.artistName, currData.artistName);
      props.appendProperty(SBProperties.albumName, currData.albumName);
      props.appendProperty(SBProperties.genre, currData.genre);
      props.appendProperty(SBProperties.producerName, currData.producerName);
      props.appendProperty(SBProperties.contentType, contentType);
      var newItem = gTestLibrary.createMediaItem
                                (newURI("http://foo.test.com/" +
                                        dataName + "_" + contentType),
                                 props);
      currData[contentType + "MediaItem"] = newItem;
    });
  }

  testHashString();
  testCalculateIdentity();
  testSaveAndGetItemWithSameIdentity();
  gTestLibrary.clear();
  log("OK");
}

// ============================================================================
// TESTS
// ============================================================================

function testHashString() {
  log("Testing sbIIdentityService.hashString...");

  /* First check that our hashing mechanism is working properly for a string
   * with a known hash. */
  var testString = "hello, world";
  var expectedHash = "5NfxtO0uQtFYmPSyewGdpA==";
  var actualHash = gIdentityService.hashString(testString);

  assertEqual(actualHash, expectedHash);

  /* Next we check that we can get the expected identity when we manually put
   * together the metadata string that is hashed to form the identity.
   * This assumes that the metadata string, before it is hashed to calculate
   * the identity, is of the form:
   *    <contentType>|<trackName>|<artistName>|<albumName>|<genre>
   * and that mediaitems without any of the relevant metadata are not given
   * an identity.
   */
  for (var dataName in gTestData) {
    var currData = gTestData[dataName];

    /* The contentType is the first property used in the metadataString that
     * we hash, so we need to handle them individually */
    CONTENT_TYPES.forEach( function (contentType) {
      var identityProperties = [];
      identityProperties.push(contentType);
      identityProperties.push(currData.trackName);
      identityProperties.push(currData.artistName);
      identityProperties.push(currData.albumName);
      identityProperties.push(currData.genre);

      var metadataString = identityProperties.join(SEPARATOR);

      /* We'll add identityProperties.length - 1 separators to the contentType
       * so we only know that we have metadata to hash if the string is longer
       * than that */
      var hasMetadata = (metadataString.length >
                         (contentType.length + identityProperties.length - 1));

      var expectedMetadataHash = currData[contentType + "ExpectedIdentity"];

      /* If we have metadata to hash, hash it and we'll compare that string.
       * If we don't have metadata, though, the identity service will not give
       * an identity so be sure that we expected null. */
      var actualMetadataHash = ((hasMetadata) ?
                                gIdentityService.hashString(metadataString) :
                                null);
      assertEqual(actualMetadataHash, expectedMetadataHash);
    });
  }

}

function testCalculateIdentity() {
  log("Testing sbIIdentityService.calculateIdentity...");

  /* We will check that we calculate identities properly for our test data items
   * with each of the contentTypes */
  for (var dataName in gTestData) {
    var currData = gTestData[dataName];

    CONTENT_TYPES.forEach( function (contentType) {
      // Check that we calculate the identity correctly for the test data item
      var testMediaItem = currData[contentType + "MediaItem"];
      var expectedHash = currData[contentType + "ExpectedIdentity"];
      var actualMediaItemHash = gIdentityService.calculateIdentityForMediaItem
                                               (testMediaItem);
      assertEqual(actualMediaItemHash, expectedHash);

      /* Check that changing an irrelevant metadata field does not change the
       * identity.  producerName is not used in the identity calculation */
      testMediaItem.setProperty(SBProperties.producerName, "A new producer!");
      var irrelevantChangeHash =
        gIdentityService.calculateIdentityForMediaItem(testMediaItem);
      assertEqual(irrelevantChangeHash, expectedHash);

      /* Check that changing a relevant metadata field changes the identity.
       * We change to the otherContentType because we already know what the
       * identity should be for this data with that contentType */
      var otherContentType = (contentType == "video") ? "audio" : "video";
      testMediaItem.setProperty(SBProperties.contentType, otherContentType);

      var actualContentTypeSwitchHash =
        gIdentityService.calculateIdentityForMediaItem(testMediaItem);

      var expectedContentTypeSwitchHash =
        currData[otherContentType + "ExpectedIdentity"];
      assertEqual(actualContentTypeSwitchHash, expectedContentTypeSwitchHash);

      // Change the contentType back and make sure everything is still good
      testMediaItem.setProperty(SBProperties.contentType, contentType);
      actualMediaItemHash = gIdentityService.calculateIdentityForMediaItem
                                            (testMediaItem);
      assertEqual(actualMediaItemHash, expectedHash);

    });
  }
}

function  testSaveAndGetItemWithSameIdentity() {
  log("Testing sbIIdentityService saving and retrieving items by identity...");
  var secondaryLibrary = createLibrary("test-identity-secondary", null , false);

  /* First we'll try to get an item with the same identity as each of our test
   * data items.  We'll try to get them from the secondary library, which
   * doesn't have anything in it yet, so those should not return anything.
   *
   * Then, we try to get an item with the same identity from gTestLibrary.
   * Though gTestLibrary holds the test data items, it should still not return
   * anything because we screen out items with the same guid as the param item.
   *
   * Last, we add a new item to the secondaryLibrary with the same metadata
   * as the test data items.  Then we search for items with the same identity
   * as our test data item in the secondLibrary and ensure that it retrieves
   * the newly created item.  This is then repeated vice-versa in gTestLibrary
   */
  var finishedCount = 0;
  for (var dataName in gTestData) {
    var currData = gTestData[dataName];

    CONTENT_TYPES.forEach( function (contentType) {
      var testMediaItem = currData[contentType + "MediaItem"];
      var expectedHash = currData[contentType + "ExpectedIdentity"];

      /* If the expectedHash is null then we didn't have enough information to
       * generate an identity, so we'll get less predictable results because
       * items in the test data have the same identity (i.e. no identity).
       * We just won't worry about those items here */
      if (expectedHash != null) {
        // secondaryLibrary shouldn't have any identity matches, yet
        var found = secondaryLibrary.containsItemWithSameIdentity(testMediaItem);
        assertTrue(!found);

        /* gTestLibrary has something with the same identity, testMediaItem, but
         * we screen out items with the same guid so we shoudln't find anything
         * here */
        found = gTestLibrary.containsItemWithSameIdentity(testMediaItem);
        assertTrue(!found);

        /* Now we'll insert an item with the same metadata into the secondary
         * library and then look for an item with the same identity as the one in
         * our test data.  We should find the created item. */
        var props =
          Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
            .createInstance(Ci.sbIMutablePropertyArray);
        props.appendProperty(SBProperties.trackName, currData.trackName);
        props.appendProperty(SBProperties.artistName, currData.artistName);
        props.appendProperty(SBProperties.albumName, currData.albumName);
        props.appendProperty(SBProperties.genre, currData.genre);
        props.appendProperty(SBProperties.producerName, currData.producerName);
        props.appendProperty(SBProperties.contentType, contentType);
        var secondaryItem = secondaryLibrary.createMediaItem
                                      (newURI("http://foo.secondarytest.com/" +
                                              dataName + "_" + contentType),
                                       props);

        /* Look for an item with the same identity as testMediaItem.  We just
         * created an item with the same metadata, so we should find one */
        found = secondaryLibrary.containsItemWithSameIdentity(testMediaItem);
        assertTrue(found);

        /* The same should be true for the reverse, looking for an item with
         * the same identity as secondaryItem in gTestLibrary */
        found = gTestLibrary.containsItemWithSameIdentity(secondaryItem);
        assertTrue(found);

        // Ensure that we can find items in a library with an identity string
        var hashPropID = SBProperties.metadataHashIdentity;
        var testIdentity = testMediaItem.getProperty(hashPropID);
        var foundItems = null;
        try {
          foundItems = gTestLibrary.getItemsByProperty(hashPropID,
                                                       testIdentity);
        }
        catch (e) {
          if (e.result == Components.results.NS_ERROR_NOT_AVAILABLE) {
            // We didn't find anything. Ignore the exception and let the test
            // fail with the next assertion so we get a more informative failure
            // message.
          }
        }

        assertTrue(foundItems.length == 1,
            'could not find item using identity and getItemsByProperty()');

        /* We verified that we found items with the same identity, make sure
         * we retrieve the items that we expect */
        var foundItems = secondaryLibrary.getItemsWithSameIdentity(testMediaItem);
        assertEqual(foundItems.length, 1);
        assertEqual(foundItems.queryElementAt(0, Ci.sbIMediaItem).guid, secondaryItem.guid);

        foundItems = gTestLibrary.getItemsWithSameIdentity(secondaryItem);
        assertEqual(foundItems.length, 1);
        assertEqual(foundItems.queryElementAt(0, Ci.sbIMediaItem).guid, testMediaItem.guid);

        /* Lastly test that we can manually submit an identity using
         * SaveIdentityToMediaItem. */
        gIdentityService.saveIdentityToMediaItem(secondaryItem, "finished");
        var foundItems = secondaryLibrary.getItemsWithSameIdentity(testMediaItem);
        assertEqual(foundItems.length, 0);
      }
    });
  }

  secondaryLibrary.clear();
}
