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
// sbIPlaylistWriterManager Object
// 

const SONGBIRD_PLAYLISTWRITERMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistWriterManager;1";
const SONGBIRD_PLAYLISTWRITERMANAGER_CLASSNAME = "Songbird Playlist Writer Manager Interface"
const SONGBIRD_PLAYLISTWRITERMANAGER_CID = Components.ID("{B93AFCDC-109C-4a21-BCEB-0BACB9D3E064}");
const SONGBIRD_PLAYLISTWRITERMANAGER_IID = Components.interfaces.sbIPlaylistWriterManager;

function CPlaylistWriterManager()
{
  this.m_rootContractID = "@songbirdnest.com/Songbird/Playlist/Writer/";
  this.m_interfaceID = Components.interfaces.sbIPlaylistWriter;
  this.m_Writers = new Array();
  this.m_Extensions = new Array();
  this.m_MIMETypes = new Array();

  // Cache the valid components
  for(var contractID in Components.classes)
  {
    if(contractID.indexOf(this.m_rootContractID) != -1)
    {
      var aWriter = Components.classes[contractID].createInstance(this.m_interfaceID);
      if(aWriter)
        this.m_Writers.push(aWriter);
    }
  }
  
  // Cache the supported strings
  for(var i in this.m_Writers)
  {
    var nExtensionsCount = new Object;
    var aExts = this.m_Writers[i].supportedFileExtensions(nExtensionsCount);
    this.m_Extensions.push(aExts);
    var nMIMETypesCount = new Object;
    var aMIMETypes = this.m_Writers[i].supportedMIMETypes(nMIMETypesCount);
    this.m_MIMETypes.push(aMIMETypes);
  }
}

CPlaylistWriterManager.prototype.constructor = CPlaylistWriterManager();

CPlaylistWriterManager.prototype.getFileExtension = function(strURL)
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
};

CPlaylistWriterManager.prototype.write = function(aGUID, aName, aOutputContentType, aWriterListener)
{
  return 0;
};

CPlaylistWriterManager.prototype.supportedFileExtensions = function(nExtCount)
{
  nExtCount.value = this.m_Extensions.length;    
  return this.m_Extensions;
};
  
CPlaylistWriterManager.prototype.supportedMIMETypes = function(nMIMECount)
{
  nMIMECount.value = this.m_MIMETypes.length;    
  return this.m_MIMETypes;   
};
  
CPlaylistWriterManager.prototype.QueryInterface = function(aIID)
{
  if (!aIID.equals(Components.interfaces.sbIPlaylistWriterManager) &&
      !aIID.equals(Components.interfaces.nsISupports)) 
  {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  
  return this;
};

/**
 * \class sbPlaylistWriterManagerModule
 * \brief 
 */
var sbPlaylistWriterManagerModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTWRITERMANAGER_CID, 
                                      SONGBIRD_PLAYLISTWRITERMANAGER_CLASSNAME, 
                                      SONGBIRD_PLAYLISTWRITERMANAGER_CONTRACTID,
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLISTWRITERMANAGER_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistWriterManagerFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistWriterManagerModule

/**
 * \class sbPlaylistWriterManagerFactory
 * \brief 
 */
var sbPlaylistWriterManagerFactory =
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
}; //sbPlaylistWriterManagerFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistWriterManagerModule;
} //NSGetModule

