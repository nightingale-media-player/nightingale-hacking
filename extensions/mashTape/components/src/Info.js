Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
var JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

const DESCRIPTION = "mashTape Provider: UberMash Artist Info Provider";
const CID         = "{7792e470-75ec-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/info/UberMash;1";

// XPCOM constructor for our Artist Info mashTape provider
function ArtistInfo() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");

	if (typeof(language) == "undefined") {
		// Get the user's locale
		var prefBranch = Cc["@mozilla.org/preferences-service;1"]
				.getService(Ci.nsIPrefService).getBranch("general.");
		var locale = prefBranch.getCharPref("useragent.locale");
		language = locale.split(/-/)[0];
		// last.fm screws up ja as jp
		if (language == "ja")
			language = "jp";
		mtUtils.log("Info", "language set to " + language);
		
	}
}

var linkMap = {
	"Musicmoz":"MusicMoz",
	"Discogs":"Discogs",
	"Wikipedia":"Wikipedia",
	"OfficialHomepage":"Homepage",
	"Fanpage":"Fan site",
	"IMDb":"IMDB",
	"Myspace":"MySpace",
	"Purevolume":"PureVolume"
};

var language;

var strings = Components.classes["@mozilla.org/intl/stringbundle;1"]
	.getService(Components.interfaces.nsIStringBundleService)
	.createBundle("chrome://mashtape/locale/mashtape.properties");

var Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);

