/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLocalDatabaseMigrationUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(s) {
  dump("----++++----++++sbLibraryMigration " +
       sbLibraryMigration.versionString + ": " +
       s +
       "\n----++++----++++\n");
}

function sbLibraryMigration()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);

  this.fromVersion = 18;
  this.toVersion   = 19;
  this.versionString = this.fromVersion + " to " +
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Nightingale Migration Handler, version 18 to 19',
  classID: Components.ID("{0dc77be1-9a74-4d19-8a11-15fa3a9dcad0}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + ' 18 to 19',

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try{

      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      // Run a query that will mark the library as migrated
      var query = this.createMigrationQuery(aLibrary);
      query.addQuery("commit");
      var retval;
      query.setAsyncQuery(false);
      query.execute(retval);

      // Raise a flag indicating that this library will need all
      // sort info to be recomputed.
      // Normally we'd call propertyCache.invalidateSortData(), but
      // at this point in startup the property cache does not exist yet.
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("nightingale.propertycache." +
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
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLibraryMigration
  ]);
}

