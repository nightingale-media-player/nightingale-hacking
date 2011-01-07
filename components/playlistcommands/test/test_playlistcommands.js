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
 * \brief Playlist Commands Unit Test File
 */

// ============================================================================
// GLOBALS & CONSTS
// ============================================================================

const NUMRECURSE = 2;
const TESTLENGTH = 500;

const PlaylistCommandsBuilder = new Components.
  Constructor("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1",
              "sbIPlaylistCommandsBuilder");

// ============================================================================
// ENTRY POINT
// ============================================================================

function runTest () {

  log("Testing Playlist Commands:");
  gTestPrefix = "PlaylistCommandsBuilder";
  testCommandsBuilder();
  log("OK");
}

// ============================================================================
// TESTS
// ============================================================================

function testCommandsBuilder() {

  var builder = new PlaylistCommandsBuilder();

  testAppendInsertRemove(builder, null, TESTLENGTH, 0);
  testCommandCallbacksAndShortcuts(builder, null, TESTLENGTH, 0);
  testSubmenus(builder, null, NUMRECURSE, 0);

  builder.shutdown();
}
