if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;

#ifdef METRICS_ENABLED
if (typeof(gMetrics) == "undefined")
  var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
        .createInstance(Ci.sbIMetrics);
#endif

const shoutcastTempLibGuid = "extensions.shoutcast-radio.templib.guid";

if (typeof(songbirdMainWindow) == "undefined")
  var songbirdMainWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow("Songbird:Main").window;

var FavouriteStreams = {
  list : null,
  genres : null,

  listListener : {
    onEnumerationBegin: function(list) {
      FavouriteStreams.genres = new Array();
      return Ci.sbIMediaListEnumerationListener.CONTINUE;
    },
    onEnumeratedItem: function(list, item) {
      item.setProperty(SC_bookmark,
          "chrome://shoutcast-radio/skin/invis-16x16.png");
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

      // OMG. I'm sorry. Forgive me.
      FavouriteStreams.oldOnPlay = songbirdMainWindow
        .gSongbirdPlayerWindow.onPlay;
      window.addEventListener("unload", function() {
        songbirdMainWindow.gSongbirdPlayerWindow.onPlay =
          FavouriteStreams.oldOnPlay;
      }, false);
      songbirdMainWindow.gSongbirdPlayerWindow.onPlay = function() {};

      pls.addEventListener("Play", this.onPlay, false);
      this.updateComments();
    }
  },

  myCellClick : function(e) {
    var prop = e.getData("property");
    var item = e.getData("item");
    if (prop == SC_bookmark) {
#ifdef METRICS_ENABLED
      // # of times a station is unfavourited
      gMetrics.metricsInc("shoutcast", "favourites", "removed");
#endif

      var list =
        document.getElementById("playlist").mediaListView.mediaList;
      list.remove(item);
    }
  },

  onPlay : function(e) {
    var item = document.getElementById("playlist").mediaListView
      .selection.currentMediaItem;
    var id = item.getProperty(SC_id);
    var plsURL = ShoutcastRadio.getListenURL(id);
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
          dump("Success\n");
          var list = aSubject;
          var name = item.getProperty(SC_streamName);
          for (var i=0; i<list.length; i++) {
            listItem = list.getItemByIndex(i);
            listItem.setProperty(SC_streamName, name);
            listItem.setProperty(SC_id, id);
            listItem.setProperty(SBProperties.outerGUID, item.guid);
          }
        } else {
          alert("Failed to load " + item.getProperty(SC_streamName) +
              "\n");
        }
      }
    }

#ifdef METRICS_ENABLED
    // # of times a station is played
    gMetrics.metricsInc("shoutcast", "station", "total.played");

    // # of times this station (ID) is played
    gMetrics.metricsInc("shoutcast", "station", "played." + id.toString());

    // # of times this genre is played
    var genre = item.getProperty(SBProperties.genre);
    gMetrics.metricsInc("shoutcast", "genre", "played." + genre);
#endif

    if (id == -1) {
      plsURL = item.getProperty(SBProperties.contentURL);
    }
    var uri = ioService.newURI(plsURL, null, null);
    plsMgr.loadPlaylist(uri, this.streamList, null, false, listener);
    
    e.stopPropagation();
    e.preventDefault();
  }
}

FavouriteStreams.onLoad();
