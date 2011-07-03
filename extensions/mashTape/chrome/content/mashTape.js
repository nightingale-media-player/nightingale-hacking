const Cc = Components.classes;
const CcID = Components.classesByID;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// Photo image height size for cutoff between small/medium
const PANE_CUTOFF = 275;

// Number of photos to attempt to preload before loading any others
const PHOTO_PRELOAD = 6;

if (typeof(gMM) == "undefined")
  var gMM = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
      .getService(Ci.sbIMediacoreManager);

if (typeof(gBrowser) == "undefined")
  var gBrowser = Cc['@mozilla.org/appshell/window-mediator;1']
        .getService(Ci.nsIWindowMediator).getMostRecentWindow('Songbird:Main')
        .window.gBrowser;

if (typeof(gMetrics) == "undefined")
  var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
        .createInstance(Ci.sbIMetrics);

if (typeof(SBProperties) == "undefined") {
    Cu.import("resource://app/jsmodules/sbProperties.jsm");
    if (!SBProperties)
        throw new Error("Import of sbProperties module failed");
}

if (typeof(mtUtils) == "undefined")
  Cu.import("resource://mashTape/mtUtils.jsm");

if (typeof mashTape == "undefined")
  var mashTape = {
    initialised : false,
    expanded: false,
    height: null,
  }

/*
 * Run once the first time mashTape is loaded - this seeds the metrics
 * counts for the defaults
 */
mashTape.firstRun = function() {
  Application.prefs.setValue("extensions.mashTape.firstrun", false);

  // By default, the info pane is selected, autohide is true, and all
  // individual tabs are enabled
  gMetrics.metricsInc("mashtape", "defaultpane", "info");
  gMetrics.metricsInc("mashtape", "autohide", "enabled");
  gMetrics.metricsInc("mashtape", "info", "tab.disabled");
  gMetrics.metricsInc("mashtape", "review", "tab.disabled");
  gMetrics.metricsInc("mashtape", "rss", "tab.disabled");
  gMetrics.metricsInc("mashtape", "photo", "tab.disabled");
  gMetrics.metricsInc("mashtape", "flash", "tab.disabled");
}

mashTape.log = function(msg) {
  mtUtils.log("mashTape", msg);
}

/*
 * Initialisation routine to get our various providers and initialise our
 * tab panels
 */
