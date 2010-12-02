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

/* Web Scraper Steps */

/*
  This file is part of the web scraper, a system for creating playlists
  out of web content. The WebScraper instantiates these steps.
  
  The web scraper is based around an extensible architecture.
  
  The basic idea is that the data flows through a graph (which is strictly
  linear at this point, though not by necessity.) Data enters via "Sources"
  and is discarded at a "Sink". Intermediate steps examine incoming items
  and annotate, modify, add, or discard items. Below is a sample "Identity"
  step, which can be added with no effect on the pipeline.
  
  Each step is implemented as a javascript generator function ScraperStep_and the steps
  below make good examples of ways in which these tools can be pieced together.
  
  (For more documentation on generators see:
    http://developer.mozilla.org/en/New_in_JavaScript_1.7
  )
  
  The default execution flow is such that each call of pipeline.next() should
  introduce a single new link to the the pipeline and run it through to the finish.
  
  Yielding something false from your step will close down the pipeline.
  
  \sa webScraper.js
*/

var WebScraperSteps = {
  /*
   * Identity
   * 
   *   A do-nothing template step for a pipeline.
   *   The "properties" starts as an anonymous object containing properties for the
   *   eventual media properties and a contextNode containing the location the properties was
   *   found.
   *   for example:
   *   { contextNode: <someNode>, SBProperties.contentURL: "http://some/file" }
   */
  Identity: function ScraperStep_Identity(scraper, node, pipeline) {
    var properties;
    while((properties = yield properties)) {
      pipeline.send(properties);
    }
  },
  
  /* DocumentURLSource
   *   Find all URL-alike items in a document and pass them
   *   into the pipeline for further processing.
   *   Must be the first step in a pipeline, as it does not
   *   check for incoming items.
   */
  DocumentURLSource: function ScraperStep_DocumentURLSource(scraper, node, pipeline) {
    if(!node){
      yield false;
    }
  
    // Use xpath instead of getElementsByTagName to preserve
    // document order between tag types.
    // XXX: note the '.' in front of the expressions to select a set of nodes 
    //      relative to the context node.
    var xpath = [ 
        ".//@href",
        // TODO: ultimately, some of these things really shouldn't be treated as urls
        //       particularly the param/@values which are used for random crap all the time
        ".//embed/@src",     ".//*[local-name()='embed']/@src", 
        ".//object/@data",   ".//*[local-name()='object']/@data",
        ".//param/@value",   ".//*[local-name()='param']/@value", 
        ".//enclosure/@url", ".//*[local-name()='enclosure']/@url"
    ].join('|');
  
    var nodeDocument;
    if (node.ownerDocument) {
      nodeDocument = node.ownerDocument;
    }
    else {
      nodeDocument = node;
    }
  
    // Yield here to defer beginning the scan while the generators start up.
    yield true;
  
    var results = nodeDocument.evaluate(
      xpath, node, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null );
   
    // for progress reporting.
    scraper._total = results.snapshotLength;
    for (var i = 0; i < results.snapshotLength; i++) {
      let contextNode = results.snapshotItem(i);
      let url = contextNode.value;
  
      // Resolve relative urls.
      try {
        url = nodeDocument.baseURIObject.resolve(url);
      }
      catch (e) {
        url = null;
      }
  
      if (url) {
        let properties = {};
        
        properties.contextNode = contextNode;
        properties[SBProperties.originURL] = properties[SBProperties.contentURL] = url;
        
        // Update progress. :)
        scraper.job._progress++;
        scraper.job.notifyJobProgressListeners();
  
        pipeline.send(properties);
      }
      
      if (i % 10 == 0) {
        yield true;
      }
    }
  },
  
  CancelScrape: function ScraperStep_CancelScrape(scraper, node, pipeline) {
    var properties;
    while((properties = yield properties)) {
      if (scraper._terminate) {
        // This will throw a StopIteration exception and roll everything up.
        return;
      }
      pipeline.send(properties);
    }
  },
  
  /* MediaURL
   *    a media url filter. throws out anything that doesn't look like a media URL
   *    not really a great policy, but at least it's easy to replace.
   */
  MediaURL: function ScraperStep_MediaURL(scraper, node, pipeline) {
    Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
    var typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                        .createInstance(Ci.sbIMediacoreTypeSniffer);
    var extensionsEnum = typeSniffer.mediaFileExtensions;
    if (!Application.prefs.getValue("songbird.mediascan.enableVideo", false)) {
      // disable video, so scan only audio - see bug 13173
      extensionsEnum = typeSniffer.audioFileExtensions;
    }
    var mediaURLExtensions = [i for each (i in ArrayConverter.JSEnum(extensionsEnum))];
    var mediaURLSchemes = ["mms", "rstp"];
    
    var properties;
    while ((properties = yield properties)) {
      var url = newURI(properties[SBProperties.contentURL])
      if (!(url instanceof Ci.nsIURL)) {
        continue;
      }

      if (mediaURLExtensions.indexOf(url.fileExtension) != -1 ||
          mediaURLSchemes.indexOf(url.scheme) != -1) {
            pipeline.send(properties);
      }
    }
  },
  
  /*
   * Hacks_DropBadUrls
   *   A step used to store hacks which work around common false positives.
   */
  Hacks_DropBadUrls: function ScraperStep_Hacks_DropBadUrls(scraper, node, pipeline) {
    var properties;
    while((properties = yield properties)) {
      var url = newURI(properties[SBProperties.contentURL])
      if (!(url instanceof Ci.nsIURL)) {
        continue;
      }
      
      if (url.fileName.indexOf("playerID") == 0) {
        // TODO:
        // This hack is here because of a common Flash Player
        // which uses a <param> value that looks like a Media URL after we resolve() it.
        // like this example found at skr**mr:
        //    <param name="FlashVars"
        //      value='playerID=1&amp;bg=0xCDDFF3&amp;leftbg=0x357DCE&amp;
        //             lefticon=0xF2F2F2&amp; rightbg=0xF06A51&amp;rightbghover
        //             =0xAF2910&amp;righticon=0xF2F2F2&amp;righticonhover=
        //             0xFFFFFF&amp;text=0x357DCE&amp;slider=0x357DCE&amp;track
        //             =0xFFFFFF&amp;border=0xFFFFFF&amp;loader=0xAF2910&amp;
        //             soundFile=http%3A%2F%2Fbzhrock.free.fr%2Fytrwynau1.mp3'>
        // it would be nice if we didn't do that, but if we don't resolve()
        // we don't pick up relative urls, and if we ignore the field altogether
        // we risk missing out on other valid media.
        continue;
      }
      pipeline.send(properties);
    }
  },
  
  /* DupeCheck
   *    Drop URLs we've already seen.
   *    Drop 'em like they're hot.
   *    Don't use this one for a web scraping session.
   *    Do use this one for a playlist parsing session.
   */
  DupeCheck: function ScraperStep_DupeCheck(scraper, node, pipeline) {
    var properties;
    while ((properties = yield properties)) {
      if (!scraper._seenURLs[properties[SBProperties.originURL]]) {
        pipeline.send(properties);
      }
      scraper._seenURLs[properties[SBProperties.originURL]] = true;
    }
  },
  
  /*
   * AddOriginPage
   * 
   *   Adds origin page information to the properties as it passes by.
   */
  AddOriginPage: function ScraperStep_AddOriginPage(scraper, node, pipeline) {
    // allow for sub-document nodes to get picked up
    if (node.ownerDocument) {
      node = node.ownerDocument;
    }
    
    var properties;
    while((properties = yield properties)) {
      properties[SBProperties.originPage] = node.URL;
      properties[SBProperties.originPageImage] = node.URL;
      properties[SBProperties.originPageTitle] = node.title;
      pipeline.send(properties);
    }
  },
  
  
  CreateMediaItem: function ScraperStep_CreateMediaItem(scraper, node, pipeline, mediaList) {
    var properties;
    while ((properties = yield properties)) {
      var url = newURI(properties[SBProperties.contentURL]);
      if (!(url instanceof Ci.nsIURL)) {
        continue;
      }
      
      // Next we convert the JS object into a property-array
      // which is used to create a media properties (which then replaces
      // the JS object in future pipeline steps.)
      
      // Because we are converting the JS object into a real media properties
      // we have to throw away the context node, as it isn't a valid media
      // properties property.
      delete properties.contextNode;
      
      var uri = newURI(properties[SBProperties.contentURL]);
      delete properties[SBProperties.contentURL];
      if (!uri) {
        continue;
      }
      
      if (!properties[SBProperties.trackName]) {
        properties[SBProperties.trackName] = url.fileName;
      }
      
      // set up the download button
      properties[SBProperties.enableAutoDownload] = "1";
      properties[SBProperties.downloadButton] = "1|0|0";
  
      // create a new media item, if necessary, or get an existing one for
      // this URI
      var mediaItem = mediaList.library.createMediaItem(
                                          uri,
                                          SBProperties.createArray(properties));
      // add it to the media list, ignoring duplicates
      if (!mediaList.contains(mediaItem)) {
        mediaList.add(mediaItem);
        pipeline.send(mediaItem);
      }
    }
  },
  
  ScanForMetadata: function ScraperStep_ScanForMetadata(scraper, node, pipeline) {
    var metadataService = 
           Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                     .getService(Components.interfaces.sbIFileMetadataService);
  
    var mediaItem;
    while(( mediaItem = yield mediaItem )) {
      try {
        // TODO: this is stupidly expensive. and verbose.
        var mediaItemsToScan = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                                 .createInstance(Ci.nsIMutableArray);
        mediaItemsToScan.appendElement(mediaItem, false);
        metadataService.read(mediaItemsToScan)
      } catch(e) {
        Cu.reportError(e);
      }
      pipeline.send(mediaItem);
    }
  },
  
  
  /* Sink
   *   A cap for a pipeline.
   *   Just absorbs any input and makes it so that your other steps
   *   don't need to do any fancy checking to see if they're the last
   *   one.
   */
  Sink: function ScraperStep_Sink(scraper, node, pipeline) {
    var properties;
    while((properties = yield properties)) {
      // do nothing.
    }
  },
  
  /* Squawk
   *   Speak your input. A debug step.
   *   Particularly handy since Venkman doesn't seem
   *   to like setting breakpoints inside generator
   *   functions.
   */
  Squawk: function ScraperStep_Squawk(scraper, node, pipeline) {
    var properties;
    while((properties = yield properties)) {
      var str = "";
      for (var i in properties) {
        str += i  + ": " + properties[i] + "\n";
      }
      Components.utils.reportError(str);
      pipeline.send(properties);
    }
  }

};
