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

try
{
  // Parse through the document to get all the urls.
  
  var href_loop = null;
  
  function CancelAsyncWebDocument()
  {
    if ( href_loop )
    {
      href_loop.cancel();
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
  
  function AsyncWebDocument(aDocument, aMediaListView)
  {
    const PROPERTYKEY_ORIGINURL = "http://songbirdnest.com/data/1.0#originUrl";
    const sbIMediaList = Components.interfaces.sbIMediaList;
    
    function EnumerationListener() {
      this.items = [];
    }
    EnumerationListener.prototype = {
      items: null,
      onEnumerationBegin: function onEnumerationBegin() {
        return true;
      },
      onEnumeratedItem: function onEnumeratedItem(list, item) {
        this.items.push(item);
        return true;
      },
      onEnumerationEnd: function onEnumerationEnd() {
        return true;
      }
    }
    
    CancelAsyncWebDocument();
    
    href_loop = new sbIAsyncForLoop
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
          if (gBrowser.playlistTree) {
            this.cancel(); 
            gBrowser.showWebPlaylist = false;
          } 

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
            this.a_array[ this.i ].addEventListener( "contextmenu",
                function (evt) {gBrowser.onLinkContext(evt)}, true );
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
        if ( !this.m_Interval ) 
          return;

        var propCache = aMediaListView.mediaList.library.
                        QueryInterface(Components.interfaces.sbILocalDatabaseLibrary).
                        propertyCache;
        propCache.write();

        aMediaListView.mediaList.endUpdateBatch();

        var propArray =
          Components.classes["@songbirdnest.com/Songbird/Properties/PropertyArray;1"]
                    .createInstance(Components.interfaces.sbIPropertyArray);

        propArray.appendProperty(PROPERTYKEY_ORIGINURL, gBrowser.currentURI.spec);
        aMediaListView.setFilters(propArray);

        SBDataSetBoolValue( "media_scan.open", false ); // ?  Don't let this go?
        SBDataSetIntValue( "webplaylist.total", this.a_array.length );
        SBDataSetIntValue( "webplaylist.current", this.a_array.length );

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
    href_loop.currentURL = SBDataGetStringValue("browser.uri");
    href_loop.handledURLs = [];
    
    // Useful function to be called by the internal code
    href_loop.handleURL = function webPlaylist_handleURL(url) {
      if (gPPS.isPlaylistURL(url)) {
        installClickHandler(this.a_array[this.i]);

        // Keep the loop going.
        return false;
      }
      
      if (!gPPS.isMediaURL(url)) {
        // decrement the total (floor is 0) to keep the percentage indicator moving
        SBDataDecrementValue("webplaylist.total", 0);

        // Keep the loop going.
        return false;
      }
      
      // This must be a media URL.
      
      // Tell other folks that we're on to the next item.
      SBDataSetIntValue("webplaylist.current", this.i + 1);
      
      installClickHandler(this.a_array[this.i]);
      
      // Don't insert it if we already did.
      if (this.handledURLs.indexOf(url) != -1) {
        // Keep the loop going.
        return false;
      }
      
      var browserURL = gBrowser.currentURI.spec;
      
      if (!this.handledURLs.length) {
        // When we first find media, flip the webplaylist.
        
        var propCache = aMediaListView.mediaList.library.
                        QueryInterface(Components.interfaces.sbILocalDatabaseLibrary).
                        propertyCache;
        propCache.write();
        
        var propArray =
          Components.classes["@songbirdnest.com/Songbird/Properties/PropertyArray;1"]
                    .createInstance(Components.interfaces.sbIPropertyArray);

        propArray.appendProperty("http://songbirdnest.com/data/1.0#originUrl",
                                 browserURL);
        aMediaListView.setFilters(propArray);
        
        var library = aMediaListView.mediaList.library;
        
        var listener = new EnumerationListener();
        library.enumerateItemsByProperty(PROPERTYKEY_ORIGINURL, browserURL, listener,
                                         sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
        var itemCount = listener.items.length;
        
        if (itemCount) {
          library.beginUpdateBatch();
          
          for (var index = 0; index < itemCount; index++) {
            var item = listener.items[index];
            library.remove(item);
          }
          
          library.endUpdateBatch();
        }

        aMediaListView.mediaList.beginUpdateBatch();

        SBDataSetBoolValue("browser.canplaylist", true);
        theWebPlaylistHasItems = true;

        // Then pretend like we clicked on it.
        if (!gBrowser.playlistTree) {
          gBrowser.onBrowserPlaylist();
        }
      }
      
      var mediaList = aMediaListView.mediaList;
      
      var uri = newURI(url);
      var mediaItem = mediaList.library.createMediaItem(uri);
      
      mediaItem.setProperty("http://songbirdnest.com/data/1.0#originUrl",
                            browserURL);
      // mediaItem.write();
      
      mediaList.add(mediaItem);
      
      this.handledURLs.push(url);
      
      // Only one synchronous database call per ui frame.
      return true;
    }
    
    SBDataSetIntValue( "webplaylist.total", href_loop.a_array.length );
    SBDataSetBoolValue( "media_scan.open", true ); // ?  Don't let this go?
  }  
}
catch ( err )
{
  alert( "sbIAsyncForLoop - load\r\n" + err );
}
