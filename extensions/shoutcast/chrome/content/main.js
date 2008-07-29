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
				"chrome://shoutcast-radio/content/directory.xul",
				"SHOUTcast", "http://shoutcast.com/favicon.ico",
				radioFolder, null);
		if (bmNode) {
			bmNode.editable = false;
			bmNode.image = "http://shoutcast.com/favicon.ico";
		}

		SPS.save();

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
			pI.userViewable = true;
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
			pI.userViewable = true;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_comment)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
					.createInstance(Ci.sbITextPropertyInfo);
			pI.id = SC_comment;
			pI.displayName = this._strings.getString("comment");
			pI.userEditable = false;
			pI.userViewable = true;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_listenerCount)) {
			var pI = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
					.createInstance(Ci.sbINumberPropertyInfo);
			pI.id = SC_listenerCount;
			pI.displayName = this._strings.getString("listenerCount");
			pI.userEditable = false;
			pI.userViewable = true;
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_bookmark)) {
			var builder = Cc[
				"@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
				.createInstance(Ci.sbIImagePropertyBuilder);
			builder.propertyID = SC_bookmark;
			builder.displayName = this._strings.getString("bookmark");
			builder.userEditable = false;
			builder.userViewable = true;
			var pI = builder.get();
			pMgr.addPropertyInfo(pI);
		}
		if (!pMgr.hasProperty(SC_unfave)) {
			var builder = Cc[
				"@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
				.createInstance(Ci.sbIImagePropertyBuilder);
			builder.propertyID = SC_unfave;
			builder.displayName = this._strings.getString("bookmark");
			builder.userEditable = false;
			builder.userViewable = true;
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

		Application.prefs.setValue("extensions.shoutcast-radio.filter", "");
/*
		if (!("sbIAlbumArtService" in Ci)) {
			// Album art manager isn't installed
			alert("Album art manager isn't installed");
			installXPI("http://addons.songbirdnest.com/xpis/969");
		}
		*/
	},

	onUnLoad: function() {
		this._initialized = false;
	},
};

var curTrackListener = function(e){
	var gPPS = Cc['@songbirdnest.com/Songbird/PlaylistPlayback;1']
			.getService(Ci.sbIPlaylistPlayback);

	// get the list that owns this guid
	var list = gPPS.playingView.mediaList;
	if (list.getProperty(SBProperties.customType) == "radio_tempStreamList") {
		var streamName = list.getItemByGuid(gPPS.currentGUID)
				.getProperty(SC_streamName);

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
