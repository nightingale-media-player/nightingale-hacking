Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: MTV Music Video";
const CID         = "4926a290-b59b-11dd-ad8b-0800200c9a66";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/flash/MTVMusicVideo;2";

// XPCOM constructor for our MTVMusicVideo mashTape provider
function MTVMusicVideo() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashTape/mtUtils.jsm");
}

MTVMusicVideo.prototype.constructor = MTVMusicVideo;
MTVMusicVideo.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeFlashProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "MTV Music Video",
	providerType: "flash",
	providerUrl: "http://mtvmusic.com",
	providerIcon: "chrome://mashTape/content/tabs/mtv.png",

	query: function(artist, callback) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://api.mtvnservices.com/1/video/search/?" +
			"term=" + artist + "&max-results=100&start-index=1"
		mtUtils.log("MTVMusicVideo", "URL: " + url);

		var name = this.providerName;
		var providerurl = this.providerUrl;

		req.open("GET", url, true);
		req.onreadystatechange = function(ev) {
			return function(updateFn) {
				if (req.readyState != 4)
					return;
				if (req.status == 200) {
					var x = new XML(req.responseText.replace(
						/<\?xml version="1.0" encoding="iso-8859-1"\?>/, ""));
					var mrssNs = new Namespace("http://search.yahoo.com/mrss/");
					var atomNs = new Namespace("http://www.w3.org/2005/Atom");

					var results = new Array();
					for each (var entry in x..atomNs::entry) {
						var dateObj = new Date();
						dateObj.setISO8601(entry.atomNs::published);

						// Find the largest thumbnail
						var thumbnail;
						var thumbnailWidth = 0;
						for each (var thumb in entry..mrssNs::thumbnail) {
							if (parseInt(thumb.@width) > thumbnailWidth) {
								thumbnailWidth = thumb.@width;
								thumbnail = thumb.@url;
							}
						}

						var item = {
							title: entry.atomNs::title,
							url: entry.mrssNs::player.@url,
							swfUrl: entry.mrssNs::content.@url,
							duration: entry.mrssNs::content.@duration,
							time: dateObj.getTime(),
							thumbnail: thumbnail,
							author: entry.atomNs::author.atomNs::name,
							authorUrl: entry.atomNs::author.atomNs::uri,
							description: "",
							provider: name,
							providerUrl: providerurl,
							ratio: 1.33,
							width: 320,
							height: 240
						}
						results.push(item);
					}

					results.wrappedJSObject = results;
					updateFn.wrappedJSObject.update(CONTRACTID, results);
				}
			}(callback);
		}
		req.send(null);
	},
}

var components = [MTVMusicVideo];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([MTVMusicVideo]);
}
