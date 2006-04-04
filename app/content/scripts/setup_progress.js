var label;
var progressmeter;
var upd;
var n_ext;
var cur_ext;
var destFile;

function init()
{
  label = document.getElementById("setupprogress_label");
  progressmeter = document.getElementById("setupprogress_progress");
  upd = window.arguments[0].QueryInterface(Components.interfaces.sbIUpdate);
  upd.setNeedRestart(false);
  n_ext = upd.getNumExtensions();
  cur_ext = -1;
  setTimeout(installNextXPI, 0);
}

function installNextXPI()
{
  cur_ext++;
  if (cur_ext+1 > n_ext) { window.close(); return; }
  
  if (!upd.getExtensionInstallState(cur_ext)) {
    setTimeout(installNextXPI, 0);
    return;
  }

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var downloading = songbirdStrings.GetStringFromName("setupprogress.downloading");
  label.setAttribute("value", downloading + " " + upd.getExtensionName(cur_ext));
  progressmeter.setAttribute("value", 0);
  
  destFile = upd.downloadFile(upd.getExtensionURL(cur_ext), downloadListener);
}

var downloadListener = 
{
  onProgress: function(upd, percent)
  {
    progressmeter.setAttribute("value", percent);
  },
  
  onDownloadComplete: function(upd)
  {
    var r = upd.installXPI(destFile);
    if (r == 0) {
      var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
      var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
      var couldnotinstall = songbirdStrings.GetStringFromName("setupprogress.couldnotinstall");
      sbMessageBox("Error", couldnotinstall + " " + upd.getExtensionName(cur_ext), false); 
    } else upd.setNeedRestart(true);
    upd.deleteLastDownloadedFile();
    setTimeout(installNextXPI, 0);
  },

  onError: function(upd)
  {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var downloading = songbirdStrings.GetStringFromName("setupprogress.couldnotdownload");
    sbMessageBox("Error", couldnotdownload + upd.getExtensionName(cur_ext), false);
    setTimeout(installNextXPI, 0);
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIDownloadObserver) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  }
}

function handleKeyDown(event) 
{
  const VK_ENTER = 13;
  const VK_ESCAPE = 27;
  if (event.keyCode == VK_ESCAPE ||
      event.keyCode == VK_ENTER) {
    event.preventDefault();
  }
}
