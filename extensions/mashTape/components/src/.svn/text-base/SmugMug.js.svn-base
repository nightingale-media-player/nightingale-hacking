Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: SmugMug Provider";
const CID         = "{68551fc0-6a50-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/photo/SmugMug;1";

// XPCOM constructor for our SmugMug mashTape provider
function SmugMug() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

SmugMug.prototype.constructor = SmugMug;
SmugMug.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapePhotoProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "SmugMug",
	providerType: "photo",
	providerIcon: "chrome://mashTape/content/tabs/smugmug.png",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);

		var prefBranch = Cc["@mozilla.org/preferences-service;1"]
			.getService(Ci.nsIPrefService).getBranch("extensions.mashTape.");
		var keywords = prefBranch.getCharPref("photo.keywords");
		if (keywords != "")
			searchTerms += "%20" + escape(keywords);

		var url = "http://www.smugmug.com/hack/feed.mg?Type=keyword&format=rss&ImageCount=100&Data=" + escape(searchTerms);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var results = new Array();
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="utf-8"?>', ""));
				var mediaNs = new Namespace("http://search.yahoo.com/mrss/");

				for each (var entry in x..item) {
					if (entry..mediaNs::content.length() < 4)
						continue;
					var small = entry..mediaNs::content[1].@url;
					var med = entry..mediaNs::content[2].@url;
					var large = entry..mediaNs::content[3].@url;
					var escapedTitle = entry.title.split(/[\n\r]/).join(" ")
						.replace(/'/g, "\'");
					dump("title: " + escapedTitle + ":\n");
					var item = {
						title: escapedTitle,
						url: entry.link,
						small: small,
						medium: med,
						large: large,
						owner: entry.mediaNs::copyright.toString(),
						ownerUrl: entry.mediaNs::copyright.@url.toString(),
						time: Date.parse(entry.pubDate.toString()),
						width: entry..mediaNs::content[2].@width,
						height: entry..mediaNs::content[2].@height,
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

var components = [SmugMug];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([SmugMug]);
}
