/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

//
// sbIPlaylist Object
//

const SONGBIRD_PLAYLISTMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistManager;1";
const SONGBIRD_PLAYLISTMANAGER_CLASSNAME = "Songbird PlaylistManager Interface"
const SONGBIRD_PLAYLISTMANAGER_CID = Components.ID("{88823823-6b51-400f-b376-7e651ecbd924}");
const SONGBIRD_PLAYLISTMANAGER_IID = Components.interfaces.sbIPlaylistManager;

const PLAYLIST_TABLE_NAME = "__table__";

const SIMPLEPLAYLIST_LIST_TABLE_NAME = "simpleplaylist_list";
const PLAYLIST_LIST_TABLE_NAME = "playlist_list";
const DYNAMICPLAYLIST_LIST_TABLE_NAME = "dynamicplaylist_list";
const SMARTPLAYLIST_LIST_TABLE_NAME = "smartplaylist_list";

const PLAYLIST_DESC_TABLE_CREATE = " (column_name TEXT UNIQUE NOT NULL, readable_name TEXT, is_visible INTEGER(0, 1) DEFAULT 0, default_visibility INTEGER(0, 1) DEFAULT 0, is_metadata INTEGER(0, 1) DEFAULT 0, sort_weight INTEGER DEFAULT 0, width INTEGER DEFAULT -1, type TEXT DEFAULT 'text', readonly INTEGER(0,1) DEFAULT 0)";
const PLAYLIST_TABLE_CREATE = " (playlist_id INTEGER PRIMARY KEY, playlist_uuid BLOB, playlist_service_uuid BLOB, playlist_order INTEGER DEFAULT -1, playlist_has_been_processed INTEGER(0,1) DEFAULT 0)";

const SIMPLEPLAYLIST_LIST_TABLE_CREATE = "CREATE TABLE simpleplaylist_list (service_uuid BLOB NOT NULL, name TEXT UNIQUE NOT NULL, readable_name TEXT NOT NULL, description TEXT DEFAULT '', type TEXT DEFAULT '', base_type TEXT DEFAULT 'simple')";
const PLAYLIST_LIST_TABLE_CREATE = "CREATE TABLE playlist_list (service_uuid BLOB NOT NULL, name TEXT UNIQUE NOT NULL, readable_name TEXT NOT NULL, description TEXT DEFAULT '', type TEXT DEFAULT '', base_type TEXT DEFAULT 'playlist')";
const DYNAMICPLAYLIST_LIST_TABLE_CREATE = "CREATE TABLE dynamicplaylist_list (service_uuid BLOB NOT NULL, name TEXT UNIQUE NOT NULL, readable_name TEXT NOT NULL, url TEXT NOT NULL UNIQUE, periodicity INTEGER NOT NULL DEFAULT 0, description TEXT DEFAULT '', type TEXT DEFAULT '', last_update INTEGER DEFAULT 0, base_type TEXT DEFAULT 'dynamic')";

const SMARTPLAYLIST_LIST_TABLE_CREATE = "CREATE TABLE smartplaylist_list (service_uuid BLOB NOT NULL, name BLOB PRIMARY KEY NOT NULL, readable_name TEXT NOT NULL, description TEXT DEFAULT '', library BLOB NOT NULL, limit_value INTEGER NOT NULL DEFAULT 0, limit_type TEXT DEFAULT '', selected_by TEXT DEFAULT '', match_on TEXT DEFAULT '', query TEXT DEFAULT '', type TEXT DEFAULT '', base_type TEXT DEFAULT 'smart')";
const SMARTPLAYLIST_CONSTRAINT_LIST_TABLE_CREATE = " (id INTEGER PRIMARY KEY, column_name TEXT NOT NULL DEFAULT '', constraint_name TEXT NOT NULL DEFAULT '', constraint_value TEXT NOT NULL DEFAULT '')";

function CPlaylistManager()
{
}

/* I actually need a constructor in this case. */
CPlaylistManager.prototype.constructor = CPlaylistManager;

