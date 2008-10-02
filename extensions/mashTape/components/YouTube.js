Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: YouTube Provider";
const CID         = "{e0f43860-6a5c-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/flash/YouTube;1";

function debugLog(funcName, str) {
	dump("*** YouTube.js::" + funcName + " // " + str + "\n");
}

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

// XPCOM constructor for our YouTube mashTape provider
function YouTube() {
	this.wrappedJSObject = this;
}

YouTube.prototype.constructor = YouTube;
YouTube.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeFlashProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "YouTube",
	providerType: "flash",
	providerUrl: "http://www.youtube.com",
	providerIcon: "chrome://mashTape/content/tabs/youtube.png",
	searchURL: "http://gdata.youtube.com/feeds/videos?vq=",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var query = "%22" + escape(searchTerms) + "%22";
		debugLog("URL", this.searchURL + query);
		req.open("GET", this.searchURL + query, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var results = new Array();
				var x = new XML(this.responseText.replace(
						"<?xml version='1.0' encoding='UTF-8'?>", ""));

				// Define our XML namespaces
				var atomns = new Namespace('http://www.w3.org/2005/Atom');
				var medians = new Namespace('http://search.yahoo.com/mrss/');
				var ytns = new
					Namespace('http://gdata.youtube.com/schemas/2007');

				for each (var entry in x..atomns::entry) {
					// Find the <link> that is to the HTML page
					var url;
					for each (var link in entry.atomns::link) {
						if (link.@type == "text/html") {
							url = link.@href;
							break;
						}
					}

					// Find the media:content that's the SWF
					var swfUrl = null;
					for each (var media in entry..medians::content) {
						if (media.@type == "application/x-shockwave-flash") {
							swfUrl = media.@url;
							break;
						}
					}
					if (swfUrl == null)
						continue;

					// We want to externally script the swf player
					swfUrl += "&enablejsapi=1";

					// Find the largest thumbnail
					var thumbnail;
					var thumbnailWidth = 0;
					for each (var thumb in entry..medians::thumbnail) {
						if (parseInt(thumb.@width) > thumbnailWidth) {
							thumbnailWidth = thumb.@width;
							thumbnail = thumb.@url;
						}
					}
					
					// Calculate the publication date
					var dateObj = new Date();
					dateObj.setISO8601(entry.atomns::published);

					var authorUrl = "http://www.youtube.com/user/" +
						entry.atomns::author.atomns::name;

					var item = {
						title: entry.atomns::title,
						url: url,
						swfUrl: swfUrl,
						duration: entry..ytns::duration.@seconds,
						time: dateObj.getTime(),
						thumbnail: thumbnail,
						author: entry.atomns::author.atomns::name,
						//authorUrl: entry.atomns::author.atomns::uri,
						authorUrl: authorUrl,
						description: entry.atomns::content,
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						ratio: 1.33,
						width: 320,
						height: 240,
					}
					results.push(item);
				}

				results.wrappedJSObject = results;
				this.updateFn.wrappedJSObject.update(CONTRACTID, results);
			}
		}
		req.send(null);
	},
}

var components = [YouTube];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([YouTube]);
}
