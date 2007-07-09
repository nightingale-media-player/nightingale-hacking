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

// Open functions
//
// This file is not standalone


try
{
  // TODO: Remove this
  var URL_PLAYLIST_DISPLAY = "chrome://songbird/content/xul/playlist_test2.xul?"

  function SBFileOpen( )
  {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                           .getService(Components.interfaces.sbIPlaylistPlayback);
    // Make a filepicker thingie
    var fp = Components.classes["@mozilla.org/filepicker;1"]
            .createInstance(Components.interfaces.nsIFilePicker);

    // get some text for the filepicker window
    var sel = "Select";
    try
    {
      sel = theSongbirdStrings.getString("faceplate.select");
    } catch(e) {}

    // initialize the filepicker with our text, a parent and the mode
    fp.init(window, sel, Components.interfaces.nsIFilePicker.modeOpen);

    // Tell it what filters to be using
    var mediafiles = "Media Files";
    try
    {
      mediafiles = theSongbirdStrings.getString("open.mediafiles");
    } catch(e) {}

    // ask the playback core for supported extensions
    var files = "";
    var eExtensions = PPS.getSupportedFileExtensions(); 
    while (eExtensions.hasMore()) {
      files += ( "*." + eExtensions.getNext() + "; ");
    }

    // add a filter to show only supported media files
    fp.appendFilter(mediafiles, files);

    // Show the filepicker
    var fp_status = fp.show();
    if ( fp_status == Components.interfaces.nsIFilePicker.returnOK )
    {
      // And if we're good, play it.
      seen_playing.boolValue = false;
      theTitleText.stringValue = fp.file.leafName;
      theArtistText.stringValue = "";
      theAlbumText.stringValue = "";

      // Use a nsIURI because it is safer and contains the scheme etc...
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var uri = ios.newFileURI(fp.file, null, null);
      
      // Import the item.
      var item = SBImportURLIntoMainLibrary(uri);
      
      var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                .getService(Components.interfaces.sbILibraryManager);

      // Display the propery view and play the newly imported item.
      SBDisplayViewForListAndPlayItem(libraryManager.mainLibrary, item);
    }
  }

  function SBUrlOpen( )
  {
    // Make a magic data object to get passed to the dialog
    var url_open_data = new Object();
    url_open_data.URL = URL.stringValue;
    url_open_data.retval = "";
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/open_url.xul", "open_url", "chrome,centerscreen", url_open_data ); 
    if ( url_open_data.retval == "ok" )
    {
      var library = SBGetWebLibrary();
      var item = SBImportURLIntoWebLibrary(url_open_data.URL);

      // And if we're good, play it.
      theTitleText.stringValue = url_open_data.URL;
      theArtistText.stringValue = "";
      theAlbumText.stringValue = "";
      
      SBDisplayViewForListAndPlayItem(library, item);
    }  
  }

  function SBPlaylistOpen()
  {
    try
    {
      var aPlaylistReaderManager =
        Components.classes["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                  .getService(Components.interfaces.sbIPlaylistReaderManager);

      // Make a filepicker thingie
      var nsIFilePicker = Components.interfaces.nsIFilePicker;
      var fp = Components.classes["@mozilla.org/filepicker;1"]
              .createInstance(nsIFilePicker);
      var sel = "Open Playlist";
      try
      {
        sel = theSongbirdStrings.getString("faceplate.open.playlist");
      } catch(e) {}
      fp.init(window, sel, nsIFilePicker.modeOpen);
      
      // Tell it what filters to be using
      var filterlist = "";
      var extensionCount = new Object;
      var extensions = aPlaylistReaderManager.supportedFileExtensions(extensionCount);
      
      var first = true;
      for(var i = 0; i < extensions.length; i++)
      {
        var ext_list = extensions[i].split(",");
        for(var j = 0; j < ext_list.length; j++)
        {
          var ext = ext_list[j];
          if (ext.length > 0) {
            if (!first) // skip the first one
              filterlist += ";";
            first = false;
            filterlist += "*." + ext;
          }
        }
      }
      
      var playlistfiles = "Playlist Files";
      try
      {
        playlistfiles = theSongbirdStrings.getString("open.playlistfiles");
      } catch(e) {}
      fp.appendFilter(playlistfiles, filterlist);
      
      // Show it
      var fp_status = fp.show();
      if ( fp_status == nsIFilePicker.returnOK )
      {
        var library = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                .getService(Components.interfaces.sbILibraryManager).mainLibrary;

        // Create the media list
        var mediaList = library.createMediaList("simple");
        mediaList.name = fp.file.leafName;
        mediaList.setProperty("http://songbirdnest.com/data/1.0#originURL", fp.fileURL.spec);

        aPlaylistReaderManager.originalURI = fp.fileURL;
        var success = aPlaylistReaderManager.loadPlaylist(fp.fileURL, mediaList, null, false, null);
        if (success == 1) {
          var array = Components.classes["@mozilla.org/array;1"]
                                .createInstance(Components.interfaces.nsIMutableArray);
          for (var i = 0; i < mediaList.length; i++) {
            array.appendElement(mediaList.getItemByIndex(i), false);
          }

          // Send the items in the new media list to the metadata scanner
          var metadataJobManager =
            Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                      .getService(Components.interfaces.sbIMetadataJobManager);
          var metadataJob = metadataJobManager.newJob(array, 5);

          // Give the new media list focus
          if (gBrowser) {
            var librarySPS =
              Components.classes['@songbirdnest.com/servicepane/library;1']
                        .getService(Components.interfaces.sbILibraryServicePaneService);
            var node = librarySPS.getNodeForLibraryResource(mediaList);
            if (node) {
              gBrowser.loadURI(node.url);
            }
          }
        }

      }
    }
    catch(err)
    {
      alert(err);
    }
  }

  function log(str)
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage( str );
  }

  function SBUrlExistsInDatabase( the_url )
  {
    var retval = false;
    try
    {
      aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"];
      if (aDBQuery)
      {
        aDBQuery = aDBQuery.createInstance();
        aDBQuery = aDBQuery.QueryInterface(Components.interfaces.sbIDatabaseQuery);
        
        if ( ! aDBQuery )
        {
          return false;
        }
        
        aDBQuery.setAsyncQuery(false);
        aDBQuery.setDatabaseGUID("testdb-0000");
        aDBQuery.addQuery('select * from test where url="' + the_url + '"' );
        var ret = aDBQuery.execute();
        
        resultset = aDBQuery.getResultObject();
       
        // we didn't find anything that matches our url
        if ( resultset.getRowCount() != 0 )
        {
          retval = true;
        }
      }
    }
    catch(err)
    {
      alert(err);
    }
    return retval;
  }


// Help
function onHelp()
{
  var helpitem = document.getElementById("help.topics");
  onMenu(helpitem);
}



function SBOpenPreferences(paneID)
{
  // On all systems except Windows pref changes should be instant.
  //
  // In mozilla this is the browser.prefereces.instantApply pref,
  // and is set at compile time.
  var instantApply = navigator.userAgent.indexOf("Windows") == -1;
	
  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:Preferences");
  if (win) {
    win.focus();
    if (paneID) {
      var pane = win.document.getElementById(paneID);
      win.document.documentElement.showPane(pane);
    }
  }
  else
    openDialog("chrome://browser/content/preferences/preferences.xul", "Preferences", features, paneID);
    
  // to open connection settings only:
  // SBOpenModalDialog("chrome://browser/content/preferences/connection.xul", "chrome,centerscreen", null); 
}

function SBSetDownloadFolder()
{
  // Just open the window, we don't care what the user does in it.
  SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,centerscreen", null ); 
}

/*function SBOpenDownloadManager()
{
  var dlmgr = Components.classes['@mozilla.org/download-manager;1'].getService();
  dlmgr = dlmgr.QueryInterface(Components.interfaces.nsIDownloadManager);

  var windowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  windowMediator = windowMediator.QueryInterface(Components.interfaces.nsIWindowMediator);

  var dlmgrWindow = windowMediator.getMostRecentWindow("Download:Manager");
  if (dlmgrWindow) {
    dlmgrWindow.focus();
  }
  else {
    openDialog("chrome://mozapps/content/downloads/downloads.xul", "Download:Manager", "chrome,centerscreen", null);
   }
}*/

function SBWatchFolders()
{
  SBOpenModalDialog( "chrome://songbird/content/xul/watch_folders.xul", "", "chrome,centerscreen", null ); 
}

var theFileScanIsOpen = SB_NewDataRemote( "media_scan.open", null );
function SBScanMedia( )
{
  theFileScanIsOpen.boolValue = true;
  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  const CONTRACTID_FILE_PICKER = "@mozilla.org/filepicker;1";
  var fp = Components.classes[CONTRACTID_FILE_PICKER].createInstance(nsIFilePicker);
  var welcome = "Welcome";
  var scan = "Scan";
  try
  {
    welcome = theSongbirdStrings.getString("faceplate.welcome");
    scan = theSongbirdStrings.getString("faceplate.scan");
  } catch(e) {}
  if (getPlatformString() == "Darwin") {
    fp.init( window, scan, nsIFilePicker.modeGetFolder );
    var defaultDirectory =
    Components.classes["@mozilla.org/file/directory_service;1"]
              .getService(Components.interfaces.nsIProperties)
              .get("Home", Components.interfaces.nsIFile);
    defaultDirectory.append("Music");
    fp.displayDirectory = defaultDirectory;
  } else {
    fp.init( window, welcome + "\n\n" + scan, nsIFilePicker.modeGetFolder );
  }
  var res = fp.show();
  if ( res == nsIFilePicker.returnOK )
  {
    var media_scan_data = new Object();
    media_scan_data.URL = fp.file.path;
    media_scan_data.retval = "";
    // Open the modal dialog
    //SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,centerscreen", media_scan_data ); 
    SBOpenWindow( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,centerscreen", media_scan_data ); 
  }
  theFileScanIsOpen.boolValue = false;
}

function SBMabOpen()
{
  var mab_data = new Object();
  mab_data.retval = "";
  
  // Open the non modal dialog
  SBOpenWindow( "chrome://songbird/content/xul/mab.xul", "Mozilla Amazon Browser", "chrome", mab_data ); 
}


/** Legacy function **/
function SBNewPlaylist()
{
  return makeNewPlaylist("simple"); 
}

function SBNewSmartPlaylist()
{
  return makeNewPlaylist("smart"); 
}

/**
 * Create a new playlist of the given type, using the service pane
 * to determine context and perform renaming
 *
 * Note: This function should move into the window controller somewhere
 *       once it exists.
 */
function makeNewPlaylist(mediaListType) {
  var servicePane = gServicePane;

  // Try to find the currently selected service pane node
  var selectedNode;
  if (servicePane) {
    selectedNode = servicePane.getSelectedNode();
  }
  
  // Ask the library service pane provider to suggest where
  // a new playlist should be created
  var librarySPS = Components.classes['@songbirdnest.com/servicepane/library;1']
                             .getService(Components.interfaces.sbILibraryServicePaneService);
  var library = librarySPS.suggestLibraryForNewList(mediaListType, selectedNode);
  
  // Looks like no libraries support the given mediaListType
  if (!library) {
    throw("Could not find a library supporting lists of type " + mediaListType); 
  }
  
  // Create the playlist
  var mediaList = library.createMediaList(mediaListType);
  
  // Give the playlist a default name
  // TODO: Localization should be done internally
  mediaList.name = SBString("playlist", "Playlist");

  // If we have a servicetree, tell it to make the new playlist node editable
  if (servicePane) {
    // Find the servicepane node for our new medialist
    var node = librarySPS.getNodeForLibraryResource(mediaList);
    
    if (node) {
      // Ask the service pane to start editing our new node
      // so that the user can give it a name
      servicePane.startEditingNode(node);
    } else {
      throw("Error: Couldn't find a service pane node for the list we just created\n");
    }

  // Otherwise pop up a dialog and ask for playlist name
  } else {
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"  ]
                                  .getService(Components.interfaces.nsIPromptService);

    var input = {value: mediaList.name};
    var title = SBString("newPlaylist.title", "Create New Playlist");
    var prompt = SBString("newPlaylist.prompt", "Enter the name of the new playlist.");

    if (promptService.prompt(window, title, prompt, input, null, {})) {
      mediaList.name = input.value;
    }
  }
  return mediaList;
}


function SBKoshiOpen()
{
  // Make a magic data object to get passed to the dialog
  var koshi_data = new Object();
  koshi_data.retval = "";
  // Open the window
  SBOpenModalDialog( "chrome://songbird/content/xul/koshi_test.xul", "", "chrome,centerscreen", koshi_data ); 
}

function SBExtensionsManagerOpen()
{
  const EM_TYPE = "Extension:Manager";
  
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theEMWindow = wm.getMostRecentWindow(EM_TYPE);
  if (theEMWindow) {
    theEMWindow.focus();
    return;
  }
  
  const EM_URL = "chrome://mozapps/content/extensions/extensions.xul?type=extensions";
  const EM_FEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
  window.openDialog(EM_URL, "", EM_FEATURES);
}

function SBTrackEditorOpen()
{
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theTE = wm.getMostRecentWindow("track_editor");
  if (theTE) {
    theTE.focus();
  } else {
    const TEURL = "chrome://songbird/content/xul/trackeditor.xul";
    const TEFEATURES = "chrome,dialog=no,resizable=no";
    SBOpenWindow(TEURL, "track_editor", TEFEATURES, gBrowser.currentPlaylist); 
  }
}

function SBSubscribe(mediaList, defaultUrl)
{
  // Make sure the argument is a dynamic media list
  if (mediaList) {
    if (!(mediaList instanceof Components.interfaces.sbIMediaList))
      throw Components.results.NS_ERROR_INVALID_ARG;

    var isSubscription =
      mediaList.getProperty("http://songbirdnest.com/data/1.0#isSubscription");
    if (isSubscription != "1")
      throw Components.results.NS_ERROR_INVALID_ARG;
  }

  if (defaultUrl && !(defaultUrl instanceof Components.interfaces.nsIURI))
    throw Components.results.NS_ERROR_INVALID_ARG;

  var params = Components.classes["@mozilla.org/array;1"]
                         .createInstance(Components.interfaces.nsIMutableArray);
  params.appendElement(mediaList, false);
  params.appendElement(defaultUrl, false);

  // Open the window
  SBOpenModalDialog("chrome://songbird/content/xul/subscribe.xul",
                    "",
                    "chrome,centerscreen",
                    params); 
}

// TODO: This function should be renamed.  See openAboutDialog in browserUtilities.js
function About( )
{
  // Make a magic data object to get passed to the dialog
  var about_data = new Object();
  about_data.retval = "";
  // Open the modal dialog
  SBOpenModalDialog( "chrome://songbird/content/xul/about.xul", "about", "chrome,centerscreen", about_data ); 
  if ( about_data.retval == "ok" )
  {
  }  
}

function SBNewFolder() {
  var servicePane = gServicePane;

  // Try to find the currently selected service pane node
  var selectedNode;
  if (servicePane) {
    selectedNode = servicePane.getSelectedNode();
  }
  
  // The bookmarks service knows how to make folders...
  var bookmarks = Components.classes['@songbirdnest.com/servicepane/bookmarks;1']
      .getService(Components.interfaces.sbIBookmarks);
  
  // ask the bookmarks service to make a new folder
  var folder = bookmarks.addFolder(SBString('bookmarks.newfolder.defaultname',
                                            'New Folder'));
 
  // start editing the new folder
  if (gServicePane) {
    // we can find the pane so we can edit it inline
    gServicePane.startEditingNode(folder);
  } else {
    // or not - let's pop a dialog
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"  ]
                                  .getService(Components.interfaces.nsIPromptService);

    var input = {value: folder.name};
    var title = SBString("bookmarks.newfolder.title", "Create New Playlist");
    var prompt = SBString("bookmarks.newfolder.prompt", "Enter the name of the new playlist.");

    if (promptService.prompt(window, title, prompt, input, null, {})) {
      folder.name = input.value;
    }
  }
  return folder;
}


/**
 * Opens the update manager and checks for updates to the application.
 */
function checkForUpdates()
{
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);
  var prompter = 
      Components.classes["@mozilla.org/updates/update-prompt;1"].
      createInstance(Components.interfaces.nsIUpdatePrompt);

  // If there's an update ready to be applied, show the "Update Downloaded"
  // UI instead and let the user know they have to restart the browser for
  // the changes to be applied. 
  if (um.activeUpdate && um.activeUpdate.state == "pending")
    prompter.showUpdateDownloaded(um.activeUpdate);
  else
    prompter.checkForUpdates();
}

function buildHelpMenu()
{
  var updates = 
      Components.classes["@mozilla.org/updates/update-service;1"].
      getService(Components.interfaces.nsIApplicationUpdateService);
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);

  // Disable the UI if the update enabled pref has been locked by the 
  // administrator or if we cannot update for some other reason
  var checkForUpdates = document.getElementById("updateCmd");
  var canUpdate = updates.canUpdate;
  checkForUpdates.setAttribute("disabled", !canUpdate);
  if (!canUpdate)
    return; 

  var strings = document.getElementById("songbird_strings");
  var activeUpdate = um.activeUpdate;
  
  // If there's an active update, substitute its name into the label
  // we show for this item, otherwise display a generic label.
  function getStringWithUpdateName(key) {
    if (activeUpdate && activeUpdate.name)
      return strings.getFormattedString(key, [activeUpdate.name]);
    return strings.getString(key + "Fallback");
  }
  
  // By default, show "Check for Updates..."
  var key = "default";
  if (activeUpdate) {
    switch (activeUpdate.state) {
    case "downloading":
      // If we're downloading an update at present, show the text:
      // "Downloading Firefox x.x..." otherwise we're paused, and show
      // "Resume Downloading Firefox x.x..."
      key = updates.isDownloading ? "downloading" : "resume";
      break;
    case "pending":
      // If we're waiting for the user to restart, show: "Apply Downloaded
      // Updates Now..."
      key = "pending";
      break;
    }
  }
  checkForUpdates.label = getStringWithUpdateName("updateCmd_" + key);
}

