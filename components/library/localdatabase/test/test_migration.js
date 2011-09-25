/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \brief Test file to ensure that the version information between the schema
 * and the Migration Helper are the same.
 */

/**
 * \brief Reads in the schema.sql file and parses out the library_metadata
 * version information.
 * \returns int value of schema version, -1 if unable to find.
 */
function getLatestSchemaVersionFromFile() {
  // the schema file may be in a jar, so we alway need to use the chrome url
  const schemaSpec = "chrome://nightingale/content/library/localdatabase/schema.sql";
  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  var schemaUri = ioService.newURI(schemaSpec, null, null);
  var channel = ioService.newChannelFromURI(schemaUri);
  var rawStream = channel.open();
  var inStream = Cc["@mozilla.org/scriptableinputstream;1"]
                   .createInstance(Ci.nsIScriptableInputStream);
  inStream.init(rawStream);
  
  var data = "", buffer;
  while ((buffer = inStream.read(~0))) {
    data += buffer;
  }
  inStream.close();
  rawStream.close();

  const versionLineReg = /^insert into library_metadata/;
  for each (var line in data.split(/\n/)) {
    if (versionLineReg.test(line)) {
      return parseInt(line.match(/'(\d+)'/)[1]);
    }
  }

  return -1;
}

function runTest() {

  var databaseGUID = "test_migration";
  var library = createLibrary(databaseGUID);
  
  var migrationHelper = 
    Cc["@getnightingale.com/Nightingale/Library/LocalDatabase/MigrationHelper;1"]
      .createInstance(Ci.sbILocalDatabaseMigrationHelper);

  var latestSchemaVersion = getLatestSchemaVersionFromFile();
      
  assertEqual(migrationHelper.getLatestSchemaVersion(), 
              latestSchemaVersion,
              "Error: Schema Version incorrect. Please ensure that the schema version in schema.sql matches the version in the Migration Helper!");
  
  assertEqual(migrationHelper.canMigrate(1, 1000), false);
  assertEqual(migrationHelper.canMigrate(5, 6), true);
}
