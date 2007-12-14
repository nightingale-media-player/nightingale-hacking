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

/*
 * From sbDownloadPropertyInfo.h
 * These should be made available from JS
*/
const SBDOWNLOADMODE_ENONE            = 0;
const SBDOWNLOADMODE_ENEW             = 1;
const SBDOWNLOADMODE_ESTARTING        = 2;
const SBDOWNLOADMODE_EDOWNLOADING     = 3;
const SBDOWNLOADMODE_EPAUSED          = 4;
const SBDOWNLOADMODE_ECOMPLETE        = 5;
const SBDOWNLOADMODE_EFAILED          = 6;

//
// Web Playlist loader using sbIAsyncForLoop
//

if (typeof(SBProperties) == "undefined") {
  Components.utils.import("resource://app/components/sbProperties.jsm");
  if (!SBProperties)
    throw new Error("Import of sbProperties module failed!");
}

try
{
  // Parse through the document to get all the urls.

  function CancelAsyncWebDocument(href_loop, newMediaListView)
  {
    if ( href_loop )
    {
      href_loop.cancel();

      // Murder his whole family.
      for ( var i = 0; i < href_loop.childLoops.length; i++ )
        CancelAsyncWebDocument( href_loop.childLoops[ i ], newMediaListView );

      // Make sure to clean up any medialists we have lying around as long as
      // they're not the same as the new media list.
      var oldMediaList = href_loop.mediaList;
      var keepOldMediaList = false;
      if (newMediaListView) {
        var newMediaList = newMediaListView.mediaList;
        if (newMediaList && oldMediaList.equals(newMediaList))
          keepOldMediaList = true;
      }
      if (!keepOldMediaList && oldMediaList && !oldMediaList.length) {
        var library = oldMediaList.library;
        library.remove(oldMediaList);
      }

      href_loop = null;
    }
  }

  function AsyncWebDocument(aDocument, aMediaListView, old_href_loop, context, aTitle)
  {
    const CONTRACTID_ARRAY = "@mozilla.org/array;1";
    const CONTRACTID_METADATAJOBMANAGER =
      "@songbirdnest.com/Songbird/MetadataJobManager;1";
    const CONTRACTID_FAVICONSERVICE = "@mozilla.org/browser/favicon-service;1";
    
    const nsIFaviconService = Components.interfaces.nsIFaviconService;
    const nsIMutableArray = Components.interfaces.nsIMutableArray;
    const sbIMediaList = Components.interfaces.sbIMediaList;
    const sbIMetadataJobManager = Components.interfaces.sbIMetadataJobManager;

    CancelAsyncWebDocument(old_href_loop, aMediaListView);

    var href_loop = new sbIAsyncForLoop
    ( // This is an argument list passed to the sbIAsyncForLoop constructor.

      function webPlaylist_initEval() {
        this.i = 0;
      },

      function webPlaylist_whileEval() {
        var retval = (this.i < this.a_array.length) ||
                     (this.i < this.embed_array.length) ||
                     (this.i < this.object_array.length) ||
                     (this.i < this.frame_array.length) ||
                     (this.i < this.iframe_array.length);

        for ( var i = 0; i < this.childLoops.length; i++ ) {
          retval |= this.childLoops[ i ].m_Interval != null;
        }
        return retval;
      },

      function webPlaylist_stepEval() {
        this.i++;
      },

      function webPlaylist_bodyEval() {
        try  {

          // Check if clearInterval has been called (see asyncForLoop.js:66)
          if (!this.m_Interval) {
            return true;
          }

          var loop_break = false;

          // "A" tags
          if (
              ( this.i < this.a_array.length ) &&
              ( this.a_array[ this.i ].href ) &&
              ( this.a_array[ this.i ].addEventListener )
            )
          {
            var url = this.a_array[ this.i ].href;
            if ( url )
              loop_break = this.handleURL( url );
          }
          // "Embed" tags
          if (
              ( this.i < this.embed_array.length )
            )
          {
            var url = this.embed_array[ this.i ].getAttribute("src");
            if ( url )
              loop_break |= this.handleEmbedURL( url );
          }
          // "Object" tags
          if (
              ( this.i < this.object_array.length )
            )
          {
            var url = this.object_array[ this.i ].getAttribute("data");
            if ( url )
              loop_break |= this.handleEmbedURL( url );
          }
          // "Frame" tags
          if (
              ( this.i < this.frame_array.length )
            )
          {
            var doc = this.frame_array[ this.i ].contentDocument;
            if ( doc ) {
              var newloop = AsyncWebDocument( doc, this.mediaListView, null, context, aTitle );
              newloop.child = true;
              this.childLoops.push( newloop );
            }
          }
          // "IFrame" tags
          if (
              ( this.i < this.iframe_array.length )
            )
          {
            var doc = this.iframe_array[ this.i ].contentDocument;
            if ( doc ) {
              var newloop = AsyncWebDocument( doc, this.mediaListView, null, context, aTitle );
              newloop.child = true;
              this.childLoops.push( newloop );
            }
          }

          return loop_break;
        }
        catch ( err )
        {
          Components.utils.reportError(err);
        }
        return 0;
      },

      function webPlaylist_finishedEval() {
        if (!this.m_Interval)
          return;

        var mediaList = aMediaListView.mediaList;
        var library = mediaList.library;

        var foundItemsLength = this.items.length;
        if (foundItemsLength) {
          // Show the playlist now that we're going to populate it.
          if (context) {
              context.playlistHasItems = true;
              context.showPlaylist = true;
          }

          var self = this;
          var mediaItemsToScan;

          var batchFunction = function() {
            // Only clear the list if we have anything to add.
            if (mediaList.length) {
              // This will need to be fixed for multiple frames with media urls
              mediaList.clear();
            }
            for (var index = 0; index < foundItemsLength; index++) {
              var item = self.items[index];
              var mediaItem;

              if (item instanceof Components.interfaces.sbIMediaItem) {
                mediaItem = item;
              }
              else {
                // This must be just an URL.
                var url = item;
                var name = "";

                // Or, uhm, maybe it's an Object?
                if (item instanceof Object) {
                  url = item.url;
                  name = item.name;
                }

                var uri = newURI(url);

                // Set the originURL/Page values to remember our state.
                var propArray = [
                    [SBProperties.originPage, self.currentURL],
                    [SBProperties.originPageImage, self.currentURL],
                    [SBProperties.originURL, url],
                    [SBProperties.originPageTitle, self.currentTitle],
                    [SBProperties.enableAutoDownload, "1"],
                    [SBProperties.downloadButton, "1|0|0"]
                  ];
                // Add the track name if requested.
                if (name.length > 0) {
                  propArray.push([SBProperties.trackName, name])
                }

                // Make a new media item for it
                mediaItem = library.createMediaItem(uri,
                                // Set the properties for later tracking
                                SBProperties.createArray( propArray ) );

                // Make sure we scan it for metadata
                if (!mediaItemsToScan) {
                  mediaItemsToScan = Components.classes[CONTRACTID_ARRAY]
                                              .createInstance(nsIMutableArray);
                }
                mediaItemsToScan.appendElement(mediaItem, false);
              }
              
              var mainLibraryHasItem = false;
              var downloadStatus = 0;

              try {
                // Get the current status of this item
                downloadStatus = mediaItem.getProperty(SBProperties.downloadButton);
                downloadStatus = downloadStatus.split("|");
                downloadStatus = downloadStatus[0];

                // See if the item is in the main library
                var originURL;
                
                originURL = mediaItem.getProperty(SBProperties.originURL);

                var libraryManager =
                    Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                     .getService(Ci.sbILibraryManager);
                var mainLibrary = libraryManager.mainLibrary;

                var listener = {
                  items: [],
                  onEnumerationBegin: function() {
                    return true;
                  },
                  onEnumeratedItem: function(list, item) {
                    this.items.push(item);
                    return true;
                  },
                  onEnumerationEnd: function() {
                    return true;
                  }
                };

                mainLibrary.enumerateItemsByProperty(
                                    SBProperties.originURL,
                                    originURL,
                                    listener);
                if (listener.items.length > 0) {
                  mainLibraryHasItem = true;
                }
              } catch (err) { }
            
              // If the item is not currently downloading and does not
              // appear in the main library then reset the download button
              if ( ( (downloadStatus > SBDOWNLOADMODE_EPAUSED) ||
                   (downloadStatus < SBDOWNLOADMODE_ESTARTING) ) &&
                   (!mainLibraryHasItem) ) {
                // Not in library so reset download button
                mediaItem.setProperty(SBProperties.downloadButton, "1|0|0");
                mediaItem.setProperty(SBProperties.downloadDetails, "");
              } else if (mainLibraryHasItem) {
                // But if it does show up in the main library.
                if (downloadStatus == SBDOWNLOADMODE_EFAILED) {
                    mediaItem.setProperty(SBProperties.downloadButton,
                                          SBDOWNLOADMODE_EFAILED + "|0|0");
                    mediaItem.setProperty(SBProperties.downloadDetails,
                                          SBString("device.download.error",
                                                   "Failed"));
                } else {
                    mediaItem.setProperty(SBProperties.downloadButton,
                                          SBDOWNLOADMODE_ECOMPLETE + "|1|1");
                    mediaItem.setProperty(SBProperties.downloadDetails,
                                          SBString("device.download.complete",
                                                   "Complete"));
                }
              }
              
              mediaList.add(mediaItem);
            }
          };

          mediaList.runInBatchMode(batchFunction);

          // Create a metadata task if there's anything to scan.
          if (mediaItemsToScan && mediaItemsToScan.length) {
            // Then submit the job
            var metadataJobManager =
              Components.classes[CONTRACTID_METADATAJOBMANAGER]
                        .getService(sbIMetadataJobManager);
            var metadataJob = metadataJobManager.newJob(mediaItemsToScan, 5);
          }
        }
        else if ( this.child == false ) {
          // Only if we are a parent job, see if anyone found anything
          if ( ! this.checkForAnything( this ) ) {
            // Nothing was found, so remove the media list we were using from
            // the library. DON'T touch aMediaListView after this call!
            library.remove(mediaList);
            delete mediaList;
            delete aMediaListView;
          }
        }
        if ( this.child == false ) {
          // Parent job can stop counting progress.
          context.progressTotal = context.progressCurrent = 1;
        }
      },

      20, // 20 steps per interval

      0 // No pause per interval (each UI frame)
    );
    // End of class construction for sbIAsyncPlaylist

    // Attach a whole bunch of stuff to it so it can do its job in one pass.
    href_loop.a_array = aDocument.getElementsByTagName("A");
    href_loop.embed_array = aDocument.getElementsByTagName("EMBED");
    href_loop.object_array = aDocument.getElementsByTagName("OBJECT");
    href_loop.frame_array = aDocument.getElementsByTagName("FRAME");
    href_loop.iframe_array = aDocument.getElementsByTagName("IFRAME");
    href_loop.currentURL = aDocument.location;
    href_loop.currentTitle = aTitle;
    href_loop.items = [];
    href_loop.seenURLs = [];
    href_loop.childLoops = [];
    href_loop.mediaListView = aMediaListView;
    href_loop.mediaList = aMediaListView.mediaList;
    href_loop.child = false; // might get set to true, later.

    href_loop.checkForAnything = function webPlaylist_checkForAnything() {
      // Do I have anything?
      var retval = this.items.length > 0;
      // Do my children have anything?
      for ( var i = 0; i < this.childLoops.length; i++ ) {
        retval |= this.childLoops[ i ].checkForAnything();
      }
      return retval;
    }

    // Useful function to be called by the internal code
    href_loop.handleURL = function webPlaylist_handleURL(url, force) {

      // Tell other folks that we're on to the next item.
      if (context)
        context.progressCurrent = context.progressCurrent + 1;

      if (!force) {
        // Make sure this is a well-formed url.
        try {
          url = newURI(url).spec;
        }
        catch (e) {
          return false;
        }

        if (gPPS.isPlaylistURL(url)) {
          // Keep the loop going.
          return false;
        }

        if (!gPPS.isMediaURL(url)) {
          // Keep the loop going.
          return false;
        }
      }

      // Shortcut
      if (force) {
        this.items.push(url);
        this.seenURLs.push(url);
        return true;
      }

      // Don't insert it if we already did.
      if (this.seenURLs.indexOf(url) != -1) {
        return false;
      }
      
      // Since the verification is asynchronous, we have to add this URL to the list
      // of seen URLs before verification begins. New items may start getting 
      // processed without the first item finishes.
      this.seenURLs.push(url);

      var observer = {
        url : "",
        manager : null,
        onStartRequest : function(aRequest,aContext) {
        },
        onStopRequest : function(aRequest, aContext, aStatusCode) {
          this.manager.popAsync();
          try {
            if (aStatusCode == 0) {
              // Get the redirected URL.
              var uriChecker =
                aRequest.QueryInterface(Components.interfaces.nsIURIChecker);
              if ( uriChecker ) {
                var url = uriChecker.baseChannel.URI.spec;
                var contentType = uriChecker.baseChannel.contentType;

                // XXXAus: We know for sure these follow contentTypes are not media we can playback.
                // See bug #4844 for more information.
                if(contentType == "text/html" ||
                   contentType == "application/atom+xml" ||
                   contentType == "application/rdf+xml" ||
                   contentType == "application/rss+xml" ||
                   contentType == "application/xml") {
                   return;
                 }

                // Try to see if we've already found and scanned this url
                var listener = {
                  foundItem: null,
                  onEnumerationBegin: function onEnumerationBegin() {
                    return this.foundItem == null;
                  },
                  onEnumeratedItem: function onEnumeratedItem(list, item) {
                    this.foundItem = item;
                    return false; // Just take the first item found
                  },
                  onEnumerationEnd: function onEnumerationEnd() {
                    return;
                  }
                };

                var library = aMediaListView.mediaList.library;
                library.enumerateItemsByProperty(SBProperties.originURL, url, listener );
                library.enumerateItemsByProperty(SBProperties.contentURL, url, listener );
                
                if (listener.foundItem) {
                  this.manager.items.push(listener.foundItem);
                }
                else {
                  this.manager.items.push(url);
                }
              }
            }
          } catch( e ) {
            alert( "AsyncWebplaylist default urlobserver\n\n" + e );
          }
        }
      };
      
      observer.manager = this;
      observer.url = url;
      
      var checker = Components.classes["@mozilla.org/network/urichecker;1"].createInstance(Components.interfaces.nsIURIChecker);
      var uri = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURI);
      
      uri.spec = url;
      checker.init(uri);
      checker.asyncCheck(observer, null);
      
      this.pushAsync();
      
      return true;
    };

    href_loop.handleEmbedURL = function( url )
    {
      var retval = false;
      var location = "" + gBrowser.mCurrentBrowser.currentURI.spec;

      // TODO: Make this an API.

      // set the pref 'songbird.scraper.disable.youtube' as a string to '1'
      // to block and '0' to allow.
      var disableYouTube = SBDataGetBoolValue("scraper.disable.youtube");

      //
      // YouTube videos embedded in other webpages
      //
      if ( !disableYouTube && url.indexOf( "www.youtube.com/v/" ) != -1 ) {
        // This has to traverse a redirect, asynchronously.
        var observer = {
          url : "",
          manager : null,
          onStartRequest : function(aRequest,aContext) {
          },
          onStopRequest : function(aRequest, aContext, aStatusCode) {
            this.manager.popAsync();
            try {
              if (aStatusCode == 0) {
                // Get the redirected URL.
                var uriChecker =
                  aRequest.QueryInterface(Components.interfaces.nsIURIChecker);
                if ( uriChecker ) {
                  var url = uriChecker.baseChannel.URI.spec;

                  // Crack it for the get_video URL.
                  var seek = "jp.swf";
                  var key = url.indexOf( seek );
                  if ( key != -1 ) {
                    var obj = {};
                    obj.url = "http://www.youtube.com/get_video" + url.substr( key + seek.length );
                    obj.name = "*** YouTube Video ***";
                    this.manager.handleURL( obj, true ); // Force the webplaylist to accept the obj.
                  }
                }
              }
            } catch( e ) {
              alert( "AsyncWebplaylist youtube urlobserver\n\n" + e );
            }
          }
        };
        observer.manager = this;
        observer.url = url;
        var checker = Components.classes["@mozilla.org/network/urichecker;1"].createInstance(Components.interfaces.nsIURIChecker);
        var uri = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURI);
        uri.spec = url;
        checker.init(uri);
        checker.asyncCheck(observer, null);
        this.pushAsync();
        retval = true;
      }
      //
      // YouTube videos on youtube.com
      //
      else if ( !disableYouTube && location.indexOf( "youtube.com" ) != -1 ) {
        // Crack it for the get_video URL.
        var seek = "video_id";
        var key = url.indexOf( seek );
        if ( key != -1 ) {
          var obj = {};
          obj.url = "http://www.youtube.com/get_video?" + url.substr( key );
          obj.name = "*** YouTube Video ***";
          this.handleURL( obj, true ); // Force the webplaylist to accept the obj.
          retval = true;
        }
      }
      //
      // This is the boring default functionality.
      //
      else {
        retval = this.handleURL( url );
      }
      return retval;
    }

    if (context) {
      context.progressTotal += href_loop.a_array.length +
                               href_loop.embed_array.length +
                               href_loop.object_array.length;
    }
    return href_loop;
  }
}
catch ( err )
{
  alert( "sbIAsyncForLoop - load\r\n" + err );
}
