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
// sbIPlaylist Object
//


const SONGBIRD_PLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/Playlist;1";
const SONGBIRD_PLAYLIST_CLASSNAME = "Songbird Playlist Interface"
const SONGBIRD_PLAYLIST_CID = Components.ID("{c3c120f4-5ab6-4402-8cab-9b8f1d788769}");
const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

const PLAYLIST_LIST_TABLE_NAME = "playlist_list";

function CPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
}

CPlaylist.prototype.constructor = CPlaylist;

/* the CPlaylist class def */
CPlaylist.prototype = 
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
      return resObj.getRowCell(0, 0);
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
      return resObj.getRowCell(0, 0);
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
      strQuery += "WHERE = \"" + strColumn + "\"";
      
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
      
      return resObj.getRowCell(0, 0);
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
      return resObj.getRowCount();
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

      if(resObj.getRowCount())
        return resObj.getRowCell(0, 0);
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

      if(resObj.getRowCount())
        return resObj.getRowCell(0, 0);    
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
      nValueCount = resObj.getColumnCount();
      
      for(var i = 0; i < nValueCount; i++)
      {
        aValues.push(resObj.getRowCell(0, i));
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
      nValueCount = resObj.getColumnCount();
      
      for(var i = 0; i < nValueCount; i++)
      {
        aValues.push(resObj.getRowCell(0, i));
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
    this.m_queryObject.addQuery("UPDATE " + PLAYLIST_LIST_TABLE_NAME + " SET readable_name = \"" + strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return;
  },
  
  GetReadableName: function()
  {
    var strReadableName = "";
    
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT readable_name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    
    if(resObj.getRowCount())
      strReadableName = resObj.getRowCell(0, 0);    
    
    return strReadableName;
  },

  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLIST_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbPlaylistModule
 * \brief 
 */
var sbPlaylistModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLIST_CID, 
                                      SONGBIRD_PLAYLIST_CLASSNAME, 
                                      SONGBIRD_PLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbPlaylistFactory
 * \brief 
 */
var sbPlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_PLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylist()).QueryInterface(iid);
    }
}; //sbPlaylistFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistModule;
} //NSGetModule