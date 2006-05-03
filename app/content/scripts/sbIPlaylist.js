//
// sbIPlaylist Object
//

const SONGBIRD_PLAYLIST_CONTRACTID = "@songbird.org/Songbird/Playlist;1";
const SONGBIRD_PLAYLIST_CLASSNAME = "Songbird Playlist Interface"
const SONGBIRD_PLAYLIST_CID = Components.ID('{C2B560D7-A145-4dd3-9040-F1682F17BCA6}');
const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

const PLAYLIST_LIST_TABLE_NAME = "playlist_list";

function CPlaylist()
{
}

/* the CPlaylist class def */
CPlaylist.prototype = 
{
  m_strName: "",
  m_strReadableName: "",
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
  
  AddByGUID: function(mediaGUID, serviceGUID, nPosition, bWillRunLater)
  {
    if(m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery("");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
      
      return true;
    }
    
    return false;
  },
  
  RemoveByGUID: function(mediaGUID, bWillRunLater)
  {
    if(m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery("");
      
      if(!bWillRunLater)
      {
        this.m_queryObject.Execute();
        this.m_queryObject.WaitForCompletion();
      }
            
      return true;
    }

    return false;
  },
  
  RemoveByIndex: function(mediaIndex, bWillRunLater)
  {
    if(m_queryObject != null)
    {
      if(!bWillRunLater)
        this.m_queryObject.ResetQuery();
      
      this.m_queryObject.AddQuery("");
      
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
    if(m_queryObject != null)
    {
      
      return true;
    }
    
    return -1;
  },
  
  FindByIndex: function(mediaIndex)
  {
    if(m_queryObject != null)
    {
      
      return true;
    }
    
    return "";
  },

  GetColumns: function(nColumnCount)
  {
    nColumnCount = 0;
    var aColumns = new Array();
    
    if(m_queryObject != null)
    {
      
    }
    
    return aColumns;
  }
  
  AddColumn(strColumn)
  {
    
  },
  
  DeleteColumn(strColumn)
  {
  },

  GetColumnValueByIndex: function(mediaIndex, strColumn)
  {
    if(m_queryObject != null)
    {
    }
    
    return "";
  },
  
  wstring GetColumnValueByGUID(in wstring mediaGUID, in wstring strColumn);

  void GetColumnValuesByIndex(in PRInt32 mediaIndex, in PRUint32 nColumnCount, [array, size_is(nColumnCount)] in wstring aColumns,
    out PRUint32 nValueCount, [array, size_is(nValueCount), retval] out wstring aValues);
  void GetColumnValuesByGUID(in wstring mediaGUID, in PRUint32 nColumnCount, [array, size_is(nColumnCount)] in wstring aColumns,
    out PRUint32 nValueCount, [array, size_is(nValueCount), retval] out wstring aValues);
  
  void SetColumnValueByIndex(in PRInt32 mediaIndex, in wstring strColumn, in wstring strValue);
  void SetColumnValueByGUID(in wstring mediaGUID, in wstring strColumn, in wstring strValue);

  void SetColumnValuesByIndex(in PRInt32 mediaIndex, in PRUint32 nColumnCount, [array, size_is(nColumnCount)] in wstring aColumns,
    out PRUint32 nValueCount, [array, size_is(nValueCount), retval] out wstring aValues);
  void SetColumnValuesByGUID(in wstring mediaGUID, in PRUint32 nColumnCount, [array, size_is(nColumnCount)] in wstring aColumns,
    out PRUint32 nValueCount, [array, size_is(nValueCount), retval] out wstring aValues);
  
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
    this.m_queryObject.AddQuery("UPDATE " + PLAYLIST_LIST_TABLE_NAME + " SET readable_name = \"" + this.m_strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    return;
  },
  
  GetReadableName: function()
  {
    var strReadableName = "";
    this.m_queryObject.ResetQuery();
    this.m_queryObject.AddQuery("SELECT readable_name FROM " + PLAYLIST_LIST_TABLE_NAME + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    var resObj = m_queryObject.GetResultObject();
    
    if(resObj.GetRowCount())
      strReadableName = resObj.GetRowCell(0, 0);    
    
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