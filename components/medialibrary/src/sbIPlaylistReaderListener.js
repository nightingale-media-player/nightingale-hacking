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
// sbIPlaylistReaderListener Object
//

const SONGBIRD_PLAYLISTREADERLISTENER_CONTRACTID = "@songbird.org/Songbird/PlaylistReaderListener;1";
const SONGBIRD_PLAYLISTREADERLISTENER_CLASSNAME = "Songbird Playlist Reader Listener"
const SONGBIRD_PLAYLISTREADERLISTENER_IID = Components.interfaces.sbIPlaylistReaderListener;
const SONGBIRD_PLAYLISTREADERLISTENER_CID = Components.ID("{5770099E-503B-4b76-B1A6-0BC53F53D2BF}");

function CPlaylistReaderListener()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", this );
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/sbIDataRemote.js", this );
}

CPlaylistReaderListener.prototype.constructor = CPlaylistReaderListener;

CPlaylistReaderListener.prototype = 
{
  originalURL: "",
  serviceGuid: "",
  destinationURL: "",
  destinationTable: "",
  readableName: "",
  playlistType: "",
  description: "",
  appendOrReplace: false,
  
  onLocationChange: function(aWebProgress, aRequest, aLocation)
  {
  },

  onProgressChange: function(aWebProgress, aRequest, curSelfProgress, maxSelfProgress, curTotalProgress, maxTotalProgress)
  {
  },

  onSecurityChange: function(aWebProgress, aRequest, aStateFlags)
  {
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & 16 /*this.STATE_STOP*/)
    {
      const PlaylistReaderManager = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
      var aPlaylistReader = (new PlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);
      
      const PlaylistManager = new Components.Constructor("@songbird.org/Songbird/PlaylistManager;1", "sbIPlaylistManager");
      var aPlaylistManager = (new PlaylistManager()).QueryInterface(Components.interfaces.sbIPlaylistManager);
      
      var strContentType = "";
      var aChannel = aRequest.QueryInterface(Components.interfaces.nsIChannel);
      if(aChannel)
      {
        strContentType = aChannel.contentType;
        dump("CPlaylistReaderListener::onStateChange - Playlist Content Type: " + strContentType + "\n");
      }
      aPlaylistReader.originalURL = this.originalURL;
      var success = aPlaylistReader.LoadPlaylist(this.destinationURL, this.serviceGuid, this.destinationTable, 
                                                 this.readableName, this.playlistType, this.description, 
                                                 strContentType, this.appendOrReplace, null);
      
      if(success)
      {
        var dpDownloadContext = new this.sbIDataRemote( "download.context" );
        var dpDownloadTable = new this.sbIDataRemote( "download.table" );

        var dbQuery = new this.sbIDatabaseQuery();
        dbQuery.SetDatabaseGUID(this.serviceGuid);
        dbQuery.SetAsyncQuery(false);
        var playlist = aPlaylistManager.GetDynamicPlaylist(this.destinationTable, dbQuery);
        
        if(playlist)
        {
          const SUBSCRIBE_FOLDER_KEY = "download.folder";
          var destFolder = this.SBDataGetValue(SUBSCRIBE_FOLDER_KEY);
 
          aDeviceManager = Components.classes["@songbird.org/Songbird/DeviceManager;1"].createInstance(Components.interfaces.sbIDeviceManager);
          if(!aDeviceManager) return false;
        
          aDownloadDevice = aDeviceManager.GetDevice('Songbird Download Device');
          if(!aDownloadDevice) return false;

          var downloadTable = {};
          aDownloadDevice.AutoDownloadTable('', this.serviceGuid, this.destinationTable, '', 0, null, '', destFolder, downloadTable);
          
          dpDownloadContext.SetValue( aDownloadDevice.GetContext('') );
          dpDownloadTable.SetValue( downloadTable.value );
        }
      }
    }
  },
  
  onStatusChange: function(aWebProgress, aRequest, aStateFlags, strStateMessage)
  {
  },
  
  QueryInterface: function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistReaderListener) &&
        !aIID.equals(Components.interfaces.nsIWebProgressListener) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports))
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
};

/**
 * \class sbPlaylistReaderListenerModule
 * \brief 
 */
var sbPlaylistReaderListenerModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTREADERLISTENER_CID, 
                                      SONGBIRD_PLAYLISTREADERLISTENER_CLASSNAME, 
                                      SONGBIRD_PLAYLISTREADERLISTENER_CONTRACTID,
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLISTREADERLISTENER_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistReaderListenerFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbPlaylistReaderListenerFactory
 * \brief 
 */
var sbPlaylistReaderListenerFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_PLAYLISTREADERLISTENER_IID) &&
            !iid.equals(Components.interfaces.nsIWebProgressListener) &&
            !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylistReaderListener()).QueryInterface(iid);
    }
}; //sbPlaylistReaderListenerFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistReaderListenerModule;
} //NSGetModule