mashTape.init = function(e) {
  // remove our listener
  window.removeEventListener("DOMContentLoaded", mashTape.init, false);

  if (Application.prefs.getValue("extensions.mashTape.firstrun", false))
    mashTape.firstRun();

  mashTape.compMgr =
    Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

  mashTape.prevArtist = "";

  mashTape.npDeck = 1;
  var count = Application.prefs.getValue("extensions.mashTape.firstRunCount",0);
  if (count < 10) {
    mashTape.npDeck = 0;
    Application.prefs.setValue("extensions.mashTape.firstRunCount", ++count);
  }
  document.getElementById("mashTape-deck").selectedIndex = mashTape.npDeck;

  // Save a reference to our display pane we were loaded in
	var displayPaneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
			.getService(Ci.sbIDisplayPaneManager);
	var dpInstantiator = displayPaneMgr.getInstantiatorForWindow(window);
	mashTape.displayPane = dpInstantiator.displayPane;

  // Load our strings
  mashTape.strings = Components.classes["@mozilla.org/intl/stringbundle;1"]
    .getService(Components.interfaces.nsIStringBundleService)
    .createBundle("chrome://mashtape/locale/mashtape.properties");

  // Add our listener for location changes
  gBrowser.addProgressListener(mashTape.locationListener,
      Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);

  // Save a reference to the mashTapeManager
  mashTape.mgr = Cc["@songbirdnest.com/mashTape/manager;1"]
    .getService(Ci.sbIMashTapeManager);

  // Setup our preferences observer
  mashTape._prefBranch = Cc["@mozilla.org/preferences-service;1"]
    .getService(Ci.nsIPrefService).getBranch("extensions.mashTape.")
    .QueryInterface(Ci.nsIPrefBranch2);
  mashTape._prefBranch.addObserver("", mashTape.prefObserver, false);

  // class names for our index listings (RSS & Flash)
  mashTape.classes = ["row-odd", "row-even"];

  // Array to hold our mashTape providers
  mashTape.providers = new Array();
  mashTape.providers["info"] = new Array();
  mashTape.providers["review"] = new Array();
  mashTape.providers["rss"] = new Array();
  mashTape.providers["photo"] = new Array();
  mashTape.providers["flash"] = new Array();

  // Keep track of our enabled providers
  mashTape.enabledProviders = new Array();

  var oldProviders = Application.prefs.getValue(
      "extensions.mashTape.providers.all_previous", "").split(",");
  var newProviders = new Array();
  // Scan all XPCOM components to inventory our mashTape providers
  for (var component in Cc) {
    if (component.indexOf("mashTape/provider") >= 0) {
      var mtClass = component.substring(
        component.indexOf("mashTape/provider") + 18, component.length);
      mtClass = mtClass.substring(0, mtClass.indexOf("/"));
      mashTape.providers[mtClass].push(component);
      mashTape.log("Found " + mtClass + " provider: " + component);
      
      var existedPrior = false;
      var clsid = mashTape.compMgr.contractIDToCID(component);
      clsid = clsid.toString();
      newProviders.push(clsid);
      for each (var prev in oldProviders) {
        if (prev == clsid)
          existedPrior = true;
      }
      if (!existedPrior) {
        mashTape.log(component + " is new, auto-enabling");
        mashTape.enableProvider(clsid, mtClass);
      }
    }
  }
  Application.prefs.setValue("extensions.mashTape.providers.all_previous",
      newProviders.join(","));

  // set some shortcuts
  mashTape.infoTab = document.getElementById("mashTape-nav-tab-info");
  mashTape.reviewTab = document.getElementById("mashTape-nav-tab-review");
  mashTape.rssTab = document.getElementById("mashTape-nav-tab-rss");
  mashTape.photoTab = document.getElementById("mashTape-nav-tab-photo");
  mashTape.flashTab = document.getElementById("mashTape-nav-tab-flash");

  // add our listener to change tabs upon radio selection
  var radioGroup = document.getElementById("mashTape-nav-radiogroup");
  radioGroup.addEventListener("click", mashTape.navChange, false);

  // select the default tab
  var defTab = Application.prefs.getValue("extensions.mashTape.defaultpane",
                                          "info");
  radioGroup.selectedItem = document.getElementById("mashTape-nav-tab-"+defTab);
  mashTape.selectPane();
  
  // initialise our auto-hidden value
  mashTape.autoHidden = false;

  // update the nav to reflect enabled/disabled tabs/providers
  mashTape.updateNav();

  mashTape.photoFrame = document.getElementById("mashTape-frame-photo");
  
  gMM.addListener(mashTape);

  var SB_NewDataRemote = Components.Constructor(
      "@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
  mashTape.pausedDr = SB_NewDataRemote("faceplate.paused", null);
  mashTape.pausedDr.bindObserver(mashTape.playingObserver, true);
  
  /*
  if (Application.prefs.getValue("extensions.mashTape.expanded", false))
    mashTape.maximiseDisplayPane(null);
  */

  mashTape.setupAlreadyPlaying();
}

// Switch tabs/decks/panels
mashTape.navChange = function(e) {
  var selectedTab = e.currentTarget.selectedItem.id;
  var newTab = selectedTab.replace(/^mashTape-nav-tab-/, '');
  mashTape.selectPane(newTab);
  e.preventDefault();
  e.stopPropagation();
  mashTape.playPausePhotoListener(newTab == "photo");
  mashTape.selectedTab = newTab;
}

// Update the navigation block (enabling/disabling visibility of headers
// such as 'Reviews' based on whether or not there are providers enabled
// for that header
mashTape.updateNav = function() {
  if (mashTape.providers["info"].length == 0 ||
      !Application.prefs.getValue("extensions.mashTape.info.enabled", true))
    mashTape.infoTab.style.visibility = "collapse";
  else
    mashTape.infoTab.style.visibility = "visible";
  if (mashTape.providers["review"].length == 0 ||
      !Application.prefs.getValue("extensions.mashTape.review.enabled", true))
    mashTape.reviewTab.style.visibility = "collapse";
  else
    mashTape.reviewTab.style.visibility = "visible";
  if (mashTape.providers["rss"].length == 0 ||
      !Application.prefs.getValue("extensions.mashTape.rss.enabled", true))
    mashTape.rssTab.style.visibility = "collapse";
  else
    mashTape.rssTab.style.visibility = "visible";
  if (mashTape.providers["photo"].length == 0 ||
      !Application.prefs.getValue("extensions.mashTape.photo.enabled", true))
    mashTape.photoTab.style.visibility = "collapse";
  else
    mashTape.photoTab.style.visibility = "visible";
  if (mashTape.providers["flash"].length == 0 ||
      !Application.prefs.getValue("extensions.mashTape.flash.enabled", true))
    mashTape.flashTab.style.visibility = "collapse";
  else
    mashTape.flashTab.style.visibility = "visible";
}

// Given a pane, switch the deck to it.  If none was given, then select the
// default one.
mashTape.selectPane = function(pane) {
  var contentDeck = document.getElementById("mashTape-content-deck");
  if (pane == null) {
    pane = Application.prefs.getValue("extensions.mashTape.defaultpane","info");

    // also need to select the tab nav element itself
    document.getElementById("mashTape-nav-radiogroup").selectedItem =
        document.getElementById("mashTape-nav-tab-" + pane);
  }
  switch(pane) {
    case "info":
      if (mashTape.infoTab.style.visibility != "collapse") {
        contentDeck.selectedIndex = 0;
        break;
      }
    case "review":
      if (mashTape.reviewTab.style.visibility != "collapse") {
        contentDeck.selectedIndex = 1;
        break;
      }
    case "rss":
      if (mashTape.rssTab.style.visibility != "collapse") {
        contentDeck.selectedIndex = 2;
        break;
      }
    case "photo":
      if (mashTape.photoTab.style.visibility != "collapse") {
        contentDeck.selectedIndex = 3;
        break;
      }
    case "flash":
      if (mashTape.flashTab.style.visibility != "collapse") {
        contentDeck.selectedIndex = 4;
        break;
      }
    default:
      break;
  }
}

mashTape.unload = function() {
  // remove thyself!
  window.removeEventListener("unload", mashTape.unload, false);

  // clean up other listeners
  gBrowser.removeProgressListener(mashTape.locationListener);
  mashTape._prefBranch.removeObserver("", mashTape.prefObserver);
  gMM.removeListener(mashTape);

  // Save our expanded state to a pref
  Application.prefs.setValue("extensions.mashTape.expanded", mashTape.expanded);
  
  // if we're expanded, then unexpand
  /*
  if (mashTape.expanded)
    mashTape.maximiseDisplayPane(null);
  */

  // destroy the dataremotes
  mashTape.pausedDr.unbind();
}

mashTape.setupAlreadyPlaying = function() {
  if ((gMM.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING) ||
      (gMM.status.state == Ci.sbIMediacoreStatus.STATUS_PAUSED) ||
      (gMM.status.state == Ci.sbIMediacoreStatus.STATUS_BUFFERING)
      )
  {
    mashTape.iframeLoadCount = 0;
    for each (var iframe in document.getElementsByTagName("iframe")) {
      if (typeof(iframe) == "object") {
        iframe.addEventListener("DOMContentLoaded",
                                mashTape.iframeLoadListener, false);
        mashTape.iframeLoadCount++;
      }
    }
  }
}

mashTape.iframeLoadListener = function(e) {
  mashTape.iframeLoadCount--;
  if (mashTape.iframeLoadCount == 0) {
    mashTape.showTabPanels();
    
    // in the event we're already playing a track we should trigger the
    // update, but we need to wait for all the iframe DOMs to finish
    // loading first
    var i = gMM.sequencer.view.getItemByIndex(
          gMM.sequencer.viewPosition);
    var artist = i.getProperty(SBProperties.artistName);
    var album = i.getProperty(SBProperties.albumName);
    var track = i.getProperty(SBProperties.trackName);

    if (artist == null || artist == "") {
      mashTape.noDataTab("info");
      mashTape.noDataTab("review");
      mashTape.noDataTab("rss");
      mashTape.noDataTab("photo");
      mashTape.noDataTab("flash");
    }
    
    mashTape.update(artist, album, track);
  }
}


/*
 * Show all the tab panels, switch to the actual content panes
 */
mashTape.showTabPanels = function() {
  var deck = document.getElementById("mashTape-deck");
  deck.selectedIndex = 2;

  for each (var prov in ["info", "review", "rss", "photo", "flash"]) {
    var provDeck = document.getElementById("mashTape-panel-" + prov);
    provDeck.selectedIndex = 0;
  }
}

mashTape.onMediacoreEvent = function(ev) {
  switch (ev.type) {
    case Ci.sbIMediacoreEvent.TRACK_CHANGE:
      mashTape.onTrackChange(ev.data);
      break;
    case Ci.sbIMediacoreEvent.STREAM_END:
    case Ci.sbIMediacoreEvent.STREAM_STOP:
      // workaround for STREAM_END being fired in between track changes
      // check the media core status to see if it's actually stopped or
      // not.  when STREAM_END is fired in between track changes, the
      // media core status is still playing, not STOPPED
      if (gMM.status.state == Ci.sbIMediacoreStatus.STATUS_STOPPED)
        mashTape.onStop();
      break;
    default:
      break;
  }
}

/*
 * PlaylistPlaybackService observers for track change & stop
 */
// Show the "Nothing playing" stack pane when the user stops playback
mashTape.onStop = function() {
  var deck = document.getElementById("mashTape-deck");
  deck.selectedIndex = mashTape.npDeck;
}

mashTape.onTrackChange = function(item, view, index) {
  mashTape.showTabPanels();
  var artist = item.getProperty(SBProperties.artistName);
  var album = item.getProperty(SBProperties.albumName);
  var track = item.getProperty(SBProperties.trackName);

  if (artist == null || artist == "") {
    mashTape.noDataTab("info");
    mashTape.noDataTab("review");
    mashTape.noDataTab("rss");
    mashTape.noDataTab("photo");
    mashTape.noDataTab("flash");
  } else
    mashTape.update(artist, album, track);
}

mashTape.playingObserver = {
  observe: function(subject, topic, data) {
    if (gMM.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING
        && topic == "faceplate.paused" && data == "0")
    {
      // we were previously paused and we just unpaused
      var detailFrame = document.getElementById("mashTape-panel-flash-detail");
      var videoWindow = detailFrame.contentWindow;
      var event = videoWindow.document.createEvent("Events");
      event.initEvent("songbirdPlaybackResumed", false, false);
      videoWindow.dispatchEvent(event);
    }
  }
}

/*
 * Called when a tab has no given data to display
 */
mashTape.noDataTab = function(providerType) {
  var thisDeck = document.getElementById("mashTape-panel-" + providerType);
  thisDeck.selectedIndex = 1;
  /*
  var panel = mashTape.tabPanels[providerType];
  var deck = mashTape.tabPanels[providerType].firstChild;
  deck.selectedIndex = 0;
  var key = "extensions.mashTape.msg.no_" + providerType + "_found";
  panel.infoMessage.setAttribute("value", mashTape.strings.GetStringFromName(key));
  */
}

/*
 * Check to make sure that all enabled providers are still installed
 */
mashTape.updateEnabledProviders = function(providerType) {
  var prefKey = "extensions.mashTape." + providerType + ".providers";
  mashTape.enabledProviders[providerType] =
      Application.prefs.getValue(prefKey, "").split(",");
  var i=0;
  var changed = false;
  while (i<mashTape.enabledProviders[providerType].length) {
    var clsid = mashTape.enabledProviders[providerType][i];
    if (typeof(CcID[clsid]) == "undefined") {
      // remove this from the enabled providers list
      mashTape.log("provider " + clsid + " is uninstalled, disabling");
      mashTape.enabledProviders[providerType].splice(i, 1);
      changed = true;
    } else {
      i++;
    }
  }
  if (changed)
    Application.prefs.setValue(prefKey,
        mashTape.enabledProviders[providerType].join(","));
}

/*
 * Enable one provider
 */
mashTape.enableProvider = function(clsid, providerType) {
  var prefKey = "extensions.mashTape." + providerType + ".providers";
  mashTape.enabledProviders[providerType] =
      Application.prefs.getValue(prefKey, "").split(",");

  switch (providerType) {
    // multiple providers allowed
    case "rss":
    case "review":
    case "flash":
      mashTape.enabledProviders[providerType].push(clsid);
      Application.prefs.setValue(prefKey,
          mashTape.enabledProviders[providerType].join(","));
      break;
    // only one provider allowed
    case "photo":
    case "info":
      mashTape.enabledProviders[providerType] = new Array(clsid);
      Application.prefs.setValue(prefKey, clsid);
      break;
  }
}

/*
 * The main routine; this gets triggered everytime mashTape should refresh
 * its data and redraw
 */
mashTape.update = function(artist, album, track) {
  mashTape.pendingCallbacks = new Array();
  mashTape.pendingCallbacks["info"] = new Object;
  mashTape.pendingCallbacks["review"] = new Object;
  mashTape.pendingCallbacks["rss"] = new Object;
  mashTape.pendingCallbacks["photo"] = new Object;
  mashTape.pendingCallbacks["flash"] = new Object;
  mashTape.pendingCallbacks["info"].pending = 0;
  mashTape.pendingCallbacks["review"].pending = 0;
  mashTape.pendingCallbacks["rss"].pending = 0;
  mashTape.pendingCallbacks["photo"].pending = 0;
  mashTape.pendingCallbacks["flash"].pending = 0;
  mashTape.pendingCallbacks["info"].valid = 0;
  mashTape.pendingCallbacks["review"].valid = 0;
  mashTape.pendingCallbacks["rss"].valid = 0;
  mashTape.pendingCallbacks["photo"].valid = 0;
  mashTape.pendingCallbacks["flash"].valid = 0;

  // i'm using the artist name as the UID for now, if/when we evolve to
  // having track-specific providers, we'll do something trickier
  // XXX is it time to evolve?
  var uid = encodeURIComponent(artist + album + track);

  if (mashTape.prevArtist != artist) {
    mashTape.resetInfo();
    mashTape.updateEnabledProviders("info");
    if (mashTape.enabledProviders["info"].length > 0 &&
        Application.prefs.getValue("extensions.mashTape.info.enabled",
        true))
    {
      // Since only one info provider is allowed at a time, we'll just
      // grab the first one (since the prefs UI will do the restriction of
      // 'only one' for us
      var clsid = mashTape.enabledProviders["info"][0];
      var infoProvider = CcID[clsid].createInstance(
          Ci.sbIMashTapeInfoProvider);
      var callback = new mashTape.displayCallback(uid);
      mashTape.pendingCallbacks["info"].pending +=
        infoProvider.numSections;
      infoProvider.query(artist, callback);
    }
  }

  if (mashTape.providers["review"].length > 0) {
    mashTape.resetReviewFrame();
    mashTape.updateEnabledProviders("review");
    if (Application.prefs.getValue("extensions.mashTape.review.enabled", true))
    {
      for (var i=0; i<mashTape.enabledProviders["review"].length; i++) {
        var clsid = mashTape.enabledProviders["review"][i];
        var prov = CcID[clsid].createInstance(Ci.sbIMashTapeReviewProvider);
        var callback = new mashTape.displayCallback(uid);
        prov.queryFull(artist, album, track, callback);
        mashTape.pendingCallbacks["review"].pending++;
      }
    }
  }

  mashTape.resetRssFrame();
  mashTape.updateEnabledProviders("rss");
  if (Application.prefs.getValue("extensions.mashTape.rss.enabled", true))
  {
    for (var i=0; i<mashTape.enabledProviders["rss"].length; i++) {
      var clsid = mashTape.enabledProviders["rss"][i];
      var prov = CcID[clsid].createInstance(Ci.sbIMashTapeRSSProvider);
      var callback = new mashTape.displayCallback(uid);
      prov.query(artist, callback);
      mashTape.pendingCallbacks["rss"].pending++;
    }
  }

  if (mashTape.prevArtist != artist || !mashTape.photoFrameLoaded) {
    mashTape.photoFrameLoaded = false;
    mashTape.photosReady = null;
    mashTape.resetPhotoFrame();
    mashTape.updateEnabledProviders("photo");
    if (Application.prefs.getValue("extensions.mashTape.photo.enabled",
          true))
    {
      for (var i=0; i<mashTape.enabledProviders["photo"].length; i++) {
        var clsid = mashTape.enabledProviders["photo"][i];
        var provider = CcID[clsid].createInstance(
          Ci.sbIMashTapePhotoProvider);
        var callback = new mashTape.displayCallback(uid);
        provider.query(artist, callback);
        mashTape.pendingCallbacks["photo"].pending++;
      }
    }
  }

  mashTape.resetFlashFrame();
  mashTape.updateEnabledProviders("flash");
  if (Application.prefs.getValue("extensions.mashTape.flash.enabled", true))
  {
    setTimeout(mashTape.loadFirstFlashFeed, 10000);
    for (var i=0; i<mashTape.enabledProviders["flash"].length; i++) {
      var clsid = mashTape.enabledProviders["flash"][i];
      var prov = CcID[clsid].createInstance(Ci.sbIMashTapeFlashProvider);
      var callback = new mashTape.displayCallback(uid);
      prov.query(artist, callback);
      mashTape.pendingCallbacks["flash"].pending++;
    }
  }

  mashTape.prevArtist = artist;

  /*
  dump("Outgoing callbacks:\n");
  dump("  info  : " + mashTape.pendingCallbacks["info"].pending + "\n");
  dump("  review: " + mashTape.pendingCallbacks["review"].pending + "\n");
  dump("  rss   : " + mashTape.pendingCallbacks["rss"].pending + "\n");
  dump("  photo : " + mashTape.pendingCallbacks["photo"].pending + "\n");
  dump("  flash : " + mashTape.pendingCallbacks["flash"].pending + "\n");
  */
}

mashTape.displayCallback = function(uid) {
  this.wrappedJSObject = this;
  this.uid = uid;
}
mashTape.displayCallback.prototype =  {
  update: function(contractId, results, section) {
    // this happens when the window has been closed (e.g. display pane
    // hidden/unloaded) before a callback has returned
    if (typeof(gMM) == "undefined")
      return;
    var i = gMM.sequencer.view.getItemByIndex(
          gMM.sequencer.viewPosition);
    var thisUID = encodeURIComponent(i.getProperty(SBProperties.artistName)
            + i.getProperty(SBProperties.albumName)
            + i.getProperty(SBProperties.trackName));
    if (this.uid != thisUID) {
      //dump("> callback triggered with a different UID, aborting.\n");
      return;
    }
    var clsid = mashTape.compMgr.contractIDToCID(contractId);
    var provider = Cc[contractId].createInstance(Ci.sbIMashTapeProvider);

    var classPending = mashTape.pendingCallbacks[provider.providerType];
    if (results != null &&
      (provider.providerType == "info" || results.length > 0))
    {
      //if (provider.providerType == "info")
      //  dump("valid data for : " + section + "\n");
      classPending.valid++;
    }
    classPending.pending--;

    /*
    if (provider.providerType == "info") {
      dump(section + " returned\n");
      dump("info pending: " + classPending.pending +
          " / " + classPending.valid + "\n");
    } else if (provider.providerType == "rss") {
      dump("rss: " + provider.providerName + " returned, pending: " +
          classPending.pending + "\n");
    }
    */

    if (classPending.pending == 0 && classPending.valid == 0) {
      // All our pending callbacks completed, let's check to see if
      // we actually got any valid results for this providerType
      mashTape.noDataTab(provider.providerType);
    }

    switch(provider.providerType) {
      case "info":
        mashTape.updateInfo(provider, results, section);
        break;
      case "review":
        var splitter =
            document.getElementById("mashTape-panel-review-splitter");
        mashTape.updateReviewFeeds(provider, results);
        if (results != null &&
              splitter.getAttribute("state") == "collapsed")
        {
          splitter.setAttribute("state", "open");
        }
        if (classPending.pending == 0)
          mashTape.loadFirstReviewFeed()
        break;
      case "rss":
        var splitter = document.getElementById("mashTape-panel-rss-splitter");
        mashTape.updateRssFeeds(provider, results);
        if (results != null &&
              splitter.getAttribute("state") == "collapsed")
        {
          splitter.setAttribute("state", "open");
        }
        if (classPending.pending == 0)
          mashTape.loadFirstRssFeed()
        break;
      case "photo":
        mashTape.updatePhoto(provider, results);
        break;
      case "flash":
        var splitter =
            document.getElementById("mashTape-panel-flash-splitter");
        mashTape.updateFlash(provider, results);
        if (results != null && results.length > 0 && !mashTape.expanded
          && splitter.getAttribute("state") == "collapsed")
        {
          splitter.setAttribute("state", "open");
        }
        if (classPending.pending == 0)
          mashTape.loadFirstFlashFeed()
        break;
      default:
        alert("Unrecognised provider callback: " + providerType);
    }
  }
}

/****************************************************************************
 * ARTIST INFO PROVIDER FUNCTIONS
 ****************************************************************************/
mashTape.resetInfo = function() {
  var infoFrame = document.getElementById("mashTape-frame-info");
  infoFrame.contentWindow.scrollTo(0,0);
  var doc = infoFrame.contentWindow.document;
  var photoDiv = doc.getElementById("artist-photo");
  while (photoDiv.firstChild)
    photoDiv.removeChild(photoDiv.firstChild);
  var discoDiv = doc.getElementById("discography-text");
  while (discoDiv.firstChild)
    discoDiv.removeChild(discoDiv.firstChild);
  var membersDiv = doc.getElementById("members-text");
  while (membersDiv.firstChild)
    membersDiv.removeChild(membersDiv.firstChild);
  var tagsDiv = doc.getElementById("tags-text");
  while (tagsDiv.firstChild)
    tagsDiv.removeChild(tagsDiv.firstChild);
  var linksDiv = doc.getElementById("links-text");
  while (linksDiv.firstChild)
    linksDiv.removeChild(linksDiv.firstChild);
  var bioText = doc.getElementById("bio-text");
  while (bioText.firstChild)
    bioText.removeChild(bioText.firstChild);

  bioText.style.height = "auto";
  discoDiv.style.height = "auto";
  bioText.setAttribute("mashTape-collapsed", true);
  discoDiv.setAttribute("mashTape-collapsed", true);
  doc.getElementById("bio-toggle-text").innerHTML =
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.readmore");
  doc.getElementById("discography-toggle-text").innerHTML =
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.viewmore");

  // Hide sections
  doc.getElementById("bio").style.display = "none";
  doc.getElementById("artist-photo").style.display = "none";
  doc.getElementById("discography").style.display = "none";
  doc.getElementById("members").style.display = "none";
  doc.getElementById("tags").style.display = "none";
  doc.getElementById("links").style.display = "none";
}

mashTape.updateInfo = function(provider, results, section) {
  if (results == null && section != "photo")
    return;
  var infoFrame = document.getElementById("mashTape-frame-info");
  var doc = infoFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];
  
  provider = provider.QueryInterface(Ci.sbIMashTapeInfoProvider);
  var faviconSrc;
  var faviconUrl;
  var faviconTitle;

  switch(section) {
    case "bio":
      doc.getElementById("bio").style.display = "block";
      var bioDiv = doc.getElementById("bio-text");
      faviconSrc = provider.providerIconBio;
      faviconUrl = results.bioUrl;
      
      if (typeof(results.bioText) == "undefined" ||
          results.bioText == null)
      {
        var noBioFound = mashTape.strings.formatStringFromName(
          "extensions.mashTape.info.no_bio", [mashTape.prevArtist],1);
        var bioText = noBioFound;
        if (typeof(results.bioEditUrl) != "undefined")
          bioText += "  <a href='" +
          results.bioEditUrl + "'>" +
          mashTape.strings.GetStringFromName(
            "extensions.mashTape.info.create") + "</a>";
        bioDiv.innerHTML = bioText;
        doc.getElementById("bio-toggle-text").style.display = "none";
        break;
      }
      bioDiv.innerHTML = results.bioText.toString();
      bioDiv.setAttribute("mashTape-full-height", bioDiv.scrollHeight);

      // 98px is the line height (14px) * 7 rows of text.  if we've got more
      // than 7 lines of text, then use the "Read More" link to only show
      // an initial amount
      if (bioDiv.scrollHeight > 98) {
        // determine where to set initial height to (bug 19980)
        var height = 0;
        for (var i=0; i<bioDiv.childNodes.length; i++) {
          if (bioDiv.childNodes[i].offsetTop <= 100)
            height = bioDiv.childNodes[i].offsetTop + 5;
        }
        if (height == 0)
          height = 98;
        doc.getElementById("bio-toggle-text").style.display = "block";
        bioDiv.style.height = height + "px";
      } else
        doc.getElementById("bio-toggle-text").style.display = "none";
  
      // notify listeners
      mashTape.mgr.updateInfo("bio", results.bioText.toString());

      var bioUrl = results.bioUrl;
      if (typeof(bioUrl) != "string")
        bioUrl = bioUrl.toString();
      mashTape.mgr.updateInfo("biourl", bioUrl);
      break;

    case "photo":
      doc.getElementById("artist-photo").style.display = "block";
      var photoDiv = doc.getElementById("artist-photo");
      var artistImage = doc.createElement("img");
      // adjust width
      artistImage.addEventListener("load", function() {
        var width = this.naturalWidth + "px";
        if (this.naturalWidth > 150)
          width = "150px";
        doc.getElementById("artist-photo").style.width = width;
        doc.getElementById("content").style.marginLeft = width;
        
        // notify listeners
        mashTape.mgr.updateInfo("photo", this.src);
      }, false);
      if (results != null)
        artistImage.src = results;
      else {
        artistImage.onerror = function() {
          this.src =
            "chrome://mashtape/skin/generic-artist-photo.jpg";
        }
        artistImage.src =
          "chrome://songbird/skin/artist-info/default-photo.png";
      }
      artistImage.className = "artistImage";
      photoDiv.appendChild(artistImage);
      break;

    case "discography":
      doc.getElementById("discography").style.display = "block";
      var discoDiv = doc.getElementById("discography-text");

      // Sort in chronological order
      results.discography.sort(function(a,b) {
        var a = (a.release_date == null) ? "" : a.release_date;
        var b = (b.release_date == null) ? "" : b.release_date;
        return a < b;
      });

      results.discography.forEach(function(val, i, arr) {
        var albumName = val.title;
        var albumImage = val.artwork;
        var url = val.link;

        var albumDiv = doc.createElement("div");
        albumDiv.className = "discography-album";

        var imageLink = doc.createElement("a");
        if (url)
          imageLink.href = url;
        var image = doc.createElement("img");
        image.className = "album-art";
        image.setAttribute("title", val.tooltip);
        if (albumImage)
          image.src = albumImage;
        else
          image.src = //"chrome://mashtape/skin/no-cover.png";
            "chrome://songbird/skin/album-art/default-cover.png";
        image.width = 64;
        image.height = 64;
        imageLink.appendChild(image);
        albumDiv.appendChild(imageLink);

        var name = doc.createElement("span");
        name.class = "album-name";
        name.innerHTML = albumName;
        if (url) {
          var link = doc.createElement("a");
          link.href = url;
          link.setAttribute("title", val.tooltip);
          link.appendChild(name);
          albumDiv.appendChild(link);
        } else
          albumDiv.appendChild(name);
        
        if (val.release_date != null) {
          var release = doc.createElement("span");
          release.className = "release-date";
          release.innerHTML = "<br />(" +
              val.release_date.substr(0,4) + ")";
          albumDiv.appendChild(release);
        }

        discoDiv.appendChild(albumDiv);
      });

      discoDiv.setAttribute("mashTape-full-height",discoDiv.scrollHeight);
      if (discoDiv.scrollHeight > 70) {
        doc.getElementById("discography-toggle-text").style.display =
          "block";
        discoDiv.style.height = "70px";
      } else
        doc.getElementById("discography-toggle-text").style.display =
          "none";

      faviconSrc = provider.providerIconDiscography;
      faviconUrl = results.url;
      break;

    case "members":
      if (results.members.length == 0)
        return;
      doc.getElementById("members").style.display = "block";
      var membersDiv = doc.getElementById("members-text");
      var list = doc.createElement("ul");
      list.className = "flat";
      results.members.forEach(function(val, i, arr) {
        var li = doc.createElement("li");
        var str = val;
        if (i == arr.length-1)
          li.className = "last";
        li.innerHTML = str;
        list.appendChild(li);
      });
      membersDiv.appendChild(list);
      
      faviconSrc = provider.providerIconMembers;
      faviconUrl = results.url;
      break;

    case "tags":
      doc.getElementById("tags").style.display = "block";
      var tagsDiv = doc.getElementById("tags-text");
      var list = doc.createElement("ul");
      list.className = "flat";
      results.tags.forEach(function(val, i, arr) {
        if (val.count >= 10) {
          var li = doc.createElement("li");
          var link = doc.createElement("a");
          link.href = val.url;
          var tagName = doc.createTextNode(val.name);
          link.appendChild(tagName);
          li.appendChild(link);
          if (i == arr.length-1 ||
            (i < arr.length-1 && arr[i+1].count < 10))
            li.className = "last";
          list.appendChild(li);
        }
      });
      tagsDiv.appendChild(list);
      
      faviconSrc = provider.providerIconTags;
      faviconUrl = results.url;
      break;

    case "links":
      if (results.links.length == 0)
        return;
      doc.getElementById("links").style.display = "block";
      var linksDiv = doc.getElementById("links-text");
      results.links.sort(function(a,b) {
        if (a.name == "Homepage")
          return -1;
        else if (b.name == "Homepage")
          return 1;
        else
          return a.name < b.name;
      });
      var table = doc.createElement("table");
      table.className = "table-links";
      results.links.forEach(function(val, i, arr) {
        var tr = doc.createElement("tr");
        var colName = doc.createElement("td");
        var colUrl = doc.createElement("td");
        colUrl.className = "url";
        var textNode = doc.createElement("span");
        textNode.className = "linkType";
        textNode.innerHTML = decodeURIComponent(val.name) + ":";
        colName.appendChild(textNode);
        var link = doc.createElement("a");
        link.href = val.url;
        link.innerHTML = val.url;
        colUrl.appendChild(link);
        tr.appendChild(colName);
        tr.appendChild(colUrl);
        table.appendChild(tr);
      });
      linksDiv.appendChild(table);

      faviconSrc = provider.providerIconLinks;
      faviconUrl = results.url;
      break;

    default:
      dump("*** updateInfo called for unknown section: "+section+"\n");
  }
  
  if (section != "photo") {
    var favicon = doc.getElementById("favicon-" + section);
    favicon.src = faviconSrc;
    favicon.setAttribute("title", mashTape.strings.formatStringFromName(
        "extensions.mashTape.msg.provider_tooltip", [results.provider],
        1));
    var link = doc.getElementById("link-" + section);
    link.href = faviconUrl;
  }
  return;
}

