Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "Songkick Concerts Provider XPCOM Component";
const CID         = "{33268520-4d39-11dd-ae16-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/Songbird/Concerts/Songkick;1";

// Songbird's Songkick API key - unique to Songbird, don't use anywhere else
const SongkickAPIKey = "BBb1rqkkzs5Jovfz";

// Testing flags
const SHORT_TIMER = true;

const onTourIconSrc = "chrome://concerts/skin/icon-ticket.png";
const noTourIconSrc = "";

var concertExtensionPrefs = Cc["@mozilla.org/preferences-service;1"]
		.getService(Ci.nsIPrefService).getBranch("extensions.concerts.");

function debugLog(funcName, str) {
	var debug = concertExtensionPrefs.getBoolPref("debug");
	if (debug)
		dump("*** Songkick.js::" + funcName + " // " + str + "\n");
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
			//		value + "' from cache for key " + key);
			return value;
		}
		
		value = func.apply(this, arguments);
		this._cache[funcName][key] = value;
		//debugLog("memoized " + funcName, " adding '" +
		//		value + "' to cache for key " + key);
		return value;
	}
}



function skStreamListener(async, callback, skObj, city) {
	this._data = [];
	this._async = async;
	this.skSvc = Cc["@songbirdnest.com/Songbird/Concerts/Songkick;1"]
		.getService(Ci.sbISongkick);
	this.callback = callback;
	this.skObj = skObj;
	this.city = city;
}

skStreamListener.prototype = {
	onStartRequest: function(aReq, aContext) {
		this.i = 0;
	},
	onStopRequest: function(aReq, aContext, aStatusCode) {
		debugLog("skStreamListener::stop", "Job:" + this.callback +
				" // Status:" + aStatusCode + " // " + "Async:" + this._async);
		var prefService = Cc["@mozilla.org/preferences-service;1"]
				.getService(Ci.nsIPrefService);
		var prefs = prefService.getBranch("extensions.concerts.");
		if (aStatusCode == Components.results.NS_OK) {
			if (this.callback == 0) {
				// flag that we succeeded
				prefs.setBoolPref("networkfailure", false);
				this.skSvc.processConcerts(this._async, this.city,
						this._data.join(""));
			} else if (this.callback == 1)
				this.skSvc.processLocations(this._data.join(""));
		} else {
			debugLog("skStreamListener::stop", "Network failure!  " +
					"Async:" + this._async + " // Callback Registered:" +
					(this.skObj.displayCallback != null));

			// Release the locks
			if (this.callback == 0) {
				debugLog("skStreamListener::stop", "Flagging the failure");
				this.skObj.concertRefreshRunning = false;
				debugLog("refreshConcerts", "CONCERT REFRESH RUNNING:" +
						this.skObj.concertRefreshRunning);
				// flag that we had a network failure
				prefs.setBoolPref("networkfailure", true);
			} else
				this.skObj.locationRefreshRunning = false;
			
			// Show the timeout error
			if ((!this._async && this.skObj.displayCallback))
				this.skObj.displayCallback.timeoutError();
		}
	},
	onDataAvailable: function(aReq, aContext, aInputStream, aOffset, aCount) {
		var binInputStream = Cc["@mozilla.org/binaryinputstream;1"]
					.createInstance(Ci.nsIBinaryInputStream);
		binInputStream.setInputStream(aInputStream);
		this._data.push(binInputStream.readBytes(binInputStream.available()));
		binInputStream.close();
	}
};

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

