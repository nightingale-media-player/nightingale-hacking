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
// CPlaylistBase Object
//

function CPlaylistBase()
{
}

/* the CPlaylistBase class def */
CPlaylistBase.prototype = 
{
  m_strPlaylistTableName: "",
  m_strName: "",
  m_strReadableName: "",
  
  m_queryObject: null,
  m_internalQueryObject: null,
  
  setQueryObject: function(queryObj)
  {
    this.m_queryObject = queryObj; 
    this.m_internalQueryObject.setDatabaseGUID(queryObj.getDatabaseGUID());
    
    return;
  },
  
  getQueryObject: function()
  {
    return this.m_queryObject;
  },
  
  addByGUID: function(mediaGUID, serviceGUID, nPosition, bReplace, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      if(bReplace)
      {
        var index = this.findByGUID(mediaGUID);
        if(index != -1)
          return true;
      }
      
      var query_str = "INSERT INTO \"" + this.m_strName + "\" (playlist_uuid, playlist_service_uuid) VALUES (\"" + mediaGUID + "\", \"" + serviceGUID + "\")"
      this.m_queryObject.addQuery(query_str);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  removeByGUID: function(mediaGUID, bWillRunLater)
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
  
  removeByIndex: function(mediaIndex, bWillRunLater)
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
  
  moveByGUID: function(mediaGUID, nPosition)
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },
  
  moveByIndex: function(mediaIndex, nPosition)
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },

  findByGUID: function(mediaGUID)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.resetQuery();
      this.m_internalQueryObject.addQuery("SELECT playlist_id FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      this.m_internalQueryObject.execute();
      this.m_internalQueryObject.waitForCompletion();
      
      var resObj = this.m_internalQueryObject.getResultObject();
      if (resObj.getRowCount() > 0)
        return resObj.getRowCell(0, 0);
    }
    
    return -1;
  },
  
  findByIndex: function(mediaIndex)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.resetQuery();
      this.m_internalQueryObject.addQuery("SELECT playlist_uuid FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + nIndex + "\"");
      
      this.m_internalQueryObject.execute();
      this.m_internalQueryObject.waitForCompletion();
      
      var resObj = this.m_internalQueryObject.getResultObject();
      if (resObj.getRowCount() > 0)
        return resObj.getRowCell(0, 0);
    }
    
    return "";
  },

  getColumnInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "_desc\" UNION SELECT * FROM library_desc ORDER BY sort_weight, column_name ASC");
      
      var exec = this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  },
  
  setColumnInfo: function(strColumn, 
    strReadableName, 
    isVisible, 
    defaultVisibility, 
    isMetadata, 
    sortWeight, 
    colWidth,
    dataType,
    readOnly,
    bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE \"" + this.m_strName + "_desc\" SET ";
      strQuery += "readable_name = \"" + strReadableName + "\", ";
      strQuery += "is_visible = \"" + (isVisible ? 1: 0) + "\", ";
      strQuery += "default_visibility = \"" + (defaultVisibility ? 1 : 0) + "\", ";
      strQuery += "is_metadata = \"" + (isMetadata ? 1 : 0) + "\", ";
      strQuery += "sort_weight = \"" + sortWeight + "\", ";
      strQuery += "width = \"" + colWidth + "\", ";
      strQuery += "type = \"" + dataType + "\", ";
      strQuery += "readonly = \"" + (readOnly ? 1 : 0) + "\" ";
      strQuery += " WHERE column_name = \"" + strColumn + "\" ";
      
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }    
  },

  getTableInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("PRAGMA table_info(\"" + this.m_strName + "\")");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  },
  
  addColumn: function(strColumn, strDataType)
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
  
  deleteColumn: function(strColumn)
  {
    return;
  },

  getNumEntries: function()
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
  
  getEntry: function(nEntry)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "\" LEFT JOIN library ON \"" + 
                                  this.m_strName + "\".playlist_uuid = library.uuid WHERE \"" + 
                                  this.m_strName + "\".playlist_id = \"" + nEntry + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();

      return 1;
    }
    
    return 0;
  },

  getAllEntries: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "\" LEFT JOIN library ON \"" + this.m_strName + "\".playlist_uuid = library.uuid");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      return resObj.getRowCount();
    }
    
    return 0;
  },

  getColumnValueByIndex: function(mediaIndex, strColumn)
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
  
  getColumnValueByGUID: function(mediaGUID, strColumn)
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

  getColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount)
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
  
  getColumnValuesByGUID: function(mediaGUID, nColumnCount, aColumns, nValueCount)
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
  
  setColumnValueByIndex: function(mediaIndex, strColumn, strValue)
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
  
  setColumnValueByGUID: function(mediaGUID, strColumn, strValue)
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

  setColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount, aValues)
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
  
  setColumnValuesByGUID: function(mediaGUID, nColumnCount, aColumns, nValueCount, aValues)
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
  
  setName: function(strName)
  {
    this.m_strName = strName;
    return;
  },
  
  getName: function()
  {
    return this.m_strName;
  },
  
  setReadableName: function(strReadableName)
  {
    this.m_queryObject.resetQuery();
    
    strReadableName = strReadableName.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE " + this.m_strPlaylistTableName + " SET readable_name = \"" + strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return;
  },
  
  getReadableName: function()
  {
    var strReadableName = "";
    
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT readable_name FROM " + this.m_strPlaylistTableName + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    
    if(resObj.getRowCount())
      strReadableName = resObj.getRowCell(0, 0);    
    
    return strReadableName;
  },

  setDescription: function(strDescription)
  {
    this.m_queryObject.resetQuery();
    
    strDescription = strDescription.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE " + this.m_strPlaylistTableName +
                                " SET description = \"" + strDescription +
                                "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return;
  },
  
  getDescription: function()
  {
    var strDescription = "";
    
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT description FROM " + this.m_strPlaylistTableName +
                                " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    
    if(resObj.getRowCount())
      strDescription = resObj.getRowCell(0, 0);    
    
    return strDescription;
  },

  setType: function(strType)
  {
    this.m_queryObject.resetQuery();
    
    strType = strType.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE " + this.m_strPlaylistTableName +
                                " SET type = \"" + strType +
                                "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return;
  },
  
  getType: function()
  {
    var strType = "";
    
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT type FROM " + this.m_strPlaylistTableName +
                                " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    
    if(resObj.getRowCount())
      strType = resObj.getRowCell(0, 0);    
    
    return strType;
  }
};