/****************************************************************************
 * REVIEW PROVIDER FUNCTIONS
 ****************************************************************************/
mashTape.resetReviewFrame = function() {
  // Clear existing data
  var indexFrame = document.getElementById("mashTape-panel-review-index");
  var body = indexFrame.contentWindow.document.getElementsByTagName("body")[0];
  while (body.firstChild)
    body.removeChild(body.firstChild);

  var detailFrame = document.getElementById("mashTape-panel-review-detail");
  var doc = detailFrame.contentWindow.document;

  // Hide any existing content
  var content = doc.getElementById("actual-content");
  content.style.display = "none";

  // Collapse the splitter
  var splitter = document.getElementById("mashTape-panel-review-splitter");
  splitter.setAttribute("state", "collapsed");

  // Put in the loading image for the detail frame
  var loading = doc.getElementById("loading");
  loading.style.display = "block";
  var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;
  // 64 is the height of the load.gif
  loading.style.marginTop = (paneHeight-64)/2 + "px";
}

mashTape.loadFirstReviewFeed = function() {
  var detailFrame = document.getElementById("mashTape-panel-review-detail");
  var indexFrame = document.getElementById("mashTape-panel-review-index");
  // only load if the loading is still showing (meaning the user hasn't
  // clicked on something else to load in the meantime)
  if (detailFrame.contentWindow.document
                 .getElementById("loading").style.display == "block")
  {
    var doc = indexFrame.contentWindow.document;
    var body = doc.getElementsByTagName("body")[0];
    var first = body.getElementsByTagName("div")[0];
    mashTape.loadReviewDetail(first);
  }
}

