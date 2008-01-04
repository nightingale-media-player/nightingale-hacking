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

function runTest () {

  Components.utils.import("resource://app/components/sbProperties.jsm");

  var library = createLibrary("test_guid_property", null, false);
  library.clear();

  var item = library.createMediaItem(newURI("http://foo.com/"));

  assertEqual(item.guid, item.getProperty(SBProperties.GUID));
  var properties = item.getProperties();
  assertEqual(item.guid, properties.getPropertyValue(SBProperties.GUID));

  try {
    item.setProperty(SBProperties.GUID, "foo");
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  try {
    item.setProperties(SBProperties.createArray([
      [SBProperties.GUID, "foo"]
    ]));
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  // Passing in a guid as a parameter to create should not do anything bad
  var item2 = library.createMediaItem(newURI("http://foo.com/"),
                                      SBProperties.createArray([
                                        [SBProperties.GUID, "foo"]
                                      ]));
  assertTrue(item2.guid != "foo");

  var listener = {
    _item: null,
    onEnumerationBegin: function(aMediaList) {
    },
    onEnumeratedItem: function(aMediaList, aMediaItem) {
      this._item = aMediaItem;
    },
    onEnumerationEnd: function(aMediaList, aStatusCode) {
    }
  };

  library.enumerateItemsByProperty(SBProperties.GUID,
                                   item.guid,
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertTrue(listener._item.equals(item));
}

