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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm");

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

// big-ass debug function
function DEBUG(msg) {
  return;

  function repr(x) {
    if (x == undefined) {
      return 'undefined';
    } else if (x == null) {
      return 'null';
    } else if (typeof x == 'function') {
      return x.name+'(...)';
    } else if (typeof x == 'string') {
      return x.toSource().match(/^\(new String\((.*)\)\)$/)[1];
    } else if (typeof x == 'number') {
      return x.toSource().match(/^\(new Number\((.*)\)\)$/)[1];
    } else if (typeof x == 'object' && x instanceof Array) {
      var value = '';
      for (var i=0; i<x.length; i++) {
        if (i) value = value + ', ';
        value = value + repr(x[i]);
      }
      return '['+value+']';
    } else if (x instanceof Ci.nsISupports) {
      return x.toString();
    } else {
      return x.toSource();
    }
  }
  dump('DEBUG '+DEBUG.caller.name);
  if (msg == undefined) {
    // when nothing is passed in, print the arguments
    dump('(');
    for (var i=0; i<DEBUG.caller.length; i++) {
      if (i) dump(', ');
      dump(repr(DEBUG.caller.arguments[i]));
    }
    dump(')');
  } else {
    dump(': ');
    if (typeof msg != 'object' || msg instanceof Array) {
      dump(repr(msg));
    } else {
      dump(msg.toSource());
    }
  }
  dump('\n');
}


  ////////////////////
 // Initialization //
////////////////////
function sbAutoDownloader() {
  DEBUG();

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, 'songbird-library-manager-ready', false);
}
sbAutoDownloader.prototype._libraryManager = null;
sbAutoDownloader.prototype._library = null;
sbAutoDownloader.prototype._helper = null;
sbAutoDownloader.prototype._queue = [];
sbAutoDownloader.prototype._timer = null;

  ///////////
 // XPCOM //
///////////
sbAutoDownloader.prototype.classDescription =
    'Songbird Auto Downloader Service';
sbAutoDownloader.prototype.classID =
    Components.ID("{a3d7426b-0b22-4f07-b72c-e44bab0759f7}");
sbAutoDownloader.prototype.contractID = '@songbirdnest.com/autodownloader;1';
sbAutoDownloader.prototype.flags = Ci.nsIClassInfo.SINGLETON;
sbAutoDownloader.prototype.interfaces =
    [Ci.nsISupports, Ci.nsIClassInfo, Ci.nsIObserver, Ci.sbIMediaListListener];
sbAutoDownloader.prototype.getHelperForLanguage = function(x) { return null; }
sbAutoDownloader.prototype.getInterfaces =
function sbAutoDownloader_getInterfaces(count, array) {
  array.value = this.interfaces;
  count.value = array.value.length;
}
sbAutoDownloader.prototype.QueryInterface =
    XPCOMUtils.generateQI(sbAutoDownloader.prototype.interfaces);


  /////////////////
 // nsIObserver //
/////////////////
sbAutoDownloader.prototype.observe =
function sbAutoDownloader_observe(subject, topic, data) {
  DEBUG();

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);

  if (topic == "songbird-library-manager-ready") {
    obs.removeObserver(this, "songbird-library-manager-ready");
    obs.addObserver(this, "songbird-library-manager-before-shutdown", false);

    // get the library manager
    this._libraryManager = Cc['@songbirdnest.com/Songbird/library/Manager;1']
                             .getService(Ci.sbILibraryManager);

    // get the main library
    this._library = this._libraryManager.mainLibrary;

    // watch for added items
    this._library.addListener(this, false,
        Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED);

    this._helper = Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                     .getService(Ci.sbIDownloadDeviceHelper);

  } else if (topic == "songbird-library-manager-before-shutdown") {
    obs.removeObserver(this, "songbird-library-manager-before-shutdown");

    if (this._library) {
      this._library.removeListener(this);
    }

    if (this._timer) {
      this._clearTimer();
    }
  } else if (topic == 'timer-callback') {
    while (this._queue.length) {
      var item = this._queue.shift();
      var playlist = this._helper.getDownloadMediaList();
      if (!this._library.contains(item)) {
        // it's been removed from the library
        continue;
      }
      if (playlist.contains(item)) {
        // it's already in the download playlist
        continue;
      }
      if (item.getProperty(SBProperties.destination)) {
        // it's already been processed by the download device
        continue;
      }
      this._helper.downloadItem(item);
    }
    this._clearTimer();
  }
}


  //////////////////////////
 // sbIMediaListListener //
//////////////////////////
sbAutoDownloader.prototype.onItemAdded =
function sbAutoDownloader_onItemAdded(aMediaList, aMediaItem) {
  DEBUG();
  if ((aMediaItem.getProperty(SBProperties.enableAutoDownload) == "1") &&
      aMediaItem.contentSrc.scheme.match(/^http/)) {
    // Don't download items already in the download medialist.
    if (!this._helper.getDownloadMediaList().contains(aMediaItem)) {
      this._queue.push(aMediaItem);
      if (!this._timer) {
        this._setUpTimer();
      }
    }
  }
}
sbAutoDownloader.prototype.onBeforeItemRemoved =
function sbAutoDownloader_onBeforeItemRemoved(aMediaList, aMediaItem) {
  DEBUG();
}
sbAutoDownloader.prototype.onAfterItemRemoved =
function sbAutoDownloader_onAfterItemRemoved(aMediaList, aMediaItem) {
  DEBUG();
}
sbAutoDownloader.prototype.onItemUpdated =
function sbAutoDownloader_onItemUpdated(aMediaList, aMediaItem, aProperties) {
  DEBUG();
}
sbAutoDownloader.prototype.onListCleared =
function sbAutoDownloader_onListCleared(aMediaList) {
  DEBUG();
}
sbAutoDownloader.prototype.onBatchBegin =
function sbAutoDownloader_onBatchBegin(aMediaList) {
  DEBUG();
}
sbAutoDownloader.prototype.onBatchEnd =
function sbAutoDownloader_onBatchEnd(aMediaList) {
  DEBUG();
}


  /////////////////////
 // Private Methods //
/////////////////////
sbAutoDownloader.prototype._setUpTimer =
function sbAutoDownloader__setUpTimer() {
  DEBUG();
  if (!this._timer) {
    // set up the timer
    this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    this._timer.init(this, 500, Ci.nsITimer.TYPE_REPEATING_SLACK);
  }
}
sbAutoDownloader.prototype._clearTimer =
function sbAutoDownloader__clearTimer() {
  DEBUG();
  if (this._timer) {
    this._timer.cancel();
    this._timer = null;
  }
}



var NSGetModule = XPCOMUtils.generateNSGetModule(
  [
    sbAutoDownloader,
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      sbAutoDownloader.prototype.classDescription,
      "service," + sbAutoDownloader.prototype.contractID,
      true,
      true);
  }
);
