Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Flickr Provider";
const CID         = "{a86cc290-6990-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/photo/Flickr;1";

// XPCOM constructor for our Flickr mashTape provider
function Flickr() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

Flickr.prototype.constructor = Flickr;
Flickr.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapePhotoProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Flickr",
	providerType: "photo",
	providerIcon: "chrome://mashTape/content/tabs/flickr.png",
	limit: 100,
	sort: "&sort=relevance",
	searchType: "&text=",
	searchURL: "http://api.flickr.com/services/rest/?" +
		"method=flickr.photos.search&api_key=461354d3cdcc5425055670cbd3b8e16e" +
		"&extras=owner_name,date_taken,o_dims,original_format",

	query: function(searchTerms, updateFn) {
		var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);

		var prefBranch = Cc["@mozilla.org/preferences-service;1"]
			.getService(Ci.nsIPrefService).getBranch("extensions.mashTape.");
		var keywords = prefBranch.getCharPref("photo.keywords");
		if (keywords != "")
			searchTerms += "%20" + escape(keywords);

		var query = "&per_page=" + this.limit + this.sort + this.searchType +
			'"' + searchTerms + '"';
		/*
		if (this.searchType.substr(1,3) == "tex")
			query += "%20concert";
		else
			query += ",concert";
			*/
		var url = this.searchURL + query;

		var songbirdmode = false;
		if (searchTerms == "Songbird") {
			songbirdmode = true;
			// all photos tagged 'mashtape'
			//url = "http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=33a6c9b21ada1e5b7d85f5cde788e6c7&extras=owner_name,date_taken,o_dims,original_format&user_id=11878025@N03&tags=mashtape";
			// all photos in the mashTape set
			url = "http://api.flickr.com/services/rest/?method=flickr.photosets.getPhotos&api_key=33a6c9b21ada1e5b7d85f5cde788e6c7&photoset_id=72157607854460548&extras=owner_name,date_taken,o_dims,original_format";
		}

		mtUtils.log("Flickr", "URL:" + url);
			
		req.open("GET", url, true);
		req.provider = this;
		req.updateFn = updateFn;
		req.onreadystatechange = function() {
			if (this.readyState != 4)
				return;
			if (this.status == 200) {
				var x = new XML(this.responseText.replace(
						'<?xml version="1.0" encoding="utf-8" ?>', ""));
				var results = new Array();
				var set;
				if (songbirdmode)
					set = x..photoset.photo;
				else
					set = x..photos.photo;
				for each (var entry in set) {
					var origFormat = entry.@originalformat.toString();
					//if (origFormat == "")
					//	continue;

					var width = entry.@o_width.toString();
					var height = entry.@o_height.toString();

					var imgUrl = "http://farm" + entry.@farm +
						".static.flickr.com/" + entry.@server + "/" +
						entry.@id + "_" + entry.@secret; // + ".jpg";
					var url = "http://www.flickr.com/photos/" +
						entry.@owner + "/" + entry.@id;
					var timestamp = entry.@datetaken;
					var year = timestamp.substr(0,4);
					var mon = timestamp.substr(5,2) - 1;
					var date = timestamp.substr(8,2);
					var hour = timestamp.substr(11,2);
					var min = timestamp.substr(14,2);
					var sec = timestamp.substr(17,2);
					timestamp = Date.UTC(year,mon,date,hour,min,sec);
					var item = {
						title: entry.@title.toString(),
						url: url,
						small: imgUrl + "_m.jpg",
						medium: imgUrl + ".jpg",
						large: imgUrl + "_b.jpg",
						owner: entry.@ownername.toString(),
						ownerUrl: "http://flickr.com/photos/" +
							escape(entry.@ownername.toString()),
						time: timestamp,
						width: width,
						height: height,
					}
					results.push(item);
				}
				mtUtils.log("Flickr", x..photos.photo.length()+" photos found");

				results.wrappedJSObject = results;
				this.updateFn.wrappedJSObject.update(CONTRACTID, results);
			}
		}
		req.send(null);
	},
}

var components = [Flickr];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([Flickr]);
}


