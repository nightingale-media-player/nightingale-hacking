/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */
 
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/DebugUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof Facebook == 'undefined') {
  var Facebook = {};
}

const FB_LOGIN_SUCCESS_URI = "http://www.facebook.com/connect/login_success.html";
const FB_GRAPH_API_BASE_URI = "https://graph.facebook.com";

const FB_OAUTH_PATH = "/oauth/authorize?";
const FB_OAUTH_CLIENT_ID = "client_id=" + Facebook.appId;
const FB_OAUTH_REDIR_URI = "redirect_uri=http://www.facebook.com/connect/login_success.html";
const FB_OAUTH_TYPE = "type=user_agent";
const FB_OAUTH_DISPLAY = "display=popup";
const FB_OAUTH_PERMS = "scope=" + Facebook.appPerms;

const FB_AUTHORIZE_URL = FB_GRAPH_API_BASE_URI + 
                         FB_OAUTH_PATH + 
                         FB_OAUTH_CLIENT_ID + "&" +
                         FB_OAUTH_REDIR_URI + "&" +
                         FB_OAUTH_TYPE + "&" +
                         FB_OAUTH_DISPLAY + "&" +
                         FB_OAUTH_PERMS;

var facebookAuthorize = {
  TRACE: DebugUtils.generateLogFunction("sbFacebookAuthorize", 5),
  LOG:   DebugUtils.generateLogFunction("sbFacebookAuthorize", 3),

  /*
   * Internal Data
   */  
  _browser: null,
  
  /*
   * Internal Methods
   */
  _init: function facebookAuthorize__init() {
    this.TRACE("_init");
    
    this._browser = document.getElementById("facebook_auth_browser");
    this._browser.addProgressListener(facebookAuthorize);
    
    // We assume that facebookAuthorize only gets loaded when the user isn't
    // logged in and/or has not provided the application with the necessary
    // permissions so we can go directly to attempting to login.
    this._browser.loadURI(FB_AUTHORIZE_URL);
  },
  
  _shutdown: function facebookAuthorize__shutdown() {
    this.TRACE("_shutdown");
    
    this._browser.removeProgressListener(this);
  },
  
  _closeSelf: function facebookAuthorize__closeSelf() {
    setTimeout(function() { window.close(); }, 0);
  },

  /*
   * nsIWebProgressListener 
   */
  onStateChange: function facebookAuthorize_onStateChange(aWebProgress,
                                                          aRequest,
                                                          aStateFlags,
                                                          aStatus)
  {
    return;
  },

  onProgressChange: function facebookAuthorize_onProgressChange(aWebProgress,
                                                                aRequest,
                                                                aCurSelfProgress,
                                                                aMaxSelfProgress,
                                                                aCurTotalProgress,
                                                                aMaxTotalProgress)
  {
    return;
  },

  onLocationChange: function facebookAuthorize_onLocationChange(aWebProgress,
                                                                aRequest,
                                                                aLocation)
  {
    var url = aLocation.QueryInterface(Ci.nsIURL);
    if(url.spec.indexOf(FB_LOGIN_SUCCESS_URI) == 0) {
      var token = url.ref.split("&")[0].split("=");
      if(token[0] == "access_token") {
        var appToken = Facebook.api._getAppTokenRemote();
        appToken.stringValue = decodeURIComponent(token[1]);
        
        this._closeSelf();
      }
      else {
        // XXXAus: Handle Errors.
      }
    }
    
    return;
  },

  onStatusChange: function facebookAuthorize_onStatusChange(aWebProgress,
                                                            aRequest,
                                                            aStatus,
                                                            aMessage)
  {
    return;
  },

  onSecurityChange: function facebookAuthorize_onSecurityChange(aWebProgress,
                                                                aRequest,
                                                                aState)
  {
    return;
  },

  /*
   * XPCOM QI
   */
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};
