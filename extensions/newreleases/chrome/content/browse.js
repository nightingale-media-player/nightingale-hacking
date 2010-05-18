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

if (typeof(Cc) == "undefined")
	var Cc = Components.classes;
if (typeof(Ci) == "undefined")
	var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
	var Cu = Components.utils;

Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");

if (typeof(songbirdMainWindow) == "undefined")
	var songbirdMainWindow = Cc["@mozilla.org/appshell/window-mediator;1"]
			.getService(Ci.nsIWindowMediator)
			.getMostRecentWindow("Songbird:Main").window;

if (typeof(gBrowser) == "undefined")
	var gBrowser = Cc["@mozilla.org/appshell/window-mediator;1"]
			.getService(Ci.nsIWindowMediator)
			.getMostRecentWindow("Songbird:Main").window.gBrowser;

if (typeof(gMetrics) == "undefined")
	var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
    		.createInstance(Ci.sbIMetrics);

function flushDisplay() {
	if (typeof(Components) == "undefined")
		return;
	while ((typeof(Components) != "undefined") && 
			Components.classes["@mozilla.org/thread-manager;1"].getService()
			.currentThread.hasPendingEvents())
	{
		Components.classes["@mozilla.org/thread-manager;1"].getService()
				.currentThread.processNextEvent(true);
	}
}

var letterIndices = ['#', 'Z','Y','X','W','V','U','T','S','R','Q','P','O','N',
                     'M','L','K','J','I','H','G','F','E','D','C','B','A'];

// Special database ID for new releases in all countries
const allEntriesID = "ALL";

// If a country name needs an extra word for wording to flow better (ex.
// "United States" vs. "the United States") then add it to the following
// hash.  Currently the logic only adds additional words before the
// country name.
var customCountries = new Array();
customCountries["Netherlands"] = "the";
customCountries["Russian Federation"] = "the";
customCountries["United Kingdom"] = "the";
customCountries["United States"] = "the";

if (typeof NewReleaseAlbum == 'undefined') {
	var NewReleaseAlbum = {};
}

NewReleaseAlbum.unload = function() {
	Components.classes["@songbirdnest.com/newreleases;1"]
		.getService(Ci.nsIClassInfo).wrappedJSObject.unregisterDisplayCallback();
}

NewReleaseAlbum.init = function() {
	// Set the tab title
	var servicePaneStr = Cc["@mozilla.org/intl/stringbundle;1"]
		.getService(Ci.nsIStringBundleService)
		.createBundle("chrome://newreleases/locale/overlay.properties");
	document.title = servicePaneStr.GetStringFromName("servicePane.Name");

	var self = this;

	if (typeof(Ci.sbIMediacoreManager) != "undefined")
		NewReleaseAlbum.newMediaCore = true;
	else
		NewReleaseAlbum.newMediaCore = false;

	this.dateFormat = "%a, %b %e, %Y";
	if (navigator.platform.substr(0, 3) == "Win")
		this.dateFormat = "%a, %b %#d, %Y";

	this.Cc = Components.classes;
	this.Ci = Components.interfaces;

	// Load the iframe
	this.iframe = document.getElementById("newRelease-listings");

	// Set a pointer to our document so we can update it later as needed
	this._browseDoc = this.iframe.contentWindow.document;
	
	// Set our string bundle for localised string lookups
	this._strings = document.getElementById("newReleases-strings");

	// Apply styles to the iframe
	var headNode = this._browseDoc.getElementsByTagName("head")[0];
	var cssNode = this._browseDoc.createElementNS(
			"http://www.w3.org/1999/xhtml", "html:link");
	cssNode.type = 'text/css';
	cssNode.rel = 'stylesheet';
	cssNode.href = 'chrome://songbird/skin/html.css';
	cssNode.media = 'screen';
	headNode.appendChild(cssNode);

	cssNode = this._browseDoc.createElementNS(
			"http://www.w3.org/1999/xhtml", "html:link");
	cssNode.type = 'text/css';
	cssNode.rel = 'stylesheet';
	cssNode.href = 'chrome://newreleases/skin/browse.css';
	cssNode.media = 'screen';
	headNode.appendChild(cssNode);

	// Get the New Releases XPCOM service
	this.nrSvc = Cc["@songbirdnest.com/newreleases;1"]
		.getService(Ci.nsIClassInfo).wrappedJSObject;
	
	// Set the checked state for the filter checkbox
	this.filterLibraryArtists = Application.prefs
			.getValue("extensions.newreleases.filterLibraryArtists", true);
	
	// Get pref to display play link or not
	this.showPlayLink = Application.prefs
			.getValue("extensions.newreleases.showplaylink", false);
	this.showGCal = Application.prefs
			.getValue("extensions.newreleases.showgcal", false);
	this.showExtendedInfo = Application.prefs
			.getValue("extensions.newreleases.showExtendedInfo", false);

	gMetrics.metricsInc("newReleases", "servicepane.clicked", "");
	// Register our UI display callback with the newReleases object
	if (!this.nrSvc.hasDisplayCallback()) {
		var displayCB = new this.displayCallback(this);
		displayCB.wrappedJSObject = displayCB;
		this.nrSvc.registerDisplayCallback(displayCB);
	}

	if (Application.prefs.get("extensions.newreleases.firstrun").value) {
		// Load the first run page in the deck
		var deck = document.getElementById("newReleases-deck");
		deck.setAttribute("selectedIndex", 1);

                // Load location information
		this.nrSvc.refreshLocations();
		NewReleaseOptions.init();
	} else {
		if (this.nrSvc.newReleasesRefreshRunning) {
			// If the data refresh is already happening,
			// switch the deck to the loading page
			var deck = document.getElementById("newReleases-deck");
			deck.setAttribute("selectedIndex", 2);
			var label = document.getElementById("loading-label");
			var progressbar = document.getElementById("loading-progress");
			label.value = this.nrSvc.progressString;
			progressbar.value = this.nrSvc.progressPercentage;
			this.waitForRefreshToFinish(this);
		} else {
			// Otherwise populate the fields in the options page, and continue
			// to the listing view
			NewReleaseOptions.init();
			this.browseNewReleases(this);
		}
	}
}

