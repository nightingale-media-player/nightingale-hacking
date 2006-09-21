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
// sbISmartPlaylist Object
//

const SONGBIRD_SMARTPLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/SmartPlaylist;1";
const SONGBIRD_SMARTPLAYLIST_CLASSNAME = "Songbird Smart Playlist Interface"
const SONGBIRD_SMARTPLAYLIST_CID = Components.ID("{a81b9c4d-e578-4737-840f-43d404c98423}");
const SONGBIRD_SMARTPLAYLIST_IID = Components.interfaces.sbISmartPlaylist;

function CSmartPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
  this.m_strPlaylistTableName = "smartplaylist_list";
}

CSmartPlaylist.prototype = new CPlaylistBase();
CSmartPlaylist.prototype.constructor = CSmartPlaylist;

/* the CSmartPlaylist class def */
CSmartPlaylist.prototype.m_strQuery = "";
  
CSmartPlaylist.prototype.addByGUID = function(mediaGUID, serviceGUID, nPosition, bReplace, bWillRunLater) {
  return false;
};
  
CSmartPlaylist.prototype.removeByGUID = function(mediaGUID, bWillRunLater) {
  return false;
};
  
CSmartPlaylist.prototype.removeByIndex = function(mediaIndex, bWillRunLater) {
  return false;
};

CSmartPlaylist.prototype.moveByGUID = function(mediaGUID, nPosition) {
  return false;
};
  
CSmartPlaylist.prototype.moveByIndex = function(mediaIndex, nPosition) {
  return false;
};


CSmartPlaylist.prototype.getColumnInfo = function()
{
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT * FROM library_desc");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
  }
};
  
CSmartPlaylist.prototype.setColumnInfo = function(strColumn, 
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
    
    strQuery = "UPDATE \"library_desc\" SET ";
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


CSmartPlaylist.prototype.addColumn = function(strColumn, strDataType) {
  return;
};
  
CSmartPlaylist.prototype.deleteColumn = function(strColumn) {
  return;
};

CSmartPlaylist.prototype.setColumnValueByIndex = function(mediaIndex, strColumn, strValue) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    strValue = strValue.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE library SET " + strColumn + " = \"" + strValue + "\" WHERE id = \"" + mediaIndex + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CSmartPlaylist.prototype.setColumnValueByGUID = function(mediaGUID, strColumn, strValue) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    strValue = strValue.replace(/"/g, "\"\"");
    this.m_queryObject.addQuery("UPDATE library SET " + strColumn + " = \"" + strValue + "\" WHERE uuid = \"" + mediaGUID + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};

CSmartPlaylist.prototype.setColumnValuesByIndex = function(mediaIndex, nColumnCount, aColumns, nValueCount, aValues) {
  if(this.m_queryObject != null ||
     nColumnCount != nValueCount)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
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
    this.m_queryObject.addQuery(strQuery);
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }

  return;
};
  
CSmartPlaylist.prototype.setColumnValuesByGUID = function(mediaGUID, nColumnCount, aColumns, nValueCount, aValues) {
  if(this.m_queryObject != null ||
     nColumnCount != nValueCount)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
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
    this.m_queryObject.addQuery(strQuery);
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }

  return;
};
  
CSmartPlaylist.prototype.setLibrary = function(strLibrary, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strColumn = "library";
    var strValue = strLibrary;
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CSmartPlaylist.prototype.getLibrary = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumn = "library";
    this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return "";
};

CSmartPlaylist.prototype.setLimitAndType = function(nLimit, strLimitType, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strColumnA = "limit_value";
    var strColumnB = "limit_type";
    var strValueA = nLimit;
    var strValueB = strLimitType;
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName+ "\" SET " + strColumnA + " = \"" + strValueA + "\", " + strColumnB + " = \"" + strValueB + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CSmartPlaylist.prototype.getLimit = function()
{
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumn = "limit_value";
    this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return "";
};
  
CSmartPlaylist.prototype.getLimitType = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumn = "limit_type";
    this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return "";
};
  
CSmartPlaylist.prototype.getLimitAndType = function(nLimit, strLimitType) {
  nLimit.value = 0;
  strLimitType.value = "";
  
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumnA = "limit_value";
    var strColumnB = "limit_type";
    this.m_queryObject.addQuery("SELECT " + strColumn + ", " + strColumnB + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
    {
      nLimit.value = resObj.getRowCell(0, 0);
      strLimitType.value = resObj.getRowCell(0, 1);
    }
  }
};
  
CSmartPlaylist.prototype.setSelectedBy = function(strSelectedBy, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strColumn = "selected_by";
    var strValue = strSelectedBy;
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};

CSmartPlaylist.prototype.getSelectedBy = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumn = "selected_by";
    this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return "";
};

