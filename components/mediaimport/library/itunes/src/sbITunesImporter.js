/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

/*******************************************************************************
 *******************************************************************************
 *
 * Javascript source for the iTunes importer component.
 *
 *******************************************************************************
 ******************************************************************************/

/* *****************************************************************************
 *
 * iTunes importer component imported services.
 *
 ******************************************************************************/

/* Songbird imports. */
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component configuration.
 *
 *******************************************************************************
 ******************************************************************************/

var CompConfig =
{
    /*
     * className                    Name of component class.
     * cid                          Component CID.
     * contractID                   Component contract ID.
     * ifList                       List of external component interfaces.
     *
     * dataFormatVersion            Data format version.
     *                                1: < extension version 2.0
     *                                2: extension version 2.0
     * localeBundlePath             Path to locale string bundle.
     * prefPrefix                   Prefix for component preferences.
     *
     * addTrackBatchSize            Number of tracks to add at a time.
     *
     * sigHashType                  Hash type to use for signature computation.
     * sigDBGUID                    GUID of signature storage database.
     * sigDBTable                   Signature storage database table name.
     *
     * reqPeriod                    Request processor execution period in
     *                              milliseconds.
     * reqPctCPU                    Request processor CPU execution percentage.
     */

    className: "iTunes importer",
    cid: Components.ID("{D6B36046-899A-4C1C-8A97-67ADC6CB675F}"),
    contractID: "@songbirdnest.com/Songbird/ITunesImporter;1",
    ifList: [ Components.interfaces.nsISupports,
              Components.interfaces.nsIObserver,
              Components.interfaces.sbILibraryImporter ],

    dataFormatVersion: 2,
    localeBundlePath: "chrome://itunes_importer/locale/Importer.properties",
    prefPrefix: "library_import.itunes",

    addTrackBatchSize: 100,

    sigHashType: "MD5",
    sigDBGUID: "songbird",
    sigDBTable: "itunes_signatures",

    reqPeriod: 50,
    reqPctCPU: 50
};


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component class.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * Component
 *
 *   This function is the constructor for the component.
 */

function Component()
{
}


/*
 * Component prototype object.
 */

