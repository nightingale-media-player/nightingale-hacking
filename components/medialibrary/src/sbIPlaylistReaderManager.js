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
// sbIPlaylistReaderManager Object
// 

const SONGBIRD_PLAYLISTREADERMANAGER_CONTRACTID = "@songbird.org/Songbird/PlaylistReaderManager;1";
const SONGBIRD_PLAYLISTREADERMANAGER_CLASSNAME = "Songbird Playlist Reader Manager Interface"
const SONGBIRD_PLAYLISTREADERMANAGER_CID = Components.ID('{18032AD6-CB1F-403d-B3F5-7FE8EB579C28}');
const SONGBIRD_PLAYLISTREADERMANAGER_IID = Components.interfaces.sbIPlaylistReaderManager;

function CPlaylistReaderManager()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", this );
}

CPlaylistReaderManager.prototype.constructor = CPlaylistReaderManager;

CPlaylistReaderManager.prototype = 
{
  originalURL: "",
  
  m_rootContractID: "@songbird.org/Songbird/Playlist/Reader/",
  m_interfaceID: Components.interfaces.sbIPlaylistReader,
  m_Browser: null,
  m_Listener: null,

  GetTempFilename: function()
  {
    var strTempFile = "";
    
    var aDirectoryService = Components.classes["@mozilla.org/file/directory_service;1"].createInstance();
    aDirectoryService = aDirectoryService.QueryInterface(Components.interfaces.nsIProperties);
    
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
    var aUUID = aUUIDGenerator.generateUUID();
    
    var bResult = new Object;
    var aTempFolder = aDirectoryService.get("DefProfLRt", Components.interfaces.nsIFile, bResult);
    
    aTempFolder.append(aUUID);
    
    return aTempFolder.path;
  },
  
  GetFileExtension: function(strURL)
  {
    if(!strURL) return "";
    
    var aURL = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURL);
    var strExtension = "";
    
    try
    {
      aURL.spec = strURL;
      strExtension = aURL.fileExtension;    
    }
    catch(err)
    {
      throw strURL + " - " + err;
    }
    
    return strExtension;
  },

  //sbIPlaylistReaderManager
  LoadPlaylist: function(strURL, strGUID, strName, strReadableName, strPlaylistType, strDescription, strContentType, bAppendOrReplace, playlistReaderListener)
  {
    const PlaylistManager = new Components.Constructor("@songbird.org/Songbird/PlaylistManager;1", "sbIPlaylistManager");
    const PlaylistReaderListener = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
    
    var err = new Object();
    var theExtension = this.GetFileExtension(strURL);
    var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    var aLocalURI = Components.classes["@mozilla.org/network/simple-uri;1"].createInstance(Components.interfaces.nsIURI);

    try
    {
      aLocalURI.spec = strURL;
    }
    catch ( err )
    {
      // WHOA!
//      throw( "\r\nsbIPlaylistReaderManager::LoadPlaylist Error loading url '" + strURL + "'\r\n" + err + "\r\n" );
      return false;
    }
    
    if(aLocalURI.scheme == "file")
    {
      var aPlaylistManager = new PlaylistManager();
      aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);

      var aDBQuery = Components.classes["@songbird.org/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
      
      aDBQuery.SetAsyncQuery(false);
      aDBQuery.SetDatabaseGUID(strGUID);

      var playlist = aPlaylistManager.GetPlaylist(strName, aDBQuery);
      
      if(!playlist)
        aPlaylistManager.CreatePlaylist(strName, strReadableName, strDescription, strPlaylistType, aDBQuery);
        
      err.value = 0;
      
      var aURL = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURL);
      aURL.spec = strURL;

      return this.Read(strURL, strGUID, strName, strContentType, bAppendOrReplace, err);
    }
    else 
    {
      // Remember the original url.
      this.originalURL = strURL;
      
      var destFile = this.GetTempFilename() + "." + theExtension;
      var aBrowser = (Components.classes["@mozilla.org/embedding/browser/nsWebBrowser;1"]).createInstance(Components.interfaces.nsIWebBrowserPersist);

      if(!aBrowser) return false;
      this.m_Browser = aBrowser;      
    	
    	var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      aLocalFile.initWithPath(destFile);
      
      var aRegisterFileForDelete = Components.classes["@mozilla.org/uriloader/external-helper-app-service;1"].createInstance(Components.interfaces.nsPIExternalAppLauncher);
      aRegisterFileForDelete.deleteTemporaryFileOnExit(aLocalFile);
      
      var aLocalURI = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService).newURI(strURL, null, null);
              
      if(playlistReaderListener)
      {
        this.m_Listener = playlistReaderListener;
      }
      else
      {
        var aListener = (new PlaylistReaderListener()).QueryInterface(Components.interfaces.sbIPlaylistReaderListener);
        this.m_Listener = aListener;
      }
      
      this.m_Listener.originalURL = this.originalURL;
      this.m_Listener.serviceGuid = strGUID;
      this.m_Listener.destinationURL = "file:///" + destFile;
      this.m_Listener.destinationTable = strName;
      this.m_Listener.readableName = strReadableName;
      this.m_Listener.playlistType = strPlaylistType;
      this.m_Listener.description = strDescription;
      this.m_Listener.appendOrReplace = bAppendOrReplace;
      
