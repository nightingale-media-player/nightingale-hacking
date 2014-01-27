try {
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm"); 
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbCoverHelper.jsm");
}
catch (error) {alert("Unity Integration: Unexpected error - module import error\n\n" + error)}

const UNITY_PLAYBACK_STATE_PAUSED = 0;
const UNITY_PLAYBACK_STATE_PLAYING = 1;
const UNITY_PLAYBACK_STATE_STOPED = -1;

if (typeof UnityIntegration == 'undefined') {
	var UnityIntegration = {};
};

UnityIntegration.soundMenu = {
	xulAppInfo: null,
	stringConverter: null,
	gMM: null,
	wm: null,
	unityServiceProxy: null,
	observerService: null,
	mainwindow: null,
	prefs: null,
	lastItem: null,

	onLoad: function () {
		var mainwindow = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIWebNavigation)
								.QueryInterface(Components.interfaces.nsIDocShellTreeItem).rootTreeItem
								.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindow);

		this.xulAppInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
		this.stringConverter = Components.classes['@mozilla.org/intl/scriptableunicodeconverter'].getService(Components.interfaces.nsIScriptableUnicodeConverter);
		this.gMM = Components.classes["@songbirdnest.com/Songbird/Mediacore/Manager;1"].getService(Components.interfaces.sbIMediacoreManager);
		this.wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
		this.mainwindow = this.wm.getMostRecentWindow("Songbird:Main");
		this.prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch("extensions.unity-integration.");
		this.prefs.QueryInterface(Components.interfaces.nsIPrefBranch2);

		UnityIntegration.soundMenu.preferencesObserver.register();

		this.unityServiceProxy = Components.classes["@LookingMan.org/Songbird-Nightingale/UnityProxy;1"].getService(Components.interfaces.IUnityProxy);

		var windowTitle = mainwindow.document.getElementById("mainplayer").getAttribute("title");

		if (this.xulAppInfo.name == "Songbird")
			this.unityServiceProxy.InitializeFor("songbird.desktop", windowTitle);
		else if (this.xulAppInfo.name == "Nightingale")
			this.unityServiceProxy.InitializeFor("nightingale.desktop", windowTitle);
		else {
			alert("Unity Integration: Unexpected error - your application is not supported")
			return;
		}

		UnityIntegration.soundMenu.preferencesObserver.observe(null, "needInit", "enableNotifications");
		UnityIntegration.soundMenu.preferencesObserver.observe(null, "needInit", "hideOnClose");

		this.stringConverter.charset = 'utf-8';
		var mm = this.gMM;
		var that = this;

		var soundMenuObserver = {
			observe : function(subject, topic, data) {
				switch (topic) {
					case "sound-menu-play":
						// If we are already playing something just pause/unpause playback
						var sbIMediacoreStatus = Components.interfaces.sbIMediacoreStatus;
						if (mm.status.state == sbIMediacoreStatus.STATUS_PAUSED) {
							mm.playbackControl.play();
						} else if(mm.status.state == sbIMediacoreStatus.STATUS_PLAYING ||
							mm.status.state == sbIMediacoreStatus.STATUS_BUFFERING) {
							that.unityServiceProxy.SoundMenuSetPlayingState(true);
							// Otherwise dispatch a play event.  Someone should catch this
							// and intelligently initiate playback.  If not, just have
							// the playback service play the default.
						} else {
							var event = document.createEvent("Events");
							event.initEvent("Play", true, true);
							var notHandled = that.mainwindow.dispatchEvent(event);
							if (notHandled) {
								// If we have no context, initiate playback
								// via the root application controller
								var app = Components.classes["@songbirdnest.com/Songbird/ApplicationController;1"]
													.getService(Components.interfaces.sbIApplicationController);
								app.playDefault();
							}
						}
						break;

					case "sound-menu-pause":
						songbird.pause();
						break;

					case "sound-menu-next":
						songbird.next();
						break;

					case "sound-menu-previous":
						songbird.previous();
						break;
				}
			}
		};

		this.observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
		this.observerService.addObserver(soundMenuObserver, "sound-menu-play", false);
		this.observerService.addObserver(soundMenuObserver, "sound-menu-pause", false);
		this.observerService.addObserver(soundMenuObserver, "sound-menu-next", false);
		this.observerService.addObserver(soundMenuObserver, "sound-menu-previous", false);

		this.gMM.addListener({
			onMediacoreEvent : function(event) {
				var item = event.data;
				if (that.gMM.sequencer.view == null) return;
				var list = that.gMM.sequencer.view.mediaList;

				switch (event.type) {
					case Components.interfaces.sbIMediacoreEvent.TRACK_CHANGE:
						if (that.lastItem == that.gMM.sequencer.currentItem) break;
						else that.lastItem = that.gMM.sequencer.currentItem;

						var artist = that.stringConverter.ConvertFromUnicode(that.gMM.sequencer.currentItem.getProperty(SBProperties.artistName));
						var album = that.stringConverter.ConvertFromUnicode(that.gMM.sequencer.currentItem.getProperty(SBProperties.albumName));
						var track = that.stringConverter.ConvertFromUnicode(that.gMM.sequencer.currentItem.getProperty(SBProperties.trackName));
						var coverFilePath = null;

						var resourceURL = that.gMM.sequencer.currentItem.getProperty(SBProperties.primaryImageURL);
						if (resourceURL != null) {
							try {
								var ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
								var fileURI = ios.newURI(decodeURI(resourceURL), null, null).QueryInterface(Components.interfaces.nsIFileURL);
								var fileurl = fileURI.file.path; // code from FolderSync
                				if (fileURI.file.exists())
                  					coverFilePath = fileurl;
							} catch(e) {
									Cu.reportError(e + " " + resourceURL);
							}
						}

						that.unityServiceProxy.SoundMenuSetTrackInfo(track, artist, album, coverFilePath);
						break;

					case Components.interfaces.sbIMediacoreEvent.STREAM_START:
						that.unityServiceProxy.SoundMenuSetPlayingState(UNITY_PLAYBACK_STATE_PLAYING);
						break;

					case Components.interfaces.sbIMediacoreEvent.STREAM_PAUSE:
						that.unityServiceProxy.SoundMenuSetPlayingState(UNITY_PLAYBACK_STATE_PAUSED);
						break;

					case Components.interfaces.sbIMediacoreEvent.STREAM_STOP:
					case Components.interfaces.sbIMediacoreEvent.STREAM_END:
						that.unityServiceProxy.SoundMenuSetPlayingState(UNITY_PLAYBACK_STATE_STOPED);
						break;

					default:
						break;
				}
			}
		});
	},

	onUnLoad: function () {
		UnityIntegration.soundMenu.preferencesObserver.unregister();
	},

	registerOnClose: function (force) {
		if (this.prefs.getBoolPref("hideOnClose")) {
			var that = this;
			this.mainwindow.onclose = function () {
				that.unityServiceProxy.HideWindow(); 
				return false;
			}
		} else if (force) {
			this.mainwindow.onclose = "";
		}
	},

	preferencesObserver: {
		register: function () {
			UnityIntegration.soundMenu.prefs.addObserver("", this, false);
		},

		unregister: function () {
			UnityIntegration.soundMenu.prefs.removeObserver("", this);
		},

		observe: function (aSubject, aTopic, aData) {
			if(aTopic != "nsPref:changed" && aTopic != "needInit") return;

			switch (aData) {
				case "hideOnClose":
					UnityIntegration.soundMenu.registerOnClose(true);
					break;

				case "enableNotifications":
					var enableNotifPref = UnityIntegration.soundMenu.prefs.getBoolPref("enableNotifications");
					var enableNotifTBtn = document.getElementById("unity-integration-enableNotifTButton");
					if (enableNotifPref) {
						enableNotifTBtn.checked = false;
						enableNotifTBtn.setAttribute("tooltiptext", "Notifications enabled");
						enableNotifTBtn.image = "chrome://unity-integration/content/notif-enabled.png";
					} else {
						enableNotifTBtn.checked = true;
						enableNotifTBtn.setAttribute("tooltiptext", "Notifications disabled");
						enableNotifTBtn.image = "chrome://unity-integration/content/notif-disabled.png";
					}
					UnityIntegration.soundMenu.unityServiceProxy.EnableNotifications(enableNotifPref);
			}
		}
	}
};

function onNotifTButtonClick (element) {
	UnityIntegration.soundMenu.prefs.setBoolPref("enableNotifications", !element.checked);
}

window.addEventListener("load",   function(e) { UnityIntegration.soundMenu.onLoad(); },   false);
window.addEventListener("unload", function(e) { UnityIntegration.soundMenu.onUnLoad(); }, false);
