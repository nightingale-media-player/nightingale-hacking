/*
Copyright (c) 2008, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of Pioneers of the Inevitable, Songbird, nor the names
	  of its contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

if (typeof Search == 'undefined') {
  var Search = {};
}

if (typeof kPlaylistCommands == 'undefined') {
  Components.utils.import("resource://app/components/kPlaylistCommands.jsm");
}

function LOG(text) {
  // dump(text);
}

// Ian's magical enumerator enumerator function
function enumerate(enumerator, func) {
  while(enumerator.hasMoreElements()) {
    try {
      func(enumerator.getNext());
    } catch(e) {
      Cu.reportError(e);
    }
  }
}


/**
 * Controller for search events
 */
Search = {
  playlist : null,
  progress_label: null,
  progress: null,

  /**
   * Initialize our playlist, etc.
   */
  init : function() {
    this._strings = document.getElementById("seeqpod-strings");
    this.playlist = document.getElementById("search-result");
    this.progress_label = document.getElementById('progress-label');
    this.progress = document.getElementById('progress');
    this._metrics =
      Components.classes['@songbirdnest.com/Songbird/Metrics;1']
      .getService(Ci.sbIMetrics);

    // Setup our library
    Library.init();

    // evil hacks from matt
    this.playlist._setupColumnSpec = function() {
      this._saveColumnSpecTimeout = null;
    }
    this.playlist._setupColumns = function() {
      LOG('_setupColumns\n');
      var columnMap = ColumnSpecParser.parseColumnSpec(Library.columnSpec).columnMap;
      this._columnSpecOrigin = ColumnSpecParser.prototype.ORIGIN_ATTRIBUTE;
      this._removeAllChildren(this._treecols);

      // Get the list of columns from the property manager and add them to the
      // tree
      var numColumns = columnMap.length;

      // Add the columns that have saved settings, keeping track of which column
      // were added
      var alreadyAdded = {};
      var addedColumns = 0;
      var ordinal = 1;
      var sortProperty;
      var sortDirection;
      columnMap.forEach(
        function(columnInfo) {
          var propertyInfo = this._pm.getPropertyInfo(columnInfo.property);
          if(propertyInfo.userViewable) {
            if (!sortProperty && columnInfo.sort) {
              sortProperty = columnInfo.property;
              sortDirection = columnInfo.sort;
            }
            this._appendColumn(propertyInfo,
                               false,
                               columnInfo.width,
                               ordinal);
            alreadyAdded[columnInfo.property] = true;
            addedColumns++;
            ordinal += 2;
          }
        },
        this);
      if (this.isSortable == "1" && this.tree.view) {
        var ldtv = this.tree.view.QueryInterface(this._Ci.sbILocalDatabaseTreeView);
        ldtv.setSort(sortProperty, sortDirection == "ascending");
      }
      if (this.tree.columns) {
        this.tree.columns.invalidateColumns();
      }
    }


    // what was the last query?
    var last_query = SBDataGetStringValue('seeqpod.query');
    // Bind the playlist widget to our library to show the last results
    Search.filter(last_query);
    // put the query in our search box
    document.getElementById('search-term').value = last_query;

    // if a search is pending, do it
    if (SBDataGetBoolValue('seeqpod.searchnow')) {
      setTimeout(function() {
        Search.doSearch(document.getElementById('search-term').value);
      });
      SBDataSetBoolValue('seeqpod.searchnow', false);
    }
  },

  /**
   * filter the library to show only @aQuery
   */
  filter: function(aQuery) {
    // Create a filtered view of the web library
    var view = Library.library.createView();
    // Add a filter for our seeqpod query filter
    var filterIndex = view.cascadeFilterSet.appendFilter(Library.seeqpodQuery);
    // Filter our view of web library based on the term
    view.cascadeFilterSet.set(this.filterIndex, [aQuery], 1);

    // Get playlist commands (context menu, keyboard shortcuts, toolbar)
    // Note: playlist commands currently depend on the playlist widget.
    var mgr =
      Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Components.interfaces.sbIPlaylistCommandsManager);
    var cmds = mgr.request(kPlaylistCommands.MEDIAITEM_DEFAULT);
    // Set up the playlist widget
    this.playlist.bind(view, cmds);
  },

  /**
   * Process key pressed on our page
   */
  doKeyPress: function(aEvent) {
    // Get the pressed key code.
    var keyCode = aEvent.keyCode;

    // Do a search
    if ((keyCode == Components.interfaces.nsIDOMKeyEvent.DOM_VK_ENTER) ||
        (keyCode == Components.interfaces.nsIDOMKeyEvent.DOM_VK_RETURN)) {
      this.doSearch(document.getElementById('search-term').value);
      aEvent.stopPropagation();
      aEvent.preventDefault();
    }

    // Clear the search entry form
    if (keyCode == Components.interfaces.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      document.getElementById('search-term').value = "";
      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  },

  /**
   * Invoked when the user press the magic button
   */
  doSearch : function(value) {
    SBDataSetStringValue('seeqpod.query', value);

    // In case we are still processing things from the previous search
    ResultsManager.stop();

    // Filter in old results
    Search.filter(value);
    // Clear the old results
    ResultsManager.clearOldResults();
    // Clear the filter
    Search.filter('');

    // Hide our suggestion
    document.getElementById('spellcheck').className = 'hidden';

    // Repopulate term in case we came from something suggested
    document.getElementById('search-term').value = value;

    if (value != "") {
      // Suggest alternate spelling
      this.suggestSpelling(value);

      // We got something to search for
      this.populateResults(value);
    }
  },

  suggestSpelling: function(term) {
    SeeqPod.spellcheck(term, 9.0, function(alternative) {
      if (alternative != null) {
        document.getElementById('spellcheck').className = 'unhidden';
        document.getElementById('suggestion').value = alternative;
      }
    });
  },

  /**
   * Iterate over the search result and populate the play list
   */
  populateResults: function(term) {
    // Analytic on search
    this._metrics.metricsInc('seeqpod', 'search', '');

    document.getElementById('seeqpod-window').className = 'busy';

    // Update the progress ui
    this.progress_label.value = this._strings.getString('progress.search');
    this.progress.value = 0;

    var offset = 0;
    var count = 50;
    var max = Application.prefs.get('extensions.seeqpod.maxresults').value;

    var uriArray =
        Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
        .createInstance(Components.interfaces.nsIMutableArray);
    var propertyArrayArray =
        Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
        .createInstance(Components.interfaces.nsIMutableArray);

    function processResult(result) {
      for (var i=0; result && i < result.length; i++) {
        var track = result[i];
        function decode(t) {
          try {
            // seeqpod puts binary in the form \xXX where X is a
            // hex char
            // so, let's turn this into a valid uri encoded string
            t = t.replace('%','%25')
               .replace(/\\x([0-9a-fA-F][0-9a-fA-F])/g, '%$1');
            // and let unescape do the hard work
            t = unescape(t);
          } catch(e) {
            // if something fails, we don't really care that much
          }
          return t;
        }
        function tagText(tagName) {
          // get the text from the DOM
          var t = track.getElementsByTagName(tagName)[0].textContent;
          return decode(t);
        }

        // add the URI to the property array
        uriArray.appendElement(newURI(tagText('location')), false);

        // create a property array
        var propertyArray =
          Components.classes['@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1']
          .createInstance(Components.interfaces.sbIMutablePropertyArray);

        // Add properties from the seeqpod API response
        propertyArray.appendProperty(SBProperties.artistName, tagText('creator'));
        propertyArray.appendProperty(SBProperties.trackName, tagText('title'));
        propertyArray.appendProperty(SBProperties.albumName, tagText('album'));

        // Add properites for downloading
        propertyArray.appendProperty(SBProperties.enableAutoDownload, "1");
        propertyArray.appendProperty(SBProperties.downloadButton, "1|0|0");

        LOG('adding result: '+tagText('location')+'\n');
        LOG('  track: '+tagText('title')+'\n');
        LOG('  album: '+tagText('album')+'\n');
        LOG('  artist: '+tagText('creator')+'\n');

        // append it to the property array array
        propertyArrayArray.appendElement(propertyArray, false);
      }

      ResultsManager.addResultCandidates(uriArray, propertyArrayArray);

      if (result && result.length && offset + count < max) {
        // more to do
        offset += count;

        Search.progress.value = parseInt(50 * offset / max);

        SeeqPod.search(term, offset, count, processResult);
      } else {
        // no more to do
        ResultsManager.checkLinks();
      }
    }

    // start the search
    ResultsManager.init();
    SeeqPod.search(term, offset, count, processResult);
  },
  /**
   * Cancel the search
   */
  cancel: function(preserveResults) {
    SeeqPod.cancel();
    ResultsManager.stop();
    if (!preserveResults) {
      Search.filter('');
    }
    document.getElementById('seeqpod-window').className = '';
  }
}

