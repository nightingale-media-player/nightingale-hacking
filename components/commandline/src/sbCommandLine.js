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
 * \file sbCommandLine.js
 * \brief Implementation of the interface nsICommandLine
 * \todo Implement the -play functionality
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const SONGBIRD_CLH_CONTRACTID = "@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird";
const SONGBIRD_CLH_CID = Components.ID("{128badd1-aa05-4508-87cc-f3cb3e9b5499}");
const SONGBIRD_CLH_CLASSNAME = "Songbird Command Line Handler";
// "m" for ordinary priority see sbICommandLineHandler.idl
const SONGBIRD_CLH_CATEGORY= "m-songbird-clh";

/**
 * /brief Songbird commandline handler
 */
function sbCommandLineHandler() {
}

sbCommandLineHandler.prototype = {
  // there are specific formatting guidelines for help test, see nsICommandLineHandler
  helpInfo : "  -test [tests]        Run tests on the components listed in the\n" +
             "                       optional comma-separated list of tests.\n" +
             "                       If no tests are passed in ALL tests will be run.\n",

  handle : function (cmdLine) {
    var tests = null;
    var exception = false;
    try {
      tests = cmdLine.handleFlagWithParam("test", false);
    }
    catch (e) {
      // cmdLine throws if there is no param for the flag, but we want the
      // parameter to be optional, so catch the exception and let ourself
      // know that things are okay. The flag existed without a param.
      exception = true;
    }

    // if there was a parameter or if we had a flag and no param
    if (tests != null || exception) {
      // we're running tests, make sure we don't open a window
      cmdLine.preventDefault = true;
      var testHarness = Cc["@songbirdnest.com/Songbird/TestHarness;1"].getService(Ci.sbITestHarness);
      testHarness.init(tests);
      testHarness.run();
    }

    // XXX bug 2186
    // This section is an un-finished beginning of handling a commandline arg
    // pointing to media to be played on startup. We need to define how this
    // should be handled before implementing.
    /*
    try {
      // will throw if no param specified
      var uristr = cmdLine.handleFlagWithParam("play", false);
      if (uristr) {
        // convert uristr to an nsIURI
        var uri = cmdLine.resolveURI(uristr);

        // At this point we need to duplicate the code in the default
        // command line handler to find/open Songbird and play the
        // URI passed in.

        // since we're opening the window ourself, don't let the default do it
        cmdLine.preventDefault = true;
      }
    }
    catch (e) {
      Components.utils.reportError("incorrect parameter passed to -play on the command line.");
    }
    */
  },

  QueryInterface : function clh_QI(iid) {
    if (iid.equals(Ci.nsICommandLineHandler) ||
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}; // sbCommandeLineHandler

/**
 * /brief The module for getting the commandline handler
 */
const sbCommandLineHandlerModule = {
  registerSelf : function (compMgr, fileSpec, location, type) {
    compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_CLH_CID,
                                    SONGBIRD_CLH_CLASSNAME,
                                    SONGBIRD_CLH_CONTRACTID,
                                    fileSpec,
                                    location,
                                    type);

    var catMan = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    catMan.addCategoryEntry("command-line-handler",
                            SONGBIRD_CLH_CATEGORY,
                            SONGBIRD_CLH_CONTRACTID,
                            true,
                            true);
  },

  getClassObject : function (compMgr, cid, iid) {
    if (!cid.equals(SONGBIRD_CLH_CID))
      throw Cr.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    return this.mFactory;
  },

  mFactory : {
    createInstance : function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return (new sbCommandLineHandler()).QueryInterface(iid);
    }
  },

  unregisterSelf : function (compMgr, location, type) {
    compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.unregisterFactoryLocation(SONGBIRD_CLH_CID, location);

    var catMan = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    catMan.deleteCategoryEntry("command-line-handler", SONGBIRD_CLH_CATEGORY);
  },

  canUnload : function (compMgr) {
    return true;
  },

  QueryInterface : function (iid) {
    if ( !iid.equals(Ci.nsIModule) ||
         !iid.equals(Ci.nsISupports) )
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }

}; // sbCommandLineHandlerModule

function NSGetModule(comMgr, fileSpec)
{
  return sbCommandLineHandlerModule;
}

