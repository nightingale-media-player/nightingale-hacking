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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/FolderUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;
const CE = Components.Exception;

const PREF_DOWNLOAD_FOLDER       = {audio: "songbird.download.music.folder",
                                    video: "songbird.download.video.folder"};
const PREF_DOWNLOAD_ALWAYSPROMPT = {audio: "songbird.download.music.alwaysPrompt",
                                    video: "songbird.download.video.alwaysPrompt"};
const _MFM_GET_MANAGED_PATH_OPTIONS = Ci.sbIMediaFileManager.MANAGE_MOVE |
                                      Ci.sbIMediaFileManager.MANAGE_COPY |
                                      Ci.sbIMediaFileManager.MANAGE_RENAME;

/**
 * \brief Get the name of the platform we are running on.
 * \return The name of the OS platform. (ie. Windows).
 * \retval Windows_NT Running under Windows.
 * \retval Darwin Running under Darwin/OS X.
 * \retval Linux Running under Linux.
 * \retval SunOS Running under Solaris.
 */
function getPlatformString() 
{
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    return sysInfo.getProperty("name");
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Windows") != -1)
      return "Windows_NT";
    else if (user_agent.indexOf("Mac OS X") != -1)
      return "Darwin";
    else if (user_agent.indexOf("Linux") != -1)
      return "Linux";
    else if (user_agent.indexOf("SunOS") != -1)
      return "SunOS";
    return "";
  }
}

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

  // Ensure all paths are lowercase for Windows.
  // See bug #7178.
  var actualPath = path;
  if(getPlatformString() == "Windows_NT") {
    actualPath = actualPath.toLowerCase();
  }

  try {
    file.initWithPath(actualPath);
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

  // Ensure all paths are lowercase for Windows.
  // See bug #7178.
  var actualPath = path;
  if(getPlatformString() == "Windows_NT") {
    actualPath = actualPath.toLowerCase();
  }
  
  return "file://" + actualPath;
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
  var downloadFolder = this._getDownloadFolder_nothrow(aMediaItem.contentType);
  if (!downloadFolder) {
    return;
  }

  let uriArray           = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
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

  // Pass the downloadFolder as the 'media-folder' option for the
  // Media File Manager.  The MFM will then put the file in the
  // user's preferred folder (or possibly a subfolder thereof)
  this._setDownloadDestinationIfNotSet([item],
                                       {'media-folder': downloadFolder});
  this._checkAndRemoveExistingItem(item);
  this.getDownloadMediaList().add(item);
}

sbDownloadDeviceHelper.prototype.downloadSome =
function sbDownloadDeviceHelper_downloadSome(aMediaItems)
{
  let uriArray           = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);

  var items = [];
  var foundTypes = {};
  while (aMediaItems.hasMoreElements()) {
    let item = aMediaItems.getNext();
    if (item.library.equals(this._mainLibrary)) {
      items.push(item);
    }
    else {
      this._checkAndRemoveExistingItem(item);
      this._addItemToArrays(item, uriArray, propertyArrayArray);
    }
    // Keep track of which media types are present.  This
    // will be used below to get the download folders for
    // these media types:
    foundTypes[item.contentType] = true;
  }

  // Get the user's preferred download folder for each media type
  // in the item list and set up Media File Manager options with
  // those folders.  The MFM will then sort the files into the
  // respective folders (or possibly subfolders thereof).  Do this
  // before creating the media items in case the user is prompted
  // to choose a folder and clicks cancel:
  mfmFolders = {};
  try {
    for (let contentType in foundTypes) {
      let folder = this.getDownloadFolder(contentType);
      if (folder) {
        mfmFolders["media-folder:" + contentType] = folder;
      }
    }
  }
  catch (e if e.result == Cr.NS_ERROR_ABORT) {
    return;
  }

  if (uriArray.length > 0) {
    let addedItems = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                             propertyArrayArray,
                                                             true);
    for (let i = 0; i < addedItems.length; i++) {
      items.push(addedItems.queryElementAt(i, Ci.sbIMediaItem));
    }
  }

  this._setDownloadDestinationIfNotSet(items, mfmFolders);
  this.getDownloadMediaList().addSome(ArrayConverter.enumerator(items));
}

sbDownloadDeviceHelper.prototype.downloadAll =
function sbDownloadDeviceHelper_downloadAll(aMediaList)
{
  let uriArray           = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);
  let propertyArrayArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);

  var items = [];
  var foundTypes = {};
  var isForeign = aMediaList.library.equals(this._mainLibrary);
  for (let i = 0; i < aMediaList.length; i++) {
    var item = aMediaList.getItemByIndex(i);
    if (isForeign) {
      this._checkAndRemoveExistingItem(item);      
      this._addItemToArrays(item, uriArray, propertyArrayArray);
    }
    else {
      items.push(item);
    }
    // Keep track of which media types are present.  This
    // will be used below to get the download folders for
    // these media types:
    foundTypes[item.contentType] = true;
  }

  // Get the user's preferred download folder for each media type
  // in the item list and set up Media File Manager options with
  // those folders.  The MFM will then sort the files into the
  // respective folders (or possibly subfolders thereof).  Do this
  // before creating the media items in case the user is prompted
  // to choose a folder and clicks cancel.
  mfmFolders = {};
  try {
    for (let contentType in foundTypes) {
      let folder = this.getDownloadFolder(contentType);
      if (folder) {
        mfmFolders["media-folder:" + contentType] = folder;
      }
    }
  }
  catch (e if e.result == Cr.NS_ERROR_ABORT) {
    return;
  }

  if (uriArray.length > 0) {
    let addedItems = this._mainLibrary.batchCreateMediaItems(uriArray,
                                                             propertyArrayArray,
                                                             true);
    for (let i = 0; i < addedItems.length; i++) {
      items.push(addedItems.queryElementAt(i, Ci.sbIMediaItem));
    }
  }

  this._setDownloadDestinationIfNotSet(items, mfmFolders);
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

