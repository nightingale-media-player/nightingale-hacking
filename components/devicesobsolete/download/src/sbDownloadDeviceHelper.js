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
 * Returns a file url for the path.
 */
function makeFileURL(path)
{
  // Can't use the IOService because we have callers off of the main thread...
  // Super lame hack instead.
  return "file://" + path;
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
  var downloadFileURL = this._safeDownloadFileURL();
  if (!downloadFileURL) {
    return;
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
    this._addItemToArrays(aMediaItem, uriArray, propertyArrayArray);
    let items = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                        propertyArrayArray,
                                                        true);
    item = items.queryElementAt(0, Ci.sbIMediaItem);
  }

  this._setDownloadDestination([item], downloadFileURL);
  this.getDownloadMediaList().add(item);
}

sbDownloadDeviceHelper.prototype.downloadSome =
function sbDownloadDeviceHelper_downloadSome(aMediaItems)
{
  var downloadFileURL = this._safeDownloadFileURL();
  if (!downloadFileURL) {
    return;
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
      this._addItemToArrays(item, uriArray, propertyArrayArray);
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

  this._setDownloadDestination(items, downloadFileURL);
  this.getDownloadMediaList().addSome(ArrayConverter.enumerator(items));
}

sbDownloadDeviceHelper.prototype.downloadAll =
function sbDownloadDeviceHelper_downloadAll(aMediaList)
{
  var downloadFileURL = this._safeDownloadFileURL();
  if (!downloadFileURL) {
    return;
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
      this._addItemToArrays(item, uriArray, propertyArrayArray);
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

  this._setDownloadDestination(items, downloadFileURL);
  this.getDownloadMediaList().addSome(ArrayConverter.enumerator(items));
}

sbDownloadDeviceHelper.prototype.getDownloadMediaList =
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
}

sbDownloadDeviceHelper.prototype.getDefaultMusicFolder =
function sbDownloadDeviceHelper_getDefaultMusicFolder()
{
  const dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIDirectoryServiceProvider);
  const platform = Cc["@mozilla.org/xre/app-info;1"].
                   getService(Ci.nsIXULRuntime).
                   OS;

  var musicDir;
  switch (platform) {
    case "WINNT":
      musicDir = dirService.getFile("Pers", {});
      musicDir.append("My Music");
      break;

    case "Darwin":
      musicDir = dirService.getFile("Music", {});
      break;

    case "Linux":
      musicDir = dirService.getFile("Home", {});
      musicDir.append("Music");

      if (!folderIsValid(musicDir)) {
        musicDir = musicDir.parent;
        musicDir.append("music");
      }
      break;

    default:
      // Fall through and use the Desktop below.
      break;
  }

  // Make sure that the directory exists and is writable.
  if (!folderIsValid(musicDir)) {
    // Great, default to the Desktop... This should work on all OS's.
    musicDir = dirService.getFile("Desk", {});

    // We should never get something bad here, but just in case...
    if (!folderIsValid(musicDir)) {
      Cu.reportError("Desktop directory is not a directory!");
      throw Cr.NS_ERROR_FILE_NOT_DIRECTORY;
    }
  }

  return musicDir;
}

sbDownloadDeviceHelper.prototype.getDownloadFolder =
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
    downloadFolder = this.getDefaultMusicFolder();
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
}

sbDownloadDeviceHelper.prototype._safeDownloadFileURL =
function sbDownloadDeviceHelper__safeDownloadFileURL()
{
  // This function returns null if the user cancels the dialog.
  try {
    return makeFileURL(this.getDownloadFolder().path);
  }
  catch (e if e.result == Cr.NS_ERROR_ABORT) {
  }
  return "";
}

sbDownloadDeviceHelper.prototype._addItemToArrays =
function sbDownloadDeviceHelper__addItemToArrays(aMediaItem,
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

  aPropertyArrayArray.appendElement(dest, false);
}

sbDownloadDeviceHelper.prototype._setDownloadDestination =
function sbDownloadDeviceHelper__setDownloadDestination(aItems,
                                                        aDownloadPath)
{
  for each (var item in aItems) {
    try {
      item.setProperty(SBProperties.destination, aDownloadPath);
    }
    catch (e) {
      // We're not allowed to set the download destination on remote media items
      // so that call may fail. The remoteAPI should unwrap all items and lists
      // *before* handing them to us, so throw the error with a slightly more
      // helpful message.
      throw new CE("Download destination could not be set on this media item:\n"
                   + "  " + item + "\nIs it actually a remote media item?",
                   e.result);
    }
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbDownloadDeviceHelper]);
}
