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
  
  this.fromVersion = 13;
  this.toVersion   = 14;
  this.versionString = this.fromVersion + " to " + 
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version 13 to 14',
  classID: Components.ID("{cd3b44bf-1f72-4006-aa06-2f00fc0948bb}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + ' 13 to 14',

  _databaseLocation: null,
  _databaseGUID: null,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try{

      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      var query = this._createQuery();

      // Update the schema version to the destination version.
      query.addQuery("update library_metadata set value = '" 
                     + this.toVersion + "' where name = 'version'");
      query.addQuery("commit");

      // Try to reduce db size
      query.addQuery("VACUUM");

      var retval;
      query.setAsyncQuery(true);
      query.execute(retval);

      // Raise a flag indicating that this library will need all 
      // sort info to be recomputed.
      // Normally we'd call propertyCache.invalidateSortData(), but 
      // at this point in startup the property cache does not exist yet.
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
  },

  _createQuery: function sbLibraryMigration_createQuery() {
    var query = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
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

