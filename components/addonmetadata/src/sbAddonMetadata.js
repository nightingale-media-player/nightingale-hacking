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
 

//
// TODO: 
//   * Do not load disabled extensions!
//   * Cache, profile, etc.
//   * Remove debug dumps
//

/**
 * \file sbAddonMetadata.js
 * \brief Provides an nsIRDFDataSource with the contents of all 
 *        addon install.rdf files.
 */ 
 
const CONTRACTID = "@mozilla.org/rdf/datasource;1?name=addon-metadata";
const CLASSNAME = "Songbird Addon Metadata Datasource";
const CID = Components.ID("{a1edd551-0f29-4ce9-aebc-92fbee77f37e}");
const IID = Components.interfaces.nsIRDFDataSource;

const FILE_INSTALL_MANIFEST = "install.rdf";
const RDFURI_INSTALL_MANIFEST_ROOT    = "urn:mozilla:install-manifest";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";


function SONGBIRD_NS(property) {
  return PREFIX_NS_SONGBIRD + property;
}

function EM_NS(property) {
  return PREFIX_NS_EM + property;
}

function ITEM_NS(id) {
  return PREFIX_ITEM_URI + id;
}



/**
 * /class AddonMetadata
 * /brief Provides an nsIRDFDataSource with the contents of all 
 *        addon install.rdf files.
 */
function AddonMetadata() {
  dump("\nAddonMetadata: constructed\n");
  
  this._RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                   .getService(Components.interfaces.nsIRDFService);

  
  this._datasource = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"]
                               .createInstance(Components.interfaces.nsIRDFDataSource);
  this._buildDatasource();
  
  var os  = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  os.addObserver(this, "xpcom-shutdown", false);
};

AddonMetadata.prototype = {

  constructor: AddonMetadata,

  _RDF: null,
  _datasource: null,

  
  /**
   * Populate the datasource with the contents of all 
   * addon install.rdf files.
   */
  _buildDatasource: function _buildDatasource() {
    dump("\nAddonMetadata: _buildDatasource \n");

    // Make a container to list all installed extensions
    var itemRoot = this._RDF.GetResource(RDFURI_ITEM_ROOT);    
    var cu = Components.classes["@mozilla.org/rdf/container-utils;1"]
                       .getService(Components.interfaces.nsIRDFContainerUtils);
    var container = cu.MakeSeq(this._datasource, itemRoot);

    var extManager = Components.classes["@mozilla.org/extensions/manager;1"]
                           .getService(Components.interfaces.nsIExtensionManager);
   
    // Read the install.rdf for every addon
    var addons = extManager.getItemList(Components.interfaces.nsIUpdateItem.TYPE_ADDON, {});
    for (var i = 0; i < addons.length; i++)
    {
      var id = addons[i].id;
      dump("\nAddonMetadata:  loading install.rdf for id {" + id +  "}\n");
      
      var location = extManager.getInstallLocation(id);
      var installManifestFile = location.getItemFile(id, FILE_INSTALL_MANIFEST);

      if (!installManifestFile.exists()) {
        this._reportErrors(["install.rdf for id " + id +  " was not found " + 
                  "at location " + installManifestFile.path]);
      }
      
      var manifestDS = this._getDatasource(installManifestFile);
      var itemNode = this._RDF.GetResource(ITEM_NS(id));
      // Copy the install.rdf metadata into the master datasource
      this._copyManifest(itemNode, manifestDS);
      
      // Add the new install.rdf root to the list of extensions
      container.AppendElement(itemNode);
    }
    
    dump("\nAddonMetadata: _buildDatasource complete \n");
  },
  
 
  
  /**
   * Copy all nodes and assertions into the main datasource,
   * renaming the install manifest to the id of the extension.
   */
  _copyManifest:  function _copyManifest(itemNode, manifestDS) {
  
    // Copy all resources from the manifest to the main DS
    var resources = manifestDS.GetAllResources();
    while (resources.hasMoreElements()) {
      var resource = resources.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      
      // Get all arcs out of the resource
      var arcs = manifestDS.ArcLabelsOut(resource);
      while (arcs.hasMoreElements()) {
        var arc = arcs.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
              
        // For each arc, get all targets
        var targets = manifestDS.GetTargets(resource, arc, true);
        while (targets.hasMoreElements()) {
          var target = targets.getNext().QueryInterface(Components.interfaces.nsIRDFNode);
          
          // If this resource is the manifest root, replace it
          // with a resource representing the current addon
          newResource = resource;
          if (resource.Value == RDFURI_INSTALL_MANIFEST_ROOT) {
            newResource = itemNode;
          }
                    
          // Otherwise, assert into the main ds
          this._datasource.Assert(newResource, arc, target, true);
        }
      }                   
    }
  },

  



  /**
   * Gets a datasource from a file.
   * @param   file
   *          The nsIFile that containing RDF/XML
   * @returns RDF datasource
   */
  _getDatasource: function _getDatasource(file) {
    var ioServ = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);
    var fph = ioServ.getProtocolHandler("file")
                    .QueryInterface(Components.interfaces.nsIFileProtocolHandler);

    var fileURL = fph.getURLSpecFromFile(file);
    var ds = this._RDF.GetDataSourceBlocking(fileURL);

    return ds;
  },


  
  _deinit: function _deinit() {
    dump("\nAddonMetadata: deinit\n");

    this._RDF = null;
    this._datasource = null;
  },
    
  

  // watch for XRE startup and shutdown messages 
  observe: function(subject, topic, data) {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    switch (topic) {
    case "xpcom-shutdown":
      os.removeObserver(this, "xpcom-shutdown");
      this._deinit();
      break;
    }
  },
  
  
  
  _reportErrors: function _reportErrors(errorList) {
    var consoleService = Components.classes["@mozilla.org/consoleservice;1"].
         getService(Components.interfaces.nsIConsoleService);
    for (var i = 0; i  < errorList.length; i++) {
      consoleService.logStringMessage("Addon Metadata Reader: " + errorList[i]);
      dump("AddonMetadataReader: " + errorList[i] + "\n");
    }
  },



  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    
    // THIS IS SO WRONG! 
    // We can get away with it though since we really only want the datasource
    // and never the object that builds it.
    if (iid.equals(IID)) {
      return this._datasource;
    }
    
    return this;
  }
}; // AddonMetadata.prototype







/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this._objects) {
      var obj = this._objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  _makeFactory: #1= function(ctor) {
    function ci(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new ctor()).QueryInterface(iid);
    } 
    return { createInstance: ci };
  },
  
  _objects: {
    AddonMetadata:     {   CID        : CID,
                           contractID : CONTRACTID,
                           className  : CLASSNAME,
                           factory    : #1#(AddonMetadata)
                         },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule


