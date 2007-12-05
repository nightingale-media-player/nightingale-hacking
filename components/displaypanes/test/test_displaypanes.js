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

  log("Testing DisplayPanes Service:");

  paneMgr = Components.classes["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                      .getService(Components.interfaces.sbIDisplayPaneManager);

  testDisplayPanesService();

  log("OK");
}

function testDisplayPanesService() {

  var cbparam;
  var listener = {
    onRegisterContent: function(aPane) { cbparam = aPane; },
    onUnregisterContent: function(aPane) { cbparam = aPane; },
    onPaneInfoChanged: function(aPane) { cbparam = aPane; },
    onRegisterInstantiator: function(aInstantiator) { cbparam = aInstantiator; },
    onUnregisterInstantiator: function(aInstantiator) { cbparam = aInstantiator; },

    QueryInterface : function(iid) {
      if (iid.equals(Components.interfaces.sbIDisplayPaneListener) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  }
  
  paneMgr.addListener(listener);

  log("Testing pane registration");

  cbparam = null;
  paneMgr.registerContent("url1", "title1", "icon1", 10, 20, "group1", false);
  testInfo(cbparam, "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");

  cbparam = null;
  paneMgr.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  testInfo(cbparam, "url2", "title2", "icon2", 30, 40, "group2");
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");

  cbparam = null;
  paneMgr.registerContent("url3", "title3", "icon3", 50, 60, "group3", false);
  testInfo(cbparam, "url3", "title3", "icon3", 50, 60, "group3");
  testContent("url3", "url3", "title3", "icon3", 50, 60, "group3");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");

  cbparam = null;
  paneMgr.updateContentInfo("url3", "title4", "icon4");
  testInfo(cbparam, "url3", "title4", "icon4", 50, 60, "group3");
  
  log("Testing pane unregistration");

  cbparam = null;
  paneMgr.unregisterContent("url2");
  testInfo(cbparam, "url2", "title2", "icon2", 30, 40, "group2");
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url3", "url3", "title4", "icon4", 50, 60, "group3");

  cbparam = null;
  paneMgr.unregisterContent("url1");
  testInfo(cbparam, "url1", "title1", "icon1", 10, 20, "group1");
  testContent("url3", "url3", "title4", "icon4", 50, 60, "group3");

  cbparam = null;
  paneMgr.unregisterContent("url3");
  testInfo(cbparam, "url3", "title4", "icon4", 50, 60, "group3");
  var contentlist = paneMgr.contentList;
  assertBool(!contentlist.hasMoreElements(), "contentList should be empty");
  contentlist = null;

  paneMgr.registerContent("url1", "title1", "icon1", 10, 20, "group1", false);
  testContent("url1", "url1", "title1", "icon1", 10, 20, "group1");
  paneMgr.registerContent("url3", "title3", "icon3", 50, 60, "group3", false);
  testContent("url3", "url3", "title3", "icon3", 50, 60, "group3");
  paneMgr.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  testContent("url2", "url2", "title2", "icon2", 30, 40, "group2");
  paneMgr.unregisterContent("url3");
  paneMgr.unregisterContent("url1");
  paneMgr.unregisterContent("url2");
  contentlist = paneMgr.contentList;
  assertBool(!contentlist.hasMoreElements(), "contentList should be empty");
  contentlist = null;

  log("Testing host registration");

  cbparam = null;
  var h1 = makeInstantiator("group1");
  paneMgr.registerInstantiator(h1);
  assertEquals(cbparam.contentGroup, "group1", "contentGroup");

  cbparam = null;
  var h2 = makeInstantiator("group2");
  paneMgr.registerInstantiator(h2);
  assertEquals(cbparam.contentGroup, "group2", "contentGroup");

  cbparam = null;
  var h3 = makeInstantiator("group3");
  paneMgr.registerInstantiator(h3);
  assertEquals(cbparam.contentGroup, "group3", "contentGroup");

  var hosts = paneMgr.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group1", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group3", "contentGroup");
  hosts = null;
  
  log("Testing host unregistration");

  cbparam = null;
  paneMgr.unregisterInstantiator(h1);
  assertEquals(cbparam.contentGroup, "group1", "contentGroup");

  hosts = paneMgr.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  assertEquals(hosts.getNext().contentGroup, "group3", "contentGroup");

  hosts = null;
  cbparam = null;
  paneMgr.unregisterInstantiator(h3);
  assertEquals(cbparam.contentGroup, "group3", "contentGroup");

  hosts = paneMgr.instantiatorsList;
  assertBool(hosts.hasMoreElements(), "instantiatorsList should not be empty");
  assertEquals(hosts.getNext().contentGroup, "group2", "contentGroup");
  hosts = null;
  cbparam = null;
  paneMgr.unregisterInstantiator(h2);
  assertEquals(cbparam.contentGroup, "group2", "contentGroup");

  hosts = paneMgr.instantiatorsList;
  assertBool(!hosts.hasMoreElements(), "instantiatorsList should be empty");
  hosts = null;
  

  log("Testing spawning");
  paneMgr.registerInstantiator(h1);
  paneMgr.registerInstantiator(h2);
  paneMgr.registerInstantiator(h3);
  paneMgr.registerContent("url2", "title2", "icon2", 30, 40, "group2", false);
  loaded = null;
  group = null;
  paneMgr.showPane("url2");
  testInfo(loaded, "url2", "title2", "icon2", 30, 40, "group2");
  assertEquals(group, "group2", "group");

  paneMgr.removeListener(listener);
}

function testContent(url, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup) {
  var info = paneMgr.getPaneInfo(url);
  testInfo(info, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup);
}

function testInfo(info, matchurl, matchtitle, matchicon, matchwidth, matchheight, matchgroup) {
  assertEquals(info.contentUrl, matchurl, "contentUrl");
  assertEquals(info.contentTitle, matchtitle, "contentTitle");
  assertEquals(info.contentIcon, matchicon, "contentIcon");
  assertEquals(info.defaultWidth, matchwidth, "defaultWidth");
  assertEquals(info.defaultHeight, matchheight, "defaultHeight");
  assertEquals(info.suggestedContentGroups, matchgroup, "suggestedContentGroups");
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
      if (iid.equals(Components.interfaces.sbIDisplayPaneInstantiator) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  }
  return instantiator;
}

