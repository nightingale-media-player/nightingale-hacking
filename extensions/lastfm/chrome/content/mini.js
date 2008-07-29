// overlay for the miniplayer - don't present a UI, but do log in

// Make a namespace.
if (typeof Lastfm == 'undefined') {
  var Lastfm = {};
}

// Called when the window finishes loading
Lastfm.onLoad = function() {
  // the window has finished loading

  // get the XPCOM service as a JS object
  this._service = Components.classes['@songbirdnest.com/lastfm;1']
    .getService().wrappedJSObject

  // if we have a username & password, try to log in
  if (this._service.username && this._service.password) {
    this._service.login();
  }
}

window.addEventListener("load", function(e) { Lastfm.onLoad(e); }, false);

