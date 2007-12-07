/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
//  Popup Blocker Hat and PageReport Status Panel
//

try 
{

  // Module specific global for auto-init/deinit support
  var popupBlocker = {};
  popupBlocker.init_once = 0;
  popupBlocker.deinit_once = 0;
  popupBlocker.onLoad = function()
  {
    if (popupBlocker.init_once++) { 
      dump("WARNING: popupBlocker double init!!\n"); 
      return; 
    }
    gPopupBlockerObserver.init();
  }
  popupBlocker.onUnload = function()
  {
    if (popupBlocker.deinit_once++) { 
      dump("WARNING: popupBlocker double deinit!!\n"); 
      return; 
    }
    window.removeEventListener("load", popupBlocker.onLoad, false);
    window.removeEventListener("unload", popupBlocker.onUnload, false);
    gPopupBlockerObserver.shutdown();
  }

  // Auto-init/deinit registration
  window.addEventListener("load", popupBlocker.onLoad, false);
  window.addEventListener("unload", popupBlocker.onUnload, false);

  var gPopupBlockerObserver = {
    _reportButton: null,
    _kIPM: Components.interfaces.nsIPermissionManager,
    _prefService : null,
    
    _blockedPopupAllowSite: null,
    _blockedPopupEditSettings: null,
    _blockedPopupDontShowMessage: null,
    _blockedPopupSeparator: null,
    _blockedPopupMenu: null,
    
    _sbs: null,
    _brandingStrings: null,
    _browserStrings: null,
    _prefStrings: null,
    
    init: function() {
      this._sbs = Components.classes["@mozilla.org/intl/stringbundle;1"]
                  .getService(Components.interfaces.nsIStringBundleService);
      this._brandingStrings = this._sbs.createBundle(
                              "chrome://branding/locale/brand.properties");
      this._browserStrings = this._sbs.createBundle(
                             "chrome://browser/locale/browser.properties");
      this._prefStrings = this._sbs.createBundle(
                             "chrome://browser/locale/preferences/preferences.properties");

      this._prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefBranch);
      gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);
    },
    
    shutdown: function() {
      gBrowser.removeEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);
    },
    
    onUpdatePageReport: function() {
      if (!gPopupBlockerObserver._reportButton) {
        gPopupBlockerObserver._reportButton = document.getElementById("page-report-button");
      }
      if (gBrowser.selectedBrowser.pageReport) {
        gPopupBlockerObserver._reportButton.setAttribute("blocked", "true");
        gPopupBlockerObserver.showNotification();
      } else {
        gPopupBlockerObserver._reportButton.removeAttribute("blocked");
      }
    },
    
    getElements: function(menu) {
      this._blockedPopupAllowSite = menu.getElementsByAttribute("sbid", "blockedPopupAllowSite")[0];
      this._blockedPopupEditSettings = menu.getElementsByAttribute("sbid", "blockedPopupEditSettings")[0];
      this._blockedPopupDontShowMessage = menu.getElementsByAttribute("sbid", "blockedPopupDontShowMessage")[0];
      this._blockedPopupSeparator = menu.getElementsByAttribute("sbid", "blockedPopupSeparator")[0];
    },
    
    showNotification: function() {
      if (this._prefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var pageReport = gBrowser.selectedBrowser.pageReport;
        var popupCount = pageReport.length;

        var brandShortName = "Songbird";
        try {
          brandShortName = this._brandingStrings.GetStringFromName("brandShortName");
        } catch (e) {}
        
        var message;
        if (popupCount > 1)
          message = "%S prevented this site from opening %S popup windows.";
        else
          message = "%S prevented this site from opening a popup window.";
        try {
          if (popupCount > 1)
            message = this._browserStrings.GetStringFromName("popupWarningMultiple");
          else
            message = this._browserStrings.GetStringFromName("popupWarning");
        } catch(e) { }
        message = message.replace("%S", brandShortName);
        if (popupCount > 1) message = message.replace("%S", popupCount);

        var notificationName = "popup-blocked";
        var iconURL = "chrome://global/skin/icons/pagereport.png";
        var optionsLabel, optionsAccessKey;
        optionsLabel = "Options";
        optionsAccessKey = "O";
        try { 
          optionsLabel = this._browserStrings.getString("popupWarningButton");
        } catch (e) {}
        try { 
          optionsAccessKey = 
            this._browserStrings.getString("popupWarningButton.accesskey"); 
        } catch (e) {}
        
        var button = [
                      {
                        label: optionsLabel,
                        accessKey: optionsAccessKey,
                        popup: "blockedPopupOptions",
                        callback: null
                      }
                      ];

        var browser = gBrowser.selectedBrowser;
        var notificationBox = browser.parentNode;
        const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
        gBrowser.showNotification(browser, 
                                  "popup-blocked",
                                  message,
                                  iconURL,
                                  priority,
                                  button);
      }
    },
    
    fillPopupList: function(aEvent) {
      this.getElements(aEvent.target);
    
      var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
      try {
        this._blockedPopupAllowSite.hidden = false;
        var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                          .getService(this._kIPM);
        if (pm.testPermission(uri, "popup") == this._kIPM.ALLOW_ACTION) {
          // Offer an item to block popups for this site, if a whitelist entry exists
          // already for it.
          var blockString = this._browserStrings.GetStringFromName("popupBlock").replace("%S", uri.host);
          this._blockedPopupAllowSite.setAttribute("label", blockString);
          this._blockedPopupAllowSite.setAttribute("block", "true");
        }
        else {
          // Offer an item to allow popups for this site
          var allowString = this._browserStrings.GetStringFromName("popupAllow").replace("%S", uri.host);
          this._blockedPopupAllowSite.setAttribute("label", allowString);
          this._blockedPopupAllowSite.removeAttribute("block");
        }
      }
      catch (e) {
        this._blockedPopupAllowSite.hidden = true;
      }
    
      while (aEvent.target.lastChild != this._blockedPopupSeparator)
        aEvent.target.removeChild(aEvent.target.lastChild);

      var pageReport = gBrowser.selectedBrowser.pageReport;
      if (pageReport && pageReport.length > 0) {
        this._blockedPopupSeparator.hidden = false;
        for (var i = 0; i < pageReport.length; ++i) {
          var popupURIspec = pageReport[i].popupWindowURI.spec;
          if (popupURIspec == "" || popupURIspec == "about:blank" ||
              popupURIspec == uri.spec)
            continue;
    
          var menuitem = document.createElement("menuitem");
          menuitem.setAttribute("sbtype", "popupBlockerItem");
          var label = this._browserStrings.GetStringFromName("popupShowPopupPrefix")
                                    .replace("%S", popupURIspec);
          menuitem.setAttribute("label", label);
          menuitem.requestingWindow = pageReport[i].requestingWindow;
          menuitem.requestingDocument = pageReport[i].requestingDocument;
          menuitem.setAttribute("popupWindowURI", popupURIspec);
          menuitem.setAttribute("popupWindowFeatures", pageReport[i].popupWindowFeatures);
          menuitem.setAttribute("oncommand", "gPopupBlockerObserver.showBlockedPopup(event);");
          aEvent.target.appendChild(menuitem);
        }
      }
      else {
        this._blockedPopupSeparator.hidden = true;
      }
    
      var showMessage = true;
      try {
        showMessage = this._prefService.getBoolPref("privacy.popups.showBrowserMessage");
      } catch (e) { }
      
      this._blockedPopupDontShowMessage.setAttribute("checked", !showMessage);
      if (aEvent.target.localName == "popup") {
        this._blockedPopupDontShowMessage.setAttribute("label", 
          this._browserStrings.GetStringFromName("popupWarningDontShowFromMessage"));
      } else {
        this._blockedPopupDontShowMessage.setAttribute("label", 
          this._browserStrings.GetStringFromName("popupWarningDontShowFromStatusbar"));
      }
    },
    
    showBlockedPopup: function(event) {
      var target = event.target;
      var popupWindowURI = target.getAttribute("popupWindowURI");
      var features = target.getAttribute("popupWindowFeatures");
      var name = target.getAttribute("popupWindowName");
        
      var dwi = target.requestingWindow;
        
      // If we have a requesting window and the requesting document is
      // still the current document, open the popup.
      if (dwi && dwi.document == target.requestingDocument) {
        dwi.open(popupWindowURI, name, features);
      }
    },
    
    editPopupSettings: function () {
      var host = "";
      try {
        var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
        host = uri.host;
      }
      catch (e) { }
    
      var params = { blockVisible   : false,
                     sessionVisible : false,
                     allowVisible   : true,
                     prefilledHost  : host,
                     permissionType : "popup",
                     windowTitle    : this._prefStrings.GetStringFromName("popuppermissionstitle"),
                     introText      : this._prefStrings.GetStringFromName("popuppermissionstext") };
      var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Components.interfaces.nsIWindowMediator);
      var existingWindow = wm.getMostRecentWindow("Browser:Permissions");
      if (existingWindow) {
        existingWindow.initWithParams(params);
        existingWindow.focus();
      }
      else
        window.openDialog("chrome://browser/content/preferences/permissions.xul",
                          "_blank", "resizable,dialog=no,centerscreen", params);
    },
    
    dontShowMessage: function () {
      var showMessage = true;
      try { 
        showMessage = this._prefService.
                          getBoolPref("privacy.popups.showBrowserMessage"); 
      } catch (e) {}
      var firstTime = false;
      try { 
        firstTime = this._prefService.
                        getBoolPref("privacy.popups.firstTime"); 
      } catch (e) {}
    
      // If the info message is showing at the top of the window, and the user has never
      // hidden the message before, show an info box telling the user where the info
      // will be displayed.
      if (showMessage && firstTime) {
        window.openDialog("chrome://browser/content/pageReportFirstTime.xul", "_blank",
                "dependent");
      }
    
      this._prefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    
      var browser = gBrowser.selectedBrowser;
      var notificationBox = browser.parentNode;
      var notification = notificationBox.getNotificationWithValue("popup-blocked");
      if (notification)
        notification.close();
    },
    
    toggleAllowPopupsForSite: function(event) {
      var currentURI = gBrowser.selectedBrowser.webNavigation.currentURI;
      var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                          .getService(this._kIPM);
      var shouldBlock = event.target.getAttribute("block") == "true";
      var perm = shouldBlock ? this._kIPM.DENY_ACTION : this._kIPM.ALLOW_ACTION;
      pm.add(currentURI, "popup", perm);

      var browser = gBrowser.selectedBrowser;
      var notificationBox = browser.parentNode;
      var notification = notificationBox.getNotificationWithValue("popup-blocked");
      if (notification)
        notification.close();
    }
  }
}
catch (err)
{
  dump("popupBlocker.js - " + err);
}


