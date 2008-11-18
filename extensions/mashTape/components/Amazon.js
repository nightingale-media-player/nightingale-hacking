Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Amazon Reviews Provider";
const CID         = "{96a0c280-b4d3-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/review/Amazon;1";

function Amazon() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

Amazon.prototype.constructor = Amazon;
Amazon.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeReviewProvider,
			Ci.sbIMashTapeProvider]),

	providerName: "Amazon",
	providerUrl: "http://amazon.com",
	providerType: "review",
	providerIcon: "chrome://mashtape/content/tabs/amazon.png",

	query: function(searchTerms, updateFn) {
	},

	queryFull: function(artist, album, track, callback) {
		// Fire off an ItemSearch to get ASINs to do ItemLookups on
		var searchReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://ecs.amazonaws.com/onca/xml?Service=AWSECommerceService&AWSAccessKeyId=1DQMN6E25AX32KXFT902&Operation=ItemSearch&SearchIndex=Music&Artist=" + encodeURIComponent(artist) + "&Title=" + encodeURIComponent(album);
		mtUtils.log("Amazon", "ItemSearch URL: " + url);
		var reviewFn = this.getReviews;
		searchReq.open("GET", url, true);
		searchReq.onreadystatechange = function(ev) {
			return function(artistName, albumName, trackName,
					reviewFn, updateFn) {
				if (searchReq.readyState != 4)
					return;
				if (searchReq.status == 200) {
					var xmlText = searchReq.responseText.replace(
							/<\?xml version="1.0" [^>]*>/, "");
					var x = new XML(xmlText);
					var ns = new Namespace("http://webservices.amazon.com/AWSECommerceService/2005-10-05");
					var data = new Object;
					data.count = 0;
					data.asins = new Array;
					data.reviews = new Array;
					data.reviewHash = new Object;

					var asins = new Object;
					for each (var item in x..ns::Item) {
						var asin = item.ns::ASIN.toString();
						if (typeof(asins[asin]) != "undefined")
							continue;

						asins[asin] = 1;
						data.asins.push(asin);
						mtUtils.log("Amazon", "Found ASIN: " + asin);
						data.count++;
					}
					for (var i=0; i<data.asins.length; i++)
						reviewFn(i, data, updateFn);
					if (data.asins.length == 0)
						updateFn.wrappedJSObject.update(CONTRACTID, null);
				}
			}(artist, album, track, reviewFn, callback);
		}
		searchReq.send(null);
	},

	getReviews: function(idx, data, updateFn) {
		var asin = data.asins[idx];
		var reviewReq = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
			.createInstance(Ci.nsIXMLHttpRequest);
		var url = "http://ecs.amazonaws.com/onca/xml?Service=AWSECommerceService&Version=2005-03-23&Operation=ItemLookup&SubscriptionId=1BTS253HGNBMVK0CP6G2%20%20&ItemId=" + asin + "&ResponseGroup=EditorialReview,Reviews&ReviewSort=-HelpfulVotes";
		mtUtils.log("Amazon", "ItemLookup (" + asin + ") URL: " + url);
		reviewReq.open("GET", url, true);
		reviewReq.onreadystatechange = function(ev) {
			return function(idx, data, updateFn) {
				if (reviewReq.readyState != 4)
					return;
				if (reviewReq.status == 200) {
					var xmlText = reviewReq.responseText.replace(
							/<\?xml version="1.0" [^>]*>/, "");
					var x = new XML(xmlText);
					var ns = new Namespace("http://webservices.amazon.com/AWSECommerceService/2005-03-23");

					for each (var r in x..ns::CustomerReviews.ns::Review) {
						var timestamp = r.ns::Date.toString();
						var year = timestamp.substr(0,4);
						var mon = timestamp.substr(5,2) - 1;
						var date = timestamp.substr(8,2);
						var dateObj = new Date(year, mon, date);
						var review = {
							sortRank: r.ns::HelpfulVotes,
							rating: r.ns::Rating,
							author: r.ns::CustomerId,
							time: dateObj.getTime(),
							title: r.ns::Summary,
							content: r.ns::Content.toString()
						}

						mtUtils.log("Amazon", "Found review: " + review.title);
						data.reviews.push(review);
					}

					// assemble the editorial review
					for each (var r in x..ns::EditorialReview) {
						if (r.ns::Source.toString().indexOf("Amazon") == -1)
							continue;
						var editorial = {
							sortRank: 9999999,
							rating: -1,
							author: r.ns::Source.toString(),
							time: 0,
							title: r.ns::Source.toString() + " Editorial",
							content: r.ns::Content.toString()
						}

						mtUtils.log("Amazon", "Found editorial review.");
						data.reviews.push(editorial);
					}

					// if this was the last one, then call the display callback
					data.count--;
					if (data.count == 0) {
						mtUtils.log("Amazon", "Total reviews found: " +
								data.reviews.length);
						// filter out the duplicates
						var hashes = new Object;
						var results = new Array;
						for (var i=0; i<data.reviews.length; i++) {
							var r = data.reviews[i];
							if (typeof(hashes[r.content]) != "undefined")
								continue;
							hashes[r.content] = 1;
							results.push(r);
						}
						// re-sort the reviews
						mtUtils.log("Amazon", "Post-filtering: " +
								results.length);
						updateFn.update(CONTRACTID, results);
					}
				}
			}(idx, data, updateFn.wrappedJSObject)
		}
		reviewReq.send(null);
	}
}

var components = [Amazon];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([Amazon]);
}

