/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

const SONGBIRD_PLAYLISTMANAGER_CONTRACTID = "@songbird.org/Songbird/PlaylistManager;1";
const SONGBIRD_PLAYLISTMANAGER_CLASSNAME = "Songbird PlaylistManager Interface"
const SONGBIRD_PLAYLISTMANAGER_CID = Components.ID('{0BE3A41A-6673-494A-A53E-9740A98ACFF7}');
const SONGBIRD_PLAYLISTMANAGER_IID = Components.interfaces.sbIPlaylistManager;

const PLAYLIST_TABLE_NAME = "__table__";

const SIMPLEPLAYLIST_LIST_TABLE_NAME = "simpleplaylist_list";
const PLAYLIST_LIST_TABLE_NAME = "playlist_list";
const DYNAMICPLAYLIST_LIST_TABLE_NAME = "dynamicplaylist_list";
const SMARTPLAYLIST_LIST_TABLE_NAME = "smartplaylist_list";

const PLAYLIST_DESC_TABLE_CREATE = " (column_name TEXT UNIQUE NOT NULL, readable_name TEXT, is_visible INTEGER(0, 1) DEFAULT 0, default_visibility INTEGER(0, 1) DEFAULT 0, is_metadata INTEGER(0, 1) DEFAULT 0, sort_weight INTEGER DEFAULT 0, width INTEGER DEFAULT 0, type TEXT DEFAULT 'text')";
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
  CreateDefaultPlaylistManager: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery(SIMPLEPLAYLIST_LIST_TABLE_CREATE);
      queryObj.AddQuery(PLAYLIST_LIST_TABLE_CREATE);
      queryObj.AddQuery(DYNAMICPLAYLIST_LIST_TABLE_CREATE);
      queryObj.AddQuery(SMARTPLAYLIST_LIST_TABLE_CREATE);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
    }
    
    return;
  },
  
  CreateSimplePlaylist: function(strName, strReadableName, strDescription, strType, nMetaFieldCount, aMetaFields, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbird.org/Songbird/SimplePlaylist;1", "sbISimplePlaylist");

    if(queryObj != null)
    {
      queryObj.ResetQuery();

      var strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.AddQuery(strQuery);
      
      var i = 0;
      strQuery = "CREATE TABLE \"" + strName + "\" (id INTEGER PRIMARY KEY AUTOINCREMENT, url TEXT UNIQUE NOT NULL ";
      
      for(i = 0; i < nMetaFieldCount; ++i)
      {
        strQuery += ", " + aMetaFields[i] + " TEXT ";
          
        queryObj.AddQuery("INSERT OR REPLACE INTO \"" + strName + "_desc\" (column_name) VALUES (\"" + aMetaFields[i] + "\")");
      }
      
      strQuery += ")";
      
      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");
      
      queryObj.AddQuery(strQuery);
      queryObj.AddQuery("INSERT INTO " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, type) VALUES (\"" + queryObj.GetDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \""+ strDescription + "\", \"" + strType + "\")");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var simplePlaylist = (new SimplePlaylist()).QueryInterface(Components.interfaces.sbISimplePlaylist);
      
      simplePlaylist.SetName(strName);
      simplePlaylist.SetQueryObject(queryObj);
      
      return simplePlaylist;      
    }
    
    return null;
  },
  
  CreatePlaylist: function(strName, strReadableName, strDescription, strType, queryObj)
  {
    const Playlist = new Components.Constructor("@songbird.org/Songbird/Playlist;1", "sbIPlaylist");
    
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      
      var strQuery = "CREATE TABLE \"" + strName + "\"" + PLAYLIST_TABLE_CREATE;
      strReadableName = strReadableName.replace(/\"./, "\"\"");
      strDescription = strDescription.replace(/\"./, "\"\"");

      queryObj.AddQuery(strQuery);
      queryObj.AddQuery("INSERT OR REPLACE INTO " + PLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, type) VALUES (\"" + queryObj.GetDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \"" + strDescription + "\", \"" + strType + "\")");

      strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.AddQuery(strQuery);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var playlist = (new Playlist()).QueryInterface(Components.interfaces.sbIPlaylist);
      
      playlist.SetName(strName);
      playlist.SetQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },

  CreateDynamicPlaylist: function(strName, strReadableName, strDescription, strType, strURL, nPeriodicity, queryObj)
  {
    const DynamicPlaylist = new Components.Constructor("@songbird.org/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
    
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      
      var strQuery = "CREATE TABLE \"" + strName + "\"" + PLAYLIST_TABLE_CREATE;

      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");

      queryObj.AddQuery(strQuery);
      queryObj.AddQuery("INSERT OR REPLACE INTO " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, url, periodicity, description, type) VALUES (\"" + queryObj.GetDatabaseGUID() + "\", \"" + strName + "\", \"" + strReadableName + "\", \"" + strURL + "\", \"" + nPeriodicity + "\", \""+ strDescription + "\", \"" + strType + "\")");

      strQuery = "CREATE TABLE \"" + strName + "_desc\"" + PLAYLIST_DESC_TABLE_CREATE;
      queryObj.AddQuery(strQuery);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var playlist = (new DynamicPlaylist()).QueryInterface(Components.interfaces.sbIDynamicPlaylist);
      
      playlist.SetName(strName);
      playlist.SetQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },
  
  CreateSmartPlaylist: function(strName, strReadableName, strDescription, strType, strLibrary, nLimit, strLimitType, strSelectedBy, strMatchOn, queryObj)
  {
    const SmartPlaylist = new Components.Constructor("@songbird.org/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
    
    if(queryObj != null)
    {
      queryObj.ResetQuery();

      strReadableName = strReadableName.replace(/"/g, "\"\"");
      strDescription = strDescription.replace(/"/g, "\"\"");
      
      strQuery = "INSERT OR REPLACE INTO " + SMARTPLAYLIST_LIST_TABLE_NAME + " (service_uuid, name, readable_name, description, library, limit_value, limit_type, selected_by, match_on, type) VALUES (\"";
      strQuery += queryObj.GetDatabaseGUID() + "\", \"";
      strQuery += strName + "\", \"";
      strQuery += strReadableName + "\", \"";
      strQuery += strDescription + "\", \"";
      strQuery += strLibrary + "\", \"";
      strQuery += nLimit + "\", \"";
      strQuery += strLimitType + "\", \"";
      strQuery += strSelectedBy + "\", \"";
      strQuery += strMatchOn + "\", \"";
      strQuery += strType + "\")";
      queryObj.AddQuery(strQuery);

      strQuery = "CREATE TABLE \"" + strName + "_constraints\"" + SMARTPLAYLIST_CONSTRAINT_LIST_TABLE_CREATE;
      queryObj.AddQuery(strQuery);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var playlist = (new SmartPlaylist()).QueryInterface(Components.interfaces.sbISmartPlaylist);
      
      playlist.SetName(strName);
      playlist.SetQueryObject(queryObj);
      
      return playlist;
    }
    
    return null;
  },
  
  CopySimplePlaylist: function(strSourceDB, strSourceName, strSourceFilterColumn, nSourceFilterValueCount, aSourceFilterValues, strDestDB, strDestName, strReadableName, strDescription, strType, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbird.org/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
    
    if(queryObj != null)
    {
      var bSrcIsSimple = true;
      queryObj.SetDatabaseGUID(strSourceDB);
      var pSourcePlaylist = this.GetSimplePlaylist(strSourceName, queryObj);
     
      if(pSourcePlaylist == null)
      {
        pSourcePlaylist = this.GetPlaylist(strSourceName, queryObj);
        
        if(pSourcePlaylist)
        {
          bSrcIsSimple = false;
        }
      }
      
      if(pSourcePlaylist)
      {
        pSourcePlaylist.GetTableInfo();
        
        var resObj = queryObj.GetResultObject();
      
        var i = 0;
        var rowCount = resObj.GetRowCount();
        var strColName = "", strColType = "", columnList = "";
        var aCols = new Array;
        var aColType = new Array;
        
        for(i = 0; i < rowCount; i++)
        {
          strColName = resObj.GetRowCell(i, 1);
          strColType = resObj.GetRowCell(i, 2);
         
          if(strColName != "playlist_id") 
          {
            aCols.push(strColName);
            aColType.push(strColType);
            
            columnList += strColName;
            
            if(i + 1 != rowCount)
              columnList += ", ";
          }
        }

        queryObj.ResetQuery();
        queryObj.SetDatabaseGUID(strDestDB);
        
        this.CreateDefaultPlaylistManager(queryObj);
        
        var pDestPlaylist = this.GetSimplePlaylist(strDestName, queryObj);
        
        if(!pDestPlaylist)
        {
          pDestPlaylist = this.CreateSimplePlaylist(strDestName, strReadableName, strDescription, strType, 0, null, queryObj);

          if(pDestPlaylist)
          {
            for(i = 0; i < rowCount; i++)
            {
              pDestPlaylist.AddColumn(aCols[i], aColType[i]);
            }
          }
        } 

        queryObj.ResetQuery();
        queryObj.SetDatabaseGUID(strSourceDB);
        
        // Do not use DB qualifier if using the same database
        var dbQualifier = "";
        if (strSourceDB != strDestDB)
        {
	        queryObj.AddQuery("ATTACH DATABASE \"" + strDestDB + ".db\" AS \"" + strDestDB + "\"");
	        dbQualifier = strDestDB + "\".\"";
	    }

        var strSimpleQuery = "INSERT INTO \""  + dbQualifier + strDestName + "\" (" + columnList + ") SELECT ";
        strSimpleQuery += columnList + " FROM \"" + strSourceName + "\"";
        
        var strQuery = "INSERT INTO \"" + dbQualifier + strDestName + "\" (url ";
        if(rowCount) strQuery += ", ";
        strQuery += columnList + ") SELECT url ";
        if(rowCount) strQuery += ", ";
        strQuery += columnList + " FROM \"" + strSourceName + "\" LEFT JOIN library ON \"" + strSourceName + "\".playlist_uuid = library.uuid";
        
        var strFilterQuery = " WHERE ";
        
        if(strSourceFilterColumn != "" && nSourceFilterValueCount && aSourceFilterValues)
        {
          strFilterQuery += "playlist_" + strSourceFilterColumn + " IN (";
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
          queryObj.AddQuery(strSimpleQuery);
        }
        else
        {
          queryObj.AddQuery(strQuery);
        }

        queryObj.AddQuery("DETACH DATABASE \"" + strDestDB + "\"");
          
        var success = queryObj.Execute();
        queryObj.WaitForCompletion();
        
        var error = queryObj.GetLastError();
        queryObj.SetDatabaseGUID(strDestDB);
      
        return pDestPlaylist;
      }
    }
    
    return null;
  },

  CopyPlaylist: function(strSourceDB, strSourceName, strDestDB, strDestName, strReadableName, strDescription, strType, queryObj)
  {
    const Playlist = new Components.Constructor("@songbird.org/Songbird/Playlist;1", "sbIPlaylist");

    if(queryObj != null)
    {
      queryObj.SetDatabaseGUID(strSourceDB);
      var pSourcePlaylist = this.GetSimplePlaylist(strSourceName, queryObj);
      
      if(pSourcePlaylist)
      {
        pSourcePlaylist.GetTableInfo();
        var resObj = queryObj.GetResultObject();

        queryObj.SetDatabaseGUID(strDestDB);
        var pDestPlaylist = this.CreatePlaylist(strDestName, strReadableName, strDescription, strType, queryObj);
        
        if(pDestPlaylist)
        {
        
          var i = 0;
          var rowCount = resObj.GetRowCount();
          var columnList = "";
          
          for(i = 0; i < rowCount; i++)
          {
            var strColName = resObj.GetRowCellByColumn(i, "name");
            var strColType = resObj.GetRowCellByColumn(i, "type");
            
            columnList +=  strColName;
            if(i != rowCount - 1)
              columnList += ", ";
              
            pDestPlaylist.AddColumn(strColName, strColType);
          }
       
          queryObj.ResetQuery();
          queryObj.SetDatabaseGUID(strSourceDB);
          
          queryObj.AddQuery("INSERT INTO \"db/" + strDestDB + "\".\"" + strDestName + "\" (" + columnList + ") SELECT " + columnList + " FROM \"" + strSourceName + "\"");
          queryObj.Execute();
          queryObj.WaitForCompletion();
          
          queryObj.SetDatabaseGUID(strDestDB);
       
          return pDestPlaylist;
        }
      }
    }
    
    return null;
  },

  DeleteSimplePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var resObj = queryObj.GetResultObject();
      
      if(resObj.GetRowCount() > 0)
      {
        queryObj.AddQuery("DROP TABLE \"" + strName + "\"");
        queryObj.AddQuery("DELETE FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.Execute();
        queryObj.WaitForCompletion();
        
        return 1;
      }
    }
    
    return 0;
  },
  
  DeletePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var resObj = queryObj.GetResultObject();
      
      if(resObj.GetRowCount() > 0)
      {
        queryObj.AddQuery("DROP TABLE \"" + strName + "\"");
        queryObj.AddQuery("DELETE FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.Execute();
        queryObj.WaitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
 
  DeleteDynamicPlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var resObj = queryObj.GetResultObject();
      
      if(resObj.GetRowCount() > 0)
      {
        queryObj.AddQuery("DROP TABLE \"" + strName + "\"");
        queryObj.AddQuery("DELETE FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        queryObj.Execute();
        queryObj.WaitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
  
  DeleteSmartPlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var resObj = queryObj.GetResultObject();
      
      if(resObj.GetRowCount() > 0)
      {
        queryObj.AddQuery("DROP TABLE \"" + strName + "\"");
        queryObj.AddQuery("DROP TABLE \"" + strName + "_constraints\"");
        queryObj.AddQuery("DELETE FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
        
        queryObj.Execute();
        queryObj.WaitForCompletion();
        
        return 1;
      }
    }

    return 0;
  },
  
  GetAllPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.SetDatabaseGUID("*");
      
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME);
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + PLAYLIST_LIST_TABLE_NAME);
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME);
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SMARTPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.Execute();
      
      if(!queryObj.IsAyncQuery());
        queryObj.WaitForCompletion();
    }
  },
 
  GetSimplePlaylistList: function(queryobj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.Execute();
      if(!queryObj.IsAyncQuery())      
        queryObj.WaitForCompletion();
    }
  },
  
  GetPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + PLAYLIST_LIST_TABLE_NAME);
      
      queryObj.Execute();
      if(!queryObj.IsAyncQuery())      
        queryObj.WaitForCompletion();
    }
  },
 
  GetDynamicPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.Execute();
      if(!queryObj.IsAyncQuery())      
        queryObj.WaitForCompletion();
    }
  },
  
  GetSmartPlaylistList: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT service_uuid, name, readable_name, type, base_type, description FROM " + SMARTPLAYLIST_LIST_TABLE_NAME);
      
      queryObj.Execute();
      if(!queryObj.IsAyncQuery())
        queryObj.WaitForCompletion();
    }
  },
 
  GetSimplePlaylist: function(strName, queryObj)
  {
    const SimplePlaylist = new Components.Constructor("@songbird.org/Songbird/SimplePlaylist;1", "sbISimplePlaylist");

    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + SIMPLEPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();

      if(queryObj.GetResultObject().GetRowCount() > 0)
      {
        var simplePlaylist = (new SimplePlaylist()).QueryInterface(Components.interfaces.sbISimplePlaylist);
        
        simplePlaylist.SetName(strName);
        simplePlaylist.SetQueryObject(queryObj);
        
        return simplePlaylist;
      }
    }

    return null;
  },
  
  GetPlaylist: function(strName, queryObj)
  {
    const Playlist = new Components.Constructor("@songbird.org/Songbird/Playlist;1", "sbIPlaylist");

    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();

      var playlist = null;
      if(queryObj.GetResultObject().GetRowCount() > 0)
      {
        playlist = (new Playlist()).QueryInterface(Components.interfaces.sbIPlaylist);
        
        playlist.SetName(strName);
        playlist.SetQueryObject(queryObj);
      
        return playlist;
      }
      else
      {
        playlist = this.GetDynamicPlaylist(strName, queryObj);
        if(playlist)
        {
          return playlist.QueryInterface(Components.interfaces.sbIPlaylist);
        }
        
        playlist = this.GetSmartPlaylist(strName, queryObj);
        if(playlist)
          return playlist.QueryInterface(Components.interfaces.sbIPlaylist);
      }
    }

    return null;
  },

  GetDynamicPlaylist: function(strName, queryObj)
  {
    const DynamicPlaylist = new Components.Constructor("@songbird.org/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");

    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();

      var resObj = queryObj.GetResultObject();
      if(resObj.GetRowCount() > 0)
      {
        var playlist = new DynamicPlaylist();
        if(playlist)
        {
          playlist = playlist.QueryInterface(Components.interfaces.sbIDynamicPlaylist);
          playlist.SetName(strName);
          playlist.SetQueryObject(queryObj);
        
          return playlist;
        }
      }
    }

    return null;
  },
  
  GetSmartPlaylist: function(strName, queryObj)
  {
    const SmartPlaylist = new Components.Constructor("@songbird.org/Songbird/SmartPlaylist;1", "sbISmartPlaylist");

    if(queryObj != null)
    {
      queryObj.ResetQuery();
      queryObj.AddQuery("SELECT name FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + strName + "\"");

      queryObj.Execute();
      queryObj.WaitForCompletion();

      var resObj = queryObj.GetResultObject();
      if(resObj.GetRowCount() > 0)
      {
        var playlist = (new SmartPlaylist()).QueryInterface(Components.interfaces.sbISmartPlaylist);
        
        playlist.SetName(strName);
        playlist.SetQueryObject(queryObj);
      
        return playlist;
      }
    }

    return null;
  },

  GetDynamicPlaylistsForUpdate: function(queryObj)
  {
    if(queryObj != null)
    {
      queryObj.SetDatabaseGUID("*");
      
      var strQuery = "SELECT service_uuid, name, periodicity, last_update FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME;
      queryObj.ResetQuery();
      queryObj.AddQuery(strQuery);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      var resObj = queryObj.GetResultObject();
      
      var dNow = new Date();
      var playlists = new Array();
      
      for(var i = 0; i < resObj.GetRowCount(); ++i)
      {
        var lastUpdate = parseInt(resObj.GetRowCellByColumn(i, "last_update"));
        var periodicity = parseInt(resObj.GetRowCellByColumn(i, "periodicity"));
        
        if(dNow.getTime() - lastUpdate >= (periodicity * 1000 * 60))
        {
          var name = resObj.GetRowCellByColumn(i, "name");
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

        queryObj.ResetQuery();
        queryObj.AddQuery(strQuery);
        
        queryObj.Execute();
        queryObj.WaitForCompletion();
        
        resObj = queryObj.GetResultObject();
        return parseInt(resObj.GetRowCount());
      }
    }
    
    return 0;
  },

  SetDynamicPlaylistLastUpdate: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      var dNow = new Date();
      
      queryObj.ResetQuery();
      queryObj.AddQuery("UPDATE \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" SET last_update = " + dNow.getTime() + " WHERE name = \"" + strName + "\"");
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      return queryObj.GetLastError() ? false : true;
    }
    
    return false;
  },

  PurgeTrackByGUIDFromPlaylists: function(mediaGUID, strDBGUID)
  {
    var queryObj = Components.classes["@songbird.org/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
    if(queryObj)
    {
      queryObj.SetAsyncQuery(false);
      queryObj.SetDatabaseGUID(strDBGUID);
      
      var strQuery = "SELECT name FROM " + PLAYLIST_LIST_TABLE_NAME + " UNION ";
      strQuery += "SELECT name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME;
      
      queryObj.AddQuery(strQuery);
      queryObj.Execute();
      
      var resObj = queryObj.GetResultObject();
      var rowCount = resObj.GetRowCount();
      
      var q = Components.classes["@songbird.org/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
      q.SetAsyncQuery(false);
      q.SetDatabaseGUID(strDBGUID);
      
      for(var i = 0; i < rowCount; i++)
      {
        var plName = resObj.GetRowCellByColumn(i, "name");
        q.AddQuery("DELETE FROM \"" + plName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      }
      
      q.Execute();
    }
  },

  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
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
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTMANAGER_CID, 
                                      SONGBIRD_PLAYLISTMANAGER_CLASSNAME, 
                                      SONGBIRD_PLAYLISTMANAGER_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
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
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylistManager()).QueryInterface(iid);
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
