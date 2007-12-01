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
 * \brief AddOn Panes Unit Test File
 */
 
var addOnPanes = null;
var loaded = null;
var group = null;

function runTest () {

  log("Testing AddOnPanes Service:");

  addOnPanes = Components.classes["@songbirdnest.com/Songbird/AddOnPanes;1"]
                   .getService(Components.interfaces.sbIAddOnPanes);

  testAddOnPanesService();

  log("OK");
}

function testAddOnPanesService() {

  var cbparam;
  var listener = {
    onRegisterContent: function(aPane) { cbparam = aPane; },
    onUnregisterContent: function(aPane) { cbparam = aPane; },
    onPaneInfoChanged: function(aPane) { cbparam = aPane; },
    onRegisterInstantiator: function(aInstantiator) { cbparam = aInstantiator; },
    onUnregisterInstantiator: function(aInstantiator) { cbparam = aInstantiator; },

    QueryInterface : function(iid) {
      if (iid.equals(Components.interfaces.sbIAddOnPanesListener) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  }
  
  addOnPanes.addListener(listener);

  log("Testing pane registration");

  cbparam = null;
  addOnPanes.registerContent("url1", "title1", "icon1", 10, 20, "group1", false);
  testInfo(cbparam, "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");

  cbparam = null;
  addOnPanes.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  testInfo(cbparam, "url2", "title2", "icon2", 30, 40, "group2");
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");

  cbparam = null;
  addOnPanes.registerContent("url3", "title3", "icon3", 50, 60, "group3", false);
  testInfo(cbparam, "url3", "title3", "icon3", 50, 60, "group3");
  testContent("url3", "url3", "title3", "icon3", 50, 60, "group3");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");

  cbparam = null;
  addOnPanes.updateContentInfo("url3", "title4", "icon4");
  testInfo(cbparam, "url3", "title4", "icon4", 50, 60, "group3");
  
  log("Testing pane unregistration");

  cbparam = null;
  addOnPanes.unregisterContent("url2");
  testInfo(cbparam, "url2", "title2", "icon2", 30, 40, "group2");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url3", "url3", "title4", "icon4", 50, 60, "group3");

  cbparam = null;
  addOnPanes.unregisterContent("url1");
  testInfo(cbparam, "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url3", "url3", "title4", "icon4", 50, 60, "group3");

  cbparam = null;
  addOnPanes.unregisterContent("url3");
  testInfo(cbparam, "url3", "title4", "icon4", 50, 60, "group3");
  var contentlist = addOnPanes.contentList;
  assertBool(!contentlist.hasMoreElements(), "contentList should be empty");
  contentlist = null;

  addOnPanes.registerContent("url1", "title1", "icon1", 10, 20, "group1", false);
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  addOnPanes.registerContent("url3", "title3", "icon3", 50, 60, "group3", false);
  testContent("url3", "url3", "title3", "icon3", 50, 60, "group3");
  addOnPanes.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");
  addOnPanes.unregisterContent("url3");
  addOnPanes.unregisterContent("url1");
  addOnPanes.unregisterContent("url2");
  contentlist = addOnPanes.contentList;
  assertBool(!contentlist.hasMoreElements(), "contentList should be empty");
  contentlist = null;

  log("Testing host registration");

  cbparam = null;
  var h1 = makeInstantiator("group1");
  addOnPanes.registerInstantiator(h1);
  assertEquals(cbparam.contentGroup, "group1", "contentGroup");

  cbparam = null;
  var h2 = makeInstantiator("group2");
  addOnPanes.registerInstantiator(h2);
  assertEquals(cbparam.contentGroup, "group2", "contentGroup");

  cbparam = null;
  var h3 = makeInstantiator("group3");
  addOnPanes.registerInstantiator(h3);
  assertEquals(cbparam.contentGroup, "group3", "contentGroup");

  var hosts = addOnPanes.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group1", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group3", "contentGroup");
  hosts = null;
  
  log("Testing host unregistration");

  cbparam = null;
  addOnPanes.unregisterInstantiator(h1);
  assertEquals(cbparam.contentGroup, "group1", "contentGroup");

  hosts = addOnPanes.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group3", "contentGroup");

  hosts = null;
  cbparam = null;
  addOnPanes.unregisterInstantiator(h3);
  assertEquals(cbparam.contentGroup, "group3", "contentGroup");

  hosts = addOnPanes.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  hosts = null;
  cbparam = null;
  addOnPanes.unregisterInstantiator(h2);
  assertEquals(cbparam.contentGroup, "group2", "contentGroup");

  hosts = addOnPanes.instantiatorsList;
  assertBool(!hosts.hasMoreElements(), "instantiatorsList should be empty");
  hosts = null;
  

  log("Testing spawning");
  addOnPanes.registerInstantiator(h1);
  addOnPanes.registerInstantiator(h2);
  addOnPanes.registerInstantiator(h3);
  addOnPanes.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  loaded = null;
  group = null;
  addOnPanes.showPane("url2");
  testInfo(loaded, "url2", "title2", "icon2", 30, 40, "group2");
  assertEquals(group, "group2", "group");

  addOnPanes.removeListener(listener);
}

function testContent(url, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup) {
  var info = addOnPanes.getPaneInfo(url);
  testInfo(info, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup);
}

function testInfo(info, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup) {
  assertEquals(info.contentUrl, matchurl, "contentUrl");
  assertEquals(info.contentTitle, matchtitle, "contentTitle");
  assertEquals(info.contentIcon, matchicon, "contentIcon");
  assertEquals(info.defaultWidth, matchwidth, "defaultWidth");
  assertEquals(info.defaultHeight, matchheight, "defaultHeight");
  assertEquals(info.suggestedContentGroup, matchgroup, "suggestedContentGroup");
}

function assertEquals(s1, s2, msgprefix) {
  if (s1 != s2) {
    fail(msgprefix + " should be " + s2 + ", not " + s1 + "!");
  }
}

function assertBool(boolval, msg) {
  if (!boolval) {
    fail(msg);
  }
}

function assertValue(value, expected, msg) {
  if (value != expected) {
    fail(msg);
  }
}

function makeInstantiator(groupid) {
  var instantiator = {
    get contentGroup() { return groupid; },
    get contentUrl() { return ""; },
    get contentTitle() { return ""; },
    get contentIcon() { return ""; },
    get collapsed() {},
    set collapsed(val) {},
    loadContent: function(aPane) { loaded = aPane; group = groupid; },
    hide: function() { },
    QueryInterface : function(iid) {
      if (iid.equals(Components.interfaces.sbIAddOnPanesInstantiator) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  }
  return instantiator;
}

