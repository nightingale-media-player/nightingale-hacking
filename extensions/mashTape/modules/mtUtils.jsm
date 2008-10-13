var EXPORTED_SYMBOLS = [ "Date", "mtUtils" ];

const Cc = Components.classes;
const Ci = Components.interfaces;

Date.prototype.setISO8601 = function (string) {
    var regexp = "([0-9]{4})(-([0-9]{2})(-([0-9]{2})" +
        "(T([0-9]{2}):([0-9]{2})(:([0-9]{2})(\.([0-9]+))?)?" +
        "(Z|(([-+])([0-9]{2}):([0-9]{2})))?)?)?)?";
    var d = string.match(new RegExp(regexp));

    var offset = 0;
    var date = new Date(d[1], 0, 1);

    if (d[3]) { date.setMonth(d[3] - 1); }
    if (d[5]) { date.setDate(d[5]); }
    if (d[7]) { date.setHours(d[7]); }
    if (d[8]) { date.setMinutes(d[8]); }
    if (d[10]) { date.setSeconds(d[10]); }
    if (d[12]) { date.setMilliseconds(Number("0." + d[12]) * 1000); }
    if (d[14]) {
        offset = (Number(d[16]) * 60) + Number(d[17]);
        offset *= ((d[15] == '-') ? 1 : -1);
    }

    offset -= date.getTimezoneOffset();
    time = (Number(date) + (offset * 60 * 1000));
    this.setTime(Number(time));
}

Date.prototype.ago = function() {
	var timestamp = this.getTime();
	var agoVal = '';
	var val;

	// Store the current time
	var current_time = new Date();

	if (current_time-0 < timestamp-0) {
		agoVal = "future";
	}
	else {
		// Determine the difference, between the time now and the timestamp
		var difference = (current_time - timestamp) / 1000;
	
		var strings = Components.classes["@mozilla.org/intl/stringbundle;1"]
            .getService(Components.interfaces.nsIStringBundleService)
            .createBundle("chrome://mashtape/locale/mashtape.properties");

		// Set the periods of time
		var periods = [
			"extensions.mashTape.date.sec",
			"extensions.mashTape.date.min",
			"extensions.mashTape.date.hour",
			"extensions.mashTape.date.day",
			"extensions.mashTape.date.week",
			"extensions.mashTape.date.month",
			"extensions.mashTape.date.year",
			"extensions.mashTape.date.decade"
		];

		// Set the number of seconds per period
		var lengths = [1, 60, 3600, 86400, 604800,
			2630880, 31570560, 315705600];

		// Determine which period we should use, based on the number of seconds
		// lapsed.  If the difference divided by the seconds is more than 1,
		// we use that. Eg 1 year / 1 decade = 0.1, so we move on. Go from
		// decades backwards to seconds
		for (val = lengths.length - 1;
				(val >= 0) && ((number = difference / lengths[val]) <= 1);
				val--);

		// Ensure the script has found a match
		if (val < 0) val = 0;

		// Determine the minor value, to recurse through
		var new_time = current_time - (difference % lengths[val]);

		// Set the current value to be floored
		var number = Math.floor(number);

		// If required, make plural
		var key = periods[val];
		if(number != 1)
			key += "s";

		// Return text
		try {
			agoVal = strings.formatStringFromName(key, [number], 1);
		} catch (e) {
		}
	}
	return agoVal;
}

var mtUtils = {
	_prefBranch : Cc["@mozilla.org/preferences-service;1"]
						.getService(Ci.nsIPrefService)
						.getBranch("extensions.mashTape."),
						
	_errorConsole : Cc["@mozilla.org/consoleservice;1"]
						.getService(Ci.nsIConsoleService),

	log: function(providerName, msg) {
		if (!this._prefBranch.getBoolPref("debug"))
			return;

		this._errorConsole.logStringMessage("[" + providerName + "] " + msg);
	}
}
