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

const RDFURI_INSTALL_MANIFEST_ROOT    = "urn:mozilla:install-manifest";
const TYPE_THEME = 4;
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";

function getManifestProperty(manifest, property) {
  var target = manifest.GetTarget(null, gRDF.GetResource(EM_NS(property)), true);
  var val = stringData(target);
  return val === undefined ? intData(target) : val;
}

function stringData(literalOrResource) {
  if (literalOrResource instanceof Components.interfaces.nsIRDFLiteral)
    return literalOrResource.Value;
  if (literalOrResource instanceof Components.interfaces.nsIRDFResource)
    return literalOrResource.Value;
  return undefined;
}

function intData(literal) {
  if (literal instanceof Components.interfaces.nsIRDFInt)
    return literal.Value;
  return undefined;
}

function EM_NS(property) {
  return PREFIX_NS_EM + property;
}

function ITEM_NS(id) {
  return PREFIX_ITEM_URI + id;
}

function fillFeathersList(menu) {
  try 
  {
    /* clear the existing children */
    var children = menu.childNodes;
    for (var i = children.length; i > 0; --i) {
      menu.removeChild(children[i - 1]);
    }

    /* access the list of skins from the extensions manager and read extra data using the corresponding rdf datasource */
    var extmgr = Components.classes["@mozilla.org/extensions/manager;1"].getService(Components.interfaces.nsIExtensionManager);
    var RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
     
    var items = extmgr.getItemList(TYPE_THEME, {});
    for (var i in items)
    {
      var item = RDF.GetResource(ITEM_NS(items[i].id));
      var target = extmgr.datasource.GetTarget(item, RDF.GetResource(EM_NS("internalName")), true);
      var internalName = stringData(target);
      if (internalName == undefined) internalName = intData(target);
      if (internalName == undefined) internalName = "";
      var feathers = items[i].name;

      var item = document.createElement("menuitem");
      className = menu.parentNode.getAttribute("class");

      item.setAttribute("label", feathers);
      item.setAttribute("featherid", internalName);
      item.setAttribute("name", "feathers.switch");
      item.setAttribute("type", "radio");
      item.setAttribute("class", className);
      var curFeathers = ""; // !
      if (curFeathers == feathers) {
        item.setAttribute("checked", "true");
      }
      item.setAttribute("oncommand", "switchFeathers(\"" + internalName + "\")");

      menu.appendChild(item);
    }
  }
  catch ( err )
  {
    alert( "switch_feathers.xml - " + err );
  }
}
