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

// For Songbird properties.
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");


// Open functions
//
// This file is not standalone

try
{
  function _SBShowMainLibrary()
  {
    // Make sure the main library is viewed
    var browser = SBGetBrowser();
    if (!browser) {
      Components.utils.reportError("_SBShowMainLibrary - Cannot view the main library without a browser object");
    }
    browser.loadMediaList(LibraryUtils.mainLibrary);
  }
    
  function _SBGetCurrentView()
  {
    var browser = SBGetBrowser();
    var mediaPage = browser.currentMediaPage;
    if (!mediaPage) {
      Components.utils.reportError("_SBGetCurrentView - Cannot return a view without a mediaPage");
      return;
    }
    var view = mediaPage.mediaListView;
    if (!view) {
      Components.utils.reportError("_SBGetCurrentView - Cannot return a view");
      return;
    }
    return view;
  }

  function SBFileOpen( )
  {
    // Make a filepicker thingie.
    var nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
            .createInstance(nsIFilePicker);

    // Get some text for the filepicker window.
    var sel = SBString("faceplate.select", "Select", theSongbirdStrings);

    // Initialize the filepicker with our text, a parent and the mode.
    fp.init(window, sel, nsIFilePicker.modeOpen);

    var mediafiles = SBString("open.mediafiles", "Media Files", theSongbirdStrings);

    // ask the playback core for supported extensions
    var extensions = gTypeSniffer.mediaFileExtensions;
    if (!Application.prefs.getValue("songbird.mediascan.enableVideo", false)) {
      // disable video, so scan only audio - see bug 13173
      extensions = gTypeSniffer.audioFileExtensions;
    }
    var exts = ArrayConverter.JSArray(extensions);

    // map ["mp3", "ogg"] to ["*.mp3", "*.ogg"]
    var filters = exts.map(function(x){return "*." + x});

    // Add a filter to show all files.
    fp.appendFilters(nsIFilePicker.filterAll);
    // Add a filter to show only HTML files.
    fp.appendFilters(nsIFilePicker.filterHTML);
    // Add a filter to show only supported media files.
    fp.appendFilter(mediafiles, filters.join(";"));

    // Add a new filter for each and every file extension.
    var filetype;
    for (i in exts)
    {
      filetype = SBFormattedString("open.filetype",
                                   [exts[i].toUpperCase(), filters[i]]);
      fp.appendFilter(filetype, filters[i]);
    }

    // Show the filepicker
    var fp_status = fp.show();
    if ( fp_status == nsIFilePicker.returnOK )
    {
      // Use a nsIURI because it is safer and contains the scheme etc...
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var uri = ios.newFileURI(fp.file, null, null);
      
      // Linux specific hack to be able to read badly named files (bug 6227)
      // nsIIOService::newFileURI actually forces to be valid UTF8 - which isn't
      // correct if the file on disk manages to have an incorrect name
      // note that Mac OSX has a different persistentDescriptor
      if (fp.file instanceof Components.interfaces.nsILocalFile) {
        switch(getPlatformString()) {
          case "Linux":
            var spec = "file://" + escape(fp.file.persistentDescriptor);
            uri = ios.newURI(spec, null, null);
        }
      }

      // See if we're asking for an extension
      if ( isXPI( uri.spec ) )
      {
        installXPI( uri.spec );
      }
      else if ( gTypeSniffer.isValidMediaURL(uri) )
      {
        if (fp.file instanceof Components.interfaces.nsILocalFile) {
          if (!fp.file.exists()) {
            gBrowser.addTab(uri.spec);
            return;
          }
        }

        // And if we're good, play it.
        SBDataSetStringValue("metadata.title", fp.file.leafName);
        SBDataSetStringValue("metadata.artist", "");
        SBDataSetStringValue("metadata.album", "");

        // Display the main library
        _SBShowMainLibrary();
                
        // Import the item.
        var view = _SBGetCurrentView();
        
        var item = SBImportURLIntoMainLibrary(uri);
        // Wait for the item to show up in the view before trying to play it
        // and give it time to sort (given 10 tracks per millisecond)
        var interval = setInterval(function() {
          // If the view has been updated then we're good to go
          var index;
          try {
            index = view.getIndexForItem(item);
          }
          catch (e) {
            // If we get anything but not available then that's bad and we need to bail
            if (Components.lastResult == Components.results.NS_ERROR_NOT_AVAILABLE) {
              // It's not there so wait for the next interval
              return;
            }
            // Unexpected error, cancel the interval and rethrow
            clearInterval(interval);
            throw e;
          }
          clearInterval(interval);
          // If we have a browser, try to show the view
          if (window.gBrowser) {
            gBrowser.showIndexInView(view, index);
          }
          index = view.getIndexForItem(item);
          // Play the item
          gMM.sequencer.playView(view, index);
        }, 500);
      }
      else
      {
        // Unknown type, let the browser figure out what the best course
        // of action is.
        gBrowser.loadURI( uri.spec );
      }
    }
  }

  function MediaUriCheckerObserver(uri) {
    this._uri = uri;
  }

  MediaUriCheckerObserver.prototype.onStartRequest =
  function MediaUriCheckerObserver_onStartRequest(aRequest, aContext)
  {
  }

  MediaUriCheckerObserver.prototype.onStopRequest =
  function MediaUriCheckerObserver_onStopRequest(aRequest, aContext, aStatusCode)
  {
    if (!Components.isSuccessCode(aStatusCode)) {
      var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                             .getService(Ci.sbILibraryManager);

      library = libraryManager.mainLibrary;
      var mediaItem =
            getFirstItemByProperty(library,
                                   "http://songbirdnest.com/data/1.0#contentURL",
                                   this._uri.spec);
      if (mediaItem)
        library.remove(mediaItem);

      gBrowser.addTab(this._uri.spec);
    }

    return;
  }

  function SBUrlOpen( parentWindow )
  {
    // Make a magic data object to get passed to the dialog
    var url_open_data = new Object();
    url_open_data.URL = SBDataGetStringValue("faceplate.play.url");
    url_open_data.retval = "";
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/openURL.xul", "open_url", "chrome,centerscreen", url_open_data, parentWindow );
    if ( url_open_data.retval == "ok" )
    {
      var library = LibraryUtils.webLibrary;

     // Use a nsIURI because it is safer and contains the scheme etc...
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var url = url_open_data.URL;
      var uri = null;
      var isLocal = false;

      if (getPlatformString() == "Windows_NT") {
        // Handle the URL starts with something like "c:\"
        isLocal = /^.:/.test(url);
      } else {
        // URL starts with "/" on MacOS/Linux/OpenSolaris
        isLocal = (url[0] == '/');
      }

      try {
        if (isLocal) {
          var file = Components.classes["@mozilla.org/file/local;1"]
                               .createInstance(Ci.nsILocalFile);
          file.initWithPath(url);
          uri = ios.newFileURI(file);
        } else {
          uri = ios.newURI(url, null, null);
        }
      }
      catch(e) {
        // Bad URL :(
        Components.utils.reportError(e);
        return;
      }

      // See if we're asking for an extension
      if ( isXPI( uri.spec ) )
      {
        installXPI( uri.spec );
      }
      else if ( gTypeSniffer.isValidMediaURL(uri) )
      {
        if (uri.scheme == "file") {
          var pFileURL = uri.QueryInterface(Ci.nsIFileURL);
          if (pFileURL.file && !pFileURL.file.exists()) {
            gBrowser.addTab(uri.spec);
            return;
          }
        }

        if (uri.scheme == "http") {
          var checker = Cc["@mozilla.org/network/urichecker;1"]
            .createInstance(Ci.nsIURIChecker);
          checker.init(uri);
          checker.asyncCheck(new MediaUriCheckerObserver(uri), null);
        }

        var item = null;

        // Doesn't import local file to the web Library
        if (uri.scheme != "file")
          item = SBImportURLIntoWebLibrary(uri);

        // Display the main library
        _SBShowMainLibrary();

        var view = _SBGetCurrentView();
        var targetLength = view.length + 1;

        // Import the item.
        item = SBImportURLIntoMainLibrary(uri);

        // Wait for the item to show up in the view before trying to play it
        // and give it time to sort (given 10 tracks per millisecond)
        var interval = setInterval(function() {
          // If the view has been updated then we're good to go
          if (view.length == targetLength) {
            clearInterval(interval);
            
            // show the view and play
            var index = view.getIndexForItem(item);
        
            // If we have a browser, try to show the view
            if (window.gBrowser) {
              gBrowser.showIndexInView(view, index);
            }
        
            // Play the item
            gMM.sequencer.playView(view, index);
          }
        }, 500);
      }
      else
      {
        // Unknown type, let the browser figure out what the best course
        // of action is.
        gBrowser.loadURI( uri.spec );
      }
    }
  }

  function SBPlaylistOpen()
  {
    try
    {
      var aPlaylistReaderManager =
        Components.classes["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                  .getService(Components.interfaces.sbIPlaylistReaderManager);

      // Make a filepicker thingie.
      var nsIFilePicker = Components.interfaces.nsIFilePicker;
      var fp = Components.classes["@mozilla.org/filepicker;1"]
                         .createInstance(nsIFilePicker);

      // Get some text for the filepicker window.
      var sel = SBString("faceplate.open.playlist", "Open Playlist",
                         theSongbirdStrings);

      // Initialize the filepicker with our text, a parent and the mode.
      fp.init(window, sel, nsIFilePicker.modeOpenMultiple);

      var playlistfiles = SBString("open.playlistfiles", "Playlist Files",
                                   theSongbirdStrings);

      var ext;
      var exts = new Array();
      var extensionCount = new Object;
      var extensions = aPlaylistReaderManager.supportedFileExtensions(extensionCount);

      for (i in extensions)
      {
        ext = extensions[i];
        if (ext)
          exts.push(ext);
      }

      // map ["m3u", "pls"] to ["*.m3u", "*.pls"]
      var filters = exts.map(function(x){return "*." + x});

      // Add a filter to show all files.
      fp.appendFilters(nsIFilePicker.filterAll);
      // Add a filter to show only supported playlist files.
      fp.appendFilter(playlistfiles, filters.join(";"));

      // Add a new filter for each and every file extension.
      var filetype;
      for (i in exts)
      {
        filetype = SBFormattedString("open.filetype", 
                                     [exts[i].toUpperCase(), filters[i]]);
        fp.appendFilter(filetype, filters[i]);
      }

      // Show it
      var fp_status = fp.show();
      if ( fp_status == nsIFilePicker.returnOK )
      {
        var aFile, aURI, allFiles = fp.files;
        var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService)
        while(allFiles.hasMoreElements()) {
          aFile = allFiles.getNext().QueryInterface(Components.interfaces.nsIFile);
          aURI = ioService.newFileURI(aFile);
          SBOpenPlaylistURI(aURI, aFile.leafName);
        }
      }
    }
    catch(err)
    {
      alert(err);
    }
  }
  
  function SBOpenPlaylistURI(aURI, aName) {
    var uri = aURI;
    if(!(aURI instanceof Components.interfaces.nsIURI)) {
      uri = newURI(aURI);
    }
    var name = aName;
    if (!aName) {
      name = uri.path;
      var p = name.lastIndexOf(".");
      if (p != -1) name = name.slice(0, p);
      p = name.lastIndexOf("/");
      if (p != -1) name = name.slice(p+1);
    }
    var aPlaylistReaderManager =
      Components.classes["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                .getService(Components.interfaces.sbIPlaylistReaderManager);

    var library = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                            .getService(Components.interfaces.sbILibraryManager).mainLibrary;

    // Create the media list
    var mediaList = library.createMediaList("simple");
    mediaList.name = name;
    mediaList.setProperty("http://songbirdnest.com/data/1.0#originURL", uri.spec);

    aPlaylistReaderManager.originalURI = uri;
    var success = aPlaylistReaderManager.loadPlaylist(uri, mediaList, null, false, null);
    if (success == 1 &&
        mediaList.length) {
      var array = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                            .createInstance(Components.interfaces.nsIMutableArray);
      for (var i = 0; i < mediaList.length; i++) {
        array.appendElement(mediaList.getItemByIndex(i), false);
      }
      
      var metadataService = 
         Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                   .getService(Components.interfaces.sbIFileMetadataService);
      var metadataJob = metadataService.read(array);
      

      // Give the new media list focus
      if (typeof gBrowser != 'undefined') {
        gBrowser.loadMediaList(mediaList);
      }
    } else {
      library.remove(mediaList);
      return null;
    }
    return mediaList;
  }

  function SBLibraryOpen( parentWindow, silent )
  {
    // Get the default library importer.  Do nothing if none available.
    var libraryImporterManager =
          Cc["@songbirdnest.com/Songbird/LibraryImporterManager;1"]
            .getService(Ci.sbILibraryImporterManager);
    var libraryImporter = libraryImporterManager.defaultLibraryImporter;
    if (!libraryImporter)
      return null;

    var importTracks = Application.prefs.getValue(
          "songbird.library_importer.import_tracks", false);
    var importPlaylists = Application.prefs.getValue(
          "songbird.library_importer.import_playlists", false);
    if (!importTracks && !importPlaylists)
      return null;

    // Present the import library dialog.
    var doImport;
    if (!silent) {
      doImport = {};
      WindowUtils.openModalDialog
                    (parentWindow,
                     "chrome://songbird/content/xul/importLibrary.xul",
                     "",
                     "chrome,centerscreen",
                     [ "manual" ],
                     [ doImport ]);
      doImport = doImport.value == "true" ? true : false;
    } else {
      doImport = true;
    }

    // Import the library as user directs.
    var job = null;
    if (doImport) {
      var libraryFilePath = Application.prefs.getValue
                              ("songbird.library_importer.library_file_path",
                               "");
      job = libraryImporter.import(libraryFilePath, "songbird", false);
      SBJobUtils.showProgressDialog(job, window, 0, true);
    }
    return job;
  }

  function log(str)
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage( str );
  }

  // This function should be called when we need to open a URL but gBrowser is 
  // not available. Eventually this should be replaced by code that opens a new 
  // Songbird window, when we are able to do that, but for now, open in the 
  // default external browser.
  // If what you want to do is ALWAYS open in the default external browser,
  // use SBOpenURLInDefaultBrowser directly!
  function SBBrowserOpenURLInNewWindow( the_url ) {
    SBOpenURLInDefaultBrowser(the_url);
  }
  
  // This function opens a URL externally, in the default web browser for the system
  function SBOpenURLInDefaultBrowser( the_url ) {
    var externalLoader = (Components
              .classes["@mozilla.org/uriloader/external-protocol-service;1"]
            .getService(Components.interfaces.nsIExternalProtocolService));
    var nsURI = (Components
            .classes["@mozilla.org/network/io-service;1"]
            .getService(Components.interfaces.nsIIOService)
            .newURI(the_url, null, null));
    externalLoader.loadURI(nsURI, null);
  }

// Help
function onHelp()
{
  var helpitem = document.getElementById("menuitem_help_topics");
  onMenu(helpitem);
}

function SBOpenEqualizer() 
{
  var features = "chrome,titlebar,toolbar,centerscreen,resizable=no";
  SBOpenWindow( "chrome://songbird/content/xul/mediacore/mediacoreEqualizer.xul", "Equalizer", features);
}

function SBOpenPreferences(paneID, parentWindow)
{
  if (!parentWindow) parentWindow = window;

  // On all systems except Windows pref changes should be instant.
  //
  // In mozilla this is the browser.prefereces.instantApply pref,
  // and is set at compile time.
  var instantApply = navigator.userAgent.indexOf("Windows") == -1;

  // BUG 5081 - You can't call restart in a modal window, so
  // we're making prefs non-modal on all platforms.
  // Original line:  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");
  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : "");

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:Preferences");
  if (win) {
    win.focus();
    if (paneID) {
      var pane = win.document.getElementById(paneID);
      win.document.documentElement.showPane(pane);
    }
    return win;
  } else {
    return parentWindow.openDialog("chrome://browser/content/preferences/preferences.xul", "Preferences", features, paneID);
  }

  // to open connection settings only:
  // SBOpenModalDialog("chrome://browser/content/preferences/connection.xul", "chrome,centerscreen", null);
}