/* the CPlaylistManager class def */
CPlaylistManager.prototype = 
{
  /**
   * see nsIClassInfo
   */
  getInterfaces: function (count) {
    var ifaces = [
      Components.interfaces.nsISupports,
      Components.interfaces.nsIClassInfo,
      SONGBIRD_PLAYLISTMANAGER_IID
    ];
    count.value = ifaces.length;
    return ifaces;
  },
  getHelperForLanguage: function (language) {
    return null;
  },
  contractID: SONGBIRD_PLAYLISTMANAGER_CONTRACTID,
  classDescription: SONGBIRD_PLAYLISTMANAGER_CLASSNAME,
  classID: SONGBIRD_PLAYLISTMANAGER_CID,
  implementationLanguage:
    Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Components.interfaces.nsIClassInfo.THREADSAFE,

  createDefaultPlaylistManager: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery(SIMPLEPLAYLIST_LIST_TABLE_CREATE);
      queryObj.addQuery(PLAYLIST_LIST_TABLE_CREATE);
      queryObj.addQuery(DYNAMICPLAYLIST_LIST_TABLE_CREATE);
      queryObj.addQuery(SMARTPLAYLIST_LIST_TABLE_CREATE);
      
      queryObj.execute();
      queryObj.waitForCompletion();
    }
    
    return;
  },
  
  createSimplePlaylist: function(strName, strReadableName, strDescription, strType, nMetaFieldCount, aMetaFields, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");

    if(queryObj != null)
    {
      queryObj.resetQuery();

      var strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.addQuery(strQuery);
      
      var i = 0;
      strQuery = "CREATE TABLE \"" + strName + "\" (id INTEGER PRIMARY KEY AUTOINCREMENT, url TEXT UNIQUE NOT NULL ";
      
      for(i = 0; i < nMetaFieldCount; ++i)
      {
        strQuery += ", " + aMetaFields[i] + " TEXT ";
          
        queryObj.addQuery("INSERT OR REPLACE INTO \"" + strName + "_desc\" (column_name) VALUES (\"" + aMetaFields[i] + "\")");
      }
      
      strQuery += ")";
      
      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");
      
      queryObj.addQuery(strQuery);
      queryObj.addQuery("INSERT INTO " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, type) VALUES (\"" + queryObj.getDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \""+ strDescription + "\", \"" + strType + "\")");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var simplePlaylist = (new SimplePlaylist()).QueryInterface(Components.interfaces.sbISimplePlaylist);
      
      simplePlaylist.setName(strName);
      simplePlaylist.setQueryObject(queryObj);
      
      return simplePlaylist;      
    }
    
    return null;
  },
  
  createPlaylist: function(strName, strReadableName, strDescription, strType, queryObj)
  {
    const Playlist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");
    
    if(queryObj != null)
    {
      queryObj.resetQuery();
      
      var strQuery = "CREATE TABLE \"" + strName + "\"" + PLAYLIST_TABLE_CREATE;
      strReadableName = strReadableName.replace(/\"./, "\"\"");
      strDescription = strDescription.replace(/\"./, "\"\"");

      queryObj.addQuery(strQuery);
      queryObj.addQuery("INSERT OR REPLACE INTO " + PLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, type) VALUES (\"" + queryObj.getDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \"" + strDescription + "\", \"" + strType + "\")");

      strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.addQuery(strQuery);
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var playlist = (new Playlist()).QueryInterface(Components.interfaces.sbIPlaylist);
      
      playlist.setName(strName);
      playlist.setQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },

  createDynamicPlaylist: function(strName, strReadableName, strDescription, strType, strURL, nPeriodicity, queryObj)
  {
    const DynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
    
    if(queryObj != null)
    {
      queryObj.resetQuery();
      
      var strQuery = "CREATE TABLE \"" + strName + "\"" + PLAYLIST_TABLE_CREATE;

      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");

      queryObj.addQuery(strQuery);
      queryObj.addQuery("INSERT OR REPLACE INTO " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, url, periodicity, description, type) VALUES (\"" + queryObj.getDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \"" + strURL + "\", \"" + nPeriodicity + "\", \""+ strDescription + "\", \"" + strType + "\")");

      strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.addQuery(strQuery);
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var playlist = (new DynamicPlaylist()).QueryInterface(Components.interfaces.sbIDynamicPlaylist);
      
      playlist.setName(strName);
      playlist.setQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },
  
  createSmartPlaylist: function(strName, strReadableName, strDescription, strType, strLibrary, nLimit, strLimitType, strSelectedBy, strMatchOn, queryObj)
  {
    const SmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
    
    if(queryObj != null)
    {
      queryObj.resetQuery();

      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");
      
      strQuery = "INSERT OR REPLACE INTO " + SMARTPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, library, limit_value, limit_type, selected_by, match_on, type) VALUES (\"";
      strQuery += queryObj.getDatabaseGUID() + "\", \"";
      strQuery += strName + "\", \"";
      strQuery += strReadableName + "\", \"";
      strQuery += strDescription + "\", \"";
      strQuery += strLibrary + "\", \"";
      strQuery += nLimit + "\", \"";
      strQuery += strLimitType + "\", \"";
      strQuery += strSelectedBy + "\", \"";
      strQuery += strMatchOn + "\", \"";
      strQuery += strType + "\")";
      queryObj.addQuery(strQuery);

      strQuery = "CREATE TABLE \"" + strName + "_constraints\"" + SMARTPLAYLIST_CONSTRAINT_LIST_TABLE_CREATE;
      queryObj.addQuery(strQuery);
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var playlist = (new SmartPlaylist()).QueryInterface(Components.interfaces.sbISmartPlaylist);
      
      playlist.setName(strName);
      playlist.setQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },
  
  copySimplePlaylist: function(strSourceDB, strSourceName, strSourceFilterColumn, nSourceFilterValueCount, aSourceFilterValues, strDestDB, strDestName, strReadableName, strDescription, strType, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
    
    if(queryObj != null)
    {
      var bSrcIsSimple = true;
      queryObj.setDatabaseGUID(strSourceDB);
      var pSourcePlaylist = this.getSimplePlaylist(strSourceName, queryObj);
     
      if(pSourcePlaylist == null)
      {
        pSourcePlaylist = this.getPlaylist(strSourceName, queryObj);
        
        if(pSourcePlaylist)
        {
          bSrcIsSimple = false;
        }
      }
      
      if(pSourcePlaylist)
      {
        pSourcePlaylist.getTableInfo();
        
        var resObj = queryObj.getResultObject();
      
        var i = 0;
        var rowCount = resObj.getRowCount();
        var strColName = "", strColType = "", columnList = "";
        var aCols = new Array;
        var aColType = new Array;
        
        for(i = 0; i < rowCount; i++)
        {
          strColName = resObj.getRowCell(i, 1);
          strColType = resObj.getRowCell(i, 2);
         
          if(strColName != "playlist_id") 
          {
            aCols.push(strColName);
            aColType.push(strColType);
            
            columnList += strColName;
            
            if(i + 1 != rowCount)
              columnList += ", ";
          }
        }

        queryObj.resetQuery();
        queryObj.setDatabaseGUID(strDestDB);
        
        this.createDefaultPlaylistManager(queryObj);
        
        var pDestPlaylist = this.getSimplePlaylist(strDestName, queryObj);
        
        if(!pDestPlaylist)
        {
          pDestPlaylist = this.createSimplePlaylist(strDestName, strReadableName, strDescription, strType, 0, null, queryObj);

          if(pDestPlaylist)
          {
            for(i = 0; i < aCols.length; i++)
            {
              pDestPlaylist.addColumn(aCols[i], aColType[i]);
            }
          }
        } 

        queryObj.resetQuery();
        queryObj.setDatabaseGUID(strSourceDB);
        
        // Do not use DB qualifier if using the same database
        var dbQualifier = "";
        if (strSourceDB != strDestDB)
        {
          queryObj.addQuery("ATTACH DATABASE \"" + strDestDB + ".db\" AS \"" + strDestDB + "\"");
          dbQualifier = strDestDB + "\".\"";
        }

        var strSimpleQuery = "INSERT OR IGNORE INTO \""  + dbQualifier + strDestName + "\" (" + columnList + ") SELECT ";
        strSimpleQuery += columnList + " FROM \"" + strSourceName + "\"";
        
        var strQuery = "INSERT OR IGNORE INTO \"" + dbQualifier + strDestName + "\" (url ";
        if(rowCount)
          strQuery += ", ";

        strQuery += columnList + ") SELECT url ";

        if(rowCount)
          strQuery += ", ";

        strQuery += columnList + " FROM \"" + strSourceName + "\" LEFT JOIN library ON \"" + strSourceName + "\".playlist_uuid = library.uuid";
        
        var strFilterQuery = " WHERE \"" + strSourceName + "\".url NOT IN " + dbQualifier + strDestName + "\".url AND ";
        
        if(strSourceFilterColumn != "" && nSourceFilterValueCount && aSourceFilterValues)
        {
          if(strSourceName != "library")
            strFilterQuery += "playlist_";
            
          strFilterQuery += strSourceFilterColumn + " IN (";
          for(var i = 0; i < nSourceFilterValueCount; ++i)
          {
            strFilterQuery += "\"" + aSourceFilterValues[i] + "\"";
            if(i + 1 != nSourceFilterValueCount)
              strFilterQuery += ", ";
          }
          strFilterQuery += ")";
          
          strSimpleQuery += strFilterQuery;
          strQuery += strFilterQuery;
        }

        if(bSrcIsSimple)
        {
          queryObj.addQuery(strSimpleQuery);
        }
        else
        {
          queryObj.addQuery(strQuery);
        }

        queryObj.addQuery("DETACH DATABASE \"" + strDestDB + "\"");

        var success = queryObj.execute();
        queryObj.waitForCompletion();
        
        var error = queryObj.getLastError();
        queryObj.setDatabaseGUID(strDestDB);
      
        return pDestPlaylist;
      }
    }
    
    return null;
  },

  copyPlaylist: function(strSourceDB, strSourceName, strDestDB, strDestName, strReadableName, strDescription, strType, queryObj)
  {
    const Playlist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");

    if(queryObj != null)
    {
      queryObj.setDatabaseGUID(strSourceDB);
      var pSourcePlaylist = this.getSimplePlaylist(strSourceName, queryObj);
      
      if(pSourcePlaylist)
      {
        pSourcePlaylist.getTableInfo();
        var resObj = queryObj.getResultObject();

        queryObj.setDatabaseGUID(strDestDB);
        var pDestPlaylist = this.createPlaylist(strDestName, strReadableName, strDescription, strType, queryObj);
        
        if(pDestPlaylist)
        {
        
          var i = 0;
          var rowCount = resObj.getRowCount();
          var columnList = "";
          
          for(i = 0; i < rowCount; i++)
          {
            var strColName = resObj.getRowCellByColumn(i, "name");
            var strColType = resObj.getRowCellByColumn(i, "type");
            
            columnList +=  strColName;
            if(i != rowCount - 1)
              columnList += ", ";
              
            pDestPlaylist.addColumn(strColName, strColType);
          }
       
          queryObj.resetQuery();
          queryObj.setDatabaseGUID(strSourceDB);
          
          queryObj.addQuery("INSERT INTO \"db/" + strDestDB + "\".\"" + strDestName + "\" (" + columnList + ") SELECT " + columnList + " FROM \"" + strSourceName + "\"");
          queryObj.execute();
          queryObj.waitForCompletion();
          
          queryObj.setDatabaseGUID(strDestDB);
       
          return pDestPlaylist;
        }
      }
    }
    
    return null;
  },

  deleteSimplePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var resObj = queryObj.getResultObject();
      
      if(resObj.getRowCount() > 0)
      {
        queryObj.addQuery("DROP TABLE \"" + strName + "\"");
        queryObj.addQuery("DELETE FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.execute();
        queryObj.waitForCompletion();
        
        return 1;
      }
    }
    
    return 0;
  },
  
  deletePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var resObj = queryObj.getResultObject();
      
      if(resObj.getRowCount() > 0)
      {
        queryObj.addQuery("DROP TABLE \"" + strName + "\"");
        queryObj.addQuery("DELETE FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.execute();
        queryObj.waitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
 
  deleteDynamicPlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var resObj = queryObj.getResultObject();
      
      if(resObj.getRowCount() > 0)
      {
        queryObj.addQuery("DROP TABLE \"" + strName + "\"");
        queryObj.addQuery("DELETE FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.execute();
        queryObj.waitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
  
  deleteSmartPlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var resObj = queryObj.getResultObject();
      
      if(resObj.getRowCount() > 0)
      {
        queryObj.addQuery("DROP TABLE \"" + strName + "\"");
        queryObj.addQuery("DROP TABLE \"" + strName + "_constraints\"");
        queryObj.addQuery("DELETE FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        
        queryObj.execute();
        queryObj.waitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
  
  getAllPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.setDatabaseGUID("*");
      
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME);
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + PLAYLIST_LIST_TABLE_NAME);
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME);
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SMARTPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.execute();
      
      if(!queryObj.isAyncQuery());
        queryObj.waitForCompletion();
    }
  },
 
  getSimplePlaylistList: function(queryobj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.execute();
      if(!queryObj.isAyncQuery())      
        queryObj.waitForCompletion();
    }
  },
  
  getPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + PLAYLIST_LIST_TABLE_NAME);
      
      queryObj.execute();
      if(!queryObj.isAyncQuery())      
        queryObj.waitForCompletion();
    }
  },
 
  getDynamicPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.execute();
      if(!queryObj.isAyncQuery())      
        queryObj.waitForCompletion();
    }
  },
  
  getSmartPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SMARTPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.execute();
      if(!queryObj.isAyncQuery())
        queryObj.waitForCompletion();
    }
  },
 
  getSimplePlaylist: function(strName, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");

    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();

      if(queryObj.getResultObject().getRowCount() > 0)
      {
        var simplePlaylist = (new SimplePlaylist()).QueryInterface(Components.interfaces.sbISimplePlaylist);
        
        simplePlaylist.setName(strName);
        simplePlaylist.setQueryObject(queryObj);
        
        return simplePlaylist;
      }
    }

    return null;
  },
  
  getPlaylist: function(strName, queryObj)
  {
    const Playlist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");

    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();

      var playlist = null;
      if(queryObj.getResultObject().getRowCount() > 0)
      {
        playlist = (new Playlist()).QueryInterface(Components.interfaces.sbIPlaylist);
        
        playlist.setName(strName);
        playlist.setQueryObject(queryObj);
      
        return playlist;
      }
      else
      {
        playlist = this.getDynamicPlaylist(strName, queryObj);
        if(playlist)
        {
          return playlist.QueryInterface(Components.interfaces.sbIPlaylist);
        }
        
        playlist = this.getSmartPlaylist(strName, queryObj);
        if(playlist)
          return playlist.QueryInterface(Components.interfaces.sbIPlaylist);
      }
    }

    return null;
  },

  getDynamicPlaylist: function(strName, queryObj)
  {
    const DynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");

    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();

      var resObj = queryObj.getResultObject();
      if(resObj.getRowCount() > 0)
      {
        var playlist = new DynamicPlaylist();
        if(playlist)
        {
          playlist = playlist.QueryInterface(Components.interfaces.sbIDynamicPlaylist);
          playlist.setName(strName);
          playlist.setQueryObject(queryObj);
        
          return playlist;
        }
      }
    }

    return null;
  },
  
  getSmartPlaylist: function(strName, queryObj)
  {
    const SmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");

    if(queryObj != null)
    {
      queryObj.resetQuery();
      queryObj.addQuery("SELECT name FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");

      queryObj.execute();
      queryObj.waitForCompletion();

      var resObj = queryObj.getResultObject();
      if(resObj.getRowCount() > 0)
      {
        var playlist = (new SmartPlaylist()).QueryInterface(Components.interfaces.sbISmartPlaylist);
        
        playlist.setName(strName);
        playlist.setQueryObject(queryObj);
      
        return playlist;
      }
    }

    return null;
  },

  getDynamicPlaylistsForUpdate: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.setDatabaseGUID("*");
      
      var strQuery = "SELECT service_uuid, name, periodicity, last_update FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME;
      queryObj.resetQuery();
      queryObj.addQuery(strQuery);
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      var resObj = queryObj.getResultObject();
      
      var dNow = new Date();
      var playlists = new Array();
      
      for(var i = 0; i < resObj.getRowCount(); ++i)
      {
        var lastUpdate = parseInt(resObj.getRowCellByColumn(i, "last_update"));
        var periodicity = parseInt(resObj.getRowCellByColumn(i, "periodicity"));
        
        if(dNow.getTime() - lastUpdate >= (periodicity * 1000 * 60))
        {
          var name = resObj.getRowCellByColumn(i, "name");
          playlists.push(name);
        }
      }      
      
      if(playlists.length)
      {
        strQuery = "SELECT * FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name IN (";
        for(i = 0; i < playlists.length; ++i)
        {
          strQuery += "\"" + playlists[i] + "\"";
          if(i + 1 != playlists.length)
            strQuery += ", ";
        }
        
        strQuery += ")";

        queryObj.resetQuery();
        queryObj.addQuery(strQuery);
        
        queryObj.execute();
        queryObj.waitForCompletion();
        
        resObj = queryObj.getResultObject();
        return parseInt(resObj.getRowCount());
      }
    }
    
    return 0;
  },

  setDynamicPlaylistLastUpdate: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      var dNow = new Date();
      
      queryObj.resetQuery();
      queryObj.addQuery("UPDATE \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" SET last_update = " + dNow.getTime() + " WHERE name = \"" + strName + "\"");
      
      queryObj.execute();
      queryObj.waitForCompletion();
      
      return queryObj.getLastError() ? false : true;
    }
    
    return false;
  },

  purgeTrackByGUIDFromPlaylists: function(mediaGUID, strDBGUID)
  {
    var queryObj = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
    if(queryObj)
    {
      queryObj.setAsyncQuery(false);
      queryObj.setDatabaseGUID(strDBGUID);
      
      var strQuery = "SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " UNION ";
      strQuery += "SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME;
      
      queryObj.addQuery(strQuery);
      queryObj.execute();
      
      var resObj = queryObj.getResultObject();
      var rowCount = resObj.getRowCount();
      
      var q = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
      q.setAsyncQuery(false);
      q.setDatabaseGUID(strDBGUID);
      
      for(var i = 0; i < rowCount; i++)
      {
        var plName = resObj.getRowCellByColumn(i, "name");
        q.addQuery("DELETE FROM \"" + plName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      }
      
      q.execute();
    }
  },

  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.nsISupports) &&
        !iid.equals(Components.interfaces.nsIClassInfo) &&
        !iid.equals(SONGBIRD_PLAYLISTMANAGER_IID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

/**
 * \class sbPlaylistManagerModule
 * \brief 
 */
var sbPlaylistManagerModule = 
{
  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.nsISupports) &&
        !iid.equals(Components.interfaces.nsIModule))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTMANAGER_CID, 
                                    SONGBIRD_PLAYLISTMANAGER_CLASSNAME, 
                                    SONGBIRD_PLAYLISTMANAGER_CONTRACTID, 
                                    fileSpec, location, type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if (!cid.equals(SONGBIRD_PLAYLISTMANAGER_CID))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistManagerFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbPlaylistManagerFactory
 * \brief 
 */
var sbPlaylistManagerFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTMANAGER_IID) &&
        !iid.equals(Components.interfaces.nsIClassInfo) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_INVALID_ARG;

    return new CPlaylistManager().QueryInterface(iid);
  }
} //sbPlaylistFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistManagerModule;
} //NSGetModule
