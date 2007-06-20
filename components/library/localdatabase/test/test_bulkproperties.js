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

function runTest () {

  Components.utils.import("resource://app/components/sbProperties.jsm");

  var SB_NS = "http://songbirdnest.com/data/1.0#";

  var databaseGUID = "test_bulkproperties";
  var library = createLibrary(databaseGUID, null, false);

  var listener = new TestMediaListListener();
  library.addListener(listener);

  var uri = "http://floo.com/";
  var item = library.createMediaItem(newURI(uri));
  var props = item.getProperties(null);

  var originalPropCount = props.length;
  assertTrue(props.length > 1);
  assertEqual(props.getPropertyValue(SB_NS + "contentURL"), uri);

  props = item.getProperties(SBProperties.createArray([
    [SBProperties.contentURL, uri],
    [SBProperties.hidden, "0"]
  ]));

  assertEqual(props.length, 2);
  assertEqual(props.getPropertyValue(SB_NS + "contentURL"), uri);
  assertEqual(props.getPropertyValue(SB_NS + "hidden"), "0");

  var a = SBProperties.createArray([
    [SBProperties.albumName, "album name"],
    [SBProperties.artistName, "artist name"]
  ]);

  listener.reset();
  item.setProperties(a);
  item.write();

  props = item.getProperties(null);
  assertEqual(props.length, originalPropCount + 2);
  assertEqual(props.getPropertyValue(SB_NS + "contentURL"), uri);
  assertEqual(props.getPropertyValue(SB_NS + "albumName"), "album name");
  assertEqual(props.getPropertyValue(SB_NS + "artistName"), "artist name");

  var changedProps = listener.updatedProperties;
  assertEqual(changedProps.length, 2);
  assertEqual(changedProps.getPropertyValue(SB_NS + "albumName"), "");
  assertEqual(changedProps.getPropertyValue(SB_NS + "artistName"), "");

  a = SBProperties.createArray([
    [SBProperties.albumName, "album name2"],
    [SBProperties.artistName, "artist name2"]
  ]);

  listener.reset();
  item.setProperties(a);
  item.write();

  props = item.getProperties(null);
  assertEqual(props.length, originalPropCount + 2);
  assertEqual(props.getPropertyValue(SB_NS + "contentURL"), uri);
  assertEqual(props.getPropertyValue(SB_NS + "albumName"), "album name2");
  assertEqual(props.getPropertyValue(SB_NS + "artistName"), "artist name2");

  changedProps = listener.updatedProperties;
  assertEqual(changedProps.length, 2);
  assertEqual(changedProps.getPropertyValue(SB_NS + "albumName"), "album name");
  assertEqual(changedProps.getPropertyValue(SB_NS + "artistName"), "artist name");

  // TODO: Add a test for setting a property to null

  // Try an invalid value
  a = SBProperties.createArray([
    [SBProperties.trackNumber, "invalid"]
  ]);
  try {
    item.setProperties(a);
    fail("Exception not thrown");
  }
  catch (e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }
}

