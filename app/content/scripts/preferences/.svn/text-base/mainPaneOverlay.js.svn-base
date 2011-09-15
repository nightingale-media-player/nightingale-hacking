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

const PREF_DOWNLOAD_MUSIC_FOLDER      = "songbird.download.music.folder";
const PREF_DOWNLOAD_MUSIC_ALWAYS_ASK  = "songbird.download.music.alwaysPrompt";
const PREF_DOWNLOAD_VIDEO_FOLDER      = "songbird.download.video.folder";
const PREF_DOWNLOAD_VIDEO_ALWAYS_ASK  = "songbird.download.video.alwaysPrompt";

// Use var, not const, so that we won't conflict with other overlays that define
// the same variables
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

Cu.import("resource://app/jsmodules/PlatformUtils.jsm");

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
  _defaultVideoFolder: null,

  /**
   * This is the element that will display the pretty file path.
   */
  _musicPrefBox: null,
  _videoPrefBox: null,

  /**
   * These are the preference elements that we will create.
   */
  _musicFolderPref: null,
  _musicFolderAskPref: null,
  _videoFolderPref: null,
  _videoFolderAskPref: null,

  /**
   * The string bundle we'll use.
   */
  _strings: null,

  /**
   * Getter/setter for the music folder UI widget
   */
  get musicFolder() {
    return this._musicPrefBox.folder;
  },
  set musicFolder(val) {
    this._musicPrefBox.folder = val;
    return this.musicFolder;
  },

  /**
   * Getter/setter for the video folder UI widget
   */
  get videoFolder() {
    return this._videoPrefBox.folder;
  },

  set videoFolder(val) {
    this._videoPrefBox.folder = val;
    return this.videoFolder;
  },

  /**
   * Getter/setter to control the music radio buttons.
   */
  get musicFolderAsk() {
    return this._musicPrefBox.ask;
  },
  set musicFolderAsk(val) {
    this._musicPrefBox.ask = val;
    return this.musicFolderAsk;
  },

  /**
   * Getter/setter to control the video radio buttons.
   */
  get videoFolderAsk() {
    return this._videoPrefBox.ask;
  },
  set videoFolderAsk(val) {
    this._videoPrefBox.ask = val;
    return this.videoFolderAsk;
  },

  /**
   * This builds the UI dynamically.
   */
  _createMusicUIElements: function() {
    // Make some preference elements.
    const preferencesElement = document.getElementById("mainPreferences");

    this._musicFolderAskPref = document.createElement("preference");
    this._musicFolderAskPref.setAttribute("id", PREF_DOWNLOAD_MUSIC_ALWAYS_ASK);
    this._musicFolderAskPref.setAttribute("name",
                                          PREF_DOWNLOAD_MUSIC_ALWAYS_ASK);
    this._musicFolderAskPref.setAttribute("type", "bool");
    this._musicFolderAskPref.setAttribute(
                          "onchange",
                          "SongbirdMainPaneOverlay.onMusicAskPrefChanged();");
    preferencesElement.appendChild(this._musicFolderAskPref);

    this._musicFolderPref = document.createElement("preference");
    this._musicFolderPref.setAttribute("id", PREF_DOWNLOAD_MUSIC_FOLDER);
    this._musicFolderPref.setAttribute("name", PREF_DOWNLOAD_MUSIC_FOLDER);
    this._musicFolderPref.setAttribute("type", "unichar");
    this._musicFolderPref.setAttribute(
                       "onchange",
                       "SongbirdMainPaneOverlay.onMusicFolderPrefChanged();");
    preferencesElement.appendChild(this._musicFolderPref);

    // Build the music downloads box
    this._musicPrefBox = document.createElement("sb-savefolder-box");
    this._musicPrefBox.setAttribute("id","musicDownloadsGroup");
    this._musicPrefBox.setAttribute(
                       "oncommand",
                       "SongbirdMainPaneOverlay.onUserChangedMusicBox()");

    // Insert it into the DOM to apply the XBL binding before trying
    // to access its properties:
    const mainPrefPane = document.getElementById("paneMain");
    const downloadsGroup = document.getElementById("downloadsGroup");
    mainPrefPane.insertBefore(this._musicPrefBox, downloadsGroup);

    // Populate the labels.  Control labels are borrowed from the
    // corresponding elements in the existing moz file downloads
    // groupbox.  Other labels are taken from the strings bundle:
    this._musicPrefBox.title =
      this._strings.getString("prefs.main.downloads.audio.label");
    this._musicPrefBox.folderLabel =
      document.getElementById("saveTo").getAttribute("label"),
    this._musicPrefBox.browseLabel =
      document.getElementById("chooseFolder").getAttribute("label"),
    this._musicPrefBox.askLabel =
      document.getElementById("alwaysAsk").getAttribute("label");
    this._musicPrefBox.browseWindowTitle =
      this._strings.getString("prefs.main.downloads.audio.chooseTitle");
  },

  _createVideoUIElements: function() {
    // Make some preference elements.
    const preferencesElement = document.getElementById("mainPreferences");

    this._videoFolderAskPref = document.createElement("preference");
    this._videoFolderAskPref.setAttribute("id", PREF_DOWNLOAD_VIDEO_ALWAYS_ASK);
    this._videoFolderAskPref.setAttribute("name",
                                        PREF_DOWNLOAD_VIDEO_ALWAYS_ASK);
    this._videoFolderAskPref.setAttribute("type", "bool");
    this._videoFolderAskPref.setAttribute(
                          "onchange",
                          "SongbirdMainPaneOverlay.onVideoAskPrefChanged();");
    preferencesElement.appendChild(this._videoFolderAskPref);

    this._videoFolderPref = document.createElement("preference");
    this._videoFolderPref.setAttribute("id", PREF_DOWNLOAD_VIDEO_FOLDER);
    this._videoFolderPref.setAttribute("name", PREF_DOWNLOAD_VIDEO_FOLDER);
    this._videoFolderPref.setAttribute("type", "unichar");
    this._videoFolderPref.setAttribute(
                        "onchange",
                        "SongbirdMainPaneOverlay.onVideoFolderPrefChanged();");
    preferencesElement.appendChild(this._videoFolderPref);

    // Build the video downloads box
    this._videoPrefBox = document.createElement("sb-savefolder-box");
    this._videoPrefBox.setAttribute("id","videoDownloadsGroup");
    this._videoPrefBox.setAttribute(
                       "oncommand",
                       "SongbirdMainPaneOverlay.onUserChangedVideoBox()");

    // Insert it into the DOM to apply the XBL binding before trying
    // to access its properties
    const mainPrefPane = document.getElementById("paneMain");
    const downloadsGroup = document.getElementById("downloadsGroup");
    mainPrefPane.insertBefore(this._videoPrefBox, downloadsGroup);

    // Populate the labels.  Control labels are borrowed from the
    // corresponding elements in the existing moz file downloads
    // groupbox.  Other labels are taken from the strings bundle:
    this._videoPrefBox.title =
      this._strings.getString("prefs.main.downloads.video.label");
    this._videoPrefBox.folderLabel =
      document.getElementById("saveTo").getAttribute("label"),
    this._videoPrefBox.browseLabel =
      document.getElementById("chooseFolder").getAttribute("label"),
    this._videoPrefBox.askLabel =
      document.getElementById("alwaysAsk").getAttribute("label");
    this._videoPrefBox.browseWindowTitle =
      this._strings.getString("prefs.main.downloads.video.chooseTitle");
  },

  _createUIElements: function() {
    var tempString;

    // Relabel the original downloads caption.
    const downloadsCaptions = document.getElementById("downloadsGroup")
                                      .getElementsByTagName("caption");
    if (downloadsCaptions.length) {
      tempString = this._strings.getString("prefs.main.browserdownloads.label");
      downloadsCaptions.item(0).setAttribute("label", tempString);
    }

    // Hide the addons button
    //   actually, we'll remove the hbox containing to the addonsGroupbox to
    //   avoid bug17280
    var addonsGroup = document.getElementById("addonsMgrGroup");
    addonsGroup.parentNode.parentNode.removeChild(addonsGroup.parentNode);

    this._createMusicUIElements();
    this._createVideoUIElements();
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
    var downloadHelper =
      Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"].
      getService(Ci.sbIDownloadDeviceHelper)
    self._defaultMusicFolder =
      downloadHelper.getDefaultDownloadFolder("audio");
    self._defaultVideoFolder =
      downloadHelper.getDefaultDownloadFolder("video");

    self._strings = document.getElementById("bundleSongbirdPreferences");

    // Build the UI. This requires strings to be defined.
    self._createUIElements();

    // Sync from preferences.
    var musicFolder = makeFile(self._musicFolderPref.value);
    if (!folderIsValid(musicFolder)) {
      // Reset to default.
      musicFolder = self._defaultMusicFolder;
      self._musicFolderPref.value = (musicFolder ? musicFolder.path : "");
    }
    self.musicFolder = musicFolder;

    self.musicFolderAsk = self._musicFolderAskPref.value;

    var videoFolder = makeFile(self._videoFolderPref.value);
    if (!folderIsValid(videoFolder)) {
      // Reset to default.
      videoFolder = self._defaultVideoFolder;
      self._videoFolderPref.value = (videoFolder ? videoFolder.path : "");
    }
    self.videoFolder = videoFolder;

    self.videoFolderAsk = self._videoFolderAskPref.value;
  },

  /**
   * Called when the user changes the music settings box
   */
  onUserChangedMusicBox: function() {
    // The user changed a pref control.  Reflect the new state to
    // the preferences.
    const self = SongbirdMainPaneOverlay;
    var path = (self.musicFolder ? self.musicFolder.path : "");
    self._musicFolderPref.value = path;
    self._musicFolderAskPref.value = self.musicFolderAsk;
  },

  /**
   * Called when the user changes the video settings box
   */
  onUserChangedVideoBox: function() {
    // The user changed a pref control.  Reflect the new state to
    // the preferences.
    const self = SongbirdMainPaneOverlay;
    var path = (self.videoFolder ? self.videoFolder.path : "");
    self._videoFolderPref.value = path;
    self._videoFolderAskPref.value = self.videoFolderAsk;
  },

  /**
   * Called when the music folder pref changes.
   */
  onMusicFolderPrefChanged: function() {
    // The folder pref changed.  Load the value and use
    // it to update the pref box UI:
    const self = SongbirdMainPaneOverlay;
    var newFolder = makeFile(self._musicFolderPref.value);
    if (!folderIsValid(newFolder)) {
      // The pref is not a valid folder.  Set it to the default
      // folder.  This function will get called again if, and only
      // if, the pref value changes here:
      newFolder = self._defaultMusicFolder;
      self._musicFolderPref.value = (newFolder ? newFolder.path : "");
    }
    
    // Update the pref box UI with the new folder setting:
    self.musicFolder = newFolder;
  },

  /**
   * Called when the video folder pref changes.
   */
  onVideoFolderPrefChanged: function() {
    // The folder pref changed.  Load the value and use
    // it to update the pref box UI:
    const self = SongbirdMainPaneOverlay;
    var newFolder = makeFile(self._videoFolderPref.value);
    if (!folderIsValid(newFolder)) {
      // The pref is not a valid folder.  Set it to the default
      // folder.  This function will get called again if, and only
      // if, the pref value changes here:
      newFolder = self._defaultVideoFolder;
      self._videoFolderPref.value = (newFolder ? newFolder.path : "");
    }
    
    // Update the pref box UI with the new folder setting:
    self.videoFolder = newFolder;
  },

  /**
   * Called when the music folder prompt setting changes.
   */
  onMusicAskPrefChanged: function() {
    // The ask pref changed.  Load the value and use
    // it to update the pref box UI:
    const self = SongbirdMainPaneOverlay;
    self.musicFolderAsk = self._musicFolderAskPref.value;
  },

  /**
   * Called when the video folder prompt setting changes.
   */
  onVideoAskPrefChanged: function() {
    // The ask pref changed.  Load the value and use
    // it to update the pref box UI:
    const self = SongbirdMainPaneOverlay;
    self.videoFolderAsk = self._videoFolderAskPref.value;
  }
};

// Don't forget to load! Can't use the standard load event because the
// individual preference panes are loaded asynchronously via
// document.loadOverlay (see preferences.xml) and the load event may fire before
// our target pane has been integrated.
window.addEventListener('paneload', SongbirdMainPaneOverlay.onPaneLoad, false);
