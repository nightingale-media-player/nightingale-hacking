// Make a namespace.
if (typeof ShoutcastRadio == 'undefined') {
	var ShoutcastRadio = {};
}

if (typeof Cc == 'undefined')
	var Cc = Components.classes;
if (typeof Ci == 'undefined')
	var Ci = Components.interfaces;

if (typeof SP == 'undefined')
	var SP = "http://songbirdnest.com/rdf/servicepane#";

if (typeof(gMM) == "undefined")
	var gMM = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
		.getService(Ci.sbIMediacoreManager);

if (typeof(gMetrics) == "undefined")
	var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
    		.createInstance(Ci.sbIMetrics);

if (typeof(FAVICON_PATH) == "undefined")
	const FAVICON_PATH = "chrome://shoutcast-radio/skin/shoutcast_favicon.png";

function findRadioNode(node) {
	if (node.isContainer && node.name != null && node.name ==
			ShoutcastRadio.Controller._strings.getString("radioFolderLabel"))
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

var metricsObserver = {
	time : null,
	onMediacoreEvent : function(ev) {
		var item = ev.data;
		if (gMM.sequencer.view == null)
			return;
		var list = gMM.sequencer.view.mediaList;
		switch (ev.type) {
			case Ci.sbIMediacoreEvent.STREAM_START:
				// first we'll get the currently playing media item
				var currentItem = gMM.sequencer.view.getItemByIndex(
						gMM.sequencer.viewPosition);
				
				// check to see if we have an active timer
				if (metricsObserver.time) {
					var now = Date.now()/1000;
					var diff = now - metricsObserver.time;
					gMetrics.metricsAdd("shoutcast", "stream", "time", diff);
				}
				
				// if our new stream we're playing isn't a shoutcast
				// stream then cancel the timer
				if (!currentItem.getProperty(SC_id)) {
					metricsObserver.time = null;
					return;
				}

				// if we're here then we're a shoutcast stream, and we should
				// start a timer
				metricsObserver.time = Date.now()/1000;
				break;
			case Ci.sbIMediacoreEvent.STREAM_END:
			case Ci.sbIMediacoreEvent.STREAM_STOP:
				// check to see if we have an active timer
				if (!metricsObserver.time) {
					metricsObserver.time = null;
					return;
				}
				var now = Date.now()/1000;
				var diff = now - metricsObserver.time;
				gMetrics.metricsAdd("shoutcast", "stream", "time", diff);
				metricsObserver.time = null;
				break;
			default:
				break;
		}
	}
}

/**
 * UI controller that is loaded into the main player window
 */
ShoutcastRadio.Controller = {
	onLoad: function() {
		// initialization code
		this._initialized = true;
		this._strings = document.getElementById("shoutcast-radio-strings");
		
		// Create a service pane node for our chrome
		var SPS = Cc['@songbirdnest.com/servicepane/service;1'].
				getService(Ci.sbIServicePaneService);
		SPS.init();
		var BMS = Cc['@songbirdnest.com/servicepane/bookmarks;1'].
				getService(Ci.sbIBookmarks);
		
		// Walk nodes to see if a "Radio" folder already exists
		var radioFolder = findRadioNode(SPS.root);
		if (radioFolder == null) {
			radioFolder = BMS.addFolder(
					this._strings.getString("radioFolderLabel"));
			radioFolder.editable = false;
		}
	
		// Sort the radio folder node in the service pane
		radioFolder.setAttributeNS(SP, "Weight", 1);
		SPS.sortNode(radioFolder);

		// Bookmark the SHOUTcast chrome
		var bmNode = BMS.addBookmarkAt(
				"chrome://shoutcast-radio/content/directory.xul", "SHOUTcast",
				FAVICON_PATH, radioFolder, null);
		if (bmNode) {
			bmNode.editable = false;
			bmNode.image = FAVICON_PATH;
		} else {
			// bookmark already exists
			var node = SPS.getNodeForURL(
					"chrome://shoutcast-radio/content/directory.xul");
			if (node.image != FAVICON_PATH) {
				node.image = FAVICON_PATH;
			}
		}

		SPS.save();

		// Attach our listener for media core events
		gMM.addListener(metricsObserver);

		// Attach our listener to the ShowCurrentTrack event issued by the
		// faceplate
		var faceplateManager =  Cc['@songbirdnest.com/faceplate/manager;1']
				.getService(Ci.sbIFaceplateManager);
		var pane = faceplateManager.getPane("songbird-dashboard");
		var sbWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
				.getService(Ci.nsIWindowMediator)
				.getMostRecentWindow("Songbird:Main").window;
		sbWindow.addEventListener("ShowCurrentTrack", curTrackListener, true);

		// Create our properties if they don't exist
		var pMgr = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
				.getService(Ci.sbIPropertyManager);
		if (!pMgr.hasProperty(SC_streamName)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
					.createInstance(Ci.sbITextPropertyInfo);
			pI.id = SC_streamName;
			pI.displayName = this._strings.getString("streamName");
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_nodeCreated)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Boolean;1"]
					.createInstance(Ci.sbIBooleanPropertyInfo);
			pI.id = SC_nodeCreated;
			pI.displayName = "Favourites Node Created"; // not user viewable
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_bitRate)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
					.createInstance(Ci.sbINumberPropertyInfo);
			pI.id = SC_bitRate;
			pI.displayName = this._strings.getString("bitRate");
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_comment)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
					.createInstance(Ci.sbITextPropertyInfo);
			pI.id = SC_comment;
			pI.displayName = this._strings.getString("comment");
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_listenerCount)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
					.createInstance(Ci.sbINumberPropertyInfo);
			pI.id = SC_listenerCount;
			pI.displayName = this._strings.getString("listenerCount");
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_bookmark)) {
			var builder = Cc[
				"@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
				.createInstance(Ci.sbIImagePropertyBuilder);
			builder.propertyID = SC_bookmark;
			builder.displayName = this._strings.getString("bookmark");
			builder.userEditable = false;
			builder.userViewable = false;
			var pI = builder.get();
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_id)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
					.createInstance(Ci.sbINumberPropertyInfo);
			pI.id = SC_id;
			pI.displayName = "StreamID";	// shouldn't ever be user-visible
			pI.userEditable = false;
			pI.userViewable = false;
			pMgr.addPropertyInfo(pI);
		}

		// Register our observer for application shutdown
		shoutcastUninstallObserver.register();

		// Bind our dataremote for track title change
		/*
		var SB_NewDataRemote = Components.Constructor(
			"@songbirdnest.com/Songbird/DataRemote;1", Ci.sbIDataRemote,
			"init");
		ShoutcastRadio.Controller.titleDr =
					SB_NewDataRemote("metadata.title", null);
		ShoutcastRadio.Controller.titleDr.bindObserver(
				ShoutcastRadio.Controller, true);
		*/
		ShoutcastRadio.Controller._prefBranch =
				Cc["@mozilla.org/preferences-service;1"]
				.getService(Ci.nsIPrefService).getBranch("songbird.metadata.")
				.QueryInterface(Ci.nsIPrefBranch2);
		ShoutcastRadio.Controller._prefBranch.addObserver("title",
				ShoutcastRadio.Controller.metadataObserver, false);

		// Reset the filter at startup
		Application.prefs.setValue("extensions.shoutcast-radio.filter", "");
	},

	onUnLoad: function() {
		this._initialized = false;
		gMM.removeListener(metricsObserver);
		//ShoutcastRadio.Controller.titleDr.unbind();
	}
}

