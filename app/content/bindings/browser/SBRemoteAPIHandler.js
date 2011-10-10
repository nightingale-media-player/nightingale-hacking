// vim: set sw=2 :miv
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END NIGHTINGALE GPL
//

/**
 * SBRemoteAPIHandler
 *
 * Remote API handling for the tab browser
 *
 * This implements the onRemoteAPI method for <sb-tabbrowser>, for interaction
 * with remote API handling.  It is loaded via a subscript loader from the
 * constructor of <sb-tabbrowser>.
 */

const Cc = Components.classes;
const Ci = Components.interfaces;

if ("undefined" == typeof(SBString)) {
  Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
}

function onRemoteAPI(event) {
  // todo:
  //       hook up an action to do when the user clicks through, probably launch prepanel

  try {
    var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                .getService(Ci.nsIStringBundleService);
    var nightingaleStrings = sbs.createBundle("chrome://nightingale/locale/nightingale.properties");
    var brandingStrings = sbs.createBundle("chrome://branding/locale/brand.properties");
  } catch (e) {
    /* just abort if we can't find strings.  It's doubtful if we would ever
     * get here anyway, but if we do, do the safest thing possible even if it
     * sucks.
     */
    return; 
  }

  var notificationName = "remoteapi-called";
  var message = SBString("rapi.access.message." + event.categoryID,
                         "Web Page has accessed %S directly",
                         nightingaleStrings);
  var editOptionsLabel = SBString("rapi.access.button.label.options",
                                  "Edit Options...",
                                  nightingaleStrings);
  var editOptionsAccessKey = SBString("rapi.access.button.accessKey.options",
                                      "O",
                                      nightingaleStrings);
  var allowAlwaysLabel = SBString("rapi.access.button.label.allow.always",
                                  "Always Allow Site",
                                  nightingaleStrings);
  var iconURL = SBString("rapi.access.iconURL", "", brandingStrings);
  var appName = SBString("brandShortName", "Nightingale", nightingaleStrings);

  message = message.replace(/\%S/, appName);

  var closure = event;
  var doc = event.currentTarget.document;
  
  var allowAlwaysCallback = function(aHat, aButtonInfo) {
    var nsIPermissionManager = Components.interfaces.nsIPermissionManager;
    var permManager = Components.classes["@mozilla.org/permissionmanager;1"]
                        .getService(nsIPermissionManager);
    permManager.add(closure.siteScope, "rapi." + closure.categoryID, nsIPermissionManager.ALLOW_ACTION);
    
    var allowEvt = Components.classes["@getnightingale.com/remoteapi/security-event;1"]
                     .createInstance(Components.interfaces.sbIMutableRemoteSecurityEvent);

    allowEvt.initSecurityEvent( doc,
                                closure.siteScope,
                                closure.category,
                                closure.categoryID,
                                true);

    aHat.closeEvent = allowEvt;

    // add metrics reporting here
  }

  var editOptionsCallback = function(aHat, aButtonInfo) {
    var editEvt = Components.classes["@getnightingale.com/remoteapi/security-event;1"]
                .createInstance(Components.interfaces.sbIMutableRemoteSecurityEvent);

    // this causes an access denied event to be sent to the page
    editEvt.initSecurityEvent( doc,
                               closure.siteScope,
                               closure.category,
                               closure.categoryID,
                               false);

    aHat.closeEvent = editEvt;

    var prefWindow = SBOpenPreferences("paneRemoteAPI");
    if (prefWindow.gRemoteAPIPane) {
      /* if the window is already open ask it to configure the 
         whitelist */
      prefWindow.gRemoteAPIPane.configureWhitelist(closure.categoryID,
          closure.siteScope.spec);
    } else {
      /* if it's not yet open, ask it to open the whitelist when 
         it's ready */
      prefWindow.pleaseConfigureWhitelist = 
          [closure.categoryID, closure.siteScope.spec];
    }
    

    // add metrics reporting here
  }

  var buttons = [
                 {
                   label: editOptionsLabel,
                   accessKey: editOptionsAccessKey,
                   popup: null,
                   callback: editOptionsCallback
                 },
                 {
                   label: allowAlwaysLabel,
                   accessKey: null,
                   popup: null,
                   callback: allowAlwaysCallback
                 }
                 ];

  var tabbrowser = event.currentTarget.gBrowser;
  var browser = tabbrowser.getBrowserForDocument(event.target);
  var notificationBox = tabbrowser.getNotificationBox(browser);
  const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
  var notification = tabbrowser.showNotification(browser, notificationName, 
                                                 message, iconURL, priority,
                                                 buttons);
  
  var defaultEvt = Components.classes["@getnightingale.com/remoteapi/security-event;1"]
                     .createInstance(Components.interfaces.sbIMutableRemoteSecurityEvent);

  defaultEvt.initSecurityEvent( doc,
                                closure.siteScope,
                                closure.category,
                                closure.categoryID,
                                false);

  notification.closeEvent = defaultEvt;
}
