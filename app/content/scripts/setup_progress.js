var label;
var progressmeter;
var bundle;
var n_ext;
var cur_ext;
var destFile;
var afailure = false;

function init()
{
  label = document.getElementById("setupprogress_label");
  progressmeter = document.getElementById("setupprogress_progress");
  bundle = window.arguments[0].QueryInterface(Components.interfaces.sbIBundle);
  bundle.setNeedRestart(false);
  n_ext = bundle.getNumExtensions();
  cur_ext = -1;
  setTimeout(installNextXPI, 0);
}

function installNextXPI()
{
  cur_ext++;
  if (cur_ext+1 > n_ext) { bundle.setInstallResult(afailure ? "failure" : "success"); window.close(); return; }
  
  if (!bundle.getExtensionInstallState(cur_ext)) {
    setTimeout(installNextXPI, 0);
    return;
  }

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var downloading = "Downloading";
  try {
    downloading = songbirdStrings.GetStringFromName("setupprogress.downloading");
  } catch (e) {}
  label.setAttribute("value", downloading + " " + bundle.getExtensionName(cur_ext));
  progressmeter.setAttribute("value", 0);
  
  destFile = bundle.downloadFile(bundle.getExtensionURL(cur_ext), downloadListener);
}

var downloadListener = 
{
  onProgress: function(bundle, percent)
  {
    progressmeter.setAttribute("value", percent);
  },
  
  onDownloadComplete: function(bundle)
  {
    var r = bundle.installXPI(destFile);
    if (r == 0) {
      var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
      var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
      var couldnotinstall = "Could not install";
      try {
        couldnotinstall = songbirdStrings.GetStringFromName("setupprogress.couldnotinstall");
      } catch (e) {}
      sbMessageBox("Error", couldnotinstall + " " + bundle.getExtensionName(cur_ext), false); 
    } else bundle.setNeedRestart(true);
    bundle.deleteLastDownloadedFile();
    setTimeout(installNextXPI, 0);
  },

  onError: function(bundle)
  {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var couldnotdownload = "Downloading";
    try {
      couldnotdownload = songbirdStrings.GetStringFromName("setupprogress.couldnotdownload");
    } catch (e) {}
    sbMessageBox("Error", couldnotdownload + " " + bundle.getExtensionName(cur_ext), false);
    afailure = true;
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
