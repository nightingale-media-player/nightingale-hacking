Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: MTV Music News Provider";
const CID         = "{f6256890-79d3-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/MTV;1";

// XPCOM constructor for our MTV mashTape provider
function MTV() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

MTV.prototype.constructor = MTV;
MTV.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "MTV Music News",
	providerUrl: "http://www.mtv.com",
	providerType: "rss",
	providerIcon: "chrome://mashtape/content/tabs/mtv.gif",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://www.mtv.com/music/artist/" +
			searchTerms.replace(/ /g, '_').toLowerCase() +
			"/rss/highlights_full.jhtml";
		mtUtils.log("MTV", "URL:" + url);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="iso-8859-1"?>', ""));
				var mrssNs = new Namespace('http://search.yahoo.com/mrss/');
				var results = new Array();
				for each (var item in x..item) {
					var content = item.description.toString();
					content = content.replace(
							/<embed src="\/player\//, 
							"<embed src=\"http://www.mtv.com/player/");
					var newsitem = {
						title: item.title,
						time: Date.parse(item.pubDate),
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						url: item.link,
						content: content,
					}
					results.push(newsitem);
				}
				
				results.wrappedJSObject = results;
				this.updateFn.wrappedJSObject.update(CONTRACTID, results);
			}
		}
		req.send(null);
	},
}

var components = [MTV];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([MTV]);
}