NewReleaseAlbum.waitForRefreshToFinish = function(self) {
	if (Components.classes["@songbirdnest.com/newreleases;1"]
		.getService(Ci.nsIClassInfo).wrappedJSObject.newReleasesRefreshRunning)
	{
		setTimeout(this.waitForRefreshToFinish(self), 100);
	} else {
		self.browseNewReleases(self);
	}
}

NewReleaseAlbum.editLocation = function() {
	// Save our current selected deck page, so if the user cancels we can
	// switch back to it
	var deck = document.getElementById("newReleases-deck");
	deck.setAttribute("previous-selected-deck", deck.selectedIndex);
	
	// Hide the stuff we don't want to display
	//document.getElementById("pref-library").style.visibility = "hidden";
	//document.getElementById("library-ontour-box").style.visibility ="collapse";

	// Not strictly required, but in the event the user hit cancel, we
	// probably want to reset to their preferred location
	NewReleaseOptions.init();

	// Make the cancel button visible (it's hidden by default for first run)
	var cancel = document.getElementById("pref-cancel-button");
	cancel.style.visibility="visible";

	// Switch to the location edit deck page
	deck.setAttribute("selectedIndex", 1);
}

NewReleaseAlbum.displayCallback = function(ticketingObj) {
	this.label = document.getElementById("loading-label"),
	this.progressbar = document.getElementById("loading-progress");
	this.newReleaseAlbum = ticketingObj;
},
NewReleaseAlbum.displayCallback.prototype = {
	loadingMessage : function(str) {
		this.label.value = str;
	},
	loadingPercentage : function(pct) {
		this.progressbar.value = pct;
	},
	timeoutError : function() {
		songbirdMainWindow.NewReleases.updateNewReleasesCount(0);
		var deck = document.getElementById("newReleases-deck");
		deck.setAttribute("selectedIndex", 4);
	},
	showListings : function() {
		this.newReleaseAlbum.browseNewReleases(this.newReleaseAlbum);
	},
	updateNewReleasesCount : function() {
		songbirdMainWindow.NewReleases.updateNewReleasesCount()
	},
	alert : function(str) { window.alert(str); },
}

// Initiate a synchronous database refresh.  This can only be triggered after
// first run, or if the user goes and changes their location
NewReleaseAlbum.loadNewReleaseData = function(country) {
	// Switch the deck to the loading page
	var deck = document.getElementById("newReleases-deck");
	deck.setAttribute("selectedIndex", 2);

	// First run is over
	if (Application.prefs.getValue("extensions.newreleases.firstrun", false)) {
		// toggle first-run
		Application.prefs.setValue("extensions.newreleases.firstrun", false);

		// setup the smart playlist
		//songbirdMainWindow.NewReleases._setupOnTourPlaylist();
	}

	// Load the new data
	var ret = this.nrSvc.refreshNewReleases(false, country);
	//songbirdMainWindow.NewReleases.updateNewReleasesCount();
	if (!ret)
		this.browseNewReleases(this);
}

NewReleaseAlbum.showNoNewReleases = function() {
	var deck = document.getElementById("newReleases-deck");
	deck.setAttribute("selectedIndex", 3);

	deck = document.getElementById("no-results-deck");
	var count = this.nrSvc.getNewReleasesCount(false);
	var label;
	var button;
	var sampletext;
	if (count == 0) {
		// no releases found, period
		label = document.getElementById("noresults-country-1");
		label.value = this._strings.getString("yourCountrySucks") +
			" ";
		// Ugly hack to tack on any special words before the
		// country name as defined by the hash above
		var countryName = this.nrSvc.getLocationString(this.pCountry);
		if (typeof(customCountries[countryName]) != "undefined")
			label.value += customCountries[countryName] + " ";
		label.value += countryName;
		sampletext = document.getElementById("sampleresults");
		sampletext.style.visibility = "hidden";
		button = document.getElementById("noresults-seeallreleases-country");
		button.style.visibility = "hidden";
	} else {
		// no library artist releases found

		// show error messages - "Well that's lame"
		label = document.getElementById("noresults-country-1");
		label.value = this._strings.getString("noLibLame");

		// "None of the artists in your library have an upcoming
		// release..."
		var countryStr = " ";
		var countryName = this.nrSvc.getLocationString(this.pCountry);
		// Ugly hack to tack on any special words before the
		// country name as defined by the hash above
		if (typeof(customCountries[countryName]) != "undefined")
			countryStr += customCountries[countryName] + " ";
		countryStr += countryName;
		label = document.getElementById("noresults-country-2");
		label.value = this._strings.getString("noLibArtistsReleases");
		label = document.getElementById("noresults-country-3");
		label.value = this._strings.getString("noLibArtistsReleases2") +
			countryStr + ".";

		// "If that changes..."
		label = document.getElementById("noresults-country-4");
		label.value = this._strings.getString("noLibArtistsReleases3");
		label = document.getElementById("noresults-country-5");
		label.value = this._strings.getString("noLibArtistsReleases4");
			
		// Set the actual button text
		button = document.getElementById("noresults-seeallreleases-country");
		button.label = this._strings.getString("seeAllNewReleases") + countryStr;

		sampletext = document.getElementById("sampleresults");
		sampletext.style.visibility = "visible";
	}
}

