//@line 37 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/downloads.js"

var gDownloadsPane = {
  chooseFolder: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    var bundlePreferences = document.getElementById("bundlePreferences");
    var title = bundlePreferences.getString("chooseDownloadFolderTitle");
    fp.init(window, title, nsIFilePicker.modeGetFolder);
    
    const nsILocalFile = Components.interfaces.nsILocalFile;
    var customDirPref = document.getElementById("browser.download.dir");
    if (customDirPref.value)
      fp.displayDirectory = customDirPref.value;
    fp.appendFilters(nsIFilePicker.filterAll);
    if (fp.show() == nsIFilePicker.returnOK) {
      var file = fp.file.QueryInterface(nsILocalFile);
      var currentDirPref = document.getElementById("browser.download.downloadDir");
      customDirPref.value = currentDirPref.value = file;
      var folderListPref = document.getElementById("browser.download.folderList");
      folderListPref.value = this._fileToIndex(file);
    }
  },
  
  onReadUseDownloadDir: function ()
  {
    var downloadFolder = document.getElementById("downloadFolder");
    var chooseFolder = document.getElementById("chooseFolder");
    var preference = document.getElementById("browser.download.useDownloadDir");
    downloadFolder.disabled = !preference.value;
    chooseFolder.disabled = !preference.value;      
    return undefined;
  },
  
  _fileToIndex: function (aFile)
  { 
    if (!aFile || aFile.equals(this._getDownloadsFolder("Desktop")))
      return 0;
    else if (aFile.equals(this._getDownloadsFolder("Downloads")))
      return 1;
    return 2;
  },
  
  _indexToFile: function (aIndex)
  {
    switch (aIndex) {
    case 0: 
      return this._getDownloadsFolder("Desktop");
    case 1:
      return this._getDownloadsFolder("Downloads");
    }
    var customDirPref = document.getElementById("browser.download.dir");
    return customDirPref.value;
  },
  
  _getSpecialFolderKey: function (aFolderType)
  {
//@line 96 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/downloads.js"
    return aFolderType == "Desktop" ? "DeskP" : "Pers";
//@line 107 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/downloads.js"
    return "Home";
  },

  _getDownloadsFolder: function (aFolder)
  {
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    var dir = fileLocator.get(this._getSpecialFolderKey(aFolder), 
                              Components.interfaces.nsILocalFile);
    if (aFolder != "Desktop")
      dir.append("My Downloads");
      
    return dir;
  },
  
  _getDisplayNameOfFile: function (aFolder)
  {
    // TODO: would like to add support for 'Downloads on Macintosh HD' 
    //       for OS X users.
    return aFolder ? aFolder.path : "";
  },
  
  readDownloadDirPref: function ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var downloadFolder = document.getElementById("downloadFolder");

    var customDirPref = document.getElementById("browser.download.dir");
    var customIndex = customDirPref.value ? this._fileToIndex(customDirPref.value) : 0;
    if (folderListPref.value == 0 || customIndex == 0)
      downloadFolder.label = bundlePreferences.getString("desktopFolderName");
    else if (folderListPref.value == 1 || customIndex == 1) 
      downloadFolder.label = bundlePreferences.getString("myDownloadsFolderName");
    else
      downloadFolder.label = this._getDisplayNameOfFile(customDirPref.value);
    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    var currentDirPref = document.getElementById("browser.download.downloadDir");
    var downloadDir = currentDirPref.value || this._indexToFile(folderListPref.value);
    var urlspec = fph.getURLSpecFromFile(downloadDir);
    downloadFolder.image = "moz-icon://" + urlspec + "?size=16";
    
    return undefined;
  },
  
  writeFolderList: function ()
  {
    var currentDirPref = document.getElementById("browser.download.downloadDir");
    return this._fileToIndex(currentDirPref.value);
  },
  
  showWhenStartingPrefChanged: function ()
  {
    var showWhenStartingPref = document.getElementById("browser.download.manager.showWhenStarting");
    var closeWhenDonePref = document.getElementById("browser.download.manager.closeWhenDone");
    closeWhenDonePref.disabled = !showWhenStartingPref.value;
  },
  
  readShowWhenStartingPref: function ()
  {
    this.showWhenStartingPrefChanged();
    return undefined;
  },
  
  showFileTypeActions: function ()
  {
    document.documentElement.openWindow("Preferences:DownloadActions",
                                        "chrome://browser/content/preferences/downloadactions.xul",
                                        "", null);
  }
};