Component.prototype.constructor = Component;
Component.prototype =
{
    /***************************************************************************
     *
     * Importer configuration.
     *
     **************************************************************************/

    /*
     * metaDataTable                Table of meta-data to extract from iTunes.
     * dataFormatVersion            Data format version.
     * localeBundlePath             Path to locale string bundle.
     * prefPrefix                   Prefix for component preferences.
     * addTrackBatchSize            Number of tracks to add at a time.
     */

    metaDataTable:
    [
        { songbird: SBProperties.albumArtistName, iTunes: "Album Artist" },
        { songbird: SBProperties.albumName,       iTunes: "Album" },
        { songbird: SBProperties.artistName,      iTunes: "Artist" },
        { songbird: SBProperties.bitRate,         iTunes: "Bit Rate" },
        { songbird: SBProperties.bpm,             iTunes: "BPM" },
        { songbird: SBProperties.comment,         iTunes: "Comments" },
        { songbird: SBProperties.composerName,    iTunes: "Composer" },
        { songbird: SBProperties.contentMimeType, iTunes: "Kind" },
        { songbird: SBProperties.discNumber,      iTunes: "Disc Number" },
        { songbird: SBProperties.duration,        iTunes: "Total Time",
          convertFunc: "convertDuration" },
        { songbird: SBProperties.genre,           iTunes: "Genre" },
        { songbird: SBProperties.lastPlayTime,    iTunes: "Play Date UTC",
          convertFunc: "convertDateTime" },
        { songbird: SBProperties.lastSkipTime,    iTunes: "Skip Date",
          convertFunc: "convertDateTime" },
        { songbird: SBProperties.playCount,       iTunes: "Play Count" },
        { songbird: SBProperties.rating,          iTunes: "Rating",
          convertFunc: "convertRating" },
        { songbird: SBProperties.sampleRate,      iTunes: "Sample Rate" },
        { songbird: SBProperties.skipCount,       iTunes: "Skip Count" },
        { songbird: SBProperties.totalDiscs,      iTunes: "Disc Count" },
        { songbird: SBProperties.totalTracks,     iTunes: "Track Count" },
        { songbird: SBProperties.trackName,       iTunes: "Name" },
        { songbird: SBProperties.trackNumber,     iTunes: "Track Number" },
        { songbird: SBProperties.year,            iTunes: "Year" }
    ],
    dataFormatVersion: CompConfig.dataFormatVersion,
    localeBundlePath: CompConfig.localeBundlePath,
    prefPrefix: CompConfig.prefPrefix,
    addTrackBatchSize: CompConfig.addTrackBatchSize,


    /***************************************************************************
     *
     * Importer fields.
     *
     **************************************************************************/

    /*
     * mOSType                      OS type.
     * mLocale                      Locale string bundle.
     * mListener                    Listener for import events.
     * mExcludedPlaylistList        List of excluded playlists.
     * mHandleImportReqFunc         handleImportReq function with object
     *                              closure.
     * mHandleAppStartupReqFunc     handleAppStartupReq function with object
     *                              closure.
     * mXMLParser                   XML parser object.
     * mXMLFile                     XML file object being parsed.
     * mIOService                   IO service component.
     * mFileProtocolHandler         File protocol handler component.
     * mPlaylistPlayback            Playlist playback services component.
     * mLibraryManager              Media library manager component.
     * mLibrary                     Media library component.
     * mPlaylist                    Current playlist component.
     * mTrackIDMap                  Map of iTunes track IDs and Songbird track
     *                              UUIDs.
     * mTrackCount                  Count of the number of tracks.
     * mNonExistentMediaCount       Count of the number of non-existent track
     *                              media files.
     * mUnsupportedMediaCount       Count of the number of unsupported media
     *                              tracks.
     * mITunesLibID                 ID of iTunes library to import.
     * mITunesLibSig                iTunes library signature.
     * mImport                      If true, import library into Songbird while
     *                              processing.
     * mImportPlaylists             If true, import playlists into Songbird
     *                              while processing.
     * mDirtyPlaylistAction         Import action to take for all dirty
     *                              playlists.
     * mLibraryFilePathPref         Import library file path preference.
     * mAutoImportPref              Auto import preference.
     * mDontImportPlaylistsPref     Don't import playlists preference.
     * mNotFirstRunDR               Not first run indication.
     * mLibPrevPathDR               Saved path of previously imported library.
     * mLibPrevModTimeDR            Saved modification time of previously
     *                              imported library.
     * mVersionDR                   Importer data format version.
     */

    mOSType: null,
    mLocale: null,
    mListener: null,
    mExcludedPlaylists: "",
    mHandleImportReqFunc: null,
    mHandleAppStartupReqFunc: null,
    mXMLParser: null,
    mXMLFile: null,
    mIOService: null,
    mFileProtocolHandler: null,
    mPlaylistPlayback: null,
    mLibraryManager: null,
    mLibrary: null,
    mPlaylist: null,
    mTrackIDMap: null,
    mTrackCount: 0,
    mNonExistentMediaCount: 0,
    mUnsupportedMediaCount: 0,
    mITunesLibID: "",
    mITunesLibSig: "",
    mImport: false,
    mImportPlaylists: false,
    mDirtyPlaylistAction: null,
    mLibraryFilePathPref: null,
    mAutoImportPref: null,
    mDontImportPlaylistsPref: null,
    mNotFirstRunDR: null,
    mLibPrevPathDR: null,
    mLibPrevModTimeDR: null,
    mVersionDR: null,


    /***************************************************************************
     *
     * Importer constants.
     *
     **************************************************************************/

    /*
     * mSlashCharCode               Character code for "/".
     */

    mSlashCharCode: "/".charCodeAt(0),


    /***************************************************************************
     *
     * Importer sbILibraryImporter interface.
     *
     **************************************************************************/

    /*
     * Attributes.
     *
     * libraryType              Library type.
     * libraryReadableType      Readable library type.
     * libraryDefaultFileName   Default library file name.
     * libraryFileExtensionList Comma separated list of library file extensions.
     */

    libraryType: "itunes",
    libraryReadableType: "iTunes",
    libraryDefaultFileName: "iTunes Music Library.xml",
    libraryFileExtensionList: "xml",


    /*
     * Attribute getters/setters.
     */

    get libraryDefaultFilePath()
    {
        var                         directoryService;
        var                         libraryFile;
        var                         libraryPath = null;

        /* Get the directory service. */
        directoryService =
            Components.classes["@mozilla.org/file/directory_service;1"].
                            createInstance(Components.interfaces.nsIProperties);

        /* Search for an iTunes library database file. */
        /*XXXErikS Should localize directory names. */
        switch (this.mOSType)
        {
            case "MacOSX" :
                libraryFile = directoryService.get
                                                ("Music",
                                                 Components.interfaces.nsIFile);
                libraryFile.append("iTunes");
                libraryFile.append(this.libraryDefaultFileName);
                break;

            case "Windows" :
                libraryFile = directoryService.get
                                                ("Pers",
                                                 Components.interfaces.nsIFile);
                libraryFile.append("My Music");
                libraryFile.append("iTunes");
                libraryFile.append(this.libraryDefaultFileName);
                break;

            default :
                libraryFile = directoryService.get
                                                ("Home",
                                                 Components.interfaces.nsIFile);
                libraryFile.append(this.libraryDefaultFileName);
                break;
        }

        /* If the library file exists, get its path. */
        if (libraryFile.exists())
            libraryPath = libraryFile.path;

        return (libraryPath);
    },


    /*
     * \brief Initiate external library importing.
     *
     * \param aLibFilePath File path to external library to import.
     * \param aGUID GUID of Songbird library into which to import.
     * \param aCheckForChanges If true, check for changes in external library
     * before importing.
     */

    import: function(aLibFilePath, aGUID, aCheckForChanges)
    {
        var                         req = {};

        /* Log progress. */
        Log(  "1: import \""
            + aLibFilePath + "\" \""
            + aGUID + "\" "
            + aCheckForChanges + "\n");

        /* Indicate that a first run scan has occurred. */
        var scanCompletePref =
                Components.classes["@songbirdnest.com/Songbird/DataRemote;1"]
                          .createInstance(Components.interfaces.sbIDataRemote);
        scanCompletePref.init("firstrun.scancomplete", null);
        scanCompletePref.boolValue = true;

        /* Issue an import request. */
        req.func = this.mHandleImportReqFunc;
        req.args = [ aLibFilePath, aGUID, aCheckForChanges ];
        ITReq.issue(req);
    },


    /*
     * \brief Set the listener for import events.
     *
     * \param aListener Import event listener.
     */

    setListener: function(aListener)
    {
        /* Set the listener. */
        this.mListener = aListener;

        /* Only process requests while a listener is set. */
        if (this.mListener)
            ITReq.start();
        else
            ITReq.stop();
    },


    /***************************************************************************
     *
     * Importer nsIObserver interface.
     *
     **************************************************************************/

    /*
     * observe
     *
     *   --> subject                Event subject.
     *   --> topic                  Event topic.
     *   --> data                   Event data.
     *
     *   This function observes the event specified by subject, topic, and data.
     */

    observe: function(subject, topic, data)
    {
        try { this.observe1(subject, topic, data); }
        catch (err) { Compomnents.utils.reportError(err); }
    },

    observe1: function(subject, topic, data)
    {
        var                         req = {};

        /* Handle application startup events as a request to process */
        /* them outside of the application startup event handler.    */
        if (topic == "app-startup")
        {
            /* Activate the first stage importer services. */
            this.activate1();

            /* Issue a handle application startup request. */
            req.func = this.mHandleAppStartupReqFunc;
            req.args = [];
            ITReq.issue(req);
        }
    },


    /***************************************************************************
     *
     * Importer nsISupports interface.
     *
     **************************************************************************/

    /*
     * QueryInterface
     *
     *   --> interfaceID            Requested interface.
     *
     *   NS_ERROR_NO_INTERFACE      Requested interface is not supported.
     *
     *   This function returns a component object implementing the interface
     * specified by interfaceID.
     */

    QueryInterface: function(interfaceID)
    {
        var                         interfaceSupported = false;
        var                         i;

        /* Check for supported interfaces. */
        for (i = 0; i < CompConfig.ifList.length; i++)
        {
            if (interfaceID.equals(CompConfig.ifList[i]))
            {
                interfaceSupported = true;
                break;
            }
        }
        if (!interfaceSupported)
            throw(Components.results.NS_ERROR_NO_INTERFACE);

        return(this);
    },


    /***************************************************************************
     *
     * Internal importer functions.
     *
     **************************************************************************/

    /*
     * activate1, activate2
     *
     *   These functions activate the importer services in multiple stages.  The
     * first stage activates enough services to run the application startup
     * request handler.  The second stage activates the remaining services and
     * should not be called until all dependent services are active.
     */

    activate1: function()
    {
        /* Initialize the request services. */
        ITReq.initialize();

        /* Manually start request processing even */
        /* if an importer listener is not set.    */
        ITReq.start();

        /* Create a handleAppStartupReq function with an object closure. */
        {
            var                     _this = this;

            this.mHandleAppStartupReqFunc = function()
            {
                _this.handleAppStartupReq();
            }
        }
    },

    activate2: function()
    {
        /* Get the OS type. */
        this.mOSType = this.getOSType();

        /* Get the locale string bundle. */
        {
            var                         stringBundleService;

            stringBundleService =
                Components.classes["@mozilla.org/intl/stringbundle;1"].
                    getService(Components.interfaces.nsIStringBundleService);
            this.mLocale =
                        stringBundleService.createBundle(this.localeBundlePath);
        }

        /* Get the list of excluded playlists. */
        this.mExcludedPlaylists = this.mLocale.GetStringFromName
                                        ("library_importer.excluded_playlists");

        /* Get the IO service component. */
        this.mIOService = Components.
                                classes["@mozilla.org/network/io-service;1"].
                                getService(Components.interfaces.nsIIOService);

        /* Get a file protocol handler. */
        this.mFileProtocolHandler =
            Components
                .classes["@mozilla.org/network/protocol;1?name=file"]
                .createInstance(Components.interfaces.nsIFileProtocolHandler);

        /* Get the playlist playback services. */
        this.mPlaylistPlayback =
                Components.
                    classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].
                    getService(Components.interfaces.sbIPlaylistPlayback);

        /* Get the media library manager service          */
        /* component and create a media library instance. */
        this.mLibraryManager =
            Components.
                classes["@songbirdnest.com/Songbird/library/Manager;1"].
                getService(Components.interfaces.sbILibraryManager);
        this.mLibrary = this.mLibraryManager.getLibrary
                                        (this.mLibraryManager.mainLibrary.guid);
        this.mLibrary = this.mLibrary.QueryInterface
                                (Components.interfaces.sbILocalDatabaseLibrary);

        /* Get the importer preferences. */
        this.mLibraryFilePathPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mLibraryFilePathPref.init
                                (this.prefPrefix + ".library_file_path", null);
        this.mAutoImportPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mAutoImportPref.init(this.prefPrefix + ".auto_import", null);
        this.mDontImportPlaylistsPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mDontImportPlaylistsPref.init
                                    (this.prefPrefix + ".dont_import_playlists",
                                     null);

        /* Get the importer data remotes. */
        this.mNotFirstRunDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mNotFirstRunDR.init(this.prefPrefix + ".not_first_run", null);
        this.mLibPrevPathDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mLibPrevPathDR.init(this.prefPrefix + ".lib_prev_path", null);
        this.mLibPrevModTimeDR =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mLibPrevModTimeDR.init(this.prefPrefix + ".lib_prev_mod_time",
                                    null);
        this.mVersionDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mVersionDR.init(this.prefPrefix + ".version", null);

        /* Migrate preference settings. */
        this.migratePrefs();

        /* Activate the database services. */
        ITDB.activate();

        /* Create a handleImportReq function with an object closure. */
        {
            var                     _this = this;

            this.mHandleImportReqFunc = function(libFilePath,
                                                 dbGUID,
                                                 checkForChanges)
            {
                _this.handleImportReq(libFilePath, dbGUID, checkForChanges)
            }
        }
    },


    /*
     * handleAppStartupReq
     *
     *   This function handles application startup events.
     */

    handleAppStartupReq: function()
    {
        var                         ctx;
        var                         state;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;

        /* Ensure the library manager services are active.       */
        /* These are required by the importer database services. */
        if (state == 1)
        {
            try
            {
                var libMgr =
                    Components.
                        classes["@songbirdnest.com/Songbird/library/Manager;1"].
                        getService(Components.interfaces.sbILibraryManager);
                if (libMgr.mainLibrary)
                    state++;
            } catch (ex) {}
        }

        /* Handle application startup. */
        if (state == 2)
        {
            this.handleAppStartupReq1();
            state++;
        }

        /* End state. */
        if (state >= 3)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();
    },

    handleAppStartupReq1: function()
    {
        var                         file;
        var                         modified = true;

        /* Activate the second stage importer services. */
        this.activate2();

        /* If this is the first time the importer has been */
        /* run, send a first run event to the listener.    */
        if (!this.mNotFirstRunDR.boolValue)
        {
            /* Send a first run event. */
            this.mListener.onFirstRun();

            /* Save indication that import has been run once. */
            this.mNotFirstRunDR.boolValue = true;
        }

        /* Initiate importing if auto-import is enabled   */
        /* and the iTunes library file has been modified. */
        else if (   this.mAutoImportPref.boolValue
                 && this.mLibraryFilePathPref.stringValue)
        {
            /* If importing the same library      */
            /* file, check its modification time. */
            if (   this.mLibraryFilePathPref.stringValue
                == this.mLibPrevPathDR.stringValue)
            {
                file = Components.classes["@mozilla.org/file/local;1"].
                          createInstance(Components.interfaces.nsILocalFile);
                file.initWithPath(this.mLibraryFilePathPref.stringValue);
                if (file.lastModifiedTime == this.mLibPrevModTimeDR.stringValue)
                    modified = false;
            }

            /* If the library file has been modified,    */
            /* re-import, checking for relavent changes. */
            if (modified)
            {
                this.import(this.mLibraryFilePathPref.stringValue,
                            "songbird",
                            true);
            }
        }
    },


    /*
     * handleImportReq
     *
     *   --> libFilePath            File path to library to import.
     *   --> dbGUID                 Database GUID of Songbird library into which
     *                              to import.
     *   --> checkForChanges        Check for changes before importing.
     *
     *   This function handles the request to import the media library at the
     * file path specified by libFilePath into the Songbird library with the
     * database GUID specified by dbGUID.
     *   If checkForChanges is true, a check is first made for any changes to
     * the media library.  If no changes are found, no importing is done.
     */

    handleImportReq: function(libFilePath, dbGUID, checkForChanges)
    {
        /* Terminate request on any exceptions. */
        try
        {
            this.handleImportReq1(libFilePath, dbGUID, checkForChanges);
        }
        catch (e)
        {
            /* Dump exception. */
            Components.utils.reportError(e);

            /* Update status. */
            ITStatus.reset();
            ITStatus.mStageText = "Library import error";
            ITStatus.mDone = true;

            /* Send an import error event. */
            this.mListener.onImportError();

            /* Complete request on error. */
            ITReq.completeFunction();
        }

        /* Update status. */
        ITStatus.update();
    },

    handleImportReq1: function(libFilePath, dbGUID, checkForChanges)
    {
        var                         ctx;
        var                         state;
        var                         tag = {};
        var                         tagPreText = {};

        /* Do nothing until the database services are synchronized. */
        if (!ITDB.sync())
            return;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;

        /* Wait until the importer is ready. */
        if (state == 1)
        {
            if (this.isReady())
                state++;
        }

        /* Initialize request processing. */
        if (state == 2)
        {
            /* Initialize importer data format version. */
            if (!this.mVersionDR.intValue)
                this.mVersionDR.intValue = this.dataFormatVersion;

            /* If checking for changes, don't import. */
            if (checkForChanges)
                this.mImport = false;
            else
                this.mImport = true;

            /* Check if playlists should be imported. */
            if (this.mImport)
            {
                this.mImportPlaylists =
                                    !this.mDontImportPlaylistsPref.boolValue;
            }
            else
            {
                this.mImportPlaylists = false;
            }

            /* Initialize status. */
            ITStatus.initialize();
            ITStatus.reset();
            if (checkForChanges)
                ITStatus.mStageText = "Checking for changes in library";
            else
                ITStatus.mStageText = "Importing library";
            ITStatus.bringToFront();

            /* Create an xml parser. */
            this.mXMLParser = new ITXMLParser(libFilePath);
            this.mXMLFile = this.mXMLParser.getFile();

            /* Initialize the iTunes library signature. */
            this.mITunesLibSig = new ITSig();

            /* Initialize the track ID map. */
            this.mTrackIDMap = {};

            /* Initialize import statistics. */
            this.mTrackCount = 0;
            this.mNonExistentMediaCount = 0;
            this.mUnsupportedMediaCount = 0;

            /* Initialize dirty playlist action. */
            this.mDirtyPlaylistAction = null;

            /* Transition to next state. */
            state++;
        }

        /* Find the iTunes library ID key. */
        if (state == 3)
        {
            this.findKey("Library Persistent ID");
            if (!ITReq.isCallPending())
                state++;
        }

        /* Get the iTunes library ID. */
        if (state == 4)
        {
            /* Get the iTunes library ID. */
            this.mXMLParser.getNextTag(tag, tagPreText);
            this.mXMLParser.getNextTag(tag, tagPreText);
            this.mITunesLibID = tagPreText.value;

            /* Add the iTunes library ID to the iTunes library signature. */
            this.mITunesLibSig.update(  "Library Persistent ID"
                                      + this.mITunesLibID);

            /* Transition to next state. */
            state++;
        }

        /* Process the library track list. */
        if (state == 5)
        {
            this.processTrackList();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Process the library playlist list. */
        if (state == 6)
        {
            this.processPlaylistList();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Complete request processing. */
        if (state == 7)
        {
            var                         signature;
            var                         storedSignature;
            var                         foundChanges;
            var                         completeMsg;

            /* Get the iTunes library signature. */
            signature = this.mITunesLibSig.getSignature();

            /* Get the stored iTunes library signature. */
            storedSignature =
                        this.mITunesLibSig.retrieveSignature(this.mITunesLibID);

            /* If imported signature changed, store new signature. */
            if (this.mImport && (signature != storedSignature))
                this.mITunesLibSig.storeSignature(this.mITunesLibID, signature);

            /* Update previous imported library path and modification time. */
            if (this.mImport || (signature == storedSignature))
            {
                this.mLibPrevPathDR.stringValue = libFilePath;
                this.mLibPrevModTimeDR.stringValue =
                                                this.mXMLFile.lastModifiedTime;
            }

            /* Update import data format version. */
            if (this.mImport)
                this.mVersionDR.intValue = this.dataFormatVersion;

            /* Dispose of the XML parser. */
            /*zzz won't happen on exceptions. */
            this.mXMLParser.close();
            this.mXMLParser = null;

            /* Check if changes were looked for and found. */
            if ((checkForChanges) && (signature != storedSignature))
                foundChanges = true;
            else
                foundChanges = false;

            /* Determine completion message. */
            if (checkForChanges)
            {
                if (foundChanges)
                    completeMsg = "Found library changes";
                else
                    completeMsg = "No library changes found";
            }
            else
            {
                completeMsg = "Library import complete";
            }

            /* Update status. */
            ITStatus.reset();
            ITStatus.mStageText = completeMsg;
            ITStatus.mDone = true;

            /* If checking for changes and changes were */
            /* found, send a library changed event.     */
            if (foundChanges)
                this.mListener.onLibraryChanged(libFilePath, dbGUID);

            /* If non-existent media is encountered, send an event. */
            if (this.mImport && (this.mNonExistentMediaCount > 0))
            {
                this.mListener.onNonExistentMedia(this.mNonExistentMediaCount,
                                                  this.mTrackCount);
            }

            /* If unsupported media is encountered, send an event. */
            if (this.mImport && (this.mUnsupportedMediaCount > 0))
                this.mListener.onUnsupportedMedia();

            /* Transition to next state. */
            state++;
        }

        /* Wait for the database services to synchronize. */
        if (state == 8)
        {
            if (ITDB.sync())
                state++;
        }

        /* End state. */
        if (state >= 9)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();

        /* Synchronize the database services. */
        ITDB.sync();
    },


    /*
     * isReady
     *
     *   <--                        True if the importer is ready to import.
     *
     *   This function checks if the importer is ready to import.  This includes
     * checking for the availability of all requisite services.
     */

    isReady: function()
    {
        var                         ready = true;

        /* If the playlist playback core is not  */
        /* available, the importer is not ready. */
        if (!this.mPlaylistPlayback.core)
            ready = false;

        return (ready);
    },


    /*
     * findKey
     *
     *   --> tgtKeyName             Target name of key to find.
     *
     *   This function searches for the next key with the name specified by
     * tgtKeyName.
     */

    findKey: function(tgtKeyName, tag, tagPreText)
    {
        var                         ctx;
        var                         state;
        var                         tag = {};
        var                         tagPreText = {};

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 0;

        /* Get the function context. */
        state = ctx.state;

        /* Find the target key. */
        while (state == 0)
        {
            /* Get the next tag. */
            this.mXMLParser.getNextTag(tag, tagPreText);

            /* If the next key is the target or no more     */
            /* tags are left, transition to the next state. */
            if ((tagPreText.value == tgtKeyName) || (!tag.value))
                state++;

            /* Check if it's time to yield. */
            if (ITReq.shouldYield())
            {
                /* Update status. */
                ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                                     / this.mXMLFile.fileSize;

                /* Yield. */
                break;
            }
        }

        /* End state. */
        if (state >= 1)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();
    },


    /*
     * getOSType
     *
     *   <--                        Returned OS type.
     *
     *   This function determines and returns the OS type from one of the
     * following values:
     *
     *   "MacOSX"
     *   "Linux"
     *   "Windows"
     */

    getOSType: function()
    {
        var                         appInfo;
        var                         osName;
        var                         osType = null;

        /* Get the OS name. */
        appInfo = Components.classes["@mozilla.org/xre/app-info;1"].
                            createInstance(Components.interfaces.nsIXULRuntime);
        osName = appInfo.OS.toLowerCase();

        /* Determine the OS type. */
        if (osName.search("darwin") != -1)
            osType = "MacOSX";
        else if (osName.search("linux") != -1)
            osType = "Linux";
        else if (osName.search("win") != -1)
            osType = "Windows";

        return (osType);
    },


    /*
     * getJSArray
     *
     *   --> nsArray                nsIArray array.
     *
     *   <--                        Javascript array.
     *
     *   This function converts the nsIArray array specified by nsArray to a
     * Javascript array and returns the results.
     */

    getJSArray: function(nsArray)
    {
        var                         enumerator;
        var                         jsArray = [];

        enumerator = nsArray.enumerate();
        while (enumerator.hasMoreElements())
        {
            jsArray.push(enumerator.getNext());
        }

        return (jsArray);
    },


    /*
     * migratePrefs
     *
     *   This function migrates preference settings from previous versions to
     * the current.
     */

    migratePrefs: function()
    {
        /* Migrate "not first run" setting. */
        if (!this.mNotFirstRunDR.boolValue)
        {
            if (this.mLibraryFilePathPref.stringValue)
                this.mNotFirstRunDR.boolValue = true;
        }

        /* Migrate importer data format version setting. */
        if (this.mNotFirstRunDR.boolValue && !this.mVersionDR.intValue)
            this.mVersionDR.intValue = 1;
    },


    /***************************************************************************
     *
     * Importer track functions.
     *
     **************************************************************************/

    /*
     * mAddTrackList                List of tracks to add.
     */

    mAddTrackList: [],


    /*
     * processTrackList
     *
     *   This function processes the library track list.
     */

    processTrackList: function()
    {
        var                         ctx;
        var                         state;
        var                         tag = {};
        var                         tagPreText = {};

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 0;

        /* Get the function context. */
        state = ctx.state;

        /* Find the track list. */
        if (state == 0)
        {
            this.findKey("Tracks");
            if (!ITReq.isCallPending())
                state++;
        }

        /* Skip the "dict" tag. */
        if (state == 1)
        {
            this.mXMLParser.getNextTag(tag, tagPreText);
            state++;
        }

        /* Process each track in list. */
        while (state == 2)
        {
            /* Process another track and add it to batch.  If */
            /* add track batch is full, process the batch.    */
            if (!this.addTrackBatchFull())
            {
                /* Get the next tag. */
                this.mXMLParser.getNextTag(tag, tagPreText);

                /* If it's a "/key" tag, process the track.  If  */
                /* it's a "/dict" tag, there are no more tracks. */
                if (tag.value == "/key")
                    this.processTrack(tag, tagPreText);
                else if (tag.value == "/dict")
                    state++;
            }
            else
            {
                /* Process add track batch. */
                this.addTrackBatchProcess();
                if (ITReq.isCallPending())
                    break;

                /* Check if it's time to yield. */
                if (ITReq.shouldYield())
                    break;
            }
        }

        /* Process the remaining add track batch. */
        if (state == 3)
        {
            if (!this.addTrackBatchEmpty())
                this.addTrackBatchProcess();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Wait for the database services to synchronize. */
        if (state == 4)
        {
            if (ITDB.sync())
                state++;
        }

        /* End state. */
        if (state >= 5)
            ITReq.completeFunction();

        /* Update status. */
        ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                             / this.mXMLFile.fileSize;

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();
    },


    /*
     * processTrack
     *
     *   --> tag.value              Track starting tag value.
     *   --> tagPreText.value       Track starting pre-text.
     *
     *   This function processes the next track with the starting tag value and
     * pre-text specified by tag and tagPreText.
     */

    processTrack: function(tag, tagPreText)
    {
        var                         trackInfoTable = {};
        var                         iTunesTrackID;
        var                         url;
        var                         metaKeys = [];
        var                         metaValues = [];
        var                         uuid;
        var                         trackURI;
        var                         trackFile;
        var                         supported;

        /* One more track. */
        this.mTrackCount++;

        /* Skip the "dict" tag. */
        this.mXMLParser.getNextTag(tag, tagPreText);

        /* Get the track info. */
        this.getTrackInfo(trackInfoTable, tag, tagPreText);

        /* Get the track media URL. */
        url = this.getTrackURL(trackInfoTable);

        /* Log progress. */
        Log("1: processTrack \"" + url + "\"\n");

        /* Set up the metadata. */
        this.setupTrackMetadata(metaKeys, metaValues, trackInfoTable);

        /* Check if the track media exists. */
        var trackExists = false;
        trackURI = this.mIOService.newURI(url, null, null);
        if (trackURI.scheme == "file")
        {
            try
            {
                trackFile = this.mFileProtocolHandler.getFileFromURLSpec(url);
                trackExists = trackFile.exists();
            }
            catch (ex)
            {
                Log("processTrack: File protocol error " + ex + "\n");
                Log(url + "\n");
            }
            if (!trackExists)
                this.mNonExistentMediaCount++;
        }

        /* Add the track content length metadata. */
        if (trackExists && trackFile)
        {
            metaKeys.push(SBProperties.contentLength);
            metaValues.push(trackFile.fileSize.toString());
        }

        /* Check if the track media is supported and */
        /* add it to the iTunes library signature.   */
        supported = this.mPlaylistPlayback.isMediaURL(url);
        if (!supported)
            this.mUnsupportedMediaCount++;
        this.mITunesLibSig.update("supported" + supported);

        /* Get the track persistent ID and add */
        /* it to the iTunes library signature. */
        iTunesTrackID = trackInfoTable["Persistent ID"];
        this.mITunesLibSig.update("Persistent ID" + iTunesTrackID);

        /* Add the track to the media library. */
        if (this.mImport && supported)
        {
            this.addTrack(trackInfoTable,
                          url,
                          iTunesTrackID,
                          metaKeys,
                          metaValues);
        }
    },


    /*
     * getTrackInfo
     *
     *   <-- trackInfoTable         Table of track information.
     *   --> tag.value              Track starting tag value.
     *   --> tagPreText.value       Track starting pre-text.
     *
     *   This function gets track information from the next track with the
     * starting tag value and pre-text specified by tag and tagPreText.  The
     * track information is returned in trackInfoTable.
     */

    getTrackInfo: function(trackInfoTable, tag, tagPreText)
    {
        var                         keyName;
        var                         keyValue;
        var                         done;

        /* Get the track info. */
        done = false;
        while (!done)
        {
            /* Get the next tag. */
            this.mXMLParser.getNextTag(tag, tagPreText);

            /* If it's a "key" tag, process the track info. */
            if (tag.value == "key")
            {
                /* Get the key name. */
                this.mXMLParser.getNextTag(tag, tagPreText);
                keyName = tagPreText.value;

                /* Get the key value.  If the next tag is */
                /* empty, use its name as the key value.  */
                this.mXMLParser.getNextTag(tag, tagPreText);
                if (tag.value.charCodeAt(tag.value.length - 1) ==
                    this.mSlashCharCode)
                {
                    keyValue = tag.value.substring(0, tag.value.length - 1);
                }
                else
                {
                    this.mXMLParser.getNextTag(tag, tagPreText);
                    keyValue = tagPreText.value;
                }

                /* Save the track info. */
                trackInfoTable[keyName] = keyValue;
            }

            /* Otherwise, if it's "/dict" tag, the track info is done. */
            else if (tag.value == "/dict")
            {
                done = true;
            }
        }
    },


    /*
     * getTrackURL
     *
     *   --> trackInfoTable         Table of track information.
     *
     *   <--                        Track URL.
     *
     *   This function extracts and returns the track URL from the track
     * information table specified by trackInfoTable.
     */

    getTrackURL: function(trackInfoTable)
    {
        var                         url;

        /* Get the track media URL and add it */
        /* to the iTunes library signature.   */
        url = trackInfoTable["Location"];
        {
            /* If file path prefix is "file://localhost//", */
            /* convert path as a UNC network path.          */
            url = url.replace(/^file:\/\/localhost\/\//, "file://///");

            /* If file path prefix is "file://localhost/", */
            /* convert path as a local file path.          */
            url = url.replace(/^file:\/\/localhost\//, "file:///");

            /* Strip trailing "/" that iTunes apparently adds sometimes. */
            url = url.replace(/\/$/, "");

            /* If bare DOS drive letter path is specified, set file scheme. */
            if (url.match(/^\w:/))
                url = "file:///" + url;

            /* If no scheme is specified, set file */
            /* scheme with a UNC network path.     */
            if (!url.match(/^\w*:/))
                url = "file:////" + url;
        }
        this.mITunesLibSig.update("url" + url);

        /* Windows is case-insensitive, so convert to lower case. */
        if (this.mOSType == "Windows")
            url = url.toLowerCase();

        return (url);
    },


    /*
     * setupTrackDuration
     *
     *   --> metaKeys               Metadata keys.
     *   <-- metaValues             Metadata values.
     *   <-- trackInfoTable         Table of track information.
     *
     *   This function extracts track duration metadata from the track
     * information table specified by trackInfoTable and places it in the
     * metadata tables specified by metaKeys and metaValues.
     */

    setupTrackDuration: function(metaKeys, metaValues, trackInfoTable)
    {
        var                         trackInfo;
        var                         duration;

        /* Do nothing if no duration metadata available. */
        trackInfo = trackInfoTable["Total Time"];
        if (!trackInfo)
            return;

        /* Get the track duration, convert it to           */
        /* micro-seconds and add it to the metadata array. */
        duration = parseInt(trackInfo) * 1000;
        metaKeys.push(SBProperties.duration);
        metaValues.push(duration);

        /* Add the metadata to the iTunes library signature. */
        this.mITunesLibSig.update(SBProperties.duration + duration);
    },


    /*
     * setupTrackRating
     *
     *   --> metaKeys               Metadata keys.
     *   <-- metaValues             Metadata values.
     *   <-- trackInfoTable         Table of track information.
     *
     *   This function extracts track rating metadata from the track information
     * table specified by trackInfoTable and places it in the metadata tables
     * specified by metaKeys and metaValues.
     */

    setupTrackRating: function(metaKeys, metaValues, trackInfoTable)
    {
        var                         trackInfo;
        var                         rating;

        /* Do nothing if no rating metadata available. */
        trackInfo = trackInfoTable["Rating"];
        if (!trackInfo)
            return;

        /* Get the iTunes track rating, convert it to a Songbird */
        /* track rating, and add it to the metadata array.       */
        rating = Math.floor((parseInt(trackInfo) + 10) / 20);
        metaKeys.push(SBProperties.rating);
        metaValues.push(rating);

        /* Add the metadata to the iTunes library signature. */
        this.mITunesLibSig.update(SBProperties.rating + rating);
    },


    /*
     * setupTrackMetadata
     *
     *   --> metaKeys               Metadata keys.
     *   <-- metaValues             Metadata values.
     *   <-- trackInfoTable         Table of track information.
     *
     *   This function extracts track metadata from the track information table
     * specified by trackInfoTable and places it in the metadata tables
     * specified by metaKeys and metaValues.
     */

    setupTrackMetadata: function(metaKeys, metaValues, trackInfoTable)
    {
        var                         metaDataEntry;
        var                         trackInfo;
        var                         metaValue;
        var                         i;

        /* Set up the metadata. */
        for (i = 0; i < this.metaDataTable.length; i++)
        {
            /* Get the metadata entry name. */
            metaDataEntry = this.metaDataTable[i];

            /* Add the metadata to the metadata array. */
            trackInfo = trackInfoTable[metaDataEntry.iTunes];
            if (trackInfo)
            {
                /* Convert the metadata value from iTunes to Songbird format. */
                metaValue = this.convertMetaValue(trackInfo,
                                                  metaDataEntry.convertFunc);

                /* Add the metadata to the metadata array. */
                metaKeys.push(metaDataEntry.songbird);
                metaValues.push(metaValue);
            }

            /* Add the metadata to the iTunes library signature. */
            if (trackInfo)
                this.mITunesLibSig.update(metaDataEntry.songbird + trackInfo);
        }
    },


    /*
     * convertMetaValue
     *
     *   --> iTunesMetaValue        iTunes metadata value.
     *   --> convertFunc            Conversion function.
     *
     *   <--                        Songbird metadata value.
     *
     *   This function converts the iTunes metadata value specified by
     * iTunesMetaValue to a Songbird metadata value using the conversion
     * function specified by convertFunc.  This function returns the Songbird
     * metadata value.
     */

    convertMetaValue: function(iTunesMetaValue, convertFunc)
    {
        var                         sbMetaValue;

        /* If a conversion function was provided, use it.  Otherwise, just */
        /* copy the iTunes metadata value to the Songbird metadata value.  */
        if (convertFunc)
            sbMetaValue = this[convertFunc](iTunesMetaValue);
        else
            sbMetaValue = iTunesMetaValue;

        return sbMetaValue;
    },


    /*
     * convertRating
     *
     *   --> iTunesMetaValue        iTunes metadata value.
     *
     *   <--                        Songbird metadata value.
     *
     *   This function converts the iTunes metadata rating value specified by
     * iTunesMetaValue to a Songbird metadata rating value and returns the
     * Songbird metadata value.
     */

    convertRating: function(iTunesMetaValue)
    {
        var rating = Math.floor((parseInt(iTunesMetaValue) + 10) / 20);
        return rating.toString();
    },


    /*
     * convertDuration
     *
     *   --> iTunesMetaValue        iTunes metadata value.
     *
     *   <--                        Songbird metadata value.
     *
     *   This function converts the iTunes metadata duration value specified by
     * iTunesMetaValue to a Songbird metadata duration value and returns the
     * Songbird metadata value.
     */

    convertDuration: function(iTunesMetaValue)
    {
        var duration = parseInt(iTunesMetaValue) * 1000;
        return duration.toString();
    },


    /*
     * convertDateTime
     *
     *   --> iTunesMetaValue        iTunes metadata value.
     *
     *   <--                        Songbird metadata value.
     *
     *   This function converts the iTunes metadata date/time value specified by
     * iTunesMetaValue to a Songbird metadata date/time value and returns the
     * Songbird metadata value.
     */

    convertDateTime: function(iTunesMetaValue)
    {
        var                         sbMetaValue;

        /* Convert "1970-01-01T00:00:00Z" to "1970/01/01 00:00:00 UTC". */
        iTunesMetaValue = iTunesMetaValue.replace(/-/g, "/");
        iTunesMetaValue = iTunesMetaValue.replace(/T/g, " ");
        iTunesMetaValue = iTunesMetaValue.replace(/Z/g, " UTC");

        /* Parse the date/time string into epoc time. */
        sbMetaValue = Date.parse(iTunesMetaValue);

        return sbMetaValue.toString();
    },


    /*
     * addTrack
     *
     *   --> trackInfoTable         Table of track information.
     *   --> url                    Track media URL.
     *   --> iTunesTrackID          iTunes track ID.
     *   --> metaValues             Metadata values.
     *   --> trackInfoTable         Table of track information.
     *
     *   This function adds the track with the specified information to the
     * media library.
     */

    addTrack: function(trackInfoTable, url, iTunesTrackID, metaKeys, metaValues)
    {
        var                         addTrack = {};

        /* Add the track to the add track batch. */
        addTrack.trackInfoTable = trackInfoTable;
        addTrack.url = url;
        addTrack.iTunesTrackID = iTunesTrackID;
        addTrack.metaKeys = metaKeys;
        addTrack.metaValues = metaValues;
        this.mAddTrackList.push(addTrack);
    },


    /*
     * addTrackBatchProcess
     *
     *   This function processes the batch of tracks to be added to the library.
     */

    addTrackBatchProcess: function()
    {
        var                         ctx;
        var                         state;
        var                         addTrack;
        var                         guidList = [];
        var                         guid;
        var                         uriList = [];
        var                         uri;
        var                         iTunesTrackIDList = [];
        var                         propertyName;
        var                         i, j;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;

        /* Get the list of Songbird track GUIDs */
        /* from the list of iTunes track IDs.   */
        if (state == 1)
        {
            this.addTrackBatchGetGUIDs();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Get the list of existing track media items. */
        if (state == 2)
        {
            this.addTrackBatchGetMediaItems();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Create track media items for non-existent tracks. */
        if (state == 3)
        {
            this.addTrackBatchCreateMediaItems();
            if (!ITReq.isCallPending())
                state++;
        }

        /*XXXeps should sync any updated properties in existing tracks. */

        /* Add the tracks to the track ID map. */
        if (state == 4)
        {
            for (i = 0; i < this.mAddTrackList.length; i++)
            {
                addTrack = this.mAddTrackList[i];
                this.mTrackIDMap[addTrack.trackInfoTable["Track ID"]] =
                                                                addTrack.guid;
            }
            state++;
        }

        /* Clear add track list. */
        if (state == 5)
        {
            this.mAddTrackList = [];
            state++;
        }

        /* End state. */
        if (state >= 6)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();
    },


    /*
     * addTrackBatchGetGUIDs
     *
     *   This function gets the Songbird track GUIDs mapped to the iTunes track
     * IDs within the add track batch.
     */

    addTrackBatchGetGUIDs: function()
    {
        var                         addTrack;
        var                         guid;
        var                         guidList = [];
        var                         i;

        /* Get the list of Songbird track GUIDs */
        /* from the list of iTunes track IDs.   */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            addTrack = this.mAddTrackList[i];
            guid = ITDB.getSBIDFromITID(this.mITunesLibID,
                                        addTrack.iTunesTrackID);
            guidList[i] = guid;
        }

        /* Add the Songbird track GUIDs to the add track list. */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            this.mAddTrackList[i].guid = guidList[i];
        }
    },


    /*
     * addTrackBatchGetMediaItems
     *
     *   This function gets the existing track media items within the add track
     * batch.
     */

    addTrackBatchGetMediaItems: function()
    {
        var                         addTrack;
        var                         mediaItemList = [];
        var                         mediaItem;
        var                         guidList = [];
        var                         guid;
        var                         i;

        /* Get the list of existing track GUIDs. */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            addTrack = this.mAddTrackList[i];
            if (addTrack.guid)
                guidList.push(addTrack.guid);
        }

        /* Get the list of existing track media items. */
        for (i = 0; i < guidList.length; i++)
        {
            guid = guidList[i];
            try
            {
                mediaItem = this.mLibrary.getMediaItem(guid);
            }
            catch (e)
            {
                mediaItem = null;
            }
            mediaItemList[i] = mediaItem;
        }

        /* Add the media items to the add track list. */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            addTrack = this.mAddTrackList[i];
            if (addTrack.guid)
                addTrack.mediaItem = mediaItemList.shift();
        }
    },


    /*
     * addTrackBatchCreateMediaItems
     *
     *   This function creates media items for the tracks in the add track
     * batch.
     */

    addTrackBatchCreateMediaItems: function()
    {
        var                         ctx;
        var                         state;
        var                         createMediaItemsListener;
        var                         addTrackMediaItemList;

        var                         addTrack;
        var                         mediaItemList = [];
        var                         mediaItem;
        var                         iTunesTrackIDList = [];
        var                         guidList = [];
        var                         uriList;
        var                         trackURI;
        var                         propertyArrayArray;
        var                         propertyArray;
        var                         i, j;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;
        createMediaItemsListener = ctx.createMediaItemsListener;
        addTrackMediaItemList = ctx.addTrackMediaItemList;

        /* Set up a list of track URIs and           */
        /* properties.  Do nothing if list is empty. */
        if (state == 1)
        {
            uriList = Components.
                        classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].
                        createInstance(Components.interfaces.nsIMutableArray);
            propertyArrayArray =
                    Components.
                        classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].
                        createInstance(Components.interfaces.nsIMutableArray);
            for (i = 0; i < this.mAddTrackList.length; i++)
            {
                addTrack = this.mAddTrackList[i];
                if (!addTrack.mediaItem)
                {
                    this.addTrackMediaItemInfo(this.mAddTrackList[i],
                                               uriList,
                                               propertyArrayArray);
                }
            }
            if (uriList.length > 0)
                state++;
            else
                state = 1000;
        }

        /* Create the track media items. */
        if (state == 2)
        {
            createMediaItemsListener =
            {
                complete: false,
                mediaItems: null,
                result: Components.results.NS_OK,

                onProgress: function(aIndex) {},
                onComplete: function(aMediaItems, aResult)
                {
                    this.mediaItems = aMediaItems;
                    this.result = aResult;
                    this.complete = true;
                },
            };

            addTrackMediaItemList = {};
            this.mLibrary.batchCreateMediaItemsAsync(createMediaItemsListener,
                                                     uriList,
                                                     propertyArrayArray);
            state++;
        }

        /* Wait for media item creation to complete. */
        if (state == 3)
        {
            if (createMediaItemsListener.complete)
            {
                /* If the creation completed successfully, get the list */
                /* of created items.  Otherwise, skip to the end state. */
                if (createMediaItemsListener.result == Components.results.NS_OK)
                {
                    addTrackMediaItemList = createMediaItemsListener.mediaItems;
                }
                else
                {
                    Log("Error adding media items " +
                        createMediaItemsListener.result + "\n");
                    state = 9999;
                }
                state++;
            }
        }

        /* Add the media items to the add track list. */
        if (state == 4)
        {
            mediaItemList = this.getJSArray(addTrackMediaItemList);
            for (i = 0; i < this.mAddTrackList.length; i++)
            {
                addTrack = this.mAddTrackList[i];
                if (!addTrack.mediaItem)
                {
                    mediaItem = mediaItemList.shift();
                    if (mediaItem)
                    {
                        addTrack.mediaItem = mediaItem;
                        addTrack.guid = mediaItem.guid;
                        iTunesTrackIDList.push(addTrack.iTunesTrackID);
                        guidList.push(addTrack.guid);
                    }
                    else
                    {
                        Log("Error creating track.\n");
                    }
                }
            }
            state++;
        }

        /* Map the Songbird track GUIDs to iTunes track IDs. */
        if (state == 5)
        {
            for (i = 0; i < iTunesTrackIDList.length; i++)
            {
                ITDB.mapID(this.mITunesLibID,
                           iTunesTrackIDList[i],
                           guidList[i]);
            }
            state++;
        }

        /* End state. */
        if (state >= 6)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ctx.createMediaItemsListener = createMediaItemsListener;
        ctx.addTrackMediaItemList = addTrackMediaItemList;
        ITReq.exitFunction();
    },


    /*
     * addTrackBatchFull
     *
     *   <--                        True if add track batch is full.
     *
     *   This function returns true if the add track batch is full and needs to
     * be processed.
     */

    addTrackBatchFull: function()
    {
        if (this.mAddTrackList.length >= this.addTrackBatchSize)
            return (true);
        else
            return (false);
    },


    /*
     * addTrackBatchEmpty
     *
     *   <--                        True if add track batch is empty.
     *
     *   This function returns true if there are no tracks to be processed in
     * the add track batch.
     */

    addTrackBatchEmpty: function()
    {
        if (this.mAddTrackList.length == 0)
            return (true);
        else
            return (false);
    },


    /*
     * addTrackMediaItemInfo
     *
     *   --> aAddTrack              Track to add.
     *   --> aURIList               List of media item URI's.
     *   --> aPropertyArrayArray    Array of media item properties.
     *
     *   This function extracts track information from the track specified by
     * aAddTrack and adds it to the media item URI list specified by aURIList
     * and the media item property list specified by aPropertyArrayArray.
     */

    addTrackMediaItemInfo: function(aAddTrack, aURIList, aPropertyArrayArray)
    {
        var                         propertyArray;
        var                         trackURI;
        var                         i;

        /* Create a property array. */
        propertyArray =
            Components.classes
                ["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                .createInstance(Components.interfaces.sbIMutablePropertyArray);

        /* Add the track URI. */
        trackURI = this.mIOService.newURI(aAddTrack.url, null, null);
        aURIList.appendElement(trackURI, false);

        /* Add the track properties. */
        for (i = 0; i < aAddTrack.metaKeys.length; i++)
        {
            if (aAddTrack.metaKeys[i] &&
                aAddTrack.metaValues[i] &&
                aAddTrack.metaKeys[i].length &&
                aAddTrack.metaValues[i].length) {
                try
                {
                    propertyArray.appendProperty(aAddTrack.metaKeys[i],
                                                 aAddTrack.metaValues[i]);
                }
                catch (e)
                {
                    Log(  "Exception adding property \""
                        + aAddTrack.metaKeys[i]
                        + "\" \"" + aAddTrack.metaValues[i] + "\"\n");
                    Log(e + "\n");
                }
            }
        }

        /* If the track URI has an "http:" shceme, set the                   */
        /* downloadStatusTarget property to prevent the auto-downloader from */
        /* trying to download it.                                            */
        if (trackURI.scheme.match(/^http/))
        {
            propertyArray.appendProperty(SBProperties.downloadStatusTarget,
                                         "stream");
        }

        /* Add the track properties to the property array array. */
        aPropertyArrayArray.appendElement(propertyArray, false);
    },


    /***************************************************************************
     *
     * Importer playlist functions.
     *
     **************************************************************************/

    /*
     * mAddPlaylistTrackPlaylist    Playlist to which to add tracks.
     * mAddPlaylistTrackList        List of tracks to add.
     */

    mAddPlaylistTrackPlaylist: null,
    mAddPlaylistTrackList: null,


    /*
     * processPlaylistList
     *
     *   This function processes the library playlist list.
     */

    processPlaylistList: function()
    {
        var                         ctx;
        var                         state;
        var                         tag = {};
        var                         tagPreText = {};
        var                         processPlaylist;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 0;

        /* Get the function context. */
        state = ctx.state;
        processPlaylist = ctx.processPlaylist;

        /* Find the playlists list. */
        if (state == 0)
        {
            this.findKey("Playlists");
            if (!ITReq.isCallPending())
                state++;
        }

        /* Skip the "array" tag. */
        if (state == 1)
        {
            this.mXMLParser.getNextTag(tag, tagPreText);
            state++;
        }

        /* Process each playlist in list. */
        if (state == 2)
        {
            /* Initialize loop. */
            processPlaylist = false;
            state++;
        }
        while (state == 3)
        {
            /* Find a playlist to process. */
            if (!processPlaylist)
            {
                /* Get the next tag. */
                this.mXMLParser.getNextTag(tag, tagPreText);

                /* If it's a "dict" tag, process the playlist.  If   */
                /* it's a "/array" tag, there are no more playlists. */
                if (tag.value == "dict")
                    processPlaylist = true;
                else if (tag.value == "/array")
                    state++;
            }

            /* Process playlist if it's been reached. */
            if (processPlaylist)
            {
                this.processPlaylist();
                if (!ITReq.isCallPending())
                    processPlaylist = false;
            }

            /* Check if it's time to yield. */
            if (ITReq.shouldYield())
            {
                /* Update status. */
                ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                                     / this.mXMLFile.fileSize;

                /* Yield. */
                break;
            }
        }

        /* End state. */
        if (state >= 4)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ctx.processPlaylist = processPlaylist;
        ITReq.exitFunction();
    },


    /*
     * processPlaylist
     *
     *   This function processes the next playlist.
     */

    processPlaylist: function()
    {
        var                         ctx;
        var                         state;
        var                         playlistInfoTable;
        var                         playlistName;
        var                         iTunesPlaylistID;
        var                         importPlaylist;
        var                         action;
        var                         hasPlaylistItems;
        var                         dirtyPlaylist;

        var                         sigGen;
        var                         signature;
        var                         done;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
        {
            ctx.state = 1;
            ctx.playlistInfoTable = {};
        }

        /* Get the function context. */
        state = ctx.state;
        playlistInfoTable = ctx.playlistInfoTable;
        playlistName = ctx.playlistName;
        iTunesPlaylistID = ctx.iTunesPlaylistID;
        importPlaylist = ctx.importPlaylist;
        action = ctx.action;
        hasPlaylistItems = ctx.hasPlaylistItems;
        dirtyPlaylist = ctx.dirtyPlaylist;

        /* Get the playlist info. */
        if (state == 1)
        {
            hasPlaylistItems = this.getPlaylistInfo(playlistInfoTable);
            playlistName = playlistInfoTable["Name"];
            iTunesPlaylistID = playlistInfoTable["Playlist Persistent ID"];
            state++;
        }

        /* Check if playlist should be imported. */
        if (state == 2)
        {
            /* Assume playlist should be imported. */
            importPlaylist = true;

            /* Don't import playlists with no items. */
            if (!hasPlaylistItems)
                importPlaylist = false;

            /* Don't import master playlist. */
            else if (playlistInfoTable["Master"] == "true")
                importPlaylist = false;

            /* Don't import smart playlists. */
            else if (playlistInfoTable["Smart Info"])
                importPlaylist = false;

            /* Don't import excluded playlists. */
            else
                if (   this.mExcludedPlaylists.indexOf(":" + playlistName + ":")
                    != -1)
            {
                importPlaylist = false;
            }

            /* If importing, add playlist info  */
            /* to the iTunes library signature. */
            if (importPlaylist)
            {
                this.mITunesLibSig.update("Name" + playlistName);
                this.mITunesLibSig.update(  "Playlist Persistent ID"
                                          + iTunesPlaylistID);
            }

            /* Transition to next state. */
            state++;
        }

        /* Get the Songbird playlist. */
        if (state == 3)
        {
            if ((importPlaylist) && (this.mImportPlaylists))
            {
                /* Get the Songbird playlist ID. */
                playlistID = ITDB.getSBIDFromITID(this.mITunesLibID,
                                                  iTunesPlaylistID);

                /* If the importer data format version is less than 2, try   */
                /* getting the Songbird playlist ID from the iTunes playlist */
                /* name.                                                     */
                if (!playlistID && (this.mVersionDR.intValue < 2))
                {
                    playlistID = ITDB.getSBPlaylistIDFromITName(playlistName);
                    if (playlistID)
                    {
                        ITDB.mapID(this.mITunesLibID,
                                   iTunesPlaylistID,
                                   playlistID);
                    }
                }

                /* Get the Songbird playlist. */
                if (playlistID)
                {
                    try
                    {
                        this.mPlaylist =
                                        this.mLibrary.getItemByGuid(playlistID);
                    }
                    catch (e)
                    {
                        this.mPlaylist = null;
                    }
                }
            }

            /* Transition to next state. */
            state++;
        }

        /* Check for dirty playlist. */
        if (state == 4)
        {
            if (importPlaylist && this.mPlaylist)
                dirtyPlaylist = this.isDirtyPlaylist();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Determine what import action to take. */
        if (state == 5)
        {
            /* If not importing playlists, keep current ones.   */
            /* Otherwise, determine what import action to take. */
            if (!this.mImportPlaylists)
            {
                action = "keep";
            }
            else if (importPlaylist)
            {
                /* Default to replacing playlist. */
                action = "replace";

                /* Check for a dirty Songbird playlist.  If it's dirty, */
                /* query the user for the proper action to take.        */
                if (this.mPlaylist && dirtyPlaylist)
                    action = this.getDirtyPlaylistAction(playlistName);
            }

            /* Transition to next state. */
            state++;
        }

        /* Get Songbird playlist. */
        if (state == 6)
        {
            /* Create a fresh Songbird playlist if replacing. */
            if (importPlaylist && (action == "replace"))
            {
                /* Delete Songbird playlist if present. */
                if (this.mImport && this.mPlaylist)
                    this.mLibrary.remove(this.mPlaylist);

                /* Create the playlist. */
                if (this.mImport)
                {
                    this.mPlaylist = this.mLibrary.createMediaList("simple");
                    this.mPlaylist.name = playlistName;
                    playlistID = this.mPlaylist.guid;
                    ITDB.mapID(this.mITunesLibID, iTunesPlaylistID, playlistID);
                }
            }

            /* Transition to next state. */
            state++;
        }

        /* Process the playlist items. */
        if (state == 7)
        {
            if (importPlaylist)
                this.processPlaylistItems(action);
            else if (hasPlaylistItems)
                this.mXMLParser.skipNextElement();
            if (!ITReq.isCallPending())
                state++;
        }

        /* Delete playlist if it's empty. */
        if (state == 8)
        {
            /* Delete playlist if it's empty. */
            if (this.mImport && this.mPlaylist)
            {
                if (this.mPlaylist.isEmpty)
                {
                    this.mLibrary.remove(this.mPlaylist);
                    this.mPlaylist = null;
                }
            }

            /* Transition to next state. */
            state++;
        }

        /* Compute and store the Songbird playlist signature. */
        if (state == 9)
        {
            if (this.mImport && this.mPlaylist && (action != "keep"))
                sigGen = this.generateSBPlaylistSig(this.mPlaylist);
            else
                sigGen = null;
            if (!ITReq.isCallPending())
            {
                if (sigGen)
                {
                    signature = sigGen.getSignature();
                    sigGen.storeSignature(playlistID, signature);
                }
                state++;
            }
        }

        /* Complete playlist processing. */
        if (state == 10)
        {
            /* Clear current playlist object. */
            this.mPlaylist = null;

            /* Transition to next state. */
            state++;
        }

        /* End state. */
        if (state >= 11)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ctx.playlistInfoTable = playlistInfoTable;
        ctx.playlistName = playlistName;
        ctx.iTunesPlaylistID = iTunesPlaylistID;
        ctx.importPlaylist = importPlaylist;
        ctx.action = action;
        ctx.hasPlaylistItems = hasPlaylistItems;
        ctx.dirtyPlaylist = dirtyPlaylist;
        ITReq.exitFunction();
    },


    /*
     * isDirtyPlaylist
     *
     *   <--                        True if the current Songbird playlist is
     *                              dirty.
     *
     *   This function checks if the current Songbird playlist has been changed
     * since the last time it was imported from iTunes.  If it has, this
     * function returns true; otherwise, it returns false.
     */

    isDirtyPlaylist: function()
    {
        var                         ctx;
        var                         state;

        var                         sigGen;
        var                         signature;
        var                         storedSignature;
        var                         dirtyPlaylist;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;

        /* Compute the playlist signature. */
        if (state == 1)
        {
            sigGen = this.generateSBPlaylistSig(this.mPlaylist);
            if (!ITReq.isCallPending())
            {
                signature = sigGen.getSignature();
                state++;
            }
        }

        /* Check computed signature against stored signature. */
        if (state == 2)
        {
            /* Get the stored signature. */
            storedSignature = sigGen.retrieveSignature(this.mPlaylist.guid);

            /* If the computed signature is not the same as */
            /* the stored signature, the playlist is dirty. */
            if (signature != storedSignature)
                dirtyPlaylist = true;
            else
                dirtyPlaylist = false;

            /* Transition to next state. */
            state++;
        }

        /* End state. */
        if (state >= 3)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();

        return (dirtyPlaylist);
    },


    /*
     * getDirtyPlaylistAction
     *
     *   --> playlistName           Playlist displayable name.
     *
     *   <--                        Action to take with playlist.
     *
     *   This function returns the action to take for processing the dirty
     * playlist with the displayable name specified by playlistName.
     *   This function presents a dialog box querying the user for the action to
     * take.  If the user selects to apply the action to all dirty playlists,
     * this function will not query the user again.
     */

    getDirtyPlaylistAction: function(playlistName)
    {
        var                         action;
        var                         applyAll;

        /* If an action has been set for all playlists, just return it. */
        if (this.mDirtyPlaylistAction)
            return (this.mDirtyPlaylistAction);

        /* Send a dirty playlist event and get the action to take. */
        applyAll = {};
        action = this.mListener.onDirtyPlaylist(playlistName, applyAll);
        applyAll = applyAll.value;

        /* If the user selected to apply action */
        /* to all playlists, save the action.   */
        if (applyAll)
            this.mDirtyPlaylistAction = action;

        return (action);
    },


    /*
     * getPlaylistInfo
     *
     *   <-- playlistInfoTable      Object containing playlist info.
     *
     *   <--                        True if playlist items available.
     *
     *   This function parses information from an iTunes playlist and returns it
     * in the object specified by playlistInfoTable.  If the iTunes playlist
     * contains playlist items, this function returns true.  This function will
     * not parse the playlist items.
     */

    getPlaylistInfo: function(playlistInfoTable)
    {
        var                         tag = {};
        var                         tagPreText = {};
        var                         keyName;
        var                         keyValue;
        var                         hasPlaylistItems = false;
        var                         done;

        /* Get the playlist info. */
        done = false;
        while (!done)
        {
            /* Get the next tag. */
            this.mXMLParser.getNextTag(tag, tagPreText);

            /* If it's a "key" tag, process the playlist info. */
            if (tag.value == "key")
            {
                /* Get the key name. */
                this.mXMLParser.getNextTag(tag, tagPreText);
                keyName = tagPreText.value;

                /* If the key is "Playlist Items", the playlist */
                /* info is done.  Otherwise, get the key data.  */
                if (keyName == "Playlist Items")
                {
                    hasPlaylistItems = true;
                    done = true;
                }
                else
                {
                    /* Get the key value.  If the next tag is */
                    /* empty, use its name as the key value.  */
                    this.mXMLParser.getNextTag(tag, tagPreText);
                    if (tag.value.charCodeAt(tag.value.length - 1) ==
                        this.mSlashCharCode)
                    {
                        keyValue = tag.value.substring(0, tag.value.length - 1);
                    }
                    else
                    {
                        this.mXMLParser.getNextTag(tag, tagPreText);
                        keyValue = tagPreText.value;
                    }

                    /* Save the playlist info. */
                    playlistInfoTable[keyName] = keyValue;
                }
            }

            /* Otherwise, if it's a "/dict" tag, the playlist info is done. */
            else if (tag.value == "/dict")
            {
                done = true;
            }
        }

        return (hasPlaylistItems);
    },


    /*
     * processPlaylistItems
     *
     *   --> action                 Action to take for playlist items.
     *       "Keep"                 Keep Songbird playlist as-is.
     *       "Replace"              Replace Songbird playlist with iTunes.
     *       "Merge"                Merge Songbird playlist with iTunes.
     *
     *   This function processes the items in the current playlist taking the
     * action specified by action for each.
     */

    processPlaylistItems: function(action)
    {
        var                         ctx;
        var                         state;
        var                         tag = {};
        var                         tagPreText = {};
        var                         track;
        var                         trackID;
        var                         trackGUID;
        var                         updateSBPlaylist;
        var                         done;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 0;

        /* Get the function context. */
        state = ctx.state;

        /* Check if the Songbird playlist should be updated. */
        if (this.mImport && (action != "keep"))
            updateSBPlaylist = true;
        else
            updateSBPlaylist = false;

        /* Process each playlist item. */
        done = false;
        while (state == 0)
        {
            /* Process add playlist track batch if it's full. */
            if (this.addPlaylistTrackBatchFull())
            {
                this.addPlaylistTrackBatchProcess();
                if (ITReq.isCallPending())
                    break;
            }

            /* Get the next tag. */
            this.mXMLParser.getNextTag(tag, tagPreText);

            /* If it's a "dict" tag, process the playlist item. */
            /* If it's a "/array" tag, there are no more items. */
            if (tag.value == "dict")
            {
                /* Skip the key and integer tags  */
                /* and get the track ID and GUID. */
                this.mXMLParser.getNextTag(tag, tagPreText);
                this.mXMLParser.getNextTag(tag, tagPreText);
                this.mXMLParser.getNextTag(tag, tagPreText);
                this.mXMLParser.getNextTag(tag, tagPreText);
                trackID = tagPreText.value;
                if (updateSBPlaylist)
                    trackGUID = this.mTrackIDMap[trackID];

                /* Add the iTunes track persistent ID */
                /* to the iTunes library signature.   */
                /*zzz get ID value. */
                this.mITunesLibSig.update("Persistent ID");

                /* Add the track to the playlist. */
                if (updateSBPlaylist && trackGUID)
                {
                    try
                    {
                        track = this.mLibrary.getItemByGuid(trackGUID);
                    }
                    catch (e)
                    {
                        track = null;
                    }
                    if (track)
                        this.addPlaylistTrackBatchAdd(this.mPlaylist, track);
                }
            }
            else if (tag.value == "/array")
            {
                done = true;
            }

            /* Check if the playlist items are done. */
            if (done)
                state++;

            /* Check if it's time to yield. */
            if (ITReq.shouldYield())
            {
                /* Update status. */
                ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                                     / this.mXMLFile.fileSize;

                /* Yield. */
                break;
            }
        }

        /* Process the remaining add playlist track batch. */
        if (state == 1)
        {
            if (!this.addPlaylistTrackBatchEmpty())
                this.addPlaylistTrackBatchProcess();
            if (!ITReq.isCallPending())
                state++;
        }

        /* End state. */
        if (state >= 2)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ITReq.exitFunction();
    },


    /*
     * addPlaylistTrackBatchAdd
     *
     *   --> playlist               Playlist to which to add tracks.
     *   --> track                  Track to add.
     *
     *   This function adds the track specified by track to be added to the
     * playlist specified by playlist.  This function adds the track to the list
     * of tracks to add.  The tracks are added to the playlist when
     * addPlaylistTrackBatchProcess is invoked.
     */

    addPlaylistTrackBatchAdd: function(playlist, track)
    {
        /* Create an add playlist track list array if none present. */
        if (!this.mAddPlaylistTrackList)
        {
            this.mAddPlaylistTrackList =
                    Components.
                        classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].
                        createInstance(Components.interfaces.nsIMutableArray);
        }

        /* Add the track to the list. */
        this.mAddPlaylistTrackPlaylist = playlist;
        this.mAddPlaylistTrackList.appendElement(track, false);
    },


    /*
     * addPlaylistTrackBatchProcess
     *
     *   This function processes the batch of tracks to be added to the
     * playlist.
     */

    addPlaylistTrackBatchProcess: function()
    {
        var                         i;

        /* Add the tracks to the playlist. */
        this.mAddPlaylistTrackPlaylist.addSome
                                    (this.mAddPlaylistTrackList.enumerate());
        this.mAddPlaylistTrackPlaylist = null;
        this.mAddPlaylistTrackList = null;
    },


    /*
     * addPlaylistTrackBatchFull
     *
     *   <--                        True if add playlist track batch is full.
     *
     *   This function returns true if the add playlst track batch is full and
     * needs to be processed.
     */

    addPlaylistTrackBatchFull: function()
    {
        if (   this.mAddPlaylistTrackList
            && (this.mAddPlaylistTrackList.length >= this.addTrackBatchSize))
        {
            return (true);
        }
        else
        {
            return (false);
        }
    },


    /*
     * addPlaylistTrackBatchEmpty
     *
     *   <--                        True if add playlist track batch is empty.
     *
     *   This function returns true if there are no tracks to be processed in
     * the add playlist track batch.
     */

    addPlaylistTrackBatchEmpty: function()
    {
        if (   !this.mAddPlaylistTrackList
            || (this.mAddPlaylistTrackList.length == 0))
        {
            return (true);
        }
        else
        {
            return (false);
        }
    },


    /***************************************************************************
     *
     * Importer signature generation functions.
     *
     **************************************************************************/

    /*
     * generateSBPlaylistSig
     *
     *   --> playlist               Songbird playlist.
     *
     *   <--                        Signature generator.
     *
     *   This function generates a signature for the Songbird playlist specified
     * by playlist and returns a signature generator object for it.
     */

    generateSBPlaylistSig: function(playlist)
    {
        var                         ctx;
        var                         state;
        var                         sigGen;
        var                         signature = null;
        var                         i, j;

        /* Initialize the function context. */
        ctx = ITReq.enterFunction();
        if (!ctx.state)
            ctx.state = 1;

        /* Get the function context. */
        state = ctx.state;
        sigGen = ctx.sigGen;
        entries = ctx.entries;
        rowCount = ctx.rowCount;
        i = ctx.i;

        /* Create a signature generator. */
        if (state == 1)
        {
            sigGen = new ITSig();
            state++;
        }

        /* Generate a signature over all entries. */
        if (state == 2)
        {
            /* Initialize loop. */
            i = 0;
            state++;
        }
        while (state == 3)
        {
            /* Check look condition. */
            if (!(i < playlist.length))
            {
                state++;
                break;
            }

            /* Update the signature with the media GUID. */
            sigGen.update(playlist.getItemByIndex(i).guid);

            /* Loop. */
            i++;

            /* Check if it's time to yield. */
            if (ITReq.shouldYield())
            {
                /* Update status. */
                ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                                     / this.mXMLFile.fileSize;

                /* Yield. */
                break;
            }
        }

        /* End state. */
        if (state >= 4)
            ITReq.completeFunction();

        /* Update the function context. */
        ctx.state = state;
        ctx.sigGen = sigGen;
        ctx.i = i;
        ITReq.exitFunction();

        return (sigGen);
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component module services.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * NSGetModule
 *
 *   This function returns the component module.
 */

function NSGetModule(comMgr, fileSpec)
{
    return (ComponentModule);
}


/*
 * Component module class.
 */

var ComponentModule =
{
    /*
     * registerSelf
     *
     *   This function registers the component module.
     */

    registerSelf: function(compMgr, fileSpec, location, type)
    {
        var                         categoryManager;

        /* Register the component factory. */
        compReg = compMgr.QueryInterface
                                (Components.interfaces.nsIComponentRegistrar);
        compReg.registerFactoryLocation(CompConfig.cid,
                                        CompConfig.className,
                                        CompConfig.contractID,
                                        fileSpec,
                                        location,
                                        type);
        /* Register to receive "app-startup" events. */
        categoryManager =
                    Components.classes
                        ["@mozilla.org/categorymanager;1"].
                        getService(Components.interfaces.nsICategoryManager);
        categoryManager.addCategoryEntry("app-startup",
                                         CompConfig.className,
                                         "service," + CompConfig.contractID,
                                         true,
                                         true);
    },


    /*
     * unregisterSelf
     *
     *   This function unregisters the component module.
     */

    unregisterSelf: function(compMgr, location, type)
    {
        var                         categoryManager;

        /* Unregister to receive "app-startup" events. */
        categoryManager =
                    Components.classes
                        ["@mozilla.org/categorymanager;1"].
                        getService(Components.interfaces.nsICategoryManager);
        categoryManager.deleteCategoryEntry("app-startup",
                                            CompConfig.contractID,
                                            true);
    },


    /*
     * getClassObject
     *
     *   This function returns a factory for the component.
     */

    getClassObject: function(compMgr, classID, interfaceID)
    {
        /* Check for supported interfaces. */
        if(!interfaceID.equals(Components.interfaces.nsIFactory))
            throw(Components.results.NS_ERROR_NOT_IMPLEMENTED);

        /* Check for supported component types. */
        if(!classID.equals(CompConfig.cid))
            throw(Components.results.NS_ERROR_NO_INTERFACE);

        return(ComponentFactory);
    },


    /*
     * canUnload
     *
     *   Not sure what this does.
     */

    canUnload: function(compMgr)
    {
        return true;
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * Component factory.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * Component factory class.
 */

var ComponentFactory =
{
    /*
     * createInstance
     *
     *   --> outer                  ???
     *   --> interfaceID            ID of interface instance to create.
     *
     *   This function creates an instance of the component providing the
     * interface specified by interfaceID.
     */

    createInstance: function(outer, interfaceID)
    {
        var                         instance;
        var                         interfaceSupported = false;
        var                         i;

        /*zzz?*/
        if (outer != null)
            throw (Components.results.NS_ERROR_NO_AGGREGATION);

        /* Check for supported interfaces. */
        for (i = 0; i < CompConfig.ifList.length; i++)
        {
            if (interfaceID.equals(CompConfig.ifList[i]))
            {
                interfaceSupported = true;
                break;
            }
        }
        if (!interfaceSupported)
            throw Components.results.NS_ERROR_INVALID_ARG;

        /* Create an instance of the component. */
        instance = (new Component()).QueryInterface(interfaceID);

        return (instance);
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * XML parser services.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * ITXMLParser
 *
 *   This function is the constructor for the XML parser.
 */

function ITXMLParser(filePath)
{
    var                         fileInputStream;

    /* Initialize a file object. */
    this.mFile = Components.classes["@mozilla.org/file/local;1"].
                            createInstance(Components.interfaces.nsILocalFile);
    this.mFile.initWithPath(filePath);

    /* Initialize a file input stream. */
    fileInputStream = Components.
                    classes["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Components.interfaces.nsIFileInputStream);
    fileInputStream.init(this.mFile, 1, 0, 0);

    /* Get a seekable input stream. */
    this.mSeekableStream = fileInputStream.QueryInterface
                                    (Components.interfaces.nsISeekableStream);

    /* Initialize a scriptable input stream. */
    this.mInputStream =
        Components
            .classes["@mozilla.org/intl/converter-input-stream;1"]
            .createInstance(Components.interfaces.nsIConverterInputStream);
    this.mInputStream.init(fileInputStream,
                           "UTF-8",
                           this.readSize,
                           Components.interfaces.nsIConverterInputStream
                                               .DEFAULT_REPLACEMENT_CHARACTER);
}


/*
 * ITXMLParser prototype object.
 */

ITXMLParser.prototype.constructor = ITXMLParser;
ITXMLParser.prototype =
{
    /***************************************************************************
     *
     * XML parser configuration.
     *
     **************************************************************************/

    /*
     * readSize                     Number of bytes to read when filling stream
     *                              buffer.
     */

    readSize: 10000,


    /***************************************************************************
     *
     * XML parser fields.
     *
     **************************************************************************/

    /*
     * mFile                        File from which to read.
     * mSeekableStream              Seekable stream from which to read.
     * mInputStream                 Input stream from which to read.
     * mBuffer                      Temporary buffer to hold data.
     * mTagList                     List of available tags.
     * mNextTagIndex                Index of next tag to get from tag list.
     */

    mFile: null,
    mSeekableStream: null,
    mInputStream: null,
    mBuffer: "",
    mTagList: [],
    mNextTagIndex: 0,


    /***************************************************************************
     *
     * Public XML parser methods.
     *
     **************************************************************************/

    /*
     * close
     *
     *   This function closes the XML parser.
     */

    close: function()
    {
        /* Close the input stream. */
        this.mInputStream.close();
    },


    /*
     * getNextTag
     *
     *   <-- tag.value              Tag string.
     *   <-- tagPreText.value       Text before tag.
     *
     *   This function gets the next tag and returns it in tag.value.  The text
     * between the previous tag and the returned tag is returned in
     * tagPreText.value.
     */

    getNextTag: function(tag, tagPreText)
    {
        var                         tagSplit;

        /* If the end of the tag list is reached, read more tags. */
        if (this.mNextTagIndex >= this.mTagList.length)
        {
            this.mTagList.length = 0;
            this.mNextTagIndex = 0;
            this.readTags();
        }

        /* Return the next tag. */
        if (this.mTagList.length > 0)
        {
            tagSplit = this.mTagList[this.mNextTagIndex].split("<");
            this.mNextTagIndex++;
            tag.value = tagSplit[1];
            tagPreText.value = this.decodeEntities(tagSplit[0]);
        }
        else
        {
            tag.value = null;
            tagPreText.value = null;
        }
    },


    /*
     * skipNextElement
     *
     *   This function skips the next element and all its descendents.
     */

    mSlashCharCode: "/".charCodeAt(0),

    skipNextElement: function()
    {
        var                         tag = {};
        var                         tagPreText = {};
        var                         depth = 0;

        /* Loop down through the next element until its end. */
        do
        {
            /* Get the next tag. */
            this.getNextTag(tag, tagPreText);

            /* If the tag starts with a "/", decrement the depth. */
            if (tag.value.charCodeAt(0) == this.mSlashCharCode)
            {
                depth--;
            }

            /* Otherwise, if the tag does not end with a */
            /* "/" (empty element), increment the depth. */
            else if (tag.value.charCodeAt(tag.value.length - 1) !=
                     this.mSlashCharCode)
            {
                depth++;
            }
        } while (depth > 0);
    },


    /*
     * tell
     *
     *   <--                        Current offset within the input stream.
     *
     *   This function returns the current offset within the input stream.
     */

    tell: function()
    {
        var                         offset;

        /* Get the current input stream offset   */
        /* and subtract the current buffer size. */
        offset = this.mSeekableStream.tell() - this.mBuffer.length;

        return (offset);
    },


    /*
     * getFile
     *
     *   <--                        Data file.
     *
     *   This function returns the nsIFile object being parsed.
     */

    getFile: function()
    {
        return (this.mFile.QueryInterface(Components.interfaces.nsIFile));
    },


    /***************************************************************************
     *
     * Private XML parser methods.
     *
     **************************************************************************/

    /*
     * readTags
     *
     *   This function reads data from the input stream and divides it into the
     * tag list, mTagList.
     */

    readTags: function()
    {
        var                     readData;
        var                     readCount;

        /* Read more data into buffer. */
        readData = {};
        readCount = this.mInputStream.readString(this.readSize, readData);
        if (readCount > 0)
            this.mBuffer += readData.value;

        /* Read from the stream until a tag is found. */
        while ((this.mTagList.length == 0) && (this.mBuffer))
        {
            /* Split stream buffer at the tag ends. */
            this.mTagList = this.mBuffer.split(">");

            /* Pop off remaining data after last tag end.  If no */
            /* tag was found, read more data into the buffer.    */
            if (this.mTagList.length > 1)
            {
                this.mBuffer = this.mTagList.pop();
            }
            else
            {
                /* No tags were found. */
                this.mTagList = [];

                /* Read more data.  If no more data */
                /* is available, empty buffer.      */
                readData = {};
                readCount = this.mInputStream.readString(this.readSize,
                                                         readData);
                if (readCount > 0)
                    this.mBuffer += readData.value;
                else
                    this.mBuffer = "";
            }
        }
    },


    /*
     * decodeEntitites
     *
     *   --> str                String to decode.
     *
     *   <--                    String with decoded entities.
     *
     *   This function searches for XML entities within the string specified by
     * str, decodes and replaces them, and returns the resulting string.
     */

    decodeEntities: function(str)
    {
        return (str.replace(/&#([0-9]*);/g, this.replaceEntity));
    },


    /*
     * replaceEntity
     *
     *   --> entityStr          Entity string.
     *   --> entityVal          Entity value.
     *
     *   <--                    Decoded entity string.
     *
     *   This function is used as a replacement function for the string object
     * replace method.  This function replaces XML entity strings.  The entity
     * string is specified by entityStr, and the entity value is specified by
     * entityVal.  This function returns the string represented by the entity.
     */

    replaceEntity: function(entityStr, entityVal)
    {
        return (String.fromCharCode(entityVal));
    }
}


/*******************************************************************************
 *******************************************************************************
 *
 * Database services.
 *
 *******************************************************************************
 ******************************************************************************/

var ITDB =
{
    /***************************************************************************
     *
     * Database services fields.
     *
     **************************************************************************/

    /*
     * mLibraryManager              Media library manager component.
     * mLibrary                     Media library component.
     * mDBQuery                     Database query component.
     * mAsyncDBQuery                Asynchronous database query component.
     * mAsyncDBQueryResetPending    True if a reset request is pending for the
     *                              asynchronous database query component.
     */

    mLibraryManager: null,
    mLibrary: null,
    mDBQuery: null,
    mAsyncDBQuery: null,
    mAsyncDBQueryResetPending: false,


    /***************************************************************************
     *
     * Database services functions.
     *
     **************************************************************************/

    /*
     * activate
     *
     *   This function activates the database services.
     */

    activate: function()
    {
        /* Get the media library manager service          */
        /* component and create a media library instance. */
        this.mLibraryManager =
            Components.
                classes["@songbirdnest.com/Songbird/library/Manager;1"].
                getService(Components.interfaces.sbILibraryManager);
        this.mLibrary = this.mLibraryManager.getLibrary
                                        (this.mLibraryManager.mainLibrary.guid);

        /* Create a Songbird library database query component. */
        this.mDBQuery = Components.
                        classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].
                        createInstance(Components.interfaces.sbIDatabaseQuery);
        this.mDBQuery.setAsyncQuery(false);
        this.mDBQuery.setDatabaseGUID("songbird");

        /* Create an asynchronous Songbird library database query component. */
        this.mAsyncDBQuery =
                    Components.
                        classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].
                        createInstance(Components.interfaces.sbIDatabaseQuery);
        this.mAsyncDBQuery.setAsyncQuery(true);
        this.mAsyncDBQuery.setDatabaseGUID("songbird");

        /* Create the iTunes ID map database table. */
        this.mDBQuery.addQuery(  "CREATE TABLE itunes_id_map "
                               + "(itunes_id TEXT UNIQUE NOT NULL, "
                               + "songbird_id TEXT UNIQUE NOT NULL)");
        this.mDBQuery.execute();
        this.mDBQuery.waitForCompletion();
    },


    /*
     * sync
     *
     *   <--                        True if synchronized.
     *
     *   This function synchronizes database operations, ensuring that any
     * asynchronous operations are complete.  If the database is synchronized,
     * this function returns true; otherwise, it returns false.
     */

    sync: function()
    {
        var                         executing;

        /* Get executing condition. */
        executing = this.mAsyncDBQuery.isExecuting();

        /* Reset the asynchronous database query if a reset is pending. */
        if (!executing && this.mAsyncDBQueryResetPending)
        {
            this.mAsyncDBQuery.resetQuery();
            this.mAsyncDBQueryResetPending = false;
        }

        /* Start executing any asynchronous database queries. */
        if (!executing && (this.mAsyncDBQuery.getQueryCount() > 0))
        {
            this.mAsyncDBQuery.execute();
            executing = true;
            this.mAsyncDBQueryResetPending = true;
        }

        return (!executing);
    },


    /*
     * mapID
     *
     *   --> iTunesLibID            iTunes library ID.
     *   --> iTunesID               iTunes ID.
     *   --> songbirdID             Songbird ID.
     *
     *   This function maps the iTunes ID specified by iTunesLibiD and iTunesID
     * to the Songbird ID specified by songbirdID.  These IDs may be for either
     * tracks or playlists.
     */

    mapID: function(iTunesLibID, iTunesID, songbirdID)
    {
        /* Add the ID mapping to the database. */
        this.mAsyncDBQuery.addQuery
                          (  "INSERT OR REPLACE INTO itunes_id_map"
                           + "(itunes_id, songbird_id) VALUES"
                           + "(\"" + iTunesLibID + iTunesID + "\", "
                           + "\"" + songbirdID +"\")");
    },


    /*
     * getSBIDFromITID
     *
     *   --> iTunesLibID            iTunes library ID.
     *   --> iTunesID               iTunes ID.
     *
     *   <--                        Songbird ID mapped to iTunesID.
     *
     *   This function returns the Songbird ID corresponding to the iTunes ID
     * specified by iTunesLibID and iTunesID.  These IDs may be for either
     * tracks or playlists.
     *
     *   This function currently ignores iTunesLibID.
     */

    getSBIDFromITID: function(iTunesLibID, iTunesID)
    {
        var                         songbirdID;

        /* Query the database for the Songbird ID. */
        this.mDBQuery.resetQuery();
        this.mDBQuery.addQuery(  "SELECT songbird_id FROM itunes_id_map WHERE "
                               + "itunes_id = \""
                               + iTunesLibID + iTunesID + "\"");
        this.mDBQuery.execute();
        this.mDBQuery.waitForCompletion();

        /* Get the Songbird ID. */
        songbirdID = this.mDBQuery.getResultObject().getRowCell(0, 0);

        return (songbirdID);
    },


    /*
     * getSBPlaylistIDFromITName
     *
     *   --> iTunesName             iTunes playlist name.
     *
     *   <--                        Songbird playlist ID.
     *
     *   This function returns the Songbird playlist ID corresponding to the
     * iTunes playlist name specified by iTunesName.
     */

    getSBPlaylistIDFromITName: function(iTunesName)
    {
        var                         playlistList;
        var                         playlist;
        var                         playlistID = null;
        var                         i;

        /* Search for a playlist with specified displayable name. */
        playlistList = this.getSBPlaylistList(this.mLibrary);
        for (i = 0; i < playlistList.length; i++)
        {
            playlist = playlistList[i];
            if (playlist.name == iTunesName)
            {
                playlistID = playlist.guid;
                break;
            }
        }

        return (playlistID);
    },


    /*
     * getSBPlaylistList
     *
     *   --> library                Library for which to get playlists.
     *
     *   <--                        List of playlists.
     *
     *   This function returns the list of playlists contained in the library
     * specified by library.
     */

    getSBPlaylistList: function(library)
    {
        var                         listener;

        /* Set up an enumeration listener. */
        listener =
        {
            items: [],

            onEnumerationBegin: function() { },
            onEnumerationEnd: function() { },

            onEnumeratedItem: function(list, item)
            {
                this.items.push(item);
            }
        }

        /* Enumerate all lists in the library. */
        library.enumerateItemsByProperty
                ("http://songbirdnest.com/data/1.0#isList",
                 "1",
                 listener /*,
                 Components.interfaces.sbIMediaList.ENUMERATIONTYPE_LOCKING*/);

        return (listener.items);
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * Signature generator class.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * ITSig
 *
 *   This function is the constructor for the generator class.
 */

function ITSig()
{
    /* Create a string converter. */
    this.mStrConverter =
        Components.
            classes["@mozilla.org/intl/scriptableunicodeconverter"].
            createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    this.mStrConverter.charset = "UTF-8";

    /* Create a hashing processor. */
    this.mHashProc = Components.classes["@mozilla.org/security/hash;1"].
                            createInstance(Components.interfaces.nsICryptoHash);
    this.mHashProc.init(this.mHashProc[this.hashType]);

    /* Create a signature database query component. */
    this.mDBQuery = Components.
                        classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].
                        createInstance(Components.interfaces.sbIDatabaseQuery);
    this.mDBQuery.setAsyncQuery(false);
    this.mDBQuery.setDatabaseGUID(this.dbGUID);

    /* Ensure the signature database table exists. */
    this.mDBQuery.addQuery(  "CREATE TABLE " + this.dbTable + " "
                           + "(id TEXT UNIQUE NOT NULL, "
                           + " signature TEXT NOT NULL)");
    this.mDBQuery.execute();
    this.mDBQuery.waitForCompletion();
    this.mDBQuery.resetQuery();
};


/*
 * Signature generator prototype.
 */

ITSig.prototype.constructor = ITSig;
ITSig.prototype =
{
    /***************************************************************************
     *
     * Signature generator configuration.
     *
     **************************************************************************/

    /*
     * hashType                     Type of hash for signature.
     * dbGUID                       Signature database GUID.
     * dbTable                      Signature database table.
     */

    hashType: CompConfig.sigHashType,
    dbGUID: CompConfig.sigDBGUID,
    dbTable: CompConfig.sigDBTable,


    /***************************************************************************
     *
     * Signature generator fields.
     *
     **************************************************************************/

    /*
     * mStrConverter                String converter component.
     * mHashProc                    Signature hashing processor component.
     * mDBQuery                     Signature database query component.
     * mSignature                   Generated signature.
     */

    mStrConverter: null,
    mHashProc: null,
    mDBQuery: null,
    mSignature: null,


    /***************************************************************************
     *
     * Signature generator functions.
     *
     **************************************************************************/

    /*
     * update
     *
     *   --> strData                Data with which to update signature.
     *
     *   This function updates the signature with the string data specified by
     * data.
     */

    update: function(strData)
    {
        var                         byteData;

        /* Convert the string data to byte data and update the signature. */
        byteData = this.mStrConverter.convertToByteArray(strData, {});
        this.mHashProc.update(byteData, byteData.length);
    },


    /*
     * getSignature
     *
     *   <--                        Finished signature.
     *
     *   This function finishes the signature generation and returns the result.
     */

    getSignature: function()
    {
        var                         hashBinData;
        var                         hashHexStr;
        var                         charCode;
        var                         i;

        /* If a complete signature has been generated, return it. */
        if (this.mSignature)
            return (this.mSignature);

        /* Finish the hashing. */
        hashBinData = this.mHashProc.finish(false);

        /* Convert the hash binary data to a hex string. */
        hashHexStr = "";
        for (i = 0; i < hashBinData.length; i++)
        {
            charCode = hashBinData.charCodeAt(i);
            if (charCode < 16)
                hashHexStr += "0";
            hashHexStr += charCode.toString(16);
        }

        /* Save the computed signature. */
        this.mSignature = hashHexStr;

        return (hashHexStr);
    },


    /***************************************************************************
     *
     * Signature storage functions.
     *
     **************************************************************************/

    /*
     * storeSignature
     *
     *   --> id                     Signature ID.
     *   --> signature              Signature to store.
     *
     *   This function stores the signature specified by signature with the ID
     * specified by id.
     */

    storeSignature: function(id, signature)
    {
        /* Store signature in signature table. */
        this.mDBQuery.resetQuery();
        this.mDBQuery.addQuery(  "INSERT OR REPLACE INTO " + this.dbTable + " "
                               + "(id, signature) VALUES "
                               + "(\"" + id + "\", "
                               + "\"" + signature + "\")");
        this.mDBQuery.execute();
        this.mDBQuery.waitForCompletion();
        this.mDBQuery.resetQuery();
    },


    /*
     * retrieveSignature
     *
     *   --> id                     ID of signature to retrieve.
     *
     *   <--                        Stored signature.
     *
     *   This function returns the signature stored with the ID specified by id.
     */

    retrieveSignature: function(id)
    {
        var                         signature;

        /* Retrieve signature from signature table. */
        this.mDBQuery.resetQuery();
        this.mDBQuery.addQuery(  "SELECT signature FROM " + this.dbTable + " "
                               + "WHERE id = \"" + id + "\"");
        this.mDBQuery.execute();
        this.mDBQuery.waitForCompletion();
        signature = this.mDBQuery.getResultObject().getRowCell(0, 0);
        this.mDBQuery.resetQuery();

        return (signature);
    }
}


/*******************************************************************************
 *******************************************************************************
 *
 * Request processing services.
 *
 *******************************************************************************
 ******************************************************************************/

var ITReq =
{
    /***************************************************************************
     *
     * Request processing configuration.
     *
     **************************************************************************/

    /*
     * period                       Request processor execution period in
     *                              milliseconds.
     * pctCPU                       Request processor CPU execution percentage.
     */

    period: CompConfig.reqPeriod,
    pctCPU: CompConfig.reqPctCPU,


    /***************************************************************************
     *
     * Request processing fields.
     *
     **************************************************************************/

    /*
     * mQueue                       Request queue.
     * mTimer                       Request processor timer.
     * mCurrentReq                  Current request being processed.
     * mCtxStack                    Request function context stack.
     * mNextCtxIndex                Index of the next function context in stack.
     * mStartTime                   Start time of request in milliseconds.
     * mYieldThreshold              Execution time threshold in milliseconds
     *                              after which request should yield.
     */

    mQueue: [],
    mTimer: null,
    mCurrentReq: null,
    mCtxStack: [],
    mNextCtxIndex: 0,
    mStartTime: 0,
    mYieldThreshold: 10,


    /***************************************************************************
     *
     * Public request processing methods.
     *
     **************************************************************************/

    /*
     * initialize
     *
     *   This function initializes the request processing services.
     */

    initialize: function()
    {
        /* Create a request processing timer. */
        this.mTimer = new Timer();

        /* Compute the yield time threshold. */
        this.mYieldThreshold = (this.period * this.pctCPU) / 100;
    },


    /*
     * finalize
     *
     *   This function finalizes the request processing services.
     */

    finalize: function()
    {
        /* Dispose of the timer. */
        this.mTimer.cancel();
        this.mTimer = null;
    },


    /*
     * start
     *
     *   This function starts the processing of requests.  If request processing
     * was previously stopped, this function will resume processing.
     */

    start: function()
    {
        this.mTimer.cancel();
        this.mTimer.init(this, "process", null, this.period, 1);
    },


    /*
     * stop
     *
     *   This function stops processing of requests.  Processing may be later
     * resumed by calling start.
     */

    stop: function()
    {
        this.mTimer.cancel();
    },


    /*
     * issue
     *
     *   --> req                    Request to issue.
     *
     *   This function issues the request specified by req.
     */

    issue: function(req)
    {
        this.mQueue.push(req);
    },


    /*
     * enterFunction
     *
     *   <--                        Function request context.
     *
     *   This function is called when a request function is entered.  This
     * function returns the request context for the calling function.  If a
     * context is not available, this function creates a new one and pushes it
     * onto the request function context stack.
     */

    enterFunction: function()
    {
        var                         ctx = null;

        /* Get the next request function context in the stack. */
        if (this.mNextCtxIndex < this.mCtxStack.length)
            ctx = this.mCtxStack[this.mNextCtxIndex];

        /* If the context was not present in the stack, */
        /* create a new one and push it onto the stack. */
        if (!ctx)
        {
            ctx = {};
            this.mCtxStack.push(ctx);
        }

        /* Move to the next context in the stack. */
        this.mNextCtxIndex++;

        return (ctx);
    },


    /*
     * exitFunction
     *
     *   This function is called when a request function exits.
     */

    exitFunction: function()
    {
        this.mNextCtxIndex--;
    },


    /*
     * completeFunction
     *
     *   This function is called when a request function completes.  It pops the
     * request function context off of the stack.
     */

    completeFunction: function()
    {
        this.mCtxStack.pop();
    },


    /*
     * isCallPending
     *
     *   <-- true                   A request function call is pending.
     *       false                  A request function call is not pending.
     *
     *   If a request function call is pending from the caller of isCallPending,
     * isCallPending returns true; otherwise, it returns false.
     *
     *   A request function call is pending if the current request function
     * context is not the last one in the context stack.
     */

    isCallPending: function()
    {
        var pending;

        /* If there is no next request function */
        /* context, there are no calls pending. */
        if (this.mNextCtxIndex >= this.mCtxStack.length)
            pending = false;
        else
            pending = true;

        return (pending);
    },


    /*
     * shouldYield
     *
     *   <-- true                   Request function should yield.
     *
     *   This function determines whether the request should yield to let the
     * system get some processing time.  If it should yield, this function
     * returns true.
     *
     *   The request processing services monitor the amount of time spent by the
     * request.  If this time exceeds a certain threshold, then the request
     * should yield.
     */

    shouldYield: function()
    {
        var                         currentTime;

        /* Get the current time. */
        currentTime = (new Date()).getTime();

        /* Determine whether request should yield. */
        if ((currentTime - this.mStartTime) >= this.mYieldThreshold)
            return (true);
        else
            return (false);
    },


    /***************************************************************************
     *
     * Private request processing methods.
     *
     **************************************************************************/

    /*
     * process
     *
     *   This function processes requests in the queue.
     */

    process: function()
    {
        try { this.process1(); }
        catch (err) { Components.utils.reportError(err); }
    },
    process1: function()
    {
        var                         req;
        var                         endTime;
        var                         adjTime;

        /* Get the current request. */
        if (!this.mCurrentReq)
        {
            this.mCurrentReq = this.mQueue.pop();
            this.mNextCtxIndex = 0;
            this.mCtxStack = [];
        }

        /* Do nothing if no requests pending. */
        if (!this.mCurrentReq)
            return;

        /* Get the request start time. */
        this.mStartTime = (new Date()).getTime();

        /* Call the request function, completing */
        /* the request on any exceptions.        */
        try
        {
            this.mCurrentCtxIndex = 0;
            req = this.mCurrentReq;
            req.func.apply(null, req.args);
        }
        catch (e)
        {
            /* Dump exception. */
            Components.utils.reportError(e);

            /* Complete request. */
            this.mCurrentReq = null;
        }

        /* Determine the timer period adjustment. */
        {
            /* Get the end time. */
            endTime = (new Date()).getTime();

            /* Adjust the period by the amount of */
            /* excess time over the threshold.    */
            adjTime = endTime - this.mStartTime - this.mYieldThreshold;
            if (adjTime < 0)
                adjTime = 0;

            /* Limit the adjustment. */
            if (adjTime > 1000)
                adjTime = 1000;
        }

        /* Set the timer period. */
        this.mTimer.setDelay(this.period + adjTime);

        /* If request function call is not pending, the request is complete. */
        if (!this.isCallPending())
            this.mCurrentReq = null;
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * Status UI services.
 *
 *******************************************************************************
 ******************************************************************************/

var ITStatus =
{
    /***************************************************************************
     *
     * Status UI fields.
     *
     **************************************************************************/

    /*
     * mStatusDataRemote            Data remote for displaying status.
     * mStageText                   Stage status text.
     * mProgressText                Progress within stage text.
     * mProgress                    Progress within stage percentage.
     * mDone                        Done status.
     */

    mStatusDataRemote: null,
    mStageText: "",
    mProgressText: "",
    mProgress: 0,
    mDone: false,


    /***************************************************************************
     *
     * Public request processing methods.
     *
     **************************************************************************/

    /*
     * initialize
     *
     *   This function initializes the status UI services.
     */

    initialize: function()
    {
        /* Initialize the status data remote. */
        this.mStatusDataRemote =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mStatusDataRemote.init("faceplate.status.text", null);

        /* Reset the status. */
        this.reset();
    },


    /*
     * finalize
     *
     *   This function finalizes the status UI services.
     */

    finalize: function()
    {
        /* Release object references. */
        this.mStatusDataRemote = null;
    },


    /*
     * bringToFront
     *
     *   This function brings the status UI to the front.
     */

    bringToFront: function()
    {
    },


    /*
     * reset
     *
     *   This function resets the status UI.
     */

    reset: function()
    {
        /* Reset status variables. */
        this.mStageText = "";
        this.mProgressText = "";
        this.mProgress = 0;
        this.mDone = false;
    },


    /*
     * update
     *
     *   This function updates the status UI with the latest status.
     */

    update: function()
    {
        var                         statusText;

        /* Update the progress value. */
        this.mProgress = Math.floor(this.mProgress);

        /* Update status data remote. */
        statusText = this.mStageText;
        if (!this.mDone)
            statusText += " " + this.mProgressText + " " + this.mProgress + "%";
        this.mStatusDataRemote.stringValue = statusText;

        /* Log status. */
        Log("Status: " + statusText + "\n");
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * Timer services.
 *zzz too generic.  Confusing with nsITimer.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * Timer
 *
 *   This function is the constructor for the timer class.
 */

function Timer()
{
}

/* Set the constructor. */
Timer.prototype.constructor = Timer;

/* Define the timer class. */
Timer.prototype =
{
    /***************************************************************************
     *
     * Timer fields.
     *
     **************************************************************************/

    /*
     *   timer                      Timer component object.
     *   handlerObj                 Timer handler object.
     *   handlerName                Timer handler object function name.
     *   handlerArg                 Timer handler object function argument.
     */

    timer: null,
    handlerObj: null,
    handlerName: null,
    handlerArg: null,


    /***************************************************************************
     *
     * Timer interface functions.
     *
     **************************************************************************/

    /*
     * init
     *
     *   --> handlerObj             Handler object.
     *   --> handlerName            Handler function name.
     *   --> handlerArg             Handler function argument.
     *   --> delay                  Timer delay.
     *   --> type                   Type of timer.
     *
     *   This function initializes the timer of type specified by type to expire
     * after the delay specified in milliseconds by delay.  The values for type
     * are the same as for the nsITimer interface.
     *   The timer is handled by the object specified by handlerObj using the
     * object function whose name is specified by handlerName.  The handler
     * function is passed the argument specified by handlerArg.
     */

    init: function(handlerObj, handlerName, handlerArg, delay, type)
    {
        /* Create the timer component object. */
        timer = Components.classes["@mozilla.org/timer;1"].
                                createInstance(Components.interfaces.nsITimer);

        /* Set up the timer object. */
        this.timer = timer;
        this.handlerObj = handlerObj;
        this.handlerName = handlerName;
        this.handlerArg = handlerArg;

        /* Initialize the timer component object. */
        timer.init(this, delay, type);
    },


    /*
     * cancel
     *
     *   This function cancels the timer.
     */

    cancel: function()
    {
        if (this.timer)
            this.timer.cancel();
        this.timer = null;
    },


    /*
     * setDelay
     *
     *   --> delay                  Delay in milliseconds.
     *
     *   This function sets the timer delay to the value specified by delay.
     */

    setDelay: function(delay)
    {
        this.timer.delay = delay;
    },


    /***************************************************************************
     *
     * Timer observer interface functions.
     *
     **************************************************************************/

    /*
     * observe
     *
     *   --> subject                Event subject.
     *   --> topic                  Event topic.
     *   --> data                   Event data.
     *
     *   This function observes the timer event specified by subject, topic, and
     * data.
     */

    observe: function(subject, topic, data)
    {
        /* Handle the timer event. */
        this.handlerObj[this.handlerName](this.handlerArg);
    },


    /***************************************************************************
     *
     * Timer supports interface functions.
     *
     **************************************************************************/

    /*
     * QueryInterface
     *
     *   --> interfaceID            Requested interface.
     *
     *   NS_ERROR_NO_INTERFACE      Requested interface is not supported.
     *
     *   This function returns a timer object implementing the interface
     * specified by interfaceID.
     */

    QueryInterface: function(interfaceID)
    {
        /* Check for supported interfaces. */
        if (   !interfaceID.equals(Components.interfaces.nsISupports)
            && !interfaceID.equals(Components.interfaces.nsIObserver))
        {
            throw(Components.results.NS_ERROR_NO_INTERFACE);
        }

        return(this);
    }
};


/*******************************************************************************
 *******************************************************************************
 *
 * Logging services.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * Log
 *
 *   --> msg                    Message to log.
 *
 *   This function logs the message specified by msg.
 */

function Log(msg)
{
    LogSvc.Log(msg);
}


/*
 * LogSvc
 *
 *   Logging services object.
 */

var LogSvc =
{
    /***************************************************************************
     *
     * Logging services configuration.
     *
     **************************************************************************/

    /*
     * prefPrefix                   Prefix for preferences.
     */

    prefPrefix: CompConfig.prefPrefix + ".logging",


    /***************************************************************************
     *
     * Logging services fields.
     *
     **************************************************************************/

    /*
     * mInitialized                 True if services are initialized.
     * mLoggingEnabledPref          Logging enabled preference.
     * mEnabled                     True if logging is enabled.
     */

    mInitialized: false,
    mLoggingEnabledPref: null,
    mEnabled: false,


    /***************************************************************************
     *
     * Logging services functions.
     *
     **************************************************************************/

    /*
     * Initialize
     *
     *   This function initializes the logging services.  If the logging
     * services have already been initialized, this function does nothing.
     */

    Initialize: function()
    {
        /* Do nothing if already initialized. */
        if (this.mInitialized)
            return;

        /* Get the logging preferences. */
        this.mLoggingEnabledPref =
                        Components.
                                classes["@mozilla.org/preferences-service;1"].
                                getService(Components.interfaces.nsIPrefBranch);
        try
        {
            this.mEnabled =
                        this.mLoggingEnabledPref.getBoolPref(  "songbird."
                                                             + this.prefPrefix
                                                             + ".enabled");
        }
        catch (e)
        {
            this.mEnabled = false;
        }

        /* The logging services are now initialized. */
        this.mInitialized = true;
    },


    /*
     * Log
     *
     *   --> msg                    Message to log.
     *
     *   This function logs the message specified by msg.
     */

    Log: function(msg)
    {
        /* Ensure the logging services are initialized. */
        this.Initialize();

        /* Do nothing if logging is not enabled. */
        if (!this.mEnabled)
            return;

        /* Log message. */
        dump(msg);
    }
};


