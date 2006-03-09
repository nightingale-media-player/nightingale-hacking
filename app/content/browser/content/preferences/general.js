//@line 36 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/general.js"

var gGeneralPane = {
  _pane: null,

  setHomePageToCurrentPage: function ()
  {
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
    var win = wm.getMostRecentWindow("navigator:browser");
    if (win) {
      var homePageField = document.getElementById("browserStartupHomepage");
      var newVal = "";

      var tabbrowser = win.document.getElementById("content");
      var l = tabbrowser.browsers.length;
      for (var i = 0; i < l; i++) {
        if (i)
          newVal += "|";
        newVal += tabbrowser.getBrowserAtIndex(i).webNavigation.currentURI.spec;
      }
      
      homePageField.value = newVal;
      this._pane.userChangedValue(homePageField);
    }
  },
  
  setHomePageToBookmark: function ()
  {
    var rv = { url: null };
    document.documentElement.openSubDialog("chrome://browser/content/bookmarks/selectBookmark.xul",
                                           "resizable", rv);  
    if (rv.url) {
      var homePageField = document.getElementById("browserStartupHomepage");
      homePageField.value = rv.url;
      this._pane.userChangedValue(homePageField);
    }
  },
  
  setHomePageToDefaultPage: function ()
  {
    var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefService);
    var pref = prefService.getDefaultBranch(null);
    var url = pref.getComplexValue("browser.startup.homepage",
                                   Components.interfaces.nsIPrefLocalizedString).data;
    var homePageField = document.getElementById("browserStartupHomepage");
    homePageField.value = url;
    
    this._pane.userChangedValue(homePageField);
  },
  
  setHomePageToBlankPage: function ()
  {
    var homePageField = document.getElementById("browserStartupHomepage");
    homePageField.value = "about:blank";
    
    this._pane.userChangedValue(homePageField);
  },
  
  // Update the Home Button tooltip on all open browser windows.
  homepageChanged: function (aEvent)
  {
    var homepage = aEvent.target.value;
    // Replace pipes with commas to look nicer.
    homepage = homepage.replace(/\|/g,', ');
    
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
    var e = wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      var win = e.getNext();
      if (!(win instanceof Components.interfaces.nsIDOMWindow))
        break;
      var homeButton = win.document.getElementById("home-button");
      if (homeButton)
        homeButton.setAttribute("tooltiptext", homepage);
    }
  },
  
  init: function ()
  {
    this._pane = document.getElementById("paneGeneral");
    
    var useButton = document.getElementById("browserUseCurrent");
    
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    var win = wm.getMostRecentWindow("navigator:browser");
    if (win) {
      var tabbrowser = win.document.getElementById("content");  
      if (tabbrowser.browsers.length > 1)
        useButton.label = useButton.getAttribute("label2");
    }
    else {
      // prefwindow wasn't opened from a browser window, so no current page
      useButton.disabled = true;
    }
  },
  
  showConnections: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/connection.xul",
                                           "", null);
  },

//@line 142 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/general.js"
  checkNow: function ()
  {
    var shellSvc = Components.classes["@mozilla.org/browser/shell-service;1"]
                             .getService(Components.interfaces.nsIShellService);
    var brandBundle = document.getElementById("bundleBrand");
    var shellBundle = document.getElementById("bundleShell");
    var brandShortName = brandBundle.getString("brandShortName");
    var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
    var promptMessage;
    const IPS = Components.interfaces.nsIPromptService;
    var psvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                         .getService(IPS);
    if (!shellSvc.isDefaultBrowser(false)) {
      promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage", 
                                                     [brandShortName]);
      var rv = psvc.confirmEx(window, promptTitle, promptMessage, 
                              (IPS.BUTTON_TITLE_YES * IPS.BUTTON_POS_0) + 
                              (IPS.BUTTON_TITLE_NO * IPS.BUTTON_POS_1),
                              null, null, null, null, { });
      if (rv == 0)
        shellSvc.setDefaultBrowser(true, false);
    }
    else {
      promptMessage = shellBundle.getFormattedString("alreadyDefaultBrowser",
                                                     [brandShortName]);
      psvc.alert(window, promptTitle, promptMessage);
    }
  }
//@line 171 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/general.js"
};