ShoutcastRadio.Controller.metadataObserver = {
	observe: function(subject, topic, data) {
		var item;
		try {
			item = gMM.sequencer.currentItem;
		} catch (e) {
			return;
		}

		if (subject instanceof Ci.nsIPrefBranch) {
			if (data == "title" && item.getProperty(SC_streamName)) {
				var title = subject.getCharPref(data);
				if (title.indexOf(item.getProperty(SC_streamName)) >= 0) {
					return;
				}
				var m = title.match(/^(.+) - ([^-]+)$/);
				if (m) {
					ShoutcastRadio.Controller.ts = Date.now();
					item.setProperty(SBProperties.artistName, m[1]);
					item.setProperty(SBProperties.trackName, m[2]);
					
					var ev = gMM.createEvent(Ci.sbIMediacoreEvent.TRACK_CHANGE,
							gMM.primaryCore, item);
					gMM.QueryInterface(Ci.sbIMediacoreEventTarget)
							.dispatchEvent(ev);
				}
			}
		}
	}
};

var curTrackListener = function(e) {
	var list;
	var gPPS;
	if (typeof(Ci.sbIMediacoreManager) != "undefined") {
		list = gMM.sequencer.view.mediaList;
	} else {
		gPPS = Cc['@songbirdnest.com/Songbird/PlaylistPlayback;1']
				.getService(Ci.sbIPlaylistPlayback);
		list = gPPS.playingView.mediaList;
	}

	// get the list that owns this guid
	if (list.getProperty(SBProperties.customType) == "radio_tempStreamList") {
		var streamName;
		if (typeof(Ci.sbIMediacoreManager) != "undefined") {
			streamName = gMM.sequencer.view.getItemByIndex(
					gMM.sequencer.viewPosition).getProperty(SC_streamName);
		} else {
			streamName = list.getItemByGuid(gPPS.currentGUID)
					.getProperty(SC_streamName);
		}

		// check to see if this tab is already loaded
		var tabs = gBrowser.mTabs;
		var found = -1;
		var loadURL = "http://shoutcast.com/directory/?s=" + escape(streamName);
		for (var i=0; i<tabs.length; i++) {
			var curBrowser = gBrowser.getBrowserAtIndex(i);
			var loadingURI = curBrowser.userTypedValue;
			var compValue;
			if (loadingURI != null)
				compValue = loadingURI;
			else
				compValue = curBrowser.currentURI.spec;
			if (compValue == loadURL) {
				found = i;
				break;
			}
		}
		if (found != -1) {
			// a tab already exists, so select it
			gBrowser.selectedTab = tabs[found];
		} else {
			// otherwise load a new tab
			gBrowser.loadOneTab(loadURL);
		}

		// prevent the event from bubbling upwards
		e.preventDefault();
	}
}