function SBOpenDownloadManager()
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
    window.open("chrome://mozapps/content/downloads/downloads.xul", "Download:Manager", "chrome,centerscreen,dialog=no,resizable", null);
  }
}

function SBScanMedia( aParentWindow, aScanDirectory )
{
  var scanDirectory = aScanDirectory;

  if ( !scanDirectory ) {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    const CONTRACTID_FILE_PICKER = "@mozilla.org/filepicker;1";
    var fp = Components.classes[CONTRACTID_FILE_PICKER].createInstance(nsIFilePicker);
    var scan = SBString("faceplate.scan", "Scan", theSongbirdStrings);
    fp.init( window, scan, nsIFilePicker.modeGetFolder );
    if (getPlatformString() == "Darwin") {
      var defaultDirectory =
      Components.classes["@mozilla.org/file/directory_service;1"]
                .getService(Components.interfaces.nsIProperties)
                .get("Home", Components.interfaces.nsIFile);
      defaultDirectory.append("Music");
      fp.displayDirectory = defaultDirectory;
    }
    var res = fp.show();
    if ( res != nsIFilePicker.returnOK )
      return null; /* no job */
    scanDirectory = fp.file;
  }

  if ( scanDirectory && !scanDirectory.exists() ) {
    var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                          .getService(Ci.nsIPromptService);
    var strTitle = SBString("media_scan.error.non_existent_directory.title");
    var strMsg = SBFormattedString
                   ("media_scan.error.non_existent_directory.msg",
                    [scanDirectory.path]);
    promptService.alert(window, strTitle, strMsg);
    return null;
  }

  if ( scanDirectory )
  {
    var importer = Cc['@songbirdnest.com/Songbird/DirectoryImportService;1']
                     .getService(Ci.sbIDirectoryImportService);
    if (typeof(ArrayConverter) == "undefined") {
      Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
    }
    if (typeof(SBJobUtils) == "undefined") {
      Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
    }  
    var directoryArray = ArrayConverter.nsIArray([scanDirectory]);
    var job = importer.import(directoryArray);
    SBJobUtils.showProgressDialog(job, window, /* no delay */ 0);
    return job;
  }
  return null; /* no job - but unreachable */
}