sbDownloadDeviceHelper.prototype.getDefaultDownloadFolder =
function sbDownloadDeviceHelper_getDefaultDownloadFolder(aContentType)
{
  var mediaDir = FolderUtils.getDownloadFolder(aContentType);
  // We should never get something bad here, but just in case...
  if (!folderIsValid(mediaDir)) {
    Cu.reportError("Desktop directory is not a directory!");
    throw Cr.NS_ERROR_FILE_NOT_DIRECTORY;
  }

  return mediaDir;
}

sbDownloadDeviceHelper.prototype.getDownloadFolder =
function sbDownloadDeviceHelper_getDownloadFolder(aContentType)
{
  const Application = Cc["@mozilla.org/fuel/application;1"].
                      getService(Ci.fuelIApplication);
  const prefs = Application.prefs;

  var downloadFolder;
  if (prefs.has(PREF_DOWNLOAD_FOLDER[aContentType])) {
    var downloadPath = prefs.get(PREF_DOWNLOAD_FOLDER[aContentType]).value;
    downloadFolder = makeFile(downloadPath);
  }

  const alwaysPrompt = prefs.getValue(PREF_DOWNLOAD_ALWAYSPROMPT[aContentType], false);
  if (folderIsValid(downloadFolder) && !alwaysPrompt) {
    return downloadFolder;
  }

  const sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
  const strings =
    sbs.createBundle("chrome://songbird/locale/songbird.properties");
  const title = strings.GetStringFromName(
                "prefs.main.downloads." + aContentType + ".chooseTitle");

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

  prefs.setValue(PREF_DOWNLOAD_FOLDER[aContentType], downloadFolder.path);
  return downloadFolder;
}

sbDownloadDeviceHelper.prototype._checkAndRemoveExistingItem =
function sbDownloadDeviceHelper__checkAndRemoveExistingItem(aMediaItem)
{
  var downloadMediaList = this.getDownloadMediaList();
  if (downloadMediaList) {
    var itemOriginUrl = aMediaItem.getProperty(SBProperties.originURL);

    var listEnumerationListener = {
      onEnumerationBegin: function() {
      },
      onEnumeratedItem: function(list, item) {
        downloadMediaList.remove(item);
      },
      onEnumerationEnd: function() {
      },
    };
    
    if ( (itemOriginUrl != "") &&
         (itemOriginUrl != null) ) {
      downloadMediaList.enumerateItemsByProperty(SBProperties.originURL,
                                                 itemOriginUrl,
                                                 listEnumerationListener);
    }
  }
}

sbDownloadDeviceHelper.prototype._getDownloadFolder_nothrow =
function sbDownloadDeviceHelper__getDownloadFolder_nothrow(aContentType)
{
  // This function returns null if the user cancels the dialog.
  try {
    return this.getDownloadFolder(aContentType);
  }
  catch (e if e.result == Cr.NS_ERROR_ABORT) {
  }
  return null;
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
    // Filter our enableAutoDownload, as the Auto Downloader
    // should ignore these items.  Their download is already
    // imminent:
    if (prop.id != SBProperties.contentSrc &&
        prop.id != SBProperties.enableAutoDownload)
    {
      dest.appendElement(prop, false);
    }
  }

  var target = aMediaItem.library.guid + "," + aMediaItem.guid;
  dest.appendProperty(SBProperties.downloadStatusTarget, target);

  aPropertyArrayArray.appendElement(dest, false);
}

sbDownloadDeviceHelper.prototype._setDownloadDestinationIfNotSet =
function sbDownloadDeviceHelper__setDownloadDestinationIfNotSet(
         aItems,
         aMediaFileManagerOptions)
{
  // Copy the Media File Manager options to an nsIPropertyBag
  var bag = Cc["@mozilla.org/hash-property-bag;1"]
              .createInstance(Ci.nsIWritablePropertyBag);
  for (prop in aMediaFileManagerOptions) {
    bag.setProperty(prop, aMediaFileManagerOptions[prop]);
  }
  
  // Use the Media File Manager to organize files into subfolders.
  // The caller will designate the root folder for each media type
  // in the MFM options bag (based on user preferences):
  var mediaFileMgr = Cc["@songbirdnest.com/Songbird/media-manager/file;1"]
                     .createInstance(Ci.sbIMediaFileManager);
  mediaFileMgr.init(bag);
  
  for each (var item in aItems) {
    try {
      var curDestination = item.getProperty(SBProperties.destination);
      if (!curDestination) {
        let destFile = mediaFileMgr.getManagedPath(
                                    item,
                                    _MFM_GET_MANAGED_PATH_OPTIONS);
        let path = destFile.path;
        
        /// @todo Video items tend to have meaningless file names, because
        ///       they lack a title property.  For now, strip off the leaf
        ///       name of unbetitled items, and
        //        sbDownloadSession::CompleteTransfer() will construct a
        //        file name from the source URL:
        if (!item.getProperty(SBProperties.title)) {
          // Leave a trailing delimiter so the path parses as a directory:
          path = path.replace(/[^/\\]*$/, "");
        }
        
        // Ensure all paths are lowercase for Windows.
        // See bug #7178.
        let actualDestination = makeFileURL(path);
        if(getPlatformString() == "Windows_NT") {
          actualDestination = actualDestination.toLowerCase();
        }
        
        item.setProperty(SBProperties.destination, actualDestination);
      }
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