mashTape.loadReviewDetail = function(entry) {
  if (typeof(entry) == "undefined")
    return;
  while (!entry.hasAttribute("mashTape-title"))
    entry = entry.parentNode;
  var title = entry.getAttribute("mashTape-title");
  var author = entry.getAttribute("mashTape-author");
  var authorUrl = entry.getAttribute("mashTape-authorUrl");
  var src = entry.getAttribute("mashTape-src");
  var time = entry.getAttribute("mashTape-time");
  var url = entry.getAttribute("mashTape-url");
  var favicon = entry.getAttribute("mashTape-favicon");
  var content = entry.getAttribute("mashTape-content");
  var rating = entry.getAttribute("mashTape-rating");

  var detailFrame = document.getElementById("mashTape-panel-review-detail");
  var doc = detailFrame.contentWindow.document;
  detailFrame.contentWindow.scrollTo(0,0);
  var detailTitle = doc.getElementById("title");
  var detailFavicon = doc.getElementById("favicon");
  var detailFaviconLink = doc.getElementById("link");
  var detailSubtitle = doc.getElementById("subtitle");
  var detailContent = doc.getElementById("content");
  var detailMore = doc.getElementById("write-review");

  // hide the loading div
  var loading = doc.getElementById("loading");
  loading.style.display = "none";

  // display the content divs
  var actualContent = doc.getElementById("actual-content");
  actualContent.style.display = "block";

  var containingDiv = entry;
  if (mashTape.selectedReview)
    mashTape.selectedReview.className =
      mashTape.selectedReview.className.replace("row-selected",
                                                "row-unselected");
  while (containingDiv.className.indexOf("row-") != 0)
    containingDiv = containingDiv.parentNode;
  containingDiv.className += " row-selected";
  mashTape.selectedReview = containingDiv;

  detailTitle.innerHTML = title;
  detailFavicon.src = favicon;
  detailFavicon.setAttribute("title", mashTape.strings.formatStringFromName(
      "extensions.mashTape.msg.review_tooltip", [src], 1));
  detailFaviconLink.href = url;

  while (detailSubtitle.firstChild)
    detailSubtitle.removeChild(detailSubtitle.firstChild);
  detailSubtitle.appendChild(doc.createTextNode(
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.by") +" "));
  var provider = doc.createElement("a");
  provider.href = authorUrl;
  provider.innerHTML = author;
  detailSubtitle.appendChild(provider);
  
  if (time > 0) {
    var dateObj = new Date(parseInt(time));
    var timestamp = doc.createElement("span");
    timestamp.className = "time";
    timestamp.innerHTML = dateObj.ago();
    detailSubtitle.appendChild(timestamp);
  }
  
  detailSubtitle.appendChild(doc.createElement("br"));

  if (rating > -1) {
    var ratings = doc.createElement("div");
    ratings.className = "ratings rate" + rating.toString();
    detailSubtitle.appendChild(ratings);
  }


  detailContent.innerHTML = content;
  detailMore.innerHTML = "<a href='" + url + "'>" + 
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.review") +
    "</a>";
}

mashTape.updateReviewFeeds = function(provider, results) {
  if (results == null)
    return;
  var indexFrame = document.getElementById("mashTape-panel-review-index");
  var doc = indexFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];

  var favicon = provider.QueryInterface(Ci.sbIMashTapeReviewProvider)
    .providerIcon;
  for (var i=0; i<results.length; i++) {
    var entryDiv = doc.createElement("div");
    entryDiv.className = "row-unselected";
    var img = doc.createElement("img");
    img.className = "favicon";
    img.src = favicon;
    img.setAttribute("title", provider.providerName);
    entryDiv.appendChild(img);
    
    var metadata = doc.createElement("div");
    metadata.className = "metadata-review";
    var link = doc.createElement("span");
    entryDiv.setAttribute("mashTape-title", results[i].title);
    entryDiv.setAttribute("mashTape-favicon", favicon);
    entryDiv.setAttribute("mashTape-author", results[i].author);
    entryDiv.setAttribute("mashTape-authorUrl", results[i].authorUrl);
    entryDiv.setAttribute("mashTape-src", results[i].provider);
    entryDiv.setAttribute("mashTape-url", results[i].url);
    entryDiv.setAttribute("mashTape-time", results[i].time);
    entryDiv.setAttribute("mashTape-rating", results[i].rating);
    entryDiv.setAttribute("mashTape-content", results[i].content);
    entryDiv.addEventListener("click", function(e) {
      mashTape.loadReviewDetail(e.target);
      e.stopPropagation();
      e.preventDefault();
    }, false);
    link.innerHTML = results[i].title;
    metadata.appendChild(link);

    if (results[i].time > 0) {
      var dateObj = new Date(results[i].time);
      var dateSpan = doc.createElement("span");
      dateSpan.className = "time";
      dateSpan.innerHTML = dateObj.ago();
      metadata.appendChild(dateSpan);
    }
    
    if (results[i].rating > -1) {
      var ratings = doc.createElement("div");
      ratings.className = "ratings rate" + results[i].rating.toString();
      metadata.appendChild(ratings);
    }

    entryDiv.appendChild(metadata);

    // Figure out where in the DOM we want to insert this node
    entryDiv.setAttribute("mashTape-timestamp", results[i].time);
    entryDiv.setAttribute("mashTape-title", results[i].title);
    var divs = body.getElementsByTagName("div");
    var inserted = false;
    //var nextClass = 0;
    for (var j=0; j<divs.length; j++) {
      if (divs[j].className.indexOf("row-") == -1)
        continue;
      //divs[j].className = mashTape.classes[nextClass];
      //nextClass = Math.abs(nextClass-1);

      var otherTimestamp = divs[j].getAttribute("mashTape-timestamp");
      if ((otherTimestamp < results[i].time && !inserted &&
            otherTimestamp > 0)
        || (results[i].time == 0 && !inserted))
      {
        /*
        if (divs[j].className == "row-even") {
          entryDiv.className = "row-even";
          divs[j].className = "row-odd";
        } else {
          entryDiv.className = "row-odd";
          divs[j].className = "row-even";
        }
        */
        body.insertBefore(entryDiv, divs[j]);
        inserted = true;
      }
    }
    if (!inserted) {
      //entryDiv.className = mashTape.classes[nextClass];
      body.appendChild(entryDiv);
    }
  }
}

/****************************************************************************
 * RSS PROVIDER FUNCTIONS
 ****************************************************************************/
mashTape.resetRssFrame = function() {
  // Clear existing data
  var indexFrame = document.getElementById("mashTape-panel-rss-index");
  var detailFrame = document.getElementById("mashTape-panel-rss-detail");
  var doc = indexFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];
  while (body.firstChild)
    body.removeChild(body.firstChild);

  doc = detailFrame.contentWindow.document;

  // Hide any existing content
  var content = doc.getElementById("actual-content");
  content.style.display = "none";

  // Collapse the splitter
  var splitter = document.getElementById("mashTape-panel-rss-splitter");
  splitter.setAttribute("state", "collapsed");

  // Put in the loading image for the detail frame
  var loading = doc.getElementById("loading");
  loading.style.display = "block";
  var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;
  // 64 is the height of the load.gif
  loading.style.marginTop = (paneHeight-64)/2 + "px";
}

