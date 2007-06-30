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
  
  function CancelAsyncWebDocument(href_loop)
  {
    if ( href_loop )
    {
      href_loop.cancel();
      
      // Murder his whole family.
      for ( var i = 0; i < href_loop.childLoops.length; i++ )
        CancelAsyncWebDocument( href_loop.childLoops[ i ] );

      // Make sure to clean up any medialists we have lying around.
      var oldMediaList = href_loop.mediaList;
      if (oldMediaList && !oldMediaList.length) {
        var library = oldMediaList.library;
        library.remove(oldMediaList);
      }

      href_loop = null;
    }
  }
  
  function installClickHandler(element) {
    // Make a closure for this.
    function handler(evt) {
      gBrowser.onMediaClick(evt);
    }
    // And attach it to the event listener.
    element.addEventListener("click", handler, true);
  }
  
  function AsyncWebDocument(aDocument, aMediaListView, old_href_loop, context)
  {
    const CONTRACTID_ARRAY = "@mozilla.org/array;1";
    const CONTRACTID_METADATAJOBMANAGER =
      "@songbirdnest.com/Songbird/MetadataJobManager;1";

    const nsIMutableArray = Components.interfaces.nsIMutableArray;
    const sbIMediaList = Components.interfaces.sbIMediaList;
    const sbIMetadataJobManager = Components.interfaces.sbIMetadataJobManager;
    
    CancelAsyncWebDocument(old_href_loop);

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
        
          // Check if clearInterval has been called (see sbIAsyncForLoop.js:66)
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
            // Add our event listeners to anything that's a link.
            this.a_array[ this.i ].addEventListener( "mouseover",
                function (evt) {gBrowser.onLinkOver(evt)}, true );
            this.a_array[ this.i ].addEventListener( "mouseout",
                function (evt) {gBrowser.onLinkOut(evt)}, true );
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
              var newloop = AsyncWebDocument( doc, this.mediaListView, null, context );
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
              var newloop = AsyncWebDocument( doc, this.mediaListView, null, context );
              newloop.child = true;
              this.childLoops.push( newloop );
            }
          }
          
          return loop_break;
        }
        catch ( err )        
        {
          alert( "async_webplaylist - aBodyEval\n\n" + err );
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
          // Let the view know that we're about to make a lot of changes.
          mediaList.beginUpdateBatch();

          // Only clear the list if we have anything to add.          
          if (mediaList.length) {
            // This will need to be fixed for multiple frames with media urls
            mediaList.clear(); 
          }
          
          // Use a try/finally construct to make sure that we call
          // endUpdateBatch no matter what happens.
          try {
            var mediaItemsToScan;
            for (var index = 0; index < foundItemsLength; index++) {
              var item = this.items[index];
              var mediaItem;

              if (item instanceof Components.interfaces.sbIMediaItem) {
                mediaItem = item;
              }
              else {
                // This must be just a URL.
                var uri = newURI(item);
                
                // Make a new media item for it
                mediaItem = library.createMediaItem(uri);
                mediaItem.setProperty(SBProperties.originPage, this.currentURL);
                
                // Make sure we scan it for metadata
                if (!mediaItemsToScan) {
                  mediaItemsToScan = Components.classes[CONTRACTID_ARRAY]
                                              .createInstance(nsIMutableArray);
                }
                mediaItemsToScan.appendElement(mediaItem, false);
              }
              mediaList.add(mediaItem);
            }
          } finally {
            // Tell the view that it can redraw itself.
            mediaList.endUpdateBatch();
          }

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
    
      // Make sure this is a well-formed url.
      try {
        url = newURI(url).spec;
      }
      catch (e) {
        return false;
      }
      
      if (gPPS.isPlaylistURL(url)) {
        installClickHandler(this.a_array[this.i]);

        // Keep the loop going.
        return false;
      }

      if (!force && !gPPS.isMediaURL(url)) {
        // Keep the loop going.
        return false;
      }

      if (context) {
        if (!this.items.length) {
          context.playlistHasItems = true;
          context.showPlaylist = true;
        }
      }
      
      // Don't insert it if we already did.
      if (this.seenURLs.indexOf(url) != -1) {
        return false;
      }

      // Try to see if we've already found and scanned this url
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

      var library = aMediaListView.mediaList.library;
      library.enumerateItemsByProperty(SBProperties.contentURL, url, listener,
                                       sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
      if (listener.foundItem) {
        this.items.push(listener.foundItem)
      }
      else {
        this.items.push(url);
      }

      this.seenURLs.push(url);
      
      // Only one synchronous database call per ui frame.
      return true;
    };
    
    href_loop.handleEmbedURL = function( url )
    {
      var retval = false;
      var location = "" + gBrowser.mCurrentBrowser.currentURI.spec;
      
      // TODO: Make this an API.
      
      //
      // YouTube videos embedded in other webpages
      //
      if ( url.indexOf( "www.youtube.com/v/" ) != -1 ) {
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
                    url = "http://www.youtube.com/get_video" + url.substr( key + seek.length );
                    this.manager.handleURL( url, true ); // Force the webplaylist to accept the url.
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
      else if ( location.indexOf( "youtube.com" ) != -1 ) {
        // Crack it for the get_video URL.
        var seek = "video_id";
        var key = url.indexOf( seek );
        if ( key != -1 ) {
          url = "http://www.youtube.com/get_video?" + url.substr( key );
          this.handleURL( url, true ); // Force the webplaylist to accept the url.
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