NewReleaseAlbum.showTimeoutError = function() {
	var deck = document.getElementById("newReleases-deck");
	deck.setAttribute("selectedIndex", 4);
}

// The following isn't currently used for anything but is saved in case
// a provider/URL is ever required
NewReleaseAlbum.openProviderPage = function() {
	var url;
	if (typeof(this.nrSvc) != "undefined")
		url = this.nrSvc.providerURL();
	else
		url = Cc["@songbirdnest.com/newreleases;1"]
			.getService(Ci.nsIClassInfo).wrappedJSObject.providerURL();

	var country = Application.prefs.getValue("extensions.newreleases.country", 0);
	if (country != 0)
		url += "?p=21&user_location=" + country;
	gBrowser.loadOneTab(url);
}

/***************************************************************************
 * Called when the user clicks "Play" next to the main artist name in the
 * artist grouping view of upcoming releases
 ***************************************************************************/
NewReleaseAlbum.playArtist = function(e) {
	var artistName = this.getAttribute("artistName");
	
	gMetrics.metricsInc("newReleases", "browse.view.artist.playartist", "");
	var list = songbirdMainWindow.NewReleases.touringPlaylist;
	var view = list.createView();
	var cfs = view.cascadeFilterSet;
	
	cfs.appendSearch(["*"], 1);
	cfs.appendFilter(SBProperties.genre);
	cfs.appendFilter(SBProperties.artistName);
	cfs.appendFilter(SBProperties.albumName);

	cfs.clearAll();
	cfs.set(2, [artistName], 1);

	if (NewReleaseAlbum.newMediaCore)
		Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
			.getService(Ci.sbIMediacoreManager)
			.sequencer.playView(view, 0);
	else
		Cc['@songbirdnest.com/Songbird/PlaylistPlayback;1']
			.getService(Ci.sbIPlaylistPlayback)
			.sequencer.playView(view, 0);
}

/***************************************************************************
 * Scrolls the iframe to the selected index block when the user clicks on
 * one of the letter or month index links in the chrome
 ***************************************************************************/
NewReleaseAlbum.indexJump = function(e) {
	var iframe = document.getElementById("newRelease-listings");
	this._browseDoc = iframe.contentWindow.document;

	var anchor;
	if (this.id.indexOf("newReleases-nav-letter-") >= 0) {
		var letter = this.id.replace("newReleases-nav-letter-", "");
		anchor = this._browseDoc.getElementById("indexLetter-" + letter);
	} else {
		var dateComponent = this.id.split("-")[3];
		anchor = this._browseDoc.getElementById("indexDate-" + dateComponent);
	}
	if (anchor) {
		var aLeft = anchor.offsetLeft;
		var aTop = anchor.offsetTop;
		iframe.contentWindow.scrollTo(0, aTop);
	}
}


