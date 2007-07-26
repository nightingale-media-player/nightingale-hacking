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
 * \brief Tests to make sure the JS and the C++ properties match
 * \see bug 3628
 */

var gProps = {};

//// read the "Canonical" C++ property names
function readCPPProps() {
  // get the app directory
  var appDir = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties)
                         .get("resource:app", Components.interfaces.nsIFile);
  
  // Get the root of the source tree
  // XXX Mook: This sucks :(
  var topsrcdir = appDir.parent.parent;
  // mac paths are a bit different
  if (getPlatform() == "Darwin") {
    topsrcdir = appDir.parent.parent.parent.parent;
  }
  // Get to the C++ file we want to read
  var file = topsrcdir.clone();
  for each (var part in ["components", "property", "src", "sbStandardProperties.h"] )
    file.append(part);

  var istream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                        .createInstance(Components.interfaces.nsIFileInputStream);
  istream.init(file, 0x01 /*PR_RDONLY*/, 0400, 0);
  istream.QueryInterface(Components.interfaces.nsILineInputStream);
  
  // read lines and look for the property names
  do {
    var line = {};
    var hasMore = istream.readLine(line);
    var result = line.value.match(/"http:\/\/songbirdnest.com\/data\/1.0#([^"]+)"/);
    if (result) {
      gProps[result[1]] = true;
    }
  } while(hasMore);
  
  istream.close();
  return true;
}

function checkJSProps() {
  var loader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                         .getService(Components.interfaces.mozIJSSubScriptLoader);
  var context = {};
  loader.loadSubScript("chrome://songbird/content/scripts/songbird_interfaces.js", context);
  var props = {};
  for (i in context) {
    if (i == "SB_PROPERTY_PREFACE") {
      // skip the prefix
      continue;
    }
    if (!(/^SB_PROPERTY/.test(i))) {
      // not a property
      continue;
    }
    var prop = context[i].match(/http:\/\/songbirdnest.com\/data\/1.0#(\w+)/)[1];
    assertTrue(prop in gProps, "property " + prop + " is only in JavaScript");
    props[prop] = true;
  }
  for (prop in gProps) {
    assertTrue(prop in props, "property " + prop + " is only in C++");
  }
}

function checkJSMProps() {
  Components.utils.import("resource://app/components/sbProperties.jsm");
  var props = {};
  for (var i in SBProperties) {
    if (typeof SBProperties[i] != "string") continue;
    if (SBProperties[i].length <= SBProperties.base.length) {
      continue;
    }
    var prop = SBProperties[i].match(/http:\/\/songbirdnest.com\/data\/1.0#(\w+)/)[1];
    assertTrue(prop in gProps, "property " + prop + " is only in sbProperties.jsm");
    props[prop] = true;
  }
  for (prop in gProps) {
    assertTrue(prop in props, "property " + prop + " is not in sbProperties.jsm");
  }
}

function runTest() {
  if (readCPPProps()) {
    checkJSProps();
    checkJSMProps();
  }
}
