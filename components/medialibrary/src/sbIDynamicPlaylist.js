/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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
// sbIDynamicPlaylist Object
//

const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

const SONGBIRD_DYNAMICPLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/DynamicPlaylist;1";
const SONGBIRD_DYNAMICPLAYLIST_CLASSNAME = "Songbird Dynamic Playlist Interface"
const SONGBIRD_DYNAMICPLAYLIST_CID = Components.ID("{6322a435-1e78-4825-91c8-520e829c23b8}");
const SONGBIRD_DYNAMICPLAYLIST_IID = Components.interfaces.sbIDynamicPlaylist;

const DYNAMICPLAYLIST_LIST_TABLE_NAME = "dynamicplaylist_list";

function CDynamicPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
}

CDynamicPlaylist.prototype.constructor = CDynamicPlaylist;

/* the CPlaylist class def */
CDynamicPlaylist.prototype = 
{
  m_strName: "",
  m_strReadableName: "",
  m_queryObject: null,
  m_internalQueryObject: null,
  
  SetQueryObject: function(queryObj)
  {
    this.m_queryObject = queryObj; 
    this.m_internalQueryObject.setDatabaseGUID(queryObj.getDatabaseGUID());
    return;
  },
  
  GetQueryObject: function()
  {
    return this.m_queryObject;
  },
  
  AddByGUID: function(mediaGUID, serviceGUID, nPosition, bReplace, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      if(bReplace)
      {
        var index = this.FindByGUID(mediaGUID);
        if(index != -1)
          return true;
      }

      this.m_queryObject.addQuery("INSERT INTO \"" + this.m_strName + "\" (playlist_uuid, playlist_service_uuid) VALUES (\"" + mediaGUID + "\", \"" + serviceGUID + "\")");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  RemoveByGUID: function(mediaGUID, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();

      this.m_queryObject.addQuery("DELETE FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  RemoveByIndex: function(mediaIndex, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();

      this.m_queryObject.addQuery("DELETE FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  MoveByGUID: function(mediaGUID, nPosition)
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },
  
  MoveByIndex: function(mediaIndex, nPosition)
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },

  FindByGUID: function(mediaGUID)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.resetQuery();
      this.m_internalQueryObject.addQuery("SELECT playlist_id FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      this.m_internalQueryObject.execute();
      this.m_internalQueryObject.waitForCompletion();
      
      var resObj = this.m_internalQueryObject.getResultObject();
      
      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
    
    return -1;
  },
  
  FindByIndex: function(mediaIndex)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.resetQuery();
      this.m_internalQueryObject.addQuery("SELECT playlist_uuid FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + nIndex + "\"");
      
      this.m_internalQueryObject.execute();
      this.m_internalQueryObject.waitForCompletion();
      
      var resObj = this.m_internalQueryObject.getResultObject();
      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
    
    return "";
  },

  GetColumnInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "_desc\" UNION SELECT * FROM library_desc ORDER BY sort_weight, column_name ASC");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  },
  
  SetColumnInfo: function(strColumn, strReadableName, isVisible, defaultVisibility, isMetadata, sortWeight, colWidth, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE \"" + this.m_strName + "_desc\" SET ";
      strQuery += "readable_name = \"" + strReadableName + "\", ";
      strQuery += "is_visible = \"" + isVisible ? 1: 0 + "\", ";
      strQuery += "default_visibility = \"" + defaultVisibility ? 1 : 0 + "\", ";
      strQuery += "is_metadata = \"" + isMetadata ? 1 : 0 + "\", ";
      strQuery += "sort_weight = \"" + sortWeight + "\", ";
      strQuery += "width = \"" + colWidth + "\" ";
      strQuery += "WHERE column_name = \"" + strColumn + "\"";
      
      this.m_queryObject.addQuery(strQuery);
      
      strQuery = "UPDATE \"library_desc\" SET ";
      strQuery += "readable_name = \"" + strReadableName + "\", ";
      strQuery += "is_visible = \"" + isVisible ? 1: 0 + "\", ";
      strQuery += "default_visibility = \"" + defaultVisibility ? 1 : 0 + "\", ";
      strQuery += "is_metadata = \"" + isMetadata ? 1 : 0 + "\", ";
      strQuery += "sort_weight = \"" + sortWeight + "\", ";
      strQuery += "width = \"" + colWidth + "\" ";
      strQuery += "WHERE column_name = \"" + strColumn + "\"";
      
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }    
  },

  GetTableInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("PRAGMA table_info(\"" + this.m_strName + "\")");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  },
  
  AddColumn: function(strColumn, strDataType)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("ALTER TABLE \"" + this.m_strName + "\" ADD COLUMN \"" + strColumn + "\" " + strDataType);
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO \"" + this.m_strName + "_desc\" (column_name) VALUES (\"" + strColumn + "\")");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }

    return;
  },
  
  DeleteColumn: function(strColumn)
  {
    return;
  },

  GetNumEntries: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT COUNT(playlist_id) FROM \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      
      return resObj.GetRowCell(0, 0);
    }
    
    return 0;
  },  
  
  GetEntry: function(nEntry)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + nEntry + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();

      return 1;
    }
    
    return 0;
  },

  GetAllEntries: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      return resObj.GetRowCount();
    }
    
    return 0;
  },

  GetColumnValueByIndex: function(mediaIndex, strColumn)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + mediaIndex + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
        
    return "";
  },
  
  GetColumnValueByGUID: function(mediaGUID, strColumn)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },

  GetColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount)
  {
    nValueCount = 0;
    var aValues = new Array();

    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.resetQuery();
      
      var i = 0;
      for( ; i < nColumnCount; i++)
      {
        strQuery += aColumns[i];
        
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + mediaIndex + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      nValueCount = resObj.GetColumnCount();
      
      for(var i = 0; i < nValueCount; i++)
      {
        aValues.push(resObj.GetRowCell(0, i));
      }    
    }
    
    return aValues;
  },
  
  GetColumnValuesByGUID: function(mediaGUID, nColumnCount, aColumns, nValueCount)
  {
    nValueCount = 0;
    var aValues = new Array();

    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.resetQuery();
      
      var i = 0;
      for( ; i < nColumnCount; i++)
      {
        strQuery += aColumns[i];
        
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      nValueCount = resObj.GetColumnCount();
      
      for(var i = 0; i < nValueCount; i++)
      {
        aValues.push(resObj.GetRowCell(0, i));
      }    
    }
    
    return aValues;
  },
  
  SetColumnValueByIndex: function(mediaIndex, strColumn, strValue)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.addQuery("UPDATE \"" + this.m_strName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE playlist_id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },
  
  SetColumnValueByGUID: function(mediaGUID, strColumn, strValue)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.addQuery("UPDATE \"" + this.m_strName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },

  SetColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount, aValues)
  {
    if(this.m_queryObject != null ||
       nColumnCount != nValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE \"" + this.m_strName + "\" SET ";
      var i = 0;
      for(; i < nColumnCount; i++)
      {
        aValues[i] = aValues[i].replace(/"/g, "\"\"");
        strQuery += aColumns[i] + " = \"" + aValues[i] + "\"";
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }

    return;
  },
  
  SetColumnValuesByGUID: function(mediaGUID, nColumnCount, aColumns, nValueCount, aValues)
  {
    if(this.m_queryObject != null ||
       nColumnCount != nValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE \"" + this.m_strName + "\" SET ";
      var i = 0;
      for(; i < nColumnCount; i++)
      {
        aValues[i] = aValues[i].replace(/"/g, "\"\"");
        strQuery += aColumns[i] + " = \"" + aValues[i] + "\"";
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE playlist_uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }

    return;
  },
  
  SetName: function(strName)
  {
    this.m_strName = strName;
    return;
  },
  
  GetName: function()
  {
    return this.m_strName;
  },
  
  SetReadableName: function(strReadableName)
  {
    this.m_queryObject.resetQuery();
    
    strReadableName = strReadableName.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " SET readable_name = \"" + strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return;
  },
  
  GetReadableName: function()
  {
    var strReadableName = "";
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT readable_name FROM " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    
    if(resObj.GetRowCount())
      strReadableName = resObj.GetRowCell(0, 0);    
    
    return strReadableName;
  },

  SetPeriodicity: function(nPeriodicity, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      this.m_queryObject.addQuery("UPDATE \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" SET periodicity  = \"" + nPeriodicity + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },
  
  GetPeriodicity: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT periodicity FROM \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      if(resObj.GetRowCount > 0)
      {
        return resObj.GetRowCell(0, 0);
      }
    }
    
    return 0;
  },
  
  SetURL: function(strURL, bWillRunLater)
  {
  if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      this.m_queryObject.addQuery("UPDATE " + DYNAMICPLAYLIST_LIST_TABLE_NAME + " SET url = \"" + strURL + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },
  
  GetURL: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT url FROM \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      if(resObj.GetRowCount > 0)
      {
        return resObj.GetRowCell(0, 0);
      }
    }

    return "";
  },

  SetLastUpdateTime: function()
  {
    if(this.m_queryObject != null)
    {
      var dNow = new Date();
      
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("UPDATE \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" SET last_update = " + dNow.getTime() + " WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }    
  },
  
  GetLastUpdateTime: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT last_update FROM \"" + DYNAMICPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      if(resObj.GetRowCount > 0)
      {
        return resObj.GetRowCell(0, 0);
      }
    }

    return 0;
  },

  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_DYNAMICPLAYLIST_IID) && 
          !iid.equals(SONGBIRD_PLAYLIST_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbDynamicPlaylistModule
 * \brief 
 */
var sbDynamicPlaylistModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_DYNAMICPLAYLIST_CID, 
                                      SONGBIRD_DYNAMICPLAYLIST_CLASSNAME, 
                                      SONGBIRD_DYNAMICPLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_DYNAMICPLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbDynamicPlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbDynamicPlaylistFactory
 * \brief 
 */
var sbDynamicPlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_DYNAMICPLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CDynamicPlaylist()).QueryInterface(iid);
    }
}; //sbDynamicPlaylistFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbDynamicPlaylistModule;
} //NSGetModule