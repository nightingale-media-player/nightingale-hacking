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
      PPS.playAndImportURL(uri.spec);

      // put the url into the search widget history???
      if (document.__SEARCHWIDGET__) document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
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
      // And if we're good, play it.
      theTitleText.stringValue = url_open_data.URL;
      theArtistText.stringValue = "";
      theAlbumText.stringValue = "";
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.playAndImportURL(url_open_data.URL);
      if (document.__SEARCHWIDGET__) document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
    }  
  }

  function SBPlaylistOpen()
  {
    try
    {
      var aPlaylistReaderManager = (new sbIPlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);
      
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
        SBScanServiceTreeNewEntryEditable(); // Do this right before you add to the servicelist?

        // And if we're good, play it.
        var plsFile = "file:///" + fp.file.path;
        var readableName = fp.file.leafName;
        var success = aPlaylistReaderManager.autoLoad(fp.fileURL.spec, "songbird", readableName, "user", plsFile, "", null);
        
        if ( ( success == true ) || ( success == 1 ) )
        {
          SBScanServiceTreeNewEntryStart(); // Do this right after you know you have added to the servicelist?  
        }

        if (theLibraryPlaylist) theLibraryPlaylist.syncPlaylistIndex(true);
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


  function SBPlayPlaylistIndex( index, playlist )
  {
    try
    {
      if (!playlist) playlist = document.__CURRENTPLAYLIST__;
      if (!playlist) playlist = document.__CURRENTWEBPLAYLIST__;
      if (!playlist) return;
      
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.playRef(playlist.ref, index);
    }
    catch ( err )
    {
      alert( err );
    }
  }
  
function switchFeathers(aFeathersName)
{
  // Figure out if we're being asked to switch to what we already are
  var currentFeathers = getPref("getCharPref", PREF_FEATHERS_SELECTED,
                                FEATHERS_DEFAULT_NAME);
  if (currentFeathers == aFeathersName)
    return;

  // Change the feathers (XUL skin) -- this only changes colors on the
  // currently loaded windows, not images. Hence the magic below.
  setPref("setCharPref", PREF_FEATHERS_SELECTED, aFeathersName);  

  // Get mainwin URL
  var mainWinURL = getPref("getCharPref", PREF_BONES_SELECTED,
                           BONES_DEFAULT_URL);
  
  // Save our current values before flipping out.
  onWindowSaveSizeAndPosition();
  
  // Open the new window

  var chromeFeatures = "chrome,modal=no,toolbar=no,popup=no";
  // can't test with accessibility.enabled here because the value hasn't been set yet (will be set after SBInitialize on the new window)
  if (aFeathersName.indexOf("/plucked") < 0) chromeFeatures += ",titlebar=no"; else chromeFeatures += ",resizable=yes";

  var newMainWin = window.open(mainWinURL, "", chromeFeatures);
  newMainWin.focus();

  // Kill this window
  onExit(true);
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

var theMediaScanIsOpen = SB_NewDataRemote( "media_scan.open", null );
function SBScanMedia( )
{
  theMediaScanIsOpen.boolValue = true;
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
    SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,centerscreen", media_scan_data ); 
  }
  theMediaScanIsOpen.boolValue = false;
}

function SBMabOpen()
{
  var mab_data = new Object();
  mab_data.retval = "";
  
  // Open the non modal dialog
  SBOpenWindow( "chrome://songbird/content/xul/mab.xul", "Mozilla Amazon Browser", "chrome", mab_data ); 
}

function SBNewPlaylist()
{
  try
  {
    SBScanServiceTreeNewEntryEditable();
    var query = new sbIDatabaseQuery();
    query.setDatabaseGUID( "songbird" );
    var playlistmanager = new sbIPlaylistManager();
    var aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
    var playlistguid = aUUIDGenerator.generateUUID();
    var name = "Playlist";
    try
    {
      name = theSongbirdStrings.getString("playlist");
    } catch(e) {}
    var playlist = playlistmanager.createPlaylist( playlistguid, name, "Playlist", "user", query );
    SBScanServiceTreeNewEntryStart();
  }
  catch ( err )
  {
    alert( "SBNewPlaylist - " + err );
  }
}

function SBMiniplayerOpen()
{
  // Get miniplayer URL
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var miniwin = "chrome://rubberducky/content/xul/miniplayer.xul";
  try {
    miniwin = prefs.getCharPref("general.bones.selectedMiniPlayerURL", miniwin);  
  } catch (err) {}

  // Open the window
  window.open( miniwin, "", "chrome,titlebar=no,resizable=no" );
  onExit();
}

function SBNewSmartPlaylist( guid, table )
{
  SBScanServiceTreeNewEntryEditable(); // Do this right before you add to the servicelist?
  
  // Make a magic data object to get passed to the dialog
  var smart_playlist = new Object();
  smart_playlist.retval = "";
  smart_playlist.guid = guid;
  smart_playlist.table = table
  // Open the window
  SBOpenModalDialog( "chrome://songbird/content/xul/smart_playlist.xul", "", "chrome,centerscreen", smart_playlist ); 
  
  if ( smart_playlist.retval == "ok" )
  {
    SBScanServiceTreeNewEntryStart(); // And this once you know you really did?
  }
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
  // for now only allow editing the main db
  if (!theLibraryPlaylist || theLibraryPlaylist.guid != "songbird") return;
  
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theTE = wm.getMostRecentWindow("track_editor");
  if (theTE) {
    theTE.focus();
  } else {
    const TEURL = "chrome://songbird/content/xul/trackeditor.xul";
    const TEFEATURES = "chrome,dialog=no,resizable=no";
    SBOpenWindow(TEURL, "track_editor", TEFEATURES, document); 
  }
}


function SBSubscribe( url, guid, table, readable_name )
{
  // Make a magic data object to get passed to the dialog
  var subscribe_data = new Object();
  subscribe_data.retval = "";
  subscribe_data.url = url;
  subscribe_data.readable_name = readable_name;

  // if we have a table and guid, we're editing an existing playlist
  // so we need to populate the edit dialog with the existing data.
  if (table && guid) {
    var playlistManager = new sbIPlaylistManager();
    var dbQuery = new sbIDatabaseQuery();

    dbQuery.setAsyncQuery(false);
    dbQuery.setDatabaseGUID(guid);

    var playlist = playlistManager.getDynamicPlaylist(table, dbQuery);
    if (playlist) {
      // if the playlist exists pull the data from it and mark ourself as
      // editing the existing playlist
      subscribe_data.time = playlist.getPeriodicity();
      subscribe_data.table = table;
      subscribe_data.guid = guid;
      subscribe_data.edit = true;
    }
  }

  // snapshot the service tree so we can find the added playlist
  SBScanServiceTreeNewEntryEditable();

  // Open the window
  SBOpenModalDialog( "chrome://songbird/content/xul/subscribe.xul", "", "chrome,centerscreen", subscribe_data ); 
  if ( subscribe_data.retval == "ok" && !subscribe_data.edit )
  {
    // if we are not editing an existing playlist open the edit box
    SBScanServiceTreeNewEntryStart();
  }
}

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
  SBOpenWindow("chrome://global/content/console.xul", "global:console", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar,titlebar");
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

  
}
catch (e)
{
  alert(e);
}