if (typeof ResultsManager == "undefined") {
  var ResultsManager={};
}
/**
 * Manage results candidate. This will take care of validating the urls and kick a
 * meatada scan job when done
 */
ResultsManager = {
  checkers: null,

  init: function() {
    this.checkers = new Object();
  },

  // return an enumerator for all the media items currently in the view
  enumerateViewItems: function() {
    var view = Search.playlist.mediaListView;
    var enumerator = { items: [] };
    for (let i=0; i<view.length; i++) {
      enumerator.items.push(view.getItemByIndex(i));
    }
    enumerator.hasMoreElements = function() { return this.items.length > 0; }
    enumerator.getNext = function() { return this.items.pop(); }
    return enumerator;
  },

  clearOldResults: function() {
    Library.library.removeSome(this.enumerateViewItems());
  },

  addResultCandidates: function(uriArray, propertyArrayArray) {
    try {
      // create items in the media library
      var itemArray = Library.library.batchCreateMediaItems(
        uriArray, propertyArrayArray, true);

      // Add items to the media list
      Library.list.addSome(itemArray.enumerate());

      // Set the query property on the items (batched)
      Library.library.runInBatchMode(function() {
        enumerate(itemArray.enumerate(), function(item) {
          item.setProperty(Library.seeqpodQuery,
                           SBDataGetStringValue('seeqpod.query'));
        });
      })
    } catch(e) {
      // an error occurred adding results
      Components.utils.reportError(e);
    }
  },

  checkLinks: function() {
    // check each of the tracks in lastSearchMediaList to see if the link is OK
    Search.progress_label.value = Search._strings.getString('progress.deadlinks');

    this.totalLinks = Library.list.length;
    this.processedLinks = 0;

    var enumerator = {};
    enumerator.onEnumerationBegin = function(list) {
      return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;};
    enumerator.onEnumerationEnd = function(list, status) { };
    enumerator.onEnumeratedItem = function(list, item) {
      ResultsManager.checkLink(item, list);
      return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
    };
    Library.list.enumerateAllItems(enumerator);

    // make sure that the link checking doesn't take more than 5 seconds
    var self = this;
    setTimeout(function() {
      if (!self.checkCompleted()) {
        self.cancelCheck();
      }
    }, 5000)
  },

  checkLink: function(item, list) {
    var uri = Components.classes["@mozilla.org/network/standard-url;1"]
        .createInstance(Components.interfaces.nsIURI);
    var checker = Components.classes["@mozilla.org/network/urichecker;1"]
        .createInstance(Components.interfaces.nsIURIChecker);

    var observer = {
      finished: false,
      onStartRequest : function(aRequest,aContext) {
      },
      onStopRequest : function(aRequest, aContext, aStatusCode) {
        this.finished = true;
        let valid = true;

        if (aStatusCode != 0) {
          // was the HTTP request successful?
          valid = false;
        } else {
          try {
            let mimetype = checker.baseChannel.contentType;
            if (!mimetype.match(/^audio/)) {
              // we only support audio/*
              valid = false;
            }
          } catch(e) {
            // an error here means we're not going to have much success
            // streaming the file
            valid = false;
          }
        }
        if (!valid) {
          // remove the item from the library
          Library.library.remove(item);
        }

        // remove this checker from the list
        delete ResultsManager.checkers[item.guid];

        // update the progress bar
        ResultsManager.processedLinks++;
        Search.progress.value = 50 +
            parseInt(50 * ResultsManager.processedLinks /
                     ResultsManager.totalLinks);

        // Signal back that we are done
        if (ResultsManager.checkCompleted()) {
          ResultsManager.removeDuplicates();
        }
      }
    };
    uri.spec = item.getProperty(SBProperties.contentURL);
    checker.init(uri);
    this.checkers[item.guid] = checker;
    checker.asyncCheck(observer, null);
  },

  checkCompleted: function() {
    for (k in this.checkers) {
      return false;
    }
    return true;
  },

  cancelCheck: function() {
    for (guid in this.checkers) {
      this.checkers[guid].cancel(Components.results.NS_BINDING_ABORTED);
    }
  },

  removeDuplicates: function() {
    Search.progress.value = 100;
    Search.progress_label.value = Search._strings.getString('progress.dupes');
    try {
      var unique = new Object();
      var dupes = [];

      function hashMediaItem(item) {
        return [item.getProperty(SBProperties.trackName),
               item.getProperty(SBProperties.albumName),
               item.getProperty(SBProperties.artistName)]
                 .toSource().toLowerCase();
      }

      var enumerator = {};
      enumerator.onEnumerationBegin = function(list) {
        return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;};
      enumerator.onEnumerationEnd = function(list, status) { };
      enumerator.onEnumeratedItem = function(list, item) {
        var hash = hashMediaItem(item);
        if (unique[hash]) {
          // not unique - add to the list
          LOG('removing dupe: '+item.contentSrc.spec+'\n')
          dupes.push(item);
        } else {
          // unique, add it to the hash
          unique[hash] = item.guid;
        }
        return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
      };
      Library.list.enumerateAllItems(enumerator);
      // if there were dupes to remove, remove them:
      if (dupes.length) {
        var enumerator = { items: dupes };
        enumerator.hasMoreElements = function() { return this.items.length > 0; }
        enumerator.getNext = function() { return this.items.pop(); }
        Library.library.removeSome(enumerator);
      }
      Search.progress.value = 100;
    } catch(e) {
      // an unexpected exception was thrown in de-duping
      Components.utils.reportError(e);
    }
    Search.filter(SBDataGetStringValue('seeqpod.query'));
    document.getElementById('seeqpod-window').className = '';
    this.scanMetadata();
  },

  scanMetadata: function() {
    // collect all the remaining media items into an XPCOM array
    var mediaItemArray =
        Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
        .createInstance(Components.interfaces.nsIMutableArray);

    enumerate(this.enumerateViewItems(), function(item) {
      LOG('adding '+item+' to mediaItemArray\n');
      mediaItemArray.appendElement(item, false);
    })

    if (mediaItemArray.length <= 0) {
      // nothing to scan
      return;
    }

    // start a metadata scanning job
    var manager =
        Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
        .getService(Components.interfaces.sbIFileMetadataService);
    var job = manager.read(mediaItemArray);
    this.metadataScanJob = job;
    job.addJobProgressListener(
      function onComplete(job) {
        if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
          return;
        }
        // The job has completed, do something usefull
        job.removeJobProgressListener(onComplete);

        // Remove media items that don't have times, probably means something is wrong
        var items = mediaItemArray.QueryInterface(Components.interfaces.nsIArray).enumerate();
        while (items.hasMoreElements()) {
          item = items.getNext();
          if (item.getProperty(SBProperties.duration) == null) {
            Library.library.remove(item);
          }
        }

        ResultsManager.metadataScanJob = null;
      }
    );
  },

  stop: function() {
    // cancel checkers
    this.cancelCheck();
    // cancel metadata scan
    if (this.metadataScanJob) {
      var cancellableJob = this.metadataScanJob.QueryInterface(Components.interfaces.sbIJobCancelable);
      if (cancellableJob && cancellableJob.canCancel) {
        cancellableJob.cancel();
      }
    }
  }
}
