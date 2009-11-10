if (typeof Cc == 'undefined')
	var Cc = Components.classes;
if (typeof Ci == 'undefined')
	var Ci = Components.interfaces;

if (typeof(gMetrics) == "undefined")
	var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
    		.createInstance(Ci.sbIMetrics);

var ConcertOptions = {
	pCountry : null,
	pState : null,
	pCity : null,

	init : function() {
		this.pCountry =
			Application.prefs.getValue("extensions.concerts.country", 1);
		this.pState =
			Application.prefs.getValue("extensions.concerts.state", 0);
		this.pCity =
			Application.prefs.getValue("extensions.concerts.city", 0);

		var countryDropdown = document.getElementById("menulist-country");
		var statesDropdown = document.getElementById("menulist-state");
		var citiesDropdown = document.getElementById("menulist-city");

		// Get our Songkick & JSON XPCOM components
		this.skSvc = Cc["@songbirdnest.com/Songbird/Concerts/Songkick;1"]
			.getService(Ci.sbISongkick);
		this.json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

		if (!this.skSvc.gotLocationInfo()) {
			this.skSvc.refreshLocations();
			ConcertTicketing.showTimeoutError();
			return;
		}
		// Loop until the location refresh is done running
		/*
		while (this.skSvc.locationRefreshRunning) {
			Cc["@mozilla.org/thread-manager;1"].getService()
					.currentThread.processNextEvent(false);
		}
		*/

		// Populate the countries dropdown and pre-select our saved pref
		this._populateCountries(this.pCountry);

		// Populate the states dropdown and pre-select our saved pref
		this._populateStates(this.pCountry, this.pState);

		// Populate the cities dropdown and pre-select our saved pref
		this._populateCities(this.pState, this.pCity);
	},

	_populateCountries : function(selectedCountry) {
		var countries = this.json.decode(this.skSvc.getLocationCountries());
		var countryDropdown = document.getElementById("menulist-country");
		countryDropdown.removeAllItems();
		var idx = 0;
		for (var i=0; i<countries.length; i++) {
			countryDropdown.appendItem(countries[i].name, countries[i].id);
			if (countries[i].id == selectedCountry)
				idx = i;
		}
		countryDropdown.selectedIndex = idx;
	},
	
	_populateStates : function(inCountry, selectedState) {
		this._states =
			this.json.decode(this.skSvc.getLocationStates(inCountry));
		this._states.sort( function(a, b) {
				return (a.name > b.name); })
		var stateDropdown = document.getElementById("menulist-state");
		stateDropdown.removeAllItems();
		if (this._states.length == 1 && this._states[0].name == "") {
			// The "state" is the whole country
			stateDropdown.disabled = true;
			this._populateCities(this._states[0].id);
			this.pState = this._states[0].id;
		} else {
			// Multiple states
			stateDropdown.disabled = false;
			var idx = 0;
			for (let i=0; i<this._states.length; i++) {
				stateDropdown.appendItem(this._states[i].name,
						this._states[i].id);
				if (this._states[i].id == selectedState)
					idx = i;
			}
			stateDropdown.selectedIndex = idx;
			this._populateCities(stateDropdown.selectedItem.value);
		}
	},
	
	_populateCities : function(inState, selectedCity) {
		var cities = this.json.decode(this.skSvc.getLocationCities(inState));
		cities.sort( function(a, b) {
				return (a.name > b.name); })
		var citiesDropdown = document.getElementById("menulist-city");
		citiesDropdown.removeAllItems();
		var idx = 0;
		for (let i=0; i<cities.length; i++) {
			citiesDropdown.appendItem(cities[i].name, cities[i].id);
			if (cities[i].id == selectedCity)
				idx = i;
		}
		citiesDropdown.selectedIndex = idx;
	},

	changeCountry : function(list) {
		var countryId = list.selectedItem.value;
		this._populateStates(countryId);
	},

	changeState : function(list) {
		var stateId = list.selectedItem.value;
		this._populateCities(stateId);
	},

	changeCity : function(list) {
	},

	cancel : function() {
		var deck = document.getElementById("concerts-deck");
		var prev = 0;
		if (deck.hasAttribute("previous-selected-deck"))
			 prev = deck.getAttribute("previous-selected-deck");
		deck.setAttribute("selectedIndex", prev);
	},
	
	save : function() {
		var countryDropdown = document.getElementById("menulist-country");
		var statesDropdown = document.getElementById("menulist-state");
		var citiesDropdown = document.getElementById("menulist-city");

		// only save city to a temporary, and make the preference
		// permanent only in _processConcerts
		var country = parseInt(countryDropdown.selectedItem.value);
		var city = parseInt(citiesDropdown.selectedItem.value);

		// unless this is firstrun, then let's save it permanently
		if (Application.prefs.getValue("extensions.concerts.firstrun", false)) {
			Application.prefs.setValue("extensions.concerts.country", country);
			Application.prefs.setValue("extensions.concerts.city", city);
			if (!statesDropdown.disabled) {
				var state = parseInt(statesDropdown.selectedItem.value);
				Application.prefs.setValue("extensions.concerts.state", state);
			}
		}

		gMetrics.metricsInc("concerts", "change.location", "");
		var box = document.getElementById("library-ontour-box");
		if (box.style.visibility != "hidden") {
			var check = document.getElementById("checkbox-library-integration");
			if (check.checked) {
				gMetrics.metricsInc("concerts", "library.ontour.checked", "");

				var hardcodedSpec = SBProperties.trackName + " 261  " +
						SBProperties.duration + " 43 " +
						SBProperties.artistName + " 173 " +
						SBProperties.albumName + " 156 " +
						SBProperties.genre + " 53 " +
						SBProperties.rating + " 80 " +
						this.skSvc.onTourImgProperty + " 10";
				
				// Get the library colspec.  If it's not null, then append
				// the On Tour image property to it
				var colSpec = LibraryUtils.mainLibrary.getProperty(
							SBProperties.columnSpec);
				if (colSpec != null) {
					// Make sure we don't already have the column visible
					if (colSpec.indexOf(this.skSvc.onTourImgProperty) == -1) {
						colSpec += " " + this.skSvc.onTourImgProperty + " 10";
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
						colSpec += " " + this.skSvc.onTourImgProperty + " 10";
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
		ConcertTicketing.loadConcertData(city);
	},
};