mashTape.loadFirstRssFeed = function() {
  // only load if the loading is still showing (meaning the user hasn't
  // clicked on something else to load in the meantime)
  var indexFrame = document.getElementById("mashTape-panel-rss-index");
  var detailFrame = document.getElementById("mashTape-panel-rss-detail");
  if (detailFrame.contentWindow.document
                 .getElementById("loading").style.display == "block")
  {
    var doc = indexFrame.contentWindow.document;
    var body = doc.getElementsByTagName("body")[0];
    var first = body.getElementsByTagName("div")[0];
    mashTape.loadRssDetail(first);
  }
}

mashTape.loadRssDetail = function(entry) {
  if (typeof(entry) == "undefined")
    return;
  while (!entry.hasAttribute("mashTape-title"))
    entry = entry.parentNode;
  var title = entry.getAttribute("mashTape-title");
  var src = entry.getAttribute("mashTape-src");
  var srcUrl = entry.getAttribute("mashTape-srcUrl");
  var time = entry.getAttribute("mashTape-time");
  var url = entry.getAttribute("mashTape-url");
  var favicon = entry.getAttribute("mashTape-favicon");
  var content = entry.getAttribute("mashTape-content");

  var detailFrame = document.getElementById("mashTape-panel-rss-detail");
  var doc = detailFrame.contentWindow.document;
  detailFrame.contentWindow.scrollTo(0,0);
  var detailTitle = doc.getElementById("title");
  var detailFavicon = doc.getElementById("favicon");
  var detailFaviconLink = doc.getElementById("link");
  var detailSubtitle = doc.getElementById("subtitle");
  var detailContent = doc.getElementById("content");
  var detailMore = doc.getElementById("read-more");

  // hide the loading div
  var loading = doc.getElementById("loading");
  loading.style.display = "none";

  // display the content divs
  var actualContent = doc.getElementById("actual-content");
  actualContent.style.display = "block";

  var containingDiv = entry;
  if (mashTape.selectedRss)
    mashTape.selectedRss.className =
      mashTape.selectedRss.className.replace("row-selected", "row-unselected");
  while (containingDiv.className.indexOf("row-") != 0)
    containingDiv = containingDiv.parentNode;
  containingDiv.className += " row-selected";
  mashTape.selectedRss = containingDiv;

  detailTitle.innerHTML = title;
  detailFavicon.src = favicon;
  detailFavicon.setAttribute("title", mashTape.strings.formatStringFromName(
      "extensions.mashTape.msg.rss_tooltip", [src], 1));
  detailFaviconLink.href = url;

  while (detailSubtitle.firstChild)
    detailSubtitle.removeChild(detailSubtitle.firstChild);
  detailSubtitle.appendChild(doc.createTextNode(
      mashTape.strings.GetStringFromName("extensions.mashTape.msg.by") + " "));
  var provider = doc.createElement("a");
  provider.href = srcUrl;
  provider.innerHTML = src;
  detailSubtitle.appendChild(provider);
  var dateObj = new Date(parseInt(time));
  var timestamp = doc.createElement("span");
  timestamp.className = "time";
  timestamp.innerHTML = dateObj.ago();
  detailSubtitle.appendChild(timestamp);

  detailContent.innerHTML = content;
  detailMore.innerHTML = "<a href='" + url + "'>" + 
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.readorig") + "</a>";

}

mashTape.updateRssFeeds = function(provider, results) {
  if (results == null)
    return;
  var indexFrame = document.getElementById("mashTape-panel-rss-index");
  var doc = indexFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];

  var favicon = provider.QueryInterface(Ci.sbIMashTapeRSSProvider)
    .providerIcon;
  for (var i=0; i<results.length; i++) {
    var entryDiv = doc.createElement("div");
    entryDiv.className = "row-unselected";
    var img = doc.createElement("img");
    img.className = "favicon";
    img.src = favicon;
    img.setAttribute("title", provider.providerName);
    entryDiv.appendChild(img);
    
    var metadata = doc.createElement("div");
    metadata.className = "metadata-rss";
    var link = doc.createElement("span");
    entryDiv.setAttribute("mashTape-title", results[i].title);
    entryDiv.setAttribute("mashTape-favicon", favicon);
    entryDiv.setAttribute("mashTape-src", results[i].provider);
    entryDiv.setAttribute("mashTape-srcUrl", results[i].providerUrl);
    entryDiv.setAttribute("mashTape-time", results[i].time);
    entryDiv.setAttribute("mashTape-url", results[i].url);
    entryDiv.setAttribute("mashTape-content", results[i].content);
    entryDiv.addEventListener("click", function(e) {
      mashTape.loadRssDetail(e.target);
      e.stopPropagation();
      e.preventDefault();
    }, false);
    link.innerHTML = results[i].title;
    metadata.appendChild(link);

    var dateObj = new Date(results[i].time);
    var dateSpan = doc.createElement("span");
    dateSpan.className = "time";
    dateSpan.innerHTML = dateObj.ago();
    metadata.appendChild(dateSpan);
    if (results[i].content.indexOf("<img") >= 0) {
      var img = doc.createElement("img");
      img.src = "chrome://mashtape/skin/photos.png";
      img.setAttribute("title", mashTape.strings.GetStringFromName(
          "extensions.mashTape.rss.photo"));
      img.className = "legend";
      metadata.appendChild(img);
    }
    if (results[i].content.indexOf("<object") >= 0 ||
          results[i].content.indexOf("<embed") >= 0) {
      var img = doc.createElement("img");
      img.src = "chrome://mashtape/skin/video.png";
      img.setAttribute("title", mashTape.strings.GetStringFromName(
          "extensions.mashTape.rss.video"));
      img.className = "legend";
      metadata.appendChild(img);
    }
    entryDiv.appendChild(metadata);

    // Figure out where in the DOM we want to insert this node
    entryDiv.setAttribute("mashTape-timestamp", results[i].time);
    entryDiv.setAttribute("mashTape-title", results[i].title);
    var divs = body.getElementsByTagName("div");
    var inserted = false;
    var nextClass = 0;
    for (var j=0; j<divs.length; j++) {
      if (divs[j].className.indexOf("row-") == -1)
        continue;
      //divs[j].className = mashTape.classes[nextClass];
      nextClass = Math.abs(nextClass-1);

      var otherTimestamp = divs[j].getAttribute("mashTape-timestamp");
      if (otherTimestamp < results[i].time && !inserted) {
        /*
        if (divs[j].className == "row-even") {
          entryDiv.className = "row-even";
          divs[j].className = "row-odd";
        } else {
          entryDiv.className = "row-odd";
          divs[j].className = "row-even";
        }
        */
        body.insertBefore(entryDiv, divs[j]);
        inserted = true;
      }
    }
    if (!inserted) {
      //entryDiv.className = mashTape.classes[nextClass];
      body.appendChild(entryDiv);
    }
  }
}