/** Legacy function **/
function SBNewPlaylist(aEnumerator)
{
  var playlist = makeNewPlaylist("simple");
  if (aEnumerator) {
    // make playlist from selected items
    playlist.addSome(aEnumerator);
  }
  return playlist;
}

function SBNewSmartPlaylist()
{
  var obj = { newSmartPlaylist: null,
              newPlaylistFunction: function() { 
                return makeNewPlaylist("smart") 
              } 
            };

  SBOpenModalDialog("chrome://songbird/content/xul/smartPlaylist.xul",
                    "Songbird:SmartPlaylist",
                    "chrome,dialog=yes,centerscreen,modal,titlebar=no",
                    obj);

  if (obj.newSmartPlaylist) {
    if (typeof gBrowser != 'undefined') {
      gBrowser.loadMediaList(obj.newSmartPlaylist);
    }
  }
  
  return obj.newSmartPlaylist;
}

/**
 * Create a new playlist of the given type, using the service pane
 * to determine context and perform renaming
 *
 * Note: This function should move into the window controller somewhere
 *       once it exists.
 */
function makeNewPlaylist(mediaListType) {
  var servicePane = null;
  if (typeof gServicePane != 'undefined') servicePane = gServicePane;

  // Try to find the currently selected service pane node
  var selectedNode;
  if (servicePane) {
    selectedNode = servicePane.getSelectedNode();
  }
  
  // ensure the service pane is initialized (safe to do multiple times)
  var servicePaneService = Components.classes['@songbirdnest.com/servicepane/service;1']
                              .getService(Components.interfaces.sbIServicePaneService);
  servicePaneService.init();

  // Ask the library service pane provider to suggest where
  // a new playlist should be created
  var librarySPS = Components.classes['@songbirdnest.com/servicepane/library;1']
                             .getService(Components.interfaces.sbILibraryServicePaneService);
  var library = librarySPS.suggestLibraryForNewList(mediaListType, selectedNode);

  // Looks like no libraries support the given mediaListType
  if (!library) {
    throw("Could not find a library supporting lists of type " + mediaListType);
  }
  
  // Make sure the library is user editable, if it is not, use the main
  // library instead of the currently selected library.
  if (!library.userEditable ||
      !library.userEditableContent) {
    var libraryManager = 
      Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                .getService(Components.interfaces.sbILibraryManager);
    
    library = libraryManager.mainLibrary;
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

  var metrics =
    Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
              .createInstance(Components.interfaces.sbIMetrics);
  metrics.metricsInc("medialist", "create", mediaListType);
  
  return mediaList;
}

/**
 * Delete a medialist, with user confirmation. If no medialist is specified,
 * the currently visible medialist will be used.
 */
function SBDeleteMediaList(aMediaList)
{
  var mediaList = aMediaList;
  if (!mediaList) {
    mediaList = _SBGetCurrentView().mediaList;
  }
  // if this list is the storage for an outer list, the outer list is the one
  // that should be deleted
  var outerListGuid = 
    mediaList.getProperty(SBProperties.outerGUID);
  if (outerListGuid)
    mediaList = mediaList.library.getMediaItem(outerListGuid);
  // smart playlists are never user editable, determine whether we can delete
  // them based on their parent library user-editable flag. Also don't delete
  // libraries or the download node.
  var listType = mediaList.getProperty(SBProperties.customType);
  if (mediaList.userEditable && 
      listType != "download" &&
      !(mediaList instanceof Ci.sbILibrary)) {
    const BYPASSKEY = "playlist.deletewarning.bypass";
    const STRINGROOT = "playlist.deletewarning.";
    if (!SBDataGetBoolValue(BYPASSKEY)) {
      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Ci.nsIPromptService);
      var check = { value: false };
      
      var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                  .getService(Ci.nsIStringBundleService);
      var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
      var strTitle = SBString(STRINGROOT + "title");
      var strMsg = SBFormattedString(STRINGROOT + "message", [mediaList.name]);

      var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceManager2);
      var selectedDevice =
        deviceManager.getDeviceForItem(mediaList);
      if (!selectedDevice &&
          (deviceManager.marshalls.length > 0)) {
        strMsg += "\n\n" + songbirdStrings.GetStringFromName("playlist.confirmremoveitem.devicewarning");
      }

      var strCheck = SBString(STRINGROOT + "check");
      
      var r = promptService.confirmEx(window, 
                              strTitle, 
                              strMsg, 
                              Ci.nsIPromptService.STD_YES_NO_BUTTONS, 
                              null, 
                              null, 
                              null, 
                              strCheck, 
                              check);
      if (check.value == true) {
        SBDataSetBoolValue(BYPASSKEY, true);
      }
      if (r == 1) { // 0 = yes, 1 = no
        return;
      }
    }
    // delete the medialist. if the medialist is visible in the browser, the
    // browser will automatically switch to the main library
    mediaList.library.remove(mediaList);
  } else {
    Components.utils.reportError("SBDeleteMediaList - Medialist is not user editable");
  }
}

