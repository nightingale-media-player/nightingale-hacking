/*
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
*/

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');


/**
 * sbCoverScanner Component
 */
function sbCoverScanner() {
  this.DEBUG = false;
  this._shutdown = false;
  this._fetcher = null;
  
  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}
  
  this._albumArtService = Cc['@songbirdnest.com/songbird-album-art-service;1']
                            .getService(Ci.sbIAlbumArtService);
}
sbCoverScanner.prototype = {
  className: 'sbCoverScanner',
  classDescription: 'Songbird Cover Scanner',
  classID: Components.ID('{4ea34ad3-fefa-497f-9394-2cf988f63ebb}'),
  contractID: '@songbirdnest.com/Songbird/cover/cover-scanner;1'
}

// nsISupports
sbCoverScanner.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.sbICoverScanner,
                       Ci.sbICoverListener]);

// sbICoverScanner
sbCoverScanner.prototype.fetchCover =
function sbCoverScanner_fetchCover(aMediaItem, aListener) {
  if (this._shutdown) {
    this._logError("Attempt to fetch cover when already shutdown.");
    return;
  }
  
  this._debug("Fetch Cover called");
  this._fetchers = this._albumArtService.getFetcherList();
  this._fetcherIndex = 0;
  this._coverListener = aListener;
  this._mediaItem = aMediaItem;
  this._getCover();
}

sbCoverScanner.prototype.shutdown =
function sbCoverScanner_shutdown() {
  if (this._fetcher) {
    this._fetcher.shutdown();
    this._fetcher = null;
  }
  this._shutdown = true;
}

// Internal

/**
 * \brief Dumps out a message if the DEBUG flag is enabled with
 *        the className pre-appended.
 * \param message String to print out.
 */
sbCoverScanner.prototype._debug =
function sbCoverScanner__debug(message) {
  if (!this.DEBUG) return;
  try {
    dump(this.className + ": " + message + "\n");
    this._consoleService.logStringMessage(this.className + ": " + message);
  } catch (err) {
    // We do not want to throw exception here
  }

}

/**
 * \brief Dumps out an error message with "the className and "[ERROR]"
 *        pre-appended, and will report the error so it will appear in the
 *        error console.
 * \param message String to print out.
 */
sbCoverScanner.prototype._logError =
function sbCoverScanner__logError(message) {
  try {
    dump(this.className + ": [ERROR] - " + message + "\n");
    Cu.reportError(this.className + ": [ERROR] - " + message);
  } catch (err) {
    // We do not want to throw exception here
  }
}

sbCoverScanner.prototype._getCover =
function sbCoverScanner__getCover() {
  if (this._shutdown) {
    this._logError("Get Cover called when already shutdown!");
    return;
  }

  if (!this._fetchers ||
      this._fetchers.length < this._fetcherIndex ||
      this._fetcherIndex < 0) {
    this._logError("Invalid index of the fetcher list [" + this._fetcherIndex +
                   "] Fetcher List = " + (this._fetchers ? this._fetchers.length : "null"));
    return;
  }

  var fetcher = this._fetchers.queryElementAt(this._fetcherIndex,
                                              Ci.nsISupportsString);
  var fetcherCid = fetcher.toString();

  if (typeof(fetcherCid) == 'undefined') {
    this._debug("Unable to get valid fetcher at index " + this._fetcherIndex);
    this._coverListener.coverFetchFailed(aMediaItem, aScope);
  } else {
    try {
      if (this._fetcher) {
        this._fetcher.shutdown();
        this._fetcher = null;
      }
      
      this._fetcher = Cc[fetcherCid].createInstance(Ci.sbICoverFetcher);
      if (this._fetcher.isEnabled) {
        this._debug("Trying to find cover from fetcher : " + this._fetcher.name);
        this._fetcher.fetchCoverForMediaItem(this._mediaItem, this, null);
      } else {
        // Fail internally so it will check the next fetcher or fail if none left.
        this.coverFetchFailed(this._mediaItem, null);
      }
    } catch (err) {
      this._logError("Unable to get fetcher: " + fetcherCid + ": " + err);
      this._coverListener.coverFetchFailed(this._mediaItem, null);
    }
  }
}

// sbICoverListener
sbCoverScanner.prototype.coverFetchSucceeded =
function sbCoverScanner_coverFetchSucceeded(aMediaItem, aScope, aURI) {
  if (this._shutdown) {
    this._logError("Recieved coverFetchSucceeded when shutdown");
    return;
  }

  // If aURL isn't a local scheme, we need to download it...
  var ioService =  Cc['@mozilla.org/network/io-service;1']
                     .getService(Ci.nsIIOService);
  this._debug("Checking aURI of " + aURI);
  var uri = ioService.newURI(aURI, null, null);
  this._debug("Fetched uri scheme is " + uri.scheme);
  switch (uri.scheme) {
    case 'file':
    case 'data':
    case 'chrome':
      // we found something
      this._debug("Cover fetch succeeded, URI = " + aURI);
      this._coverListener.coverFetchSucceeded(aMediaItem, aScope, aURI);
    break;

    // Try to download every thing else
    default:
      this._debug('Starting cover downloader for :' + aURI);
      var downloader = Cc["@songbirdnest.com/Songbird/sbCoverDownloader;1"]
                         .createInstance(Ci.sbICoverDownloader);
      downloader.downloadCover(aURI, aMediaItem, this, aScope);
    break;
  }
}

sbCoverScanner.prototype.coverFetchFailed =
function  sbCoverScanner_coverFetchFailed(aMediaItem, aScope) {
  if (this._shutdown) {
    this._logError("Recieved coverFetchFailed when shutdown");
    return;
  }

  this._debug("Fetch Cover Failed on index " + this._fetcherIndex);
  this._fetcherIndex++;
  if (this._fetcherIndex < this._fetchers.length) {
    this._debug("Trying next fetcher " + this._fetcherIndex);
    this._getCover();
  } else {
    this._debug("No more fetchers to try, failing");
    this._coverListener.coverFetchFailed(aMediaItem, aScope);
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbCoverScanner]);
}
