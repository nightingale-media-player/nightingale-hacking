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
// sbIPlaylist Object
//


const SONGBIRD_PLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/Playlist;1";
const SONGBIRD_PLAYLIST_CLASSNAME = "Songbird Playlist Interface"
const SONGBIRD_PLAYLIST_CID = Components.ID("{c3c120f4-5ab6-4402-8cab-9b8f1d788769}");
const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

function CPlaylist()
{
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
  this.m_strPlaylistTableName = "playlist_list";
}

CPlaylist.prototype = new CPlaylistBase();
CPlaylist.prototype.constructor = CPlaylist;

/* the CPlaylist class def */
CPlaylist.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsISupports) &&
      !iid.equals(SONGBIRD_PLAYLIST_IID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

/**
 * \class sbPlaylistModule
 * \brief 
 */

/*
var sbPlaylistModule = 
{
  m_loadedDeps: false,
  
  loadDependencies: function() {
    if(this.m_loadedDeps)
      return;
      
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    var dirService = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties);
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    
    var componentsDir = __LOCATION__.parent;

    var f = componentsDir.clone();
    f.append("sbIPlaylistBase.js");
    
    if(jsLoader && ioService && dirService) {
      
      try {
        var scriptUri = ioService.newFileURI(f);
        jsLoader.loadSubScript( scriptUri.spec, null );
      } catch (e) {
        dump("sbDynamicPlaylist.js: Failed to load " + scriptUri.spec + "\n");
        throw e;
      }
    }
    
    this.m_loadedDeps = true;
  },

  registerSelf: function(compMgr, fileSpec, location, type)
  {
      compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
      compMgr.registerFactoryLocation(SONGBIRD_PLAYLIST_CID, 
                                      SONGBIRD_PLAYLIST_CLASSNAME, 
                                      SONGBIRD_PLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_PLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbPlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};
*/

/**
 * \class sbPlaylistFactory
 * \brief 
 */

 /*
var sbPlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_PLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CPlaylist()).QueryInterface(iid);
    }
}; //sbPlaylistFactory
*/

/**
 * \function NSGetModule
 * \brief 
 */

/*
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistModule;
} //NSGetModule
*/