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
// sbIMediaLibrary Object
//


const SONGBIRD_MEDIALIBRARY_CONTRACTID = "@songbirdnest.com/Songbird/MediaLibrary;1";
const SONGBIRD_MEDIALIBRARY_CLASSNAME = "Songbird Media Library Interface";
const SONGBIRD_MEDIALIBRARY_CID = Components.ID("{05eceb55-7c84-4341-93d3-d19ed3620d13}");
const SONGBIRD_MEDIALIBRARY_IID = Components.interfaces.sbIMediaLibrary;

const LIBRARY_TABLE_NAME = "library";
const LIBRARY_DESC_TABLE_NAME = "library_desc";

// If you change LIBRARY_DESC, remember to change PLAYLIST_DESC!!!!
const LIBRARY_DESC_TABLE_CREATE = "CREATE TABLE library_desc (column_name TEXT UNIQUE, readable_name TEXT, is_visible INTEGER(0, 1) DEFAULT 0, default_visibility INTEGER(0, 1) DEFAULT 0, is_metadata INTEGER(0, 1) DEFAULT 0, sort_weight INTEGER DEFAULT 0, width INTEGER DEFAULT -1, type TEXT DEFAULT 'text', readonly INTEGER(0,1) DEFAULT 0)";
const LIBRARY_TABLE_CREATE = "CREATE TABLE library (id INTEGER PRIMARY KEY, uuid BLOB UNIQUE NOT NULL, service_uuid BLOB NOT NULL, url TEXT UNIQUE DEFAULT '', content_type TEXT DEFAULT '', length TEXT DEFAULT '0', artist TEXT DEFAULT '', title TEXT DEFAULT '', album TEXT DEFAULT '', genre TEXT DEFAULT '', composer TEXT DEFAULT '', producer TEXT DEFAULT '', rating INTEGER DEFAULT 0, track_no INTEGER DEFAULT 0, track_total INTEGER DEFAULT 0, disc_no INTEGER DEFAULT 0, disc_total INTEGER DEFAULT 0, year INTEGER DEFAULT 0)";
const LIBRARY_TABLE_CREATE_INDEX = "CREATE index library_index ON library(id, uuid, url, content_type, length, artist, album, genre)";

function CMediaLibrary()
{
}

/* I actually need a constructor in this case. */
CMediaLibrary.prototype.constructor = CMediaLibrary;

