Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "mashTape Manager";
const CID         = "{066D6E67-39FE-429B-80D8-99A1CEFF3934}";
const CONTRACTID  = "@songbirdnest.com/mashTape/manager;1";

function mashTapeManager() {
	this.wrappedJSObject = this;
	this._listeners = [];
}

mashTapeManager.prototype.constructor = mashTapeManager;
mashTapeManager.prototype = {
	classDescription: DESCRIPTION,
	classID:          Components.ID(CID),
	contractID:       CONTRACTID,
	QueryInterface: XPCOMUtils.generateQI([Ci.sbIMashTapeManager]),

	addListener : function(aListener) {
		if (!(aListener instanceof Ci.sbIMashTapeListener))
			throw Components.results.NS_ERROR_INVALID_ARG;

		if (this._listeners.indexOf(aListener) == -1) {
			this._listeners.push(aListener);
		}
	},
	
	removeListener : function(aListener) {
		var idx = this._listeners.indexOf(aListener);
		if (idx > -1)
			this._listeners.splice(idx, 1);
	},

	updateInfo : function(section, data) {
		try {
			this._listeners.forEach( function (listener) {
				listener.onInfoUpdated(section, data);
			});
		} catch (e) {
			dump("Error calling listener.onInfoUpdated: " + e + "\n");
		}
	},

	updatePhotos : function(photos, photoCount) {
		try {
			this._listeners.forEach( function (listener) {
				listener.onPhotosUpdated(photos, photoCount);
			});
		} catch (e) {
			dump("Error calling listener.onPhotosUpdated: " + e + "\n");
		}
	}
}

var components = [mashTapeManager];
function NSGetModule(compMgr, fileSpec) {
	return XPCOMUtils.generateModule([mashTapeManager]);
}

