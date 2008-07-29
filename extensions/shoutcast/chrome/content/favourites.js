if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;

const shoutcastTempLibGuid = "extensions.shoutcast-radio.templib.guid";

var FavouriteStreams = {
	list : null,
	genres : null,

	listListener : {
		onEnumerationBegin: function(list) {
			FavouriteStreams.genres = new Array();
			return Ci.sbIMediaListEnumerationListener.CONTINUE;
		},
		onEnumeratedItem: function(list, item) {
			var genre = item.getProperty(SBProperties.genre);
			if (typeof(FavouriteStreams.genres[genre]) == "undefined") {
				// pull genres and update this feed
				FavouriteStreams.updateGenre(genre);
			}
			return Ci.sbIMediaListEnumerationListener.CONTINUE;
		},
		onEnumerationEnd: function(list, status) {
		}
	},

	updateGenre : function(genre) {
		// Get our updated station list for this genre
		var stationList = ShoutcastRadio.getStationList(genre);

		// Grab all stations in our list with this genre
		var ourFavourites = this.list.getItemsByProperty(SBProperties.genre,
				genre);

		for (var i=0; i<ourFavourites.length; i++) {
			var item = ourFavourites.queryElementAt(i, Ci.sbIMediaItem);
			for (var j=0; j<stationList.length; j++) {
				if (stationList[j].getAttribute("id") ==
						item.getProperty(SC_id))
				{
					var comment = stationList[j].getAttribute("ct");
					item.setProperty(SC_comment, comment);
				}
			}
		}
	},

	updateComments : function() {
		this.list = document.getElementById("playlist").mediaListView.mediaList;
		this.list.enumerateAllItems(this.listListener);
	},

	onLoad : function() {
		var pls = document.getElementById("playlist");
		var list = pls.mediaListView.mediaList;
		if (list.getProperty(SBProperties.customType) == "radio_favouritesList")
		{
			pls.addEventListener("PlaylistCellClick", this.myCellClick, false);
			pls.addEventListener("Play", this.onPlay, false);
			this.updateComments();
		}
	},

	myCellClick : function(e) {
		var prop = e.getData("property");
		var item = e.getData("item");
		if (prop == SC_unfave) {
			var list =
				document.getElementById("playlist").mediaListView.mediaList;
			list.remove(item);
		}
	},

	onPlay : function(e) {
		var item = document.getElementById("playlist").mediaListView
			.selection.currentMediaItem;
		var plsURL = ShoutcastRadio.getListenURL(item.getProperty(SC_id));
		var plsMgr = Cc["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
				.getService(Ci.sbIPlaylistReaderManager);
		var listener = Cc["@songbirdnest.com/Songbird/PlaylistReaderListener;1"]
				.createInstance(Ci.sbIPlaylistReaderListener);
		var ioService = Cc["@mozilla.org/network/io-service;1"]
				.getService(Ci.nsIIOService);

		// Get our temporary stream list
		var libGuid = Application.prefs.get(shoutcastTempLibGuid);
		var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
			.getService(Ci.sbILibraryManager);
		this.tempLib = libraryManager.getLibrary(libGuid.value);
		a = this.tempLib.getItemsByProperty(
				SBProperties.customType, "radio_tempStreamList");
		this.streamList = a.queryElementAt(0, Ci.sbIMediaList);

		// clear the current list of any existing streams, etc.
		this.streamList.clear();

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
		plsMgr.loadPlaylist(uri, this.streamList, null, false, listener);
	}
}

FavouriteStreams.onLoad();
