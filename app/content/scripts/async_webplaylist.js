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
  Components.utils.import("rel:sbProperties.jsm");
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
        return (this.i < this.a_array.length) ||
               (this.i < this.embed_array.length) ||
               (this.i < this.object_array.length);
      },

      function webPlaylist_stepEval() {
        this.i++;
      },
      
      function webPlaylist_bodyEval() {
        try  {
          // Do not run while the "main" playlist is up?  This is so gross.
          //  UGH.  MUST REWRITE ENTIRE WORLD.
          /*
          if (gBrowser.playlistTree) {
            this.cancel(); 
            context.showPlaylist = false;
          } */

          // check is clearInterval has been called (see sbIAsyncForLoop.js:66)
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
              loop_break |= this.handleURL( url );
          }
          // "Object" tags
          if ( 
              ( this.i < this.object_array.length )
            )
          {
            // ?
            var url = this.object_array[ this.i ].getAttribute("src");
            if ( url )
              loop_break |= this.handleURL( url );
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

        if (mediaList.length) {
          mediaList.clear();
        }

        var foundItemsLength = this.items.length;
        if (foundItemsLength) {
          // Let the view know that we're about to make a lot of changes.
          mediaList.beginUpdateBatch();
          
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

            // XXXben HACK to make sure that the media items' properties are all
            //        written to the database before we redraw the view. The view
            //        makes a straight database call and bypasses the property
            //        cache. Remove this once bug 3037 is fixed.
            var propCache = library.
                            QueryInterface(Components.interfaces.sbILocalDatabaseLibrary).
                            propertyCache;
            propCache.write();
          }
          finally {
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
        else {
          // Nothing was found, so remove the media list we were using from
          // the library. DON'T touch aMediaListView after this call!
          library.remove(mediaList);
          delete mediaList;
          delete aMediaListView;
        }

        SBDataSetBoolValue( "media_scan.open", false ); // ?  Don't let this go?
        context.progressTotal = this.a_array.length;
        context.progressCurrent = this.a_array.length;

        // Release the global reference
        href_loop = null;
      },
      
      20, // 20 steps per interval
      
      0 // No pause per interval (each UI frame)
    );
    // End of class construction for sbIAsyncPlaylist
    
    // Attach a whole bunch of stuff to it so it can do its job in one pass.
    href_loop.a_array = aDocument.getElementsByTagName("A");
    href_loop.embed_array = aDocument.getElementsByTagName("EMBED");
    href_loop.object_array = aDocument.getElementsByTagName("OBJECT");
    href_loop.currentURL = aDocument.location;
    href_loop.items = [];
    href_loop.seenURLs = [];
    href_loop.mediaList = aMediaListView.mediaList;
    
    // Useful function to be called by the internal code
    href_loop.handleURL = function webPlaylist_handleURL(url) {
    
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

      if (!gPPS.isMediaURL(url)) {
        // decrement the total (floor is 0) to keep the percentage indicator moving
        if (context.progressTotal > 0) {
          context.progressTotal = context.progressTotal - 1;
        }

        // Keep the loop going.
        return false;
      }

      // Tell other folks that we're on to the next item.
      context.progressCurrent = this.i + 1;

      installClickHandler(this.a_array[this.i]);

      if (!this.items.length) {
        context.playlistHasItems = true;
        context.showPlaylist = true;
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
      library.enumerateItemsByProperty(SBProperties.contentUrl, url, listener,
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
    }
    
    context.progressTotal = href_loop.a_array.length;
    SBDataSetBoolValue( "media_scan.open", true ); // ?  Don't let this go?

    return href_loop;
  }  
}
catch ( err )
{
  alert( "sbIAsyncForLoop - load\r\n" + err );
}