function clearPrivateData() {
  // todo: implement sanitize.js
}

function javascriptConsole() {
  window.open("chrome://global/content/console.xul", "global:console", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar,titlebar");
}

// Match filenames ending with .xpi or .jar
function isXPI(filename) {
  return /\.(xpi|jar)$/i.test(filename);
}

// Prompt the user to install the given XPI.
function installXPI(filename) {    
  xpinstallObj = {};
  xpinstallObj[filename] = filename;
  InstallTrigger.install(xpinstallObj);
}

// Library Utilities

/**
 * \brief Get the web library from the library manager.
 */
function SBGetWebLibrary() {
  var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                  .getService(Components.interfaces.sbILibraryManager);

  var prefsService = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefService);
    
  var prefBranch = prefsService.getBranch("songbird.library.")
                      .QueryInterface(Ci.nsIPrefBranch);
  
  var webLibrary = prefBranch.getCharPref("web");
  
  return libraryManager.getLibrary(webLibrary);
}

/**
 * \brief Import a URL into the main library.
 * \param url URL of item to import, also accepts nsIURI's.
 * \return The media item that was created.
 * \retval null Error during creation of item.
 */
function SBImportURLIntoMainLibrary(url) {
  var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                  .getService(Components.interfaces.sbILibraryManager);
                                  
  var library = libraryManager.mainLibrary;

  if (getPlatformString() == "Windows_NT") url = url.toLowerCase();

  
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);

  var uri = null;
  try {
    if( typeof(url.spec) == "undefined" ) {
      uri = ioService.newURI(url, null, null);
    }
    else {
      uri = url;
    }
  }
  catch (e) {
    log(e);
    uri = null;
  }
      
  if(!uri) {
    return null;
  }
  
  // skip import of the item if it already exists
  var mediaItem = getFirstItemByProperty(library, "http://songbirdnest.com/data/1.0#contentURL", url);
  if (mediaItem) 
    return null;
  
  try {
    mediaItem = library.createMediaItem(uri);
  }
  catch(e) {
    log(e);
    mediaItem = null;
  }
  
  if(!mediaItem) {
    return null;
  }
  
  var metadataJobMgr = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
    .getService(Components.interfaces.sbIMetadataJobManager);
  
  var items = Components.classes["@mozilla.org/array;1"]
    .createInstance(Components.interfaces.nsIMutableArray);
  
  items.appendElement(mediaItem, false);
  metadataJobMgr.newJob(items, 5);
  
  return mediaItem;
}

