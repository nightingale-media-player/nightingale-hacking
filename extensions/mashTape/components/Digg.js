Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Digg Provider";
const CID         = "{9a301ac0-745c-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/Digg;1";

function debugLog(funcName, str) {
	dump("*** Digg.js::" + funcName + " // " + str + "\n");
}

// XPCOM constructor for our Digg mashTape provider
function Digg() {
	this.wrappedJSObject = this;
}

Digg.prototype.constructor = Digg;
Digg.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Digg",
	providerType: "rss",
	providerUrl: "http://digg.com",
	providerIcon: "chrome://mashtape/content/tabs/digg.gif",
	searchURL: "http://digg.com/rss_search?area=promoted&type=both&section=all"
		+ "&search=",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		req.open("GET", this.searchURL + escape('"'+searchTerms+'"'), true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var results = new Array();
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="UTF-8"?>', ""));
				for each (var entry in x..item) {
					var pubDate = entry.pubDate.toString();
					pubDate = pubDate.replace(/\+0000$/, "GMT");
					var timestamp = new Date(pubDate);
					var item = {
						title: entry.title,
						url: entry.link,
						time: timestamp.getTime(),
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						content: entry.description,
					}
					results.push(item);
				}
				this.length = x..item.length();
				
				results.wrappedJSObject = results;
				this.updateFn.wrappedJSObject.update(CONTRACTID, results);
			} else {
				debugLog("process", "FAIL: status code:" + this.status);
				this.updateFn.wrappedJSObject.update(CONTRACTID, null);
			}
		}
		req.send(null);
	},
}

var components = [Digg];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([Digg]);
}

