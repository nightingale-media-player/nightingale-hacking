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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

// From nsNetError.h
const NS_BINDING_ABORTED = 0x804b0002;

const DESCRIPTION = "sbMediaContentListener";
const CID         = "2803c9e8-b0b6-4dfe-8333-53430128f7e7";
const CONTRACTID  = "@songbirdnest.com/contentlistener/media;1";

const CONTRACTID_ARRAY              = "@songbirdnest.com/moz/xpcom/threadsafe-array;1";
const CONTRACTID_LIBRARYMANAGER     = "@songbirdnest.com/Songbird/library/Manager;1";
const CONTRACTID_OBSERVERSERVICE    = "@mozilla.org/observer-service;1";
const CONTRACTID_PREFSERVICE        = "@mozilla.org/preferences-service;1";

const CATEGORY_CONTENT_LISTENER = "external-uricontentlisteners";

const PREF_WEBLIBRARY_GUID = "songbird.library.web";

const TYPE_MAYBE_MEDIA = "application/vnd.songbird.maybe.media";
const TYPE_MAYBE_PLAYLIST = "application/vnd.songbird.maybe.playlist";

// For XPCOM boilerplate.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

// For Songbird properties.
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

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
  this._typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                        .createInstance(Ci.sbIMediacoreTypeSniffer);
  this._mm = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
               .getService(Ci.sbIMediacoreManager);
}
sbMediaContentListener.prototype = {
  _parentContentListener: null,
  
  /**
   * Needed by XPCOMUtils.
   */
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,
  _typeSniffer:     null,
  _mm:              null,

  /**
   * Takes care of adding a url to the library and playing it.
   */
  _handleMediaURI: function _handleMediaURI(aURI) {
    // TODO: Check a pref to determine if this should be going into our library?
    var libraryManager = Cc[CONTRACTID_LIBRARYMANAGER].
                         getService(Ci.sbILibraryManager);
    var library;
    var uri = aURI;
    
    if (aURI instanceof Ci.nsIFileURL) {
      // Local files go into the main library.
      library = libraryManager.mainLibrary;
      libraryManager.getContentURI(aURI).spec;
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
      },
      onEnumeratedItem: function onEnumeratedItem(list, item) {
        this.foundItem = item;
        return Ci.sbIMediaListEnumerationListener.CANCEL;
      },
      onEnumerationEnd: function onEnumerationEnd() {
      }
    };

    var url = uri.spec;
    library.enumerateItemsByProperty(SBProperties.contentURL, url, listener );
    if (!listener.foundItem) {
      var mediaItem = library.createMediaItem(aURI);

      // Get metadata going.
      var scanArray = Cc[CONTRACTID_ARRAY].createInstance(Ci.nsIMutableArray);
      scanArray.appendElement(mediaItem, false);
      
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
      var job = metadataService.read(scanArray);
    }

    var view = library.createView();
    var filter = LibraryUtils.createConstraint([
      [
        [SBProperties.contentURL, [url]]
      ]
    ]);
    view.filterConstraint = filter;

    this._mm.sequencer.playView(view, 0);
  },

  _handlePlaylistURI: function _handlePlaylistURI(aURI) {
    var app = Cc["@songbirdnest.com/Songbird/ApplicationController;1"]
                .getService(Ci.sbIApplicationController);
    var window = app.activeMainWindow;
    if (window) {
      var tabbrowser = window.document.getElementById("content");
      tabbrowser.handleMediaURL(aURI.spec, true, true, null, null);
    }
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
    
    var contentType = channel.contentType;
    
    dump("\n---------------------------\nsbMediaContentListener -- contentType: " + contentType + "\n---------------------------\n");
    
    // We seem to think this is a media file, let's make sure it doesn't have a
    // content type that we know for sure isn't media.
    if(contentType == "text/html" ||
       contentType == "application/atom+xml" ||
       contentType == "application/rdf+xml" ||
       contentType == "application/rss+xml" ||
       contentType == "application/xml") {
        
      // Bah, looks like the content type doesn't match up at all,
      // just find someone else to load it.
      return false;
    }

    // Let exceptions propogate from here!
    if (aContentType == TYPE_MAYBE_MEDIA) {
      if (!this._typeSniffer.isValidMediaURL(uri)) {
        // Hmm, badness. We can't actually play this file type. Throw an error
        // here to get the URILoader to keep trying with other content listeners.
        throw Cr.NS_ERROR_UNEXPECTED;
      }

      try {
        this._handleMediaURI(uri);
      }
      catch (e) {
        Components.utils.reportError(e);
        throw e;
      }
    }
    else if (aContentType == TYPE_MAYBE_PLAYLIST) {
      if (!this._typeSniffer.isValidWebSafePlaylistURL(uri)) {
        // We thought it looked like a playlist but it in the end 
        // it wasn't!
        throw Cr.NS_ERROR_UNEXPECTED;
      }
      
      try {
        this._handlePlaylistURI(uri);
      }
      catch (e) {
        Components.utils.reportError(e);
        throw e;
      }
    }

    // The request is active, so make sure to cancel it.
    aRequest.cancel(NS_BINDING_ABORTED);

    return true;
  },

  /**
   * See nsIURIContentListener.
   */
  isPreferred: function isPreferred(aContentType, aDesiredContentType) {
    return aContentType == TYPE_MAYBE_MEDIA ||
      aContentType == TYPE_MAYBE_PLAYLIST;
  },

  /**
   * See nsIURIContentListener.
   */
  canHandleContent: function canHandleContent(aContentType, aIsContentPreferred, aDesiredContentType) {
    return aContentType == TYPE_MAYBE_MEDIA ||
      aContentType == TYPE_MAYBE_PLAYLIST;
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
  XPCOMUtils.categoryManager.addCategoryEntry(CATEGORY_CONTENT_LISTENER,
                                              TYPE_MAYBE_PLAYLIST, CONTRACTID,
                                              true, true);
}

function preUnregister(aCompMgr, aFileSpec, aLocation) {
  XPCOMUtils.categoryManager.deleteCategoryEntry(CATEGORY_CONTENT_LISTENER,
                                                 TYPE_MAYBE_MEDIA, true);
  XPCOMUtils.categoryManager.deleteCategoryEntry(CATEGORY_CONTENT_LISTENER,
                                                 TYPE_MAYBE_PLAYLIST, true);
}

var NSGetModule = XPCOMUtils.generateNSGetModule([sbMediaContentListener],
                                                 postRegister, preUnregister);
