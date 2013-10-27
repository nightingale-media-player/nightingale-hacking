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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/DebugUtils.jsm");

const LOG = DebugUtils.generateLogFunction("ngInternalSearchService", 2);

var ngInternalSearchServiceInstance;

function ngInternalSearchService()
{
    if(ngInternalSearchServiceInstance)
        return ngInternalSearchServiceInstance;
    ngInternalSearchServiceInstance = this;    
        
    this.searchService =
      Components.classes["@mozilla.org/browser/search-service;1"].getService(Components.interfaces.nsIBrowserSearchService);
}

ngInternalSearchService.prototype = {
    classDescription: "Nightingale Internal Search Engine Registering Serivce",
    classID:          Components.ID("{738e3a66-d7b3-4c7d-94ec-a158eb753203}"),
    contractID:       "@getnightingale.com/Nightingale/internal-search-service;1",
    QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.ngIInternalSearchEnginesService]),

    /**
   * Method used to register a searchengine which should be treated as internal.
   * If the engine was hidden it will now be visible.
   * @param: searchEngineName:   Name of the targeted engine
   *         contractID:         part of the ID of the search engine handler
   *                             contract (implementing sbISearchEngine)                           
   *         liveSearch:         boolean; Whether the search should be triggered
   *                             on every keydown or only on submit
   * @return true if registered successfully, else false.
   */
  registerInternalSearchEngine : function(searchEngineName, contractID, liveSearch) {
        var engine = this.searchService.getEngineByName(searchEngineName);
        
        // Only continue if the engine isn't yet registered and exists
        if(!this.internalEngines.hasOwnProperty(searchEngineName)&&engine) {
            // default liveSearch to false
            if(liveSearch === undefined)
                liveSearch = false;
                
            this.internalEngines[searchEngineName] =
                           {'liveSearch':liveSearch,'contractID':contractID,
                                                    'wasHidden':engine.hidden};
            
            // If the engine is hidden, make it visible
            if(engine.hidden)
                engine.hidden = false;
                
            return true;
        }
        LOG("\n\nCouldn't register a search engine handler for \"" + searchEngineName +
            "\".There either is already a handler for the search engine with this name or there is no engine with this name registered.\n");
        return false;
    },
  
  /**
   * Removes the internal binding from the searchengine specified with searchEngineName
   * and rehides it, if it was hidden before being registered
   */  
  unregisterInternalSearchEngine :
    function SearchHandler_unregisterInternalSearchEngine(searchEngineName) {
        if(this.internalEngines.hasOwnProperty(searchEngineName)) {
            // reset the hidden property to the default
            if(this.internalEngines[searchEngineName].wasHidden)
                this.searchService.getEngineByName(searchEngineName).hidden = false;
                
            delete this.internalEngines[searchEngineName];
        }
        LOG("\n\nThere is no internal search engine for the name \""
            + searchEngineName + "\" registered.\n");
    },
    
    getInternalSearchEngine: function(searchEngineName) {
        if(this.internalEngines.hasOwnProperty(searchEngineName))
            return this.internalEngines[searchEngineName];
        return null;
    },
    
    internalEngines: {}
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngInternalSearchService]);