/****************************************************************************
 * PHOTO PROVIDER FUNCTIONS
 ****************************************************************************/
mashTape.resetPhotoFrame = function() {
  // delete the iframe child, create a new one, and reinsert it
  // before the no-data-found hbox
  var photoPanel = document.getElementById("mashTape-panel-photo");
  var iframe = document.getElementById("mashTape-frame-photo");
  photoPanel.removeChild(iframe);

  var iframe = document.createElement("iframe");
  iframe.id = "mashTape-frame-photo";
  iframe.className = "content-frame";
  iframe.setAttribute("flex", 1);
  iframe.setAttribute("tooltip", "aHTMLTooltip");
  iframe.setAttribute("src","chrome://mashtape/content/iframePhoto.html");

  iframe.addEventListener("DOMContentLoaded", mashTape.photoLoadListener,false);
  photoPanel.insertBefore(iframe, photoPanel.firstChild);
  mashTape.photoFrame = iframe;
}
mashTape.photoLoadListener = function(e) {
  var iframe = e.currentTarget;
  iframe.removeEventListener("DOMContentLoaded",
      mashTape.photoLoadListener, false);

  // Put in the loading image for the photo frame
  var doc = mashTape.photoFrame.contentWindow.document;
  var loading = doc.getElementById("loading");
  loading.style.display = "block";
  var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;
  // 32 is the height of the load.gif
  loading.style.marginTop = (paneHeight-32)/2 + "px";
  mashTape.photoFrameLoaded = true;
  if (mashTape.photosReady != null)
    mashTape.drawPhotoStream(mashTape.photosReady.provider,
        mashTape.photosReady.results);
}

mashTape.updatePhoto = function(provider, results) {
  if (!mashTape.photoFrameLoaded) {
    mashTape.photosReady = {
      provider : provider,
      results : results,
    }
  } else
    mashTape.drawPhotoStream(provider, results);

  // trigger mashTapeListener updates
  var photos = new Array();
  for (var i=0; i<results.length; i++) {
    var result = results[i];
    photos.push({
      imageUrl : result.medium,
      imageTitle : result.title,
      authorName : result.owner,
      authorUrl : result.ownerUrl,
      timestamp: result.time
    });
  }

  mashTape.mgr.updatePhotos(photos, photos.length);
}

