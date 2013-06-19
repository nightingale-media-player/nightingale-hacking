/*
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the nsURLFormatterService.
#
# The Initial Developer of the Original Code is
# Axel Hecht <axel@mozilla.com>
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Dietrich Ayala <dietrich@mozilla.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
*/

/**
 * Original code from Mozilla with Songbird modifications
 *   John Gaunt <redfive@songbirdnest.com>
 */

/**
 * @class sbURLFormatterService
 *
 * sbURLFormatterService exposes methods to substitute variables in URL formats.
 *
 * Mozilla Applications linking to Mozilla websites are strongly encouraged to use
 * URLs of the following format:
 *
 *   http[s]://%LOCALE%.%SERVICE%.mozilla.[com|org]/%LOCALE%/
 */

var Cr = Components.results;
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PREF_APP_DISTRIBUTION               = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION       = "distribution.version";
const PREF_APP_UPDATE_CHANNEL             = "app.update.channel";
const PREF_PARTNER_BRANCH                 = "app.partner.";


function sbURLFormatterService() {
}

sbURLFormatterService.prototype = {
  classDescription: "Songbird URL Formatter Service",
  contractID: "@songbirdnest.com/moz/sburlformatter;1",
  classID: Components.ID("{4dc7da1b-89d1-48b5-8582-64c98bfb3410}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIURLFormatter,
                                         Ci.nsIPropertyBag2]),

  _xpcom_categories: [ { category: "extension-update-params", entry: "LOCALE" },
                       { category: "extension-update-params", entry: "VENDOR" },
                       { category: "extension-update-params", entry: "NAME" },
                       { category: "extension-update-params", entry: "PRODUCT" },
                       { category: "extension-update-params", entry: "ID" },
                       { category: "extension-update-params", entry: "VERSION" },
                       { category: "extension-update-params", entry: "APPBUILDID" },
                       { category: "extension-update-params", entry: "BUILD_ID" },
                       { category: "extension-update-params", entry: "BUILD_TARGET" },
                       { category: "extension-update-params", entry: "PLATFORMVERSION" },
                       { category: "extension-update-params", entry: "PLATFORM_VERSION" },
                       { category: "extension-update-params", entry: "PLATFORMBUILDID" },
                       { category: "extension-update-params", entry: "APP" },
                       { category: "extension-update-params", entry: "OS" },
                       { category: "extension-update-params", entry: "XPCOMABI" },
                       { category: "extension-update-params", entry: "DISTRIBUTION" },
                       { category: "extension-update-params", entry: "DISTRIBUTION_VERSION" },
                       { category: "extension-update-params", entry: "CHANNEL" },
                       { category: "extension-update-params", entry: "OS_VERSION" },
                       { category: "extension-update-params", entry: "CUSTOM1" },
                       { category: "extension-update-params", entry: "CUSTOM2" } ],

  _defaults: {
    get appInfo() {
      if (!this._appInfo)
        this._appInfo = Cc["@mozilla.org/xre/runtime;1"].
                        getService(Ci.nsIXULAppInfo).
                        QueryInterface(Ci.nsIXULRuntime);
      return this._appInfo;
    },

    LOCALE: function() Cc["@mozilla.org/chrome/chrome-registry;1"].
                       getService(Ci.nsIXULChromeRegistry).getSelectedLocale('global'),
    VENDOR:           function() this.appInfo.vendor,
    NAME:             function() this.appInfo.name,
    PRODUCT:          function() this.appInfo.name,
    ID:               function() this.appInfo.ID,
    VERSION:          function() this.appInfo.version,
    APPBUILDID:       function() this.appInfo.appBuildID,
    BUILD_ID:         function() this.appInfo.appBuildID,
    BUILD_TARGET:     function() this.appInfo.OS + "_" + this.getABI(),
    PLATFORMVERSION:  function() this.appInfo.platformVersion,
    PLATFORM_VERSION: function() this.appInfo.platformVersion,
    PLATFORMBUILDID:  function() this.appInfo.platformBuildID,
    APP:              function() this.appInfo.name.toLowerCase().replace(/ /, ""),
    OS:               function() this.appInfo.OS,
    XPCOMABI:         function() this.appInfo.XPCOMABI,
    DISTRIBUTION:         function() this.getPrefValue(PREF_APP_DISTRIBUTION),
    DISTRIBUTION_VERSION: function() this.getPrefValue(PREF_APP_DISTRIBUTION_VERSION),
    CHANNEL:          function() this.getUpdateChannel(),
    OS_VERSION:       function() this.getOSVersion(),
    CUSTOM1:          function() "custom_parameter_1",
    CUSTOM2:          function() "custom_parameter_2",


    getABI: function () {
      var abi = "default";
      try {
        abi = this.appInfo.XPCOMABI;

        // Mac universal build should report a different ABI than either macppc
        // or mactel.
        var macutils = null;
        if ("@mozilla.org/xpcom/mac-utils;1" in Cc) {
          macutils = Cc["@mozilla.org/xpcom/mac-utils;1"]
                       .getService(Ci.nsIMacUtils);
        }
      
        if (macutils && macutils.isUniversalBinary)
          abi = "Universal-gcc3";
      }
      catch (e) { }

      return abi;
    },

    getPrefValue: function (aPrefName) {
      var prefSvc =  Cc['@mozilla.org/preferences-service;1'].getService(Ci.nsIPrefBranch2);
      var prefValue = "default";
      var defaults = prefSvc.QueryInterface(Ci.nsIPrefService)
                                      .getDefaultBranch(null);
      try {
        prefValue = defaults.getCharPref(aPrefName);
      } catch (e) {
        // use default when pref not found
      }
      return prefValue;
    },

    /**
     * Read the update channel from defaults only.  We do this to ensure that
     * the channel is tightly coupled with the application and does not apply
     * to other instances of the application that may use the same profile.
     */
    getUpdateChannel: function uf_getUpdateChannel() {
      var prefSvc =  Cc['@mozilla.org/preferences-service;1'].getService(Ci.nsIPrefBranch2);
      var channel = "default";
      var prefName;
      var prefValue;

      var defaults = prefSvc.QueryInterface(Ci.nsIPrefService)
                                      .getDefaultBranch(null);
      try {
        channel = defaults.getCharPref(PREF_APP_UPDATE_CHANNEL);
      } catch (e) {
        // use default when pref not found
      }

      try {
        var partners = prefSvc.getChildList(PREF_PARTNER_BRANCH, { });
        if (partners.length) {
          channel += "-cck";
          partners.sort();
    
          for each (prefName in partners) {
            prefValue = prefSvc.getCharPref(prefName);
            channel += "-" + prefValue;
          }
        }
      }
      catch (e) {
        Cu.reportError(e);
      }

      return channel;
    },

    getOSVersion: function uf_getOSVersion() {
      var osVersion = "default";
      var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                              .getService(Components.interfaces.nsIPropertyBag2);
      try {
        osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");
      }
      catch (e) {
      }

      if (osVersion) {
        try {
          osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
        }
        catch (e) {
          // Not all platforms have a secondary widget library, so an error is nothing to worry about.
        }
      }
      return encodeURIComponent(osVersion);
    }
  },

  // nsIPropertyBag
  get enumerator() { return null; },
  getProperty: function (aPropName) { return null; },

  // nsIPropertyBag2
  getPropertyAsInt32 : function(aPropName) { return null; },
  getPropertyAsUint32 : function(aPropName) { return null; },
  getPropertyAsInt64 : function(aPropName) { return null; },
  getPropertyAsUint64 : function(aPropName) { return null; },
  getPropertyAsDouble : function(aPropName) { return null; },
  getPropertyAsACString : function(aPropName) { return null; },
  getPropertyAsAUTF8String : function(aPropName) { return null; },
  getPropertyAsBool : function(aPropName) { return null; },
  getPropertyAsInterface : function(aPropName, aIID, aRetVal) { aRetVal = null; return; },
  get : function(aPropName) { return null; },
  hasKey : function(aPropName) { return (aPropName in this._defaults); },

  getPropertyAsAString: function(aPropName) {
    if (aPropName in this._defaults) // supported defaults
      return this._defaults[aPropName]();
    return "";
  },

  formatURL: function uf_formatURL(aFormat, aMappings) {
    var _this = this;
    var _overrides = aMappings;

    var replacementCallback = function(aMatch, aKey) {
      try {
        if ( _overrides ) {
          return _overrides.getProperty(aKey);
        }
      } catch (e) {
        // excpetion meaans it's not in the override
      }

      if (aKey in _this._defaults) // supported defaults
        return _this._defaults[aKey]();

      Cu.reportError("formatURL: Couldn't find value for key: " + aKey);
      return aMatch;
    }

    // Ensure 3 characters to prevent matching things like %D0%CB
    return aFormat.replace(/%(\w{3,})%/g, replacementCallback);
  },

  formatURLPref: function uf_formatURLPref(aPref, aMappings) {
    var format = null;
    var PS = Cc['@mozilla.org/preferences-service;1']
             .getService(Ci.nsIPrefBranch);

    try {
      format = PS.getComplexValue(aPref, Ci.nsISupportsString).data;
    } catch(ex) {
      Cu.reportError("formatURLPref: Couldn't get pref: " + aPref);
      return "about:blank";
    }

    if (!PS.prefHasUserValue(aPref) &&
        /^chrome:\/\/.+\/locale\/.+\.properties$/.test(format)) {
      // This looks as if it might be a localised preference
      try {
        format = PS.getComplexValue(aPref, Ci.nsIPrefLocalizedString).data;
      } catch(ex) {}
    }

    return this.formatURL(format, aMappings);
  }
};

var NSGetModule = XPCOMUtils.generateNSGetFactory([sbURLFormatterService]);
