/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
  dump("----++++----++++\sbLocalDatabaseMigrate070to08pre1 ---> " + 
       s + 
       "\n----++++----++++\n");
}

function sbLocalDatabaseMigrate070to08pre1()
{
  SBLocalDatabaseMigrationUtils.BaseMigrationHandler.call(this);
  
  this.fromVersion = 8;
  this.toVersion   = 9;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLocalDatabaseMigrate070to08pre1.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,

  classDescription: 'Nightingale Migration Handler for 0.7.0 to 0.8.0 pre with no content hash',
  classID: Components.ID("{b080f650-7ef2-11dd-ad8b-a0800200c9a6}"),
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "0.7.0 to 0.8.0 pre1",
  
  _databaseLocation: null,
  _databaseGUID: null,
  
  _mediaItems: null,
  
  migrate: function sbLDBM070to08pre1_migrate(aLibrary) {
    this._databaseGUID = aLibrary.databaseGuid;
    this._databaseLocation = aLibrary.databaseLocation;

    var query = this._createQuery();
    query.addQuery("begin");

    // We aren't filtering with URL and hash anymore, so we don't need the index
    query.addQuery("drop index idx_media_items_content_url_content_hash");
    
    // We're no longer populating the signature property, so clear out any data in case
    // we use it in the future
    query.addQuery("update media_items set content_hash=null");    

    // Finally, we update the schema version to the destination version.
    query.addQuery("update library_metadata set value = '9' where name = 'version'");

    // Our queries are all generated. Time to execute the migration.
    query.addQuery("commit");

    var retval;
    query.setAsyncQuery(true);
    query.execute(retval);
    
    var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                .createInstance(Ci.nsISupportsInterfacePointer);
    sip.data = this;
    
    this._titleText = "Library Migration Helper";
    this._statusText = "Migrating 0.7.0 database to 0.8.0pre1 database...";
    this.migrationQuery = query;
    
    this.startNotificationTimer();
    SBJobUtils.showProgressDialog(sip.data, null, 0);
    this.stopNotificationTimer();
  },
  
  _createQuery: function sbLDBM070to08pre1_createQuery() {
    var query = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
                  .createInstance(Ci.sbIDatabaseQuery);
    query.databaseLocation = this._databaseLocation;
    query.setDatabaseGUID(this._databaseGUID);
    
    return query;
  }
}


//
// Module
// 

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbLocalDatabaseMigrate070to08pre1
  ]);
}
