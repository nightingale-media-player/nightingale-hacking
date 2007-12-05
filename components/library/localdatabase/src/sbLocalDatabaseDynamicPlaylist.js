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
Components.utils.import("resource://app/components/ArrayConverter.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const SB_NS = "http://songbirdnest.com/data/1.0#";
const SB_PROP_ISSUBSCRIPTION       = SB_NS + "isSubscription";
const SB_PROP_SUBSCRIPTIONURL      = SB_NS + "subscriptionURL";
const SB_PROP_SUBSCRIPTIONINTERVAL = SB_NS + "subscriptionInterval";
const SB_PROP_SUBSCRIPTIONNEXTRUN  = SB_NS + "subscriptionNextRun";
const SB_PROP_DESTINATION          = SB_NS + "destination"

const SBLDBCOMP = "@songbirdnest.com/Songbird/Library/LocalDatabase/";

const SB_LIBRARY_MANAGER_READY_TOPIC = "songbird-library-manager-ready";
const SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC = "songbird-library-manager-before-shutdown";

const UPDATE_INTERVAL = 60 * 1000;

function d(s) {
  //dump("------------------> sbLocalDatabaseDynamicPlaylistService " + s + "\n");
}

function TRACE(s) {
  //dump("------------------> " + s + "\n");
}

// XXXsteve Any time we get a media item's guid and want to use it as an
// object property, run it through this function first.  This will force the
// string to be a javascript land string rather than a string dependent on the
// buffer held by the media item.  This it to prevent crashes when the atom
// created from that string gets deleted at shutdown.  See bmo 391590
function FIX(s) {
  var g = "x" + s;
  g = g.substr(1);
  return g;
}

function sbLocalDatabaseDynamicPlaylistService()
{
  this._started = false;
  this._scheduledLists = {};
  this._ignoreLibraryNotifications = {};
  this._libraryBatch = new MultiBatchHelper();
  this._libraryRefreshPending = {};

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC, false);
  obs.addObserver(this, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, false);
}

sbLocalDatabaseDynamicPlaylistService.prototype = {
  classDescription: "Local Database Dynamic Playlist Service",
  classID:          Components.ID("{9bb3fae0-23ad-4b96-8366-8d754a30f72e}"),
  contractID:       SBLDBCOMP + "DynamicPlaylistService;1"
}

sbLocalDatabaseDynamicPlaylistService.prototype.QueryInterface =
XPCOMUtils.generateQI([
  Ci.sbILocalDatabaseDynamicPlaylistService,
  Ci.nsIObserver,
  Ci.nsITimerCallback,
  Ci.sbILibraryManagerListener,
  Ci.sbIMediaListListener
]);

/*
 * Startup method for the dynamic playlist service
 */
sbLocalDatabaseDynamicPlaylistService.prototype._startup =
function sbLocalDatabaseDynamicPlaylistService__startup()
{
  TRACE("sbLocalDatabaseDynamicPlaylistService::_startup\n");

  if (this._started) {
    return;
  }

  // Register properties
  var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                  .getService(Ci.sbIPropertyManager);

  var prop = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
               .createInstance(Ci.sbINumberPropertyInfo);
  prop.id = SB_PROP_ISSUBSCRIPTION;
  prop.userViewable = false;
  prop.userEditable = false;
  prop.minValue = 0;
  prop.maxValue = 1;
  propMan.addPropertyInfo(prop);

  prop = Cc["@songbirdnest.com/Songbird/Properties/Info/URI;1"]
           .createInstance(Ci.sbIURIPropertyInfo);
  prop.id = SB_PROP_SUBSCRIPTIONURL;
  prop.userViewable = false;
  prop.userEditable = false;
  propMan.addPropertyInfo(prop);

  var prop = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
               .createInstance(Ci.sbINumberPropertyInfo);
  prop.id = SB_PROP_SUBSCRIPTIONINTERVAL;
  prop.userViewable = false;
  prop.userEditable = false;
  prop.minValue = 0;
  propMan.addPropertyInfo(prop);

  var prop = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
               .createInstance(Ci.sbINumberPropertyInfo);
  prop.id = SB_PROP_SUBSCRIPTIONNEXTRUN;
  prop.userViewable = false;
  prop.userEditable = false;
  prop.minValue = 0;
  propMan.addPropertyInfo(prop);

  // Set up our timer
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._timer.initWithCallback(this,
                               UPDATE_INTERVAL,
                               Ci.nsITimer.TYPE_REPEATING_SLACK);

  // Attach an observer so we can track library additions / removals
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  libraryManager.addListener(this);

  // Schedule all off the dynamic media lists in all libraries
  var libraries = libraryManager.getLibraries();
  while (libraries.hasMoreElements()) {
    var library = libraries.getNext();
    if (library instanceof Ci.sbILocalDatabaseLibrary)
      this.onLibraryRegistered(library);
  }

  // Do a queue run just in care some things should have been refreshed while
  // we were off
  this._updateSubscriptions(false);

  this._started = true;
}

