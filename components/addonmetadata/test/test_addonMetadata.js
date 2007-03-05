/*
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
 * \brief AddonMetadata test file
 */
 
// TODO:
//   - Test caching functionality 
//   - Test enabled/disabled extension functionality

const RDFURI_ADDON_ROOT               = "urn:songbird:addon:root"
const PREFIX_ADDON_URI                = "urn:songbird:addon:";
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_NS_SONGBIRD              = "http://www.songbirdnest.com/2007/addon-metadata-rdf#";

function SONGBIRD_NS(property) {
  return PREFIX_NS_SONGBIRD + property;
}

function EM_NS(property) {
  return PREFIX_NS_EM + property;
}

function ADDON_NS(id) {
  return PREFIX_ADDON_URI + id;
}



/**
 * Debug helper that serializes a DS to the console
 */
function dumpDS(prefix, ds) {
  var outputStream = {
    data: "",
    close : function(){},
    flush : function(){},
    write : function (buffer,count){
      this.data += buffer;
      return count;
    },
    writeFrom : function (stream,count){},
    isNonBlocking: false
  }

  var serializer = Components.classes["@mozilla.org/rdf/xml-serializer;1"]
                           .createInstance(Components.interfaces.nsIRDFXMLSerializer);
  serializer.init(ds);

  serializer.QueryInterface(Components.interfaces.nsIRDFXMLSource);
  serializer.Serialize(outputStream);
  
  outputStream.data.split('\n').forEach( function(line) {
    dump(prefix + line + "\n");
  });
}



/**
 * Minimal sanity check for the AddonMetadata Datasource.
 * Assumes that rubberducky will be installed.
 */
function runTest () {

  var rdfService = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                               .getService(Components.interfaces.nsIRDFService);
  var datasource = rdfService.GetDataSourceBlocking("rdf:addon-metadata");
    
  dumpDS("AddonMetadataTest DataSource: ", datasource);
  
  // Find rubberducky in the item container
  var rubberducky = null;
  var itemRoot = rdfService.GetResource(RDFURI_ADDON_ROOT);    
  var cu = Components.classes["@mozilla.org/rdf/container-utils;1"]
                     .getService(Components.interfaces.nsIRDFContainerUtils);
  var container = cu.MakeSeq(datasource, itemRoot);
  var addons = container.GetElements();

  while (addons.hasMoreElements()) {
    var addon = addons.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
    if (addon.Value == ADDON_NS("rubberducky@songbirdnest.com")) {
      rubberducky = addon;
      break;
    }
  }
  
  // Did we find rubberducky in the list?
  assertEqual(rubberducky != null, true);
  
  // Does it have a description?
  assertEqual(datasource.hasArcOut(rubberducky, 
      rdfService.GetResource(EM_NS("description"))), true); 
      
      
  // Good enough for now
  return Components.results.NS_OK;
}


