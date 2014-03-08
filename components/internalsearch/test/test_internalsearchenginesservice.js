/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test that the internal search engines service works as expected.
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  this.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  this.Ci = Components.interfaces;

const testEngine1 = {
    name: "test_engine-1",
    contractId: "songbird-internal-search",
    url: "http://google.com/?q="
};

function runTest() {
    var iseService = Cc["@getnightingale.com/Nightingale/internal-search-service;1"].getService(Ci.ngIInternalSearchEnginesService);
    var searchService = Cc["@mozilla.org/browser/search-service;1"].getService(Ci.nsIBrowserSearchService);
    assertTrue(iseService, "Couldn't get an instance of the Internal Search Service");
    //assertTrue(searchService.isInitialized, "Search service not inititalized");

    assertTrue(!searchService.getEngineByName(testEngine1.name), "Engine already exists, please check your build configuration");
    
    var thrown = false;
    try {
        iseService.registerInternalSearchEngine(testEngine1.name, "doesnt-exist");
    }
    catch(error) {
        thrown = true;
    }
    assertTrue(thrown, "Even though the given contractID doesn't implement sbISearchEngine, no error was thrown");
    assertEqual(iseService.getInternalSearchEngine(testEngine1.name), null, "Engine was registered even though the registration shouldn't have taken place");

    assertTrue(!iseService.registerInternalSearchEngine(testEngine1.name, testEngine1.contractId), "Internal Search Engine could be registered, even though no engine with those properties has been registered");
    assertEqual(iseService.getInternalSearchEngine(testEngine1.name), null, "Engine was registered even though the registration supposedly failed");

    searchService.addEngineWithDetails(testEngine1.name, null, null, testEngine1.name, "get", testEngine1.url);
    assertTrue(searchService.getEngineByName(testEngine1.name), "Test engine was not registered in browser search provider");

    assertTrue(iseService.registerInternalSearchEngine(testEngine1.name, testEngine1.contractId), "Internal Search Engine could not be registered, even though the given engine exists");
    assertTrue(!iseService.registerInternalSearchEngine(testEngine1.name, testEngine1.contractId), "Could reregister internal search engine, even though it has just been registered");

    assertTrue(iseService.getInternalSearchEngine(testEngine1.name), "Test Engine was not found, even though successfull registered");

    var internalSearchEngine, querySuccessfull = true;
    try {
        internalSearchEngine = iseService.getInternalSearchEngine(testEngine1.name).QueryInterface(Ci.ngIInternalSearchEngine);
    }
    catch(e) {
        querySuccessfull = false;
    }
    assertTrue(querySuccessfull, "Could not get Internal Search Engine from getInernalSearchEngine return value");
    assertEqual(internalSearchEngine.contractID, testEngine1.contractId, "ContracID of registered test engine does not match");
    assertTrue(!internalSearchEngine.liveSeach, "Live search is enabled, even though it should default to false");

    iseService.unregisterInternalSearchEngine(testEngine1.name);
    assertEqual(iseService.getInternalSearchEngine(testEngine1.name), null, "Internal Search Engine still registered, even though it has been unregistered");

    assertTrue(iseService.registerInternalSearchEngine(testEngine1.name, testEngine1.contractId, true), "Internal Search Engine could not be registered");
    assertTrue(iseService.getInternalSearchEngine(testEngine1.name).QueryInterface(Ci.ngIInternalSearchEngine).liveSearch, "Live seach is false, even though it has been set to true");

    iseService.unregisterInternalSearchEngine(testEngine1.name);
    searchService.removeEngine(searchService.getEngineByName(testEngine1.name));

    return;
}
