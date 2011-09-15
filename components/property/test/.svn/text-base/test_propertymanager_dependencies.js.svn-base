/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
 * \brief Test file
 */

function runTest () {
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
  var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                  .getService(Ci.sbIPropertyManager);

  // Sorting by album will sort by album->disc no->track no->track name
  // Sorting by artist will sort by artist->album->disc no->track no->track name
  // Sorting by albumartist will sort by albumartist->album->disc no->track no->track name
  // Sorting by genre will sort by genre->artist->album->disc no->track no->track name
  // Sorting by year will sort by year->artist->album->disc no->track no->track name
  // Sorting by track number will sort by track no->artist->album->disc no
  // Sorting by disc number will sort by disc no->artist->album->track no->track name

  // Expect track name to return album, artist
  var deps = propMan.getDependentProperties(SBProperties.trackName);
  assertPropertyArrayContains(deps, 
      [SBProperties.albumName, 
       SBProperties.artistName,
       SBProperties.albumArtistName,
       SBProperties.genre,
       SBProperties.year,
       SBProperties.discNumber]);
  
  // Expect album to return artist, albumartist, genre, year
  deps = propMan.getDependentProperties(SBProperties.albumName);
  assertPropertyArrayContains(deps, 
      [SBProperties.artistName,
       SBProperties.albumArtistName,
       SBProperties.genre,
       SBProperties.year,
       SBProperties.discNumber,
       SBProperties.trackNumber]);

  // Expect artist to return genre, year
  deps = propMan.getDependentProperties(SBProperties.artistName);
  assertPropertyArrayContains(deps, 
      [SBProperties.genre,
       SBProperties.year,
       SBProperties.discNumber,
       SBProperties.trackNumber]);

  // Expect contentURL to return nothing
  deps = propMan.getDependentProperties(SBProperties.contentURL);
  assertPropertyArrayContains(deps, []);
}


/**
 * Ensure that sbIPropertyArray aProps contains exactly
 * the properties specified in array aExpected
 */
function assertPropertyArrayContains(aProps, aExpected) {
  assertEqual(aProps.length, aExpected.length);
  aProps = SBProperties.arrayToJSObject(aProps);
  for each (var prop in aExpected) {
    assertTrue(prop in aProps);
  }
}
