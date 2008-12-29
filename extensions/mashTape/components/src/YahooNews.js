Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Yahoo News Provider";
const CID         = "{92cd9a40-7a16-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/YahooNews;1";

// XPCOM constructor for our Yahoo News mashTape provider
function YahooNews() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

YahooNews.prototype.constructor = YahooNews;
YahooNews.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Yahoo News",
	providerUrl: "http://news.yahoo.com",
	providerType: "rss",
	providerIcon: "chrome://mashtape/content/tabs/yahoo.ico",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://news.search.yahoo.com/news/rss?p=\"" + searchTerms +
			"\"&ei=UTF-8";
		mtUtils.log("YahooNews", "URL: " + url);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="UTF-8" ?>', ""));

				var results = new Array();
				for each (var entry in x..item) {
					var item = {
						title: entry.title,
						time: Date.parse(entry.pubDate),
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						url: entry.link,
						content: entry.description,
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

var components = [YahooNews];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([YahooNews]);
}

