/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
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

const SONGBIRD_PLAYLISTREADERMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistReaderManager;1";
const SONGBIRD_PLAYLISTREADERMANAGER_CLASSNAME = "Songbird Playlist Reader Manager Interface"
const SONGBIRD_PLAYLISTREADERMANAGER_CID = Components.ID("{ced5902c-bd90-4099-acee-77487a5b1d13}");
const SONGBIRD_PLAYLISTREADERMANAGER_IID = Components.interfaces.sbIPlaylistReaderManager;

function CPlaylistReaderManager()
{
  // Cache the valid components
  for(var contractID in Components.classes)
  {
    if(contractID.indexOf(this.m_rootContractID) != -1)
    {
      var aReader = Components.classes[contractID].createInstance(this.m_interfaceID);
      if(aReader)
        this.m_Readers.push(aReader);
    }
  }
  
  // Cache the supported strings
  for(var i in this.m_Readers)
  {
    var nExtensionsCount = new Object;
    var aExts = this.m_Readers[i].supportedFileExtensions(nExtensionsCount);
    this.m_Extensions = this.m_Extensions.concat(aExts);
    var nMIMETypesCount = new Object;
    var aMIMETypes = this.m_Readers[i].supportedMIMETypes(nMIMETypesCount);
    this.m_MIMETypes = this.m_MIMETypes.concat(aMIMETypes);
  }
}

CPlaylistReaderManager.prototype.constructor = CPlaylistReaderManager;