NewReleaseAlbum.browseNewReleases = function(ticketingObj) {
	if (this.nrSvc.drawingLock)
		return;
	this.nrSvc.drawingLock = true;
	this.abortDrawing = false;

	// Switch to the listing view
	var deck = document.getElementById("newReleases-deck");
	deck.setAttribute("selectedIndex", 0);

	this._bodyNode = this._browseDoc.getElementsByTagName("body")[0];

	// Clear any existing results
	while (this._bodyNode.firstChild) {
		this._bodyNode.removeChild(this._bodyNode.firstChild);
	}

	var doc = this._browseDoc;
	var str = this._strings;

	// Add the header block
	var headerDiv = this.createBlock("header");
	var newReleasesImage = doc.createElement("img");
	newReleasesImage.src =
		"chrome://newreleases/skin/NewReleases.png";
	newReleasesImage.id = "newReleases-logo";
	headerDiv.appendChild(newReleasesImage);

/*
	// The following isn't currently used for anything but is saved
	// in case a provider/URL should ever be added
	var songkickImage = doc.createElement("img");
	songkickImage.src =
		"chrome://newreleases/content/songkick_left.png";
	songkickImage.id = "songkick-logo";
	songkickImage.className = "clickable";
	songkickImage.addEventListener("click", this.openProviderPage,
		false);
	headerDiv.appendChild(songkickImage);
*/

	this._bodyNode.appendChild(headerDiv);

	// Add the subheader block
	var subHeaderDiv = this.createBlock("sub-header");
	// This will be populated later when we get the actual new
	// releases count
	var numNewReleasesShownDiv = this.createBlock("num-newReleases",
		true);
	subHeaderDiv.appendChild(numNewReleasesShownDiv); 
  
	var locationChangeDiv = this.createBlock("location", true);
	subHeaderDiv.appendChild(locationChangeDiv);
	var countryNameBlock = this.createBlock("location-city", true);
	this.pCountry = Application.prefs.getValue("extensions.newreleases.country",
		allEntriesID);
	var locationString = this.nrSvc.getLocationString(this.pCountry);
	countryNameBlock.appendChild(doc.createTextNode(locationString));
	locationChangeDiv.appendChild(countryNameBlock);
	var changeLocation = this.createBlock("location-change", true);
	changeLocation.appendChild(doc.createTextNode(str.getString("changeLoc")));
	locationChangeDiv.appendChild(changeLocation);
	changeLocation.addEventListener("click", NewReleaseAlbum.editLocation,
		false);

	var filterDiv = this.createBlock("filter", true);
	var checkbox = doc.createElement("input");
	checkbox.setAttribute("type", "checkbox");
	if (this.filterLibraryArtists)
		checkbox.setAttribute("checked", true);
	checkbox.addEventListener("click", function(e)
		{ NewReleaseAlbum.changeFilter(false); }, false);
	filterDiv.appendChild(checkbox);
	filterDiv.appendChild(doc.createTextNode(str.getString("filter")));
	subHeaderDiv.appendChild(filterDiv);

	var clearDiv = this.createBlock("sub-header-empty");
	clearDiv.style.clear = "both";
	subHeaderDiv.appendChild(clearDiv);
	this._bodyNode.appendChild(subHeaderDiv);

	var groupBy = Application.prefs.getValue("extensions.newreleases.groupby",
		"date");
  	// Add the nav block - the View/New Releases/Artists selector
	var navDiv = this.createBlock("newReleases-nav");
	var viewSpan = this.createBlock("newReleases-nav-view", true);
	viewSpan.appendChild(doc.createTextNode(str.getString("navView")));

	var datesSpan = this.createBlock("newReleases-nav-releases", true);
	var artistsSpan = this.createBlock("newReleases-nav-artists", true);
	if (groupBy == "artist") {
		artistsSpan.className += " newReleases-nav-selected";
		datesSpan.addEventListener("mouseover", function(e)
		{
			datesSpan.className += " newReleases-nav-hover";
		}, false);
		datesSpan.addEventListener("mouseout", function(e)
		{
			datesSpan.className = datesSpan.className.replace(
				" newReleases-nav-hover", "");
		}, false);
		datesSpan.addEventListener("click", function(e)
		{
			NewReleaseAlbum.groupBy("date");
		}, false);
	} else {
		datesSpan.className += " newReleases-nav-selected";
		artistsSpan.addEventListener("mouseover", function(e)
		{
			artistsSpan.className += " newReleases-nav-hover";
		}, false);
		artistsSpan.addEventListener("mouseout", function(e)
		{
			artistsSpan.className = artistsSpan.className.replace(
				" newReleases-nav-hover", "");
		}, false);
		artistsSpan.addEventListener("click", function(e)
		{
			NewReleaseAlbum.groupBy("artist");
		}, false);
	}

	datesSpan.appendChild(doc.createTextNode(str.getString("navDates")));
	artistsSpan.appendChild(doc.createTextNode(str.getString("navArtists")));
	navDiv.appendChild(viewSpan);
	navDiv.appendChild(datesSpan);
	navDiv.appendChild(artistsSpan);

	var indexDiv = this.createBlock("newReleases-nav-index");
	if (groupBy == "artist") {
		// group by Artist Names
		// Add the #,A-Z index letters
		for (var i in letterIndices) {
			var l = letterIndices[i];
			var letterSpan = this.createBlock("newReleases-nav-letter",
				true);
			letterSpan.id = "newReleases-nav-letter-" + l;
			letterSpan.appendChild(doc.createTextNode(l));
			indexDiv.appendChild(letterSpan);
		}
	} else {
		// group by New Release Dates
		var myDate = new Date();
		myDate.setDate(1);
		var forwardMonths =
			Application.prefs.getValue("extensions.newreleases.futuremonths",
			4);
		var dates = [];
		for (i=0; i<forwardMonths; i++) {
			var mon = myDate.getMonth();
			var year = myDate.getFullYear();
			var dateStr = myDate.toLocaleFormat("%b %Y");
			var dateSpan = this.createBlock("newReleases-nav-date", true);
			dateSpan.appendChild(doc.createTextNode(dateStr));
			dateSpan.id = "newReleases-nav-date-" + mon + year;
			myDate.setMonth(mon+1);
			dates.push(dateSpan);
		}
		for (var i in dates.reverse()) {
			indexDiv.appendChild(dates[i], null);
		}
	}
	navDiv.appendChild(indexDiv);
	clearDiv = this.createBlock("newReleases-nav-empty");
	clearDiv.style.clear = "both";
	navDiv.appendChild(clearDiv);
	this._bodyNode.appendChild(navDiv);
	
	flushDisplay();

	var newReleasesShown;
	if (groupBy == "artist")
		newReleasesShown = this.browseByArtists();
	else
		newReleasesShown = this.browseByDates();

	var bundle =
		new SBStringBundle("chrome://newreleases/locale/overlay.properties");
	var numNewReleasesStr = bundle.formatCountString("newReleasesShown",
                                                newReleasesShown,
                                                [ newReleasesShown ],
                                                "foo");

	// Ugliness to hack in articles before country name
	var customStr = "";
	if (typeof(customCountries[locationString]) != "undefined")
		customStr = " " + customCountries[locationString] + " ";
	numNewReleasesShownDiv.appendChild(doc.createTextNode(numNewReleasesStr + customStr));
	songbirdMainWindow.NewReleases.updateNewReleasesCount(newReleasesShown);

	if (newReleasesShown > 0) {
		var ft = this.createBlock("ft");
		ft.id = "ft";

/*
		// The following isn't currently used for anything but is saved
		// in case a provider/URL should ever be added
		var poweredBy = this.createBlock("poweredBy");
		var url = this.nrSvc.providerURL();
		var city = Application.prefs.getValue("extensions.newreleases.city", 0);
		if (city != 0)
			url += "?p=21&user_location=" + city;
		poweredBy.innerHTML = this._strings.getString("poweredBy") + 
			" <a target='_blank' href='" + url + "'>Songkick</a>. "
			+ this._strings.getString("tosPrefix") + 
			" <a target='_blank' href='http://www.songkick.com/terms?p=21'>" +
			this._strings.getString("tosLink") + "</a>.";
		ft.appendChild(poweredBy);
*/

		var lastUpdated = this.createBlock("lastUpdated");
		var ts = parseInt(1000*
			Application.prefs.getValue("extensions.newreleases.lastupdated", 0));
		if (ts > 0) {
			var dateString =
				new Date(ts).toLocaleFormat("%a, %b %e, %Y at %H:%M.");
			lastUpdated.innerHTML = this._strings.getString("lastUpdated") +
				" <span class='date'>" + dateString + "</span>";
			ft.appendChild(lastUpdated);
		}
		this._bodyNode.appendChild(ft);
	}

	// Debug
	/*
	var html = this._bodyNode.innerHTML;
	var textbox = this._browseDoc.createElement("textarea");
	textbox.width=50;
	textbox.height=50;
	this._bodyNode.appendChild(textbox);
	textbox.value = html;
	*/
	
	this.nrSvc.drawingLock = false;
}