/* the CMediaLibrary class def */
CMediaLibrary.prototype = 
{
  m_queryObject: null,

  m_UUIDGenerator: null,

  setQueryObject: function(queryObj)
  {
    this.m_queryObject = queryObj; 
    return;
  },
  
  getQueryObject: function()
  {
    return this.m_queryObject;
  },

  createDefaultLibrary: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      
      //Create the Library Description Table.
      this.m_queryObject.addQuery(LIBRARY_DESC_TABLE_CREATE);
      
      //Create the Library Table.
      this.m_queryObject.addQuery(LIBRARY_TABLE_CREATE);
      //Create the Library Index.
      this.m_queryObject.addQuery(LIBRARY_TABLE_CREATE_INDEX);

      // Strings beginning with & are translated in the UI.
      var id = ( "&metadata.id" );
      var row_id = ( "&metadata.row_id" );
      var uuid = ( "&metadata.uuid" );
      var service_uuid = ( "&metadata.service_uuid" );
      var url = ( "&metadata.url" );
      var content_type = ( "&metadata.content_type" );
      var length = ( "&metadata.length" );
      var artist = ( "&metadata.artist" );
      var title = ( "&metadata.title" );
      var album = ( "&metadata.album" );
      var genre = ( "&metadata.genre" );
      var composer = ( "&metadata.composer" );
      var producer = ( "&metadata.producer" );
      var rating = ( "&metadata.rating" );
      var track_no = ( "&metadata.track_no" );
      var track_total = ( "&metadata.track_total" );
      var disc_no = ( "&metadata.disc_no" );
      var disc_total = ( "&metadata.disc_total" );
      var year = ( "&metadata.year" );
      
      //Fill the Library Description Table.
      //Here's the library description table schema for quick reference.
      /*
      
      In order, from 0 to 7.
            
      0. column name, ie. "artist"
      1. readable name, ie. "Artist"
      2. is this column ever visible?, Boolean, 1 (True) or 0 (False).
      3. by default, is this column visible? Boolean, 1 (True) or 0 (False).
      4. is this a metadata column? Boolean, 1 (True) or 0 (False).
      5. default sort weight, signed integer value.
      6. default column width, in pixels, signed integer value.
      7. preferred data type of column (this *doesn't* mean the column *HAS* to have the sqlite type that matches the field type you want)
      Some example data types: text, numeric, decimal, boolean.
      8. is column read only to the user?

      */
      
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"id\", \"" + id + "\", 1, 0, 0, 0, -1, 'numeric', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"row_id\", \"" + row_id + "\", 1, 1, 0, -10000, 4, 'numeric', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"uuid\", \"" + uuid + "\", 1, 0, 0, 0, -1, 'text', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"service_uuid\", \"" + service_uuid + "\", 1, 0, 0, 0, -1, 'text', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"url\", \"" + url + "\", 1, 0, 1, 0, -1, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"content_type\", \"" + content_type + "\", 1, 0, 1, 0, -1, 'text', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"length\", \"" + length + "\", 1, 1, 1, -8000, 4, 'numeric', 1)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"artist\", \"" + artist + "\", 1, 1, 1, -7000, 25, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"title\", \"" + title + "\", 1, 1, 1, -9000, 60, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"album\", \"" + album + "\", 1, 1, 1, -6000, 25, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"genre\", \"" + genre + "\", 1, 1, 1, -5000, 10, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"composer\", \"" + composer + "\", 1, 0, 1, 0, -1, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"producer\", \"" + producer + "\", 1, 0, 1, 0, -1, 'text', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"rating\", \"" + rating + "\", 1, 1, 1, 1000, 10, 'numeric', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"track_no\", \"" + track_no + "\", 1, 0, 1, 0, -1, 'numeric', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"track_total\", \"" + track_total + "\", 1, 0, 1, 0, -1, 'numeric', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"disc_no\", \"" + disc_no + "\", 1, 0, 1, 0, -1, 'numeric', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"disc_total\", \"" + disc_total + "\", 1, 0, 1, 0, -1, 'numeric', 0)");
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"year\", \"" + year + "\", 1, 0, 1, 0, -1, 'numeric', 0)");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
    
    return;
  },

  addMedia: function(strMediaURL, nMetaKeyCount, aMetaKeys, nMetadataValueCount, aMetadataValues, bCheckForUniqueFileName, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if (!this.m_UUIDGenerator)
        this.m_UUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
      var guid = this.m_UUIDGenerator.generateUUID();
      var dbguid = this.m_queryObject.getDatabaseGUID();
      var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
      var strQuery = "SELECT uuid FROM library WHERE ";
      
      if(bCheckForUniqueFileName)
      {
        var filenamePos = strMediaURL.lastIndexOf("/") + 1;
        if(!filenamePos) filenamePos = strMediaURL.lastIndexOf("\\") + 1;
        
        if(filenamePos)        
        {
          var filename = strMediaURL.substring(filenamePos);
          if(filename != "")
            strQuery += "url LIKE \"%" + filename + "\"";
        }
      }
      else
      {
        strQuery += "url = \"" + strMediaURL + "\"";
      }
      
      aDBQuery.setAsyncQuery(true);
      aDBQuery.setDatabaseGUID(dbguid);
      
      aDBQuery.resetQuery();
      aDBQuery.addQuery(strQuery);
      
      aDBQuery.execute();
      aDBQuery.waitForCompletion();

      var resObj = aDBQuery.getResultObject();
      if(resObj.getRowCount() > 0)
      {
        guid = resObj.getRowCell(0, 0);
        if(guid)
        {
          //strQuery = "UPDATE " + LIBRARY_TABLE_NAME + " SET url = \"" + strMediaURL + "\" WHERE uuid = \"" + guid + "\"";
          return guid;
        }
      }
      else
      {
        var i = 0;
        strQuery = "INSERT OR REPLACE INTO " + LIBRARY_TABLE_NAME + " (uuid, service_uuid, url";
        for(; i < nMetaKeyCount; i++)
        {
          strQuery += ", ";
          strQuery += aMetaKeys[i];
        }
        
        strQuery += ") VALUES (\"" + guid + "\", \"" + dbguid + "\", \"" + strMediaURL + "\"";
        
        for(i = 0; i < nMetadataValueCount; i++)
        {
          strQuery += ", ";
          aMetadataValues[i] = aMetadataValues[i].replace(/"/g, "\"\"");
          strQuery += "\"" + aMetadataValues[i] + "\"";
        }
        
        strQuery += ")";
      }
        
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
     
      return guid;
    }
    
    return "";
  },
  
  removeMediaByURL: function(strMediaURL, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
    
      this.m_queryObject.addQuery("DELETE FROM " + LIBRARY_TABLE_NAME + " WHERE url = \"" + strMediaURL + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  removeMediaByGUID: function(mediaGUID, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
        
      this.m_queryObject.addQuery("DELETE FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"");
      
      const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
      var aPlaylistManager = (new PlaylistManager()).QueryInterface(Components.interfaces.sbIPlaylistManager);
      aPlaylistManager.purgeTrackByGUIDFromPlaylists(mediaGUID, this.m_queryObject.getDatabaseGUID());
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  purgeDefaultLibrary : function( bWillRunLater )
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
        
      this.m_queryObject.addQuery("DELETE FROM " + LIBRARY_TABLE_NAME );
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  findByGUID: function(mediaGUID)
  {
    var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
    aDBQuery.setAsyncQuery(false);
    aDBQuery.setDatabaseGUID(this.m_queryObject.getDatabaseGUID());

    aDBQuery.addQuery("SELECT url FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"");
      
    aDBQuery.execute();
      
    var resObj = aDBQuery.getResultObject();
    return resObj.getRowCell(0, 0);
  },
  
  findByURL: function(strURL)
  {
    var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
    aDBQuery.setDatabaseGUID(this.m_queryObject.getDatabaseGUID());

    aDBQuery.addQuery("SELECT uuid FROM " + LIBRARY_TABLE_NAME + " WHERE url = \"" + strURL + "\"");
      
    aDBQuery.execute();
    aDBQuery.waitForCompletion();
      
    var resObj = aDBQuery.getResultObject();
    return resObj.getRowCell(0, 0);
  },

  addColumn: function(strColumn, strDataType)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("ALTER TABLE \"" + LIBRARY_TABLE_NAME + "\" ADD COLUMN \"" + strColumn + "\" " + strDataType);
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO \"" + LIBRARY_DESC_TABLE_NAME + "_desc\" (column_name) VALUES (\"" + strColumn + "\")");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }

    return;
  },
  
  deleteColumn: function(strColumn)
  {
    return;
  },

  getColumnInfo: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT * FROM " + LIBRARY_DESC_TABLE_NAME + " ORDER BY sort_weight, column_name ASC");
      
      this.m_queryObject.execute();
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
      
      var strQuery = "UPDATE " + LIBRARY_DESC_TABLE_NAME + " SET ";
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

  getMetadataFields: function(nMetadataFieldCount)
  {
    var aMetadataFields = new Array();
    nMetadataFieldCount.value = 0;
    
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT column_name FROM " + LIBRARY_DESC_TABLE_NAME + " WHERE is_metadata = 1");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();

      var resObj = this.m_queryObject.getResultObject();
      nMetadataFieldCount.value = resObj.getRowCount();
      
      for(var i = 0; i < nMetadataFieldCount.value; i++)
      {
        aMetadataFields.push(resObj.getRowCell(i, 0));
      }
    }
    
    return aMetadataFields;
  },
  
  addMetadataField: function(strField, strFieldType, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
        
      this.m_queryObject.addQuery("ALTER TABLE \"" + LIBRARY_TABLE_NAME + "\" ADD COLUMN \"" + strColumn + "\" " + strDataType);
      this.m_queryObject.addQuery("INSERT OR REPLACE INTO \"" + LIBRARY_DESC_TABLE_NAME + "_desc\" (column_name, is_metadata) VALUES (\"" + strColumn + "\", \"1\")");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },
  
  deleteMetadataField: function(strField, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
    
      if(!bWillRunLater)
      {
      }
    }
    
    return;
  },

  getUniqueValuesByColumn: function(strField, nValueCount)
  {
    nValueCount.value = 0;
    var aValues = new Array();
    
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT DISTINCT(" + strField + ") FROM " + LIBRARY_TABLE_NAME + " ORDER BY " + strField + " ASC" );
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      nValueCount.value = resObj.getRowCount();
      
      for(var i = 0; i < nValueCount.value; i++)
      {
        aValues.push(resObj.getRowCell(i, 0));
      }
    }
    
    return aValues;
  },
  
  setValueByIndex: function(mediaIndex, strField, strValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.addQuery("UPDATE " + LIBRARY_TABLE_NAME + " SET " + strField + " = \"" + strValue + "\" WHERE id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  setValuesByIndex: function(mediaIndex, nMetaKeyCount, aMetaKeys, nMetaValueCount, aMetaValues, bWillRunLater)
  {
    if(this.m_queryObject != null ||
       nMetaKeyCount != nMetaValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE " + LIBRARY_TABLE_NAME + " SET ";
      var i = 0;
      for(; i < nMetaKeyCount; i++)
      {
        aMetaValues[i] = aMetaValues[i].replace(/"/g, "\"\"");
        strQuery += aMetaKeys[i] + " = \"" + aMetaValues[i] + "\"";
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  getValueByIndex: function(mediaIndex, strField)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT " + strField + " FROM " + LIBRARY_TABLE_NAME + " WHERE id = \"" + mediaIndex + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      
      if(resObj.getRowCount())
        return resObj.getRowCell(0, 0);
    }
    
    return "";
  },
  
  getValuesByIndex: function(mediaIndex, nMetaKeyCount, aMetaKeys, nMetadataValueCount)
  {
    nMetadataValueCount = 0;
    var aMetadataValues = new Array();
    
    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.resetQuery();
      
      var i = 0;
      for( ; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i];
        
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM " + LIBRARY_TABLE_NAME + " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      nMetadataValueCount = resObj.getColumnCount();
      
      for(var i = 0; i < nMetadataValueCount; i++)
      {
        aMetadataValues.push(resObj.getRowCell(0, i));
      }
    }
    
    return aMetadataValues;
  },
  
  setValueByGUID: function(mediaGUID, strField, strValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      strValue = strValue.replace(/"/g, "\"\"");
      this.m_queryObject.addQuery("UPDATE " + LIBRARY_TABLE_NAME + " SET " + strField + " = \"" + strValue + "\" WHERE uuid = \"" + mediaGUID + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
    }
    
    return;
  },

  setValuesByGUID: function(mediaGUID, nMetaKeyCount, aMetaKeys, nMetaValueCount, aMetaValues, bWillRunLater)
  {
    if(this.m_queryObject != null ||
       nMetaKeyCount != nMetaValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.resetQuery();
      
      var strQuery = "UPDATE " + LIBRARY_TABLE_NAME + " SET ";
      var i = 0;
      for(; i < nMetaKeyCount; i++)
      {
        aMetaValues[i] = aMetaValues[i].replace(/"/g, "\"\"");
        strQuery += aMetaKeys[i] + " = \"" + aMetaValues[i] + "\"";
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.execute();
        this.m_queryObject.waitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  getValueByGUID: function(mediaGUID, strField)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.resetQuery();
      this.m_queryObject.addQuery("SELECT " + strField + " FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"");
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      
      if(resObj.getRowCount())
        return resObj.getRowCell(0, 0);
    }
    
    return "";
  },
  
  getValuesByGUID: function(mediaGUID, nMetaKeyCount, aMetaKeys, nMetadataValueCount)
  {
    nMetadataValueCount = 0;
    var aMetadataValues = new Array();
    
    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.resetQuery();
      
      var i = 0;
      for( ; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i];
        
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.addQuery(strQuery);
      
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
      
      var resObj = this.m_queryObject.getResultObject();
      nMetadataValueCount = resObj.getColumnCount();
      
      for(var i = 0; i < nMetadataValueCount; i++)
      {
        aMetadataValues.push(resObj.getRowCell(0, i));
      }
    }
    
    return aMetadataValues;
  },

  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_MEDIALIBRARY_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
  
};

/**
 * \class sbMediaLibraryModule
 * \brief 
 */
var sbMediaLibraryModule = 
{
  //firstTime: true,
  
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    //if(this.firstTime)
    //{
      //this.firstTime = false;
      //throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
    //}
 
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_MEDIALIBRARY_CID, 
                                    SONGBIRD_MEDIALIBRARY_CLASSNAME, 
                                    SONGBIRD_MEDIALIBRARY_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_MEDIALIBRARY_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbMediaLibraryFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbMediaLibraryFactory
 * \brief 
 */
var sbMediaLibraryFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_MEDIALIBRARY_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CMediaLibrary()).QueryInterface(iid);
  }
}; //sbMediaLibraryFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{
  return sbMediaLibraryModule;
} //NSGetModule