ArtistInfo.prototype.constructor = ArtistInfo;
ArtistInfo.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeInfoProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Last.fm + MusicBrainz + Freebase",
	providerType: "info",
	numSections: 6,
	providerIconBio : "chrome://mashtape/content/tabs/lastfm.png",
	providerIconDiscography: "chrome://mashtape/content/tabs/musicbrainz.png",
	providerIconMembers: "chrome://mashtape/content/tabs/freebase.png",
	providerIconTags: "chrome://mashtape/content/tabs/lastfm.png",
	providerIconLinks: "chrome://mashtape/content/tabs/musicbrainz.png",

	triggerLastFM: function(artist, callback) {
		/* Go retrieve the artist bio */
		var bioReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo" +
			"&api_key=ad68d3b69dee88a912b193a35d235a5b" +
			"&artist=" + encodeURIComponent(artist);
		var prefBranch = Cc["@mozilla.org/preferences-service;1"]
			.getService(Ci.nsIPrefService).getBranch("extensions.mashTape.");
		var autolocalise = prefBranch.getBoolPref("info.autolocalise");
		if (autolocalise && language)
			url += "&lang=" + language;
		mtUtils.log("Info", "Last.FM Bio URL: " + url);
		bioReq.open("GET", url, true);
		bioReq.onreadystatechange = function(ev) {
			return function(artistName, updateFn) {
				if (bioReq.readyState != 4)
					return;
				if (bioReq.status == 200) {
					var xmlText = bioReq.responseText.replace(
							/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
							"");
					var x = new XML(xmlText);

					var imgUrl;
					var bio = new Object;
					for each (var img in x.artist.image) {
						if (img.@size.toString() === "small") {
							imgUrl = img;
						} else if (img.@size.toString() === "large") {
							imgUrl = img;
						} else if (img.@size.toString() === "medium") {
							imgUrl = img;
						}
					}

					bio.provider = "Last.fm";
					bio.bioUrl = "http://www.last.fm/music/" + artistName;
					bio.bioEditUrl = "http://www.last.fm/music/" +
							artistName + "/+wiki/edit";
					var bioContent =x.artist.bio.content.toString().split("\n");
					if (bioContent.length == 0 ||
							(x.artist.bio.content.toString() == ""))
					{
						bio.bioText = null;
						updateFn.wrappedJSObject.update(CONTRACTID, bio, "bio");
						updateFn.wrappedJSObject.update(CONTRACTID,
								null, "photo");
						return;
					}

					while (bioContent[bioContent.length-1].match(/^\s*$/))
						bioContent.pop();
					bio.bioText = "<p>" + bioContent.join("</p><p>") + "</p>";
          bio.bioText = bio.bioText.replace(/http:\/\/ws.audioscrobbler.com/g, 'http://last.fm');
					
					updateFn.wrappedJSObject.update(CONTRACTID, bio, "bio");
					updateFn.wrappedJSObject.update(CONTRACTID, imgUrl,"photo");
				}
			}(artist, callback);
		}
		bioReq.send(null);

		/* Now retrieve the artist tags */
		var tagsReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://ws.audioscrobbler.com/1.0/artist/" +
			escape(artist) + "/toptags.xml";
		mtUtils.log("Info", "Last.FM Tags URL: " + url);
		tagsReq.open("GET", url, true);
		tagsReq.updateFn = callback;
		tagsReq.artist = artist;
		tagsReq.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var xmlText = this.responseText.replace(
						/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
						"");
				var x = new XML(xmlText);

				var data = new Object;
        data.provider = "Last.fm";
				data.url = "http://www.last.fm/music/" +
					encodeURIComponent(this.artist) + "/+tags";
				data.tags = new Array();
				for each (var tag in x..tag) {
					data.tags.push({
						name: tag.name,
						count: tag.count,
						url: tag.url
					});
				}

				if (data.tags.length == 0) {
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							null, "tags");
					return;
				}
				data.wrappedJSObject = data;
				this.updateFn.wrappedJSObject.update(CONTRACTID, data, "tags");
			} else {
				this.updateFn.wrappedJSObject.update(CONTRACTID, null, "tags");
			}
		}
		tagsReq.send(null);
	},

	triggerFreebase: function(artist, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		artist = artist.replace(/&/g, "%26");
		var url = 'http://www.freebase.com/api/service/mqlread?queries={' +
			'"qDiscography":{"query":[{"album":[{"id":null,"name":null,"release_date":null}],"id":null,"name":"' + (artist) + '","type":"/music/artist"}]},' +
			'"qMembers":{"query":[{"member":[{"*":null}],"name":"' + (artist) + '","type":"/music/musical_group"}]},' +
			'"qArticle":{"query":[{"/common/topic/article":[{"*":null}],"name":"' + (artist) + '","type":"/music/artist"}]},' +
			'"qImages":{"query":[{"/common/topic/image":[{"*":null}],"name":"' + (artist) + '","type":"/music/artist"}]},' +
			'"qLinks":{"query":[{"/common/topic/webpage":[{"*":null}],"name":"' + (artist) + '","type":"/music/artist"}]}' +
		'}';
		mtUtils.log("Info", "Freebase URL: " + url);
		req.open("GET", url, true);
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var results = JSON.decode(this.responseText);
			
				if (typeof(results.qMembers.result) == 'undefined' ||
						results.qMembers.result.length == 0)
				{
					// No member relationships found
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							null, "members");
				} else {
					var data = new Object;
					data.provider = "Freebase";
					data.url = "http://www.freebase.com";
					data.members = new Array();
					if (typeof(results.qMembers.result[0].member) !=
							'undefined')
					{
						var members = results.qMembers.result[0].member;
						for (var i in members)
							data.members.push(members[i].member);
					}
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							data, "members");
				}
			}
		}
		req.send(null);
	},

	triggerMusicBrainz: function(artist, updateFn) {
		// First lookup the artist to get the MBID
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url =
			"http://musicbrainz.org/ws/1/artist/?type=xml&inc=sa-Album&query=";
		mtUtils.log("Info", "MusicBrainz URL: " + url +
				encodeURIComponent(artist));
		req.open("GET", url + artist, true);
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var xmlText = this.responseText.replace(
						/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
						"");
				var x = new XML(xmlText);

				var mbns = new Namespace('http://musicbrainz.org/ns/mmd-1.0#');
				var extns = new Namespace('http://musicbrainz.org/ns/ext-1.0#');

				if (x..mbns::artist.length() <= 0) {
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							null, "links");
					return;
				}

				var mbId = x..mbns::artist[0].@id;
				mtUtils.log("Info", "MBID: " + mbId);
				
				var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
					.createInstance(Ci.nsIXMLHttpRequest);
				var url = "http://musicbrainz.org/ws/1/artist/" + mbId +
					"?type=xml&inc=url-rels";
				req.open("GET", url, true);
				req.updateFn = updateFn;
				req.mbId = mbId;
				req.onreadystatechange = function() {
					if (this.readyState != 4)
						return;
					if (this.status != 200)
						return;
					var xmlText = this.responseText.replace(
							/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
							"");
					var x = new XML(xmlText);
					var mbns =
						new Namespace('http://musicbrainz.org/ns/mmd-1.0#');

					var data = new Object;
					data.provider = "MusicBrainz";
					data.url = "http://www.musicbrainz.org/artist/" + this.mbId;
					data.links = new Array();
					if (x..mbns::relation.length() == 0) {
						this.updateFn.wrappedJSObject.update(CONTRACTID,
								null, "links");
						return;
					}
					for each (var link in x..mbns::relation) {
						var type = link.@type.toString();
						var url = link.@target.toString();
						
						var name;
						if (typeof(linkMap[type]) != 'undefined')
							name = linkMap[type];
						else
							name = type;
						if (type == "Wikipedia") {
							var country = url.substr(7,2);
							name += " (" + country + ")";
						}
						data.links.push({name:name, url:url});
					}

          // Add provider (MusicBrainz) to the list of links
          data.links.push({name:data.provider, url:data.url});
					
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							data, "links");
				};
				req.send(null);
			}
		}
		req.send(null);

		// Look up discography information
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://musicbrainz.org/ws/1/release/?type=xml" +
			"&artist=" + artist + "&releasetypes=Album+Official&limit=100";
		mtUtils.log("Info", "MusicBrainz Discography URL: " + url);
		req.open("GET", url, true);
		req.updateFn = updateFn;
		req.albumMetadata = this.getAlbumMetadata;
		req.artist = artist;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var xmlText = this.responseText.replace(
						/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/,
						"");
				var x = new XML(xmlText);
				var mbns = new Namespace('http://musicbrainz.org/ns/mmd-1.0#');
				var data = new Object;
				if (x..mbns::release.length() == 0) {
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							null, "discography");
					return;
				}
				for each (var release in x..mbns::release) {
					// Use the oldest release date to avoid re-releases, and
					// track whether this is a US release (for foreign/import
					// de-duping - see below)
					var releaseDate = "9999";
					var usRelease = false;
					for each (var event in release..mbns::event) {
						if (event.@country.toUpperCase() == "US")
							usRelease = true;
						if (event.@date.toString() < releaseDate) {
							releaseDate = event.@date.toString();
						}
					}
					if (releaseDate == "" || releaseDate == "9999")
						releaseDate = null;

					// Craft an URL to an Amazon link from the ASIN
					var disableAmazon = Application.prefs.getValue(
								"extensions.mashTape.info.amazonstore.disabled",
								false);
					var asin = release.mbns::asin.toString();
					var link = null;
					var tooltip = null;
					if (!disableAmazon && asin != "") {
						link = "http://www.amazon.com/gp/product/" + asin;
						tooltip = strings.GetStringFromName(
								"extensions.mashTape.info.amazon");
					}
					
					var albumArt = null;
					
					var title = release.mbns::title.toString();
					var item = {
						mbid: release.@id,
						title: title,
						asin: release.mbns::asin.toString(),
						artwork: albumArt,
						release_date: releaseDate,
						artistMbid: release.mbns::artist.@id.toString(),
						artistName: release.mbns::artist.mbns::name.toString(),
						link: link,
						tooltip: tooltip,
					};

					// Only send one (let's prefer the US) of each title
					// You can thank "Weezer" for giving me headaches on this
					if (typeof(data[title]) != "undefined" && !usRelease)
						continue;

					// Otherwise let's track this
					data[title] = item;
				}

				data.pending = 0;
				data.artist = this.artist;
				for (var i in data) {
					if (typeof(data[i]) == "object") {
						data.pending++;
						this.albumMetadata(i, data, this.updateFn);
					}
				}
			}
		}
		req.send(null);
	},

	getAlbumMetadata : function(i, data, updateFn) {
		var url = "http://ws.audioscrobbler.com/2.0/?method=album.getinfo" +
			"&api_key=ad68d3b69dee88a912b193a35d235a5b" +
			"&mbid=" + data[i].mbid;
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		mtUtils.log("Info", "Last.fm Album URL: " + url);
		req.open("GET", url, true);
		req.updateFn = updateFn;
		req.data = data;
		req.i = i;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var xmlText = req.responseText.replace(
					/<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/, "");
				mtUtils.log("Info", "albumMetadata: " + 
						this.data[this.i].mbid + " complete! " +
						this.data.pending + " left.");
				var x = new XML(xmlText);
				var url = x.album.url.toString();
				if (url != "") {
					this.data[this.i].link = url;
					this.data[this.i].tooltip = strings.GetStringFromName(
							"extensions.mashTape.info.lastfm");
				}
				var artwork = null;
				for each (var image in x..image) {
					if (image.@size.toString() == "medium")
						artwork = image.toString();
				}
				this.data[this.i].artwork = artwork;
			}
			this.data.pending--;
			if (this.data.pending == 0) {
				var results = new Object;
				results.discography = new Array();
				var mbId;
				for each (var item in this.data) {
					if (typeof(item) == "object") {
						if (item.artistName == this.data.artist)
							mbId = item.artistMbid;
						results.discography.push(item);
					}
				}

				if (results.length == 0)
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							null, "discography");
				else {
					results.provider = "MusicBrainz";
					results.url = "http://www.musicbrainz.org/artist/" + mbId;
					this.updateFn.wrappedJSObject.update(CONTRACTID,
							results, "discography");
				}
			}
		}
		req.send(null);
	},

	query: function(artist, updateFn) {
		this.triggerLastFM(artist, updateFn);
		this.triggerFreebase(artist, updateFn);
		this.triggerMusicBrainz(artist, updateFn);
	},
}

var components = [ArtistInfo];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([ArtistInfo]);
}

