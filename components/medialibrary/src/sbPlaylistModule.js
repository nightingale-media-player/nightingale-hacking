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

const componentData = 
  [
  {cid: null,
   contractid: null,
   classname: null,
   script: "sbPlaylistBase.js",
   constructor: null},
   
  {cid: Components.ID("{c3c120f4-5ab6-4402-8cab-9b8f1d788769}"),
   contractid: "@songbirdnest.com/Songbird/Playlist;1",
   classname: "Songbird Playlist Interface",
   script: "sbPlaylist.js",
   constructor: "CPlaylist"},

  {cid: Components.ID("{6322a435-1e78-4825-91c8-520e829c23b8}"),
   contractid: "@songbirdnest.com/Songbird/DynamicPlaylist;1",
   classname: "Songbird Dynamic Playlist Interface",
   script: "sbDynamicPlaylist.js",
   constructor: "CDynamicPlaylist"},

  {cid: Components.ID("{a81b9c4d-e578-4737-840f-43d404c98423}"),
   contractid: "@songbirdnest.com/Songbird/SmartPlaylist;1",
   classname: "Songbird Smart Playlist Interface",
   script: "sbSmartPlaylist.js",
   constructor: "CSmartPlaylist"}

  ];

/**
 * \class sbPlaylistModule
 * \brief 
 */
var sbPlaylistModule = 
{
  m_loadedDeps: false,
  
  loadDependencies: function() {
    if(this.m_loadedDeps)
      return;
      
    var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
    var dirService = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties);
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    
    var componentsDir = __LOCATION__.parent.parent;

    for( var i = 0; i < componentData.length; i++) {
      var scriptName = componentData[i].script;
      if(!scriptName)
        continue;
        
      var f = componentsDir.clone();
      f = f.QueryInterface(Components.interfaces.nsILocalFile);
      
      f.appendRelativePath("scripts");
      f.append(scriptName);
      
      if(jsLoader) {
        try {
          var scriptUri = ioService.newFileURI(f);
          jsLoader.loadSubScript( scriptUri.spec, null );
        } catch (e) {
          dump("sbPlaylistModule.js: Failed to load " + scriptUri.spec + "\n");
          throw e;
        }
      }
    }
    
    this.m_loadedDeps = true;
  },

  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);

    if(!this.m_loadedDeps)
      this.loadDependencies();
    
    for(var i = 0; i < componentData.length; i++) {
      var comp = componentData[i];
      if(!comp.cid)
        continue;
      
      compMgr.registerFactoryLocation(comp.cid,
                                      comp.classname,
                                      comp.contractid,
                                      fileSpec,
                                      location,
                                      type);
    }
  },

  makeFactoryFor: function(constructor) {
    var factory = {
      QueryInterface: function(aIID) {
        if(!aIID.equals(Components.interfaces.nsISupports) &&
           !aIID.equals(Components.interfaces.nsIFactory))
          throw Components.results.NS_ERROR_NO_INTERFACE;
        
        return this;
      },
      
      createInstance: function(outer, iid) {
        if(outer != null)
          throw Components.results.NS_ERROR_NO_AGGREGATION;
        return (new constructor()).QueryInterface(iid);
      }
    }
    
    return factory;
  },
  
  getClassObject: function(compMgr, cid, iid) 
  {
    if(!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    
    if(!this.m_loadedDeps)
      this.loadDependencies();
    
    for(var i = 0; i < componentData.length; i++) {
      if(cid.equals(componentData[i].cid)) {
        return this.makeFactoryFor(eval(componentData[i].constructor));
      }
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(compMgr) { 
    return true; 
  }
};

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec) { 
  return sbPlaylistModule;
} //NSGetModule