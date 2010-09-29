/*
 *
 *=BEGIN SONGBIRD LICENSE
 *
 * Copyright(c) 2009-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * For information about the licensing and copyright of this Add-On please
 * contact POTI, Inc. at customer@songbirdnest.com.
 *
 *=END SONGBIRD LICENSE
 *
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ObserverUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "New Releases XPCOM Service";
const CID         = "{95b00029-b9d1-4691-b6b4-8180bea463f1}";
const CONTRACTID  = "@songbirdnest.com/newreleases;1";

// MusicBrainz namespace
const mbns = new Namespace('http://musicbrainz.org/ns/mmd-1.0#');

// Database ID for all release results
const allEntriesID = "ALL";

// Number of milliseconds in a 30-day month
const monthInMillis = 3600*24*1000*30;

// Testing flags
const SHORT_TIMER = true;

const newReleaseIconSrc = "chrome://newreleases/skin/node-icon.png";
const noReleaseIconSrc = "";

var monthArray = new Array("January",
                           "February",
                           "March",
                           "April",
                           "May",
                           "June",
                           "July",
                           "August",
                           "September",
                           "October",
                           "November",
                           "December");

var newReleaseExtensionPrefs = Cc["@mozilla.org/preferences-service;1"]
                                 .getService(Ci.nsIPrefService)
                                 .getBranch("extensions.newreleases.");

function debugLog(funcName, str) {
  var debug = newReleaseExtensionPrefs.getBoolPref("debug");
  if (debug)
    dump("*** newReleases.js::" + funcName + " // " + str + "\n");
}

// Helper to memoize a function on an object.
// Caches return values in this._cache using the
// name of the function.
function memoize(func) {
  var funcName = func.name;
  if (!funcName) {
    throw new Error("memoize requires a named function to work correctly");
  }
  return function(key) {
    if (!this._cache) {
      this._cache = {};
    }
    if (!this._cache[funcName]) {
      this._cache[funcName] = {};
    }
    var value = this._cache[funcName][key];
    if (value !== undefined) {
      //debugLog("memoized " + funcName, " returning '" +
      //         value + "' from cache for key " + key);
      return value;
    }

    value = func.apply(this, arguments);
    this._cache[funcName][key] = value;
    //debugLog("memoized " + funcName, " adding '" +
    //         value + "' to cache for key " + key);
    return value;
  }
}

function pseudoThread(gen) {
  var self = this;
  var callback = {
    observe:function(subject, topic, data) {
      switch(topic) {
        case "timer-callback":
          try{
            gen.next();
          } catch (e if e instanceof StopIteration) {
            gen.close();
            self.threadTimer.cancel();
            debugLog("pseudoThread", "Thread exited");
            // no worries
          } catch(e) {
            gen.close();
            self.threadTimer.cancel();
            debugLog("pseudoThread", e);
            Components.utils.reportError(e);
          };
      }
    }
  }

  this.threadTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this.threadTimer.init(callback, 0, Ci.nsITimer.TYPE_REPEATING_SLACK);
}

// NewReleases XPCOM component constructor
function NewReleases() {
  // Make the service available via JS so we can avoid the XPConnect complexity
  this.wrappedJSObject = this;

  // Load our string bundle & cache strings
  var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                       .getService(Ci.nsIStringBundleService)
                       .createBundle("chrome://newreleases/locale/newReleases.properties");

  this.strDownloading = stringBundle.GetStringFromName("downloading");
  this.strDlComplete = stringBundle.GetStringFromName("downloadComplete");
  this.strNewReleasesFound = stringBundle.GetStringFromName("releasesFound");
  this.strNewReleasesFoundP = stringBundle.GetStringFromName("releasesFoundP");
  this.strProcessing = stringBundle.GetStringFromName("processing");
  this.strOf = stringBundle.GetStringFromName("of");
  this.strUnknown = stringBundle.GetStringFromName("unknown");
  this.strFinishing = stringBundle.GetStringFromName("finishing");
  this.strComplete = stringBundle.GetStringFromName("complete");

  this.countryStringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                               .getService(Ci.nsIStringBundleService)
                               .createBundle("chrome://newreleases/locale/country.properties");
  // Setup our properties
  var pMgr = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
               .getService(Ci.sbIPropertyManager);
  if (!pMgr.hasProperty(this.newReleaseImgProperty)) {
    debugLog("constructor", "Creating New Release image property:" +
             this.newReleaseImgProperty);
    var builder = Cc["@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
                    .createInstance(Ci.sbIImagePropertyBuilder);
    builder.propertyID = this.newReleaseImgProperty;
    builder.displayName = stringBundle.GetStringFromName("newReleaseProperty");
    builder.userEditable = false;
    builder.userViewable = true;
    var pI = builder.get();
    pMgr.addPropertyInfo(pI);
  } else {
    debugLog("constructor", "BAD: PROPERTY MANAGER ALREADY HAS " +
             "IMAGE PROPERTY REGISTERED\n");
  }
  if (!pMgr.hasProperty(this.newReleaseUrlProperty)) {
    debugLog("constructor", "Creating New Release URL property:" +
             this.newReleaseUrlProperty);
    var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
               .createInstance(Ci.sbITextPropertyInfo);
    pI.id = this.newReleaseUrlProperty;
    pI.displayName = "Not visible"
    pI.userEditable = false;
    pI.userViewable = false;
    pMgr.addPropertyInfo(pI);
  }

  // Register the property with the smart playlist registrar
  var registrar =
        Cc["@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1"]
          .getService(Ci.sbISmartPlaylistPropertyRegistrar);
  registrar.registerPropertyToContext("default", this.newReleaseImgProperty,
                                      50, "d");
  debugLog("constructor", "newReleaseImgProperty reg'd w/ smartpls registrar");

  // Setup our database
  this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
               .createInstance(Ci.sbIDatabaseQuery);
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);
  var dbdir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
  this.newReleasesDbURI = ios.newFileURI(dbdir);
  this._db.databaseLocation = this.newReleasesDbURI;
  this._db.setDatabaseGUID("releases");
  this._db.setAsyncQuery(false);
  this._db.resetQuery();
  this._cache = {};

  // Get our prefBranch
  var prefService = Cc["@mozilla.org/preferences-service;1"]
                      .getService(Ci.nsIPrefService);
  this.prefs = prefService.getBranch("extensions.newreleases.");

  // Instantiate nsIJSON & sbIMetrics
  this.json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
  this.metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
                   .createInstance(Ci.sbIMetrics);
}

NewReleases.prototype.constructor = NewReleases;
NewReleases.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIClassInfo]),

  _db : null,
  _cache: null,
  _batch: null,
  _observerSet: null,
  newReleasesRefreshRunning : false,
  locationRefreshRunning : false,
  drawingLock : false,
  displayCallback : null,
  spsUpdater : null,
  prefs : null,
  self : this,
  releasesTotal : 0,

  newReleaseImgProperty : "http://songbirdnest.com/data/1.0#artistNewRelease",
  newReleaseUrlProperty :
    "http://songbirdnest.com/data/1.0#artistNewReleaseUrl",

  progressString : "",
  progressPercentage : 0,

  /*********************************************************************
   * Sets up a timer to run repeatedly to asynchronously refresh the
   * releases data.  The timer fires, triggering the timer-callback code
   * which calls refreshNewReleases() on the currently saved city
   *********************************************************************/
  startRefreshThread : function() {
    debugLog("startRefreshThread", "Initialising refresh timer");
    this.refreshTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    // If testing, once per minute - otherwise once per half hour
    var delay = SHORT_TIMER ? 60000 : 1800000;
    debugLog("startRefreshThread", "Timer running every " +
             (delay / 1000) + " seconds");
    this.refreshTimer.init(this, delay, Ci.nsITimer.TYPE_REPEATING_SLACK);

    // Fire a one-shot 5 seconds in
    debugLog("startRefreshThread", "Firing a 1-shot in 5 seconds..");
    this.initialTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.initialTimer.init(this, 5000, Ci.nsITimer.TYPE_ONE_SHOT);

    // Add a media list listener to the main library
    this._batch = new LibraryUtils.BatchHelper();
    var mainLib = Cc['@songbirdnest.com/Songbird/library/Manager;1']
                    .getService(Ci.sbILibraryManager).mainLibrary;
    var artistPropArray =
          Cc['@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1']
            .createInstance(Ci.sbIPropertyArray);
    artistPropArray.appendProperty(SBProperties.artistName, "ignored");
    mainLib.addListener(this, false,
                        mainLib.LISTENER_FLAGS_ITEMUPDATED |
                        mainLib.LISTENER_FLAGS_ITEMADDED |
                        mainLib.LISTENER_FLAGS_BATCHBEGIN |
                        mainLib.LISTENER_FLAGS_BATCHEND |
                        mainLib.LISTENER_FLAGS_BEFOREITEMREMOVED,
                        artistPropArray);
    debugLog("startRefreshThread", "Attached ITEM UPDATED listener");

    // Observe before library manager shutdown events to clean up our library
    // usage
    this._observerSet = new ObserverSet();
    this._observerSet.add(this,
                          "songbird-library-manager-before-shutdown",
                          false,
                          true);
  },

  observe: function(subject, topic, data) {
    switch(topic) {
      case "timer-callback":
        //debugLog("observe", "Timer triggered!");
        // Get the saved country code
        var country = this.prefs.getCharPref("country");
        var firstrun = this.prefs.getBoolPref("firstrun");
        if (!firstrun)
          this.refreshNewReleases(true, country);
        break;

      case "songbird-library-manager-before-shutdown":
        // Remove our library listener
        var mainLib = Cc['@songbirdnest.com/Songbird/library/Manager;1']
                        .getService(Ci.sbILibraryManager).mainLibrary;
        mainLib.removeListener(this);

        // Remove all observers.
        this._observerSet.removeAll();
        this._observerSet = null;

        break;

      default:
        break;
    }
  },

  /*********************************************************************
   * Routine fired for when media is added to the main library.  We need
   * to do this so we can update its new release status (if we already have
   * new release data)
   *********************************************************************/
  onItemUpdated : function(list, item, index) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return true;
    var artist = item.getProperty(SBProperties.artistName);
    var url = this.getArtistUrlIfHasRelease(artist);
    if (url != null) {
      debugLog("onItemUpdated", "New new release item, artist: " + artist);
      // Set the new release properties for this track
      item.setProperty(this.newReleaseImgProperty, newReleaseIconSrc);
      item.setProperty(this.newReleaseUrlProperty, url);
      debugLog("onItemUpdated", "New release status set to true");

      // Set the new release this artist is playing at to be flagged
      // as "library artists"
      if (this._batch.isActive()) {
        // We're in batch mode!
        debugLog("onItemUpdated", "Adding " + artist + " to the batch");
        this._batchArtistsAdded[artist] = 1;
      } else {
        // Running on a single item:
        this._db.resetQuery();
        this._db.addQuery("UPDATE playing_at " +
                          "SET libraryArtist=1 " +
                          "WHERE artistid = '" +
                          escape(artist).replace(/'/g, "''") + "'");
        this._db.execute();
        this.spsUpdater();
      }
    }
  },

  onItemAdded : function(list, item, index) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return true;
    var artist = item.getProperty(SBProperties.artistName);
    if (artist != null) {
      //debugLog("onItemAdded", "New media item, artist: " + artist);
      this.onItemUpdated(list, item, index);
    }
  },

  onBatchBegin : function(list) {
    debugLog("onBatchBegin", "Running in batch mode...");
    if (!this._batch.isActive()) {
      this._batchArtistsAdded = {};
      this._batchArtistsRemoved = {};
    }
    this._batch.begin();
  },

  onBatchEnd : function(list) {
    this._batch.end();
    if (!this._batch.isActive()) {
      // For batch addition
      this._db.resetQuery();
      var artistsAdded = false;
      for (artist in this._batchArtistsAdded) {
        this._db.addQuery("UPDATE playing_at " +
                          "SET libraryArtist=1 " +
                          "WHERE artistid = '" +
                          escape(artist).replace(/'/g, "''") + "'");
        debugLog("onBatchEnd", "Adding update query for " + artist);
        artistsAdded = true;
      }
      delete this._batchArtistsAdded;
      if (artistsAdded) {
        this._db.execute();
        this.spsUpdater();
      }

      // For batch removal
      for (artist in this._batchArtistsRemoved) {
        debugLog("onBatchEnd", "Reseting for " + artist);
        this.resetReleaseDataForArtist(list, artist);
      }
      delete this._batchArtistsRemoved;

      debugLog("onBatchEnd", "Done");
    }
  },

  resetReleaseDataForArtist : function(list, artist) {
    debugLog("resetReleaseDataForArtist", "init");
    var otherTracks = true;
    var arr;
    try {
      arr = list.getItemsByProperty(SBProperties.artistName, artist);
    } catch (e) {
    }
    if (typeof(arr) == "undefined")
      otherTracks = false;
    debugLog("resetReleaseDataForArtist", "Artist: " + artist
      //+ " -- track: " + item.getProperty(SBProperties.trackName)
      //+ " -- tracks left: " + arr.length
      //+ " -- New Release:" + item.getProperty(this.newReleaseImgProperty)
    );
    if (!otherTracks) {
      debugLog("resetReleaseDataForArtist", "Clearing release data for " + artist);
      // There were no other tracks, so update the SQLite DB
      // This actually turns out to be really annoying, we have to
      // first get all the releases that this artist is playing in
      // and set playing_at.libraryArtist = 0.  For each release, we
      // have to determine if this artist was the *only* library
      // artist there.  If so, then we also need to set
      // anyLibraryArtist = 0. this is expensive, but fortunately
      // should be rare (I hope)
      //
      // First update all playing_at entries for this artist to be 0
      this._db.resetQuery();
      this._db.addQuery("UPDATE playing_at " +
                        "SET libraryArtist=0 " +
                        "WHERE artistid = '" +
                        escape(artist).replace(/'/g, "''") + "'");
      this._db.execute();
      this.spsUpdater();
    }
  },

  /*********************************************************************
   * Routine fired for when media is removed from the main library.  We need
   * to do this so we can update its new release status (if we already have
   * new release data)
   *********************************************************************/
  onBeforeItemRemoved : function(list, item, index) {
    // If this artist doesn't have new release, then we can quit now
    var itemNewRelease = item.getProperty(this.newReleaseImgProperty);
    if (itemNewRelease == null || itemNewRelease == noReleaseIconSrc)
      return false;

    var artist = item.getProperty(SBProperties.artistName);
    if (artist == null)
      return false;

    // Need to see if there are any other tracks by this artist in the
    // library.  If so, then we bail out.  If not, then this was the last
    // track - and we should update the database accordingly
    if (this._batch.isActive()) {
      debugLog("onBeforeItemRemoved", "Track by " + artist + " removed");
      this._batchArtistsRemoved[artist] = 1;
    } else {
      this.resetReleaseDataForArtist(list, artist);
    }
    return false;
  },

  /*********************************************************************
   * Initiates a releases data refresh for a given country.  This handles
   * the logic of checking to ensure updates aren't triggered too often,
   * or that multiple refreshes aren't occuring simultaneously.
   *********************************************************************/
  refreshNewReleases : function(async, country) {
    // Only allow asynchronous refreshes of the releases data every 48
    // hours, or if the forcerefresh pref is set
    var now = Date.now()/1000;
    var lastUpdated = 0;
    if (this.prefs.prefHasUserValue("lastupdated"))
      lastUpdated = this.prefs.getIntPref("lastupdated");
    var forceRefresh = false;
    if (this.prefs.prefHasUserValue("forcerefresh"))
      forceRefresh = this.prefs.getBoolPref("forcerefresh");
    if (async && (now - lastUpdated < 172800) && !forceRefresh) {
      debugLog("refreshNewReleases", "Refresh period is once/48 hrs");
      return false;
    }

    // Don't refresh if a refresh is already running
    if (this.newReleasesRefreshRunning) {
      debugLog("refreshNewReleases", "New releases refresh already running!");
      return false;
    }
    this.newReleasesRefreshRunning = true;
    debugLog("refreshNewReleases", "NEW RELEASES REFRESH RUNNING:" +
                                   this.newReleasesRefreshRunning);

    this._cache = {}; // Flush any cached data, since it will be invalid

    this.progress(this.strDownloading, 5);

    var now = new Date();
    now.setDate(15); //set to middle-of-month

    var pastMonthsToGet = 0;
/*
    // Other areas must also be cleaned up for this to work
    // properly including timestamp checks

    // Number of months of past releases to get
    pastMonthsToGet = this.prefs.getIntPref("pastmonths");
    if (pastMonthsToGet != null && pastMonthsToGet > 0) {
      // For each month of past results to get, set
      // the time back a month
      now.setTime(now.getTime() - monthInMillis*pastMonthsToGet);
    }
*/

    var monthsToGet = this.prefs.getIntPref("futuremonths");
    var masterList = new Array();

    try
    {
      for (var i = 0-pastMonthsToGet; i < monthsToGet; i++){
        //build date string
        var year = now.getFullYear();
        var month = 0;
        if (now.getMonth() < 9) month = "0" + (now.getMonth()+1);
        else month = (now.getMonth()+1);

        //get releases for that month
        var releases = this.getReleasesForTimePeriod(year, month);
        masterList.push(releases);

        //advance month by adding 32 days
        now.setTime(now.getTime() + monthInMillis);
      }

      this.processLocations(masterList);
      this.processNewReleases(async, masterList);
    }
    catch(e)
    {
      dump("Whoops! " + e.message + "\n");
      this.newReleasesRefreshRunning = false;
      this.prefs.setBoolPref("networkfailure", true);
      if (!async)
        this.displayCallback.timeoutError();
    }

    return true;
  },

  getReleasesForTimePeriod: function(year, month){
    var date = year + "-" + month + "-**";
    var releaseList = new Array();
    var offset = 0;
    var releasesCount = 0;
    var offsetInterval = 100;

    try
    {
      while (offset <= releasesCount)
      {
        var url = "http://musicbrainz.org/ws/1/release/?type=xml&limit=100&date=" +
                  date + "&offset=" + offset;
        var refreshThread = new refreshNewReleasesThread(this);
        var response = refreshThread.startRefresh(url);

        if (response != null)
        {
          var x = response.replace(
                    /<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
                    "");
          var xmlText = new XML(x);

          if (xmlText..mbns::['release-list'] != null &&
              xmlText..mbns::['release-list'].length() != 0)
          {
            // This really shouldn't change except for the first pass
            releasesCount = parseInt(xmlText..mbns::['release-list'][0].@count);
          }

          releaseList.push(response);
        }
        offset += offsetInterval;
      }
    }
    catch (e)
    {
      throw(e);
    }

    return releaseList;
  },

  /*********************************************************************
   * Triggered by refreshNewReleases upon retrieving all results.
   * processNewReleases is passed a boolean indicating whether it should
   * process asynchronously or not (if not, then it should update the UI
   * using the displayCallback).  It's also passed an array containing
   * all the retrieved results in XML format chopped in 100-record
   * increments.
   *********************************************************************/
  processNewReleases : function(async, masterList) {
    debugLog("processNewReleases","Dispatching generator to process xmlData");
    pseudoThread(this.processNewReleasesGenerator(async, masterList));
  },
  processNewReleasesGenerator : function(async, masterList) {
    debugLog("processNewReleases", "Initialised and created");

    // Get the library artist names into an array
    var mainLib = LibraryUtils.mainLibrary;
    var enumerator = mainLib.getDistinctValuesForProperty(
                               SBProperties.artistName);
    var libraryArtists = new Array();
    while (enumerator.hasMore()) {
      var artistName = enumerator.getNext().toUpperCase();
      libraryArtists[artistName] = true;
      if (async)
        yield true;
    }
    this.progress(this.strDlComplete, 5);

    // Setup our database
    var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                .createInstance(Ci.sbIDatabaseQuery);
    dbq.databaseLocation = this.newReleasesDbURI;
    dbq.setDatabaseGUID("releases");
    dbq.setAsyncQuery(false);
    dbq.resetQuery();

    this._cache = {}; // Flush any cached data, since it will be invalid

    dbq.addQuery("DROP TABLE IF EXISTS releases");
    dbq.addQuery("DROP TABLE IF EXISTS artists");
    dbq.addQuery("DROP TABLE IF EXISTS playing_at");
    dbq.addQuery("CREATE TABLE releases (" +
                 "timestamp INTEGER, title TEXT, label TEXT, " +
                 "type TEXT, tracks TEXT, country TEXT, gcaldate TEXT)");
    // Index releases list
    dbq.addQuery("create unique index if not exists releaseID on " +
                 "releases (timestamp, title, country)");
    dbq.addQuery("CREATE TABLE IF NOT EXISTS artists (" +
                 "name TEXT COLLATE NOCASE, artistURL TEXT)");
    dbq.addQuery("CREATE TABLE IF NOT EXISTS playing_at (" +
                 "releaseid INTEGER, artistid TEXT, " +
                 "libraryArtist INTEGER)");

    dbq.addQuery("create unique index if not exists releaseID on " +
                 "artists (artistURL)");

    dbq.execute();
    dbq.resetQuery();

    debugLog("processNewReleases", "DB query setup complete");

    // Save our current timestamp to denote when it was last updated
    this.prefs.setIntPref("lastupdated", Date.now()/1000);
    this.metrics.metricsInc("newReleases", "data.refresh", "");

    if (async)
      yield true;
    // Create a second database query to do our artist table
    // insertions and selections
    var artistDbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                      .createInstance(Ci.sbIDatabaseQuery);
    artistDbq.databaseLocation = this.newReleasesDbURI;
    artistDbq.setDatabaseGUID("releases");
    artistDbq.setAsyncQuery(false);

    var dbCounter = 0;
    // used to check for uniqueness
    var artistList = new Array();
    var releaseList = new Array();
    var progressCounter = 1;

    // First loop is for each month, second loop is for each
    // chunk of records (MusicBrainz max: 100)
    for (var i = 0; i < masterList.length; i++){
      for (var j = 0; j < masterList[i].length; j++){
        // Use e4x to parse the XML data
        var xmlData = new String(masterList[i][j]);
        xmlData = xmlData.replace('<?xml version="1.0" encoding="UTF-8"?>', "");
        var x = new XML(xmlData);

        debugLog("processNewReleases", "XML parsing complete");

        // This may not be the best marker for number of releases
        var releasesLength = x..mbns::release.length();
        this.progress(this.strNewReleasesFound + " " +
                      this.releasesTotal + " " +
                      this.strNewReleasesFoundP, 10);

        if (async)
          yield true;

        // Iterate over each release and add them to the database
        debugLog("processNewReleases", "Beginning releases loop");
        for (var k = 0; k<releasesLength; k++) {
          var release = x..mbns::release[k];
          this.progress(this.strProcessing + " " + (1+parseInt(progressCounter)) +
                        " " + this.strOf + " " + this.releasesTotal,
                        10+parseInt((progressCounter/this.releasesTotal)*90));
          yield true;

          // Artist
          var artist = release..mbns::artist[0].mbns::name.toString();
          if (artist == null || artist == "" || artist.length <= 0)
            artist = this.strUnknown;

          // Album title
          var title = release..mbns::title.toString();
          if (title == null || title == "" || title.length <= 0)
            title = this.strUnknown;

          // Release type
          var releaseType = release.@type.toString();
          if (releaseType == null || releaseType == "" || releaseType.length <= 0)
            releaseType = this.strUnknown;

          // Track count
          var trackCount = release..mbns::['track-list'].@count;
          if (trackCount == null || trackCount == "" || trackCount.length() <= 0)
            trackCount = this.strUnknown;

          if (async)
            yield true;

          // A single release can have multiple "release events"
          eventNodes = release..mbns::['release-event-list'][0].mbns::event;
          var releaseEvents = new Array();
          oldCountry = null;
          oldDate = null;

          // We make the assumption that there are no dupes
          for (var l = 0; l < eventNodes.length(); l++)
          {
            progressCounter++;
            curEventNode = eventNodes[l];

            // Label
            if (curEventNode..mbns::label.length() > 0)
              label = curEventNode..mbns::label[0].mbns::name[0].toString();
            if (label == null || label == "" || label.length <= 0)
              label = this.strUnknown;
            label = label.replace(/"/g, "'");

            // Country
            country = curEventNode.@country.toString();
            if (country == null || country == "" || country.length <= 0)
              country = this.strUnknown;

            // Event date
            var tmpDate = curEventNode.@date.toString();
            if (tmpDate == null || tmpDate == "" || tmpDate.length <= 0)
              continue;

            // Sometimes the service returns invalid dates
            var datePattern = /\d{4}\-\d{2}\-\d{2}/;
            if (!(tmpDate.match(datePattern))) continue;

            // Get ready for some date-fun
            eventDate = new Date(tmpDate.replace(/-/g, "/"));

            // GCal date
            releaseDate = tmpDate.replace(/-/g, "/");
            var gcalDate = eventDate.toLocaleFormat("%Y%m%d");

            //check that we dont have this event already
            //duplicates caused by different barcodes (which we don't read)
            // This assumes that the events are ordered somehow
            if (country == oldCountry && gcalDate == oldDate) continue;

            // Ensure that release is in the future before
            // adding to DB
            var now = new Date();
            var timestamp = eventDate.getTime();

            if (timestamp > now.getTime())
            {
              // Valid release event, track the artist
              if (typeof(artistList[escape(artist)]) == "undefined")
              {
                // Update artists DB
                artistDbq.resetQuery();
                var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=1&sql=" +
                          escape(artist);
                var query = "INSERT OR IGNORE INTO artists VALUES (" +
                            "'" + escape(artist) + "', '" + url + "')";
                artistDbq.addQuery(query);
                artistDbq.execute();
                artistList[escape(artist)] = 1;
              }

              // Check if the artist is in the user's library
              var thisLibArtist = 0;
              if (typeof(libraryArtists[artist.toUpperCase()]) != "undefined")
                thisLibArtist = 1;

              var query = 'INSERT OR IGNORE INTO releases VALUES ( "' +
                          timestamp + '", "' +
                          escape(title) + '", "' +
                          label + '", "' +
                          releaseType + '", "' +
                          trackCount + '", "' +
                          country + '", "' +
                          gcalDate + '" )';
              dbq.addQuery(query);
              dbq.addQuery("SELECT ROWID FROM releases WHERE timestamp=" +
                           '"' + timestamp + '"' +
                           " AND title=" + '"' + escape(title) + '"' +
                           " AND country=" + '"' + country + '"');
              dbq.execute();
              var result = dbq.getResultObject();
              var releaseId = result.getRowCell(0,0);

              releaseEvents[releaseId] = 1;

              oldCountry = country;
              oldDate = gcalDate;

              if (async)
                yield true;
            }

            for (releaseId in releaseEvents)
            {
              if (typeof(releaseList[releaseId]) == "undefined")
              {
                dbq.addQuery("INSERT OR IGNORE INTO playing_at VALUES ('" +
                             releaseId + "', '" + escape(artist) + "', '" +
                             thisLibArtist + "')");
                releaseList[releaseId] = escape(artist);
              }
              //else if (releaseList[releaseId] != escape(artist))
              //{
                // Somehow a release with the same release date,
                // name, and country is already attributed to
                // another artist.  What should we do here?
              //}
            }

            // Only run the actual DB insert every 20 new releases
            if (!(dbCounter % 20) && dbq.getQueryCount() > 0) {
              debugLog("processNewReleases", "async: inserting into DB now");
              dbq.execute();
              dbq.resetQuery();
              yield true;
            }

            dbCounter++; //increment our id
          }
        }
      }
    }

    this.progress(this.strFinishing, 100);
    yield true;
    if (dbq.getQueryCount() > 0) {
      debugLog("processNewReleases", "finishing up DB insertions\n");
      dbq.execute();
      dbq.resetQuery();
    }

    debugLog("processNewReleases", "end release loop");

    this.progress(this.strComplete, 100);
    yield true;

    // We've got all our results now, so if we're running synchronously,
    // we can go ahead and switch the display back to the browse listing
    // while we go ahead and update the library items in the background
    if (!async) {
      if (this.displayCallback != null)
        this.displayCallback.showListings();
    }

    // No longer refreshing the DB, clear the lock
    debugLog("processNewReleases", "Done, clearing the lock");
    this.newReleasesRefreshRunning = false;
    debugLog("refreshNewReleases", "NEW RELEASES REFRESH RUNNING:" +
                                   this.newReleasesRefreshRunning);

    // Update the release count in the service pane
    this.spsUpdater();
    yield true;

    // Enumerate the user's library for each track that has the
    // new release status flag set, and clear it
    debugLog("processNewReleases", "clearing library new release properties");
    try {
      var itemEnum = mainLib.getItemsByProperty(this.newReleaseImgProperty,
                                                newReleaseIconSrc).enumerate();
      while (itemEnum.hasMoreElements()) {
        var item = itemEnum.getNext();
        item.setProperty(this.newReleaseImgProperty, noReleaseIconSrc);
        yield true;
      }
    } catch (e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // ignore, just meant there were no items
    }

    // Now for each artist in the artistList array, update the
    // new release status flag for all tracks in the library that match
    // the artist
    debugLog("processNewReleases", "setting new library new release properties");
    for (artist in artistList) {
      var artistName = unescape(artist).toUpperCase();
      try {
        var itemEnum = mainLib.getItemsByProperty(
                                 SBProperties.artistName, artistName).enumerate();
        while (itemEnum.hasMoreElements()) {
           var item = itemEnum.getNext();
          /*
          // Handy to have, but commenting out to avoid unnecessary
          // getProperty() calls
          debugLog("processNewReleases", "Track: " +
                   item.getProperty(SBProperties.trackName) +
                   " by " + artistName + " has new release");
          */
          item.setProperty(this.newReleaseImgProperty, newReleaseIconSrc);
          // Need an artist URL here but not automatically
          // available through MusicBrainz
          var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=1&sql=" +
                    escape(artistName);
          item.setProperty(this.newReleaseUrlProperty, url);
          yield true;
        }
      } catch (e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
        // ignore, just meant there were no items
      }
    }

    // Go rebuild the artists new release smart playlist
    try {
      var itemEnum = LibraryUtils.mainLibrary.getItemsByProperty(
                       SBProperties.customType,
                       "newreleases_artistsReleases").enumerate();
      while (itemEnum.hasMoreElements()) {
        var list = itemEnum.getNext();
        list.rebuild();
      }
    } catch (e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // ignore, just meant there were no items
    }

    throw StopIteration;
  },

  /*********************************************************************
   * Register a display callback which processNewReleases can use to trigger
   * UI updates for a synchronous process
   *********************************************************************/
  hasDisplayCallback : function() { return (this.displayCallback != null); },
  registerDisplayCallback : function(fn) {
    this.displayCallback = fn.wrappedJSObject;
    debugLog("registerDisplayCallback", "Display callback registered");
  },
  registerSPSUpdater : function(fn) {
    this.spsUpdater = fn.wrappedJSObject;
    debugLog("registerSPSUpdater", "Servicepane updater registered");
  },
  unregisterDisplayCallback : function() {
    debugLog("unregisterDisplayCallback", "Removing display callback");
    if (this.displayCallback != null)
      this.displayCallback = null;
  },
  unregisterSPSUpdater : function() {
    debugLog("unregisterSPSUpdater", "Removing Servicepane updater");
    if (this.spsUpdater != null)
      this.spsUpdater = null;
  },

  /*********************************************************************
   * Returns JSON encoding of countries, states, and cities in the
   * database.
   *********************************************************************/
  getLocationCountries : function() {
    var countries = new Array();

    // Always ensure that the ALL entry is at the top
    this._db.resetQuery();
    this._db.addQuery("SELECT * FROM countries WHERE id = '" + allEntriesID + "'");
    var ret = this._db.execute();
    if (ret == 0)
    {
      var result = this._db.getResultObject();
      var name = result.getRowCellByColumn(0, "name");
      countries.push({"id": allEntriesID, "name": name});
    }

    // ... now get the rest
    this._db.resetQuery();
    this._db.addQuery("SELECT * FROM countries ORDER BY name");
    ret = this._db.execute();

    if (ret == 0) {
      var result = this._db.getResultObject();
      for (var i=0; i<result.getRowCount(); i++) {
        var id = result.getRowCellByColumn(i, "id");
        var name = result.getRowCellByColumn(i, "name");
        if (id != allEntriesID)
          countries.push({"id": id, "name": name});
      }
    }
    return this.json.encode(countries);
  },

  /*********************************************************************
   * Returns the country name
   *********************************************************************/
  getLocationString : function(country) {
    this._db.resetQuery();
    var query = "SELECT countries.name FROM countries " +
                "WHERE countries.id= '" + country + "'";
    this._db.addQuery(query);
    var ret = this._db.execute();
    if (ret != 0)
      return ("Error!" + ret);
    var result = this._db.getResultObject();
    if (result.getRowCount() > 0) {
      return result.getRowCell(0,0);
    } else {
      return ("Error! Rowcount:" + result.getRowCount());
    }
  },

  /*********************************************************************
   * Returns the number of new releases in the database.  If
   * filterLibraryArtists is true, then it will only return the number
   * of new releases that have artists in the user's main library.  The
   * total is independent of the selected location.
   *********************************************************************/
  getNewReleasesCount : function(filterLibraryArtists) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return (0);

    this._db.resetQuery();
    try {
      if (filterLibraryArtists) {
        this._db.addQuery("SELECT count(distinct releaseid) " +
                          "from playing_at where libraryArtist=1");
      }
      else {
        this._db.addQuery("SELECT count(distinct releaseid) " +
                          "from playing_at");
      }

      var ret = this._db.execute();
    } catch (e) {
      return (0);
    }
    var result = this._db.getResultObject();
    var count = result.getRowCell(0, 0);
    return (count);
  },

  /*********************************************************************
   * For a given artist name, return whether the artist has release or not
   *********************************************************************/
  getReleaseStatus : memoize(function getReleaseStatus(artist) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return false;
    this._db.resetQuery();
    var query = 'SELECT count(*) FROM playing_at ' +
                ' JOIN releases on releaseid=releases.ROWID ' +
                " WHERE playing_at.artistid = '" +
                escape(artist).replace(/'/g, "''") + "'" +
                ' AND releases.timestamp > ' + parseInt(Date.now()/1000);
    this._db.addQuery(query);
    var ret = this._db.execute();
    var result = this._db.getResultObject();
    var count = result.getRowCell(0, 0);
    return (count > 0);
  }),

  /*********************************************************************
   * For a given artist name, return their New Release URL
   *********************************************************************/
  getArtistUrl : memoize(function getArtistUrl(artist) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return null;
    this._db.resetQuery();
    this._db.addQuery("SELECT artistURL FROM artists " +
                      "where name = '" + artist.replace(/'/g, "''") + "'");
    var ret = this._db.execute();
    var result = this._db.getResultObject();
    if (result.getRowCount() > 0) {
      debugLog("getArtistUrl", "Artist URL found");
      var url = result.getRowCellByColumn(0, "artistURL");
      return (url);
    } else {
      return null;
    }
  }),

  getArtistReleaseUrl : function(artist) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return null;
    this._db.resetQuery();
    this._db.addQuery("SELECT artistURL FROM artists " +
                      "where name = '" + artist.replace(/'/g, "''") + "'");
    var ret = this._db.execute();
    var result = this._db.getResultObject();
    if (result.getRowCount() > 0) {
      debugLog("getArtistUrl", "Artist URL found");
      var url = result.getRowCellByColumn(0, "artistURL");
      return (url);
    } else {
      return null;
    }
  },

  /*********************************************************************
   * For a given artist name, return the release URL if they have release
   *********************************************************************/
  getArtistUrlIfHasRelease : memoize(function getArtistUrlIfHasRelease(artist) {
    var firstrun = this.prefs.getBoolPref("firstrun");
    if (firstrun)
      return null;
    //debugLog("getArtistUrlIfHasRelease", "New media item, artist: " + artist);
    this._db.resetQuery();
    this._db.addQuery("SELECT artistURL FROM artists " +
                      ' JOIN playing_at on playing_at.artistid = artists.name ' +
                      ' JOIN releases on playing_at.releaseid=releases.ROWID ' +
                      " WHERE artists.name = '" +
                      escape(artist).replace(/'/g, "'") + "'" +
                      ' AND releases.timestamp > ' + parseInt(Date.now()/1000));
    var ret = this._db.execute();
    var result = this._db.getResultObject();
    if (result.getRowCount() > 0) {
      debugLog("getArtistUrlIfHasRelease", "Artist URL found");
      var url = result.getRowCellByColumn(0, "artistURL");
      return (url);
    } else {
      return null;
    }
  }),

  // The following isn't currently used for anything but is saved in case
  // a provider/URL is ever required
  /*********************************************************************
   * Returns the URL for the provider's homepage
   *********************************************************************/
  providerURL : function() {
    return ("http://www.allmusic.com");
  },

  /*********************************************************************
   * Updates the location data, this checks to make sure we don't update
   * more than once per week.  It then sets up the channel and fires it
   * off.
   *********************************************************************/
  refreshLocations : function() {
    debugLog("refreshLocations", "Triggered");

    // Only allow asynchronous refreshes of the location data once
    // per week
    var now = parseInt(Date.now()/1000);
    var lastUpdated = 0;
    if (this.prefs.prefHasUserValue("locations.lastupdated"))
      lastUpdated = this.prefs.getIntPref("locations.lastupdated");
    var forceRefresh = false;
    if (this.prefs.prefHasUserValue("locations.forcerefresh"))
      forceRefresh = this.prefs.getBoolPref("locations.forcerefresh");
    if (!this.gotLocationInfo())
      forceRefresh = true;
    if ((now - lastUpdated < 604800) && !forceRefresh) {
      debugLog("refreshLocations", "Don't refresh >1/week");
      return;
    }

    // Don't refresh if a refresh is already running
    if (this.locationRefreshRunning) {
      debugLog("refreshLocations", "Refresh already running");
      return;
    }
    this.locationRefreshRunning = true;
    debugLog("refreshLocations", "Refresh initiated");

    // Update the last-updated time
    this.prefs.setIntPref("locations.lastupdated", Date.now()/1000);
    debugLog("refreshLocations", "Updating last updated time");

    var now = new Date();
    now.setDate(15); //set to middle-of-month

    var pastMonthsToGet = 0;
/*
    // Other areas must also be cleaned up for this to work
    // properly including timestamp checks

    // Number of months of past releases to get
    pastMonthsToGet = this.prefs.getIntPref("pastmonths");
    if (pastMonthsToGet != null && pastMonthsToGet > 0) {
      // For each month of past results to get, set
      // the time back a month
      now.setTime(now.getTime() - monthInMillis*pastMonthsToGet);
    }
*/

    var monthsToGet = this.prefs.getIntPref("futuremonths");
    var masterList = new Array();
    try
    {
      for (var i = 0-pastMonthsToGet; i < monthsToGet; i++){
        //build date string
        var year = now.getFullYear();
        var month = 0;
        if (now.getMonth() < 9) month = "0" + (now.getMonth()+1);
        else month = (now.getMonth()+1);

        //get releases for that month
        var releases = this.getReleasesForTimePeriod(year, month);
        masterList.push(releases);

        //advance month by adding 32 days
        now.setTime(now.getTime() + monthInMillis);
      }

      this.processLocations(masterList);
    }
    catch(e)
    {
      dump("Whoops! " + e.message + "\n");
      this.newReleasesRefreshRunning = false;
      this.prefs.setBoolPref("networkfailure", true);
      this.displayCallback.timeoutError();
    }
  },

  processLocations: function(masterList) {
    debugLog("processLocations", "Processing location XML data");
    var eventCounter = 0;

    // Translate location XML data to store it in our DB
    this._db.resetQuery();
    this._db.addQuery("DROP TABLE IF EXISTS countries");
    this._db.addQuery("CREATE TABLE countries (id TEXT, name TEXT)");
    this._db.addQuery("INSERT OR IGNORE INTO countries VALUES ('" +
                      allEntriesID + "', '" +
                      this.getCountryName(allEntriesID) + "')");

    var countryList = new Array();

    // First loop is for each month, second loop is for each
    // chunk of records (MusicBrainz max: 100)
    for (var i = 0; i < masterList.length; i++){
      for (var j = 0; j < masterList[i].length; j++){
        var xmlData = new String(masterList[i][j]);
        xmlData = xmlData.replace('<?xml version="1.0" encoding="UTF-8"?>', "");
        var x = new XML(xmlData);

        var countryCount = x..mbns::event.length();
        debugLog("processLocations", "# countries:" + countryCount);

        // A single release can have multiple "release events"
        // so check each one
        for (var k=0; k<countryCount; k++) {
          eventCounter++;
          var country = x..mbns::event[k].@country;
          var eventDate = x..mbns::event[k].@date.toString();
          if (eventDate == null || eventDate == "" || eventDate.length <= 0)
            continue;
          var releaseDate = new Date(eventDate.replace(/-/g, "/"));
          var now = new Date();

          // Only add country if it hasn't already been added
          // and the event is in the future.  Countries are per-
          // event
          if ((typeof(countryList[country]) == "undefined") &&
              (releaseDate.getTime() > now.getTime()))
          {
            var query = "INSERT OR IGNORE INTO countries VALUES " +
                        "('" + country + "', '" +
                        this.getCountryName(country.toString()) + "')";
            this._db.addQuery(query);
            countryList[country] = 1;
          }
        }
      }
    }
    this.releasesTotal = eventCounter;
    var ret = this._db.execute();
    this.locationRefreshRunning = false;
    debugLog("processLocations", "Location data processed.");
  },

  gotLocationInfo : function() {
    var result = null;
    var count = 0;

    try {
      // First check if the table exists
      this._db.resetQuery();
      this._db.addQuery("SELECT count(*) FROM sqlite_master WHERE name='countries'");
      this._db.execute();
      result = this._db.getResultObject();
      count = result.getRowCell(0,0);
      if (count > 0)
      {
        // The table exists so get the country count
        this._db.addQuery("SELECT count(*) from countries");
        this._db.execute();
      }
      else
      {
        debugLog("gotLocationInfo", "Database countries does not exist");
        return false;
      }
    } catch (e) {
      debugLog("gotLocationInfo", "Error checking if 'countries' table exists");
      return false;
    }
    result = this._db.getResultObject();
    count = result.getRowCell(0,0);

    // Comparing against 1 because there'll always be at
    //  least one entry for "ALL"
    if (count <= 1)
      debugLog("gotLocationInfo", count + "No location info yet");
    return (count > 1);
  },

  progress : function(str, pct) {
    if (str != "") {
      this.progressString = str;
    }
    if (pct != null) {
      this.progressPercentage = pct;
    }

    if (typeof(this.displayCallback) == "undefined") {
      debugLog("progress", "displayCallback is undefined - register!");
      //SBDataSetStringValue("faceplate.status.text", str);
    } else {
      if (str != "" && this.displayCallback != null)
        this.displayCallback.loadingMessage(str);
      if (pct != null && this.displayCallback != null) {
        this.displayCallback.loadingPercentage(pct);
      }
    }
  },

  getCountryName : function(code) {
    return this.countryStringBundle.GetStringFromName("country" + code);
  },

  /*********************************************************************
   * Return enumerators for iterating through artists and releases
   *********************************************************************/
  artistReleaseEnumerator : function(filter, country) {
    return new skArtistReleaseEnumerator(filter, country);
  },
  releaseEnumerator : function(sort, filter, country) {
    return new skReleaseEnumerator(sort, filter, country);
  }
}

refreshNewReleasesThread = function(nrObj) {
  this.nrObj = nrObj;
  this.lookupTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this.req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
               .createInstance(Components.interfaces.nsIXMLHttpRequest);
}

refreshNewReleasesThread.prototype = {
  observe : function foo(subject, topic, data) {
    if ((topic == 'timer-callback') && (subject == this.lookupTimer)) {
      if (this.req.readyState != 0 && this.req.readyState != 4)
      {
        dump("Lookup is still going ...\n");
        this.req.onreadystatechange = function () {};
        this.req.abort();

        // Revert to the download problem page
        this.nrObj.newReleasesRefreshRunning = false;
        this.nrObj.prefs.setBoolPref("networkfailure", true);
        this.nrObj.displayCallback.timeoutError();
      }
    }
  },

  startRefresh : function(url) {
    try
    {
      this.req.open("GET", url, false);
      this.lookupTimer.init(this, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      this.req.send(null);
      this.nrObj.prefs.setBoolPref("networkfailure", false);
      this.lookupTimer.cancel();
      return this.req.responseText;
    }
    catch (e)
    {
      this.lookupTimer.cancel();
      this.nrObj.prefs.setBoolPref("networkfailure", true);
      throw(e);
    }
  },
}

/**************************************************************************
 * Enumerator implementing the nsISimpleEnumerator interface to enumerate
 * over all the artists we know about that currently have a new release
 **************************************************************************/
skArtistReleaseEnumerator = function(filter, country) {
  this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
               .createInstance(Ci.sbIDatabaseQuery);
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);
  var dbdir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

  var uri = ios.newFileURI(dbdir);
  this._db.databaseLocation = uri;
  this._db.setDatabaseGUID("releases");

  this._artistDb = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                     .createInstance(Ci.sbIDatabaseQuery);
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);
  var dbdir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

  var uri = ios.newFileURI(dbdir);
  this._artistDb.databaseLocation = uri;
  this._artistDb.setDatabaseGUID("releases");

  this._db.resetQuery();
  var query = "SELECT * FROM playing_at" +
              " JOIN artists ON playing_at.artistid = artists.name" +
              " JOIN releases ON playing_at.releaseid = releases.ROWID";

  if (country != allEntriesID)
    query += " WHERE releases.country = '" + country + "'";

  if (filter)
  {
    if (country != allEntriesID)
      query += " AND playing_at.libraryArtist = 1";
    else
      query += " WHERE playing_at.libraryArtist = 1";
  }

  query += " ORDER BY artists.name COLLATE NOCASE, releases.timestamp, releases.title COLLATE NOCASE";
  this._db.addQuery(query);
  var ret = this._db.execute();
  if (ret == 0) {
    this._results = this._db.getResultObject();
    this._rowIndex = 0;
  } else {
    this._results = null;
  }

  this.countryStringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                               .getService(Ci.nsIStringBundleService)
                               .createBundle("chrome://newreleases/locale/country.properties");
}
skArtistReleaseEnumerator.prototype = {
  hasMoreElements : function() {
    if (this._results == null)
      return false;
    return (this._rowIndex < this._results.getRowCount());
  },

  // nsISimpleEnumerator
  getNext : function() {
    if (!(this.hasMoreElements())) {
      throw Components.results.NS_ERROR_FAILURE;
    }

    var artistname = this._results.getRowCellByColumn(this._rowIndex, "name");
    var id = this._results.getRowCellByColumn(this._rowIndex, "releaseid");
    var ts = this._results.getRowCellByColumn(this._rowIndex, "timestamp");
    var title = this._results.getRowCellByColumn(this._rowIndex, "title");
    var label = this._results.getRowCellByColumn(this._rowIndex, "label");
    var type = this._results.getRowCellByColumn(this._rowIndex, "type");
    var tracks = this._results.getRowCellByColumn(this._rowIndex, "tracks");
    var country = this._results.getRowCellByColumn(this._rowIndex, "country");
    var gcaldate = this._results.getRowCellByColumn(this._rowIndex, "gcaldate");

    this._rowIndex++;

    var item = {
      artistname: unescape(artistname),
      id: id,
      ts: ts,
      title: unescape(title),
      label: label,
      type: type,
      tracks: tracks,
      country: this.countryStringBundle.GetStringFromName("country" + country),
      gcaldate: gcaldate
    }

    item.wrappedJSObject = item;
    return item;
  },

  // nsISimpleEnumerator
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
}

/**************************************************************************
 * Enumerator implementing the nsISimpleEnumerator interface to enumerate
 * over all the releases we know about
 **************************************************************************/
skReleaseEnumerator = function(sort, filter, country) {
  this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
               .createInstance(Ci.sbIDatabaseQuery);
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);
  var dbdir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

  var uri = ios.newFileURI(dbdir);
  this._db.databaseLocation = uri;
  this._db.setDatabaseGUID("releases");

  this._artistDb = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                     .createInstance(Ci.sbIDatabaseQuery);
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);
  var dbdir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

  var uri = ios.newFileURI(dbdir);
  this._artistDb.databaseLocation = uri;
  this._artistDb.setDatabaseGUID("releases");

  var sortBy;
  switch (sort) {
    case "date":
      sortBy="timestamp";
      break;
    case "venue":
      sortBy="venue";
      break;
    case "title":
    default:
      sortBy="title";
      break;
  }

  this._db.resetQuery();
  var query = "SELECT * FROM playing_at" +
              " JOIN artists ON playing_at.artistid = artists.name" +
              " JOIN releases ON playing_at.releaseid = releases.ROWID";

  if (country != allEntriesID)
    query += " WHERE releases.country = '" + country + "'";

  if (filter)
  {
    if (country != allEntriesID)
      query += " AND playing_at.libraryArtist = 1";
    else
      query += " WHERE playing_at.libraryArtist = 1";
  }

  query += " ORDER BY " + sortBy + ", artists.name COLLATE NOCASE, releases.title COLLATE NOCASE";
  this._db.addQuery(query);
  var ret = this._db.execute();
  if (ret != 0) {
    throw Components.results.NS_ERROR_FAILURE;
  }

  if (ret == 0) {
    this._results = this._db.getResultObject();
    this._rowIndex = 0;
  } else {
    this._results = null;
  }

  this.countryStringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                               .getService(Ci.nsIStringBundleService)
                               .createBundle("chrome://newreleases/locale/country.properties");
}

skReleaseEnumerator.prototype = {
  // nsISimpleEnumerator
  hasMoreElements: function hasMoreElements() {
    if (this._results == null)
      return false;
    return (this._rowIndex < this._results.getRowCount());
  },

  // nsISimpleEnumerator
  getNext: function getNext() {
    if (!(this.hasMoreElements())) {
      throw Components.results.NS_ERROR_FAILURE;
    }

    var artistname = this._results.getRowCellByColumn(this._rowIndex, "name");
    var id = this._results.getRowCellByColumn(this._rowIndex, "releaseid");
    var ts = this._results.getRowCellByColumn(this._rowIndex, "timestamp");
    var title = this._results.getRowCellByColumn(this._rowIndex, "title");
    var label = this._results.getRowCellByColumn(this._rowIndex, "label");
    var type = this._results.getRowCellByColumn(this._rowIndex, "type");
    var tracks = this._results.getRowCellByColumn(this._rowIndex, "tracks");
    var country = this._results.getRowCellByColumn(this._rowIndex, "country");
    var gcaldate = this._results.getRowCellByColumn(this._rowIndex, "gcaldate");

    this._rowIndex++;

    var item = {
      artistname: unescape(artistname),
      id: id,
      ts: ts,
      title: unescape(title),
      label: label,
      type: type,
      tracks: tracks,
      country: this.countryStringBundle.GetStringFromName("country" + country),
      gcaldate: gcaldate};
    item.wrappedJSObject = item;
    return item;
  },

  // nsISimpleEnumerator
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
}

var components = [NewReleases];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([NewReleases]);
}