function SBExtensionsManagerOpen( parentWindow )
{
  if (!parentWindow) parentWindow = window;
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
  parentWindow.openDialog(EM_URL, "", EM_FEATURES);
}

function SBTrackEditorOpen( initialTab, parentWindow ) {
  if (!parentWindow) parentWindow = window;
  var browser;
  if (typeof SBGetBrowser == 'function') 
    browser = SBGetBrowser();
  if (browser) {
    if (browser.currentMediaPage) {
      var view = browser.currentMediaPage.mediaListView;
      if (view) {
        var numSelected = view.selection.count;
        if (numSelected > 1) {
          const BYPASSKEY = "trackeditor.multiplewarning.bypass";
          const STRINGROOT = "trackeditor.multiplewarning.";
          if (!SBDataGetBoolValue(BYPASSKEY)) {
            var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                          .getService(Components.interfaces.nsIPromptService);
            var check = { value: false };
            
            var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                .getService(Components.interfaces.nsIStringBundleService);
            var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
            var strTitle = songbirdStrings.GetStringFromName(STRINGROOT + "title");
            var strMsg = songbirdStrings.formatStringFromName(STRINGROOT + "message", [numSelected], 1);
            var strCheck = songbirdStrings.GetStringFromName(STRINGROOT + "check");
            
            var r = promptService.confirmEx(window, 
                                    strTitle, 
                                    strMsg, 
                                    Ci.nsIPromptService.STD_YES_NO_BUTTONS, 
                                    null, 
                                    null, 
                                    null, 
                                    strCheck, 
                                    check);
            if (check.value == true) {
              SBDataSetBoolValue(BYPASSKEY, true);
            }
            if (r == 1) { // 0 = yes, 1 = no
              return;
            }
          }
        } else if (numSelected < 1) {
          // no track is selected, can't invoke the track editor on nothing !
          return;
        }
         
        // xxxlone> note that the track editor is modal, so the window will 
        // never exist. the code is left here in case we ever change back to
        // a modeless track editor.
        var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Components.interfaces.nsIWindowMediator);
        var theTE = wm.getMostRecentWindow("Songbird:TrackEditor");
        if (theTE) {
          theTE.focus();
        } else {
          SBOpenModalDialog("chrome://songbird/content/xul/trackEditor.xul", 
                            "Songbird:TrackEditor", "chrome,centerscreen", 
                            initialTab, parentWindow);
        }
      }
    }
  }
}

