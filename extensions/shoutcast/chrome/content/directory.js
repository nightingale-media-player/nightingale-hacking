if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

if (typeof(gPPS) == "undefined")
	var gPPS = Cc['@songbirdnest.com/Songbird/PlaylistPlayback;1']
			.getService(Ci.sbIPlaylistPlayback);

if (typeof(songbirdMainWindow) == "undefined")
	var songbirdMainWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
			.getService(Ci.nsIWindowMediator)
			.getMostRecentWindow("Songbird:Main").window;

if (typeof(gBrowser) == "undefined")
	var gBrowser = Cc["@mozilla.org/appshell/window-mediator;1"]
			.getService(Ci.nsIWindowMediator)
			.getMostRecentWindow("Songbird:Main").window.gBrowser;

if (typeof(ioService) == "undefined")
	var ioService = Cc["@mozilla.org/network/io-service;1"]
			.getService(Ci.nsIIOService);

const shoutcastTempLibGuid = "extensions.shoutcast-radio.templib.guid";
const shoutcastLibraryGuid = "extensions.shoutcast-radio.library.guid";
const shoutcastPlaylistInit = "extensions.shoutcast-radio.plsinit";
const shoutcastGenre = "extensions.shoutcast-radio.genre";
const shoutcastMinBitRate = "extensions.shoutcast-radio.min-bit-rate";
const shoutcastMinListeners = "extensions.shoutcast-radio.min-listeners";
const shoutcastCheckBitRate = "extensions.shoutcast-radio.limit-bit-rate";
const shoutcastCheckListeners = "extensions.shoutcast-radio.limit-listeners";
const defaultGenre = "sbITop";

if (typeof(kPlaylistCommands) == "undefined") {
	Cu.import("resource://app/components/kPlaylistCommands.jsm");
	if (!kPlaylistCommands)
		throw new Error("Import of kPlaylistCommands module failed!");
}

if (typeof(SBProperties) == "undefined") {
	Cu.import("resource://app/jsmodules/sbProperties.jsm");
	if (!SBProperties)
		throw new Error("Import of sbProperties module failed");
}

if (typeof(LibraryUtils) == "undefined") {
	Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
	if (!LibraryUtils)
		throw new Error("Import of sbLibraryUtils module failed");
}

