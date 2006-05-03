/*
 * Software Copyright © 2005 Pioneers of the Inevitable
 *
 * songbird@songbirdnest.com - <chirp, chirp>
 *
 * Q Public License v1.0
 * 
 * Copyright © 1999 Trolltech AS, Norway.
 * 
 * Everyone is permitted to copy and distribute this license document.
 * 
 * The intent of this license is to establish freedom to share and change the software 
 * regulated by this license under the open source model.
 * 
 * This license applies to any software containing a notice placed by the copyright holder 
 * saying that it may be distributed under the terms of the Q Public License version 1.0. 
 * Such software is herein referred to as the Software. This license covers modification  
 * and distribution of the Software, use of third-party application programs based on the 
 * Software, and development of free software which uses the Software.
 */
//
// sbIMediaLibrary Object
//
const SONGBIRD_MEDIALIBRARY_CONTRACTID = "@songbird.org/Songbird/MediaLibrary;1";
const SONGBIRD_MEDIALIBRARY_CLASSNAME = "Songbird Media Library Interface";
const SONGBIRD_MEDIALIBRARY_CID = Components.ID("{ca5c9b2c-e061-4c98-8333-9c839efc0d7f}");
const SONGBIRD_MEDIALIBRARY_IID = Components.interfaces.sbIMediaLibrary;

const LIBRARY_TABLE_NAME = "library";
const LIBRARY_DESC_TABLE_NAME = "library_desc";

const LIBRARY_DESC_TABLE_CREATE = "CREATE TABLE library_desc (column_name TEXT, readable_name TEXT, is_visible INTEGER(0, 1) DEFAULT 0, default_visibility INTEGER(0, 1) DEFAULT 0, is_metadata INTEGER(0, 1) DEFAULT 0, sort_weight INTEGER DEFAULT 0, width INTEGER DEFAULT 0)";
const LIBRARY_TABLE_CREATE = "CREATE TABLE library (id INTEGER PRIMARY KEY AUTOINCREMENT, uuid BLOB, service_uuid BLOB, url TEXT, content_type TEXT, length TEXT, artist TEXT, title TEXT, album TEXT, genre TEXT, composer TEXT, producer TEXT, rating INTEGER, track_no INTEGER, track_total INTEGER, disc_no INTEGER, disc_total INTEGER, year INTEGER)";

function CMediaLibrary()
{
}

