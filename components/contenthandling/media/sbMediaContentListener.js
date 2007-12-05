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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

// From nsNetError.h
const NS_BINDING_ABORTED = 0x804b0002;

const DESCRIPTION = "sbMediaContentListener";
const CID         = "2803c9e8-b0b6-4dfe-8333-53430128f7e7";
const CONTRACTID  = "@songbirdnest.com/contentlistener/media;1";

const CONTRACTID_ARRAY              = "@mozilla.org/array;1";
const CONTRACTID_LIBRARYMANAGER     = "@songbirdnest.com/Songbird/library/Manager;1";
const CONTRACTID_METADATAJOBMANAGER = "@songbirdnest.com/Songbird/MetadataJobManager;1";
const CONTRACTID_OBSERVERSERVICE    = "@mozilla.org/observer-service;1";
const CONTRACTID_PLAYLISTPLAYBACK   = "@songbirdnest.com/Songbird/PlaylistPlayback;1";
const CONTRACTID_PREFSERVICE        = "@mozilla.org/preferences-service;1";

const CATEGORY_CONTENT_LISTENER = "external-uricontentlisteners";

const PREF_WEBLIBRARY_GUID = "songbird.library.web";

const TYPE_MAYBE_MEDIA = "application/vnd.songbird.maybe.media";

// For XPCOM boilerplate.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

// For Songbird properties.
Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

/**
 * An implementation of nsIURIContentListener that prevents audio and video
 * files from being downloaded through the Download Manager.
 *
 * This component works hand-in-hand with the sbMediaSniffer component. When
 * a URI is first resolved the content sniffer is called to override the MIME
 * type of files that we recognize as media files. Thereafter no other content
 * listeners will be able to support the MIME type except for this component.
 */
function sbMediaContentListener() {
}
sbMediaContentListener.prototype = {
  _parentContentListener: null,
  
  /**
   * Needed by XPCOMUtils.
   */
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,

  /**
   * Takes care of adding a url to the library and playing it.
   */
  _handleURI: function _handleURI(aURI, aPPS) {
    // TODO: Check a pref to determine if this should be going into our library?
    var libraryManager = Cc[CONTRACTID_LIBRARYMANAGER].
                         getService(Ci.sbILibraryManager);
    var library;
    
    if (aURI instanceof Ci.nsIFileURL) {
      // Local files go into the main library.
      library = libraryManager.mainLibrary;
    }
    else {
      // All other files go into the web library.
      var prefService =
        Cc[CONTRACTID_PREFSERVICE].getService(Ci.nsIPrefBranch);
      
      var webLibraryGUID = prefService.getCharPref(PREF_WEBLIBRARY_GUID);
      library = libraryManager.getLibrary(webLibraryGUID);
    }

    // Try to see if we've already found and scanned this url.
    var listener = {
      foundItem: null,
      onEnumerationBegin: function onEnumerationBegin() {
        return true;
      },
      onEnumeratedItem: function onEnumeratedItem(list, item) {
        this.foundItem = item;
        return false;
      },
      onEnumerationEnd: function onEnumerationEnd() {
        return;
      }
    };

    var url = aURI.spec;
    library.enumerateItemsByProperty(SBProperties.contentURL, url, listener );
    if (!listener.foundItem) {
      var mediaItem = library.createMediaItem(aURI);

      // Get metadata going.
      var scanArray = Cc[CONTRACTID_ARRAY].createInstance(Ci.nsIMutableArray);
      scanArray.appendElement(mediaItem, false);

      var metadataJobManager = Cc[CONTRACTID_METADATAJOBMANAGER].
                               getService(Ci.sbIMetadataJobManager);
      var metadataJob = metadataJobManager.newJob(scanArray, 5);
    }

    var view = library.createView();
    var filter = LibraryUtils.createConstraint([
      [
        [SBProperties.contentURL, url]
      ]
    ]);
    view.filterConstraint = filter;;

    aPPS.playView(view, 0);
  },

  /**
   * See nsIURIContentListener.
   */
  onStartURIOpen: function onStartURIOpen(aURI) {
    return false;
  },

  /**
   * See nsIURIContentListener.
   */
  doContent: function doContent(aContentType, aIsContentPreferred, aRequest, aContentHandler) {
    // XXXben Sometime in the future we may have a stream listener that we can
    //        hook up here to grab the file as it is being downloaded. For now,
    //        though, cancel the request and send the URL off to the playback
    //        service.
    var channel = aRequest.QueryInterface(Ci.nsIChannel);
    var uri = channel.URI;

    var pps = Cc[CONTRACTID_PLAYLISTPLAYBACK].
              getService(Ci.sbIPlaylistPlayback);

    if (!pps.isMediaURL(uri.spec)) {
      // Hmm, badness. We can't actually play this file type. Throw an error
      // here to get the URILoader to keep trying with other content listeners.
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    // Let exceptions propogate from here!
    try {
      this._handleURI(uri, pps);
    }
    catch (e) {
      Components.utils.reportError(e);
      throw e;
    }

    // The request is active, so make sure to cancel it.
    aRequest.cancel(NS_BINDING_ABORTED);

    return true;
  },

  /**
   * See nsIURIContentListener.
   */
  isPreferred: function isPreferred(aContentType, aDesiredContentType) {
    return aContentType == TYPE_MAYBE_MEDIA;
  },

  /**
   * See nsIURIContentListener.
   */
  canHandleContent: function canHandleContent(aContentType, aIsContentPreferred, aDesiredContentType) {
    return aContentType == TYPE_MAYBE_MEDIA;
  },

  /**
   * See nsIURIContentListener.
   */
  loadCookie: null,

  /**
   * See nsIURIContentListener.
   */
  get parentContentListener() {
    return null;
  },
  set parentContentListener(val) {
    // We don't support parent content listeners.
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  /**
   * See nsISupports.
   */
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIURIContentListener,
                                         Ci.nsISupportsWeakReference])
}

/**
 * XPCOM component registration.
 */
function postRegister(aCompMgr, aFileSpec, aLocation) {
  XPCOMUtils.categoryManager.addCategoryEntry(CATEGORY_CONTENT_LISTENER,
                                              TYPE_MAYBE_MEDIA, CONTRACTID,
                                              true, true);
}

function preUnregister(aCompMgr, aFileSpec, aLocation) {
  XPCOMUtils.categoryManager.deleteCategoryEntry(CATEGORY_CONTENT_LISTENER,
                                                 TYPE_MAYBE_MEDIA, true);
}

var NSGetModule = XPCOMUtils.generateNSGetModule([sbMediaContentListener],
                                                 postRegister, preUnregister);
