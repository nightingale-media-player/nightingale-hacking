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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/ArrayConverter.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;
const CE = Components.Exception;

const PREF_DOWNLOAD_MUSIC_FOLDER       = "songbird.download.music.folder";
const PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT = "songbird.download.music.alwaysPrompt";

/**
 * Our super-smart heuristic for determining if the download folder is valid.
 */
function folderIsValid(folder) {
  try {
    return folder && folder.isDirectory();
  }
  catch (e) {
  }
  return false;
}

/**
 * Returns an nsILocalFile or null if the path is bad.
 */
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

/**
 * Returns an nsIURI or null if the path is bad.
 */
function makeFileURI(path)
{

  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var file = makeFile(path);

  var uri;
  try {
    uri = ios.newFileURI(file);
  }
  catch (e) {
    return null;
  }
  return uri;
}

function sbDownloadDeviceHelper()
{
  this._mainLibrary = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                        .getService(Ci.sbILibraryManager).mainLibrary;
}

sbDownloadDeviceHelper.prototype =
{
  classDescription: "Songbird Download Device Helper",
  classID:          Components.ID("{576b6833-15d8-483a-84d6-2fbd329c82e1}"),
  contractID:       "@songbirdnest.com/Songbird/DownloadDeviceHelper;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIDownloadDeviceHelper])
}

sbDownloadDeviceHelper.prototype.downloadItem =
function sbDownloadDeviceHelper_downloadItem(aMediaItem)
{
  var downloadFileURI = this._safeDownloadFileURI();
  if (!downloadFileURI) {
    return;
  }

  if (!this.downloadMediaList) {
    throw Cr.NS_ERROR_FAILURE;
  }

  let uriArray           = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);

  let item;
  if (aMediaItem.library.equals(this._mainLibrary)) {
    item = aMediaItem;
  }
  else {
    this._addItemToArrays(aMediaItem, downloadFileURI, uriArray,
                          propertyArrayArray);
    let items = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                        propertyArrayArray,
                                                        true);
    item = items.queryElementAt(0, Ci.sbIMediaItem);
  }

  this.downloadMediaList.add(item);
}

sbDownloadDeviceHelper.prototype.downloadSome =
function sbDownloadDeviceHelper_downloadSome(aMediaItems)
{
  var downloadFileURI = this._safeDownloadFileURI();
  if (!downloadFileURI) {
    return;
  }

  if (!this.downloadMediaList) {
    throw Cr.NS_ERROR_FAILURE;
  }

  let uriArray           = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);

  var items = [];
  while (aMediaItems.hasMoreElements()) {
    let item = aMediaItems.getNext();
    if (item.library.equals(this._mainLibrary)) {
      items.push(item);
    }
    else {
      this._addItemToArrays(item, downloadFileURI, uriArray,
                            propertyArrayArray);
    }
  }

  if (uriArray.length > 0) {
    let addedItems = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                             propertyArrayArray,
                                                             true);
    for (let i = 0; i < addedItems.length; i++) {
      items.push(addedItems.queryElementAt(i, Ci.sbIMediaItem));
    }
  }

  this.downloadMediaList.addSome(ArrayConverter.enumerator(items));
}

sbDownloadDeviceHelper.prototype.downloadAll =
function sbDownloadDeviceHelper_downloadAll(aMediaList)
{
  var downloadFileURI = this._safeDownloadFileURI();
  if (!downloadFileURI) {
    return;
  }

  if (!this.downloadMediaList) {
    throw Cr.NS_ERROR_FAILURE;
  }

  let uriArray           = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@mozilla.org/array;1"]
                             .createInstance(Ci.nsIMutableArray);

  var items = [];
  var isForeign = aMediaList.library.equals(this._mainLibrary);
  for (let i = 0; i < aMediaList.length; i++) {
    var item = aMediaList.getItemByIndex(i);
    if (isForeign) {
      this._addItemToArrays(item, downloadFileURI, uriArray,
                            propertyArrayArray);
    }
    else {
      items.push(item);
    }
  }

  if (uriArray.length > 0) {
    let addedItems = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                             propertyArrayArray,
                                                             true);
    for (let i = 0; i < addedItems.length; i++) {
      items.push(addedItems.queryElementAt(i, Ci.sbIMediaItem));
    }
  }

  this.downloadMediaList.addSome(ArrayConverter.enumerator(items));
}

sbDownloadDeviceHelper.prototype.__defineGetter__("downloadMediaList",
function sbDownloadDeviceHelper_getDownloadMediaList()
{
  if (!this._downloadDevice) {
    var devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;1"]
                   .getService(Ci.sbIDeviceManager);
    if (devMgr) {
      var downloadCat = "Songbird Download Device";
      if (devMgr.hasDeviceForCategory(downloadCat)) {
        this._downloadDevice = devMgr.getDeviceByCategory(downloadCat)
                                     .QueryInterface(Ci.sbIDownloadDevice);
      }
    }
  }
  return this._downloadDevice ? this._downloadDevice.downloadMediaList : null;
});

