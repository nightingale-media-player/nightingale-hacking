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
// The Playlist Reader.
// 

function CPlaylistReaderListener()
{
}

CPlaylistReaderListener.prototype.constructor = CPlaylistReaderListener;

CPlaylistReaderListener.prototype = 
{
  m_strGUID: "",
  m_strDestURL: "",
  
  QueryInterface: function(aIID) {
    if (!aIID.equals(Components.interfaces.nsIWebProgressListener) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & 16)
    {
      alert("done ---- " + this.guid + " " + this.destURL);
    }
  },
  
  onStatusChange: function(aWebProgress, aRequest, aStateFlags, strStateMessage)
  {
  },
  
  onProgressChange: function(aWebProgress, aRequest, curSelfProgress, maxSelfProgress, curTotalProgress, maxTotalProgress)
  {
  },
  
  onSecurityChange: function(aWebProgress, aRequest, aStateFlags)
  {
  },
  
  onLocationChange: function(aWebProgress, aRequest, aLocation) 
  {
  }
};

function CPlaylistReader()
{
  this.m_rootContractID = "@songbirdnest.com/Songbird/Playlist/Reader/";
  this.m_interfaceID = Components.interfaces.sbIPlaylistReader;

  this.m_browserObj = null;

  this.GetFileExtension = function GetFileExtension(strURL)
  {
    var theExtension = "";
    var extPos = strURL.indexOf("?");
    
    if(extPos == -1)
    {
      extPos = strURL.lastIndexOf(".") + 1;
      theExtension = strURL.substr(extPos)
    }
    else
    {
      var str = strURL.substr(0, extPos);
      extPos = str.lastIndexOf(".") + 1;
      theExtension = str.substr(extPos);
    }
    
    return theExtension;
  }
  
  this.AutoLoad = function AutoLoad(strURL, strGUID, strName, strType, strDesc, strContentType, webListener)
  {
    try
    {
      var theExtension = this.GetFileExtension(strURL);
      
      var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      var aLocalURI = Components.classes["@mozilla.org/network/simple-uri;1"].createInstance(Components.interfaces.nsIURI);
      aLocalURI.spec = strURL;
      
      if(aLocalURI.scheme == "file")
      {
        const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
        var aPlaylistManager = new PlaylistManager();
        aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);

        var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
        aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
  
        var strDestTable = aUUIDGenerator.generateUUID();
        var aDBQuery = new sbIDatabaseQuery();
        
        aDBQuery.SetAsyncQuery(false);
        aDBQuery.SetDatabaseGUID(strGUID);

        aPlaylistManager.CreatePlaylist(strDestTable, strName, strDescription, strType, aDBQuery);

        var err = new Object();
        err.value = 0;
        
        return this.Read(strURL, strGUID, strDestTable, err);
      }
      else
      {
        var aBrowser = (Components.classes["@mozilla.org/embedding/browser/nsWebBrowser;1"]).createInstance();
        aBrowser = aBrowser.QueryInterface(Components.interfaces.nsIWebBrowserPersist);
        
    		var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
		    aLocalFile.QueryInterface(Components.interfaces.nsILocalFile);
        aLocalFile.initWithPath("C:\\test.pls");
        
        var aLocalURI = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService).newURI(strURL, null, null);
                
        if(webListener != null)
        {
          aBrowser.progressListener = webListener;
        }
        else
        {
          var aListener = (new CPlaylistReaderListener()).QueryInterface(Components.interfaces.nsIWebProgressListener);
          alert(aListener);
          aListener.m_strGUID = strGUID;
          aListener.m_strDestURL = "C:\\test.pls";
          
          aBrowser.progressListener = aListener;
        }
        
        aBrowser.saveURI(aLocalURI, null, null, null, "", aLocalFile);
      }
      
      return true;
    }
    catch(err)
    {
      alert(err);
    }
 
    return false;
  }
  
  this.Read = function Read(strURL, strGUID, strDestTable, errorCode)
  {
    var theExtension = this.GetFileExtension(strURL);
  
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(thePlaylistReader.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
     
        try
        {
          aReader = aReader.createInstance();
          aReader = aReader.QueryInterface(thePlaylistReader.m_interfaceID);
        }
        catch ( err )
        {
          throw "playlist_reader.read - " + contractID + " - " + err;
        }
        
        if(aReader)
        {
          var nExtensionsCount = new Object;
          var theExtensions = aReader.SupportedFileExtensions(nExtensionsCount);
          
          for(var i = 0; i < theExtensions.length; i++)
          {
            if(theExtensions[i] == theExtension)
            {
              var bRet = aReader.Read(strURL, strGUID, strDestTable, errorCode);
              return bRet;
            }
          }
        }
      }
    }
  }
  
  this.SupportedFileExtensions = function SupportedFileExtensions()
  {
    var theExtensions = new Array;
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(thePlaylistReader.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
     
        aReader = aReader.createInstance();
        aReader = aReader.QueryInterface(thePlaylistReader.m_interfaceID);
        
        if(aReader)
        {
          var nExtensionsCount = new Object;
          var aExts = aReader.SupportedFileExtensions(nExtensionsCount);
          theExtensions.push(aExts);
        }
      }
    }
    
    return theExtensions;
  }
  
  this.SupportedMIMETypes = function SupportedMIMETypes()
  {
    var theMIMETypes = new Array;
    for(var contractID in Components.classes)
    {
      if(contractID.indexOf(thePlaylistReader.m_rootContractID) != -1)
      {
        var aReader = Components.classes[contractID];
     
        aReader = aReader.createInstance();
        aReader = aReader.QueryInterface(thePlaylistReader.m_interfaceID);
        
        if(aReader)
        {
          var nMIMETypesCount = new Object;
          theMIMETypes.push(aReader.SupportedMIMETypes(nMIMETypesCount));
        }
      }
    }
    
    return theMIMETypes;   
  }
}

var thePlaylistReader = new CPlaylistReader();