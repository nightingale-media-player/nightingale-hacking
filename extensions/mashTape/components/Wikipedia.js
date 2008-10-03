Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://gre/modules/JSON.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Wikipedia (localized) Artist Info Provider";
const CID         = "{a7780910-8c10-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/info/WikipediaLocal;1";

function debugLog(funcName, str) {
	dump("*** Wikipedia.js::" + funcName + " // " + str + "\n");
}

var language;
var homepage;
var wikipedia;

// XPCOM constructor for our Artist Info mashTape provider
function ArtistInfo() {
	this.wrappedJSObject = this;

	if (typeof(language) == "undefined") {
		// Get the user's locale
		var prefBranch = Cc["@mozilla.org/preferences-service;1"]
				.getService(Ci.nsIPrefService).getBranch("general.");
		var locale = prefBranch.getCharPref("useragent.locale");
		language = locale.split(/-/)[0];
		debugLog("Constructor", "language set to " + language);
	}
	if (typeof(homepage) == "undefined") {
		// Initialise to English for now
		homepage = "Homepage";
		wikipedia = "Wikipedia";

		if (language == "en")
			return;
		var gtReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
				.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://ajax.googleapis.com/ajax/services/language/translate"
			+ "?v=1.0&q=Wikipedia%7CHomepage&langpair=en%7C" + language;
		debugLog("Constructor", "translating homepage & wikipedia terms");
		debugLog("Constructor translate URL", url);
		gtReq.open("GET", url, true);
		gtReq.onreadystatechange = function(ev) {
			if (this.readyState != 4)
				return;
			if (this.status != 200)
				return;
			var results = JSON.fromString(this.responseText);
			if (results.responseStatus == 200) {
				var text = results.responseData.translatedText.split("|");
				wikipedia = text[0].replace(/^[\s]*/, '').replace(/[\s]*$/, '');
				homepage = text[1].replace(/^[\s]*/, '').replace(/[\s]*$/, '');
				debugLog("Homepage", homepage);
				debugLog("Wikipedia", wikipedia);
			}
		}
		gtReq.send(null);
	}
}

ArtistInfo.prototype.constructor = ArtistInfo;
ArtistInfo.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeInfoProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Wikipedia Auto-Localized",
	providerType: "info",
	numSections: 4,
	providerIconBio : "chrome://mashtape/content/tabs/wikipedia.png",
	providerIconDiscography : "chrome://mashtape/content/tabs/wikipedia.png",
	providerIconMembers : "chrome://mashtape/content/tabs/wikipedia.png",
	providerIconTags : "chrome://mashtape/content/tabs/wikipedia.png",
	providerIconLinks : "chrome://mashtape/content/tabs/wikipedia.png",

	query: function(artist, callback) {
		// Initiate the request
		var dbReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
				.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://dbpedia.org:8890/sparql?" +
				"default-graph-uri=http%3A%2F%2Fdbpedia.org" +
				"&query=DESCRIBE+%3Chttp://dbpedia.org/resource/" +
				artist.replace(/ /g, "_") + "%3E&output=xml";
		debugLog("Wikipedia (DBpedia) URL", url);
		dbReq.open("GET", url, true);
		dbReq.onreadystatechange = function(ev) {
			return function(updateFn) {
				if (dbReq.readyState != 4)
					return;
				if (dbReq.status != 200)
					return;
				var xmlText = dbReq.responseText.replace(
						/<\?xml version="1.0" encoding="utf-8" \?>/,
						"");
				var x = new XML(xmlText);

				// Set our E4X namespaces
				var dbNs = new Namespace("http://dbpedia.org/property/");
				var foafNs = new Namespace("http://xmlns.com/foaf/0.1/");
				var rdfNs = new Namespace(
								"http://www.w3.org/1999/02/22-rdf-syntax-ns#");
				var xmlNs = new Namespace(
								"http://www.w3.org/XML/1998/namespace");

				// Links
				var links = new Object;
				links.provider = "Wikipedia";
				links.links = new Array();
				if (x..foafNs::homepage.length() >= 1) {
					links.links.push({
						name: homepage,
						url: x..foafNs::homepage[0].@rdfNs::resource
					});
				}
				
				var wikiUrl = null;
				var enUrl = null;
				for each (var resource in x..rdfNs::Description) {
					var child = resource.child(0);
					if (child.localName() == ("wikipage-" + language)) {
						wikiUrl = decodeURI(child.@rdfNs::resource);
						links.links.push({
							name: wikipedia,
							url: wikiUrl
						});
					} else if (child.localName() == ("page")) {
						enUrl = decodeURI(child.@rdfNs::resource);
						links.links.push({
							name: wikipedia,
							url: enUrl
						});
					}
				}
				if (wikiUrl == null)
					wikiUrl = enUrl;
				links.url = wikiUrl;
	
				var bio = new Object;
				var bioText = null;
				// Artist bio
				var englishBio = null;
				for each (var bio in x..dbNs::abstract) {
					var thisLang = bio.@xmlNs::lang.toString();
					debugLog("Language scan", "Found: " + thisLang);
					if (thisLang == language)
						bioText = bio.toString();
					if (thisLang == "en")
						englishBio = bio.toString();
				}
				if (bioText == null)
					bioText = englishBio;
				if (bioText == null)
					updateFn.wrappedJSObject.update(CONTRACTID, null, "bio");
				else {
					bio.provider = "Wikipedia";
					bio.bioText = bioText;
					bio.bioUrl = wikiUrl;
					bio.bioEditUrl = wikiUrl;
					updateFn.wrappedJSObject.update(CONTRACTID, bio, "bio");
				}

				// Members
				var members = new Object;
				members.provider = "Wikipedia";
				members.url = wikiUrl;
				members.members = new Array();
				if (x..dbNs::currentMembers.length() == 1 &&
					x..dbNs::currentMembers.@rdfNs::resource.toString() == "")
				{
					var memberStr = x..dbNs::currentMembers.toString();
					members.members = memberStr.split(/\n/);
				} else {
					for each (var memberName in x..dbNs::currentMembers) {
						var name = memberName.@rdfNs::resource.toString();
						var uri = name.split(/\//);
						name = unescape(uri[uri.length-1].replace(/_/g, " "));
						members.members.push(name);
					}
				}

			
				var image = x..dbNs::img.@rdfNs::resource.toString();
				if (image == "")
					updateFn.wrappedJSObject.update(CONTRACTID, null, "photo");
				else {
					updateFn.wrappedJSObject.update(CONTRACTID, image, "photo");
				}

				if (members.members.length > 0)
					updateFn.wrappedJSObject.update(CONTRACTID, members,
						"members");
				else
					updateFn.wrappedJSObject.update(CONTRACTID, null,
						"members");

				if (links.links.length > 0)
					updateFn.wrappedJSObject.update(CONTRACTID, links, "links");
				else
					updateFn.wrappedJSObject.update(CONTRACTID, null, "links");
			}(callback);
		}
		dbReq.send(null);
	},
}

var components = [ArtistInfo];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([ArtistInfo]);
}