NewReleaseAlbum.browseByDates = function() {
	var lastDate = "";
	var dateBlock = null;
	var releaseTable = null;
	var releases = new this.nrSvc.releaseEnumerator("date",
		this.filterLibraryArtists,
		this.pCountry);
	var newReleasesShown = 0;

	var contentsNode = this._browseDoc.createElement("div");
	contentsNode.id = "releases-contents";
	this._bodyNode.appendChild(contentsNode);
	
	var today = new Date();
	var todayMon = today.getMonth();
	var todayDate = today.getDate();
	var todayYear = today.getFullYear();

	gMetrics.metricsInc("newReleases", "browse.view.date", "");
	while (releases.hasMoreElements()) {
		if (this.abortDrawing)
			return;
		var release = releases.getNext().wrappedJSObject;

		var thisDateObj = new Date();
		thisDateObj.setTime(release.ts);
		var thisMon = thisDateObj.getMonth();
		var thisYear = thisDateObj.getFullYear();
		var thisIndex = thisMon + "" + thisYear;
		var thisDate = thisYear + '-' + thisMon + '-' + thisDateObj.getDate();

		// Don't show past releases
		if (((thisMon < todayMon) && (thisYear <= todayYear)) ||
			(thisMon == todayMon && thisDateObj.getDate() < todayDate)) {
			continue;
		}

		var doc = this._browseDoc;
		var dateIndex = doc.getElementById("newReleases-nav-date-" + thisIndex);
		if (dateIndex == null) {
			continue;
		}
		dateIndex.className += " newReleases-nav-date-link";
		dateIndex.addEventListener("mouseover", function(e)
		{
			var el = e.currentTarget;
			el.className += " newReleases-nav-hover";
		}, false);
		dateIndex.addEventListener("mouseout", function(e)
		{
			var el = e.currentTarget;
			el.className = el.className.replace(" newReleases-nav-hover", "");
		}, false);
		dateIndex.addEventListener("click", NewReleaseAlbum.indexJump, false);

		if (thisDate != lastDate) {
			// Create the block for release listings of the same letter index
			var dateDiv = this.createBlock("indexDiv");

			// Create the letter index block
			var dateIndexContainer = this.createDateBlock(thisDateObj);

			// Create the actual release listing block for this date
			dateBlock = this.createBlock("releaseListing");
			dateBlock.className += " releaseListingDate";

			// Create a new release listing block
			var releaseTableBlock = this.createBlock("artistBlock");
			
			// Create the table for new releases
			releaseTable = this.createTableDateView();
			releaseTableBlock.appendChild(releaseTable);

			// Assemble the pieces together
			dateBlock.appendChild(releaseTableBlock);
			dateDiv.appendChild(dateIndexContainer);
			dateDiv.appendChild(dateBlock);
			contentsNode.appendChild(dateDiv);
			flushDisplay();
		}
		
		// Create the table row representing this release
		var thisRelease = this.createRowDateView(release);
		releaseTable.appendChild(thisRelease);

		lastDate = thisDate;
		newReleasesShown++;
	}
	
	if (newReleasesShown == 0) {
		if (Application.prefs.getValue("extensions.newreleases.networkfailure",
					false))
			NewReleaseAlbum.showTimeoutError();
		else
			NewReleaseAlbum.showNoNewReleases();
	}

	return newReleasesShown;
}

