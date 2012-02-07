// vim: fileencoding=utf-8 shiftwidth=2

var systray = {
  onHideButtonClick:
    function systray_onHideButtonClick() {
      var trayIcon = document.getElementById("trayicon-mainwin");
      trayIcon.minimizeWindow();
    },

  restore:
    function systray_restore() {
      var trayIcon = document.getElementById("trayicon-mainwin");
      trayIcon.restoreWindow();
      window.focus();
    },

  observe:
    function systray_observe(aSubject, aTopic, aData) {
      if (aSubject instanceof Components.interfaces.nsIPrefBranch) {
        return systray.observePref(aSubject, aTopic, aData);
      }
      return systray.observeRemote(aSubject, aTopic, aData);
    },

  observePref:
    function systray_observePref(aSubject, aTopic, aData) {
      switch(aData) {
        case "sb.notify":
          this._doAlertNotification =
            (aSubject.PREF_BOOL == aSubject.getPrefType(aData) &&
             aSubject.getBoolPref(aData));
          break;
      }
    },

  _doAlertNotification: false,
  _dataremote_timer: null,
  observeRemote:
    function systray_observeRemote(aSubject, aTopic, aData) {
      // songbird likes to send *lots* of notifications at once
      // so we do a workaround by delaying the alert by half a second, and abort
      // if a new one shows up within that time
      function showAlert(aSystray) {
        const alsertsSvc =
          Components.classes["@mozilla.org/alerts-service;1"]
                    .getService(Components.interfaces.nsIAlertsService);
        var title = SBDataGetStringValue("metadata.title");
        var artist = SBDataGetStringValue("metadata.artist");
        var album = SBDataGetStringValue("metadata.album");
        var position = SBDataGetStringValue("metadata.position.str");
        var length = SBDataGetStringValue("metadata.length.str");
        if (title + artist + album + position + length == "") {
          // bogus alert, nothing to show
          return;
        }
        trayIcon.title = title + '\n' + artist + '\n' + album + '\n' + position + '/' + length;
        if (aSystray._doAlertNotification) {
          alsertsSvc.showAlertNotification("chrome://branding/content/nightingale-64.png",
                                          title, artist + " : " + album + " - " + position + '/' + length,
                                          false, "", null);
        }
      }
      clearTimeout(systray._dataremote_timer);
      systray._dataremote_timer = setTimeout(showAlert, 500, this);
      var trayIcon = document.getElementById("trayicon-mainwin");
      trayIcon.title = SBDataGetStringValue("metadata.title") +
                        '\n' +
                        SBDataGetStringValue("metadata.artist") +
                        '\n' +
                        SBDataGetStringValue("metadata.album") + 
                        '\n' +
                        SBDataGetStringValue("metadata.position.str") +
                        '/' +
                        SBDataGetStringValue("metadata.length.str");
    },

  _urlRemote: null,
  _titleRemote: null,
  _artistRemote: null,
  _positionRemote: null,
  _lengthRemote: null,

  attachRemotes:
    function systray_attachRemotes() {
      // get notified about track changes
      // (watch url primarily, but also title for streams)
      systray._urlRemote = SB_NewDataRemote("metadata.url", null);
      systray._urlRemote.bindObserver(systray, true);
      systray._titleRemote = SB_NewDataRemote("metadata.title", null);
      systray._titleRemote.bindObserver(systray, true);
      systray._artistRemote = SB_NewDataRemote("metadata.artist", null);
      systray._artistRemote.bindObserver(systray, true);
      systray._positionRemote = SB_NewDataRemote("metadata.position.str", null);
      systray._positionRemote.bindObserver(systray, true);
      systray._lengthRemote = SB_NewDataRemote("metadata.length.str", null);
      systray._lengthRemote.bindObserver(systray, true);
  },

  detachRemotes:
    function systray_detachRemotes() {
      removeEventListener("unload", arguments.callee, false);
      systray._urlRemote.unbind();
      systray._titleRemote.unbind();
      systray._artistRemote.unbind();
      systray._positionRemote.unbind();
      systray._lengthRemote.unbind();
  },

  destroy:
    function systray_destroy() {
      systray._prefBranch.removeObserver("sb.", systray);
      systray._prefBranch = null;
      systray.detachRemotes();
    }
};

(function() {
  if ("undefined" != typeof(onHideButtonClick)) {
    // WTF, there already is a hidden button handler.
    // we're probably overlaying the wrong window or something
    Components.utils.reportError("systray: already have " + onHideButtonClick);
    return;
  }
  // yes, it's silly how much work it is to get the "hide" button
  const NS_XUL =
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  var titlebar = document.getElementsByTagName("sb-sys-titlebar")[0];
  if (!titlebar) {
    Components.utils.reportError("systray: cannot find the title bar");
    return;
  }
  var sysButtons = document.getAnonymousNodes(titlebar)[0]
                           .getElementsByTagNameNS(NS_XUL,"sb-sys-buttons")[0];
  if (!sysButtons) {
    Components.utils.reportError("systray: cannot find the system buttons");
    return;
  }
  var hideButton;
  hideButton = document.getAnonymousElementByAttribute(sysButtons, "sbid", "hide");
  if (!hideButton) {
    Components.utils.reportError("systray: cannot find hide button");
    return;
  }

  window.onHideButtonClick = systray.onHideButtonClick;
  hideButton.setAttribute("hidden", "false");

  addEventListener("unload", systray.destroy, false);

  systray._prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                                  .getService(Components.interfaces.nsIPrefService)
                                  .getBranch("extensions.minimizetotray.")
                                  .QueryInterface(Components.interfaces.nsIPrefBranch2);
  systray._prefBranch.addObserver("sb.", systray, false);

  systray.observePref(systray._prefBranch, "nsPref:changed", "sb.notify");

  systray.attachRemotes();
})();