function SBGetArtworkOpen() {
  var browser = SBGetBrowser();
  var view = null;
  if (browser && browser.currentMediaPage) {
    view = browser.currentMediaPage.mediaListView;
  }
  
  if (!view)
    return;

  var artworkScanner = Cc["@songbirdnest.com/Songbird/album-art/scanner;1"]
                         .createInstance(Ci.sbIAlbumArtScanner);
  artworkScanner.scanListForArtwork(view.mediaList);
  SBJobUtils.showProgressDialog(artworkScanner, window, 0);  
}

function SBRevealFile( initialTab, parentWindow ) {
  if (!parentWindow) parentWindow = window;
  var browser;
  var view = _SBGetCurrentView();
  if (!view) { return; }
  
  var numSelected = view.selection.count;
  if (numSelected != 1) { return; }
  
  var item = null;
  var selection = view.selection.selectedIndexedMediaItems;
  item = selection.getNext().QueryInterface(Ci.sbIIndexedMediaItem).mediaItem;
  
  if (!item) {
    Cu.reportError("Failed to get media item to reveal.")
    return;
  }
  
  var uri = item.contentSrc;
  if (!uri || uri.scheme != "file") { return; }
  
  let f = uri.QueryInterface(Ci.nsIFileURL).file;
  try {
    // Show the directory containing the file and select the file
    f.QueryInterface(Ci.nsILocalFile);
    f.reveal();
  } catch (e) {
    // If reveal fails for some reason (e.g., it's not implemented on unix or
    // the file doesn't exist), try using the parent if we have it.
    let parent = f.parent.QueryInterface(Ci.nsILocalFile);
    if (!parent)
      return;
  
    try {
      // "Double click" the parent directory to show where the file should be
      parent.launch();
    } catch (e) {
      // If launch also fails (probably because it's not implemented), let the
      // OS handler try to open the parent
      var parentUri = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService).newFileURI(parent);

      var protocolSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"]
                          .getService(Ci.nsIExternalProtocolService);
      protocolSvc.loadURI(parentUri);
    }
  }
}