function SBImportURLIntoWebLibrary(url) {
  var library = SBGetWebLibrary();
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);

  var uri = null;
  try {
    if( typeof(url.spec) == "undefined" ) {
      uri = ioService.newURI(url, null, null);
    }
    else {
      uri = url;
    }
  }
  catch (e) {
    log(e);
    uri = null;
  }
      
  if(!uri) {
    return null;
  }
  
  var mediaItem = null;
  try {
    mediaItem = library.createMediaItem(uri);
  }
  catch(e) {
    log(e);
    mediaItem = null;
  }
  
  if(!mediaItem) {
    return null;
  }
  
  var metadataJobMgr = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
    .getService(Components.interfaces.sbIMetadataJobManager);
  
  var items = Components.classes["@mozilla.org/array;1"]
    .createInstance(Components.interfaces.nsIMutableArray);
  
  items.appendElement(mediaItem, false);
  metadataJobMgr.newJob(items, 5);
  
  return mediaItem;
}

function SBDisplayViewForListAndPlayItem(list, item) {
  var view = list.createView();
  var index = list.indexOf(item, 0);

  // Get the tabbed browser, load new library view, unfiltered.
  gBrowser.loadMediaList(list, null, null, null);
  
  // Play the item that was just added.
  gPPS.playView(view, index);
}
  
function getFirstItemByProperty(aMediaList, aProperty, aValue) {

  var listener = {
    item: null,
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.item = item;
      return false;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  aMediaList.enumerateItemsByProperty(aProperty,
                                      aValue,
                                      listener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  return listener.item;
}


}
catch (e)
{
  alert(e);
}