//      this.m_Browser.persistFlags |= 2; // PERSIST_FLAGS_BYPASS_CACHE;
      this.m_Browser.progressListener = this.m_Listener;
      
      this.m_Browser.saveURI(aLocalURI, null, null, null, "", aLocalFile);

      return true;
    }
      
    return true;
  },

  AutoLoad: function(strURL, strGUID, strName, strType, strDesc, strContentType, playlistReaderListener)
  {
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);

    var strDestTable = aUUIDGenerator.generateUUID();
    
    return this.LoadPlaylist(strURL, strGUID, strDestTable, strName, strType, strDesc, strContentType, false, playlistReaderListener);
  },
  
  Read: function(strURL, strGUID, strDestTable, strContentType, bAppendOrReplace)
  {
    var theExtension = this.GetFileExtension(strURL);
  
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(this.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
        
        aReader = aReader.createInstance();
        aReader = aReader.QueryInterface(this.m_interfaceID);
        
        if(aReader)
        {
          if(strContentType == "")
          {
            var nExtensionsCount = new Object;
            var theExtensions = aReader.SupportedFileExtensions(nExtensionsCount);
            
            for(var i = 0; i < theExtensions.length; i++)
            {
              if(theExtensions[i] == theExtension)
              {
                var errorCode = new Object;
                var bRet = false;
                
                // Handoff the original url
                aReader.originalURL = this.originalURL;
                this.originalURL = "";
                
                bRet = aReader.Read(strURL, strGUID, strDestTable, bAppendOrReplace, errorCode);
                
                dump("CPlaylistReaderManager::Read (by extension: " + theExtensions[i] + ") - Last Attempt: " + bRet + "\n");
                
                if(bRet)
                  return bRet;
              }
            }
          }
          else
          {
            var nMIMTypeCount = new Object;
            var theMIMETypes = aReader.SupportedMIMETypes(nMIMTypeCount);
            
            for(var i = 0; i < theMIMETypes.length; i++)
            {
              if(theMIMETypes[i] == strContentType)
              {
                var errorCode = new Object;
                var bRet = false;
                
                // Handoff the original url
                aReader.originalURL = this.originalURL;
                this.originalURL = "";
                
                bRet = aReader.Read(strURL, strGUID, strDestTable, bAppendOrReplace, errorCode);
                
                dump("CPlaylistReaderManager::Read (by mime type: " + theMIMETypes[i] + ") - Last Attempt: " + bRet + "\n");
                
                if(bRet)     
                  return bRet;
              }
            }
          }
        }
      }
    }
    
    return false;
  },
  
  SupportedFileExtensions: function(nExtCount)
  {
    var theExtensions = new Array;
    nExtCount.value = 0;
    
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(this.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
     
        aReader = aReader.createInstance();
        aReader = aReader.QueryInterface(this.m_interfaceID);
        
        if(aReader)
        {
          var nExtensionsCount = new Object;
          var aExts = aReader.SupportedFileExtensions(nExtensionsCount);
          theExtensions.push(aExts);
        }
      }
    }

    nExtCount.value = theExtensions.length;    
    return theExtensions;
  },
  
  SupportedMIMETypes: function(nMIMECount)
  {
    var theMIMETypes = new Array;
    nMIMECount.value = 0;
    
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(this.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
     
        aReader = aReader.createInstance();
        aReader = aReader.QueryInterface(this.m_interfaceID);
        
        if(aReader)
        {
          var nMIMETypesCount = new Object;
          var aMIMETypes = aReader.SupportedMIMETypes(nMIMETypesCount);
          theMIMETypes.push(aMIMETypes);
        }
      }
    }

    nMIMECount.value = theMIMETypes.length;    
    return theMIMETypes;   
  },
  
  QueryInterface: function(aIID) {
    if (!aIID.equals(Components.interfaces.sbIPlaylistReaderManager) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
};

/**
 * \class sbPlaylistReaderManagerModule
 * \brief 
 */
var sbPlaylistReaderManagerModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTREADERMANAGER_CID, 
                                      SONGBIRD_PLAYLISTREADERMANAGER_CLASSNAME, 
                                      SONGBIRD_PLAYLISTREADERMANAGER_CONTRACTID,
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLISTREADERMANAGER_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistReaderManagerFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistReaderManagerModule

/**
 * \class sbPlaylistReaderManagerFactory
 * \brief 
 */
var sbPlaylistReaderManagerFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_PLAYLISTREADERMANAGER_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylistReaderManager()).QueryInterface(iid);
    }
}; //sbPlaylistReaderManagerFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistReaderManagerModule;
} //NSGetModule