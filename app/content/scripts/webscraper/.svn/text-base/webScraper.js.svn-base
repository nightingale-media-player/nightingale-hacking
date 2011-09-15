// vim: set sw=2 :miv
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

/*
  The WebScraper is a system for extracting media from web pages.
  It is composed of a number of steps which run in the background
  after a page's DOM content is loaded, and fills in a playlist
  provided by the sbTabBrowserTab with any located media.
  
  The system is designed to be extensible, but at the moment,
  no extensibility API is provided.
  
  \sa scraperSteps.js
*/

Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");

function WebScraper(medialist) {
  this.medialist = medialist;

  // Add a .job to implement sbIJobProgress
  this.job = new SBJobUtils.JobBase();
  this.job._progress = 0;
  this.job._total = 0;
  
  // Create a DOM event listener set.
  this._domEventListenerSet = new DOMEventListenerSet();

  this._terminate = false;
  this._seenURLs = {};
}
WebScraper.prototype = {
  start: function WebScraper_start(aNode) {
    if (!aNode) {
      Components.utils.reportError(
        "WebScraper::start(aNode) called with nothing to scrape!\n");
      return;
    }
    
    let webScraper = this;
    webScraper.job.notifyJobProgressListeners();
    
    // TODO: This is where extensibility will come in, somehow.
    let steps = [
      WebScraperSteps.DocumentURLSource,
      WebScraperSteps.CancelScrape,
      WebScraperSteps.MediaURL,
      WebScraperSteps.Hacks_DropBadUrls,
      WebScraperSteps.DupeCheck,
      //WebScraperSteps.Squawk,
      WebScraperSteps.AddOriginPage,
      WebScraperSteps.CreateMediaItem,
      WebScraperSteps.ScanForMetadata,
      WebScraperSteps.Sink
    ];

    // build the pipeline with the given document
    let pipeline = null;
    for (let i in steps.reverse()) {
      pipeline = steps[i](webScraper, aNode, pipeline, this.medialist);
      pipeline.next();
    }

    // Watch for DOM nodes getting added to documents we scrape.
    if(aNode instanceof Document){
      this._domEventListenerSet.add
        (aNode,
         "DOMNodeInserted",
         function(event) { webScraper.start(event.originalTarget) },
         true);
    }

    // begin the pipeline.
    pseudoThread(pipeline);
  },

  cancel: function WebScraper_cancel() {
    this._domEventListenerSet.removeAll();
    this._terminate = true;
  }
};

// Handy bit of reusable code:
// do cooperative multitasking using the window event handler and generators
function pseudoThread(gen) {
  try{
    if (gen.next()) {
      window.setTimeout(function() {
        try {
          pseudoThread(gen);
        }
        catch (e) {
          Components.utils.reportError(e);
        }
          
      }, 0);
    } else {
      gen.close();	
    }
  } catch (e if e instanceof StopIteration) {
        // no worries
  } catch(e) {
    gen.close();
    Cu.reportError(e);
  };
}