sbDownloadDeviceHelper.prototype.__defineGetter__("defaultMusicFolder",
function sbDownloadDeviceHelper_getDefaultMusicFolder()
{
  // XXXben Cache the result of this method? Maybe we can't due to the case
  //        ambiguity of the music folder on linux?
  const dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIDirectoryServiceProvider);
  const platform = Cc["@mozilla.org/xre/app-info;1"].
                   getService(Ci.nsIXULRuntime).
                   OS;

  var musicDir;
  switch (platform) {
    case "WINNT":
      var docsDir = dirService.getFile("Pers", {});
      musicDir = docsDir.append("My Music");
      break;

    case "Darwin":
      musicDir = dirService.getFile("Music", {});
      break;

    case "Linux":
      var homeDir = dirService.getFile("Home", {});
      musicDir = homeDir.clone().append("Music");
      if (!musicDir.isDirectory()) {
        musicDir = homeDir.append("music");
      }
      break;

    default:
      // Fall through and use the Desktop below.
      break;
  }

  // Make sure that the directory exists and is writable.
  if (!folderIsValid(musicDir)) {
    // Great, default to the Desktop... This should work on all OS's.
    musicDir = dirService.get("Desk", {});

    // We should never get something bad here, but just in case...
    if (!folderIsValid(musicdir)) {
      Cu.reportError("Desktop directory is not a directory!");
      throw Cr.NS_ERROR_FILE_NOT_DIRECTORY;
    }
  }

  return musicDir;
});

sbDownloadDeviceHelper.prototype.__defineGetter__("downloadFolder",
function sbDownloadDeviceHelper_getDownloadFolder()
{
  const Application = Cc["@mozilla.org/fuel/application;1"].
                      getService(Ci.fuelIApplication);
  const prefs = Application.prefs;

  var downloadFolder;
  if (prefs.has(PREF_DOWNLOAD_MUSIC_FOLDER)) {
    var downloadPath = prefs.get(PREF_DOWNLOAD_MUSIC_FOLDER).value;
    downloadFolder = makeFile(downloadPath);
  }

  if (!folderIsValid(downloadFolder)) {
    // The pref was either bad or empty. Use (and write) the default.
    downloadFolder = this.defaultMusicFolder;
    prefs.setValue(PREF_DOWNLOAD_MUSIC_FOLDER, downloadFolder.path);
  }

  const alwaysPrompt = prefs.getValue(PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT, false);
  if (!alwaysPrompt) {
    return downloadFolder;
  }

  const sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
  const strings =
    sbs.createBundle("chrome://songbird/locale/songbird.properties");
  const title =
    strings.GetStringFromName("prefs.main.musicdownloads.chooseTitle");

  const wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
  const mainWin = wm.getMostRecentWindow("Songbird:Main");

  // Need to prompt, launch the filepicker.
  const folderPicker = Cc["@mozilla.org/filepicker;1"].
                       createInstance(Ci.nsIFilePicker);
  folderPicker.init(mainWin, title, Ci.nsIFilePicker.modeGetFolder);
  folderPicker.displayDirectory = downloadFolder;

  if (folderPicker.show() != Ci.nsIFilePicker.returnOK) {
    // This is our signal that the user cancelled the dialog. Some folks won't
    // care, but most will.
    throw new CE("User canceled the download dialog.", Cr.NS_ERROR_ABORT);
  }

  downloadFolder = folderPicker.file;

  prefs.setValue(PREF_DOWNLOAD_MUSIC_FOLDER, downloadFolder.path);
  return downloadFolder;
});

sbDownloadDeviceHelper.prototype._safeDownloadFileURI =
function sbDownloadDeviceHelper__safeDownloadFileURI()
{
  // This function returns null if the user cancels the dialog.
  try {
    return makeFileURI(this.downloadFolder.path).spec;
  }
  catch (e if e.result == Cr.NS_ERROR_ABORT) {
  }
  return null;
}

sbDownloadDeviceHelper.prototype._addItemToArrays =
function sbDownloadDeviceHelper__addItemToArrays(aMediaItem,
                                                 aDownloadPath,
                                                 aURIArray,
                                                 aPropertyArrayArray)
{
  aURIArray.appendElement(aMediaItem.contentSrc, false);

  var dest   = SBProperties.createArray();
  var source = aMediaItem.getProperties();
  for (let i = 0; i < source.length; i++) {
    var prop = source.getPropertyAt(i);
    if (prop.id != SBProperties.contentSrc) {
      dest.appendElement(prop, false);
    }
  }

  var target = aMediaItem.library.guid + "," + aMediaItem.guid;
  dest.appendProperty(SBProperties.downloadStatusTarget, target);

  // Set the destination property so that the download device doesn't re-query
  // the user if the 'always' pref isn't set.
  dest.appendProperty(SBProperties.destination, aDownloadPath);

  aPropertyArrayArray.appendElement(dest, false);
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbDownloadDeviceHelper]);
}
