Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://gre/modules/JSON.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Yahoo Music Provider";
const CID         = "{e9586520-7a11-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/flash/YahooMusic;1";

const ymAppId = 
	"bTRLglbV34HSXzLdIRsXlMwzHz61hHyGpQtdw_sqaP.QJw_k1jqFWZKbXTNnUhm6x0G6";
const ymSwfUrl =
	"http://d.yimg.com/cosmos.bcst.yahoo.com/up/fop/embedflv/swf/fop.swf";

function debugLog(funcName, str) {
	dump("*** YahooMusic.js::" + funcName + " // " + str + "\n");
}

// XPCOM constructor for our Yahoo Music mashTape provider
function YahooMusic() {
	this.wrappedJSObject = this;
}

YahooMusic.prototype.constructor = YahooMusic;
YahooMusic.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeFlashProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Yahoo Music",
	providerType: "flash",
	providerUrl: "http://music.yahoo.com",
	providerIcon: "chrome://mashTape/content/tabs/yahoo.ico",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://us.music.yahooapis.com/video/v1/list/search/video/" +
			searchTerms + "?appid=" + ymAppId + "&format=json";
		debugLog("URL", url);
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var videoResults = JSON.fromString(this.responseText);

				var results = new Array();
				for each (var video in videoResults.Videos.Video) {
					var id = video.id;
					if (typeof(id) == "undefined")
						continue;
					var time = video.copyrightYear;
					if (typeof(time) == "undefined")
						time = 0;
					else
						time = new Date(video.copyrightYear, 0, 1).getTime();
					var duration = video.duration;
					if (typeof(duration) == "undefined")
						duration = 0;

					var item = {
						title: video.title,
						url: "http://new.music.yahoo.com/videos/--" + id,
						swfUrl: "http://d.yimg.com/cosmos.bcst.yahoo.com/up/fop/embedflv/swf/fop.swf?id=v" + id + "&eID=1301797&ympsc=4195351&lang=en&enableFullScreen=1&autoStart=0&eh=mashTapeVideo.yahooListener",
						duration: duration,
						time: time,
						thumbnail: "http://d.yimg.com/img.music.yahoo.com/image/v1/video/" + id + "?size=120",
						author: video.Artist.name,
						authorUrl: video.Artist.website,
						description: "",
						provider: this.provider.providerName,
						providerUrl: this.provider.providerUrl,
						ratio: 1.59,
						width: 450,
						height: 283
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

var components = [YahooMusic];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([YahooMusic]);
}
