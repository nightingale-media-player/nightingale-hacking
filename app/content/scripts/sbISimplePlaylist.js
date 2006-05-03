//
// sbIPlaylist Object
//

const SONGBIRD_SIMPLEPLAYLIST_CONTRACTID = "@songbird.org/Songbird/SimplePlaylist;1";
const SONGBIRD_SIMPLEPLAYLIST_CLASSNAME = "Songbird SimplePlaylist Interface"
const SONGBIRD_SIMPLEPLAYLIST_CID = Components.ID('{7B2945F6-6A00-4489-AD2F-95BA25F4D1EA}');
const SONGBIRD_SIMPLEPLAYLIST_IID = Components.interfaces.sbISimplePlaylist;

function CSimplePlaylist()
{
}

/* the CPlaylist class def */
CSimplePlaylist.prototype = 
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
  
  AddByURL: function(strURL, nColumnCount, aColumns, nValueCount, aValues)
  {
    
  },
 
  RemoveByURL: function(strURL)
  {
  },
  
  RemoveByIndex: function(nIndex)
  {
  },

  FindByURL: function(strURL)
  {
    return -1;
  },
  
  FindByIndex: function(nIndex)
  {
    return "";
  },

  GetColumns: function(nColumnCount)
  {
    nColumnCount = 0;
    var aColumns = new Array();
    
    return aColumns;
  },
  
  AddColumn: function(strColumn)
  {
    return;
  },
  
  DeleteColumn: function(strColumn)
  {
    return;
  },

  GetColumnValueByIndex: function(mediaIndex, strColumn)
  {
    return "";
  },
  
  GetColumnValueByURL: function(mediaURL, strColumn)
  {
    return "";
  },

  GetColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount)
  {
    nValueCount = 0;
    var aValues = new Array();
    
    return aValues;
  },
  
  GetColumnValuesByURL: function(mediaURL, nColumnCount, aColumns, nValueCount)
  {
    nValueCount = 0;
    var aValues = new Array();
    
    return aValues;
  },
  
  SetColumnValueByIndex: function(mediaIndex, strColumn, strValue)
  {
    return;
  },
  
  SetColumnValueByURL: function(mediaURL, strColumn, strValue)
  {
    return;
  },

  SetColumnValuesByIndex: function(mediaIndex, nColumnCount, aColumns, nValueCount, aValues)
  {
    return;
  },
  
  SetColumnValuesByURL: function(mediaURL, nColumnCount, aColumns, nValueCount, aValues)
  {
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
    this.m_queryObject.AddQuery("UPDATE " +  + " SET readable_name = \"" + this.m_strReadableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    return;
  },
  
  GetReadableName: function()
  {
    var strReadableName = "";
    this.m_queryObject.ResetQuery();
    this.m_queryObject.AddQuery("SELECT readable_name FROM " + this.m_strName + " WHERE name = \"" + this.m_strName + "\"");
    
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
          !iid.equals(SONGBIRD_SIMPLEPLAYLIST_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbSimplePlaylistModule
 * \brief 
 */
var sbSimplePlaylistModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_SIMPLEPLAYLIST_CID, 
                                      SONGBIRD_SIMPLEPLAYLIST_CLASSNAME, 
                                      SONGBIRD_SIMPLEPLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_SIMPLEPLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbSimplePlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbSimplePlaylistModule

/**
 * \class sbSimplePlaylistFactory
 * \brief 
 */
var sbSimplePlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_SIMPLEPLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CSimplePlaylist()).QueryInterface(iid);
    }
}; //sbSimplePlaylistFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbSimplePlaylistModule;
} //NSGetModule