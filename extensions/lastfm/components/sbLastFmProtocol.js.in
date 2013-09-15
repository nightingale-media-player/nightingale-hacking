/*
Copyright (c) 2008, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of Pioneers of the Inevitable, Songbird, nor the names
    of its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// these constants make everything better
const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// import the XPCOM helper
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

function sbLastFmProtocol() {
}

sbLastFmProtocol.prototype = {
  // XPCOM magics
  classDescription: 'Last.fm protocol handler',
  contractID: '@mozilla.org/network/protocol;1?name=lastfm',
  classID: Components.ID('2c2f00b0-833e-4006-a622-d31010f50fa6'),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler]),

  // nsIProtocolHandler attributes
  scheme: 'lastfm',
  defaultPort: -1,
  protocolFlags: Ci.nsIProtocolHandler.URI_LOADABLE_BY_ANYONE |
    Ci.nsIProtocolHandler.URI_NORELATIVE | 
    Ci.nsIProtocolHandler.URI_NON_PERSISTABLE | 
    Ci.nsIProtocolHandler.URI_DOES_NOT_RETURN_DATA,
};

sbLastFmProtocol.prototype.newURI = 
function sbLastFmProtocol_newURI(aSpec, aOriginCharset, aBaseURI) {
  var uri = Components.classes['@mozilla.org/network/simple-uri;1']
    .createInstance(Ci.nsIURI);
  uri.spec = aSpec;
  return uri;
}

sbLastFmProtocol.prototype.newChannel = 
function sbLastFmProtocol_newChannel(aURI) {
  /* ask the sbLastFm service to start playback */
  var svc = Cc['@songbirdnest.com/lastfm;1'].getService(Ci.sbILastFmRadio);
  if (svc.radioPlay(aURI.spec)) {
    /* create dummy nsIChannel instance */
    var ios = Components.classes['@mozilla.org/network/io-service;1']
      .getService(Ci.nsIIOService);
    return ios.newChannel('javascript:void', null, null);
  } else {
    var path = aURI.path.replace(/^\/\//, "");
    dump("failed to play: " + path + "\n");
    /*  failed for some reason, spin over to the web equivalent */
    var ios = Components.classes['@mozilla.org/network/io-service;1']
      .getService(Ci.nsIIOService);
    return ios.newChannel('http://last.fm/listen/' + path, null, null);
  }
}

sbLastFmProtocol.prototype.allowPort = 
function sbLastFmProtocol_allowPort(port, scheme) {
  return false;
}

// nsIProtocolHandler


// XPCOM glue stuff
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFmProtocol]);
}
