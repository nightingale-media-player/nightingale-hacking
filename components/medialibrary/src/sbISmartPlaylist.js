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
// sbISmartPlaylist Object
//

const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

const SONGBIRD_SMARTPLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/SmartPlaylist;1";
const SONGBIRD_SMARTPLAYLIST_CLASSNAME = "Songbird Smart Playlist Interface"
const SONGBIRD_SMARTPLAYLIST_CID = Components.ID("{a81b9c4d-e578-4737-840f-43d404c98423}");
const SONGBIRD_SMARTPLAYLIST_IID = Components.interfaces.sbISmartPlaylist;

const SMARTPLAYLIST_LIST_TABLE_NAME = "smartplaylist_list";

function CSmartPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
}

CSmartPlaylist.prototype.constructor = CSmartPlaylist;

/* the CPlaylist class def */
CSmartPlaylist.prototype = 
{
  m_strName: "",
  m_strReadableName: "",
  m_queryObject: null,
  m_internalQueryObject: null,
  m_strQuery: "",
  
  SetQueryObject: function(queryObj)
  {
    this.m_queryObject = queryObj;
    this.m_internalQueryObject.SetDatabaseGUID(queryObj.GetDatabaseGUID());
    return;
  },
  
  GetQueryObject: function()
  {
    return this.m_queryObject;
  },
  
  AddByGUID: function(mediaGUID, serviceGUID, nPosition, bReplace, bWillRunLater)
  {
    return false;
  },
  
  RemoveByGUID: function(mediaGUID, bWillRunLater)
  {
    return false;
  },
  
  RemoveByIndex: function(mediaIndex, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();

      this.m_queryObject.AddQuery("DELETE FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  MoveByGUID: function(mediaGUID, nPosition)
  {
    return false;
  },
  
  MoveByIndex: function(mediaIndex, nPosition)
  {
    return false;
  },

  FindByGUID: function(mediaGUID)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.ResetQuery();
      this.m_internalQueryObject.AddQuery("SELECT id FROM \"" + this.m_strName + "\" WHERE playlist_uuid = \"" + mediaGUID + "\"");
      
      this.m_internalQueryObject.Execute();
      this.m_internalQueryObject.WaitForCompletion();
      
      var resObj = this.m_internalQueryObject.GetResultObject();
      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
    
    return -1;
  },
  
  FindByIndex: function(mediaIndex)
  {
    if(this.m_internalQueryObject != null)
    {
      this.m_internalQueryObject.ResetQuery();
      this.m_internalQueryObject.AddQuery("SELECT uuid FROM \"" + this.m_strName + "\" WHERE playlist_id = \"" + nIndex + "\"");
      
      this.m_internalQueryObject.Execute();
      this.m_internalQueryObject.WaitForCompletion();
      
      var resObj = this.m_internalQueryObject.GetResultObject();
      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
    
    return "";
  },

  GetColumnInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT * FROM library_desc");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
    }
  },
  
  SetColumnInfo: function(strColumn, strReadableName, isVisible, defaultVisibility, isMetadata, sortWeight, colWidth, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strQuery = "UPDATE \"" + this.m_strName + "_desc\" SET ";
      strQuery += "readable_name = \"" + strReadableName + "\", ";
      strQuery += "is_visible = \"" + isVisible ? 1: 0 + "\", ";
      strQuery += "default_visibility = \"" + defaultVisibility ? 1 : 0 + "\", ";
      strQuery += "is_metadata = \"" + isMetadata ? 1 : 0 + "\", ";
      strQuery += "sort_weight = \"" + sortWeight + "\", ";
      strQuery += "width = \"" + colWidth + "\" ";
      strQuery += "WHERE column_name = \"" + strColumn + "\"";
      
      this.m_queryObject.AddQuery(strQuery);
      
      strQuery = "UPDATE \"library_desc\" SET ";
      strQuery += "readable_name = \"" + strReadableName + "\", ";
      strQuery += "is_visible = \"" + isVisible ? 1: 0 + "\", ";
      strQuery += "default_visibility = \"" + defaultVisibility ? 1 : 0 + "\", ";
      strQuery += "is_metadata = \"" + isMetadata ? 1 : 0 + "\", ";
      strQuery += "sort_weight = \"" + sortWeight + "\", ";
      strQuery += "width = \"" + colWidth + "\" ";
      strQuery += "WHERE column_name = \"" + strColumn + "\"";
      
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }    
  },

  GetTableInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("PRAGMA table_info(\"" + this.m_strName + "\")");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
    }
  },
  
  AddColumn: function(strColumn, strDataType)
  {
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
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT COUNT(id) FROM \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      
      return resObj.GetRowCell(0, 0);
    }
    
    return 0;
  },  
  
  GetEntry: function(nEntry)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT * FROM \"" + this.m_strName + "\" WHERE id = \"" + nEntry + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();

      return 1;
    }
    
    return 0;
  },

  GetAllEntries: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT * FROM \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCount();
    }
    
    return 0;
  },

  GetColumnValueByIndex: function(mediaIndex, strColumn)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + this.m_strName + "\" WHERE id = \"" + mediaIndex + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);
    }
        
    return "";
  },
  
  GetColumnValueByGUID: function(mediaGUID, strColumn)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + this.m_strName + "\" WHERE uuid = \"" + mediaGUID + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

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
      this.m_queryObject.ResetQuery();
      
      var i = 0;
      for( ; i < nColumnCount; i++)
      {
        strQuery += aColumns[i];
        
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM \"" + this.m_strName + "\" WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
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
      this.m_queryObject.ResetQuery();
      
      var i = 0;
      for( ; i < nColumnCount; i++)
      {
        strQuery += aColumns[i];
        
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM \"" + this.m_strName + "\" WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
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
        this.m_queryObject.ResetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.AddQuery("UPDATE library SET " + strColumn + " = \"" + strValue + "\" WHERE id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  SetColumnValueByGUID: function(mediaGUID, strColumn, strValue)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.AddQuery("UPDATE library SET " + strColumn + " = \"" + strValue + "\" WHERE uuid = \"" + mediaGUID + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
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
        this.m_queryObject.ResetQuery();
      
      var strQuery = "UPDATE library SET ";
      var i = 0;
      for(; i < nColumnCount; i++)
      {
        aValues[i] = aValues[i].replace(/"/g, "\"\"");
        strQuery += aColumns[i] + " = \"" + aValues[i] + "\"";
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
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
        this.m_queryObject.ResetQuery();
      
      var strQuery = "UPDATE library SET ";
      var i = 0;
      for(; i < nColumnCount; i++)
      {
        aValues[i] = aValues[i].replace(/"/g, "\"\"");
        strQuery += aColumns[i] + " = \"" + aValues[i] + "\"";
        if(i < nColumnCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
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
    this.m_queryObject.ResetQuery();
    
    strReadableName = strReadableName.replace(/"/g, "\"\"");
    this.m_queryObject.AddQuery("UPDATE " + SMARTPLAYLIST_LIST_TABLE_NAME + " SET readable_name = \"" + strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    return;
  },
  
  GetReadableName: function()
  {
    var strReadableName = "";
    this.m_queryObject.ResetQuery();
    this.m_queryObject.AddQuery("SELECT readable_name FROM " + SMARTPLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    var resObj = this.m_queryObject.GetResultObject();
    
    if(resObj.GetRowCount())
      strReadableName = resObj.GetRowCell(0, 0);    
    
    return strReadableName;
  },

  SetLibrary: function(strLibrary, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strColumn = "library";
      var strValue = strLibrary;
      
      this.m_queryObject.AddQuery("UPDATE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  GetLibrary: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumn = "library";
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },

  SetLimitAndType: function(nLimit, strLimitType, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strColumnA = "limit_value";
      var strColumnB = "limit_type";
      var strValueA = nLimit;
      var strValueB = strLimitType;
      
      this.m_queryObject.AddQuery("UPDATE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET " + strColumnA + " = \"" + strValueA + "\", " + strColumnB + " = \"" + strValueB + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  GetLimit: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumn = "limit_value";
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },
  
  GetLimitType: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumn = "limit_type";
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },
  
  GetLimitAndType: function(nLimit, strLimitType)
  {
    nLimit.value = 0;
    strLimitType.value = "";
    
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumnA = "limit_value";
      var strColumnB = "limit_type";
      this.m_queryObject.AddQuery("SELECT " + strColumn + ", " + strColumnB + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
      {
        nLimit.value = resObj.GetRowCell(0, 0);
        strLimitType.value = resObj.GetRowCell(0, 1);
      }
    }
  },
  
  SetSelectedBy: function(strSelectedBy, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strColumn = "selected_by";
      var strValue = strSelectedBy;
      
      this.m_queryObject.AddQuery("UPDATE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  GetSelectedBy: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumn = "selected_by";
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },

  SetMatch: function(strMatchOn, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strColumn = "match_on";
      var strValue = strMatchOn;
      
      this.m_queryObject.AddQuery("UPDATE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  GetMatch: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      var strColumn = "match_on";
      this.m_queryObject.AddQuery("SELECT " + strColumn + " FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return "";
  },

  AddConstraint: function(strMetadata, strConstraint, strConstraintValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strQuery = "INSERT INTO \"" + this.m_strName + "_constraints\" (column_name, constraint_name, constraint_value) VALUES (\"";
      strQuery += strMetadata + "\", \"";
      strQuery += strConstraint + "\", \"";
      strQuery += strConstraintValue + "\")";
      
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
  },
  
  GetConstraintCount: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT COUNT(id) FROM \"" + this.m_strName + "_constraints\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();

      if(resObj.GetRowCount())
        return resObj.GetRowCell(0, 0);    
    }
      
    return 0;
  },
  
  GetAllConstraints: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT * FROM \"" + this.m_strName + "_constraints\"" );
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCount();
    }
    
    return 0;
  },
  
  GetConstraint: function(nIndex)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT * FROM \"" + this.m_strName + "_constraints\" WHERE id = " + nIndex);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      if(resObj.GetRowCount())
        return true;
    }
    
    return false;
  },
  
  ReplaceConstraint: function(nIndex, strMetadata, strConstraint, strConstraintValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strQuery = "INSERT OR REPLACE INTO \"" + this.m_strName + "_constraints\" VALUES (\"";
      strQuery += nIndex + "\", \"";
      strQuery += strMetadata + "\", \"";
      strQuery += strConstraint + "\", \"";
      strQuery += strConstraintValue + "\")";
      
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
  },
  
  RemoveConstraint: function(nIndex, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
        
      this.m_queryObject.AddQuery("DELETE FROM \"" + this.m_strName + "_constraints\" WHERE id = " + nIndex);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },

  RemoveAllConstraints: function(bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
        
      this.m_queryObject.AddQuery("DELETE FROM \"" + this.m_strName + "_constraints\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },

  SetQuery: function(strQuery, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strColumn = "query";
      var strValue = strQuery;
      
      this.m_queryObject.AddQuery("UPDATE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },
  
  GetQuery: function()
  {
    return this.m_strQuery;
  },
  
  RebuildPlaylist: function()
  {
    if(this.m_queryObject != null)
    {
      var resObj = null;
      var strQuery = "";
      var strTheSmartQuery = "SELECT * FROM library";
 
      strQuery = "SELECT * FROM \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" WHERE name = \"" + this.m_strName + "\"";
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery(strQuery);
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();

      resObj = this.m_queryObject.GetResultObject();
      
      var library = resObj.GetRowCellByColumn(0, "library");
      var limitValue = resObj.GetRowCellByColumn(0, "limit_value");
      var limitType = resObj.GetRowCellByColumn(0, "limit_type");
      var selectedBy = resObj.GetRowCellByColumn(0, "selected_by");
      var match = resObj.GetRowCellByColumn(0, "match_on");
      
      if(match == "any") match = "OR";
      else if(match == "all") match = "AND";
      
      strQuery = "SELECT * FROM \"" + this.m_strName + "_constraints\"";
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery(strQuery);
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      resObj = this.m_queryObject.GetResultObject();
     
      var rowCount = resObj.GetRowCount();
      
      if(rowCount)
        strTheSmartQuery += " WHERE ";
      
      var constraint = "", constraintValue = "", i = 0;
      for(; i < rowCount; ++i)
      {
        strTheSmartQuery += resObj.GetRowCellByColumn(i, "column_name") + " ";
        constraint = resObj.GetRowCellByColumn(i, "constraint_name");
        constraintValue = resObj.GetRowCellByColumn(i, "constraint_value");
        
        switch(constraint)
        {
          case "contains": strTheSmartQuery += "LIKE \"%" + constraintValue + "%\""; break;
          case "does not contain": strTheSmartQuery += "NOT LIKE \"%" + constraintValue + "%\""; break;
          case "is": strTheSmartQuery += "= \"" + constraintValue + "\""; break;
          case "is not": strTheSmartQuery += "!= \"" + constraintValue + "\""; break;
          case "starts with": strTheSmartQuery += "LIKE \"" + constraintValue + "%\""; break;
          case "ends with": strTheSmartQuery += "LIKE \"%" + constraintValue + "\""; break;
        }
        
        if(i != rowCount - 1)
          strTheSmartQuery += " " + match + " ";
      }
      
      if(limitValue != 0)
      {
      }
      
      this.m_strQuery = strTheSmartQuery;
      
      this.m_queryObject.ResetQuery();
      
      strQuery = "UPDATE OR IGNORE \"" + SMARTPLAYLIST_LIST_TABLE_NAME + "\" SET query = x\"" + strTheSmartQuery + "\" WHERE name = \"" + this.m_strName + "\"";
      
      this.m_queryObject.AddQuery(strQuery);
      
      this.m_queryObject.AddQuery("DROP VIEW \"" + this.m_strName + "\"")
      this.m_queryObject.AddQuery("CREATE VIEW \"" + this.m_strName + "\" AS " + this.m_strQuery);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      return true; 
    }
    
    return false;    
  },
  
  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_SMARTPLAYLIST_IID) && 
          !iid.equals(SONGBIRD_PLAYLIST_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbSmartPlaylistModule
 * \brief 
 */
var sbSmartPlaylistModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_SMARTPLAYLIST_CID, 
                                      SONGBIRD_SMARTPLAYLIST_CLASSNAME, 
                                      SONGBIRD_SMARTPLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_SMARTPLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbSmartPlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbSmartPlaylistModule

/**
 * \class sbSmartPlaylistFactory
 * \brief 
 */
var sbSmartPlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_SMARTPLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CSmartPlaylist()).QueryInterface(iid);
    }
}; //sbSmartPlaylistFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbSmartPlaylistModule;
} //NSGetModule