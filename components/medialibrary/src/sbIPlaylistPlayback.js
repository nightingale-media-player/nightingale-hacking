/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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
// sbIPlaylistPlayback Object
//

const SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID = "@songbird.org/Songbird/PlaylistPlayback;1";
const SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME = "Songbird PlaylistPlayback Interface"
const SONGBIRD_PLAYLISTPLAYBACK_CID = Components.ID('{5D32F88B-D83F-42f9-BF6E-D56E833A78BA}');
const SONGBIRD_PLAYLISTPLAYBACK_IID = Components.interfaces.sbIPlaylistPlayback;

function CPlaylistPlayback()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://Songbird/content/scripts/songbird_interfaces.js", this );
  jsLoader.loadSubScript( "chrome://Songbird/content/scripts/sbIDataRemote.js", this );

  this.m_queryObject = new this.sbIDatabaseQuery();
}

/* I actually need a constructor in this case. */
CPlaylistPlayback.prototype.constructor = CPlaylistPlayback;

/* the CPlaylistPlayback class def */
CPlaylistPlayback.prototype = 
{
  m_Library: null,
  m_PlaylistManager: null,
  m_queryObject: null,
  
  m_strDBGUID: "",
  m_strTable: "",

  m_strCurrentURL: "",
  m_strCurrentGUID: null,

  m_bShuffle: false,
  m_nRepeatMode: 0,
  
  Play: function(dbGUID, strTable, nStartPos)
  {
    this.m_queryObject.SetDatabaseGUID(dbGUID);
    this.m_strDBGUID = dbGUID;
    this.m_strTable = strTable;
    
    return true;
  },
  
  PlayObject: function(playlistObj, nStartPos)
  {
    return false;
  },
  
  Pause: function()
  {
  },
  
  Stop: function()
  {
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
    return this.m_strCurrentURL;
  },
  
  Reset: function()
  {
    return false;
  },

  SetRepeat: function(repeatMode)
  {
    if(repeatMode >= 0 && repeatMode <= 2)
      this.m_nRepeatMode = repeatMode;
  },
  
  GetRepeat: function()
  {
    return this.m_nRepeatMode;
  },

  GetShuffle: function()
  {
    return this.m_bShuffle;
  },
  
  SetShuffle: function(bShuffle)
  {
    this.m_bShuffle = bShuffle;
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