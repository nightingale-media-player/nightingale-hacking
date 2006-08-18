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
// sbIPlaylistReaderListener Object
//

const SONGBIRD_PLAYLISTREADERLISTENER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistReaderListener;1";
const SONGBIRD_PLAYLISTREADERLISTENER_CLASSNAME = "Songbird Playlist Reader Listener"
const SONGBIRD_PLAYLISTREADERLISTENER_IID = Components.interfaces.sbIPlaylistReaderListener;
const SONGBIRD_PLAYLISTREADERLISTENER_CID = Components.ID("{b4fac7ab-7d23-47c5-98e0-7e59266e2a28}");

function CPlaylistReaderListener()
{
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
      var playlistReaderMngr = Components.classes["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                                      .createInstance(Components.interfaces.sbIPlaylistReaderManager);
      var playlistManager =   Components.classes["@songbirdnest.com/Songbird/PlaylistManager;1"]
                                      .createInstance(Components.interfaces.sbIPlaylistManager);
      
      var strContentType = "";
      var aChannel = aRequest.QueryInterface(Components.interfaces.nsIChannel);
      if(aChannel)
      {
        try {
          strContentType = aChannel.contentType;
//          dump("CPlaylistReaderListener::onStateChange - Playlist Content Type: " + strContentType + "\n");
        } catch (err) {
          dump("CPlaylistReaderListener::onStateChange - NO CONTENT TYPE AVAILABLE\n");
        } // Grrrr.
      }
      playlistReaderMngr.originalURL = this.originalURL;
      var success = playlistReaderMngr.loadPlaylist(this.destinationURL, this.serviceGuid, this.destinationTable, 
                                                    this.readableName, this.playlistType, this.description, 
                                                    strContentType, this.appendOrReplace, null);
      
      if(success)
      {
        const DataRemote = new Components.Constructor( "@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
        var dbQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                                      .createInstance(Components.interfaces.sbIDatabaseQuery);

        var dpDownloadContext = new DataRemote( "download.context", null );
        var dpDownloadTable = new DataRemote( "download.table", null );

        dbQuery.setDatabaseGUID(this.serviceGuid);
        dbQuery.setAsyncQuery(false);
        var playlist = playlistManager.getDynamicPlaylist(this.destinationTable, dbQuery);
        
        if(playlist)
        {
          //var destFolderRemote = new DataRemote( "download.folder", null );
          var destFolder = (new DataRemote("download.folder", null)).stringValue;

          var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                      getService(Components.interfaces.sbIDeviceManager);
          if (!deviceManager)
            return false;
        
          var downloadDevice = null;
          var downloadCategory = 'Songbird Download Device';
          if (deviceManager.hasDeviceForCategory(downloadCategory)) {
            downloadDevice =
              deviceManager.getDeviceByCategory(downloadCategory);
          }
          
          if( !downloadDevice)
            return false;

          var downloadTable = {};
          downloadDevice.autoDownloadTable('', this.serviceGuid, this.destinationTable, '', 0, null, '', destFolder, downloadTable);
          
          dpDownloadContext.stringValue = downloadDevice.getContext('');
          dpDownloadTable.stringValue = downloadTable.value;
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