// Songkick XPCOM component constructor
function Songkick() {
	// Load our string bundle & cache strings
	var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
		.getService(Ci.nsIStringBundleService)
		.createBundle("chrome://concerts/locale/songkick.properties");

	// Setup our properties
	var pMgr = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
			.getService(Ci.sbIPropertyManager);
	if (!pMgr.hasProperty(this.onTourImgProperty)) {
		debugLog("constructor", "Creating on tour image property:" +
				this.onTourImgProperty);
		var builder = Cc[
			"@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
			.createInstance(Ci.sbIImagePropertyBuilder);
		builder.propertyID = this.onTourImgProperty;
		builder.displayName = stringBundle.GetStringFromName("onTourProperty");
		builder.userEditable = false;
		builder.userViewable = true;
		var pI = builder.get();
		pMgr.addPropertyInfo(pI);
	} else {
		debugLog("constructor", "BAD: PROPERTY MANAGER ALREADY HAS " +
				"IMAGE PROPERTY REGISTERED\n");
	}
	if (!pMgr.hasProperty(this.onTourUrlProperty)) {
		debugLog("constructor", "Creating on tour URL property:" +
				this.onTourUrlProperty);
		var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
			.createInstance(Ci.sbITextPropertyInfo);
		pI.id = this.onTourUrlProperty;
		pI.displayName = "Not visible"
		pI.userEditable = false;
		pI.userViewable = false;
		pMgr.addPropertyInfo(pI);
	}

	// Register the property with the smart playlist registrar
	var registrar =
		Cc["@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1"]
			.getService(Ci.sbISmartPlaylistPropertyRegistrar);
	registrar.registerPropertyToContext("default", this.onTourImgProperty,
			50, "d");
	debugLog("constructor", "OnTourImgProperty reg'd w/ smartpls registrar");

	// Setup our database
	this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
			.createInstance(Ci.sbIDatabaseQuery);
	var ios = Cc["@mozilla.org/network/io-service;1"]
			.createInstance(Ci.nsIIOService);
	var dbdir = Cc["@mozilla.org/file/directory_service;1"]
			.createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
	this.concertDbURI = ios.newFileURI(dbdir);
	this._db.databaseLocation = this.concertDbURI;
	this._db.setDatabaseGUID("concerts");
	this._db.setAsyncQuery(false);
	this._db.resetQuery();
	this._cache = {};

	// Get our prefBranch
	var prefService = Cc["@mozilla.org/preferences-service;1"]
			.getService(Ci.nsIPrefService);
	this.prefs = prefService.getBranch("extensions.concerts.");

	this.strDownloading = stringBundle.GetStringFromName("downloading");
	this.strDlComplete = stringBundle.GetStringFromName("downloadComplete");
	this.strDlComplete = stringBundle.GetStringFromName("downloadComplete");
	this.strConcertFound = stringBundle.GetStringFromName("concertFound");
	this.strConcertFoundP = stringBundle.GetStringFromName("concertFoundP");
	this.strProcessing = stringBundle.GetStringFromName("processing");
	this.strOf = stringBundle.GetStringFromName("of");
	this.strFinishing = stringBundle.GetStringFromName("finishing");
	this.strComplete = stringBundle.GetStringFromName("complete");

	// Instantiate nsIJSON & sbIMetrics
	this.json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
	this.metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
    		.createInstance(Ci.sbIMetrics);
}

Songkick.prototype.constructor = Songkick;
Songkick.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbISongkick]),

	_db : null,
	_cache: null,
	_batch: null,
	concertRefreshRunning : false,
	locationRefreshRunning : false,
	drawingLock : false,
	displayCallback : null,
	spsUpdater : null,
	prefs : null,
	self : this,

	onTourImgProperty : "http://songbirdnest.com/data/1.0#artistOnTour",
	onTourUrlProperty : "http://songbirdnest.com/data/1.0#artistOnTourUrl",

	progressString : "",
	progressPercentage : 0,

	/*********************************************************************
	 * Sets up a timer to run repeatedly to asynchronously refresh the
	 * concert data.  The timer fires, triggering the timer-callback code
	 * which calls refreshConcerts() on the currently saved city
	 *********************************************************************/
	startRefreshThread : function() {
		debugLog("startRefreshThread", "Initialising refresh timer");
		this.refreshTimer = Cc["@mozilla.org/timer;1"]
			.createInstance(Ci.nsITimer);
		
		// If testing, once per minute - otherwise once per half hour
		var delay = SHORT_TIMER ? 60000 : 1800000;
		debugLog("startRefreshThread", "Timer running every " +
			(delay / 1000) + " seconds");
		this.refreshTimer.init(this, delay, Ci.nsITimer.TYPE_REPEATING_SLACK);

		// Fire a one-shot 5 seconds in
		debugLog("startRefreshThread", "Firing a 1-shot in 5 seconds..");
		this.initialTimer = Cc["@mozilla.org/timer;1"]
			.createInstance(Ci.nsITimer);
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
	},

	observe: function(subject, topic, data) {
		switch(topic) {
			case "timer-callback":
				//debugLog("observe", "Timer triggered!");
				// Get the saved city code
				var city = this.prefs.getIntPref("city");
				var firstrun = this.prefs.getBoolPref("firstrun");
				if (!firstrun)
					this.refreshConcerts(true, city);
		}
	},

	/*********************************************************************
	 * Routine fired for when media is added to the main library.  We need
	 * to do this so we can update its on tour status (if we already have
	 * concert data)
	 *********************************************************************/
	onItemUpdated : function(list, item, index) {
		var firstrun = this.prefs.getBoolPref("firstrun");
		if (firstrun)
			return true;
		var artist = item.getProperty(SBProperties.artistName);
		var url = this.getArtistUrlIfOnTour(artist);
		if (url != null) {
			debugLog("onItemUpdated", "New on tour item, artist: " + artist);
			// Set the touring properties for this track
			item.setProperty(this.onTourImgProperty, onTourIconSrc);
			item.setProperty(this.onTourUrlProperty, url);
			debugLog("onItemUpdated", "Touring status set to true");

			// Set the concerts this artist is playing at to be flagged
			// as "library artists"
			if (this._batch.isActive()) {
				// We're in batch mode!
				debugLog("onItemUpdated", "Adding " + artist + " to the batch");
				this._batchArtistsAdded[artist] = 1;
			} else {
				// Running on a single item:
				this._db.resetQuery();
				this._db.addQuery("UPDATE playing_at " +
						"SET libraryArtist=1,anyLibraryArtist=1 " +
						"WHERE artistid = " +
							"(SELECT ROWID FROM artists " +
							"WHERE name='" + artist.replace(/'/g, "''") + "')");
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
			for (let artist in this._batchArtistsAdded) {
				this._db.addQuery("UPDATE playing_at " +
						"SET libraryArtist=1,anyLibraryArtist=1 " +
						"WHERE artistid = " +
							"(SELECT ROWID FROM artists " +
							"WHERE name='" + artist.replace(/'/g, "''") + "')");
				debugLog("onBatchEnd", "Adding update query for " + artist);
				artistsAdded = true;
			}
			delete this._batchArtistsAdded;
			if (artistsAdded) {
				this._db.execute();
				this.spsUpdater();
			}

			// For batch removal
			for (let artist in this._batchArtistsRemoved) {
				debugLog("onBatchEnd", "Reseting for " + artist);
				this.resetTourDataForArtist(list, artist);
			}
			delete this._batchArtistsRemoved;

			debugLog("onBatchEnd", "Done");
		}
	},
	
	resetTourDataForArtist : function(list, artist) {
		debugLog("resetTourDataForArtist", "init");
		var otherTracks = true;
		var arr;
		try {
			arr = list.getItemsByProperty(SBProperties.artistName, artist);
		} catch (e) {
		}
		if (typeof(arr) == "undefined")
			otherTracks = false;
		debugLog("resetTourDataForArtist", "Artist: " + artist
			//+ " -- track: " + item.getProperty(SBProperties.trackName)
			//+ " -- tracks left: " + arr.length
			//+ " -- Touring:" + item.getProperty(this.onTourImgProperty)
		);
		if (!otherTracks) {
			debugLog("resetTourDataForArtist", "Clearing touring data for " +
					artist);
			// There were no other tracks, so update the SQLite DB
			// This actually turns out to be really annoying, we have to
			// first get all the concerts that this artist is playing in
			// and set playing_at.libraryArtist = 0.  For each concert, we
			// have to determine if this artist was the *only* library
			// artist there.  If so, then we also need to set
			// anyLibraryArtist = 0. this is expensive, but fortunately
			// should be rare (I hope) 
			//
			// First update all playing_at entries for this artist to be 0
			this._db.resetQuery();
			this._db.addQuery("UPDATE playing_at " +
					"SET libraryArtist=0 " +
					"WHERE artistid = " +
						"(SELECT ROWID FROM artists " +
						"WHERE name='" + artist.replace(/'/g, "''") + "')");
			this._db.execute();
			this.spsUpdater();

			// Yay 3 level query.  The inner-most gets all concerts whose
			// SUM(libraryArtist) is 0 (which means all concerts who don't
			// have any libraryartists playing, but for which their
			// anyLibraryArtist flag is still 1).  We then select the
			// concertID from that result, and update those concerts so
			// that anyLibraryArtist=0
			this._db.resetQuery();
			this._db.addQuery("UPDATE playing_at " +
					"SET anyLibraryArtist=0 " +
					"WHERE concertid in " +
						"(SELECT concertid FROM " +
							"(SELECT concertid, " +
							"SUM(libraryArtist) AS stillvalid " +
							"FROM playing_at " +
							"WHERE anyLibraryArtist=1 " +
							"GROUP BY concertid " +
							"HAVING stillvalid=0))");
			this._db.execute();
			this.spsUpdater();
		}
	},

	/*********************************************************************
	 * Routine fired for when media is removed from the main library.  We need
	 * to do this so we can update its on tour status (if we already have
	 * concert data)
	 *********************************************************************/
	onBeforeItemRemoved : function(list, item, index) {
		// If this artist wasn't on tour, then we can quit now
		var itemTouring = item.getProperty(this.onTourImgProperty);
		if (itemTouring == null || itemTouring == noTourIconSrc)
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
			this.resetTourDataForArtist(list, artist);
		}
		return false;
	},

	/*********************************************************************
	 * Initiates a concert data refresh for a given city.  This handles
	 * the logic of checking to ensure updates aren't triggered too often,
	 * or that multiple refreshes aren't occuring simultaneously.  It then
	 * sets up a gzip uncompressor channel (skStreamListener) and fires it.
	 *********************************************************************/
	refreshConcerts : function(async, city) {
		// Only allow asynchronous refreshes of the concert data every 48
		// hours, or if the forcerefresh pref is set
		var now = Date.now()/1000;
		var lastUpdated = 0;
		if (this.prefs.prefHasUserValue("lastupdated"))
			lastUpdated = this.prefs.getIntPref("lastupdated");
		var forceRefresh = false;
		if (this.prefs.prefHasUserValue("forcerefresh"))
			forceRefresh = this.prefs.getBoolPref("forcerefresh");
		if (async && (now - lastUpdated < 172800) && !forceRefresh) {
			// debugLog("refreshConcerts", "Refresh period is once/48 hrs");
			return false;
		}

		// Don't refresh if a refresh is already running
		if (this.concertRefreshRunning) {
			debugLog("refreshConcerts", "Concert refresh already running!");
			return false;
		}
		this.concertRefreshRunning = true;
		debugLog("refreshConcerts", "CONCERT REFRESH RUNNING:" +
				this.concertRefreshRunning);

		this._cache = {}; // Flush any cached data, since it will be invalid

		// Set our URL to load
		//var city = this.prefs.getIntPref("city");
		var url = "http://api-static.songkick.com/api/V2/concertdata.xml.gz" +
			"?cityid=" + city + "&key=" + SongkickAPIKey;
		debugLog("refreshConcerts", "URL:" + url);
		debugLog("refreshConcerts", "Starting concert data refresh");
	
		this.progress(this.strDownloading, 5);

		// Instantiate our stream listener
		var myListener = new skStreamListener(async, 0, this, city);
		
		// Get the converter service
		var converterService = Cc["@mozilla.org/streamConverters;1"]
				.getService(Ci.nsIStreamConverterService);

		// Instantiate our gzip decompresser converter
		var converter = converterService.asyncConvertData("gzip",
				"uncompressed", myListener, null);
		// Create our input channel to read the gzip'd XML database
		var chan = Cc["@mozilla.org/network/input-stream-channel;1"].
			createInstance(Ci.nsIInputStreamChannel);

		// Get the IO service
		var ioService = Cc["@mozilla.org/network/io-service;1"]
			.getService(Ci.nsIIOService);
		// Create an nsIURI
		var mtUri = ioService.newURI(url, null, null);
		// Create a channel from that URI
		var chan = ioService.newChannelFromURI(mtUri);

		// Initiate the asynchronous open.  This will initiate the
		// connection to Songkick, grab the gzip'd data and pass it to
		// our gzip converter which will then call the skStreamListener,
		// so our completion hook is fired in the skStreamListener's
		// onStopRequest()
		chan.asyncOpen(converter, null);

		return true;
	},

	/*********************************************************************
	 * Triggered by skStreamListener upon a successful channel close.
	 * processConcerts is passed a boolean indicating whether it should
	 * process asynchronously or not (if not, then it should update the UI
	 * using the displayCallback).  It's also passed the xmlData that the
	 * channel uncompressed from the gzip'd data.
	 *********************************************************************/
	processConcerts : function(async, city, xmlData) {
		debugLog("processConcerts","Dispatching generator to process xmlData");
		debugLog("processConcerts","city code:" + city);
		pseudoThread(this.processConcertsGenerator(async, xmlData, city));
	},
	processConcertsGenerator : function(async, xmlData, cityID) {
		debugLog("processConcerts", "Initialised and created");
		
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
		this.progress(this.strDlComplete, 10);
		
		// Setup our database
		var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
				.createInstance(Ci.sbIDatabaseQuery);
		dbq.databaseLocation = this.concertDbURI;
		dbq.setDatabaseGUID("concerts");
		dbq.setAsyncQuery(false);
		dbq.resetQuery();

		this._cache = {}; // Flush any cached data, since it will be invalid

		dbq.addQuery("DROP TABLE IF EXISTS concerts");
		dbq.addQuery("DROP TABLE IF EXISTS playing_at");
		dbq.addQuery("CREATE TABLE concerts (id INTEGER, " +
				"timestamp INTEGER, venue TEXT, city TEXT, title TEXT, " +
				"concertURL TEXT, venueURL TEXT, tickets INTEGER)");
		dbq.addQuery("CREATE TABLE IF NOT EXISTS artists (" +
				"name TEXT COLLATE NOCASE, artistURL TEXT)");
		dbq.addQuery("CREATE TABLE IF NOT EXISTS playing_at (" +
				"concertid INTEGER, artistid INTEGER, " + 
				"anyLibraryArtist INTEGER, libraryArtist INTEGER)");
		
		dbq.addQuery("create unique index if not exists artistID on " +
				"artists (artistURL)");
		dbq.execute();
		dbq.resetQuery();

		debugLog("processConcerts", "DB query setup complete");

		// Save our current timestamp to denote when it was last updated
		this.prefs.setIntPref("lastupdated", Date.now()/1000);
		this.metrics.metricsInc("concerts", "data.refresh", "");

		if (async)
			yield true;
		// Use e4x to parse the XML data 
		xmlData = xmlData.replace('<?xml version="1.0" encoding="UTF-8"?>', "");
		var x = new XML(xmlData);
		debugLog("processConcerts", "XML parsing complete");

		// Save our citypage so we can do deep-links back to Songkick
		var citypage = x.SkCityPage;
		this.prefs.setCharPref("citypage", citypage);
		
		// Create an array to track the artists on tour
		var artistsOnTour = new Array();
		var concertLength = x.Concert.length();
		this.progress(this.strConcertFound + " " + concertLength + " " +
				this.strConcertFoundP, 10);
		
		if (async)
			yield true;
		// Create a second database query to do our artist table
		// insertions and selections
		var artistDbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
			.createInstance(Ci.sbIDatabaseQuery);
		artistDbq.databaseLocation = this.concertDbURI;
		artistDbq.setDatabaseGUID("concerts");
		artistDbq.setAsyncQuery(false);

		// Iterate over each concert and add them to the database
		debugLog("processConcerts", "Beginning concert loop");
		for (let i in x.Concert) {
			var concert = x.Concert[i];
			this.progress(this.strProcessing + " " + (1+parseInt(i)) +
						" " + this.strOf + " " + concertLength,
						10+parseInt((i/concertLength)*90));
			yield true;
			var concertID = concert.@id;
			var concertDate = concert.Date;
			var timestamp;
			var year = concertDate.substr(0, 4);
			var month = concertDate.substr(5, 2);
			var day = concertDate.substr(8, 2);
			timestamp = Date.UTC(year,month-1,day,19,0,0)/1000;
			var venue = concert.Venue.toString();
			var venueURL = concert.SkVenuePage;
			var city = concert.City;
			var title = concert.Title.toString();
			var concertURL = concert.SkConcertPage;
			var tickets = concert.Tickets == "true";
			
			// The list of artists playing at this concert
			var artistList = new Array();
			var libraryArtistFound = 0;
			// Iterate over each artist, and add them to the database
			var artists = concert.Artists.Artist;
			for (let j in artists) {
				artistDbq.resetQuery();
				var artist = artists[j].Name.toString();
				var artistURL = artists[j].SkArtistPage;
				var query = "insert or ignore into artists values (" +
						"'" + artist.replace(/'/g, "''") + "', '" +
						artistURL + "')";
				artistDbq.addQuery(query);
				artistDbq.addQuery("select ROWID from artists where artistURL="
						+ '"' + artistURL + '"');
				var ret = artistDbq.execute();
				var result = artistDbq.getResultObject();
				var artistID = result.getRowCell(0,0);
				if (async)
					yield true;
				var thisLibArtist = 0;
				var artistName = artist.toUpperCase();
				if (typeof(libraryArtists[artistName]) != "undefined") {
					libraryArtistFound = 1;
					thisLibArtist = 1;
				}
				artistList[artistID] = thisLibArtist;
				artistsOnTour[artistName] = artistURL;
			}
			
			// Enter the concert info into the DB
			var ticketsAvail = 0;
			if (tickets)
				ticketsAvail = 1;
			var query = "insert into concerts values (" + concertID +
					", " + timestamp + ", '" + venue.replace(/'/g, "''") +
					"', '" + city + "', '" + title.replace(/'/g, "''") +
					"', '" + concertURL + "', '" + venueURL + "', '" +
					ticketsAvail + "')";
			dbq.addQuery(query);

			for (let artistid in artistList) {
				dbq.addQuery("insert into playing_at values (" +
						concertID + "," + artistid + "," + libraryArtistFound
						+ "," + artistList[artistid] + ")");
			}
			
			// Only run the actual DB insert every 20 concerts
			if (i % 20) {
				// debugLog("processConcerts", "async: inserting into DB now");
				dbq.execute();
				dbq.resetQuery();
				yield true;
			}
		}
		this.progress(this.strFinishing, 100);
		yield true;
		if (dbq.getQueryCount() > 0) {
			debugLog("processConcerts", "finishing up DB insertions\n");
			dbq.execute();
			dbq.resetQuery();
		}
		
		debugLog("processConcerts", "end concert loop");

		this.progress(this.strComplete, 100);
		yield true;
	
		// We've got all our results now, so if we're running synchronously,
		// we can go ahead and switch the display back to the browse listing
		// while we go ahead and update the library items in the background
		if (!async) {
			dbq.resetQuery();
			debugLog("processConcerts", "city code:" + cityID);
			var query = "select state,country from cities where id=" + cityID;
			dbq.addQuery(query);
			var ret = dbq.execute();
			if (ret != 0)
				throw ("Error!" + ret);
			var result = dbq.getResultObject();
			var state;
			var country;
			debugLog("processConcerts", "row count:" + result.getRowCount());
			if (result.getRowCount() > 0) {
				state = result.getRowCellByColumn(0, "state");
				country = result.getRowCellByColumn(0, "country");
			} else {
				throw ("Error! Rowcount:" + result.getRowCount());
			}
			debugLog("processConcerts", "country:"+country+"//state:"+state);
			this.prefs.setIntPref("country", country);
			this.prefs.setIntPref("state", state);
			this.prefs.setIntPref("city", cityID);
			if (this.displayCallback != null)
				this.displayCallback.showListings();
		}

		// No longer refreshing the DB, clear the lock
		debugLog("processConcerts", "Done, clearing the lock");
		this.concertRefreshRunning = false;
		debugLog("refreshConcerts", "CONCERT REFRESH RUNNING:" +
				this.concertRefreshRunning);
		
		// Update the concert count in the service pane
		this.spsUpdater();
		yield true;

		// Enumerate the user's library for each track that has the
		// on tour status flag set, and clear it
		debugLog("processConcerts", "clearing library on tour properties");
		try {
			var itemEnum = mainLib.getItemsByProperty(this.onTourImgProperty,
					onTourIconSrc).enumerate();
			while (itemEnum.hasMoreElements()) {
				var item = itemEnum.getNext();
				item.setProperty(this.onTourImgProperty, noTourIconSrc);
				yield true;
			}
		} catch (e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
			// ignore, just meant there were no items
		}

		// Now for each artist in the artistsOnTour array, update the
		// on tour status flag for all tracks in the library that match
		// the artist
		debugLog("processConcerts", "setting new library on tour properties");
		for (let artistName in artistsOnTour) {
			try {
				var itemEnum = mainLib.getItemsByProperty(
						SBProperties.artistName, artistName).enumerate();
				while (itemEnum.hasMoreElements()) {
					var item = itemEnum.getNext();
					/*
					// Handy to have, but commenting out to avoid unnecessary
					// getProperty() calls
					debugLog("processConcerts", "Track: " +
							item.getProperty(SBProperties.trackName) +
							" by " + artistName + " is on tour");
					*/
					item.setProperty(this.onTourImgProperty, onTourIconSrc);
					item.setProperty(this.onTourUrlProperty,
							artistsOnTour[artistName]);
					yield true;
				}
			} catch (e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
				// ignore, just meant there were no items
			}
		}
		
		// Go rebuild the artists touring smart playlist
		try {
			var itemEnum = LibraryUtils.mainLibrary.getItemsByProperty(
					SBProperties.customType,
					"concerts_artistsTouring").enumerate();
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
	 * Register a display callback which processConcerts can use to trigger
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
		this._db.resetQuery();
		this._db.addQuery("select * from countries");
		var ret = this._db.execute();
		var countries = new Array();
		if (ret == 0) {
			var result = this._db.getResultObject();
			for (let i=0; i<result.getRowCount(); i++) {
				var id = result.getRowCellByColumn(i, "id");
				var name = result.getRowCellByColumn(i, "name");
				countries.push({"id": id, "name": name});
			}
		}
		return this.json.encode(countries);
	},
	getLocationStates : function(country) {
		this._db.resetQuery();
		this._db.addQuery("select * from states where country=" + country);
		var ret = this._db.execute();
		var states = new Array();
		if (ret == 0) {
			var result = this._db.getResultObject();
			for (let i=0; i<result.getRowCount(); i++) {
				var id = result.getRowCellByColumn(i, "id");
				var name = result.getRowCellByColumn(i, "name");
				states.push({"id": id, "name": name});
			}
		}
		return this.json.encode(states);
	},
	getLocationCities : function(state) {
		this._db.resetQuery();
		this._db.addQuery("select * from cities where state=" + state);
		var ret = this._db.execute();
		var cities = new Array();
		if (ret == 0) {
			var result = this._db.getResultObject();
			for (let i=0; i<result.getRowCount(); i++) {
				var id = result.getRowCellByColumn(i, "id");
				var name = escape(result.getRowCellByColumn(i, "name"));
        // bug 19997
        // database value is stored incorrectly, so compensate for it by
        // escaping the string to URI safe values, and then running
        // decodeURIComponent to get it back into UTF8
        name = decodeURIComponent(unescape(name));
				cities.push({"id": id, "name": name});
			}
		}
		return this.json.encode(cities);
	},

	/*********************************************************************
	 * Returns the city name only, e.g. "SF Bay Area"
	 *********************************************************************/
	getCityString : function(city) {
		this._db.resetQuery();
		var query = "select cities.name " +
				"from cities where cities.id=" + city;
		this._db.addQuery(query);
		var ret = this._db.execute();
		if (ret != 0)
			return ("Error!" + ret);
		var result = this._db.getResultObject();
		if (result.getRowCount() > 0) {
			var cityName = result.getRowCell(0,0);
			return (cityName);
		} else {
			return ("Error! Rowcount:" + result.getRowCount());
		}
	},

	/*********************************************************************
	 * Returns the full location name, e.g. "City, State Country"
	 *********************************************************************/
	getLocationString : function(country, state, city) {
		this._db.resetQuery();
		var query = "select cities.name,states.name,countries.name " +
				"from cities join states on cities.state=states.id " +
				"join countries on states.country=countries.id " +
				"where cities.id=" + city;
		this._db.addQuery(query);
		var ret = this._db.execute();
		if (ret != 0)
			return ("Error!" + ret);
		var result = this._db.getResultObject();
		if (result.getRowCount() > 0) {
			var cityName = result.getRowCell(0,0);
			var stateName = result.getRowCell(0,1);
			var countryName = result.getRowCell(0,2);
			if (stateName == "") {
				return (cityName + ", " + countryName);
			} else {
				return (cityName + ", " + stateName + " " + countryName);
			}
		} else {
			return ("Error! Rowcount:" + result.getRowCount());
		}
	},

	/*********************************************************************
	 * Returns the number of concerts in the database.  If
	 * filterLibraryArtists is true, then it will only return the number
	 * of concerts that have artists in the user's main library.
	 *********************************************************************/
	getConcertCount : function(filterLibraryArtists) {
		var firstrun = this.prefs.getBoolPref("firstrun");
		if (firstrun)
			return (0);

    var groupBy = this.prefs.getCharPref("groupby");
		this._db.resetQuery();

    // Get today's date... sort of strange, but we want 00:00:00, e.g.
    // stroke of midnight this morning, hence the double new Date()
    var today = new Date();
    var todayMon = today.getMonth();
    var todayDate = today.getDate();
    var todayYear = today.getFullYear();
    today = new Date(todayYear, todayMon, todayDate);
    today = parseInt(today.getTime() / 1000);

		try {
      if (groupBy == "artist") {
        var query = "SELECT COUNT(distinct concertid) FROM playing_at" +
          " JOIN concerts ON playing_at.concertid = concerts.id" +
          " WHERE concerts.timestamp >= " + today;
        if (filterLibraryArtists)
          query += " AND playing_at.libraryArtist = 1";
        this._db.addQuery(query);
      } else {
        var ceilingMon = todayMon+5;
        var ceilingYear = todayYear;
        if (ceilingMon > 11) {
          ceilingMon -= 11;
          ceilingYear++;
        }
        var ceiling = new Date(ceilingYear, ceilingMon, 1);
        ceiling = parseInt(ceiling.getTime() / 1000);
        var query = "SELECT COUNT(distinct id) FROM playing_at" +
          " JOIN concerts ON playing_at.concertid = concerts.id" +
          " WHERE concerts.timestamp >= " + today +
          " AND concerts.timestamp < " + ceiling;
        if (filterLibraryArtists)
          query += " AND playing_at.anyLibraryArtist = 1";
        this._db.addQuery(query);
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
	 * For a given artist name, return whether the artist is on tour or not
	 *********************************************************************/
	getTourStatus : memoize(function getTourStatus(artist) {
		var firstrun = this.prefs.getBoolPref("firstrun");
		if (firstrun)
			return false;
		this._db.resetQuery();
		this._db.addQuery('SELECT count(*) FROM playing_at ' +
				'JOIN artists on artistid=artists.ROWID ' +
				'JOIN concerts on concertid=concerts.id ' +
				"WHERE artists.name = '" + artist.replace(/'/g, "''") + "'" +
				'AND concerts.timestamp > ' + parseInt(Date.now()/1000));
		var ret = this._db.execute();
		var result = this._db.getResultObject();
		var count = result.getRowCell(0, 0);
		return (count > 0);
	}),
	
	/*********************************************************************
	 * For a given artist name, return their Songkick tour URL
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
	
	getArtistOnTourUrl : function(artist) {
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
	 * For a given artist name, return the tour URL if they are on tour
	 *********************************************************************/
	getArtistUrlIfOnTour : memoize(function getArtistUrlIfOnTour(artist) {
		var firstrun = this.prefs.getBoolPref("firstrun");
		if (firstrun)
			return null;
		//debugLog("getArtistUrlIfOnTour", "New media item, artist: " + artist);
		this._db.resetQuery();
		this._db.addQuery("SELECT artistURL FROM artists " +
			'JOIN playing_at on playing_at.artistid = artists.ROWID ' +
			'JOIN concerts on playing_at.concertid=concerts.id ' +
			"WHERE artists.name = '" + artist.replace(/'/g, "'") + "'" +
			'AND concerts.timestamp > ' + parseInt(Date.now()/1000));
		var ret = this._db.execute();
		var result = this._db.getResultObject();
		if (result.getRowCount() > 0) {
			debugLog("getArtistUrlIfOnTour", "Artist URL found");
			var url = result.getRowCellByColumn(0, "artistURL");
			return (url);
		} else {
			return null;
		}
	}),

	/*********************************************************************
	 * Returns the URL for the provider's homepage
	 *********************************************************************/
	providerURL : function() {
		return ("http://www.songkick.com");
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

		var url = "http://api-static.songkick.com/api/V2/locationdata.xml.gz"
			+ "?key=" + SongkickAPIKey;
		
		// Instantiate our stream listener
		var myListener = new skStreamListener(false, 1, this);
		
		// Get the converter service
		var converterService = Cc["@mozilla.org/streamConverters;1"]
				.getService(Ci.nsIStreamConverterService);

		// Instantiate our gzip decompresser converter
		var converter = converterService.asyncConvertData("gzip",
				"uncompressed", myListener, null);
		// Create our input channel to read the gzip'd XML database
		var chan = Cc["@mozilla.org/network/input-stream-channel;1"].
			createInstance(Ci.nsIInputStreamChannel);

		// Get the IO service
		var ioService = Cc["@mozilla.org/network/io-service;1"]
			.getService(Ci.nsIIOService);
		// Create an nsIURI
		var mtUri = ioService.newURI(url, null, null);
		// Create a channel from that URI
		var chan = ioService.newChannelFromURI(mtUri);

		debugLog("refreshLocations", "Opening channel");
		// Initiate the asynchronous open.  This will initiate the connection
		// to Songkick, grab the gzip'd data and pass it to our gzip converter
		// which will then call the skStreamListener, so our completion hook is
		// fired in the skStreamListener's onStopRequest()
		chan.asyncOpen(converter, null);
	},

	processLocations: function(xmlData) {
		debugLog("processLocations", "Processing location XML data");

		// Translate location XML data to store it in our DB
		this._db.resetQuery();
		this._db.addQuery("DROP TABLE IF EXISTS cities");
		this._db.addQuery("DROP TABLE IF EXISTS states");
		this._db.addQuery("DROP TABLE IF EXISTS countries");
		this._db.addQuery("create table cities (id integer, state integer, " +
				"country integer, lat real, lng real, name text)");
		this._db.addQuery("create table states (id integer, " +
				"country integer, name text)");
		this._db.addQuery("create table countries (id integer, name text)");
		
		xmlData = xmlData.replace('<?xml version="1.0" encoding="UTF-8"?>', "");
		var x = new XML(xmlData);
		debugLog("processLocations", "# countries:" + x..Country.length());
		for (let i=0; i<x..Country.length(); i++) {
			var countryId = x..Country[i].@id;
			var countryName = x..Country[i].@name;
			var query = "insert into countries values " +
					"(" + countryId + ", '" + countryName + "')";
			this._db.addQuery(query);
			debugLog("processLocations", "# states:" +
					x..Country[i].State.length());
			for (let j=0; j<x..Country[i].State.length(); j++) {
				var stateId = x..Country[i].State[j].@id;
				var stateName = x..Country[i].State[j].@name;
				var query = "insert into states values " +
						"(" + stateId + ", " + countryId + ", '" +
						stateName + "')";
				this._db.addQuery(query);
				for (let k=0; k<x..Country[i].State[j].City.length(); k++) {
					var cityId = x..Country[i].State[j].City[k].@id;
					var cityName =
						x..Country[i].State[j].City[k].@name.toString();
					var cityLat =
						x..Country[i].State[j].City[k].@lat.toString();
					var cityLng =
						x..Country[i].State[j].City[k].@lng.toString();
					if (!cityLat)
						cityLat = "0";
					if (!cityLng)
						cityLng = "0";
					var query = "insert into cities values " +
							"(" + cityId + ", " + stateId + ", " + countryId +
							", " + cityLat + ", " + cityLng + ", '" +
							cityName.replace(/'/g, "''") + "')";
					this._db.addQuery(query);
				}

			}
		}
		var ret = this._db.execute();
		this.locationRefreshRunning = false;
		debugLog("processLocations", "Location data processed.");
	},

	gotLocationInfo : function() {
		this._db.resetQuery();
		this._db.addQuery("select count(*) from cities");
		try {
			this._db.execute();
		} catch (e) {
			dump("FAIL\n");
			return (-1);
		}
		var result = this._db.getResultObject();
		var count = result.getRowCell(0,0);
		if (count <= 0)
			debugLog("gotLocationInfo", count + "No location info yet");
		return (count > 0);
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

	/*********************************************************************
	 * Return enumerators for iterating through artists and concerts
	 *********************************************************************/
	artistConcertEnumerator : function(filter) {
		return new skArtistConcertEnumerator(filter);
	},
	concertEnumerator : function(sort, filter) {
		return new skConcertEnumerator(sort, filter);
	}
}

/**************************************************************************
 * Enumerator implementing the nsISimpleEnumerator interface to enumerate
 * over all the artists we know about that are currently performing in an
 * upcoming concert
 **************************************************************************/
skArtistConcertEnumerator = function(filter) {
	this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
		.createInstance(Ci.sbIDatabaseQuery);
	var ios = Cc["@mozilla.org/network/io-service;1"]
		.createInstance(Ci.nsIIOService);
	var dbdir = Cc["@mozilla.org/file/directory_service;1"]
		.createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

	var uri = ios.newFileURI(dbdir);
	this._db.databaseLocation = uri;
	this._db.setDatabaseGUID("concerts");

	this._artistDb = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
		.createInstance(Ci.sbIDatabaseQuery);
	var ios = Cc["@mozilla.org/network/io-service;1"]
		.createInstance(Ci.nsIIOService);
	var dbdir = Cc["@mozilla.org/file/directory_service;1"]
		.createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

	var uri = ios.newFileURI(dbdir);
	this._artistDb.databaseLocation = uri;
	this._artistDb.setDatabaseGUID("concerts");
	
	var query = "SELECT * FROM playing_at" +
		" JOIN artists ON playing_at.artistid = artists.ROWID" +
		" JOIN concerts ON playing_at.concertid = concerts.id";
	if (filter)
		query += " WHERE playing_at.libraryArtist = 1";
	query += " ORDER BY artists.name COLLATE NOCASE";
	this._db.addQuery(query);
	var ret = this._db.execute();
	if (ret == 0) {
		this._results = this._db.getResultObject();
		this._rowIndex = 0;
	} else {
		this._results = null;
	}
}
skArtistConcertEnumerator.prototype = {
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

		var artistname =
			this._results.getRowCellByColumn(this._rowIndex, "name");
		var artisturl =
			this._results.getRowCellByColumn(this._rowIndex, "artistURL");
		var id = this._results.getRowCellByColumn(this._rowIndex, "id");
		var ts = this._results.getRowCellByColumn(this._rowIndex, "timestamp");
		var venue = this._results.getRowCellByColumn(this._rowIndex, "venue");
		var city = this._results.getRowCellByColumn(this._rowIndex, "city");
		var title = this._results.getRowCellByColumn(this._rowIndex, "title");
		var url =
			this._results.getRowCellByColumn(this._rowIndex,"concertURL");
		var lib =
			this._results.getRowCellByColumn(this._rowIndex,"libraryArtist");
		var venueURL = this._results.getRowCellByColumn(this._rowIndex,
				"venueURL");
		var ticket = this._results.getRowCellByColumn(this._rowIndex,"tickets");

		this._artistDb.resetQuery();
		this._artistDb.addQuery("select * from artists join playing_at on playing_at.concertid=" + id + " and playing_at.artistid = artists.ROWID");
		var ret = this._artistDb.execute();
		if (ret != 0) {
			throw Components.results.NS_ERROR_FAILURE;
		}
		var results = this._artistDb.getResultObject();
		var artists = new Array();
		for (let i=0; i<results.getRowCount(); i++) {
			var artistName = results.getRowCellByColumn(i, "name");
			var artistURL = results.getRowCellByColumn(i, "artistURL");
			artists.push({name:artistName, url:artistURL});
		}
		this._rowIndex++;
	
		var item = {
				artistname: artistname,
				artisturl: artisturl,
				id: id,
				ts: ts,
				venue: venue,
				city: city,
				title: title,
				url: url,
				venueURL: venueURL,
				tickets: ticket,
				artists: artists,
				libartist: lib
		}

		item.wrappedJSObject = item;
		return item;
	},
		
	// nsISimpleEnumerator
	QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
}

/**************************************************************************
 * Enumerator implementing the nsISimpleEnumerator interface to enumerate
 * over all the concerts we know about
 **************************************************************************/
skConcertEnumerator = function(sort, filter) {
	this._db = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
		.createInstance(Ci.sbIDatabaseQuery);
	var ios = Cc["@mozilla.org/network/io-service;1"]
		.createInstance(Ci.nsIIOService);
	var dbdir = Cc["@mozilla.org/file/directory_service;1"]
		.createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

	var uri = ios.newFileURI(dbdir);
	this._db.databaseLocation = uri;
	this._db.setDatabaseGUID("concerts");

	this._artistDb = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
		.createInstance(Ci.sbIDatabaseQuery);
	var ios = Cc["@mozilla.org/network/io-service;1"]
		.createInstance(Ci.nsIIOService);
	var dbdir = Cc["@mozilla.org/file/directory_service;1"]
		.createInstance(Ci.nsIProperties).get("ProfD", Ci.nsIFile);

	var uri = ios.newFileURI(dbdir);
	this._artistDb.databaseLocation = uri;
	this._artistDb.setDatabaseGUID("concerts");
	
	var sortBy;
	switch (sort) {
		case "date":	sortBy="timestamp";
						break;
		case "venue":	sortBy="venue";
						break;
		case "title":
		default:		sortBy="title";
						break;
	}
	var query = "SELECT * FROM concerts" +
		" JOIN playing_at ON playing_at.concertid = concerts.id" +
		" JOIN artists ON playing_at.artistid = artists.ROWID";
	if (filter)
		query += " WHERE playing_at.anyLibraryArtist = 1";
	query += " ORDER BY " + sortBy;
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
}

skConcertEnumerator.prototype = {
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

		var id = this._results.getRowCellByColumn(this._rowIndex, "id");
		var id = this._results.getRowCellByColumn(this._rowIndex, "id");
		var ts = this._results.getRowCellByColumn(this._rowIndex, "timestamp");
		var venue = this._results.getRowCellByColumn(this._rowIndex, "venue");
		var city = this._results.getRowCellByColumn(this._rowIndex, "city");
		var title = this._results.getRowCellByColumn(this._rowIndex, "title");
		var url = this._results.getRowCellByColumn(this._rowIndex,"concertURL");
		var vURL = this._results.getRowCellByColumn(this._rowIndex, "venueURL");
		var ticket = this._results.getRowCellByColumn(this._rowIndex,"tickets");

		// Loop over the successive rows to gather all the artists playing
		// at this concert
		var i=0;
		var artists = new Array();
		while (this._results.getRowCellByColumn(this._rowIndex + i, "id") == id)
		{
			var artistName = this._results.getRowCellByColumn(
					this._rowIndex + i, "name");
			var artistURL = this._results.getRowCellByColumn(
					this._rowIndex + i, "artistURL");
			artists.push({name:artistName, url:artistURL});
			i++;
		}

		// Start the next getNext() call at the next concertID
		this._rowIndex += i;

		var item = {
				id: id,
				ts: ts,
				venue: venue,
				city: city,
				title: title,
				url: url,
				venueURL: vURL,
				tickets: ticket,
				artists: artists};
		item.wrappedJSObject = item;
		return item;
	},
	
	// nsISimpleEnumerator
	QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
}

var components = [Songkick];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([Songkick]);
}

