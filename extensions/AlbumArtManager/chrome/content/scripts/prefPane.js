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

// Use var, not const, so that we won't conflict with other overlays that define
// the same variables
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

function makeFile(path) {
  var file = Cc["@mozilla.org/file/local;1"].
             createInstance(Ci.nsILocalFile);
  try {
    file.initWithPath(path);
  }
  catch (e) {
    return null;
  }
  return file;
}

function folderIsValid(folder) {
  try {
    return folder && folder.isDirectory();
  }
  catch (e) {
  }
  return false;
}


var gAlbumArtManagerPane = {
  _folder: null,            // Folder we are set to download covers to
  
  _getDefaultDownloadFolder: function() {
    return Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
            .getService(Ci.sbIDownloadDeviceHelper)
            .getDefaultMusicFolder();
  },

  /**
   * \brief opens the pref pane when requested by the preferences window.
   */
  openPrefsPane: function() {
    var windowMediator;
    var window;

    /* Get the main Songbird window. */
    windowMediator = Components.classes
                        ["@mozilla.org/appshell/window-mediator;1"].
                        getService(Components.interfaces.nsIWindowMediator);
    window = windowMediator.getMostRecentWindow("Songbird:Main");

    /* Open the preferences pane. */
    window.SBOpenPreferences("library_albumartmanager_pref_pane");
  },
  
  /**
   * \brief opens the priority manager window
   */
  openPriorityManager: function() {
    document.documentElement.openWindow("AlbumArtmanager:PriorityManager",
                                        "chrome://albumartmanager/content/xul/fetcherPriorities.xul",
                                        "", null);
  },
  
  /**
   * \brief checks that the default cover file is not blank
   */
  checkDefaultCoverFile: function() {
    var coverFileFormat = document.getElementById("aamCoverFileFormat");
    if (coverFileFormat.value == "") {
      coverFileFormat.value = "%artist% - %album%";
    }
    return true;
  },

  /**
   * Enables/disables the folder field and Browse button based on whether a
   * default download directory is being used.
   */
  readUseDownloadDir: function ()
  {
    var downloadFolder = document.getElementById("aamDownloadFolder");
    var chooseFolder = document.getElementById("aamChooseFolder");
    //var coverFileFormat = document.getElementById("aamCoverFileFormat");
    var preference = document.getElementById("songbird.albumartmanager.cover.usealbumlocation");
    downloadFolder.disabled = preference.value;
    chooseFolder.disabled = preference.value;
    //coverFileFormat.disabled = preference.value;
    
    // don't override the preference's value in UI
    return undefined;
  },

  /**
   * Nice getter/setter for the path that takes care of setting the icon and
   * hiding the full path from the UI.
   */
  get folder() {
    return this._folder;
  },
  set folder(val) {
    // Bail if the value is the same.
    if (this._folder && this._folder.equals(val)) {
      return val;
    }

    // Figure out the display name and icon for the path.
    const ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
    const fph = ios.getProtocolHandler("file").
                    QueryInterface(Ci.nsIFileProtocolHandler);

    var leafName, iconURL, path;
    if (val) {
      leafName = val.leafName;
      iconURL = "moz-icon://" + fph.getURLSpecFromFile(val) + "?size=16";
      path = val.path;
    }
    else {
      leafName = iconURL = path = "";
    }

    // Update the UI.
    var pathField = document.getElementById("aamDownloadFolder");
    pathField.setAttribute("label", leafName);
    pathField.setAttribute("image", iconURL);

    var folderPref = document.getElementById("songbird.albumartmanager.cover.folder");
    folderPref.value = val;

    this._folder = val;
    return val;
  },

  /**
   * Displays a file picker in which the user can choose the location where
   * covers are saved, updating preferences and UI in response to the choice,
   * if one is made.
   */
  chooseFolder: function ()
  {
    var bundlePreferences = document.getElementById("albumartmanagerbundle");
    var title = bundlePreferences.getString("albumartmanager.prefs.chooseDownloadFolderTitle");
    var folderPicker = Cc["@mozilla.org/filepicker;1"]
                       .createInstance(Ci.nsIFilePicker);
    folderPicker.init(window, title, Ci.nsIFilePicker.modeGetFolder);
    folderPicker.displayDirectory = this.folder;

    if (folderPicker.show() == Ci.nsIFilePicker.returnOK) {
      var selectedFolder = folderPicker.file;
      if (!folderIsValid(selectedFolder)) {
        selectedFolder = this._getDefaultDownloadFolder();
      }
      this.folder = selectedFolder;

      this.folder = selectedFolder;
    }
  },

  /**
   * Initializes the covers folder display settings based on the user's
   * preferences.
   */
  displayDownloadDirPref: function ()
  {
    var folderPref = document.getElementById("songbird.albumartmanager.cover.folder");
    var newFolder = folderPref.value;
    if (!folderIsValid(newFolder)) {
      newFolder = this._getDefaultDownloadFolder();
    }
    this.folder = newFolder;
  }
};
