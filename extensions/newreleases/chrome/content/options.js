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

if (typeof Cc == 'undefined')
	var Cc = Components.classes;
if (typeof Ci == 'undefined')
	var Ci = Components.interfaces;

if (typeof(gMetrics) == "undefined")
	var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
    		.createInstance(Ci.sbIMetrics);

/* FIX:
** - onTour property stuff
*/
var NewReleaseOptions = {
	pCountry : null,

	init : function() {
		this.pCountry =
			Application.prefs.getValue("extensions.newreleases.country", 1);
		var countryDropdown = document.getElementById("menulist-country");

		// Get our New Releases & JSON XPCOM components
		this.nrSvc = Cc["@songbirdnest.com/newreleases;1"]
			.getService(Ci.nsIClassInfo).wrappedJSObject;
		this.json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

		if (!this.nrSvc.gotLocationInfo()) {
			this.nrSvc.refreshLocations();
			NewReleaseAlbum.showTimeoutError();
			return;
		}

		// Loop until the location refresh is done running
		/*
		while (this.nrSvc.locationRefreshRunning) {
			Cc["@mozilla.org/thread-manager;1"].getService()
					.currentThread.processNextEvent(false);
		}
		*/

		// Populate the countries dropdown and pre-select our saved pref
		this._populateCountries(this.pCountry);
	},

	_populateCountries : function(selectedCountry) {
		var countries = this.json.decode(this.nrSvc.getLocationCountries());
		var countryDropdown = document.getElementById("menulist-country");
		countryDropdown.removeAllItems();
		var idx = 0;
		for (var i=0; i<countries.length; i++) {
			var country = countries[i].name;
			countryDropdown.appendItem(country, countries[i].id);
			if (countries[i].id == selectedCountry)
				idx = i;
		}
		countryDropdown.selectedIndex = idx;
	},
	
	changeCountry : function(list) {
		var countryId = list.selectedItem.value;
	},

	cancel : function() {
		var deck = document.getElementById("newReleases-deck");
		var prev = 0;
		if (deck.hasAttribute("previous-selected-deck"))
			 prev = deck.getAttribute("previous-selected-deck");
		deck.setAttribute("selectedIndex", prev);
	},
	
	save : function() {
		var countryDropdown = document.getElementById("menulist-country");
		var country = countryDropdown.selectedItem.value;

		// unless this is firstrun, then let's save it permanently
		//if (Application.prefs.getValue("extensions.newreleases.firstrun", false)) {
		Application.prefs.setValue("extensions.newreleases.country", country);

		gMetrics.metricsInc("newReleases", "change.location", "");
/*
		var box = document.getElementById("library-ontour-box");
		if (box.style.visibility != "hidden") {
			var check = document.getElementById("checkbox-library-integration");
			if (check.checked) {
				gMetrics.metricsInc("newReleases", "library.ontour.checked", "");

				var hardcodedSpec = SBProperties.trackName + " 261  " +
						SBProperties.duration + " 43 " +
						SBProperties.artistName + " 173 " +
						SBProperties.albumName + " 156 " +
						SBProperties.genre + " 53 " +
						SBProperties.rating + " 80 " +
						this.nrSvc.onTourImgProperty + " 10";
				
				// Get the library colspec.  If it's not null, then append
				// the On Tour image property to it
				var colSpec = LibraryUtils.mainLibrary.getProperty(
							SBProperties.columnSpec);
				if (colSpec != null) {
					// Make sure we don't already have the column visible
					if (colSpec.indexOf(this.nrSvc.onTourImgProperty) == -1) {
						colSpec += " " + this.nrSvc.onTourImgProperty + " 10";
						LibraryUtils.mainLibrary.setProperty(
								SBProperties.columnSpec, colSpec);
					}
				} else {
					// If it was null, then look for the default colspec
					colSpec = LibraryUtils.mainLibrary.getProperty(
							SBProperties.defaultColumnSpec);
					// If default colspec isn't null, then append the On Tour
					// image property to that
					if (colSpec != null) {
						colSpec += " " + this.nrSvc.onTourImgProperty + " 10";
						LibraryUtils.mainLibrary.setProperty(
								SBProperties.defaultColumnSpec, colSpec);
					} else {
						// Failing that, just set the default colspec to be
						// our hardcoded one above (adapted from
						// sbColumnSpecParser.jsm)
						LibraryUtils.mainLibrary.setProperty(
								SBProperties.defaultColumnSpec, hardcodedSpec);
					}
				}
			}
		}
*/
		NewReleaseAlbum.loadNewReleaseData();
	},
};
