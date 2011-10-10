/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *   Mozilla      
 *
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK *****
 */

/*
 * Original test file pulled from the mozilla source:
 * mozilla/toolkit/mozapps/extensions/test/unit/test_bug384052.js
 * and modified by:
 *   John Gaunt <redfive@getnightingale.com>
 */

//const Cc = Components.classes;
//const Ci = Components.interfaces;

const CLASS_ID = Components.ID("{12345678-1234-1234-1234-123456789abc}");
//const CONTRACT_ID = "@mozilla.org/test-parameter-source;1";
const CONTRACT_ID = "@getnightingale.com/moz/sburlformatter;1";

var gTestURL = "http://127.0.0.1:4444/?itemID=%ITEM_ID%&custom1=%CUSTOM1%&custom2=%CUSTOM2%";
var gExpectedURL = null;
var gSeenExpectedURL = false;

var gComponentRegistrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
var gObserverService = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
var gCategoryManager = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);

// Factory for our parameter handler
var paramHandlerFactory = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFactory) || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  createInstance: function(outer, iid) {
    var bag = Cc["@mozilla.org/hash-property-bag;1"]
                .createInstance(Ci.nsIWritablePropertyBag);
    bag.setProperty("CUSTOM1", "custom_parameter_1");
    bag.setProperty("CUSTOM2", "custom_parameter_2");
    return bag.QueryInterface(iid);
  }
};

// Observer that should recognize when extension manager requests our URL
var observer = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIObserver) || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  observe: function(subject, topic, data) {
    if (topic == "http-on-modify-request") {
      var channel = subject.QueryInterface(Ci.nsIChannel);
      if (channel.originalURI.spec == gExpectedURL) {
        log("got notification,\n exp: " + gExpectedURL + "\n got: " + channel.originalURI.spec + "\n");
        gSeenExpectedURL = true;
      }
    }
  }
}

function initTest()
{
  // Setup extension manager
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  startupEM();

  // Register our parameter handlers
//  gComponentRegistrar.registerFactory(CLASS_ID, "Test component", CONTRACT_ID, paramHandlerFactory);
//  gCategoryManager.addCategoryEntry("extension-update-params", "CUSTOM1", CONTRACT_ID, false, false);
//  gCategoryManager.addCategoryEntry("extension-update-params", "CUSTOM2", CONTRACT_ID, false, false);

  // Register observer to get notified when URLs are requested
  gObserverService.addObserver(observer, "http-on-modify-request", false);
}

function shutdownTest()
{
  shutdownEM();

//  gComponentRegistrar.unregisterFactory(CLASS_ID, paramHandlerFactory);
//  gCategoryManager.deleteCategoryEntry("extension-update-params", "CUSTOM1", false);
//  gCategoryManager.deleteCategoryEntry("extension-update-params", "CUSTOM2", false);

  gObserverService.removeObserver(observer, "http-on-modify-request");
}

//function run_test()
function runTest()
{
  initTest();

  var item = Cc["@mozilla.org/updates/item;1"].createInstance(Ci.nsIUpdateItem);
  item.init("test@mozilla.org", "1.0", "app-profile", "0.0", "100.0", "Test extension",
            null, null, "", null, null, item.TYPE_EXTENSION, "xpcshell@tests.mozilla.org");

  gExpectedURL = gTestURL.replace(/%ITEM_ID%/, item.id)
                         .replace(/%CUSTOM1%/, "custom_parameter_1")
                         .replace(/%CUSTOM2%/, "custom_parameter_2");

  // Replace extension update URL
  var origURL = null;
  try {
    origURL = prefs.getCharPref("extensions.update.url");
  }
  catch (e) {}
  gPrefs.setCharPref("extensions.update.url", gTestURL);

  // Initiate update
  gEM.update([item], 1, 2, null);

  //do_check_true(gSeenExpectedURL);
  assertTrue(gSeenExpectedURL, "gSeenExpectedURL is FALSE");

  if (origURL)
    gPrefs.setCharPref("extensions.update.url", origURL);
  else
    gPrefs.clearUserPref("extensions.update.url");

  shutdownTest();
}