var RadioDirectory = {
	playlist : null,
	tempLib : null,
	favesList : null,
	streamList : null,
	favouriteIDs : null,
	radioLib : null,

	init : function() {
		var genre;
		this._strings = document.getElementById("shoutcast-radio-strings");
		if (Application.prefs.has(shoutcastGenre)) {
			// load our old genre
			genre = Application.prefs.getValue(shoutcastGenre, defaultGenre);
		} else {
			// no genre chosen, use the default genre
			genre = defaultGenre;
			Application.prefs.setValue(shoutcastGenre, genre);
		}

		// Read in our list of genre strings
		var menulist = document.getElementById("shoutcast-genre-menulist");
		var strings = Cc["@mozilla.org/intl/stringbundle;1"]
			.getService(Ci.nsIStringBundleService)
			.createBundle("chrome://shoutcast-radio/locale/genres.properties");
		var iter = strings.getSimpleEnumeration();
		var genres = new Array();
		while (iter.hasMoreElements()) {
			var genreProp = iter.getNext()
				.QueryInterface(Ci.nsIPropertyElement);
			var genreValue = genreProp.key.substr(5);
			var genreLabel = genreProp.value;
			if (genreValue == "TOP")
				continue;
			dump(genreValue + "/" + genreLabel + "\n");
			genres.push({value:genreValue, label:genreLabel});
		}

		// Add our custom genres
		var customGenres = Application.prefs.getValue(
				"extensions.shoutcast-radio.custom-genres", "").split(",");
		for (i in customGenres) {
			if (customGenres[i].length > 0) {
				var custom = customGenres[i];
				custom = custom.replace(/^\s*/,'').replace(/\s*$/,'');
				genres.push({value:custom, label:custom});
			}
		}

		// Sort our genres
		genres.sort(function(a,b) {
			return (a.label.toUpperCase() > b.label.toUpperCase());
		});

		// Add the "Top Stations" item first
		menulist.appendItem(strings.GetStringFromName("genreTOP"), "sbITop");
		menulist.menupopup.appendChild(document.createElementNS(
				"http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
				"menuseparator"));

		// Build the menulist
		var found = false;
		for (i in genres) {
			var thisgenre = genres[i];
			var el = menulist.appendItem(thisgenre.label, thisgenre.value);
			if (genre == thisgenre.value) {
				menulist.selectedItem = el;
				found = true;
			}
		}
		if (!found)
			menulist.selectedIndex = 0;

		this.favouriteIDs = [];

		// Setup our references to the SHOUTcast libraries
		this.getLibraries();

		// Bind the playlist widget to our library
		this.playlist = document.getElementById("shoutcast-directory");
		var libraryManager = Cc['@songbirdnest.com/Songbird/library/Manager;1']
							   .getService(Ci.sbILibraryManager);
		this.playlist.bind(this.radioLib.createView());
		
		// If this is the first time we've loaded the playlist, clear the 
		// normal columns and use the stream ones
		if (!Application.prefs.getValue(shoutcastPlaylistInit, false)) {
			Application.prefs.setValue(shoutcastPlaylistInit, true);
			var colSpec = SC_streamName + " 268 " + SC_bitRate + " 55 " +
					SC_comment + " 175 " + SC_listenerCount + " 55 " +
					SC_bookmark + " 55";
			this.radioLib.setProperty(SBProperties.defaultColumnSpec, colSpec);
			this.playlist.clearColumns();
			this.playlist.appendColumn(SC_streamName, "268");
			this.playlist.appendColumn(SC_bitRate, "55");
			this.playlist.appendColumn(SC_comment, "175");
			this.playlist.appendColumn(SC_listenerCount, "55");
			this.playlist.appendColumn(SC_bookmark, "55");
		}

		var ldtv = this.playlist.tree.view
				.QueryInterface(Ci.sbILocalDatabaseTreeView);
		ldtv.setSort(SC_listenerCount, 0);
		
		this.playlist.addEventListener("PlaylistCellClick",
				onPlaylistCellClick, false);
		this.playlist.addEventListener("Play", onPlay, false);

		// Load the tracks for our selected genre
		this.loadTable(genre);
		
		// Reload the filter value
		var filterValue = Application.prefs.getValue(
				"extensions.shoutcast-radio.filter", "");
		if (filterValue != "") {
			document.getElementById("filter").value = filterValue;
			this.setFilter(filterValue);
		}
	},

	getLibraries : function() {
		var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
			.getService(Ci.sbILibraryManager);

		var libGuid = Application.prefs.getValue(shoutcastLibraryGuid, "");
		if (libGuid != "") {
			// XXX should error check this
			var libGuid = Application.prefs.get(shoutcastLibraryGuid);
			this.radioLib = libraryManager.getLibrary(libGuid.value);
		} else {
			this.radioLib = createLibrary("shoutcast_radio_library", null,
					false);
			// doesn't manifest itself in any user visible way, so i think
			// it's safe to not localise
			this.radioLib.name = "SHOUTcast Radio";
			this.radioLib.setProperty(SBProperties.hidden, "1");
			dump("*** Created SHOUTcast Radio library, GUID: " +
					this.radioLib.guid);
			libraryManager.registerLibrary(this.radioLib, true);
			Application.prefs.setValue(shoutcastLibraryGuid,
					this.radioLib.guid);
		}

		libGuid = Application.prefs.getValue(shoutcastTempLibGuid, "");
		if (libGuid != "") {
			// XXX should error check this
			var libGuid = Application.prefs.get(shoutcastTempLibGuid);
			this.tempLib = libraryManager.getLibrary(libGuid.value);

			// Get our favourites & stream lists
			var a = this.tempLib.getItemsByProperty(
					SBProperties.customType, "radio_favouritesList");
			this.favesList = a.queryElementAt(0, Ci.sbIMediaList);
			a = this.tempLib.getItemsByProperty(
					SBProperties.customType, "radio_tempStreamList");
			this.streamList = a.queryElementAt(0, Ci.sbIMediaList);
		} else {
			this.tempLib = createLibrary("shoutcast_temp_library", null,
					false);
			// doesn't manifest itself in any user visible way, so i think
			// it's safe to not localise
			this.tempLib.name = "Temporary Library";
			this.tempLib.setProperty(SBProperties.hidden, "1");
			this.tempLib.setProperty(SBProperties.isReadOnly, "1");
			dump("*** Created SHOUTcast Temporary Radio library, GUID: " +
					this.tempLib.guid);
			libraryManager.registerLibrary(this.tempLib, true);
			Application.prefs.setValue(shoutcastTempLibGuid,
					this.tempLib.guid);

			// the library might already exist in which case it might
			// already have a faveslist - look for it
			var propExists = true;
			var a;
			try {
				a = this.tempLib.getItemsByProperty(
						SBProperties.customType, "radio_favouritesList");
			} catch (e) {
				propExists = false;
			}
			if (propExists && a.length > 0) {
				this.favesList = a.queryElementAt(0, Ci.sbIMediaList);
			} else {
				// both our favourites list and the temporary current-stream
				// list will be children of this tempLib 
				this.favesList = this.tempLib.createMediaList("simple");
				this.favesList.setProperty(SBProperties.hidden, "1");
				this.favesList.setProperty(SBProperties.isReadOnly, "1");
			}
			
			this.favesList.name = this._strings.getString("favourites");
			var colSpec = SC_streamName + " 260 " + SC_bitRate + " 55 " +
					SC_comment + " 175 " + SBProperties.genre + " 55 " +
					SC_unfave + " 55";
			this.favesList.setProperty(SBProperties.columnSpec, colSpec);
			this.favesList.setProperty(SBProperties.defaultColumnSpec,colSpec);

			// Set the Media View to be list only (not filter)
			var mpManager = Cc["@songbirdnest.com/Songbird/MediaPageManager;1"]
					.getService(Ci.sbIMediaPageManager);
			var pages = mpManager.getAvailablePages(this.favesList);
			var listView = null;
			while (pages.hasMoreElements()) {
				var pageInfo = pages.getNext();
				pageInfo.QueryInterface(Ci.sbIMediaPageInfo);
				if (pageInfo.contentUrl ==
						"chrome://songbird/content/mediapages/playlistPage.xul")
					listView = pageInfo;
			}
			if (listView)
				mpManager.setPage(this.favesList, listView)

			// temporary playlist to hold the current stream to work around
			// GStreamer inability to play .pls mediaItems
			propExists = true;
			try {
				a = this.tempLib.getItemsByProperty(
						SBProperties.customType, "radio_tempStreamList");
			} catch (e) {
				propExists = false;
			}
			if (propExists && a.length > 0) {
				this.streamList = a.queryElementAt(0, Ci.sbIMediaList);
			} else {
				this.streamList = this.tempLib.createMediaList("simple");
				this.streamList.setProperty(SBProperties.hidden, "1");
				this.streamList.setProperty(SBProperties.isReadOnly, "1");
			}

			// set custom types so we can easily find them later
			this.favesList.setProperty(SBProperties.customType,
					"radio_favouritesList");
			this.streamList.setProperty(SBProperties.customType,
					"radio_tempStreamList");

			// set our flag for SP node creation to false
			this.favesList.setProperty(SC_nodeCreated, "0");
		}
		if (RadioDirectory.favesList.getProperty(SC_nodeCreated) == "0"
				&& RadioDirectory.favesList.length > 0) {
			createFavouritesNode();
		}

		// Seed the favouriteIDs array with the IDs of the stations
		for (var i=0; i<this.favesList.length; i++) {
			var item = this.favesList.getItemByIndex(i);
			var id = item.getProperty(SC_id);
			this.favouriteIDs.push(id);
		}
	},

	changeGenre : function(menulist) {
		var genre = menulist.selectedItem.value;
		Application.prefs.setValue(shoutcastGenre, genre);
		this.clearFilter();
		this.loadTable(genre);
	},

	loadTable : function(genre) {
		// reset the library
		this.radioLib.clear();

		// Make the progress meter spin
		var el = songbirdMainWindow.document
			.getElementById("sb-status-bar-status-progressmeter");
		el.mode = "undetermined";

		// if genre is null, then we're just being asked to filter our existing
		// data and we don't need to reload data
		if (genre != null) {
			var stationList = ShoutcastRadio.getStationList(genre);
								
			var count=0;
			var minBR = Application.prefs.getValue(shoutcastMinBitRate, 64);
			var minLC = Application.prefs.getValue(shoutcastMinListeners, 10);
			var checkBitRate =
				Application.prefs.getValue(shoutcastCheckBitRate, false);
			var checkListeners =
				Application.prefs.getValue(shoutcastCheckListeners, false);

			var trackArray = Cc["@mozilla.org/array;1"]
					.createInstance(Ci.nsIMutableArray);
			var propertiesArray = Cc["@mozilla.org/array;1"]
					.createInstance(Ci.nsIMutableArray);
			for (var i=0; i<stationList.length; i++) {
				var station = stationList[i];
				var name = station.getAttribute("name");
				var mimeType = station.getAttribute("mt");
				var id = station.getAttribute("id");
				var bitrate = parseInt(station.getAttribute("br"));
				var currentTrack = station.getAttribute("ct");
				var numListeners = station.getAttribute("lc");
				var genres = station.getAttribute("genre").split(" ");
				var thisgenre = genres.shift();
				// Special case for 'top 40'
				if (thisgenre == "Top" && genres.length > 0 
						&& genres[0] == "40")
					thisgenre += " 40";

				// Arbitrarily restricting to MP3 for now to eliminate
				// dependency on AAC decoder being installed
				if (mimeType != "audio/mpeg")
					continue;
				
				// Only list stations with listeners set above the preference
				if (checkListeners && numListeners < minLC)
					continue;

				// Only list stations with bitrates higher than the preference
				if (checkBitRate && bitrate < minBR) 
					continue;

				var props = Cc[
				"@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
					.createInstance(Ci.sbIMutablePropertyArray);

				var heartSrc;
				var breakSrc = "chrome://shoutcast-radio/skin/invis-16x16.png";
				if (this.favouriteIDs.indexOf(id) != -1) {
					heartSrc = "chrome://shoutcast-radio/skin/heart-active.png";
				} else {
					heartSrc = "chrome://shoutcast-radio/skin/invis-16x16.png";
				}
				var searchStr = name + " " + currentTrack;
				props.appendProperty(SC_streamName, name);
				props.appendProperty(SBProperties.bitRate, bitrate);
				props.appendProperty(SBProperties.genre, thisgenre);
				props.appendProperty(SBProperties.comment, currentTrack);
				props.appendProperty(SBProperties.contentMimeType, mimeType);
				props.appendProperty(SC_listenerCount, numListeners);
				props.appendProperty(SC_id, parseInt(id));
				props.appendProperty(SC_bookmark, heartSrc);
				props.appendProperty(SC_unfave, breakSrc);
				propertiesArray.appendElement(props, false);
				
				trackArray.appendElement(
						ioService.newURI(shoutcastTuneURL + id, null, null),
						false);
			}
			this.radioLib.batchCreateMediaItemsAsync(libListener,
				trackArray, propertiesArray, false);
		}
	},

	inputFilter : function(event) {
		//do this now rather than doing it at every comparison
		var value = event.target.value.toLowerCase();
		if (value == "") {
			this.clearFilter();
			return;
		}
		this.setFilter(value);
	},

	setFilter : function(value) {
		document.getElementById("clearFilter").disabled = value.length == 0;
		var srch = LibraryUtils.createConstraint([[["*",[value]]]]);
		var view = this.playlist.getListView();
		view.searchConstraint = srch;
		Application.prefs.setValue("extensions.shoutcast-radio.filter", value);
	},

	clearFilter : function() {
		document.getElementById("clearFilter").disabled = true;
		document.getElementById("filter").value = "";
		this.playlist.getListView().searchConstraint = null;
		Application.prefs.setValue("extensions.shoutcast-radio.filter", "");
	},
}

function createLibrary(databaseGuid, databaseLocation, init) {
	if (typeof(init) == "undefined")
		init = true;

	var directory;
	if (databaseLocation) {
		directory = databaseLocation.QueryInterface(Ci.nsIFileURL).file;
	} 
	else {
		directory = Cc["@mozilla.org/file/directory_service;1"].
		getService(Ci.nsIProperties).
		get("ProfD", Ci.nsIFile);
		directory.append("db");
	}     

	var file = directory.clone(); 
	file.append(databaseGuid + ".db");
	var libraryFactory =
		Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
		.getService(Ci.sbILibraryFactory);
	var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
		createInstance(Ci.nsIWritablePropertyBag2);
	hashBag.setPropertyAsInterface("databaseFile", file);
	var library = libraryFactory.createLibrary(hashBag);
	try {     
		if (init) {
			library.clear();
		}
	}
	catch(e) {
	}

	if (init) {
		loadData(databaseGuid, databaseLocation);
	} 
	return library;
}

var libListener = {
	onProgress: function(i) {},
	onComplete: function(array, result) {
		// Reset the progress meter
		var el = songbirdMainWindow.document
			.getElementById("sb-status-bar-status-progressmeter");
		el.mode = "";
		SBDataSetStringValue("faceplate.status.text",
				array.length + " " +
				RadioDirectory._strings.getString("stationsFound"));
	}
}

function findRadioNode(node) {
	if (node.isContainer && node.name != null &&
			node.name == RadioDirectory._strings.getString("radioFolderLabel"))
		return node;

	if (node.isContainer) {
		var children = node.childNodes;
		while (children.hasMoreElements()) {
			var child =
					children.getNext().QueryInterface(Ci.sbIServicePaneNode);
			var result = findRadioNode(child);
			if (result != null)
				return result;
		}
	}

	return null;
}

function createFavouritesNode() {
	RadioDirectory.favesList.setProperty(SC_nodeCreated, "1");
	RadioDirectory.favesList.setProperty(SBProperties.hidden, "0");

	// Get our library service pane service
	var librarySPS = Cc['@songbirdnest.com/servicepane/library;1']
			.getService(Ci.sbILibraryServicePaneService);
	var favouritesNode = librarySPS.getNodeForLibraryResource(
			RadioDirectory.favesList);

	// Get our bookmark service
	var BMS = Cc['@songbirdnest.com/servicepane/bookmarks;1'].
			getService(Ci.sbIBookmarks);

	// Get the service pane service
	var SPS = Cc['@songbirdnest.com/servicepane/service;1'].
			getService(Ci.sbIServicePaneService);

	// Find the Radio folder node
	SPS.init();
	var radioNode = findRadioNode(SPS.root);
	if (radioNode == null) {
		alert("FUBAR. Should never be reached.");
		return;
	}

	// Append the node
	radioNode.appendChild(favouritesNode);
	favouritesNode.properties = "medialist-favorites";
	favouritesNode.editable = false;
	favouritesNode.hidden = false;
	SPS.save();
}

function onPlaylistCellClick(e) {
	if (e.getData("property") == SC_bookmark) {
		var item = e.getData("item");

		// See if we have a node for this list already
		if (RadioDirectory.favesList.getProperty(SC_nodeCreated) == "0") {
			createFavouritesNode();
		}
		
		var id = item.getProperty(SC_id);
		var idx = RadioDirectory.favouriteIDs.indexOf(id);
		if (idx != -1) {
			// Already in the favourites list, so remove it
			RadioDirectory.favouriteIDs.splice(idx, 1);
			item.setProperty(SC_bookmark,
					"chrome://shoutcast-radio/skin/invis-16x16.png");
			var tree = document.getElementById("sb-playlist-tree");
			tree.treeBoxObject.clearStyleAndImageCaches();
			tree.treeBoxObject.invalidate();
			for (var i=0; i<RadioDirectory.favesList.length; i++) {
				var item = RadioDirectory.favesList.getItemByIndex(i);
				var thatId = item.getProperty(SC_id);
				if (thatId == id)
					RadioDirectory.favesList.remove(item);
			}
		} else {
			// Add to favourites
			var genreLabel =
					document.getElementById('shoutcast-genre-menulist').label;
			var genreValue =
					document.getElementById('shoutcast-genre-menulist').value;
			if (genreValue != "sbITop")
				item.setProperty(SBProperties.genre, genreLabel);
			RadioDirectory.favouriteIDs.push(id);
			item.setProperty(SC_bookmark,
					"chrome://shoutcast-radio/skin/heart-active.png");
			RadioDirectory.favesList.add(item);
		}
	}
}

function onPlay(e) {
	var item = RadioDirectory.playlist.mediaListView.selection.currentMediaItem;
	var plsURL = ShoutcastRadio.getListenURL(item.getProperty(SC_id));
    var plsMgr = Cc["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
            .getService(Ci.sbIPlaylistReaderManager);
    var listener = Cc["@songbirdnest.com/Songbird/PlaylistReaderListener;1"]
            .createInstance(Ci.sbIPlaylistReaderListener);
    var ioService = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);

	// clear the current list of any existing streams, etc.
	RadioDirectory.streamList.clear();

    listener.playWhenLoaded = true;
	listener.observer = {
		observe: function(aSubject, aTopic, aData) {
			if (aTopic == "success") {
				var list = aSubject;
				var name = item.getProperty(SC_streamName);
				for (var i=0; i<list.length; i++) {
					list.getItemByIndex(i).setProperty(SC_streamName, name);
				}
			}
		}
	}
    var uri = ioService.newURI(plsURL, null, null);
    plsMgr.loadPlaylist(uri, RadioDirectory.streamList, null, false, listener);
}
