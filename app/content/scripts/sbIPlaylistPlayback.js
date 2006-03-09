//
// sbIPlaylistPlayback Object
//

const SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID = "@songbird.org/Songbird/PlaylistPlayback;1";
const SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME = "Songbird PlaylistPlayback Interface"
const SONGBIRD_PLAYLISTPLAYBACK_CID = Components.ID('{5D32F88B-D83F-42f9-BF6E-D56E833A78BA}');
const SONGBIRD_PLAYLISTPLAYBACK_IID = Components.interfaces.sbIPlaylistPlayback;

function CPlaylistPlayback()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", this );
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/sbIDataRemote.js", this );

  this.m_queryObject = new this.sbIDatabaseQuery();
  this.m_strTable = "";
}

/* I actually need a constructor in this case. */
CPlaylistPlayback.prototype.constructor = CPlaylistPlayback;

/* the CPlaylistPlayback class def */
CPlaylistPlayback.prototype = 
{
  m_queryObject: null,
  m_strTable: "",

  Play: function(dbGUID, strTable, nStartPos)
  {
    this.m_queryObject.SetDatabaseGUID(dbGUID);
    this.m_strTable = strTable;
    
    return true;
  },
  
  PlayObject: function(playlistObj, nStartPos)
  {
    return false;
  },
  
  Next: function()
  {
    return 0;
  },
    
  Previous: function()
  {
    return 0;
  },
  
  Current: function()
  {
    return 0;
  },
  
  CurrentGUID: function()
  {
    return "";
  },
  
  CurrentURL: function()
  {
    return "";
  },
  
  Reset: function()
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },

  SetRepeat: function(repeatMode)
  {
    return;
  },
  
  GetRepeat: function()
  {
    return 0;
  },

  GetShuffle: function()
  {
    if(m_queryObject != null)
    {
      
      return true;
    }

    return false;
  },
  
  SetShuffle: function(bShuffle)
  {
    var iShuffle = 0;
    if(bShuffle == true) iShuffle = 1;
    
    this.m_queryObject.ResetQuery();
    this.m_queryObject.AddQuery("UPDATE " + this.m_strTable + " SET shuffle = \"" + iShuffle + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.Execute();
    this.m_queryObject.WaitForCompletion();
    
    return;
  },
  
  GetMetadataFields: function(nMetadataFieldCount)
  {
    nMetadataFieldCount = 0;
    var aFields = new Array();
    
    return aFields;
  },
  
  GetCurrentValue: function(strField)
  {
    return "";
  },
  
  GetCurrentValues: function(nMetadataFieldCount, aMetaFields, nMetadataValueCount)
  {
    nMetadataValueCount = 0;
    var aValues = new Array();
    
    return aValues;
  },
  
  SetCurrentValue: function(strField, strValue)
  {
  },
  
  SetCurrentValues: function(nMetadataFieldCount, aMetaFields, nMetadataValueCount, aMetaValues)
  {
  },
  
  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLISTPLAYBACK_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbPlaylistPlaybackModule
 * \brief 
 */
var sbPlaylistPlaybackModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTPLAYBACK_CID, 
                                      SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME, 
                                      SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLISTPLAYBACK_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistPlaybackFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbPlaylistPlaybackFactory
 * \brief 
 */
var sbPlaylistPlaybackFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_PLAYLISTPLAYBACK_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylistPlayback()).QueryInterface(iid);
    }
}; //sbPlaylistPlaybackFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistPlaybackModule;
} //NSGetModule

