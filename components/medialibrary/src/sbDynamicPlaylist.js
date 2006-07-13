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
// sbIDynamicPlaylist Object
//

//const SONGBIRD_PLAYLIST_IID = Components.interfaces.sbIPlaylist;

const SONGBIRD_DYNAMICPLAYLIST_CONTRACTID = "@songbirdnest.com/Songbird/DynamicPlaylist;1";
const SONGBIRD_DYNAMICPLAYLIST_CLASSNAME = "Songbird Dynamic Playlist Interface"
const SONGBIRD_DYNAMICPLAYLIST_CID = Components.ID("{6322a435-1e78-4825-91c8-520e829c23b8}");
const SONGBIRD_DYNAMICPLAYLIST_IID = Components.interfaces.sbIDynamicPlaylist;

function CDynamicPlaylist()
{
/*
  var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/sbPlaylistBase.js", this );
*/
  var query = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance();
  query = query.QueryInterface(Components.interfaces.sbIDatabaseQuery);

  this.m_internalQueryObject = query;
  this.m_strPlaylistTableName = "dynamicplaylist_list";
}

CDynamicPlaylist.prototype = new CPlaylistBase();
CDynamicPlaylist.prototype.constructor = CDynamicPlaylist;

/* the CDynamicPlaylist class def */
CDynamicPlaylist.prototype.setPeriodicity = function(nPeriodicity, bWillRunLater) {
  if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET periodicity  = \"" + nPeriodicity + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CDynamicPlaylist.prototype.getPeriodicity = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT periodicity FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    if(resObj.getRowCount > 0)
    {
      return resObj.getRowCell(0, 0);
    }
  }

  return 0;
};
  
CDynamicPlaylist.prototype.setURL = function(strURL, bWillRunLater) {
if(this.m_queryObject != null)
  {
    if(!bWillRunLater)
      this.m_queryObject.resetQuery();
    
    this.m_queryObject.addQuery("UPDATE " + this.m_strPlaylistTableName + " SET url = \"" + strURL + "\" WHERE name = \"" + this.m_strName + "\"");
    
    if(!bWillRunLater)
    {
      this.m_queryObject.execute();
      this.m_queryObject.waitForCompletion();
    }
  }
  
  return;
};
  
CDynamicPlaylist.prototype.getURL = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT url FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    if(resObj.getRowCount > 0)
    {
      return resObj.getRowCell(0, 0);
    }
  }

  return "";
};

CDynamicPlaylist.prototype.setLastUpdateTime = function() {
  if(this.m_queryObject != null)
  {
    var dNow = new Date();
    
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("UPDATE \"" + this.m_strPlaylistTableName + "\" SET last_update = " + dNow.getTime() + " WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
  }    
};
  
CDynamicPlaylist.prototype.getLastUpdateTime = function() {
  if(this.m_queryObject != null)
  {
    this.m_queryObject.resetQuery();
    this.m_queryObject.addQuery("SELECT last_update FROM \"" + this.m_strPlaylistTableName + "\" WHERE name = \"" + this.m_strName + "\"");
    
    this.m_queryObject.execute();
    this.m_queryObject.waitForCompletion();
    
    var resObj = this.m_queryObject.getResultObject();
    if(resObj.getRowCount > 0)
      return resObj.getRowCell(0, 0);
  }

  return 0;
};

CDynamicPlaylist.prototype.QueryInterface = function(iid) {
    if (!iid.equals(Components.interfaces.nsISupports) &&
        !iid.equals(SONGBIRD_DYNAMICPLAYLIST_IID) && 
        !iid.equals(SONGBIRD_PLAYLIST_IID))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
};

/**
 * \class sbDynamicPlaylistModule
 * \brief 
 */

/*
var sbDynamicPlaylistModule = 
{
  m_loadedDeps: false,
  
  loadDependencies: function() {
    if(this.m_loadedDeps)
      return;
      
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    var dirService = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties);
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    
    var componentsDir = __LOCATION__;

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
      compMgr.registerFactoryLocation(SONGBIRD_DYNAMICPLAYLIST_CID, 
                                      SONGBIRD_DYNAMICPLAYLIST_CLASSNAME, 
                                      SONGBIRD_DYNAMICPLAYLIST_CONTRACTID, 
                                      fileSpec, 
                                      location,
                                      type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
      if (!cid.equals(SONGBIRD_DYNAMICPLAYLIST_CID))
          throw Components.results.NS_ERROR_NO_INTERFACE;

      if (!iid.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

      return sbDynamicPlaylistFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};
*/

/**
 * \class sbDynamicPlaylistFactory
 * \brief 
 */

/*
var sbDynamicPlaylistFactory =
{
    createInstance: function(outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(SONGBIRD_DYNAMICPLAYLIST_IID) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return (new CDynamicPlaylist()).QueryInterface(iid);
    }
}; //sbDynamicPlaylistFactory
*/

/**
 * \function NSGetModule
 * \brief 
 */

/* 
function NSGetModule(comMgr, fileSpec)
{ 
  return sbDynamicPlaylistModule;
} //NSGetModule
*/