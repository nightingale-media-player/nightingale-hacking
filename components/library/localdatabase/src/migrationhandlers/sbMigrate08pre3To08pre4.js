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
  
  this.fromVersion = 11;
  this.toVersion   = 12;
  this.versionString = this.fromVersion + " to " + 
                       this.toVersion;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseMigration Implementation
//-----------------------------------------------------------------------------

sbLibraryMigration.prototype = {
  __proto__: SBLocalDatabaseMigrationUtils.BaseMigrationHandler.prototype,
  classDescription: 'Songbird Migration Handler for converting utf16 to utf8 storage.',
  classID: Components.ID("{E313D2F1-D1BE-4683-963F-5F43C5245C6C}"),  
  contractID: SBLocalDatabaseMigrationUtils.baseHandlerContractID + "utf16toutf8",

  _mOldLibrary: null,
  _databaseGUID: null,

  migrate: function sbLibraryMigration_migrate(aLibrary) {
    try{
      this._databaseGUID = aLibrary.databaseGuid;
      this._databaseLocation = aLibrary.databaseLocation;

      dump("Migrating " + this._databaseLocation.spec + this._databaseGUID + "\n");
      var dbParentDir = 
        this._databaseLocation.QueryInterface(Ci.nsIFileURL).file;
      var dbEngine = Cc["@songbirdnest.com/Songbird/DatabaseEngine;1"]
                       .getService(Ci.sbIDatabaseEngine);

      // Dump the existing data into a temporary file
      var oldLibDumpFile = dbParentDir.clone();
      oldLibDumpFile.append(this._databaseGUID + ".txt");
      if (oldLibDumpFile.exists()) {
        oldLibDumpFile.remove(false);
      }
      oldLibDumpFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0777);

      dbEngine.dumpDatabase(this._databaseGUID, oldLibDumpFile);

      // Close existing DB handles and move the database to a new location
      dbEngine.closeDatabase(this._databaseGUID);
   
      // Move the old database to a new temporary location
      var oldLibraryFile = dbParentDir.clone();
      oldLibraryFile.append(this._databaseGUID + ".db");
    
      // Paranoia, make sure the new temp location for the old database doesn't exist
      var tempOldLibFile = dbParentDir.clone();
      tempOldLibFile.append("old_" + oldLibraryFile.leafName);
      if (tempOldLibFile.exists()) {
        tempOldLibFile.remove(false);
      }
      oldLibraryFile.moveTo(oldLibraryFile.parent, tempOldLibFile.leafName);

      var query = this._createQuery();
      query.addQuery("begin");
      this._createLibrary(query);

      // Remove all the TABLES from the new library.
      query.addQuery("drop table if exists media_items");
      query.addQuery("drop table if exists media_list_types");
      query.addQuery("drop table if exists properties");
      query.addQuery("drop table if exists resource_properties");
      query.addQuery("drop table if exists library_metadata");
      query.addQuery("drop table if exists simple_media_lists");
      query.addQuery("drop table if exists library_media_item");
      query.addQuery("drop table if exists resource_properties_fts");

      // Bug 13033 we don't want to drop this table, we'll ignore the create in the dump
      //query.addQuery("drop table if exists resource_properties_fts_all;");

      var retval = {};
      // Read in the dumped out SQL from the dump text file
      var fileInputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                            .createInstance(Ci.nsIFileInputStream);
      fileInputStream.init(oldLibDumpFile, 1, 0, 0);

      var inputStream = Cc["@mozilla.org/intl/converter-input-stream;1"]
                          .createInstance(Ci.nsIConverterInputStream);
      inputStream.init(fileInputStream, "UTF-8", 16384,
                     Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
      inputStream = inputStream.QueryInterface(Ci.nsIUnicharLineInputStream);

      var curLine = {};
      var curQueryStr = "";
      var cont;
      do {
        cont = inputStream.readLine(curLine);
        curQueryStr += curLine.value;
        // If this string ends with a ';', it is the end of the query.
        // Let's push it into the query object.
        if (curQueryStr.charAt(curQueryStr.length - 1) == ";") {
          // check for the resource_properties_fts_all table and skip it.
          // The dump version doesn't work see bug 13033
          if (!curQueryStr.match("INSERT INTO sqlite_master.*resource_properties_fts_all")) {
            query.addQuery(curQueryStr);
          }
          curQueryStr = "";
        }
      } while (cont);

      inputStream.close();

      query.addQuery("update library_metadata set value = '" 
                     + this.toVersion + "' where name = 'version'");
      query.addQuery("commit");

      // Insert the dumped information into the new database.
      query.setAsyncQuery(true);
      query.execute(retval);
      dump("Database execute\n");
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                  .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = this;
   
      // XXX todo: Localize!
      this._titleText = "Library Migration Helper";
      this._statusText = "Converting Database Format...";
      this.migrationQuery = query;
    
      this.startNotificationTimer();
      SBJobUtils.showProgressDialog(sip.data, null, 0);
      this.stopNotificationTimer();
      // Cleanup
      tempOldLibFile.remove(false);
      oldLibDumpFile.remove(false);
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
  },
  _createLibrary : function sbLibraryMigration_createLibrary(query) {
    try {
    	// Now that we know we have appropriate permissions make a new query.
    
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);

      var schemaURI = ioService.newURI("chrome://songbird/content/library/localdatabase/schema.sql", null, null);
      var channel = ioService.newChannelFromURI(schemaURI);
      var stream = channel.open();
 
      var converterStream = Components.classes["@mozilla.org/intl/converter-input-stream;1"]
                                 .createInstance(Components.interfaces.nsIConverterInputStream);

      converterStream.init(stream,
                           "UTF-8",
                           8192,
                           Components.interfaces.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);

      var response = "";
      var result = {};
      var bytesRead = converterStream.readString(0xFFFFFFFF, result);
  
      while (bytesRead > 0) {
        response += result.value;
        bytesRead = converterStream.readString(0xFFFFFFFF, result);
      }

      converterStream.close();
      stream.close();

      const colonNewline = ";\n";
      var posStart = 0;
      var posEnd = response.indexOf(colonNewline);
      while (posEnd >= 0) {
        query.addQuery(response.substring(posStart, posEnd));
        posStart = posEnd + 2;
        posEnd = response.indexOf(colonNewline, posStart);
      }
    }
    catch (e) {
      dump("Exception: " + e);
      throw e;
    }
  }
};

//-----------------------------------------------------------------------------
// Module
//-----------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryMigration]);

