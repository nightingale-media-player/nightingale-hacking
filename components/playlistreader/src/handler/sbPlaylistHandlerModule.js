/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/**
 * \file sbPlaylistHandlerModule.js
 * \brief Based on http://mxr.mozilla.org/seamonkey/source/calendar/base/src/calItemModule.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var componentInitRun = false;

const componentData =
  [
    {cid: null,
     contractid: null,
     script: "sbPlaylistHandlerUtils.js",
     constructor: null},

   {cid: Components.ID("{ba32eac9-6732-4d5d-be50-156f52b5368b}"),
     contractid: "@songbirdnest.com/Songbird/Playlist/Reader/M3U;1",
     script: "sbM3UPlaylistHandler.js",
     constructor: "sbM3UPlaylistHandler",
     category: "playlist-reader",
     categoryEntry: "m3u"},

   {cid: Components.ID("{a6937260-0d7f-4721-8f31-4b8455cf72c9}"),
     contractid: "@songbirdnest.com/Songbird/Playlist/Reader/PLS;1",
     script: "sbPLSPlaylistHandler.js",
     constructor: "sbPLSPlaylistHandler",
     category: "playlist-reader",
     categoryEntry: "pls"},

   {cid: Components.ID("{75c8f646-e75a-4743-89cd-412fa083ce07}"),
     contractid: "@songbirdnest.com/Songbird/Playlist/Reader/HTML;1",
     script: "sbHTMLPlaylistHandler.js",
     constructor: "sbHTMLPlaylistHandler",
     category: "playlist-reader",
     categoryEntry: "html"}
  ];

var sbPlaylistHandlerModule = {
  mScriptsLoaded: false,
  loadScripts: function () {
    if (this.mScriptsLoaded)
      return;

    const jssslContractID = "@mozilla.org/moz/jssubscript-loader;1";
    const jssslIID = Ci.mozIJSSubScriptLoader;

    const dirsvcContractID = "@mozilla.org/file/directory_service;1";
    const propsIID = Ci.nsIProperties;

    const iosvcContractID = "@mozilla.org/network/io-service;1";
    const iosvcIID = Ci.nsIIOService;

    var loader = Cc[jssslContractID].getService(jssslIID);
    var dirsvc = Cc[dirsvcContractID].getService(propsIID);
    var iosvc = Cc[iosvcContractID].getService(iosvcIID);

    // Note that unintuitively, __LOCATION__.parent == .
    // We expect to find the subscripts in ./../js
    var appdir = __LOCATION__.parent.parent;
    appdir.append("scripts");

    for (var i = 0; i < componentData.length; i++) {
      var scriptName = componentData[i].script;
      if (!scriptName)
        continue;

      var f = appdir.clone();
      f.append(scriptName);

      try {
        var fileurl = iosvc.newFileURI(f);
        loader.loadSubScript(fileurl.spec, null);
      }
      catch (e) {
        dump("Error while loading " + fileurl.spec + "\n");
        throw e;
      }
    }

    this.mScriptsLoaded = true;
  },

  registerSelf: function (compMgr, fileSpec, location, type) {
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);

    var catman = Cc["@mozilla.org/categorymanager;1"]
                   .getService(Ci.nsICategoryManager);
    for (var i = 0; i < componentData.length; i++) {
      var comp = componentData[i];
      if (!comp.cid)
        continue;
      compMgr.registerFactoryLocation(comp.cid,
                                      "",
                                      comp.contractid,
                                      fileSpec,
                                      location,
                                      type);

      if (comp.category) {
        var contractid;
        if (comp.service)
          contractid = "service," + comp.contractid;
        else
          contractid = comp.contractid;
        catman.addCategoryEntry(comp.category, comp.categoryEntry,
                                contractid, true, true);
      }
    }
  },

  makeFactoryFor: function(constructor) {
    var factory = {
      QueryInterface: function (aIID) {
        if (!aIID.equals(Ci.nsISupports) &&
            !aIID.equals(Ci.nsIFactory))
            throw Cr.NS_ERROR_NO_INTERFACE;
        return this;
      },

      createInstance: function(outer, iid) {
        if (outer != null)
          throw Cr.NS_ERROR_NO_AGGREGATION;
        return (new constructor()).QueryInterface(iid);
      }
    };

    return factory;
  },

  getClassObject: function(compMgr, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    if (!this.mScriptsLoaded)
      this.loadScripts();

    for (var i = 0; i < componentData.length; i++) {
      if (cid.equals(componentData[i].cid)) {
        if (componentData[i].onComponentLoad) {
          eval(componentData[i].onComponentLoad);
        }
        // eval to get usual scope-walking
        return this.makeFactoryFor(eval(componentData[i].constructor));
      }
    }

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(compMgr) {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return sbPlaylistHandlerModule;
}