function SBSubscribe(mediaList, defaultUrl, parentWindow)
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

  var params = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                         .createInstance(Components.interfaces.nsIMutableArray);
  params.appendElement(mediaList, false);
  params.appendElement(defaultUrl, false);

  // Open the window
  SBOpenModalDialog("chrome://songbird/content/xul/subscribe.xul",
                    "",
                    "chrome,centerscreen",
                    params,
                    parentWindow);
}

function SBNewPodcast()
{
  WindowUtils.openModalDialog
    (window,
     "chrome://songbird/content/xul/podcast/podcastCreationDialog.xul",
     "",
     "chrome,centerscreen");
}

// TODO: This function should be renamed.  See openAboutDialog in browserUtilities.js
function About( parentWindow )
{
  // Make a magic data object to get passed to the dialog
  var about_data = new Object();
  about_data.retval = "";
  // Open the modal dialog
  SBOpenModalDialog( "chrome://songbird/content/xul/about.xul", "about", "chrome,centerscreen", about_data, parentWindow );
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

function javascriptConsole() {
  window.open("chrome://global/content/console.xul", "global:console", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar,titlebar");
}

// Match filenames ending with .xpi or .jar
function isXPI(filename) {
  return /\.(xpi|jar)$/i.test(filename);
}

// Prompt the user to install the given XPI.
function installXPI(localFilename)
{
  var inst = { xpi: localFilename };
  InstallTrigger.install( inst );  // "InstallTrigger" is a Moz Global.  Don't grep for it.
  // http://developer.mozilla.org/en/docs/XPInstall_API_Reference:InstallTrigger_Object
}

// Install an array of XPI files.
// @see extensions.js for more information
function installXPIArray(aXPIArray)
{
  InstallTrigger.install(aXPIArray);
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

  if (url instanceof Components.interfaces.nsIURI) url = url.spec;
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
    return mediaItem;

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

  var items = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
    .createInstance(Components.interfaces.nsIMutableArray);

  items.appendElement(mediaItem, false);

  var metadataService = 
     Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
               .getService(Components.interfaces.sbIFileMetadataService);
  var metadataJob = metadataService.read(items);

  return mediaItem;
}

function SBImportURLIntoWebLibrary(url) {
  var library = LibraryUtils.webLibrary;
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

  var items = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
    .createInstance(Components.interfaces.nsIMutableArray);

  items.appendElement(mediaItem, false);

  var metadataService = 
     Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
               .getService(Components.interfaces.sbIFileMetadataService);
  var metadataJob = metadataService.read(items);

  return mediaItem;
}


function getFirstItemByProperty(aMediaList, aProperty, aValue) {

  var listener = {
    item: null,
    onEnumerationBegin: function() {
    },
    onEnumeratedItem: function(list, item) {
      this.item = item;
      return Components.interfaces.sbIMediaListEnumerationListener.CANCEL;
    },
    onEnumerationEnd: function() {
    }
  };

  aMediaList.enumerateItemsByProperty(aProperty,
                                      aValue,
                                      listener );

  return listener.item;
}


}
catch (e)
{
  alert(e);
}
