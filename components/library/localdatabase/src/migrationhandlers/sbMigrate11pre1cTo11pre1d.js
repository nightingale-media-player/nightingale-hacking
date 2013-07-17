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
  
  this.fromVersion = 15;
  this.toVersion   = 16;
  this.versionString = this.fromVersion + " to " + 
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler, version 15 to 16',
  classID: Components.ID("{bcc617e0-1dd1-11b2-b34d-d3db6eea6e70}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + '15to16',

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

      // Raise a flag indicating that this library will need the indices
      // dependent on the 'library_collate' collation sequence to be reindexed
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("songbird.databaseengine." + 
            this._databaseGUID + ".invalidCollationIndex", true);
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

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

