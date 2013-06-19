/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * \file  sbFileDownloader.js
 * \brief Javascript source for the file downloader component.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// File downloader component.
//
//   This component provides support for downloading files.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// File downloader imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

// Songbird imports.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// File downloader configuration.
//
//------------------------------------------------------------------------------

//
// classDescription             Description of component class.
// classID                      Component class ID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// _xpcom_categories            List of component categories.
//

var sbFileDownloaderCfg= {
  classDescription: "File Downloader",
  classID: Components.ID("{f93a67e4-d97d-4607-971e-00ae2bf8ef8f}"),
  contractID: "@songbirdnest.com/Songbird/FileDownloader;1",
  ifList: [ Ci.sbIFileDownloader, Ci.nsIWebProgressListener ],
  _xpcom_categories: []
};


//------------------------------------------------------------------------------
//
// File downloader object.
//
//------------------------------------------------------------------------------

/**
 * Construct a file downloader object.
 */

function sbFileDownloader() {
}

// Define the object.
sbFileDownloader.prototype = {
  // Set the constructor.
  constructor: sbFileDownloader,

  //
  // File downloader component configuration fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //

  classDescription: sbFileDownloaderCfg.classDescription,
  classID: sbFileDownloaderCfg.classID,
  contractID: sbFileDownloaderCfg.contractID,
  _xpcom_categories: sbFileDownloaderCfg._xpcom_categories,


  //
  // File downloader fields.
  //
  //   _cfg                     Configuration settings.
  //   _webBrowserPersist       Web browser persist object.
  //

  _cfg: sbFileDownloaderCfg,
  _webBrowserPersist: null,


  //----------------------------------------------------------------------------
  //
  // File downloader sbIFileDownloader services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Number of bytes in file being downloaded.
   */

  bytesToDownload: 0,


  /**
   * \brief Number of bytes in file that have been downloaded.
   */

  bytesDownloaded: 0,


  /**
   * \brief Percentage (0-100) of bytes of file that have been downloaded.
   */

  percentComplete: 0,


  /**
   * \brief True if file download has completed, whether successful or not.
   */

  complete: false,


  /**
   * \brief True if file downloaded successfully.  Will be false if download is
   *        cancelled.
   */

  succeeded: false,


  /**
   * \brief Listener for download events.
   */

  listener: null,


  /**
   * \brief URI of source of file.
   */

  sourceURI: null,


  /**
   * \brief URI spec of source of file.
   */

  sourceURISpec: null,


  /**
   * \brief Destination file.  If not set when download is started, a temporary
   *        file will be created and set in destinationFile.
   */

  destinationFile: null,


  /**
   * \brief Destination file extension.  If a temporary file is created, set its
   *        file extension to destinationFileExtension.
   */

  destinationFileExtension: null,


  /**
   * \brief Temporary file factory to use for any temporary files.
   */

  temporaryFileFactory: null,


  /**
   * \brief Start file download from source URI to destination file.  If source
   *        URI is not specified, use source URI spec.  If destination file is
   *        not specified, create a temporary one.
   */

  start: function sbFileDownloader_start() {
    // Create a web browser persist object.
    this._webBrowserPersist =
           Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
             .createInstance(Ci.nsIWebBrowserPersist);
    this._webBrowserPersist.progressListener = this;
    this._webBrowserPersist.persistFlags =
           Ci.nsIWebBrowserPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
           Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NO_CONVERSION |
           Ci.nsIWebBrowserPersist.PERSIST_FLAGS_BYPASS_CACHE;

    // If source URI is not provided, use source URI spec.
    if (!this.sourceURI && this.sourceURISpec) {
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
      this.sourceURI = ioService.newURI(this.sourceURISpec, null, null);
    }

    // Throw an exception if no source is specified.
    if (!this.sourceURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    // If destination file is not provided, create a temporary one.
    //XXXeps if destination file is provided, should create a temporary one and
    //XXXeps move to destination when complete.
    if (!this.destinationFile) {
      if (this.temporaryFileFactory) {
        this.destinationFile =
          this.temporaryFileFactory.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                               null,
                                               this.destinationFileExtension);
      }
      else {
        var temporaryFileService =
              Cc["@songbirdnest.com/Songbird/TemporaryFileService;1"]
                .getService(Ci.sbITemporaryFileService);
        this.destinationFile =
               temporaryFileService.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                               null,
                                               this.destinationFileExtension);
      }
    }

    // Start the file download.
    this._webBrowserPersist.saveURI(this.sourceURI,
                                    null,
                                    null,
                                    null,
                                    null,
                                    this.destinationFile);
  },


  /**
   * \brief Cancel file download.
   */

  cancel: function sbFileDownloader_cancel() {
    // Cancel the file download.
    this._webBrowserPersist.cancelSave();
    this.request = null;
  },

  /**
   * \brief The request used during the transfer.
   */
  request: null,


  //----------------------------------------------------------------------------
  //
  // File downloader nsIWebProgressListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Notification indicating the state has changed for one of the requests
   * associated with aWebProgress.
   *
   * @param aWebProgress
   *        The nsIWebProgress instance that fired the notification
   * @param aRequest
   *        The nsIRequest that has changed state.
   * @param aStateFlags
   *        Flags indicating the new state.  This value is a combination of one
   *        of the State Transition Flags and one or more of the State Type
   *        Flags defined above.  Any undefined bits are reserved for future
   *        use.
   * @param aStatus
   *        Error status code associated with the state change.  This parameter
   *        should be ignored unless aStateFlags includes the STATE_STOP bit.
   *        The status code indicates success or failure of the request
   *        associated with the state change.  NOTE: aStatus may be a success
   *        code even for server generated errors, such as the HTTP 404 error.
   *        In such cases, the request itself should be queried for extended
   *        error information (e.g., for HTTP requests see nsIHttpChannel).
   */

  onStateChange: function sbFileDownloader_onStateChange(aWebProgress,
                                                         aRequest,
                                                         aStateFlags,
                                                         aStatus) {
    if (!this.request) {
      this.request = aRequest;
    }

    // Check for completion.
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      // Mark completion.
      this.complete = true;

      // Check for HTTP channel failure.
      var httpChannelFailed = false;
      try {
        var httpChannel = aRequest.QueryInterface(Ci.nsIHttpChannel);
        httpChannelFailed = (httpChannel.requestSucceeded ? false : true);
      }
      catch (ex) {}

      // Mark success if aStatus is NS_OK and the HTTP channel did not fail.
      if ((aStatus == Cr.NS_OK) && !httpChannelFailed)
        this.succeeded = true;

      // The destination file may have been overwritten.  This can cause the
      // destination file object to be invalid.  Clone it to get a usable
      // object.
      if (this.destinationFile)
        this.destinationFile = this.destinationFile.clone();

      // Delete destination file on failure.
      if (!this.succeeded) {
        this.destinationFile.remove(false);
        this.destinationFile = null;
      }

      // Remove web browser persist listener reference to avoid a cycle.
      this._webBrowserPersist.progressListener = null;

      // Call file download listener.
      if (this.listener)
        this.listener.onComplete();
    }
  },


  /**
   * Notification that the progress has changed for one of the requests
   * associated with aWebProgress.  Progress totals are reset to zero when all
   * requests in aWebProgress complete (corresponding to onStateChange being
   * called with aStateFlags including the STATE_STOP and STATE_IS_WINDOW
   * flags).
   *
   * @param aWebProgress
   *        The nsIWebProgress instance that fired the notification.
   * @param aRequest
   *        The nsIRequest that has new progress.
   * @param aCurSelfProgress
   *        The current progress for aRequest.
   * @param aMaxSelfProgress
   *        The maximum progress for aRequest.
   * @param aCurTotalProgress
   *        The current progress for all requests associated with aWebProgress.
   * @param aMaxTotalProgress
   *        The total progress for all requests associated with aWebProgress.
   *
   * NOTE: If any progress value is unknown, or if its value would exceed the
   * maximum value of type long, then its value is replaced with -1.
   *
   * NOTE: If the object also implements nsIWebProgressListener2 and the caller
   * knows about that interface, this function will not be called. Instead,
   * nsIWebProgressListener2::onProgressChange64 will be called.
   */

  onProgressChange:
    function sbFileDownloader_onProgressChange(aWebProgress,
                                               aRequest,
                                               aCurSelfProgress,
                                               aMaxSelfProgress,
                                               aCurTotalProgress,
                                               aMaxTotalProgress) {
    // Update the file download statistics.
    this.bytesToDownload = aMaxSelfProgress;
    this.bytesDownloaded = aCurSelfProgress;
    this.percentComplete =
           Math.floor((this.bytesDownloaded / this.bytesToDownload) * 100);

    // Call file download listener.
    if (this.listener)
      this.listener.onProgress();
  },


  /**
   * Called when the location of the window being watched changes.  This is not
   * when a load is requested, but rather once it is verified that the load is
   * going to occur in the given window.  For instance, a load that starts in a
   * window might send progress and status messages for the new site, but it
   * will not send the onLocationChange until we are sure that we are loading
   * this new page here.
   *
   * @param aWebProgress
   *        The nsIWebProgress instance that fired the notification.
   * @param aRequest
   *        The associated nsIRequest.  This may be null in some cases.
   * @param aLocation
   *        The URI of the location that is being loaded.
   */

  onLocationChange: function sbFileDownloader_onLocationChange(aWebProgress,
                                                               aRequest,
                                                               aLocation) {
  },


  /**
   * Notification that the status of a request has changed.  The status message
   * is intended to be displayed to the user (e.g., in the status bar of the
   * browser).
   *
   * @param aWebProgress
   *        The nsIWebProgress instance that fired the notification.
   * @param aRequest
   *        The nsIRequest that has new status.
   * @param aStatus
   *        This value is not an error code.  Instead, it is a numeric value
   *        that indicates the current status of the request.  This interface
   *        does not define the set of possible status codes.  NOTE: Some
   *        status values are defined by nsITransport and nsISocketTransport.
   * @param aMessage
   *        Localized text corresponding to aStatus.
   */

  onStatusChange: function sbFileDownloader_onStatusChange(aWebProgress,
                                                           aRequest,
                                                           aStatus,
                                                           aMessage) {
  },


  /**
   * Notification called for security progress.  This method will be called on
   * security transitions (eg HTTP -> HTTPS, HTTPS -> HTTP, FOO -> HTTPS) and
   * after document load completion.  It might also be called if an error
   * occurs during network loading.
   *
   * @param aWebProgress
   *        The nsIWebProgress instance that fired the notification.
   * @param aRequest
   *        The nsIRequest that has new security state.
   * @param aState
   *        A value composed of the Security State Flags and the Security
   *        Strength Flags listed above.  Any undefined bits are reserved for
   *        future use.
   *
   * NOTE: These notifications will only occur if a security package is
   * installed.
   */

  onSecurityChange: function sbFileDownloader_onSecurityChange(aWebProgress,
                                                               aRequest,
                                                               aState) {
  },


  //----------------------------------------------------------------------------
  //
  // File downloader nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbFileDownloaderCfg.ifList)
};


//------------------------------------------------------------------------------
//
// File downloader component services.
//
//------------------------------------------------------------------------------

var NSGetModule = XPCOMUtils.generateNSGetFactory([sbFileDownloader]);
