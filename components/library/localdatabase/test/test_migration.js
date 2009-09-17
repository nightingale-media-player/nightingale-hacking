/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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
 * \brief Test file to ensure that the version information between the schema
 * and the Migration Helper are the same.
 */

/**
 * \brief Get a file from the localdatabase folder under the application.
 * \param aFileName - String name of file to get.
 */
function getFile(aFileName) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  file.append("chrome");
  file.append("songbird");
  file.append("content");
  file.append("songbird");
  file.append("library");
  file.append("localdatabase");
  file.append(aFileName);
  return file;
}

/**
 * \brief Reads in the schema.sql file and parses out the library_metadata
 * version information.
 * \returns int value of schema version, -1 if unable to find.
 */
function getLatestSchemaVersionFromFile() {
  var schemaVersion = -1;
  
  var schemaFile = getFile("schema.sql");
  assertEqual(schemaFile.exists(),
              true,
              "Schema file not found: " + schemaFile.path);
  
  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(schemaFile, 0x01, 0444, 0);
  istream.QueryInterface(Ci.nsILineInputStream);
  
  // Now read the file line by line and find the version
  //insert into library_metadata (name, value) values ('version', '21');
  var hasmore = true;
  var versionLineReg = /^insert into library_metadata/;
  do {
    var line = {};
    hasmore = istream.readLine(line);
    if (versionLineReg.test(line.value)) {
      schemaVersion = parseInt(line.value.match(/\'\d*\'/g)[0]
                                         .replace(/\'/g, ""));
    }
  } while(hasmore && (schemaVersion == -1));
  istream.close();

  return schemaVersion;
}

function runTest() {

  var databaseGUID = "test_migration";
  var library = createLibrary(databaseGUID);
  
  var migrationHelper = 
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/MigrationHelper;1"]
      .createInstance(Ci.sbILocalDatabaseMigrationHelper);

  var latestSchemaVersion = getLatestSchemaVersionFromFile();
      
  assertEqual(migrationHelper.getLatestSchemaVersion(), 
              latestSchemaVersion,
              "Error: Schema Version incorrect. Please ensure that the schema version in schema.sql matches the version in the Migration Helper!");
  
  assertEqual(migrationHelper.canMigrate(1, 1000), false);
  assertEqual(migrationHelper.canMigrate(5, 6), true);
}