NewReleaseAlbum.browseByArtists = function() {
	var lastLetter = "";
	var lastArtist = "";
	var releaseBlock = null;
	var artistReleaseBlock = null;
	var thisMainArtistAnchor = null;

	// Placeholders for "#" block
	var othersLetterDiv = null;

	var releases = this.nrSvc.artistReleaseEnumerator(
			this.filterLibraryArtists, this.pCountry);

	var newReleasesShown = 0;
	
	var contentsNode = this._browseDoc.createElement("div");
	contentsNode.id = "releases-contents";
	this._bodyNode.appendChild(contentsNode);
	
	var today = new Date();
	var todayMon = today.getMonth();
	var todayDate = today.getDate();
	var todayYear = today.getFullYear();

	gMetrics.metricsInc("newReleases", "browse.view.artist", "");
	while (releases.hasMoreElements()) {
		if (this.abortDrawing)
			return;

		var release = releases.getNext().wrappedJSObject;

		var thisDateObj = new Date();
		thisDateObj.setTime(release.ts);
		var thisMon = thisDateObj.getMonth();
		var thisYear = thisDateObj.getFullYear();
		var thisIndex = thisMon + "" + thisYear;
		var thisDate = thisYear + '-' + thisMon + '-' + thisDateObj.getDate();

		// Don't show past releases
		if (((thisMon < todayMon) && (thisYear <= todayYear)) ||
			(thisMon == todayMon && thisDateObj.getDate() < todayDate))
		{
			continue;
		}
		var thisLetter = release.artistname[0].toUpperCase();
		if (thisLetter < 'A' || thisLetter > 'Z')
			thisLetter = '#';

		var doc = this._browseDoc;
		var letterIdx = doc.getElementById("newReleases-nav-letter-" + thisLetter);
		if (letterIdx == null) {
			continue;
		}
		letterIdx.className += " newReleases-nav-letter-link";
		letterIdx.addEventListener("mouseover", function(e)
		{
			var el = e.currentTarget;
			el.className += " newReleases-nav-hover";
		}, false);
		letterIdx.addEventListener("mouseout", function(e)
		{
			var el = e.currentTarget;
			el.className = el.className.replace(" newReleases-nav-hover", "");
		}, false);
		letterIdx.addEventListener("click", NewReleaseAlbum.indexJump, false);

		if (thisLetter != lastLetter) {
			// Create the block for release listings of the same letter index
			var letterDiv = this.createBlock("indexDiv");

			// Create the letter index block
			var letterIndexContainer = this.createLetterBlock(thisLetter);

			// Create the actual release listing block for this letter
			releaseBlock = this.createBlock("releaseListing");
			releaseBlock.className += " releaseListingLetter";

			// Assemble the pieces together
			letterDiv.appendChild(letterIndexContainer);
			letterDiv.appendChild(releaseBlock);
			//this._bodyNode.appendChild(letterDiv);
			if (thisLetter != '#')
				contentsNode.appendChild(letterDiv);
			else
			{
				// Wait until all entries have been processed
				// before tacking on the last "#" block since
				// these tend to lead to artist links that
				// don't make sense
				othersLetterDiv = letterDiv;
			}

			flushDisplay();
		}

		if (release.artistname != lastArtist) {
			// Create a new artist block
			var artistBlock = this.createBlock("artistBlock");

			// Create the main release title
			var artistName = this.createBlock("mainArtistName", true);
			thisMainArtistAnchor = this._browseDoc.createElement("a");
			var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=1&sql=" + escape(release.artistname);
			this.makeLink(thisMainArtistAnchor, url, "artist.main");
			var artistNameText =
				this._browseDoc.createTextNode(release.artistname);
			thisMainArtistAnchor.appendChild(artistNameText);
			artistName.appendChild(thisMainArtistAnchor);
			artistBlock.appendChild(artistName);

			if (release.libartist == 1 && this.showPlayLink) {
				// Create the play link
				var playBlock = this.createBlock("playBlock", true);
				var playLink = this._browseDoc.createElement("img");
				playLink.className = "ticketImage";
				playLink.src = "chrome://newreleases/skin/icon-play.png";
				playBlock.className = "playLink";
				playBlock.appendChild(playLink);
				playBlock.setAttribute("artistName", release.artistname);
				playBlock.addEventListener("click", this.playArtist, false);
				artistBlock.appendChild(playBlock);
			}

			// Create the table for releases
			artistReleaseBlock = this.createTableArtistView();
			artistBlock.appendChild(artistReleaseBlock);

			// Attach the artist block to the release listing block
			releaseBlock.appendChild(artistBlock);
		}
		
		// Create the table row representing this release
		var thisRelease = this.createRowArtistView(release,
				thisMainArtistAnchor);
		artistReleaseBlock.appendChild(thisRelease);

		lastLetter = thisLetter;
		lastArtist = release.artistname;
		
		newReleasesShown++;
	}

	if (othersLetterDiv != null)
		contentsNode.appendChild(othersLetterDiv);
	
	if (newReleasesShown == 0) {
		if (Application.prefs.getValue("extensions.newreleases.networkfailure",
					false))
			NewReleaseAlbum.showTimeoutError();
		else
			NewReleaseAlbum.showNoNewReleases();
	}

	return newReleasesShown;
}

// Takes a letter, and returns an index block for it
NewReleaseAlbum.createLetterBlock = function(letter) {
	// letter index (LHS)
	var letterIndexContainer = this.createBlock("letterIndexContainer");
	var letterIndex = this.createBlock("letterIndex");
	letterIndex.id = "indexLetter-" + letter;
	var letterIndexText = this._browseDoc.createTextNode(letter);
	letterIndex.appendChild(letterIndexText);
	letterIndexContainer.appendChild(letterIndex);
	return (letterIndexContainer);
}

// Takes a date, and returns an index block for it
NewReleaseAlbum.createDateBlock = function(dateObj) {
	var dateIndexContainer = this.createBlock("dateIndexContainer");
	var boxBlock = this.createBlock("dateIndex-box");
	var month = this.createBlock("dateIndex-month");
	var date = this.createBlock("dateIndex-date");
	var day = this.createBlock("dateIndex-day");
	dateIndexContainer.id = "indexDate-" + dateObj.getMonth() +
		dateObj.getFullYear();
	var monthText =
		this._browseDoc.createTextNode(dateObj.toLocaleFormat("%b"));
	var dateText = this._browseDoc.createTextNode(dateObj.toLocaleFormat("%d"));
	var dayText = this._browseDoc.createTextNode(dateObj.toLocaleFormat("%a"));
	month.appendChild(monthText);
	day.appendChild(dayText);
	date.appendChild(dateText);
	boxBlock.appendChild(month);
	boxBlock.appendChild(date);
	dateIndexContainer.appendChild(boxBlock);
	dateIndexContainer.appendChild(day);
	return (dateIndexContainer);
}

/***************************************************************************
 * Routines for building the actual table objects for listing individual
 * releases within
 ***************************************************************************/
NewReleaseAlbum.createTableArtistView = function() {
	var table = this.createTable();
	var headerRow = this._browseDoc.createElement("tr");
	var dateCol = this.createTableHeader("tableHeaderDate", "date");
	var titleCol = this.createTableHeader("tableHeaderTitle", "title");
	var labelCol = this.createTableHeader("tableHeaderLabel", "label");
	var countryCol = this.createTableHeader("tableHeaderCountry", "country");
	var typeCol = this.createTableHeader("tableHeaderType", "type");
	var tracksCol = this.createTableHeader("tableHeaderTracks", "tracks");
	var gCalCol = this.createTableHeader("tableHeaderGCal", "gcal");
	headerRow.appendChild(dateCol);
	headerRow.appendChild(titleCol);
	headerRow.appendChild(labelCol);
	if (this.pCountry == allEntriesID)
		headerRow.appendChild(countryCol);
	if (this.showExtendedInfo)
	{
		headerRow.appendChild(typeCol);
		headerRow.appendChild(tracksCol);
	}
	if (this.showGCal)
		headerRow.appendChild(gCalCol);
	table.appendChild(headerRow);
	return (table);
}

