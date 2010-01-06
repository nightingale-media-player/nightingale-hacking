Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Hype Machine Provider";
const CID         = "{ad43af30-7a1c-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/HypeM;1";

// XPCOM constructor for our Yahoo News mashTape provider
function HypeM() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

HypeM.prototype.constructor = HypeM;
HypeM.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Hype Machine",
	providerUrl: "http://hypem.com",
	providerType: "rss",
	providerIcon: "chrome://mashtape/content/tabs/hypem.png",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://hypem.com/feed/search/" + searchTerms + "/1/feed.xml";
		mtUtils.log("HypeM", "URL:" + url);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var x = new XML(this.responseText.replace(
					/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/, ""));

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
			} else if (this.status == 404) {
				this.updateFn.wrappedJSObject.update(CONTRACTID, null);
			}
		}
		req.send(null);
	},
}

var components = [HypeM];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([HypeM]);
}

