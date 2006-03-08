//
// sbIPlaylist Object
//

const SONGBIRD_PLAYLISTMANAGER_CONTRACTID = "@songbird.org/Songbird/PlaylistManager;1";
const SONGBIRD_PLAYLISTMANAGER_CLASSNAME = "Songbird PlaylistManager Interface"
const SONGBIRD_PLAYLISTMANAGER_CID = Components.ID('{0BE3A41A-6673-494a-A53E-9740A98ACFF7}');
const SONGBIRD_PLAYLISTMANAGER_IID = Components.interfaces.sbIPlaylistManager;

const PLAYLIST_TABLE_NAME = "__table__";
const PLAYLIST_LIST_TABLE_NAME = "playlist_list";

const PLAYLIST_TABLE_CREATE = "CREATE TABLE __table__ (id INTEGER PRIMARY KEY AUTOINCREMENT, uuid BLOB, service_uuid BLOB, has_been_processed INTEGER(0,1) DEFAULT 0)";
const PLAYLIST_LIST_TABLE_CREATE = "CREATE TABLE playlist_list (name TEXT, readable_name TEXT, description TEXT, type TEXT, random INTEGER(0,1,2) DEFAULT 0, shuffle INTEGER (0,1) DEFAULT 0)";

function CPlaylistManager()
{
}

/* the CPlaylistManager class def */
CPlaylistManager.prototype = 
{
  CreateDefaultPlaylistManager: function()
  {
    if(this.m_queryObj != null)
    {
      this.m_queryObj.ResetQuery();
      this.m_queryObj.AddQuery(PLAYLIST_LIST_TABLE_CREATE);
      
      this.m_queryObj.Execute();
      this.m_queryObject.WaitForCompletion();
    }
    
    return;
  },
  
  CreateSimplePlaylist: function(strName, nMetaFieldCount, aMetaFields, queryObj)
  {
    if(queryObj != null)
    {
    }
    
    return null;
  },
  
  CreatePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      
      var strQuery = PLAYLIST_TABLE_CREATE;
      strQuery.replace(/PLAYLIST_TABLE_NAME/, strName);
      
      queryObj.AddQuery(strQuery);
      
      queryObj.Execute();
      queryObj.WaitForCompletion();
      
      //create sbIPlaylist, set query object, set return new sbIPlaylist.
    }
    
    return null;
  },
  
  DeletePlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
      queryObj.ResetQuery();
      
      queryObj.AddQuery();
    }

    return 0;
  },
  
  GetPlaylistList: function(queryObj , nCount)
  {
    var aPlaylists = new Array();
    
    if(queryObj != null)
    {
    }

    return aPlaylists;
  },
  
  GetPlaylist: function(strName, queryObj)
  {
    if(queryObj != null)
    {
    }

    return null;
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