NewReleaseAlbum.createTableDateView = function() {
	var table = this.createTable();
	var headerRow = this._browseDoc.createElement("tr");
	var artistCol = this.createTableHeader("tableHeaderArtist", "artist");
	var titleCol = this.createTableHeader("tableHeaderTitle", "title");
	var labelCol = this.createTableHeader("tableHeaderLabel", "label");
	var countryCol = this.createTableHeader("tableHeaderCountry", "country");
	var typeCol = this.createTableHeader("tableHeaderType", "type");
	var tracksCol = this.createTableHeader("tableHeaderTracks", "tracks");
	var gCalCol = this.createTableHeader("tableHeaderGCal", "gcal");
	headerRow.appendChild(artistCol);
	headerRow.appendChild(titleCol);
	headerRow.appendChild(labelCol);
	if (this.pCountry == allEntriesID)
		headerRow.appendChild(countryCol);
	if (this.showExtendedInfo)
	{
		headerRow.appendChild(typeCol);
		headerRow.appendChild(tracksCol);
	}
	if (this.showGCal)
		headerRow.appendChild(gCalCol);
	table.appendChild(headerRow);
	return (table);
}

NewReleaseAlbum.createTable = function() {
	var table = this._browseDoc.createElement("table");
	table.setAttribute("cellpadding", "0");
	table.setAttribute("cellspacing", "0");
	table.setAttribute("border", "0");
	return (table);
}

// str is a name in the .properties localised strings
// className is the class to apply to the TH cell
NewReleaseAlbum.createTableHeader = function(str, className) {
	var col = this._browseDoc.createElement("th");
	col.className = className;
	if (str == "tableHeaderTickets" || this._strings.getString(str) == "")
		col.innerHTML = "&nbsp;";
	else {
		var colLabel =
			this._browseDoc.createTextNode(this._strings.getString(str));
		col.appendChild(colLabel);
	}
	return (col);
}
/***************************************************************************
 * Routines for building the individual release listing rows, i.e. the table
 * row representing each individual release for each of the two views
 ***************************************************************************/
NewReleaseAlbum.createRowArtistView = function(release, mainAnchor) {
	var row = this._browseDoc.createElement("tr");
	var dateCol = this.createColumnDate(release);
	var titleCol = this.createColumnTitle(release);
	var labelCol = this.createColumnLabel(release);
	var countryCol = this.createColumnCountry(release);
	var typeCol = this.createColumnType(release);
	var tracksCol = this.createColumnTracks(release);
	var gcalCol = this.createColumnGCal(release);

	row.appendChild(dateCol);
	row.appendChild(titleCol);
	row.appendChild(labelCol);
	if (this.pCountry == allEntriesID)
		row.appendChild(countryCol);

	if (this.showExtendedInfo)
	{
		row.appendChild(typeCol);
		row.appendChild(tracksCol);
	}
	if (this.showGCal)
		row.appendChild(gcalCol);
	return (row);
}

NewReleaseAlbum.createRowDateView = function(release) {
	var row = this._browseDoc.createElement("tr");
	var artistCol = this.createColumnArtist(release);
	var titleCol = this.createColumnTitle(release);
	var labelCol = this.createColumnLabel(release);
	var countryCol = this.createColumnCountry(release);
	var typeCol = this.createColumnType(release);
	var tracksCol = this.createColumnTracks(release);
	var gcalCol = this.createColumnGCal(release);

	row.appendChild(artistCol);
	row.appendChild(titleCol);
	row.appendChild(labelCol);
	if (this.pCountry == allEntriesID)
		row.appendChild(countryCol);
	if (this.showExtendedInfo)
	{
		row.appendChild(typeCol);
		row.appendChild(tracksCol);
	}
	if (this.showGCal)
		row.appendChild(gcalCol);
	return (row);
}

/***************************************************************************
 * Routine for opening links - we need it as a separate routine so we can
 * do metrics reporting, in addition to just opening the link
 ***************************************************************************/
NewReleaseAlbum.openAndReport = function(e) {
	var metric = this.getAttribute("metric-key");
	gMetrics.metricsInc("newReleases", "browse.link." + metric, "");
	gBrowser.loadOneTab(this.href);
	e.preventDefault();
	return false;
}
NewReleaseAlbum.makeLink = function(el, url, metric) {
	el.href = url;
	el.setAttribute("metric-key", metric);
	el.addEventListener("click", this.openAndReport, true);
}

// Add's Songbird's partner code only
NewReleaseAlbum.appendPartnerParam = function(url) {
	return (url + "?p=21");
}

/***************************************************************************
 * Routines for building the individual column components of each release
 * listing table row, e.g. the "Date" column, or the "Artists" column, etc.
 ***************************************************************************/
NewReleaseAlbum.createColumnDate = function(release) {
	var dateCol = this._browseDoc.createElement("td");
	dateCol.className = "date";
	var dateObj = new Date();
	dateObj.setTime(release.ts);
	var dateStr = dateObj.toLocaleFormat(this.dateFormat);
	var dateColLabel = this._browseDoc.createTextNode(dateStr);
	dateCol.appendChild(dateColLabel);
	return (dateCol);
}

NewReleaseAlbum.createColumnTitle = function(release) {
	var titleCol = this._browseDoc.createElement("td");
	titleCol.className = "title";
	var anchor = this._browseDoc.createElement("a");
	var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=2&sql=" +
		escape(release.title);
	this.makeLink(anchor, url, "title");
	var titleColLabel = this._browseDoc.createTextNode(release.title);
	anchor.appendChild(titleColLabel);
	titleCol.appendChild(anchor);
	return (titleCol);
}