mashTape.drawPhotoStream = function(provider, results) {
  var doc = mashTape.photoFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];

  var paneWidth = doc.width;
  var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;

  // hide the loading graphic
  var loading = doc.getElementById("loading");
  loading.style.display = "none";

  // unhide the buttons
  doc.getElementById("buttons").style.visibility = "visible";

  var images = doc.getElementById("box");

  // Set the mask height to the height of the display pane
  doc.getElementById("mask").style.height = paneHeight + "px";
  doc.getElementById("mask").style.display = "block";

  // Add resize handler for photostream
  mashTape.photoFrame.contentWindow.addEventListener("resize", function(e) {
    e.preventDefault();
    e.stopPropagation();

    var mTWindow = mashTape.photoFrame.contentWindow;

    var nS5 = mTWindow.nS5;
    //nS5.stop();

    var frameWidth = doc.getElementsByTagName("html")[0].clientWidth;
    var frameHeight = doc.getElementsByTagName("html")[0].clientHeight;
    //dump("New frame size: " + frameWidth + "x" + frameHeight + "\n");

    doc.getElementById("mask").style.height = frameHeight + "px";
    var imageElements = images.getElementsByTagName("img");
    for (var i=0; i<imageElements.length; i++) {
      var img = imageElements[i];
      //dump("img " + i + " - " + img.width + "x" + img.height + "\n");
      var ratio = img.height / frameHeight;
      img.height = frameHeight;
      
      // Check to see if images should be reloaded, only if we loaded
      // the small images, and our resized height is greater than the
      // display pane height cutoff
      if (mTWindow.imageItems[i].small == img.src &&
        frameHeight > PANE_CUTOFF)
      {
        img.src = mTWindow.imageItems[i].medium;
      }
    }

    // Offset recalculation for the photostream
    if (typeof(nS5) == "undefined")
      return;
    var current = nS5.currentIndex;
    //dump("current index: " + current + "\n");

    // Invalidate offsets for all subsequent images
    for (var i=current; i<imageElements.length; i++)
      mTWindow.imageItems[i].offset = null;

    // Recalculate offsets for all photos from start to current
    mTWindow.imageItems[0].offset = 0;
    for (var i=1; i<=current; i++) {
      var previousOffset = mTWindow.imageItems[i-1].offset;
      var currentOffset = previousOffset - imageElements[i-1].width;
      //dump("reset: "+ (i-1).toString() + " = "+ previousOffset + "\n");
      mTWindow.imageItems[i].offset = currentOffset;
    }

    // Reset the stream
    mTWindow.nS5.gotoSlide(current);

  }, false);

  // Build our iframe's JS image array
  mashTape.photoFrame.contentWindow.imageItems = new Array();
  if (results.length > PHOTO_PRELOAD)
    mashTape.imageLoadCount = PHOTO_PRELOAD;
  else
    mashTape.imageLoadCount = results.length;
  mashTape.imageResults = results;
  mashTape.photoStreamReady = false;
  mashTape.photoStreamRunning = false;
  for (var i=0; i<mashTape.imageLoadCount; i++) {
    var item = results[i];
    var imgcontainer = doc.createElement("span");
    var img = doc.createElement("img");
    // add a load listener for the first N items so we don't start
    // scrolling and loading the rest until those N are loaded
    img.addEventListener("load", mashTape.imageLoadListener, false);
    if (paneHeight < PANE_CUTOFF)
      img.src = item.small;
    else
      img.src = item.medium;
    img.setAttribute("mashTape-href", item.url);
    img.addEventListener("click", function() {
      mashTape.photoFrame.contentWindow.mashTape_openLink(
        this.getAttribute("mashTape-href"));
    }, false);
    img.height = paneHeight;
    imgcontainer.appendChild(img);
    images.appendChild(imgcontainer);

    var title = item.title.replace(/'/g, "\\'");
    var author = item.owner.replace(/'/g, "\\'");
    var timestamp = item.time;
    mashTape.photoFrame.contentWindow.imageItems.push({
      title: title,
      author: author,
      authorUrl: item.ownerUrl,
      date: timestamp,
      el: img,
      link: item.url,
      small: item.small,
      medium: item.medium
    });
  }
}

mashTape.imageLoadListener = function(e) {
  e.target.removeEventListener("load", mashTape.imageLoadListener, false);
  mashTape.imageLoadCount--;
  var doc = mashTape.photoFrame.contentWindow.document;

  if (mashTape.imageLoadCount > 0)
    return;

  // once we hit 0, we've loaded all our initial batch of images
  // check to see if we have remaining images we should start queuing up
  // to be loaded
  if (mashTape.imageResults.length > PHOTO_PRELOAD) {
    var images = doc.getElementById("box");
    var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;
    
    for (var i=PHOTO_PRELOAD; i<mashTape.imageResults.length; i++) {
      var item = mashTape.imageResults[i];
      var imgcontainer = doc.createElement("span");
      var img = doc.createElement("img");
      if (paneHeight < PANE_CUTOFF)
        img.src = item.small;
      else
        img.src = item.medium;
      img.setAttribute("mashTape-href", item.url);
      img.addEventListener("click", function() {
        mashTape.photoFrame.contentWindow.mashTape_openLink(
          this.getAttribute("mashTape-href"));
      }, false);
      img.height = paneHeight;
      imgcontainer.appendChild(img);
      images.appendChild(imgcontainer);

      mashTape.photoFrame.contentWindow.imageItems.push({
        title: item.title.replace(/'/g, "\\'"),
        author: item.owner.replace(/'/g, "\\'"),
        authorUrl: item.ownerUrl,
        date: item.time,
        el: img,
        link: item.url,
        small: item.small,
        medium: item.medium
      });
    }
  }

  // if the Photos tab is currently focused, then start the photo stream
  // otherwise add a listener to delay triggering the photo stream until
  // the tab is in focus
  if (document.getElementById("mashTape-nav-radiogroup").selectedItem ==
      document.getElementById("mashTape-nav-tab-photo"))
  {
    mashTape.photoFrame.contentWindow.mashTape_triggerPhotoStream(
        mashTape.photoFrame.contentWindow.imageItems, doc.width);
  } else {
    mashTape.photoStreamReady = true;
    mashTape.photoStreamRunning = false;
  }
}

mashTape.playPausePhotoListener = function(inPhotoTab) {
  if (inPhotoTab) {
    // switched to the photo tab, need to resume or start it
    if (!mashTape.photoStreamRunning) {
      mashTape.photoStreamRunning = true;
      var doc = mashTape.photoFrame.contentWindow.document;
      mashTape.photoFrame.contentWindow.mashTape_triggerPhotoStream(
          mashTape.photoFrame.contentWindow.imageItems, doc.width);
    } else {
      var mTWindow = mashTape.photoFrame.contentWindow;
      mTWindow.nS5.playpause(mTWindow.nS5.interval, 'next');
    }
  } else if (mashTape.selectedTab == "photo") {
    // previously selected tab was the photo tab, need to pause it
    var mTWindow = mashTape.photoFrame.contentWindow;
    mTWindow.nS5.playpause(mTWindow.nS5.interval, 'next');
  }
}

/****************************************************************************
 * FLASH PROVIDER FUNCTIONS
 ****************************************************************************/
mashTape.resetFlashFrame = function() {
  // Reset the index frame contents
  var indexFrame = document.getElementById("mashTape-panel-flash-index");
  var detailFrame = document.getElementById("mashTape-panel-flash-detail");
  var doc = indexFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];
  while (body.firstChild)
    body.removeChild(body.firstChild);

  doc = detailFrame.contentWindow.document;

  // Hide any existing content
  var content = doc.getElementById("actual-content");
  content.style.display = "none";
  
  // Collapse the splitter
  var splitter = document.getElementById("mashTape-panel-flash-splitter");
  splitter.setAttribute("state", "collapsed");

  // Put in the loading image for the detail frame
  var loading = doc.getElementById("loading");
  loading.style.display = "block";
  var paneHeight = doc.getElementsByTagName("html")[0].clientHeight;
  // 32 is the height of the load.gif
  loading.style.marginTop = (paneHeight-32)/2 + "px";
}

mashTape.durationFromSeconds = function(seconds) {
  // Calculate the duration
  var minutes = parseInt(seconds / 60);
  seconds = seconds % 60;
  var hours = parseInt(minutes / 60);
  minutes = minutes % 60;

  var str = seconds;
  if (parseInt(seconds) < 10)
    str = "0" + seconds.toString();
  if (minutes > 0 || hours > 0) {
    if (minutes < 10)
      minutes = "0" + minutes.toString();
    str = minutes + ":" + str;
    if (hours > 0)
      str = hours.toString() + ":" + str;
  }
  return (str);
}

mashTape.loadFirstFlashFeed = function() {
  // only load if the loading is still showing (meaning the user hasn't
  // clicked on something else to load in the meantime)
  var indexFrame = document.getElementById("mashTape-panel-flash-index");
  var detailFrame = document.getElementById("mashTape-panel-flash-detail");
  if (detailFrame.contentWindow.document
                 .getElementById("loading").style.display == "block")
  {
    var doc = indexFrame.contentWindow.document;
    var body = doc.getElementsByTagName("body")[0];
    // Only load something if we have something
    if (body.getElementsByTagName("div").length <= 0) {
      mashTape.noDataTab("flash");
      return;
    }
    var first = body.getElementsByTagName("div")[0];
    mashTape.loadFlashDetail(first);
  }
}


mashTape.loadFlashDetail = function(el) {
  if (typeof(el) == "undefined")
    return;
  var title = el.getAttribute("mashTape-title");
  var time = el.getAttribute("mashTape-time");
  var url = el.getAttribute("mashTape-url");
  var favicon = el.getAttribute("mashTape-favicon");
  var swfUrl = el.getAttribute("mashTape-video");
  var description = el.getAttribute("mashTape-description");
  var author = el.getAttribute("mashTape-author");
  var authorUrl = el.getAttribute("mashTape-author-url");
  var duration = el.getAttribute("mashTape-duration");
  var width = el.getAttribute("mashTape-width");
  var ratio = el.getAttribute("mashTape-ratio");
  var height = el.getAttribute("mashTape-height");
  var provider = el.getAttribute("mashTape-provider");
  var flashVars;
  if (el.hasAttribute("mashTape-flashvars"))
    flashVars = el.getAttribute("mashTape-flashvars");

  /* Add privileged JS to the remote window DOM */
  var indexFrame = document.getElementById("mashTape-panel-flash-index");
  var detailFrame = document.getElementById("mashTape-panel-flash-detail");
  var flashWindow = detailFrame.contentWindow;
  var doc = detailFrame.contentWindow.document;

  var script = doc.createElement("script");
  script.setAttribute("src", "chrome://mashtape/content/iframe-flash.js");
  doc.body.appendChild(script);

  doc.setUserData("mashTapeRatio", ratio, null);

  function openLink(e) {
    e.preventDefault();
    e.stopPropagation();
    var target = e.target;
    while (target != null && !target.href) {
      target = target.parentNode;
    }
    if (target == null)
      return;
    if (target.href) {
      Components.classes['@mozilla.org/appshell/window-mediator;1']
      .getService(Components.interfaces.nsIWindowMediator)
      .getMostRecentWindow('Songbird:Main').gBrowser
      .loadOneTab(target.href);
    }
  }

  if (typeof(mashTape.flashListenerAdded) == "undefined") {
    flashWindow.addEventListener('click', openLink, false);
    mashTape.flashListenerAdded = true;
  }

  var detailVideo = doc.getElementById("video");
  var detailFavicon = doc.getElementById("favicon");
  var detailFaviconLink = doc.getElementById("link");
  var detailTitle = doc.getElementById("title");
  var detailTime = doc.getElementById("time");
  var detailAuthor = doc.getElementById("author");
  var detailDescr = doc.getElementById("description");

  // hide the loading div
  var loading = doc.getElementById("loading");
  loading.style.display = "none";

  // display the content divs
  var actualContent = doc.getElementById("actual-content");
  actualContent.style.display = "block";
  doc.getElementById("caption").href = url;

  var containingDiv = el;
  if (mashTape.selectedFlash)
    mashTape.selectedFlash.className =
      mashTape.selectedFlash.className.replace("row-selected","");
  while (containingDiv.className.indexOf("row-") != 0)
    containingDiv = containingDiv.parentNode;
  containingDiv.className += " row-selected";
  mashTape.selectedFlash = containingDiv;

  // Reset some divs
  while (detailTitle.firstChild)
    detailTitle.removeChild(detailTitle.firstChild);
  while (detailAuthor.firstChild)
    detailAuthor.removeChild(detailAuthor.firstChild);
  while (detailVideo.firstChild)
    detailVideo.removeChild(detailVideo.firstChild);

  // Set the video
  var swf = doc.createElement("object");
  swf.setAttribute("data", swfUrl);

  mashTape.log("Loading movie from: " + swfUrl, true);

  swf.setAttribute("type", "application/x-shockwave-flash");
  swf.setAttribute("id", "mTFlashObject");
  swf.setAttribute("name", "mTFlashObject");
  swf.setAttribute("width", parseInt(width));
  swf.setAttribute("height",  parseInt(height));
  if (el.hasAttribute("mashTape-flashvars"))
    swf.setAttribute("flashVars", flashVars);
  swf.setAttribute("mashTape-provider", provider);

  var param = doc.createElement("param");
  param.setAttribute("name", "allowScriptAccess");
  param.setAttribute("value", "always");
  swf.appendChild(param);

  param = doc.createElement("param");
  param.setAttribute("name", "quality");
  param.setAttribute("value", "high");
  swf.appendChild(param);

  param = doc.createElement("param");
  param.setAttribute("name", "allowFullScreen");
  param.setAttribute("value", "true");
  swf.appendChild(param);

  param = doc.createElement("param");
  param.setAttribute("name", "bgcolor");
  param.setAttribute("value", "#000000");
  swf.appendChild(param);

  detailVideo.appendChild(swf);
  
  // Set the title
  var detailLink = doc.createElement("a");
  detailLink.href = url;
  //detailLink.appendChild(doc.createTextNode(title));
  detailLink.innerHTML = title;
  detailTitle.appendChild(detailLink);
  
  // Set the favicon, & duration
  detailFavicon.src = favicon;
  detailFavicon.setAttribute("title", mashTape.strings.formatStringFromName(
      "extensions.mashTape.msg.flash_tooltip", [provider], 1));
  detailFaviconLink.href = url;
  if (duration > 0)
    detailTime.innerHTML = mashTape.durationFromSeconds(duration);

  // Set the author information
  detailAuthor.appendChild(doc.createTextNode(
    mashTape.strings.GetStringFromName("extensions.mashTape.msg.by") +" "));
  var authorPage = doc.createElement("a");
  authorPage.href = authorUrl;
  authorPage.innerHTML = author;
  detailAuthor.appendChild(authorPage);

  // Dispatch a resize event to trigger the video resize
  var e = doc.createEvent("UIEvents");
  e.initUIEvent("resize", true, true, window, 1);
  doc.dispatchEvent(e);
  
  // Set the description
  //detailDescr.innerHTML = description;
}

mashTape.updateFlash = function(provider, results) {
  if (results == null)
    return;

  var indexFrame = document.getElementById("mashTape-panel-flash-index");
  var doc = indexFrame.contentWindow.document;
  var body = doc.getElementsByTagName("body")[0];

  var favicon = provider.QueryInterface(Ci.sbIMashTapeFlashProvider)
    .providerIcon;
  for (var i=0; i<results.length; i++) {
    var entryDiv = doc.createElement("div");
    entryDiv.className = "row-unselected";
    var video = results[i];

    entryDiv.setAttribute("mashTape-title", video.title);
    entryDiv.setAttribute("mashTape-favicon", favicon);
    entryDiv.setAttribute("mashTape-time", video.time);
    entryDiv.setAttribute("mashTape-url", video.url);
    entryDiv.setAttribute("mashTape-video", video.swfUrl);
    entryDiv.setAttribute("mashTape-description", video.description);
    entryDiv.setAttribute("mashTape-author", video.author);
    entryDiv.setAttribute("mashTape-author-url", video.authorUrl);
    entryDiv.setAttribute("mashTape-duration", video.duration);
    entryDiv.setAttribute("mashTape-width", video.width);
    entryDiv.setAttribute("mashTape-ratio", video.ratio);
    entryDiv.setAttribute("mashTape-height", video.height);
    entryDiv.setAttribute("mashTape-provider", provider.providerName);

    if (video.flashVars)
      entryDiv.setAttribute("mashTape-flashvars", video.flashVars);
    entryDiv.addEventListener("click", function(e) {
      var node = e.target;
      while (!node.hasAttribute("mashTape-title"))
        node = node.parentNode;
      mashTape.loadFlashDetail(node);
      e.stopPropagation();
      e.preventDefault();
    }, false);

    var thumbLink = doc.createElement("span");
    var thumbImg = doc.createElement("img");
    thumbImg.className = "thumbnail";
    thumbImg.src = results[i].thumbnail;
    thumbImg.width = "60";
    thumbLink.appendChild(thumbImg);
    entryDiv.appendChild(thumbLink);
    thumbImg.addEventListener("load", function() {
      // in the event we've changed tracks and this img is no longer
      // attached to the DOM then just bail out
      if (this.parentNode == null)
        return;
      var theDiv = this.parentNode.parentNode;
      var theImg = this;
      if (theDiv.clientHeight-10 < theImg.clientHeight)
        theDiv.style.height = (theImg.clientHeight + 5) + "px";
    }, false);

    var faviconImg = doc.createElement("img");
    faviconImg.className = "favicon favicon-flash";
    faviconImg.src = favicon;
    faviconImg.setAttribute("title", provider.providerName);
    entryDiv.appendChild(faviconImg);
    
    var metadata = doc.createElement("div");
    metadata.className = "metadata-flash";
    var link = doc.createElement("span");
    link.innerHTML = results[i].title;
    metadata.appendChild(link);
    metadata.appendChild(doc.createElement("br"));

    if (results[i].time != 0) {
      var dateObj = new Date(results[i].time);
      var dateSpan = doc.createElement("span");
      dateSpan.className = "time";
      dateSpan.innerHTML = dateObj.ago();
      metadata.appendChild(dateSpan);
    }

    entryDiv.appendChild(metadata);

    // Figure out where in the DOM we want to insert this node
    entryDiv.setAttribute("mashTape-timestamp", results[i].time);
    entryDiv.setAttribute("mashTape-title", results[i].title);
    var divs = body.getElementsByTagName("div");
    var inserted = false;
    var nextClass = 0;
    for (var j=0; j<divs.length; j++) {
      if (divs[j].className.indexOf("row-") == -1)
        continue;
      //divs[j].className = mashTape.classes[nextClass];
      nextClass = Math.abs(nextClass-1);

      var otherTimestamp = divs[j].getAttribute("mashTape-timestamp");
      if (otherTimestamp < results[i].time && !inserted) {
        /*
        if (divs[j].className == "row-even") {
          entryDiv.className = "row-even";
          divs[j].className = "row-odd";
        } else {
          entryDiv.className = "row-odd";
          divs[j].className = "row-even";
        }
        */
        body.insertBefore(entryDiv, divs[j]);
        inserted = true;
      }
    }
    if (!inserted) {
      //entryDiv.className = mashTape.classes[nextClass];
      body.appendChild(entryDiv);
    }
  }
}

/****************************************************************************
 * Miscellaneous routines
 ****************************************************************************/
// Maximise the display pane
mashTape.maximiseDisplayPane = function(ev) {
  var flashSplitter = document.getElementById("mashTape-panel-flash-splitter");
  
  var mainDoc = Cc['@mozilla.org/appshell/window-mediator;1']
    .getService(Ci.nsIWindowMediator).getMostRecentWindow('Songbird:Main')
    .window.document;
  var dp = mashTape.displayPane;
  var splitterId = mashTape.displayPane.getAttribute("splitter");
  var dpSplitter = mainDoc.getElementById(splitterId);

  if (mashTape.expanded) {
    // we're already expanded, so restore to the non-expanded state
    mashTape.expanded = false;

    dp.setAttribute("flex", 0);

    // open the splitter back up
    flashSplitter.setAttribute("state", "open");

    // reset the display pane splitter
    dpSplitter.setAttribute("collapse", "after");
    dpSplitter.setAttribute("state", "open");
    
    dp.height = mashTape.height;
    
    mashTape.displayPaneMaxButton.style.listStyleImage =
    "url('chrome://songbird/skin/display-pane/button-maximize.png')";
  } else {
    // expand!
    mashTape.expanded = true;

    mashTape.height = dp.height;

    dp.setAttribute("flex", 1);
    // collapse the splitter
    flashSplitter.setAttribute("state", "collapsed");
    
    // collapse the display pane splitter
    dpSplitter.setAttribute("collapse", "before");
    dpSplitter.setAttribute("state", "collapsed");
    
    mashTape.displayPaneMaxButton.style.listStyleImage =
    "url('chrome://songbird/skin/display-pane/button-restore.png')";
  }
}

// Our listener for auto-hiding mashTape when switching to web views
mashTape.locationListener = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
  onStateChange: function(aProgress, aRequest, aFlag, aStatus) { return(0); },
  onLocationChange:function(a, request, url) {
    if (url == null)
      return;

    if (!Application.prefs.getValue("extensions.mashTape.autohide", false))
      return;

    // don't do anything for firstrun
    if (url.spec == "chrome://songbird/content/mediapages/firstrun.xul")
      return;
    if (typeof(mashTape) == "undefined") {
      return;
    }
    var mainDoc = Cc['@mozilla.org/appshell/window-mediator;1']
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow('Songbird:Main').window.document;
    var splitterId = mashTape.displayPane.getAttribute("splitter");
    var dpSplitter = mainDoc.getElementById(splitterId);

    if ((gBrowser.currentMediaListView &&
          gBrowser.selectedTab == gBrowser.mediaTab) ||
          url.spec.indexOf("chrome://shoutcast-radio/") == 0)
    {
      // expose the mashTape display pane only if we auto-hid it
      if (mashTape.autoHidden) {
        mashTape.displayPane.collapsed = false;
        mashTape.autoHidden = false;
      }
    } else {
      // collapse the mashTape display pane if it's not already
      if (!mashTape.displayPane.collapsed) {
        mashTape.displayPane.collapsed = true;
        mashTape.autoHidden = true;
      }
    }
  },
  onProgressChange: function() {return 0;},
  onStatusChange: function() {return 0;},
  onSecurityChange: function() {return 0;},
  onLinkIconAvailable: function() {return 0;}
}

// Our observer for watching the tab-enabled/disabled preferences and hiding
// the tabs appropriately
mashTape.prefObserver = {
  observe: function(subject, topic, data) {
    if (subject instanceof Components.interfaces.nsIPrefBranch) {
      var pref = data.split(".");
      if (pref.length == 2 && pref[1] == "enabled") {
        var tab;
        var enabled = subject.getBoolPref(data);
        switch (pref[0]) {
          case "info":
            tab = mashTape.infoTab;
            break;
          case "review":
            tab = mashTape.reviewTab;
            break;
          case "rss":
            tab = mashTape.rssTab;
            break;
          case "photo":
            tab = mashTape.photoTab;
            break;
          case "flash":
            tab = mashTape.flashTab;
            break;
        }
        if (enabled) {
          tab.style.visibility = "visible";
          gMetrics.metricsInc("mashtape", pref[0], "tab.enabled");
          mashTape.noDataTab(pref[0]);
        } else {
          tab.style.visibility = "collapse";
          gMetrics.metricsInc("mashtape", pref[0], "tab.disabled");
          // if we're currently on the tab, then hide it and select some
          // other tab
          mashTape.selectPane();
        }
      } else if (data == "autohide") {
        var enabled = subject.getBoolPref(data);
        if (enabled)
          gMetrics.metricsInc("mashtape", "autohide", "enabled");
        else
          gMetrics.metricsInc("mashtape", "autohide", "disabled");
      } else if (data == "defaultpane") {
        var which = subject.getCharPref(data);
        gMetrics.metricsInc("mashtape", "defaultpane", which);
        mashTape.selectPane();
      } else if (data == "photo.speed") {
        mashTape.photoFrame.contentWindow.mashTape_updatePhotoStreamSpeed();
      }
    }
  }
}

window.addEventListener("DOMContentLoaded", mashTape.init, false);
window.addEventListener("unload", mashTape.unload, false);

mashTape.tooltip = function(tipElement)
{
  var retVal = false;
  if (tipElement.namespaceURI ==
      "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul")
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";


  var titleText = null;
  var XLinkTitleText = null;
  var direction = tipElement.ownerDocument.dir;

  while (!titleText && !XLinkTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      var defView = tipElement.ownerDocument.defaultView;
      // XXX Work around bug 350679:
      // "Tooltips can be fired in documents with no view".
      if (!defView)
        return retVal;
      direction = defView.getComputedStyle(tipElement, "")
        .getPropertyValue("direction");
    }
    tipElement = tipElement.parentNode;
  }

  var tipNode = document.getElementById("aHTMLTooltip");
  tipNode.style.direction = direction;
  
  for each (var t in [titleText, XLinkTitleText]) {
    if (t && /\S/.test(t)) {

      // Per HTML 4.01 6.2 (CDATA section), literal CRs and tabs should be
      // replaced with spaces, and LFs should be removed entirely.
      // XXX Bug 322270: We don't preserve the result of entities like &#13;,
      // which should result in a line break in the tooltip, because we can't
      // distinguish that from a literal character in the source by this point.
      t = t.replace(/[\r\t]/g, ' ');
      t = t.replace(/\n/g, '');

      tipNode.setAttribute("label", t);
      retVal = true;
    }
  }

  return retVal;
}


