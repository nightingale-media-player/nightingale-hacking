Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Google News Provider";
const CID         = "{4931c3c0-64d6-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/GoogleNews;1";

// XPCOM constructor for our Google News mashTape provider
function GoogleNews() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

GoogleNews.prototype.constructor = GoogleNews;
GoogleNews.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Google News",
	providerUrl: "http://news.google.com",
	providerType: "rss",
	providerIcon: "chrome://mashtape/content/tabs/google.gif",
	searchURL: "http://news.google.com/news?hl=en&ned=us&ie=UTF-8" +
		"&nolr=1&output=atom&q=",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		mtUtils.log("GoogleNews", "URL:" + this.searchURL +
				escape('"' + searchTerms + '"'));
		req.open("GET", this.searchURL + escape('"'+searchTerms+'"'), true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var results = new Array();
				var x = new XML(this.responseText.replace(
						/<\?xml version="1.0"[^>]*>/, ""));
				var atomns = new Namespace('http://www.w3.org/2005/Atom');
				for each (var entry in x..atomns::entry) {
					var dateObj = new Date();
					dateObj.setISO8601(entry.atomns::updated);

					var content = entry.atomns::content.toString();
					content = content.replace(
							/<img alt="" height="1" width="1">/, "");
					var item = {
						title: entry.atomns::title,
						time: dateObj.getTime(),
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						url: entry.atomns::link.@href,
						content: content,
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

var components = [GoogleNews];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([GoogleNews]);
}