var shoutcastUninstallObserver = {
	_uninstall : false,
	_tabs : null,

	observe : function(subject, topic, data) {
		if (topic == "em-action-requested") {
			// Extension has been flagged to be uninstalled
			subject.QueryInterface(Ci.nsIUpdateItem);
			if (subject.id == "shoutcast-radio@songbirdnest.com") {
				if (data == "item-uninstalled") {
					this._uninstall = true;
				} else if (data == "item-cancel-action") {
					this._uninstall = false;
				}
			}
		} else if (topic == "quit-application-granted") {
			// We're shutting down, so check to see if we were flagged
			// for uninstall - if we were, then cleanup here
			if (this._uninstall) {
				// Things to cleanup:
				// Service pane node
				var SPS = Cc['@songbirdnest.com/servicepane/service;1'].
						getService(Ci.sbIServicePaneService);

				/* Remove only the Shoutcast node 
				var shoutcastNode = SPS.getNodeForURL(
						"chrome://shoutcast-radio/content/directory.xul");
				SPS.removeNode(shoutcastNode);
				*/

				// Remove the entire Radio node & its children
				var radioFolder = findRadioNode(SPS.root);
				SPS.removeNode(radioFolder);

				// Remove preferences
				var tempLibGuid;
				var radioLibGuid;
				var prefs = Cc["@mozilla.org/preferences-service;1"].
						getService(Components.interfaces.nsIPrefService);
				var scPrefs = prefs.getBranch("extensions.shoutcast-radio.");
				if (scPrefs.prefHasUserValue("plsinit"))
					scPrefs.clearUserPref("plsinit");
				if (scPrefs.prefHasUserValue("filter"))
					scPrefs.clearUserPref("filter");
				if (scPrefs.prefHasUserValue("custom-genres"))
					scPrefs.clearUserPref("custom-genres");
				if (scPrefs.prefHasUserValue("library.guid")) {
					radioLibGuid = scPrefs.getCharPref("library.guid");
					scPrefs.clearUserPref("library.guid");
				}
				if (scPrefs.prefHasUserValue("templib.guid")) {
					tempLibGuid = scPrefs.getCharPref("templib.guid");
					scPrefs.clearUserPref("templib.guid");
				}
				scPrefs.deleteBranch("");
				
				// Unregister radioLib & favourites libraries
				var libMgr = Cc['@songbirdnest.com/Songbird/library/Manager;1']
					.getService(Ci.sbILibraryManager);
				var radioLib = libMgr.getLibrary(radioLibGuid);
				var tempLib = libMgr.getLibrary(tempLibGuid);
				radioLib.clear();
				tempLib.clear();
				libMgr.unregisterLibrary(radioLib);
				libMgr.unregisterLibrary(tempLib);
				dump("unregistered Shoutcast libraries\n");
			}
			this.unregister();
		}
	},

	register : function() {
		var observerService = Cc["@mozilla.org/observer-service;1"]
			.getService(Ci.nsIObserverService);
		observerService.addObserver(this, "em-action-requested", false);
		observerService.addObserver(this, "quit-application-granted", false);
	},

	unregister : function() {
		var observerService = Cc["@mozilla.org/observer-service;1"]
			.getService(Ci.nsIObserverService);
		observerService.removeObserver(this, "em-action-requested");
		observerService.removeObserver(this, "quit-application-granted");
	}
}

window.addEventListener("load",
		function(e) { ShoutcastRadio.Controller.onLoad(e); }, false);
window.addEventListener("unload",
		function(e) { ShoutcastRadio.Controller.onUnLoad(e); }, false);