CSmartPlaylist.prototype.setMatch = function(strMatchOn, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strColumn = "match_on";
    var strValue = strMatchOn;
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CSmartPlaylist.prototype.getMatch = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    
    var strColumn = "match_on";
    this.m_queryObject.addQuery("SELECT " + strColumn + " FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return "";
};

CSmartPlaylist.prototype.addConstraint = function(strMetadata, strConstraint, strConstraintValue, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strQuery = "INSERT INTO \"" + this.m_strName + "_constraints\" (column_name, constraint_name, constraint_value) VALUES (\"";
    strQuery += strMetadata + "\", \"";
    strQuery += strConstraint + "\", \"";
    strQuery += strConstraintValue + "\")";
    
    this.m_queryObject.addQuery(strQuery);
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
};
  
CSmartPlaylist.prototype.getConstraintCount = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT COUNT(id) FROM \"" + this.m_strName + "_constraints\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();

    if(resObj.getRowCount())
      return resObj.getRowCell(0, 0);    
  }
    
  return 0;
};
  
CSmartPlaylist.prototype.getAllConstraints = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "_constraints\"" );
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    return resObj.getRowCount();
  }
  
  return 0;
};
  
CSmartPlaylist.prototype.getConstraint = function(nIndex) {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT * FROM \"" + this.m_strName + "_constraints\" WHERE id = " + nIndex);
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    if(resObj.getRowCount())
      return true;
  }
  
  return false;
};
  
CSmartPlaylist.prototype.replaceConstraint = function(nIndex, strMetadata, strConstraint, strConstraintValue, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strQuery = "INSERT OR REPLACE INTO \"" + this.m_strName + "_constraints\" VALUES (\"";
    strQuery += nIndex + "\", \"";
    strQuery += strMetadata + "\", \"";
    strQuery += strConstraint + "\", \"";
    strQuery += strConstraintValue + "\")";
    
    this.m_queryObject.addQuery(strQuery);
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
};
  
CSmartPlaylist.prototype.removeConstraint = function(nIndex, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
      
    this.m_queryObject.addQuery("DELETE FROM \"" + this.m_strName + "_constraints\" WHERE id = " + nIndex);
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};

CSmartPlaylist.prototype.removeAllConstraints = function(bWillRunLater)
{
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
      
    this.m_queryObject.addQuery("DELETE FROM \"" + this.m_strName + "_constraints\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};

CSmartPlaylist.prototype.setQuery = function(strQuery, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    var strColumn = "query";
    var strValue = strQuery;
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET " + strColumn + " = \"" + strValue + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CSmartPlaylist.prototype.getQuery = function() {
  return this.m_strQuery;
};
  
CSmartPlaylist.prototype.rebuildPlaylist = function() {
  if(this.m_queryObject != null)
  {
    var resObj = null;
    var strQuery = "";
    var strTheSmartQuery = "SELECT * FROM library";

    strQuery = "SELECT * FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"";
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery(strQuery);
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();

    resObj = this.m_queryObject.getResultObject();
    
    var library = resObj.getRowCellByColumn(0, "library");
    var limitValue = resObj.getRowCellByColumn(0, "limit_value");
    var limitType = resObj.getRowCellByColumn(0, "limit_type");
    var selectedBy = resObj.getRowCellByColumn(0, "selected_by");
    var match = resObj.getRowCellByColumn(0, "match_on");
    
    if(match == "any") match = "OR";
    else if(match == "all") match = "AND";
    
    strQuery = "SELECT * FROM \"" + this.m_strName + "_constraints\"";
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery(strQuery);
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    resObj = this.m_queryObject.getResultObject();
   
    var rowCount = resObj.getRowCount();
    
    if(rowCount)
      strTheSmartQuery += " WHERE ";
    
    var constraint = "", constraintValue = "", i = 0;
    for(; i < rowCount; ++i)
    {
      strTheSmartQuery += resObj.getRowCellByColumn(i, "column_name") + " ";
      constraint = resObj.getRowCellByColumn(i, "constraint_name");
      constraintValue = resObj.getRowCellByColumn(i, "constraint_value");
      
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
    
    this.m_queryObject.resetQuery();
    
    strQuery = "UPDATE OR IGNORE \"" + this.m_strPlaylistTableName+ "\" SET query = x\"" + strTheSmartQuery + "\" WHERE name = \"" + this.m_strName + "\"";
    
    this.m_queryObject.addQuery(strQuery);
    
    this.m_queryObject.addQuery("DROP VIEW \"" + this.m_strName + "\"")
    this.m_queryObject.addQuery("CREATE VIEW \"" + this.m_strName + "\" AS " + this.m_strQuery);
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    return true; 
  }
  
  return false;    
};
  
CSmartPlaylist.prototype.QueryInterface = function(iid) {
    if (!iid.equals(Components.interfaces.nsISupports) &&
        !iid.equals(SONGBIRD_SMARTPLAYLIST_IID) && 
        !iid.equals(SONGBIRD_PLAYLIST_IID))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
};