sbLocalDatabaseDynamicPlaylistService.prototype._shutdown =
function sbLocalDatabaseDynamicPlaylistService__shutdown()
{
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.removeObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
  obs.removeObserver(this, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);

  if (this._started) {
    this._timer.cancel();
    this._timer = null;

    // Stop listening to the library manager
    var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                           .getService(Ci.sbILibraryManager);
    libraryManager.removeListener(this);

    // Stop listening to each library
    var libraries = libraryManager.getLibraries();
    while (libraries.hasMoreElements()) {
      this.onLibraryUnregistered(libraries.getNext());
    }
  }

  this._started = false;
}

sbLocalDatabaseDynamicPlaylistService.prototype._scheduleLibrary =
function sbLocalDatabaseDynamicPlaylistService__scheduleLibrary(aLibrary)
{
  // Add all the media lists that are subscriptions to the _scheduledList
  // array
  var self = this;
  var listener = {
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      self._scheduledLists[FIX(item.guid)] = item;
      return true;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  var pa = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
             .createInstance(Ci.sbIMutablePropertyArray);
  pa.appendProperty(SB_NS + "isList", "1");
  pa.appendProperty(SB_PROP_ISSUBSCRIPTION, "1");

  aLibrary.enumerateItemsByProperties(pa,
                                      listener );

}

sbLocalDatabaseDynamicPlaylistService.prototype._isDynamicPlaylist =
function sbLocalDatabaseDynamicPlaylistService__isDynamicPlaylist(aMediaList)
{
  return aMediaList.getProperty(SB_PROP_ISSUBSCRIPTION) == "1";
}

sbLocalDatabaseDynamicPlaylistService.prototype._removeListsFromLibrary =
function sbLocalDatabaseDynamicPlaylistService__removeListsFromLibrary(aLibrary)
{
  for each (var list in this._scheduledLists) {
    if (list.library.equals(aLibrary)) {
        delete this._scheduledLists[FIX(list.guid)];
    }
  }
}

sbLocalDatabaseDynamicPlaylistService.prototype._updateSubscriptions =
function sbLocalDatabaseDynamicPlaylistService__updateSubscriptions(aForce)
{
  var now = (new Date()).getTime() * 1000;
  for each (var list in this._scheduledLists) {

    var nextRun  = list.getProperty(SB_PROP_SUBSCRIPTIONNEXTRUN);
    var interval = list.getProperty(SB_PROP_SUBSCRIPTIONINTERVAL);

    // If there is no next run set, set it
    if (!nextRun) {
      nextRun = this._setNextRun(list);
    }

    // Update this list if we are forced or if we have an interval and the
    // current time is after the next run
    if (aForce || (interval && now > nextRun)) {
      try {
        this._updateList(list);
      }
      catch (e) {
        // Log and continue
        Components.utils.reportError(e);
      }
    }
  }
}

sbLocalDatabaseDynamicPlaylistService.prototype._updateList =
function sbLocalDatabaseDynamicPlaylistService__updateList(aList)
{
  // Get the playlist reader and load the tracks from this url
  var manager = Cc["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                  .getService(Ci.sbIPlaylistReaderManager);
  var listener = Cc["@songbirdnest.com/Songbird/PlaylistReaderListener;1"]
                   .createInstance(Ci.sbIPlaylistReaderListener);

  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  var uri = ioService.newURI(aList.getProperty(SB_PROP_SUBSCRIPTIONURL), null, null);

  var observer = new sbPlaylistReaderListenerObserver(this, aList);
  listener.observer = observer;

  manager.loadPlaylist(uri, aList, null, true, listener);
}

sbLocalDatabaseDynamicPlaylistService.prototype._setNextRun =
function sbLocalDatabaseDynamicPlaylistService__setNextRun(aList)
{
  var now = (new Date()).getTime() * 1000;
  var interval = aList.getProperty(SB_PROP_SUBSCRIPTIONINTERVAL);

  // Interval is in seconds, next run is in micro seconds
  var nextRun = now + (interval * 1000 * 1000);
  aList.setProperty(SB_PROP_SUBSCRIPTIONNEXTRUN, nextRun);
  return nextRun;
}

sbLocalDatabaseDynamicPlaylistService.prototype._beginIgnore =
function sbLocalDatabaseDynamicPlaylistService__beginIgnore(aLibrary)
{
  var count = this._ignoreLibraryNotifications[FIX(aLibrary.guid)];
  if (count)
    this._ignoreLibraryNotifications[FIX(aLibrary.guid)] = count + 1;
  else
    this._ignoreLibraryNotifications[FIX(aLibrary.guid)] = 1;
}

sbLocalDatabaseDynamicPlaylistService.prototype._endIgnore =
function sbLocalDatabaseDynamicPlaylistService__endIgnore(aLibrary)
{
  var count = this._ignoreLibraryNotifications[FIX(aLibrary.guid)];
  if (count)
    this._ignoreLibraryNotifications[FIX(aLibrary.guid)] = count - 1;
  else
    throw Cr.NS_ERROR_FAILURE;
}

sbLocalDatabaseDynamicPlaylistService.prototype._ignore =
function sbLocalDatabaseDynamicPlaylistService__ignore(aLibrary)
{
  var count = this._ignoreLibraryNotifications[FIX(aLibrary.guid)];
  return count && count > 0;
}

// sbILocalDatabaseDynamicPlaylistService
sbLocalDatabaseDynamicPlaylistService.prototype.createList =
function sbLocalDatabaseDynamicPlaylistService_createList(aLibrary,
                                                          aUri,
                                                          aIntervalSeconds,
                                                          aDestinationDirectory)
{
  if (!aLibrary ||
      !aUri ||
      aIntervalSeconds == 0 ||
      (aDestinationDirectory && !aDestinationDirectory.isDirectory()))
    throw Cr.NS_ERROR_INVALID_ARG;

  try {
    this._beginIgnore(aLibrary);
    var list = aLibrary.createMediaList("dynamic");
    this._scheduledLists[FIX(list.guid)] = list;
    this.updateList(list, aUri, aIntervalSeconds, aDestinationDirectory);
  }
  finally {
    this._endIgnore(aLibrary);
  }
  return list;
}

sbLocalDatabaseDynamicPlaylistService.prototype.updateList =
function sbLocalDatabaseDynamicPlaylistService_updateList(aMediaList,
                                                          aUri,
                                                          aIntervalSeconds,
                                                          aDestinationDirectory)
{
  if (!aMediaList ||
      !this._isDynamicPlaylist(aMediaList) ||
      !aUri ||
      aIntervalSeconds == 0 ||
      (aDestinationDirectory && !aDestinationDirectory.isDirectory()))
    throw Cr.NS_ERROR_INVALID_ARG;

  try {
    this._beginIgnore(aMediaList.library);
    aMediaList.setProperty(SB_PROP_SUBSCRIPTIONURL, aUri.spec);
    aMediaList.setProperty(SB_PROP_SUBSCRIPTIONINTERVAL, aIntervalSeconds);

    if (aDestinationDirectory) {
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
      var destinationUri = ioService.newFileURI(aDestinationDirectory);
      aMediaList.setProperty(SB_PROP_DESTINATION, destinationUri.spec);
    }
  }
  finally {
    this._endIgnore(aMediaList.library);
  }

}

sbLocalDatabaseDynamicPlaylistService.prototype.updateAllNow =
function sbLocalDatabaseDynamicPlaylistService_updateAllNow()
{
  this._updateSubscriptions(true);
}

sbLocalDatabaseDynamicPlaylistService.prototype.updateNow =
function sbLocalDatabaseDynamicPlaylistService_updateNow(aMediaList)
{
  var interval = aMediaList.getProperty(SB_PROP_SUBSCRIPTIONINTERVAL);
  var url = aMediaList.getProperty(SB_PROP_SUBSCRIPTIONURL);
  if (interval && url)
    this._updateList(aMediaList);
  else
    throw Cr.NS_ERROR_INVALID_ARG;
}

sbLocalDatabaseDynamicPlaylistService.prototype.__defineGetter__("scheduledLists",
function sbLocalDatabaseDynamicPlaylistService_scheduledLists()
{
  var a = [];
  for each (var list in this._scheduledLists) {
    a.push(list);
  }

  return ArrayConverter.enumerator(a);
});

// sbILibraryManagerListener
sbLocalDatabaseDynamicPlaylistService.prototype.onLibraryRegistered =
function sbLocalDatabaseDynamicPlaylistService_onLibraryRegistered(aLibrary)
{
  TRACE("sbLocalDatabaseDynamicPlaylistService::onLibraryRegistered");

  // Ignore non local database libraries
  if (!(aLibrary instanceof Ci.sbILocalDatabaseLibrary))
    return;

  // If a library is registered, attach a listener so we can be notified of
  // media item change notifications.
  var filter = SBProperties.createArray([[SB_PROP_ISSUBSCRIPTION, null]], false);
  aLibrary.addListener(this,
                       false,
                       Ci.sbIMediaList.LISTENER_FLAGS_ALL &
                         ~Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED,
                       filter);

  // Schedule all of the dynamic media lists in this library
  this._scheduleLibrary(aLibrary);
}

sbLocalDatabaseDynamicPlaylistService.prototype.onLibraryUnregistered =
function sbLocalDatabaseDynamicPlaylistService_onLibraryUnregistered(aLibrary)
{
  TRACE("sbLocalDatabaseDynamicPlaylistService::onLibraryUnregistered");

  // Ignore non local database libraries
  if (!(aLibrary instanceof Ci.sbILocalDatabaseLibrary))
    return;

  // Remove all the lists that are in this library
  this._removeListsFromLibrary(aLibrary);

  // Remove our listener from this library
  aLibrary.removeListener(this);
}

// sbIMediaListListener
sbLocalDatabaseDynamicPlaylistService.prototype.onItemAdded =
function sbLocalDatabaseDynamicPlaylistService_onItemAdded(aMediaList,
                                                           aMediaItem)
{
  if (this._ignore(aMediaList.library))
    return;

  // If we are in a batch, we are going to refresh the list of dynamic
  // playlists when the batch ends, so we don't need any more notifications
  if (this._libraryBatch.isActive(aMediaList.library)) {
    this._libraryRefreshPending[FIX(aMediaList.library.guid)] = true;
    return true;
  }

  // Is this new item a dynamic media list?
  if (this._isDynamicPlaylist(aMediaItem)) {

    // If there is no next run time, set one
    if (aMediaItem.getProperty(SB_PROP_SUBSCRIPTIONNEXTRUN)) {
      this._setNextRun(aMediaItem);
    }
    this._scheduledLists[FIX(aMediaItem.guid)] = aMediaItem;
    d("A new dynamic playlist was added");
  }

}

sbLocalDatabaseDynamicPlaylistService.prototype.onBeforeItemRemoved =
function sbLocalDatabaseDynamicPlaylistService_onBeforeItemRemoved(aMediaList,
                                                                   aMediaItem)
{
  if (this._ignore(aMediaList.library))
    return;

  // If we are in a batch, we are going to refresh the list of dynamic
  // playlists when the batch ends, so we don't need any more notifications
  if (this._libraryBatch.isActive(aMediaList.library)) {
    this._libraryRefreshPending[FIX(aMediaList.library.guid)] = true;
    return true;
  }

  delete this._scheduledLists[FIX(aMediaItem.guid)];
}

sbLocalDatabaseDynamicPlaylistService.prototype.onAfterItemRemoved =
function sbLocalDatabaseDynamicPlaylistService_onAfterItemRemoved(aMediaList,
                                                                  aMediaItem)
{
  // do nothing
}

sbLocalDatabaseDynamicPlaylistService.prototype.onItemUpdated =
function sbLocalDatabaseDynamicPlaylistService_onItemUpdated(aMediaList,
                                                             aMediaItem,
                                                             aProperties)
{
  if (this._ignore(aMediaList.library))
    return;

  // If we are in a batch, we are going to refresh the list of dynamic
  // playlists when the batch ends, so we don't need any more notifications
  if (this._libraryBatch.isActive(aMediaList.library)) {
    this._libraryRefreshPending[FIX(aMediaList.library.guid)] = true;
    return true;
  }

  // Make sure this list is scheduled
  this._scheduledLists[FIX(aMediaItem.guid)] = aMediaItem;
}

sbLocalDatabaseDynamicPlaylistService.prototype.onListCleared =
function sbLocalDatabaseDynamicPlaylistService_onListCleared(aMediaList)
{
  if (this._ignore(aMediaList.library))
    return;

  // If we are in a batch, we are going to refresh the list of dynamic
  // playlists when the batch ends, so we don't need any more notifications
  if (this._libraryBatch.isActive(aMediaList.library)) {
    this._libraryRefreshPending[FIX(aMediaList.library.guid)] = true;
    return true;
  }

  // Clear all schedueld items from this library
  this._removeListsFromLibrary(aMediaList.library);
}

sbLocalDatabaseDynamicPlaylistService.prototype.onBatchBegin =
function sbLocalDatabaseDynamicPlaylistService_onBatchBegin(aMediaList)
{
  this._libraryBatch.begin(aMediaList.library);
}

sbLocalDatabaseDynamicPlaylistService.prototype.onBatchEnd =
function sbLocalDatabaseDynamicPlaylistService_onBatchEnd(aMediaList)
{
  var library = aMediaList.library;
  this._libraryBatch.end(library);

  if (!this._libraryBatch.isActive(library)) {

    // If there is a refresh pending for this library, do it
    if (this._libraryRefreshPending[FIX(library.guid)]) {

      d("Refreshing dynamic playlists in library " + library);
      this._removeListsFromLibrary(library);
      this._scheduleLibrary(library);

      this._libraryRefreshPending[FIX(library.guid)] = false;
    }
  }
}

// nsITimerCallback
sbLocalDatabaseDynamicPlaylistService.prototype.notify =
function sbLocalDatabaseDynamicPlaylistService_notify(aTimer)
{
  this._updateSubscriptions(false);
}

// nsIObserver
sbLocalDatabaseDynamicPlaylistService.prototype.observe =
function sbLocalDatabaseDynamicPlaylistService_observe(aSubject, aTopic, aData)
{
  if (aTopic == SB_LIBRARY_MANAGER_READY_TOPIC) {
    this._startup();
  }
  else if(aTopic == SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC) {
    this._shutdown();
  }
}

function sbPlaylistReaderListenerObserver(aService, aList) {
  this._service = aService;
  this._list = aList;
  this._oldLength = aList.length;
}

sbPlaylistReaderListenerObserver.prototype.observe =
function sbPlaylistReaderListenerObserver_observe(aSubject, aTopic, aData)
{
  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  var uri = ioService.newURI(this._list.getProperty(SB_PROP_SUBSCRIPTIONURL), null, null);

  TRACE("Updated list with uri " + uri.spec);

  // Schedule the next run for this list
  this._service._setNextRun(this._list);

  // Check to see if this list has a custom download destination, and that
  // the destination is a directory
  var destination = this._list.getProperty(SB_PROP_DESTINATION);
  var destinationDir;
  if (destination) {
    try {
      destinationDir = ioService.newURI(destination, null, null)
                                .QueryInterface(Ci.nsIFileURL).file;
      if (!destinationDir.isDirectory())
        destinationDir = null;
    }
    catch (e) {
      // If we couldn't get a destination dir, use the default
      destinationDir = null;
    }
  }

  // Start a metadata job for the newly added items.  If there is a custom
  // destination, update each new item with it
  var array = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
  for (var i = this._oldLength; i < this._list.length; i++) {
    var item = this._list.getItemByIndex(i);
    if (destinationDir) {
      var itemUri = item.contentSrc;

      // If this is not a nsIURL, let the download manager figure it out
      if (itemUri instanceof Ci.nsIURL) {
        var dest = destinationDir.clone();
        dest.append(itemUri.QueryInterface(Ci.nsIURL).fileName);
        var destUri = ioService.newFileURI(dest);
        item.setProperty(SB_PROP_DESTINATION, destUri.spec);
      }
    }
    array.appendElement(item, false);
  }

  var metadataJobManager =
    Cc["@songbirdnest.com/Songbird/MetadataJobManager;1"]
      .getService(Ci.sbIMetadataJobManager);
  var metadataJob = metadataJobManager.newJob(array, 5);

  // Download the new items
  var ddh =
    Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
      .getService(Ci.sbIDownloadDeviceHelper);
  ddh.downloadSome(array.enumerate());
}

// sbIMedaiListFactory
function sbLocalDatabaseDynamicMediaListFactory()
{
}
sbLocalDatabaseDynamicMediaListFactory.prototype = {
  classDescription: "Local Database Dynamic Media List Factory",
  classID:          Components.ID("{1dcff5da-9a36-46d7-87fd-a6f0d0ec081f}"),
  contractID:       SBLDBCOMP + "DynamicMediaListFactory;1",

  type: "dynamic",
  createMediaList: function sbLocalDatabaseDynamicMediaListFactory_createMediaList(aInner) {

    var smlf = Cc[SBLDBCOMP + "SimpleMediaListFactory;1"]
                 .getService(Ci.sbIMediaListFactory);
    var list = smlf.createMediaList(aInner);
    list.setProperty(SB_PROP_ISSUBSCRIPTION, "1");

    //Set customType for use by metrics.
    list.setProperty(SBProperties.customType,
                     "dynamic");

    return list;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListFactory])
}

function NSGetModule(compMgr, fileSpec) {

  return XPCOMUtils.generateModule([
    sbLocalDatabaseDynamicPlaylistService,
    sbLocalDatabaseDynamicMediaListFactory
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      sbLocalDatabaseDynamicPlaylistService.prototype.classDescription,
      "service," + sbLocalDatabaseDynamicPlaylistService.prototype.contractID,
      true,
      true);
  });
}
