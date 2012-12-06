Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "Artist Profile";
const CID         = "{8d445ad0-3e27-11e2-a25f-0800200c9a66}";
const CONTRACTID  = "@songbirdnest.com/artistprofile;1";

function artistProfile() {
  this.wrappedJSObject = this;
  this.ioService = Cc["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);
  var app = Cc["@mozilla.org/fuel/application;1"]
  .getService(Ci.fuelIApplication);
  this.sbmeUrlPattern = app.prefs.getValue(
    "extensions.artistprofile.url", "http://www.songbird.me/#artist/{0}");
}

artistProfile.prototype.constructor = artistProfile;
artistProfile.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIArtistProfile]),

  hasArtist : function(aMediaItem) {
    return !!this._getArtistUrl(aMediaItem);
  },

  openArtistProfile : function(aMediaItem,aWindow) {
    var artistUrl = this._getArtistUrl(aMediaItem);
    if( !artistUrl ) {
      return;
    }
    
    var browser = null;    
    try {
    // Dig through the parent chain to find a happy gBrowser.
    for ( var win = aWindow;
      win != null && win != win.parent &&
      typeof win.gBrowser == 'undefined';
      win = win.parent ); // Yes, it is there on purpose.
      if( win ) {
        browser = win.gBrowser;
      }
    } catch(e) {}
      if( !browser ) {
        return;
      }

      var uri = this.ioService.newURI(artistUrl, null, null);
      var tab = browser.addTab(uri.spec);
      function selectNewForegroundTab(bws, tab) {
        bws.selectedTab = tab;
      }
      aWindow.setTimeout(selectNewForegroundTab, 0, browser, tab);
  },

  _getArtistUrl : function(aMediaItem) {
    var artist = aMediaItem.getProperty(SBProperties.artistName);
    if( !artist || (artist == "") ) {
      return null;
    }
    // empiric hack to improve hit ratio
    // convert to lowercase, remove all whitespaces
    artist = artist.toLowerCase().replace(/\s/gi,"");
    var artistUrl = this.sbmeUrlPattern.replace("{0}",encodeURIComponent(artist));
    return artistUrl;
    }
  }

var components = [artistProfile];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([artistProfile]);
}
