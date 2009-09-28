Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Provider: Amazon Reviews Provider";
const CID         = "{96a0c280-b4d3-11dd-ad8b-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/mashTape/provider/review/Amazon;1";

const AWSACCESSKEYID = "1DQMN6E25AX32KXFT902";
const AWSSECRETKEYID = "l981L1Q2PzJpFVLrCnJakS1+U1IvsD6ggVQDDO9w";

function Amazon() {
	this.wrappedJSObject = this;
	Components.utils.import("resource://mashtape/mtUtils.jsm");
}

function _buildQuery(queryParams) {
  // Get the timestamp
  var timestamp = new Date();
  timestamp = timestamp.toISO8601String(5);
  queryParams["Timestamp"] = timestamp;
  queryParams["AWSAccessKeyId"] = AWSACCESSKEYID;
  queryParams["Service"] = "AWSECommerceService";

  // Sort the array of query parameters
  var keys = new Array();
  for (var key in queryParams) {
    keys.push(key);
  }
  keys = keys.sort();

  // Build the query string
  var queryString = "";
  for each (var key in keys)
    queryString += "&" + key + "=" + encodeURIComponent(queryParams[key]);
  // Slice off the first "&"
  queryString = queryString.slice(1);

  // Sign it
  var stringToSign = "GET\necs.amazonaws.com\n/onca/xml\n";
  stringToSign += queryString;
  var shaObj = new jsSHA(stringToSign, "ASCII");
  var signature = shaObj.getHMAC(AWSSECRETKEYID, "ASCII", "SHA-256", "B64");

  // Make the URL
  var url = "http://ecs.amazonaws.com/onca/xml?" + queryString +
            "&Signature=" + encodeURIComponent(signature + "=");
  return url;
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

    var queryParams = new Array();
    queryParams["Artist"] = artist;
    queryParams["Title"] = album;
    queryParams["Operation"] = "ItemSearch";
    queryParams["SearchIndex"] = "Music";
    var url = _buildQuery(queryParams);

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

    var queryParams = new Array();
    queryParams["Version"] = "2005-03-23";
    queryParams["Operation"] = "ItemLookup";
    queryParams["SubscriptionId"] = "1BTS253HGNBMVK0CP6G2%20%20";
    queryParams["ItemId"] = asin;
    queryParams["ResponseGroup"] = "EditorialReview,Reviews";
    queryParams["ReviewSort"] = "-HelpfulVotes";
    var url = _buildQuery(queryParams);

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
							//author: r.ns::CustomerId.toString(),
							author: "Customer Review",
							authorUrl: "http://www.amazon.com/gp/pdp/profile/" +
								r.ns::CustomerId.toString(),
							time: dateObj.getTime(),
							title: r.ns::Summary,
							content: r.ns::Content.toString(),
							provider: "Amazon.com",
							url: "http://www.amazon.com/dp/" + asin
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
							authorUrl: "http://www.amazon.com/dp/" + asin + 
								"#productDescription",
							time: 0,
							title: r.ns::Source.toString() + " Editorial",
							content: r.ns::Content.toString(),
							provider: "Amazon.com",
							url: "http://www.amazon.com/dp/" + asin
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
							if (typeof(hashes[r.title]) != "undefined")
								continue;
							hashes[r.title] = 1;
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

