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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const FROM_VERSION = 25;
const TO_VERSION = 26;

function LOG(s) {
  dump("----++++----++++sbLibraryMigration " +
       FROM_VERSION + " to " + TO_VERSION + ": " +
       s +
       "\n----++++----++++\n");
}

function sbLibraryMigration()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  this._errors = [];
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version ' +
                     FROM_VERSION + ' to ' + TO_VERSION,
  classID: Components.ID("{0f5c7861-873d-419e-9de7-8a1d157050b0}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID +
              FROM_VERSION + 'to' + TO_VERSION,

  fromVersion: FROM_VERSION,
  toVersion: TO_VERSION,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try {
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Run a query that will mark the library as migrated
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("commit");
      var retval;
      query.setAsyncQuery(false);
      query.execute(retval);

      //
      // Raise a flag indicating that this library will need all
      // sort info to be recomputed.
      //
      // Normally we'd call propertyCache.invalidateSortData(), but
      // at this point in startup the property cache does not exist yet.
      //
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("songbird.propertycache." +
            this._databaseGUID + ".invalidSortData", true);
      prefs.QueryInterface(Ci.nsIPrefService).savePrefFile(null);
    }
    catch (e) {
      dump("Exception occured: " + e);
      throw e;
    }
  }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

