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

const PREF_DOWNLOAD_MUSIC_FOLDER       = "songbird.download.music.folder";
const PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT = "songbird.download.music.alwaysPrompt";

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

var SongbirdMainPaneOverlay = {
  /**
   * Cached default music folder for the platform this code runs on.
   */
  _defaultMusicFolder: null,

  /**
   * This is the element that will display the pretty file path.
   */
  _pathField: null,

  /**
   * Thiese are the radio items thatcontrol the 'alwaysPrompt' pref.
   */
  _radioNoPrompt: null,
  _radioPrompt: null,

  /**
   * This is the button for selecting a folder.
   */
  _browseButton: null,

  /**
   * These are the preference elements that we will create.
   */
  _folderPref: null,
  _alwaysPromptPref: null,

  /**
   * The string bundle we'll use.
   */
  _strings: null,

  /**
   * The real folder that the path field is displaying.
   */
  _folder: null,

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
    this._pathField.setAttribute("label", leafName);
    this._pathField.setAttribute("image", iconURL);

    this._folder = val;
    return val;
  },

  /**
   * Getter/setter to control the alwaysPrompt preference and associated UI.
   */
  get alwaysPrompt() {
    return this._radioPrompt.getAttribute("selected") == "true";
  },
  set alwaysPrompt(val) {
    // Update the UI.
    if (val) {
      this._radioPrompt.setAttribute("selected", "true");
      this._radioNoPrompt.removeAttribute("selected");
      this._pathField.setAttribute("disabled", "true");
      this._browseButton.setAttribute("disabled", "true");
    }
    else {
      this._radioNoPrompt.setAttribute("selected", "true");
      this._radioPrompt.removeAttribute("selected");
      this._pathField.removeAttribute("disabled");
      this._browseButton.removeAttribute("disabled");
    }

    return val;
  },

  /**
   * This builds the UI dynamically.
   */
  _createUIElements: function() {
    // Make some preference elements.
    const preferencesElement = document.getElementById("mainPreferences");

    this._alwaysPromptPref = document.createElement("preference");
    this._alwaysPromptPref.setAttribute("id", PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT);
    this._alwaysPromptPref.setAttribute("name",
                                        PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT);
    this._alwaysPromptPref.setAttribute("type", "bool");
    this._alwaysPromptPref.setAttribute("onchange",
                                        "SongbirdMainPaneOverlay.onPromptChanged();");
    preferencesElement.appendChild(this._alwaysPromptPref);

    this._folderPref = document.createElement("preference");
    this._folderPref.setAttribute("id", PREF_DOWNLOAD_MUSIC_FOLDER);
    this._folderPref.setAttribute("name", PREF_DOWNLOAD_MUSIC_FOLDER);
    this._folderPref.setAttribute("type", "unichar");
    this._folderPref.setAttribute("onchange",
                                  "SongbirdMainPaneOverlay.onFolderChanged();");
    preferencesElement.appendChild(this._folderPref);

    var tempString;

    // Relabel the original downloads caption.
    const downloadsCaptions = document.getElementById("downloadsGroup")
                                      .getElementsByTagName("caption");
    if (downloadsCaptions.length) {
      tempString = this._strings.getString("prefs.main.browserdownloads.label");
      downloadsCaptions.item(0).setAttribute("label", tempString);
    }

    // Remove the addons button
    const addonsGroup = document.getElementById("addonsMgrGroup");
    addonsGroup.setAttribute("hidden", "true");

    // Build the rest of the content.
    const groupbox = document.createElement("groupbox");
    groupbox.setAttribute("id","mediaDownloadsGroup");
    groupbox.setAttribute("flex","1");

    const caption = document.createElement("caption");
    caption.setAttribute("id","mediaDownloadsCaption");
    tempString = this._strings.getString("prefs.main.musicdownloads.label");
    caption.setAttribute("label", tempString);
    groupbox.appendChild(caption);

    const promptGroup = document.createElement("radiogroup");
    promptGroup.setAttribute("id", "mediaSaveWhere");
    promptGroup.setAttribute("oncommand",
                             "SongbirdMainPaneOverlay.onUserChangedPrompt()");
    groupbox.appendChild(promptGroup);

    const hbox = document.createElement("hbox");
    hbox.setAttribute("id", "mediaSaveToRow");
    promptGroup.appendChild(hbox);

    this._radioNoPrompt = document.createElement("radio");
    this._radioNoPrompt.setAttribute("id", "mediaSaveTo");
    tempString = document.getElementById("saveTo").getAttribute("label");
    this._radioNoPrompt.setAttribute("label", tempString);
    this._radioNoPrompt.setAttribute("value", "false");
    hbox.appendChild(this._radioNoPrompt);

    this._pathField = document.createElement("filefield");
    this._pathField.setAttribute("id", "mediaDownloadFolder");
    this._pathField.setAttribute("flex", "1");
    hbox.appendChild(this._pathField);

    this._browseButton = document.createElement("button");
    this._browseButton.setAttribute("id", "mediaChooseFolder");
    tempString = document.getElementById("chooseFolder").getAttribute("label");
    this._browseButton.setAttribute("label", tempString);
    this._browseButton.setAttribute("oncommand",
                                    "SongbirdMainPaneOverlay.onBrowse()");
    hbox.appendChild(this._browseButton);

    this._radioPrompt = document.createElement("radio");
    this._radioPrompt.setAttribute("id", "mediaAlwaysAsk");
    tempString = document.getElementById("alwaysAsk").getAttribute("label");
    this._radioPrompt.setAttribute("label", tempString);
    this._radioPrompt.setAttribute("value", "true");
    promptGroup.appendChild(this._radioPrompt);

    // Finally hook it all up
    const mainPrefPane = document.getElementById("paneMain");
    const bottomBoxElements = mainPrefPane.getElementsByAttribute("class",
                                                                  "bottomBox");

    const parent = bottomBoxElements.length ?
                   bottomBoxElements.item(0) :
                   mainPrefPane;
    parent.appendChild(groupbox);
  },

  /**
   * This function sets up our UI in the main preferences dialog.
   */
   onPaneLoad: function(event) {
     // Don't actually load until the main pane has been loaded (see the comments
     // near the matching addEventListener call at the end of this code).
     if (event.target.getAttribute("id") != "paneMain") {
       return;
     }

    const self = SongbirdMainPaneOverlay;
    window.removeEventListener('paneload', self.onPaneLoad, false);

    // Save this for later.
    self._defaultMusicFolder =
      Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"].
      getService(Ci.sbIDownloadDeviceHelper).
      getDefaultMusicFolder();

    self._strings = document.getElementById("bundleSongbirdPreferences");

    // Build the UI. This requires strings to be defined.
    self._createUIElements();

    // Sync from preferences.
    var downloadFolder = makeFile(self._folderPref.value);
    if (!folderIsValid(downloadFolder)) {
      // Reset to default.
      downloadFolder = self._defaultMusicFolder;
      self._folderPref.value = downloadFolder.path;
    }
    self.folder = downloadFolder;

    self.alwaysPrompt = self._alwaysPromptPref.value;
  },

  /**
   * Called when the browse button is clicked.
   */
  onBrowse: function() {
    const self = SongbirdMainPaneOverlay;
    const title = self._strings
                      .getString("prefs.main.musicdownloads.chooseTitle");
    var folderPicker = Cc["@mozilla.org/filepicker;1"].
                       createInstance(Ci.nsIFilePicker);
    folderPicker.init(window, title, Ci.nsIFilePicker.modeGetFolder);
    folderPicker.displayDirectory = self.folder;

    if (folderPicker.show() == Ci.nsIFilePicker.returnOK) {
      var selectedFolder = folderPicker.file;
      if (!folderIsValid(selectedFolder)) {
        selectedFolder = self._defaultMusicFolder;
      }
      self._folderPref.value = selectedFolder.path;
      self.folder = selectedFolder;
    }
  },

  /**
   * Called when the user changes the prompt setting.
   */
  onUserChangedPrompt: function() {
    const self = SongbirdMainPaneOverlay;
    self._alwaysPromptPref.value = self.alwaysPrompt;
  },

  /**
   * Called when the folder changes.
   */
  onFolderChanged: function() {
    const self = SongbirdMainPaneOverlay;
    var newFolder = makeFile(self._folderPref.value);
    if (!folderIsValid(newFolder)) {
      // Recursive!
      newFolder = self._defaultMusicFolder;
      self._folderPref.value = newFolder.path;
    }
    self.folder = newFolder;
  },

  /**
   * Called when the prompt setting changes.
   */
  onPromptChanged: function() {
    const self = SongbirdMainPaneOverlay;
    self.alwaysPrompt = self._alwaysPromptPref.value;
  }
};

// Don't forget to load! Can't use the standard load event because the
// individual preference panes are loaded asynchronously via
// document.loadOverlay (see preferences.xml) and the load event may fire before
// our target pane has been integrated.
window.addEventListener('paneload', SongbirdMainPaneOverlay.onPaneLoad, false);
