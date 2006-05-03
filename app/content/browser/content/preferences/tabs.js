//@line 36 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/tabs.js"

var gTabsPane = {
  _lastForceLinksMode: 3,
  writeWindowLinksBehavior: function ()
  {
    var tabbedOpenForce = document.getElementById("tabbedOpenForce");
    if (!tabbedOpenForce.checked) 
      return 2;
    
    var tabbedWindowLinks = document.getElementById("tabbedWindowLinks");
    this._lastForceLinksMode = parseInt(tabbedWindowLinks.value)
    return this._lastForceLinksMode;
  },
  
  readForceLinks: function ()
  {
    var preference = document.getElementById("browser.link.open_newwindow");
    var tabbedWindowLinks = document.getElementById("tabbedWindowLinks");
    tabbedWindowLinks.disabled = preference.value == 2;
    return preference.value != 2;
  },
  
  readForceLinksMode: function ()
  {
    var preference = document.getElementById("browser.link.open_newwindow");
    return preference.value != 2 ? preference.value : this._lastForceLinksMode;
  },
  
  updateWindowLinksBehavior: function ()
  {
    var preference = document.getElementById("app.update.autoInstallEnabled");
    
    return undefined;
  }
};

