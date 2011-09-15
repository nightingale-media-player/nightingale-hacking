Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Google Blog Search Provider";
const CID         = "{3814ef10-7a19-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/rss/GoogleBlogs;1";

// XPCOM constructor for our Yahoo News mashTape provider
function GoogleBlogs() {
	Components.utils.import("resource://mashtape/mtUtils.jsm");
	this.wrappedJSObject = this;
}

GoogleBlogs.prototype.constructor = GoogleBlogs;
GoogleBlogs.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeRSSProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Google Blog Search",
	providerUrl: "http://blogsearch.google.com",
	providerType: "rss",
	providerIcon: "chrome://mashtape/content/tabs/google.png",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://blogsearch.google.com/blogsearch_feeds?hl=en&c2coff=1&lr=lang_en&safe=off&q=%22" + searchTerms + "&ie=utf-8&num=10&output=rss";
		mtUtils.log("Google Blogs", "URL:" + url);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="UTF-8"?>', ""));

				var dcNs = new Namespace("http://purl.org/dc/elements/1.1/");
				var results = new Array();
				for each (var entry in x..item) {
					var dateObj = new Date();
					dateObj.setISO8601(entry.dcNs::date);
					var item = {
						title: entry.title,
						time: dateObj.getTime(),
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

var components = [GoogleBlogs];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([GoogleBlogs]);
}

