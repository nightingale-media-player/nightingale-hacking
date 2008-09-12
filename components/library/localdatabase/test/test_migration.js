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

function runTest() {

  var databaseGUID = "test_migration";
  var library = createLibrary(databaseGUID);
  
  var migrationHelper = 
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/MigrationHelper;1"]
      .createInstance(Ci.sbILocalDatabaseMigrationHelper);
      
  assertEqual(migrationHelper.latestSchemaVersion , 
              9, 
              "Error: Schema Version incorrect. Please ensure that the schema version in schema.sql matches the version we're testing for!");
  
  assertEqual(migrationHelper.canMigrate(1, 1000), false);
  assertEqual(migrationHelper.canMigrate(5, 6), true);
  
  migrationHelper.migrate(5, 6, library);
}
