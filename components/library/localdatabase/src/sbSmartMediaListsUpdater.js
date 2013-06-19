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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

const debugLog = false;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

function SmartMediaListsUpdater() {
  // ask for a callback when the library manager is ready
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, 'songbird-library-manager-ready', false); 
}

SmartMediaListsUpdater.prototype = {
  // This is us.
  constructor     : SmartMediaListsUpdater,
  classDescription: "Songbird Smart Medialists Updater Module",
  classID         : Components.ID("{35af253e-c7b0-40d6-a1a2-c747de924639}"),
  contractID      : "@songbirdnest.com/Songbird/SmartMediaListsUpdater;1",

  _xpcom_categories: [{
    category: "app-startup",
    service: true
  }],
  
  // medialistlistener batch count
  _batchCount            : 0,
  
  // hash table of properties that have been modified
  _updatedProperties     : {},
  
  // hash table of lists to update
  _updateQueue           : {},
  
  // hash table of lists to update condition to filter video items
  _updateVideoQueue      : {},
  
  // currently updating a batch of smart playlists
  _updating              : false,
  
  // Delay between the last db update and the first smart playlist rebuild
  _updateInitialDelay    : 1000,
  
  // Maximum delay between first db update and the delayed check (the actual time
  // may still be more if a batch has not finished, ie, we never check inside batches)
  _maxInitialDelay       : 5000,
  
  // Timestamp of the first time we try to run the delayed batch, reset every time
  // the check happens, but stays the same when it is further delayed)
  _timerInitTime         : null,
  
  // Cause one more checkForUpdate at the end of the current update
  _causeMoreChecks       : false,
  
  // Delay between each smart playlist rebuild
  _updateSubsequentDelay : 500,

  // Query object for db access
  _dbQuery               : null,

  // db table names
  _dirtyPropertiesTable  : "smartplupd_dirty_properties",
  _dirtyListsTable       : "smartplupd_dirty_lists",
  _updateVideoListsTable : "smartplupd_update_video_lists",
  _monitor               : null,
  
  // --------------------------------------------------------------------------
  // setup init & shutdown
  // --------------------------------------------------------------------------
  observe: function(subject, topic, data) {
    var obs = Cc["@mozilla.org/observer-service;1"]
                .getService(Ci.nsIObserverService);

    if (topic == "songbird-library-manager-ready") {
      // To register on final-ui-startup directly will break the library
      // sort data rebuilding during library migration for unknown reason.
      // Workaround by registering on songbird-library-manager-ready first.
      obs.removeObserver(this, "songbird-library-manager-ready");
      obs.addObserver(this, "final-ui-startup", false); 
    } else if (topic == "final-ui-startup") {
      // Smart Playlist Update should happen after rebuild of sortable value.
      // So wait for final-ui-startup to do the real update.
      obs.removeObserver(this, "final-ui-startup");
      obs.addObserver(this, "songbird-library-manager-before-shutdown", false);
      this.initialize();
    } else if (topic == "songbird-library-manager-before-shutdown") {
      obs.removeObserver(this, "songbird-library-manager-before-shutdown");
      this.shutdown();
    }
  },

  // --------------------------------------------------------------------------
  // Initialization
  // --------------------------------------------------------------------------
  initialize: function() {
    // listen for everything
    this._monitor = 
      new LibraryUtils.GlobalMediaListListener(this, 
                                               false,
                                               Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED |
                                               Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED |
                                               Ci.sbIMediaList.LISTENER_FLAGS_ITEMUPDATED |
                                               Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN |
                                               Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND |
                                               Ci.sbIMediaList.LISTENER_FLAGS_LISTCLEARED,
                                               null,
                                               LibraryUtils.mainLibrary);

    // Init the dirty properties db and tables
    this._dbQuery = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                      .createInstance(Ci.sbIDatabaseQuery);
    this._dbQuery.setAsyncQuery(false);
    this._dbQuery.setDatabaseGUID("songbird");

    // holds the dirty properties
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("CREATE TABLE IF NOT EXISTS " +
                           this._dirtyPropertiesTable +
                           " (propertyid TEXT UNIQUE NOT NULL)");
    this._dbQuery.execute();

    // holds the dirty lists
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("CREATE TABLE IF NOT EXISTS " +
                           this._dirtyListsTable +
                           " (listguid TEXT UNIQUE NOT NULL)");
    this._dbQuery.execute();

    // holds the update lists
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("CREATE TABLE IF NOT EXISTS " +
                           this._updateVideoListsTable +
                           " (listguid TEXT UNIQUE NOT NULL)");
    this._dbQuery.execute();

    // Apply a function to every row value #0 in a db table
    var query = this._dbQuery;
    function applyOnTableValues(aTableId, aFunction) {
      query.resetQuery();
      query.addQuery("SELECT * FROM " + aTableId);
      query.execute();
      var result = query.getResultObject();
      if (result && result.getRowCount() > 0) {
        for (var i = 0; i < result.getRowCount(); i++) {
          var value = result.getRowCell(i, 0);
          aFunction(value);
        }
      }
    }

    // remember our context
    var that = this;

    // If we have lists in the dirty lists db table, we need to add them to the
    // _updateQueue js table.
    function addToUpdateQueue(aListGuid) {
      // check that the list is still valid, just in case.
      var mediaList = LibraryUtils.mainLibrary.getMediaItem(aListGuid);
      if (mediaList instanceof Ci.sbIMediaList &&
          mediaList.type == "smart") {
        that._updateQueue[aListGuid] = mediaList;
      }
    }
    applyOnTableValues(this._dirtyListsTable, addToUpdateQueue);

    // If we have properties in the dirty properties db table, we need to add
    // them to the _updatedProperties js table.
    function addToModifiedProperties(aPropertyID) {
      that._updatedProperties[aPropertyID] = true;
    }
    applyOnTableValues(this._dirtyPropertiesTable, addToModifiedProperties);

    // If we have lists in the update video lists db table, we need to add them
    // to the _updateVideoQueue js table.
    var length = 0;
    function addToUpdateVideoQueue(aListGuid) {
      // check that the list is still valid, just in case.
      var mediaList = LibraryUtils.mainLibrary.getMediaItem(aListGuid);
      if (mediaList instanceof Ci.sbIMediaList &&
          mediaList.type == "smart") {
        that._updateVideoQueue[aListGuid] = mediaList;
        ++length;
      }
    }
    applyOnTableValues(this._updateVideoListsTable, addToUpdateVideoQueue);

    // Update the smart playlists condition to filter video items.
    if (length) {
      this.updateListConditions();
      this.resetUpdateVideoListsTable();
    }

    // Start an update if needed, after a delay. This will update any list
    // in the update queue, as well as any list whose content is dependent on
    // one of the dirty properties, if any. 
    this.delayedUpdateCheck();
  },
  
  // --------------------------------------------------------------------------
  // Shutdown
  // --------------------------------------------------------------------------
  shutdown: function() {
    // Clean up
    this._timer = null;
    this._secondaryTimer = null;
    if (this._monitor) {
      this._monitor.shutdown();
      this._monitor = null;
    }
  },

  // --------------------------------------------------------------------------
  // print a debug message in the console
  // --------------------------------------------------------------------------
  /*
  _log: function(str, isError) {
    if (debugLog || 
        isError) {
      Components.utils.reportError("smartMediaListsUpdater - " + str);
    }
  },
  */
  
  // --------------------------------------------------------------------------
  // Entering batch notification, increment counter
  // --------------------------------------------------------------------------
  onBatchBegin: function(aMediaList) {
    this._batchCount++;
  },
  
  // --------------------------------------------------------------------------
  // Leaving batch notification, decrement counter.
  // --------------------------------------------------------------------------
  onBatchEnd: function(aMediaList) {
    // If counter is zero, schedule an update check
    if (--this._batchCount == 0) {
      this.delayedUpdateCheck();
    }
  },

  // --------------------------------------------------------------------------
  // Item was added, all its properties are new, so cause all smart
  // playlists to eventually update  
  // --------------------------------------------------------------------------
  onItemAdded: function(aMediaList, aMediaItem, aIndex) {
    if (!aMediaList ||
        aMediaList instanceof Ci.sbILibrary) {
      if (aMediaItem instanceof Ci.sbIMediaList) {
        return true;
      }
      // new item imported in library,
      // record the '*' property in the update table
      this.recordUpdateProperty('*');
    } else {
      // record the fact that this playlist changed
      this.recordUpdateProperty(aMediaList.guid);
    }
    // if we are in a batch, return true so we're not told about item
    // additions in this batch anymore
    if (this._batchCount > 0) 
      return true;
    // if we are not in a batch, schedule an update check
    this.delayedUpdateCheck();
  },

  // --------------------------------------------------------------------------
  // Item was removed, all its properties are going away, so cause all
  // smart playlists to eventually update
  // --------------------------------------------------------------------------
  onAfterItemRemoved: function(aMediaList, aMediaItem, aIndex) {
    if (!aMediaList ||
        aMediaList instanceof Ci.sbILibrary) {
      if (aMediaItem instanceof Ci.sbIMediaList) {
        return true;
      }
      // item removed from library,
      // record the '*' property in the update table
      this.recordUpdateProperty('*');
    } else {
      // record the fact that this playlist changed
      this.recordUpdateProperty(aMediaList.guid);
    }
    // if we are in a batch, return true so we're not told about item
    // additions in this batch anymore
    if (this._batchCount > 0) 
      return true;
    // if we are not in a batch, schedule an update check
    this.delayedUpdateCheck();
  },

  // --------------------------------------------------------------------------
  // Item was updated, add the modified properties to the property table
  // then cause the corresponding smart playlists to eventually update
  // --------------------------------------------------------------------------
  onItemUpdated: function(aMediaList, aMediaItem, aProperties) {
    // We don't care about property changes on lists
    if (aMediaItem instanceof Ci.sbIMediaList)
      return true;
    
    // If we are in a batch, and the "update all" flag has been 
    // added to the property list, then there is no need
    // to keep tracking which properties are dirty.
    // This can save a huge amount of time when importing and
    // scanning 10,000+ tracks.
    if (this._batchCount > 0 && this._updatedProperties["*"]) {
      return true;
    }
    
    // for all properties in the array...
    for (var i=0; i<aProperties.length; i++) {
      var property = aProperties.getPropertyAt(i);
      // record the property in the updated properties table
      this.recordUpdateProperty(property.id);
    }
    // if we are in a batch, return false so that we keep receiving more
    // notifications about property changes, since these could be about
    // other properties than the ones we have been notified about in this
    // call.
    if (this._batchCount > 0) 
      return false;
    // if we are not in a batch, schedule an update check
    this.delayedUpdateCheck();
  },

  // --------------------------------------------------------------------------
  // list was cleared, add the list to the playlist update table
  // then cause the corresponding smart playlists to update
  // --------------------------------------------------------------------------
  onListCleared: function(list, excludeLists) {
    // record the fact that this playlist changed
    this.recordUpdateProperty(list.guid);
    if (this._batchCount > 0) 
      return false;
    // if we are not in a batch, schedule an update check
    this.delayedUpdateCheck();
  },

  // --------------------------------------------------------------------------
  // These do not get called since we don't ask for them, but still implement
  // the complete interface
  // --------------------------------------------------------------------------
  onBeforeListCleared: function(list, excludeLists) {},
  onBeforeItemRemoved: function(list, item, index) {},
  onItemMoved: function(list, item, index) {},
  
  // --------------------------------------------------------------------------
  // Add a property to the updated properties table
  // --------------------------------------------------------------------------
  recordUpdateProperty: function(aPropertyID) {
    // if the property is not yet in the table, add it
    if (!(aPropertyID in this._updatedProperties)) {
      //this._log("Property change : " + aPropertyID);
      this._updatedProperties[aPropertyID] = true;
      // remember that this property is dirty, so that if we are shut down
      // before the lists are updated, we can still resume the update on the
      // next startup by calling delayedUpdateCheck, which will determine which
      // lists need updating.
      this.addPropertyToDirtyTable(aPropertyID);
    }
  },
  
  // --------------------------------------------------------------------------
  // Update the smart playlist condition.
  // --------------------------------------------------------------------------
  updateListConditions: function() {
    var propertyManager =
      Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
        .getService(Ci.sbIPropertyManager);
    var typePI = propertyManager.getPropertyInfo(SBProperties.contentType);

    var condition = {
      property     : SBProperties.contentType,
      operator     : typePI.getOperator(typePI.OPERATOR_NOTEQUALS),
      leftValue    : "video",
      rightValue   : null,
      displayUnit  : null,
    };
    var defaultSmartPlaylists = [
      SBString("smart.defaultlist.highestrated", "Highest Rated"),
      SBString("smart.defaultlist.mostplayed", "Most Played"),
      SBString("smart.defaultlist.recentlyadded", "Recently Added"),
      SBString("smart.defaultlist.recentlyplayed", "Recently Played")
    ];
    var list;

    function objectConverter(a) {
      var obj = {};
      for (var i = 0; i < a.length; ++i) {
        obj[a[i]] = '';
      }
      return obj;
    }

    for (var guid in this._updateVideoQueue) {
      list = this._updateVideoQueue[guid];
      // Append the condition to filter video items.
      if (list.name in objectConverter(defaultSmartPlaylists)) {
        list.appendCondition(condition.property,
                             condition.operator,
                             condition.leftValue,
                             condition.rightValue,
                             condition.displayUnit);
      }
      else
        continue;

      list.rebuild();
    }
  },

  // --------------------------------------------------------------------------
  // Returns an array of all smart playlists
  // --------------------------------------------------------------------------
  getSmartPlaylists: function() {
    var enumListener = {
      items: [],
      onEnumerationBegin: function(aMediaList) { },
      onEnumerationEnd: function(aMediaList) { },
      onEnumeratedItem: function(aMediaList, aMediaItem) {
        // if this list is a smart playlist, add it to the array
        if (aMediaItem.type == 'smart') {
          this.items.push(aMediaItem);
        }
        // continue enumeration
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      QueryInterface:
        XPCOMUtils.generateQI([Ci.sbIMediaListEnumerationListener])
    };

    // create a property array so we can specify that we only want items with
    // isList == 1, and hidden == 0
    var pa = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
               .createInstance(Ci.sbIMutablePropertyArray);
    pa.appendProperty(SBProperties.isList, "1");
    pa.appendProperty(SBProperties.hidden, "0");
    
    // start the enumeration
    LibraryUtils.mainLibrary.
      enumerateItemsByProperties(pa, 
                                 enumListener, 
                                 Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

    // return the array of smart playlists
    return enumListener.items;
  },
  
  // --------------------------------------------------------------------------
  // schedule an update check. 
  // --------------------------------------------------------------------------
  delayedUpdateCheck: function() {
    // if the update is already taking place, only update the update queue,
    // so that the lists that need updating will be processed at the end of
    // the current queue
    if (this._updating) {
      this._causeMoreChecks = true;
      return;
    }
    // if timer has not been created, create it now
    if (!this._timer)
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    // we may have triggered this timer already, and not enough time has passed
    // for it to cause the update to begin, so cancel the timer and set it
    // again. this has the effect of delaying further the updates as more
    // medialistlistener notifications are received and until none has been
    // received for a delay long enough for the timer notification to be issued.
    // Note that we also check if 'a long time' has passed since we started
    // delaying the update check, and force it if that's the case, so as to avoid
    // being indefinitly blocked by something constantly updating properties.
    var now = new Date().getTime();
    if (!this._timerInitTime) this._timerInitTime = now;
    this._timer.cancel();
    if (this._batchCount == 0 && 
        now - this._timerInitTime > this._maxInitialDelay) {
      this.notify(this._timer);
    } else {
      // start the timer
      this._timer.initWithCallback(this,
                                   this._updateInitialDelay,
                                   Ci.nsITimer.TYPE_ONE_SHOT);
    }      
  },
  
  // --------------------------------------------------------------------------
  // timer notification
  // --------------------------------------------------------------------------
  notify: function(aTimer) {
    // The delayed update timer has expired... if there are no new batches in 
    // progress, check to see if we need to update the smart playlists
    if (this._batchCount == 0) {
      if (aTimer == this._timer) {
        // reset first delayed update check time
        this._timerInitTime = null;
        // properties have been changed some time ago, we need to go through the
        // list of smart playlists, and schedule an update for the ones whose
        // content has potentially changed
        this.checkForUpdates();
      } else if (aTimer == this._secondaryTimer) {
        // perform the update for the next smart playlist in the queue
        this.performUpdates();
      }
    } else {
      // Otherwise, defer the check for a while longer
      this.delayedUpdateCheck();
    }
  },

  // --------------------------------------------------------------------------
  // Check to see if we need to add any smart playlist in the update queue
  // due to a property change that may affect the list's content.
  // Note that this function may run while the updates are in progress (ie,
  // a property has changed while the update was already started, and it is
  // not finished yet). This is fine because all this function does is check
  // the list of modified properties and add the appropriate lists to the
  // update queue, so if the update is running, the newly added lists will
  // run after the ones that were already scheduled.
  // --------------------------------------------------------------------------
  checkForUpdates: function() {
    //this._log("checkForUpdates");
    // if no properties have been modified, no list need to be added to
    // the queue (this does not mean that the queue is empty).
    if (!this.emptyOfProperties(this._updatedProperties)) {
      // get all smart playlists
      var lists = this.getSmartPlaylists();
      // for all smart playlists...
      for each (var list in lists) {
        // if the list is already in the update queue, continue with the
        // next one
        if (list.guid in this._updateQueue)
          continue;
        // fetch the interface we need
        list.QueryInterface(Ci.sbILocalDatabaseSmartMediaList);
        // if this list is not auto updating, skip it
        if (!list.autoUpdate)
          continue;
        // if we have a limit with a matching select property, add the list to
        // the update queue and to the dirty lists table, otherwise, check
        // individual conditions for a property match
        if (list.limit != Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE && 
            ("*" in this._updatedProperties ||
             list.selectPropertyID in this._updatedProperties)) {
          this._updateQueue[list.guid] = list;
          this.addListToDirtyTable(list);
        } else {
          // for all smart playlist conditions...
          for (var c=0; c<list.conditionCount; c++) {
            // get condition at index c
            var condition = list.getConditionAt(c);
            // if the condition property is in the table, or "*" is in the table,
            // add the list to the update queue and to the dirty lists table
            if ("*" in this._updatedProperties ||
                condition.propertyID in this._updatedProperties ||
                this.isPlaylistConditionMatch(condition.propertyID, condition.leftValue, this._updatedProperties)) {
              this._updateQueue[list.guid] = list;
              this.addListToDirtyTable(list);
              // and continue on with the next list
              break;
            }
          }
        }
      }
      // reset the list of modified properties. If more properties are changed
      // while we are in the process of updating, we only want those lists that
      // match those new properties to be added to the queue (and they will only
      // be added if they are either not in there already, ie, they were not
      // scheduled for update before, or they have already been updated, in which
      // case they need to be updated again). 
      this._updatedProperties = {};
      // empty the dirty properties table, since we're about to populate the
      // dirty lists table (they're no longer needed, since they only serve to
      // let us figure out which lists need updating on next startup in the case
      // where we never got here)
      this.resetDirtyPropertiesTable();
    }
    // if we're not updating yet, check if we need to start doing so
    if (!this._updating &&
        !this.emptyOfProperties(this._updateQueue)) {
      // start updating, this will update the first list, and schedule the
      // next one.
      this.performUpdates();
    }
  },
  
  // --------------------------------------------------------------------------
  // Test whether a condition uses a rule on a dirty playlist
  // --------------------------------------------------------------------------
  isPlaylistConditionMatch: function(prop, value, dirtyprops) {
    if (prop != "http://songbirdnest.com/dummy/smartmedialists/1.0#playlist")
      return false;
    return (value in dirtyprops);
  },
  
  // --------------------------------------------------------------------------
  // actually performs the update for one list, then schedule the next
  // --------------------------------------------------------------------------
  performUpdates: function() {
    // extract and remove the first list from the queue.
    // is there a better way to do this with a map ?
    var remaining = {};
    var list;
    for (var v in this._updateQueue) { 
      if (!list)
        list = this._updateQueue[v];
      else
        remaining[v] = this._updateQueue[v];
    }
    this._updateQueue = remaining;
    
    // this should really not be happening, but test anyway
    if (!list) {
      // print a console message since this is not supposed to happen
      //this._log("list is null in sbSmartMediaListsUpdater.js", true);
      // go back to idle mode
      this._updating = false;
      // make sure the dirty lists table is empty
      this.resetDirtylistsTable();
      return;
    }

    //this._log("Updating list " + 
    //          list.name + " (" + 
    //          list.type + ", " + 
    //          list.guid + ")");

    // we are now updating a whole bunch of playlists, one at a time
    this._updating = true;

    // get a notification when the playlist is done rebuilding. The rebuild
    // is actually synchronous for now, but going via a callback means that this
    // code will not need any change if/when we use asynchronous updates instead
    list.addSmartMediaListListener(this);
    
    // cause the rebuild
    list.rebuild();
  },

  // --------------------------------------------------------------------------
  // Called when the smart playlist is done rebuilding.
  // --------------------------------------------------------------------------
  onRebuild: function(aSmartMediaList) {
    // remove listener
    aSmartMediaList.removeSmartMediaListListener(this);

    // Remove this list from the dirty db table, so we don't rebuild it on
    // next startup, even if the app is shut down before we finish to update
    // all the lists in the queue
    this.removeListFromDirtyTable(aSmartMediaList);
    
    // if there are any more lists to update, start the timer again,
    // with a shorter delay than the initial one
    if (!this.emptyOfProperties(this._updateQueue)) {
      if (!this._secondaryTimer)
        this._secondaryTimer = 
          Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

      this._secondaryTimer.initWithCallback(this,
                                            this._updateSubsequentDelay,
                                            Ci.nsITimer.TYPE_ONE_SHOT);
    } else {
      // if nothing more to do, then go back to idle mode.
      this._updating = false;
      // the dirty lists table should be empty by now, but it doesn't hurt
      // to make sure
      this.resetDirtyListsTable();
      // if we got more changes during the update, check them now
      if (this._causeMoreChecks) {
        this._causeMoreChecks = false;
        this.delayedUpdateCheck();
      }
    }
    this._currentListUpdate = null;
  },

  // --------------------------------------------------------------------------
  // returns true if a map is empty. is there a better way to do this ?
  // --------------------------------------------------------------------------
  emptyOfProperties: function(obj) {
    var hasItems = false;
    for each (var v in obj) { 
      hasItems = true; 
      break; 
    }
    return !hasItems;
  },
  
  // --------------------------------------------------------------------------
  // add one list to the dirty lists db table
  // --------------------------------------------------------------------------
  addListToDirtyTable: function(aMediaList) {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("INSERT OR REPLACE INTO " + 
                           this._dirtyListsTable + 
                           " VALUES (\"" + 
                           aMediaList.guid + 
                           "\")");
    this._dbQuery.execute();
  },
  
  // --------------------------------------------------------------------------
  // add one property to the dirty properties db table
  // --------------------------------------------------------------------------
  addPropertyToDirtyTable: function(aPropertyID) {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("INSERT OR REPLACE INTO " + 
                           this._dirtyPropertiesTable + 
                           " VALUES (\"" + 
                           aPropertyID + 
                           "\")");
    this._dbQuery.execute();
  },
  
  // --------------------------------------------------------------------------
  // remove one list from the dirty lists db table
  // --------------------------------------------------------------------------
  removeListFromDirtyTable: function(aMediaList) {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("DELETE FROM " + 
                           this._dirtyListsTable + 
                           " WHERE listguid = \"" + 
                           aMediaList.guid + 
                           "\"");
    this._dbQuery.execute();
  },
  
  // --------------------------------------------------------------------------
  // remove all rows from the dirty lists db table
  // --------------------------------------------------------------------------
  resetDirtyListsTable: function() {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("DELETE FROM " + this._dirtyListsTable);
    this._dbQuery.execute();
  },

  // --------------------------------------------------------------------------
  // remove all rows from the dirty properties db table
  // --------------------------------------------------------------------------
  resetDirtyPropertiesTable: function() {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("DELETE FROM " + this._dirtyPropertiesTable);
    this._dbQuery.execute();
  },

  // --------------------------------------------------------------------------
  // remove all rows from the update video lists db table
  // --------------------------------------------------------------------------
  resetUpdateVideoListsTable: function() {
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("DELETE FROM " + this._updateVideoListsTable);
    this._dbQuery.execute();
  },

  // --------------------------------------------------------------------------
  // QueryInterface
  // --------------------------------------------------------------------------
  QueryInterface:
    XPCOMUtils.generateQI([Ci.sbIMediaListListener,
                           Ci.nsITimerCallback,
                           Ci.sbILocalDatabaseSmartMediaListListener])

}; // SmartMediaListsUpdater.prototype

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// function postRegister(aCompMgr, aFileSpec, aLocation) {
//   // Get instantiated on startup
//   XPCOMUtils.categoryManager
//             .addCategoryEntry('app-startup',
//                               'smartplaylists-updater', 
//                               'service,@songbirdnest.com/Songbird/SmartMediaListsUpdater;1',
//                               true, 
//                               true);
// }

// module
var NSGetModule = 
  XPCOMUtils.generateNSGetFactory([SmartMediaListsUpdater], 
                                  postRegister);
