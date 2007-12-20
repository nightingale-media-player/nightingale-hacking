/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
 */

//
// Callback for XPInstall denied notification
//

(function(){
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  
  var bundleSvc = Cc["@mozilla.org/intl/stringbundle;1"]
                    .getService(Ci.nsIStringBundleService);
  var brandBundle = bundleSvc.createBundle("chrome://branding/locale/brand.properties");
  var browserBundle = bundleSvc.createBundle("chrome://browser/locale/browser.properties");
  
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefBranch);
  
  var xpinstallObserver = {
    observe: function(aSubject, aTopic, aData) {
      switch(aTopic) {
        case "xpinstall-install-blocked":
          // see browser.js gXPInstallObserver
          var installInfo = aSubject.QueryInterface(Ci.nsIXPIInstallInfo);
          var win = installInfo.originatingWindow;
          var shell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShell);
          var host = installInfo.originatingURI.host;
          var brandShortName = brandBundle.GetStringFromName("brandShortName");
          var notificationName, messageString, buttons;
          if (!prefSvc.getBoolPref("xpinstall.enabled")) {
            notificationName = "xpinstall-disabled"
            if (prefSvc.prefIsLocked("xpinstall.enabled")) {
              messageString = browserBundle.GetStringFromName("xpinstallDisabledMessageLocked");
              buttons = [];
            }
            else {
              messageString = browserBundle.formatStringFromName("xpinstallDisabledMessage",
                                                                 [brandShortName, host], 2);
  
              buttons = [{
                label: browserBundle.GetStringFromName("xpinstallDisabledButton"),
                accessKey: browserBundle.GetStringFromName("xpinstallDisabledButton.accesskey"),
                popup: null,
                callback: function editPrefs() {
                  prefSvc.setBoolPref("xpinstall.enabled", true);
                  return false;
                }
              }];
            }
          }
          else {
            notificationName = "xpinstall"
            messageString = browserBundle.formatStringFromName("xpinstallPromptWarning",
                                                               [brandShortName, host], 2);
  
            buttons = [{
              label: browserBundle.GetStringFromName("xpinstallPromptAllowButton"),
              accessKey: browserBundle.GetStringFromName("xpinstallPromptAllowButton.accesskey"),
              popup: null,
              callback: function() {
                var mgr = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                                    .createInstance(Components.interfaces.nsIXPInstallManager);
                mgr.initManagerWithInstallInfo(installInfo);
                return false;
              }
            }];
          }
  
          const iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
          // this is just a random notification box so we can get a priority
          var notificationBox = gBrowser.getNotificationBox(gBrowser.getBrowserAtIndex(0));
          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          gBrowser.showNotification(shell, notificationName, messageString,
                                    iconURL, priority, buttons);
          break;
      }
    },
    handleEvent: function() {
      os.removeObserver(xpinstallObserver, "xpinstall-install-blocked");
      removeEventListener("unload", xpinstallObserver, false);
    }
  }

  addEventListener("unload", xpinstallObserver, false);
  var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  os.addObserver(xpinstallObserver, "xpinstall-install-blocked", false);
})();
