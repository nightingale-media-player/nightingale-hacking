if (typeof Cc == 'undefined')
  var Cc = Components.classes;
if (typeof Ci == 'undefined')
  var Ci = Components.interfaces;

if (typeof(gMetrics) == "undefined")
  var gMetrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
        .createInstance(Ci.sbIMetrics);

Components.utils.import("resource://app/jsmodules/sbColumnSpecParser.jsm");

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
      var country = countries[i].name;
      if (country == "US")
        country = "United States";
      else if (country == "UK")
        country = "United Kingdom";
      countryDropdown.appendItem(country, countries[i].id);
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
    // catch the UK/New Zealand case where there are no "states"
    if (this._states.length == 1 &&
        (this._states[0].name == "" ||
         this._states[0].name.indexOf("&lt;countrywide&gt;")))
    {
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

        // Get the library colspec.  If it's not null, then append
        // the On Tour image property to it
        var parser = new ColumnSpecParser(LibraryUtils.mainLibrary, null, null,
                                          "audio");
        var foundProperty = false;
        // Scan to see if the concerts property is there already.
        for each (var column in parser.columnMap) {
          if (column.property == this.skSvc.onTourImgProperty) {
            foundProperty = true;
            break;
          }
        }

        // Only add the property if it's not already there
        if (!foundProperty) {
          // Make room for it
          ColumnSpecParser.reduceWidthsProportionally(parser.columnMap, 58);
          // Turn the colspec into a proper string
          var colspec = "";
          for each (var column in parser.columnMap) {
            colspec += column.property + " " +
                       column.width + " ";
            if (column.property == parser.sortID) {
              if (parser.sortIsAscending)
                colspec += "a ";
              else
                colspec += "d ";
            }
          }
          colspec += this.skSvc.onTourImgProperty + " 58";
          var property = SBProperties.columnSpec + "+(audio)";
          LibraryUtils.mainLibrary.setProperty(property, colspec);
        }
      }
    }
    ConcertTicketing.loadConcertData(city);
  },
};