CPlaylistReaderManager.prototype = 
{
  originalURL: "",
  
  m_rootContractID: "@songbirdnest.com/Songbird/Playlist/Reader/",
  m_interfaceID: Components.interfaces.sbIPlaylistReader,
  m_Browser: null,
  m_Listener: null,
  m_Readers: new Array(),
  m_Extensions: new Array(),
  m_MIMETypes: new Array(),

  getTempFilename: function()
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
  
  getFileExtension: function(strURL)
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
  loadPlaylist: function(strURL, strGUID, strName, strReadableName, strPlaylistType, strDescription, strContentType, bAppendOrReplace, playlistReaderListener)
  {
    const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
    const PlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
    
    var err = new Object();
    var theExtension = this.getFileExtension(strURL);
    var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    var aLocalURI = Components.classes["@mozilla.org/network/simple-uri;1"].createInstance(Components.interfaces.nsIURI);

    try
    {
      aLocalURI.spec = strURL;
    }
    catch ( err )
    {
      // WHOA!
      //throw( "\r\nsbIPlaylistReaderManager::loadPlaylist Error loading url '" + strURL + "'\r\n" + err + "\r\n" );
      return false;
    }
    
    if(aLocalURI.scheme == "file")
    {
      // Open the file and make sure something is in it
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var file;
      try {
        var uri = ios.newURI(strURL, null, null);
        file = uri.QueryInterface(Components.interfaces.nsIFileURL).file;
      }
      catch(e) {
        dump(e);
        return -1;
      }

      // Can't be a playlist if there is nothing in it
      if(!file.exists() || file.fileSize == 0) {
        return -1;
      }

      // If we are trying to load text/html, try to guess a better mime type
      if(strContentType == null || strContentType == "" || strContentType == "text/html") {
        var strContentType = this.guessMimeType(file);
        if(!strContentType) {
          return -1;
        }
      }

      // Make sure our existing playlist readers can handle this mime type
      var index;
      for( index = 0; index < this.m_MIMETypes.length; index++ )
      {
        if(this.m_MIMETypes[index] == strContentType)
          break;
      }
      if(index == this.m_MIMETypes.length)
        return -1;

      var aPlaylistManager = new PlaylistManager();
      aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);

      var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                              .createInstance(Components.interfaces.sbIDatabaseQuery);
      
      aDBQuery.setAsyncQuery(false);
      aDBQuery.setDatabaseGUID(strGUID);

      var playlist = aPlaylistManager.getPlaylist(strName, aDBQuery);

      if(!playlist)
        aPlaylistManager.createPlaylist(strName, strReadableName, strDescription, strPlaylistType, aDBQuery);

      err.value = 0;

      var aURL = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURL);
      aURL.spec = strURL;

      return this.read(strURL, strGUID, strName, strContentType, bAppendOrReplace, err);
    }
    else 
    {
      // Remember the original url.
      this.originalURL = strURL;
      
      var destFile = this.getTempFilename() + "." + theExtension;
      var aBrowser = (Components.classes["@mozilla.org/embedding/browser/nsWebBrowser;1"]).createInstance(Components.interfaces.nsIWebBrowserPersist);

      if(!aBrowser) return false;
      this.m_Browser = aBrowser;      
    	
      var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      aLocalFile.initWithPath(destFile);
      
      var aRegisterFileForDelete = Components.classes["@mozilla.org/uriloader/external-helper-app-service;1"]
                                           .getService(Components.interfaces.nsPIExternalAppLauncher);
      aRegisterFileForDelete.deleteTemporaryFileOnExit(aLocalFile);
      
      var aLocalURI = Components.classes["@mozilla.org/network/io-service;1"]
                                           .getService(Components.interfaces.nsIIOService)
                                           .newURI(strURL, null, null);
              
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

  autoLoad: function(strURL, strGUID, strName, strType, strDesc, strContentType, playlistReaderListener)
  {
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);

    var strDestTable = aUUIDGenerator.generateUUID();
    
    return this.loadPlaylist(strURL, strGUID, strDestTable, strName, strType, strDesc, strContentType, false, playlistReaderListener);
  },
  
  read: function(strURL, strGUID, strDestTable, strContentType, bAppendOrReplace)
  {
    var theExtension = this.getFileExtension(strURL);
  
    for(var r in this.m_Readers)
    {
      var aReader = this.m_Readers[r];
      if(strContentType == "")
      {
        var nExtensionsCount = new Object;
        var theExtensions = aReader.supportedFileExtensions(nExtensionsCount);
        
        for(var i = 0; i < theExtensions.length; i++)
        {
          if(theExtensions[i] == theExtension)
          {
            var errorCode = new Object;
            
            // Handoff the original url
            aReader.originalURL = this.originalURL;
            this.originalURL = "";
            
            aReader.read(strURL, strGUID, strDestTable, bAppendOrReplace, errorCode);
            return errorCode.value;
          }
        }
      }
      else
      {
        var nMIMTypeCount = new Object;
        var theMIMETypes = aReader.supportedMIMETypes(nMIMTypeCount);
        
        for(var i = 0; i < theMIMETypes.length; i++)
        {
          if(theMIMETypes[i] == strContentType)
          {
            var errorCode = new Object;
            
            // Handoff the original url
            aReader.originalURL = this.originalURL;
            this.originalURL = "";
            
            aReader.read(strURL, strGUID, strDestTable, bAppendOrReplace, errorCode);
            return errorCode.value;
            
//            dump("CPlaylistReaderManager::read (by mime type: " + theMIMETypes[i] + ") - Last Attempt: " + bRet + "\n");
          }
        }
      }
    }
    
    return -1;
  },
  
  supportedFileExtensions: function(nExtCount)
  {
    nExtCount.value = this.m_Extensions.length;    
    return this.m_Extensions;
  },
  
  supportedMIMETypes: function(nMIMECount)
  {
    nMIMECount.value = this.m_MIMETypes.length;    
    return this.m_MIMETypes;   
  },
  
  guessMimeType: function(file)
  {
    // Read a few bytes from the file
    var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                            .createInstance(Components.interfaces.nsIFileInputStream);
    var sstream = Components.classes["@mozilla.org/scriptableinputstream;1"]
                            .createInstance(Components.interfaces.nsIScriptableInputStream);
    fstream.init(file, -1, 0, 0);
    sstream.init(fstream); 
    var str = sstream.read(4096);
    sstream.close();
    fstream.close();

    // This object literal maps mime types to regexps.  If the content matches
    // the given regexp, it is assigned the given mime type.
    var regexps = {
      "application/rss+xml"  : /^<\?xml(.|\n)*<rss/,
      "application/atom+xml" : /^<\?xml(.|\n)*xmlns="http:\/\/www\.w3\.org\/2005\/Atom"/,
      "audio/x-scpls"        : /^\[playlist\]/i,
      "audio/mpegurl"        : /^#EXTM3U/i,
      "text/html"            : /<html/i,
      "audio/mpegurl"        : /^(http|mms|rtsp)/im,
      "audio/mpegurl"        : /.*(mp3|ogg|flac|wav|m4a|wma|wmv|asx|asf|avi|mov|mpg|mp4)$/im
    };

    for(var mimeType in regexps) {
      var re = regexps[mimeType];
      if(re.test(str)) {
        return mimeType;
      }
    }

    // Otherwise, we have no guess
    return null;
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