/* the CMediaLibrary class def */
CMediaLibrary.prototype = 
{
  m_queryObject: null,
  
  SetQueryObject: function(queryObj)
  {
    this.m_queryObject = queryObj; 
    return;
  },
  
  GetQueryObject: function()
  {
    return this.m_queryObject;
  },

  CreateDefaultLibrary: function()
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      
      //Create the Library Description Table.
      this.m_queryObject.AddQuery(LIBRARY_DESC_TABLE_CREATE);
      
      //Create the Library Table.
      this.m_queryObject.AddQuery(LIBRARY_TABLE_CREATE);
      
      //Fill the Library Description Table.
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"id\", \"#\", 0, 0, 0, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"uuid\", \"Media UUID\", 1, 0, 0, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"service_uuid\", \"Service UUID\", 1, 0, 0, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"url\", \"Location\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"content_type\", \"Content Type\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"length\", \"Length\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"artist\", \"Artist\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"title\", \"Title\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"album\", \"Album\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"genre\", \"Genre\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"composer\", \"Composer\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"producer\", \"Producer\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"rating\", \"Rating\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"track_no\", \"Track No.\", 1, 1, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"track_total\", \"Total No. of Tracks\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"disc_no\", \"Disc No.\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"disc_total\", \"Total No. of Discs\", 1, 0, 1, 0)");
      this.m_queryObject.AddQuery("INSERT INTO " + LIBRARY_DESC_TABLE_NAME + " VALUES (\"year\", \"Year\", 1, 0, 1, 0)");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
    }
    
    return;
  },

  AddMedia: function(strMediaURL, nMetaKeyCount, aMetaKeys, nMetadataValueCount, aMetadataValues, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"];
      aUUIDGenerator = aUUIDGenerator.createInstance();
      aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
  
      var guid = aUUIDGenerator.generateUUID();
      
      var i = 0;
      var strQuery = "INSERT INTO " + LIBRARY_TABLE_NAME + " (uuid, service_uuid, url, ";
      for(; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i]
        
        if(i != nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += ") VALUES (\"" + guid + "\", \"" + this.m_queryObject.GetDatabaseGUID() + "\", \"" + strMediaURL + "\", ";
      
      for(i = 0; i < nMetadataValueCount; i++)
      {
        strQuery += "\"" + aMetadataValues[i] + "\"";
        
        if(i != nMetadataValueCount - 1)
          strQuery += ", ";
      }
      
      strQuery += ")";
      
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  RemoveMediaByURL: function(strMediaURL, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.AddQuery("DELETE FROM " + LIBRARY_TABLE_NAME + " WHERE url = \"" + strMediaURL + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  RemoveMediaByGUID: function(mediaGUID, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.AddQuery("DELETE FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID +"\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  FindByGUID: function(mediaGUID)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT url FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCell(0, 0);
    }
    
    return "";
  },
  
  FindByURL: function(strURL)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT uuid FROM " + LIBRARY_TABLE_NAME + " WHERE url = \"" + strURL + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCell(0, 0);
    }
    
    return "";
  },

  GetMetadataFields: function(nMetadataFieldCount)
  {
    var aMetadataFields = new Array();
    nMetadataFieldCount = 0;
    
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT column_names FROM " + LIBRARY_DESC_TABLE_NAME + " WHERE is_metadata = 1");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();

      var resObj = this.m_queryObject.GetResultObject();
      nMetadataFieldCount = resObj.GetRowCount();
      
      for(var i = 0; i < nMetadataFieldCount; i++)
      {
        aMetadataFields.push(resObj.GetRowCell(i, 0));
      }
    }
    
    return aMetadataFields;
  },
  
  AddMetadataField: function(strField, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(bWillRunLater)
      {
      }
    }
    
    return;
  },
  
  DeleteMetadataField: function(strField, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(bWillRunLater)
      {
      }
    }
    
    return;
  },
  
  SetValueByIndex: function(mediaIndex, strField, strValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery("UPDATE " + LIBRARY_TABLE_NAME + " SET " + strField + " = \"" + strValue + "\" WHERE id = \"" + mediaIndex + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  SetValuesByIndex: function(mediaIndex, nMetaKeyCount, aMetaKeys, nMetaValueCount, aMetaValues, bWillRunLater)
  {
    if(this.m_queryObject != null ||
       nMetaKeyCount != nMetaValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strQuery = "UPDATE " + LIBRARY_TABLE_NAME + " SET ";
      var i = 0;
      for(; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i] + " = \"" + aMetaValues[i] + "\"";
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  GetValueByIndex: function(mediaIndex, strField)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT " + strField + " FROM " + LIBRARY_TABLE_NAME + " WHERE id = \"" + mediaIndex + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCell(0, 0);
    }
    
    return "";
  },
  
  GetValuesByIndex: function(mediaIndex, nMetaKeyCount, aMetaKeys, nMetadataValueCount)
  {
    nMetadataValueCount = 0;
    var aMetadataValues = new Array();
    
    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.ResetQuery();
      
      var i = 0;
      for( ; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i];
        
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM " + LIBRARY_TABLE_NAME + " WHERE id = \"" + mediaIndex + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      nMetadataValueCount = resObj.GetColumnCount();
      
      for(var i = 0; i < nMetadataValueCount; i++)
      {
        aMetadataValues.push(resObj.GetRowCell(0, i));
      }
    }
    
    return aMetadataValues;
  },
  
  SetValueByGUID: function(mediaGUID, strField, strValue, bWillRunLater)
  {
    if(this.m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery("UPDATE " + LIBRARY_TABLE_NAME + " SET " + strField + " = \"" + strValue + "\" WHERE uuid = \"" + mediaGUID + "\"");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
    }
    
    return;
  },

  SetValuesByGUID: function(mediaGUID, nMetaKeyCount, aMetaKeys, nMetaValueCount, aMetaValues, bWillRunLater)
  {
    if(this.m_queryObject != null ||
       nMetaKeyCount != nMetaValueCount)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      var strQuery = "UPDATE " + LIBRARY_TABLE_NAME + " SET ";
      var i = 0;
      for(; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i] + " = \"" + aMetaValues[i] + "\"";
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  GetValueByGUID: function(mediaGUID, strField)
  {
    if(this.m_queryObject != null)
    {
      this.m_queryObject.ResetQuery();
      this.m_queryObject.AddQuery("SELECT " + strField + " FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"");
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      return resObj.GetRowCell(0, 0);
    }
    
    return;
  },
  
  GetValuesByGUID: function(mediaGUID, nMetaKeyCount, aMetaKeys, nMetadataValueCount)
  {
    nMetadataValueCount = 0;
    var aMetadataValues = new Array();
    
    if(this.m_queryObject != null)
    {
      var strQuery = "SELECT ";
      this.m_queryObject.ResetQuery();
      
      var i = 0;
      for( ; i < nMetaKeyCount; i++)
      {
        strQuery += aMetaKeys[i];
        
        if(i < nMetaKeyCount - 1)
          strQuery += ", ";
      }
      
      strQuery += " FROM " + LIBRARY_TABLE_NAME + " WHERE uuid = \"" + mediaGUID + "\"";
      this.m_queryObject.AddQuery(strQuery);
      
      this.m_queryObject.Execute();
      this.m_queryObject.WaitForCompletion();
      
      var resObj = this.m_queryObject.GetResultObject();
      nMetadataValueCount = resObj.GetColumnCount();
      
      for(var i = 0; i < nMetadataValueCount; i++)
      {
        aMetadataValues.push(resObj.GetRowCell(0, i));
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