NewReleaseAlbum.createColumnLabel = function(release) {
	var labelCol = this._browseDoc.createElement("td");
	labelCol.className = "label";
	var anchor = this._browseDoc.createElement("a");
	var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=5&sql=" +
		release.label.replace(/ /g, "+");
	this.makeLink(anchor, url, "label");
	var labelColLabel = this._browseDoc.createTextNode(release.label);
	anchor.appendChild(labelColLabel);
	labelCol.appendChild(anchor);
	return (labelCol);
}

NewReleaseAlbum.createColumnCountry = function(release) {
	var countryCol = this._browseDoc.createElement("td");
	countryCol.className = "country";
	var countryColLabel = this._browseDoc.createTextNode(release.country);
	countryCol.appendChild(countryColLabel);
	return (countryCol);
}

NewReleaseAlbum.createColumnType = function(release) {
	var typeCol = this._browseDoc.createElement("td");
	typeCol.className = "type";
	var typeColLabel = this._browseDoc.createTextNode(release.type);
	typeCol.appendChild(typeColLabel);
	return (typeCol);
}

NewReleaseAlbum.createColumnTracks = function(release) {
	var tracksCol = this._browseDoc.createElement("td");
	tracksCol.className = "tracks";
	var tracksColLabel = this._browseDoc.createTextNode(release.tracks);
	tracksCol.appendChild(tracksColLabel);
	return (tracksCol);
}

NewReleaseAlbum.createColumnGCal = function(release) {
	var dateObj = new Date(release.ts*1000);
	var dateStr = dateObj.toLocaleFormat("%Y%m%d"); // T%H%M%SZ");
	var url = "http://www.google.com/calendar/event?action=TEMPLATE";
	url += "&text=" + escape(release.title);
	url += "&details=";
	url += escape(release.artistname) + ",";
	// trim the last ,
	url = url.substr(0, url.length-1);
	url += "&dates=" + dateStr + "/" + dateStr;

	var gCalLinkCol = this._browseDoc.createElement("td");
	gCalLinkCol.className = "gcal";
	var gCalLink = this._browseDoc.createElement("a");
	this.makeLink(gCalLink, url, "gcal");
	gCalLink.setAttribute("title", this._strings.getString("tableGCalTooltip"));
	
	var gCalImage = this._browseDoc.createElement("img");
	gCalImage.src = "chrome://newreleases/skin/gcal.png";
	gCalImage.className = "gcalImage";
	gCalLink.appendChild(gCalImage);

	gCalLinkCol.appendChild(gCalLink);

	return (gCalLinkCol);
}

NewReleaseAlbum.createColumnArtist = function(release) {
	var artistCol = this._browseDoc.createElement("td");
	artistCol.className = "artist";
	var anchor = this._browseDoc.createElement("a");
	var url = "http://www.allmusic.com/cg/amg.dll?P=amg&opt1=1&sql=" +
		escape(release.artistname);
	this.makeLink(anchor, url, "artistname");
	var artistColLabel =
		this._browseDoc.createTextNode(release.artistname);
	anchor.appendChild(artistColLabel);
	artistCol.appendChild(anchor);
	return (artistCol);
}

/* Generic shortcut for creating a DIV or SPAN & assigning it a style class */
NewReleaseAlbum.createBlock = function(blockname, makeSpan) {
	var block;
	if (makeSpan)
		var block = this._browseDoc.createElement("span");
	else
		var block = this._browseDoc.createElement("div");
	block.className=blockname;
	return block;
}

/* Methods connected to the groupby menulist & filter checkbox on the chrome */
NewReleaseAlbum.changeFilter = function(updateCheckbox) {
	this.filterLibraryArtists = !this.filterLibraryArtists;
	Application.prefs.setValue("extensions.newreleases.filterLibraryArtists",
			this.filterLibraryArtists);
	if (this.filterLibraryArtists)
		gMetrics.metricsInc("newReleases", "filter.library", "");
	else
		gMetrics.metricsInc("newReleases", "filter.all", "");

	/* checks the filter checkbox (for the path taken when there are no
	   results in the user's country and they click the button to see
	   all release, rather than checking the checkbox themselves */
	if (updateCheckbox) {
		var checkbox = document.getElementById("checkbox-library-artists");
		checkbox.setAttribute("checked", this.filterLibraryArtists);
	}
	//songbirdMainWindow.NewReleases.updateNewReleasesCount();
	flushDisplay();
	if (this.nrSvc.drawingLock) {
		// trigger browseByArtists|browseByDates to abort
		this.abortDrawing = true;

		// block for release of the lock, and then redraw
		this.blockAndBrowseReleases(this);
	} else {
		this.browseNewReleases(this);
	}
}

NewReleaseAlbum.groupBy = function(group) {
	Application.prefs.setValue("extensions.newreleases.groupby", group);
	
	if (this.nrSvc.drawingLock) {
		// trigger browseByArtists|browseByDates to abort
		this.abortDrawing = true;

		// block for release of the lock, and then redraw
		this.blockAndBrowseReleases(this);
	} else {
		this.browseNewReleases(this);
	}
}

/* Spins until drawingLock is released, and then triggers a browseNewReleases() */
NewReleaseAlbum.blockAndBrowseReleases = function blockAndBrowseReleases(ct) {
	if (Cc["@songbirdnest.com/newreleases;1"]
		.getService(Ci.nsIClassInfo).wrappedJSObject.drawingLock)
	{
		setTimeout(function() { blockAndBrowseReleases(ct);}, 100);
	} else {
		ct.browseNewReleases(ct);
	}
}
