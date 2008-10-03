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
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/GeneratorThread.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


/* *****************************************************************************
 *
 * iTunes importer component defs.
 *
 ******************************************************************************/

/* Component manager defs. */
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component configuration.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * className                    Name of component class.
 * cid                          Component CID.
 * contractID                   Component contract ID.
 * ifList                       List of external component interfaces.
 * categoryList                 List of component categories.
 *
 * iTunesGUIDProperty           iTunes GUID property for media items.
 *
 * dataFormatVersion            Data format version.
 *                                1: < extension version 2.0
 *                                2: extension version 2.0
 * prefPrefix                   Prefix for component preferences.
 *
 * addTrackBatchSize            Number of tracks to add at a time.
 *
 * sigHashType                  Hash type to use for signature computation.
 * sigDBGUID                    GUID of signature storage database.
 * sigDBTable                   Signature storage database table name.
 *
 * reqPeriod                    Request thread scheduling period in
 *                              milliseconds.
 * reqMaxPctCPU                 Request maximum thread CPU execution percentage.
 */

var CompConfig =
{
    className: "iTunes Library Importer",
    cid: Components.ID("{D6B36046-899A-4C1C-8A97-67ADC6CB675F}"),
    contractID: "@songbirdnest.com/Songbird/ITunesImporter;1",
    ifList: [ Components.interfaces.nsISupports,
              Components.interfaces.sbILibraryImporter,
              Components.interfaces.sbIJobProgressListener ],

    iTunesGUIDProperty: "http://songbirdnest.com/data/1.0#iTunesGUID",

    dataFormatVersion: 2,
    prefPrefix: "library_import.itunes",

    addTrackBatchSize: 300,

    sigHashType: "MD5",
    sigDBGUID: "songbird",
    sigDBTable: "itunes_signatures",

    reqPeriod: 33,
    reqMaxPctCPU: 60
};

CompConfig.categoryList =
[
    {
        category: "library-importer",
        entry:    CompConfig.className,
        value:    CompConfig.contractID
    }
];


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component.
 *
 *******************************************************************************
 ******************************************************************************/

/*
 * sbITunesImporter
 *
 *   This function is the constructor for the component.
 */

function sbITunesImporter()
{
}


/*
 * sbITunesImporter object.
 */

sbITunesImporter.prototype =
{
    /* Set the constructor. */
    constructor: sbITunesImporter,

    /***************************************************************************
     *
     * Importer configuration.
     *
     **************************************************************************/

    /*
     * classDescription             Description of component class.
     * classID                      Component class ID.
     * contractID                   Component contract ID.
     * _xpcom_categories            List of component categories.
     *
     * metaDataTable                Table of meta-data to extract from iTunes.
     * dataFormatVersion            Data format version.
     * localeBundlePath             Path to locale string bundle.
     * prefPrefix                   Prefix for component preferences.
     * addTrackBatchSize            Number of tracks to add at a time.
     */

    classDescription: CompConfig.className,
    classID: CompConfig.cid,
    contractID: CompConfig.contractID,
    _xpcom_categories: CompConfig.categoryList,

    metaDataTable:
    [
        { songbird: CompConfig.iTunesGUIDProperty, iTunes: "Persistent ID" },
        { songbird: SBProperties.albumArtistName,  iTunes: "Album Artist" },
        { songbird: SBProperties.albumName,        iTunes: "Album" },
        { songbird: SBProperties.artistName,       iTunes: "Artist" },
        { songbird: SBProperties.bitRate,          iTunes: "Bit Rate" },
        { songbird: SBProperties.bpm,              iTunes: "BPM" },
        { songbird: SBProperties.comment,          iTunes: "Comments" },
        { songbird: SBProperties.composerName,     iTunes: "Composer" },
        { songbird: SBProperties.contentMimeType,  iTunes: "Kind" },
        { songbird: SBProperties.discNumber,       iTunes: "Disc Number" },
        { songbird: SBProperties.duration,         iTunes: "Total Time",
          convertFunc: "_convertDuration" },
        { songbird: SBProperties.genre,            iTunes: "Genre" },
        { songbird: SBProperties.lastPlayTime,     iTunes: "Play Date UTC",
          convertFunc: "_convertDateTime" },
        { songbird: SBProperties.lastSkipTime,     iTunes: "Skip Date",
          convertFunc: "_convertDateTime" },
        { songbird: SBProperties.playCount,        iTunes: "Play Count" },
        { songbird: SBProperties.rating,           iTunes: "Rating",
          convertFunc: "_convertRating" },
        { songbird: SBProperties.sampleRate,       iTunes: "Sample Rate" },
        { songbird: SBProperties.skipCount,        iTunes: "Skip Count" },
        { songbird: SBProperties.totalDiscs,       iTunes: "Disc Count" },
        { songbird: SBProperties.totalTracks,      iTunes: "Track Count" },
        { songbird: SBProperties.trackName,        iTunes: "Name" },
        { songbird: SBProperties.trackNumber,      iTunes: "Track Number" },
        { songbird: SBProperties.year,             iTunes: "Year" }
    ],
    dataFormatVersion: CompConfig.dataFormatVersion,
    localeBundlePath: CompConfig.localeBundlePath,
    prefPrefix: CompConfig.prefPrefix,
    addTrackBatchSize: CompConfig.addTrackBatchSize,
    reqPeriod: CompConfig.reqPeriod,
    reqMaxPctCPU: CompConfig.reqMaxPctCPU,


    /***************************************************************************
     *
     * Importer fields.
     *
     **************************************************************************/

    /*
     * _osType                      OS type.
     * _listener                    Listener for import events.
     * _excludedPlaylists           List of excluded playlists.
     * _handleImportReqFunc         handleImportReq function with object
     *                              closure.
     * _xmlParser                   XML parser object.
     * _xmlFile                     XML file object being parsed.
     * _ioService                   IO service component.
     * _fileProtocolHandler         File protocol handler component.
     * _typeSniffer                 Mediacore Type Sniffer component.
     * _libraryManager              Media library manager component.
     * _library                     Media library component.
     * _playlist                    Current playlist component.
     * _trackIDMap                  Map of iTunes track IDs and Songbird track
     *                              UUIDs.
     * _trackCount                  Count of the number of tracks.
     * _nonExistentMediaCount       Count of the number of non-existent track
     *                              media files.
     * _unsupportedMediaCount       Count of the number of unsupported media
     *                              tracks.
     * _iTunesLibID                 ID of iTunes library to import.
     * _iTunesLibSig                iTunes library signature.
     * _import                      If true, import library into Songbird while
     *                              processing.
     * _importPlaylists             If true, import playlists into Songbird
     *                              while processing.
     * _dirtyPlaylistAction         Import action to take for all dirty
     *                              playlists.
     * _libraryFilePathPref         Import library file path preference.
     * _autoImportPref              Auto import preference.
     * _dontImportPlaylistsPref     Don't import playlists preference.
     * _libPrevPathDR               Saved path of previously imported library.
     * _versionDR                   Importer data format version.
     * _inLibraryBatch              True if beginLibraryBatch has been called
     * _timingService               sbITimingService, if enabled
     * _timingIdentifier            Identifier used with the timing service
     */

    _osType: null,
    _listener: null,
    _excludedPlaylists: "",
    _handleImportReqFunc: null,
    _xmlParser: null,
    _xmlFile: null,
    _ioService: null,
    _fileProtocolHandler: null,
    _typeSniffer: null,
    _libraryManager: null,
    _library: null,
    _playlist: null,
    _trackIDMap: null,
    _trackCount: 0,
    _nonExistentMediaCount: 0,
    _unsupportedMediaCount: 0,
    _iTunesLibID: "",
    _iTunesLibSig: "",
    _import: false,
    _importPlaylists: false,
    _dirtyPlaylistAction: null,
    _libraryFilePathPref: null,
    _autoImportPref: null,
    _dontImportPlaylistsPref: null,
    _libPrevPathDR: null,
    _versionDR: null,
    _inLibraryBatch: false,
    _timingService: null,
    _timingIdentifier: null,


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
        switch (this._osType)
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

    get libraryPreviouslyImported()
    {
        if (this.getPref("lib_prev_mod_time", "").length == 0)
            return false;
        return true;
    },

    get libraryPreviousImportPath()
    {
        return this._libPrevPathDR.stringValue;
    },


    /**
     * \brief Initialize the library importer.
     */

    initialize: function()
    {
        /* Get the OS type. */
        this._osType = this.getOSType();

        /* Get the list of excluded playlists. */
        this._excludedPlaylists =
          SBString("import_library.itunes.excluded_playlists", "");

        /* Get the IO service component. */
        this._ioService = Components.
                                classes["@mozilla.org/network/io-service;1"].
                                getService(Components.interfaces.nsIIOService);

        /* Get a file protocol handler. */
        this._fileProtocolHandler =
            Components
                .classes["@mozilla.org/network/protocol;1?name=file"]
                .createInstance(Components.interfaces.nsIFileProtocolHandler);

        /* Get the playlist playback services. */
        this._typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                              .createInstance(Ci.sbIMediacoreTypeSniffer); 

        /* Get the media library manager service          */
        /* component and create a media library instance. */
        this._libraryManager =
            Components.
                classes["@songbirdnest.com/Songbird/library/Manager;1"].
                getService(Components.interfaces.sbILibraryManager);
        this._library = this._libraryManager.getLibrary
                                        (this._libraryManager.mainLibrary.guid);
        this._library = this._library.QueryInterface
                                (Components.interfaces.sbILocalDatabaseLibrary);

        /* Get the importer preferences. */
        this._libraryFilePathPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this._libraryFilePathPref.init
                                (this.prefPrefix + ".library_file_path", null);
        this._autoImportPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this._autoImportPref.init(this.prefPrefix + ".auto_import", null);
        this._dontImportPlaylistsPref =
                        Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this._dontImportPlaylistsPref.init
                                    (this.prefPrefix + ".dont_import_playlists",
                                     null);

        /* Get the importer data remotes. */
        this._libPrevPathDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this._libPrevPathDR.init(this.prefPrefix + ".lib_prev_path", null);
        this._versionDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this._versionDR.init(this.prefPrefix + ".version", null);

        /* Set up timing if enabled */
        if ("@songbirdnest.com/Songbird/TimingService;1" in Cc) {
          this._timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                                  .getService(Ci.sbITimingService);
        }

        /* Create the iTunes properties. */
        var propertyMgr =
            Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                .getService(Ci.sbIPropertyManager);
        var propertyInfo =
                Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
                    .createInstance(Ci.sbITextPropertyInfo);
        propertyInfo.id = CompConfig.iTunesGUIDProperty;
        propertyInfo.displayName = "iTunes GUID";
        propertyInfo.userViewable = false;
        propertyInfo.userEditable = false;
        propertyMgr.addPropertyInfo(propertyInfo);

        /* Activate the database services. */
        ITDB.activate();

        /* Create a handleImportReq function with an object closure. */
        var                     _this = this;

        this._handleImportReqFunc = function(libFilePath,
                                             dbGUID,
                                             checkForChanges)
        {
            _this.handleImportReq(libFilePath, dbGUID, checkForChanges)
        }
    },


    /**
     * \brief Finalize the library importer.
     */

    finalize: function()
    {
        /* Make extra sure we aren't still in a library batch */
        this.endLibraryBatch();
    },


    /*
     * \brief Initiate external library importing.
     *
     * \param aLibFilePath File path to external library to import.
     * \param aGUID GUID of Songbird library into which to import.
     * \param aCheckForChanges If true, check for changes in external library
     * before importing.
     *
     * \return Import job object.
     */

    import: function(aLibFilePath, aGUID, aCheckForChanges)
    {
        var                         req = {};

        /* Log progress. */
        Log(  "1: import \""
            + aLibFilePath + "\" \""
            + aGUID + "\" "
            + aCheckForChanges + "\n");

        /* Do nothing if just checking for changes and */
        /* the library file has not been modified.     */
        if (aCheckForChanges &&
            (aLibFilePath == this._libPrevPathDR.stringValue))
        {
            file = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
            file.initWithPath(aLibFilePath);
            if (file.lastModifiedTime == this.getPref("lib_prev_mod_time", ""))
                return;
        }

        /* Create an importer job object. */
        this.mJob = new sbITunesImporterJob();

        /* Initialize status. */
        ITStatus.mJob = this.mJob;
        ITStatus.initialize();
        ITStatus.reset();
        ITStatus.bringToFront();

        /* Start timing, if enabled */
        if (this._timingService) {
            this._timingIdentifier = "ITunesImport-" + Date.now();
            this._timingService.startPerfTimer(this._timingIdentifier);
        }

        /* Start an import thread. */
        thread = new GeneratorThread(this.handleImportReq(aLibFilePath,
                                                          aGUID,
                                                          aCheckForChanges));
        thread.period = this.reqPeriod;
        thread.maxPctCPU = this.reqMaxPctCPU;
        this.mJob.setJobThread(thread);
        thread.start();

        /* Monitor job completion to make sure we don't leave the library
           in a batch state. */
        this.mJob.addJobProgressListener(this);

        return this.mJob;
    },


    /*
     * \brief Set the listener for import events.
     *
     * \param aListener Import event listener.
     */

    setListener: function(aListener)
    {
        /* Set the listener. */
        this._listener = aListener;
    },


    /*
     * \brief Called by mJob, used to determine when the job completes.
     *
     * \param aJob an sbIJobProgress object
     */

    onJobProgress: function(aJob) {
        // All we care about is completion
        if (aJob.status != Ci.sbIJobProgress.STATUS_RUNNING) {
            this.mJob.removeJobProgressListener(this);

            /* Make extra sure we aren't still in a library batch */
            this.endLibraryBatch();

            /* Stop timing */
            if (this._timingService) {
                this._timingService.stopPerfTimer(this._timingIdentifier);
            }
        }
    },


    /***************************************************************************
     *
     * Importer nsISupports interface.
     *
     **************************************************************************/

    QueryInterface: XPCOMUtils.generateQI(CompConfig.ifList),


    /***************************************************************************
     *
     * Internal importer functions.
     *
     **************************************************************************/

    /*
     * beginLibraryBatch
     *
     *   Attempt to force the library into an update batch
     * in order to avoid the performance hit of completing many
     * small batches.  This is dangerous, as failing to call
     * endLibraryBatch will prevent the library from updating.
     */
    beginLibraryBatch: function() {
        if (this._inLibraryBatch) {
            return;
        }
        if (this._library instanceof Ci.sbILocalDatabaseLibrary) {
            this._library.forceBeginUpdateBatch();
            this._inLibraryBatch = true;
        }
    },

    /*
     * endLibraryBatch
     *
     *   Attempt to undo a forced library batch if one is in progress
     */
    endLibraryBatch: function() {
        if (!this._inLibraryBatch) {
            return;
        }
        if (this._library instanceof Ci.sbILocalDatabaseLibrary) {
            this._library.forceEndUpdateBatch();
            this._inLibraryBatch = false;
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
            yield this.handleImportReq1(libFilePath,
                                        dbGUID,
                                        checkForChanges);
        }
        catch (e)
        {
            /* Dump exception. */
            Components.utils.reportError(e);
            LogException(e);

            /* Update status. */
            ITStatus.reset();
            ITStatus.mStageText = "Library import error";
            ITStatus.mDone = true;
            ITStatus.update();

            /* Make extra sure we aren't still in a library batch */
            this.endLibraryBatch();

            /* Send an import error event. */
            this._listener.onImportError();
        }
        finally
        {
            /* Update status. */
            ITStatus.update();
        }
    },

    handleImportReq1: function(libFilePath, dbGUID, checkForChanges)
    {
        var                         tag = {};
        var                         tagPreText = {};

        /* Wait until the importer is ready. */
        while (!this.isReady())
            yield;

        /* Initialize importer data format version. */
        if (!this._versionDR.intValue)
            this._versionDR.intValue = this.dataFormatVersion;

        /* If checking for changes, don't import. */
        if (checkForChanges)
            this._import = false;
        else
            this._import = true;

        /* Check if playlists should be imported. */
        if (this._import)
        {
            this._importPlaylists = !this._dontImportPlaylistsPref.boolValue;
        }
        else
        {
            this._importPlaylists = false;
        }

        /* Update status. */
        if (checkForChanges)
            ITStatus.mStageText = "Checking for changes in library";
        else
            ITStatus.mStageText = "Importing library";

        /* Create an xml parser. */
        this._xmlParser = new ITXMLParser(libFilePath);
        this._xmlFile = this._xmlParser.getFile();

        /* Initialize the iTunes library signature. */
        this._iTunesLibSig = new ITSig();

        /* Initialize the track ID map. */
        this._trackIDMap = {};

        /* Initialize import statistics. */
        this._trackCount = 0;
        this._nonExistentMediaCount = 0;
        this._unsupportedMediaCount = 0;

        /* Initialize dirty playlist action. */
        this._dirtyPlaylistAction = null;

        /* We are about to do a lot of work, so
           force the library into a batch state */
        this.beginLibraryBatch();

        /* Find the iTunes library ID key. */
        yield this.findKey("Library Persistent ID");

        /* Get the iTunes library ID. */
        this._xmlParser.getNextTag(tag, tagPreText);
        this._xmlParser.getNextTag(tag, tagPreText);
        this._iTunesLibID = tagPreText.value;

        /* Add the iTunes library ID to the iTunes library signature. */
        this._iTunesLibSig.update(  "Library Persistent ID"
                                  + this._iTunesLibID);

        /* Process the library track list. */
        yield this._processTrackList();

        /* Process the library playlist list. */
        yield this.processPlaylistList();

        /* End the library batch, if one is in progress */
        this.endLibraryBatch();

        var                         signature;
        var                         storedSignature;
        var                         foundChanges;
        var                         completeMsg;

        /* Get the iTunes library signature. */
        signature = this._iTunesLibSig.getSignature();

        /* Get the stored iTunes library signature. */
        storedSignature =
                        this._iTunesLibSig.retrieveSignature(this._iTunesLibID);

        /* If imported signature changed, store new signature. */
        if (this._import && (signature != storedSignature))
            this._iTunesLibSig.storeSignature(this._iTunesLibID, signature);

        /* Update previous imported library path and modification time. */
        if (this._import || (signature == storedSignature))
        {
            this._libPrevPathDR.stringValue = libFilePath;
            this.setPref("lib_prev_mod_time",
                         this._xmlFile.lastModifiedTime.toString());
        }

        /* Update import data format version. */
        if (this._import)
            this._versionDR.intValue = this.dataFormatVersion;

        /* Dispose of the XML parser. */
        /*zzz won't happen on exceptions. */
        this._xmlParser.close();
        this._xmlParser = null;

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
            this._listener.onLibraryChanged(libFilePath, dbGUID);

        /* If non-existent media is encountered, send an event. */
        if (this._import && (this._nonExistentMediaCount > 0))
        {
            this._listener.onNonExistentMedia(this._nonExistentMediaCount,
                                              this._trackCount);
        }

        /* If unsupported media is encountered, send an event. */
        if (this._import && (this._unsupportedMediaCount > 0))
            this._listener.onUnsupportedMedia();
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

        if (!this._typeSniffer)
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
        var                         tag = {};
        var                         tagPreText = {};

        /* Find the target key. */
        while (true)
        {
            /* Get the next tag. */
            this._xmlParser.getNextTag(tag, tagPreText);

            /* If the next key is the target or no more     */
            /* tags are left, transition to the next state. */
            if ((tagPreText.value == tgtKeyName) || (!tag.value))
                break;

            /* Yield if thread should. */
            yield this.yieldIfShouldWithStatusUpdate();
        }
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
     * getPref
     *
     *   --> aPrefName          Name of preference to get.
     *   --> aDefault           Default preference value.
     *
     *   <--                    Preference value.
     *
     *   This function reads and returns the value of the preference specified
     * by aPrefName.  If the preference is not set, this function returns the
     * default value specified by aDefault.
     */

    getPref: function(aPrefName, aDefault)
    {
        var Application = Cc["@mozilla.org/fuel/application;1"]
                              .getService(Ci.fuelIApplication);
        return Application.prefs.getValue(this.prefPrefix + "." + aPrefName,
                                          aDefault);
    },


    /*
     * setPref
     *
     *   --> aPrefName          Name of preference to set.
     *   --> aValue             Value to set.
     *
     *   This function sets the preference specified by aPrefName to the value
     * specified by aValue.
     */

    setPref: function(aPrefName, aValue)
    {
        var Application = Cc["@mozilla.org/fuel/application;1"]
                              .getService(Ci.fuelIApplication);
        Application.prefs.setValue(this.prefPrefix + "." + aPrefName, aValue);
    },


    /*
     * yieldWithStatusUpdate
     *
     *   This function updates the import status and yields.
     */

    yieldWithStatusUpdate: function()
    {
        /* Update status. */
        if (this._xmlParser)
        {
            ITStatus.mProgress =   (100 * this._xmlParser.tell())
                                 / this._xmlFile.fileSize;
        }
        ITStatus.update();

        /* Yield. */
        yield;
    },


    /*
     * yieldIfShouldWithStatusUpdate
     *
     *   This function checks if the thread should yield.  If it should, this
     * function updates the import status and yields.
     */

    yieldIfShouldWithStatusUpdate: function()
    {
        /* Check if it's time to yield. */
        if (GeneratorThread.shouldYield())
            yield this.yieldWithStatusUpdate();
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
        var                         tag = {};
        var                         tagPreText = {};
        var                         processPlaylist;

        /* Find the playlists list. */
        yield this.findKey("Playlists");

        /* Skip the "array" tag. */
        this._xmlParser.getNextTag(tag, tagPreText);

        /* Process each playlist in list. */
        while (true)
        {
            /* Get the next tag. */
            this._xmlParser.getNextTag(tag, tagPreText);

            /* If it's a "dict" tag, process the playlist.  If   */
            /* it's a "/array" tag, there are no more playlists. */
            if (tag.value == "dict")
                yield this.processPlaylist();
            else if (tag.value == "/array")
                break;

            /* Yield if thread should. */
            yield this.yieldIfShouldWithStatusUpdate();
        }
    },


    /*
     * processPlaylist
     *
     *   This function processes the next playlist.
     */

    processPlaylist: function()
    {
        var                         playlistInfoTable = {};
        var                         playlistName;
        var                         iTunesPlaylistID;
        var                         importPlaylist;
        var                         action;
        var                         hasPlaylistItems;
        var                         dirtyPlaylist;

        var                         sigGen;
        var                         signature;
        var                         done;

        /* Get the playlist info. */
        hasPlaylistItems = this.getPlaylistInfo(playlistInfoTable);
        playlistName = playlistInfoTable["Name"];
        iTunesPlaylistID = playlistInfoTable["Playlist Persistent ID"];

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
        else if (   this._excludedPlaylists.indexOf(":" + playlistName + ":")
                 != -1)
        {
            importPlaylist = false;
        }

        /* If importing, add playlist info  */
        /* to the iTunes library signature. */
        if (importPlaylist)
        {
            this._iTunesLibSig.update("Name" + playlistName);
            this._iTunesLibSig.update(  "Playlist Persistent ID"
                                      + iTunesPlaylistID);
        }

        /* Get the Songbird playlist. */
        if ((importPlaylist) && (this._importPlaylists))
        {
            /* Get the Songbird playlist ID. */
            playlistID = ITDB.getSBIDFromITID(this._iTunesLibID,
                                              iTunesPlaylistID);

            /* If the importer data format version is less than 2, try   */
            /* getting the Songbird playlist ID from the iTunes playlist */
            /* name.                                                     */
            if (!playlistID && (this._versionDR.intValue < 2))
            {
                playlistID = ITDB.getSBPlaylistIDFromITName(playlistName);
                if (playlistID)
                {
                    ITDB.mapID(this._iTunesLibID,
                               iTunesPlaylistID,
                               playlistID);
                }
            }

            /* Get the Songbird playlist. */
            if (playlistID)
            {
                try
                {
                    this._playlist = this._library.getItemByGuid(playlistID);
                }
                catch (e)
                {
                    this._playlist = null;
                }
            }
        }

        /* Wait for the database services to synchronize. */
        while (!ITDB.sync())
        {
            yield;
        }

        /* Check for dirty playlist. */
        if (importPlaylist && this._playlist)
        {
            dirtyPlaylist = {};
            yield this.isDirtyPlaylist(dirtyPlaylist);
            dirtyPlaylist = dirtyPlaylist.value;
        }

        /* If not importing playlists, keep current ones.   */
        /* Otherwise, determine what import action to take. */
        if (!this._importPlaylists)
        {
            action = "keep";
        }
        else if (importPlaylist)
        {
            /* Default to replacing playlist. */
            action = "replace";

            /* Check for a dirty Songbird playlist.  If it's dirty, */
            /* query the user for the proper action to take.        */
            if (this._playlist && dirtyPlaylist)
                action = this.getDirtyPlaylistAction(playlistName);
        }

        /* Create a fresh Songbird playlist if replacing. */
        if (importPlaylist && (action == "replace"))
        {
            /* Delete Songbird playlist if present. */
            if (this._import && this._playlist)
                this._library.remove(this._playlist);

            /* Create the playlist. */
            if (this._import)
            {
                this._playlist = this._library.createMediaList("simple");
                this._playlist.name = playlistName;
                playlistID = this._playlist.guid;
                ITDB.mapID(this._iTunesLibID, iTunesPlaylistID, playlistID);
            }
        }

        /* Wait for the database services to synchronize. */
        while (!ITDB.sync())
        {
            yield;
        }

        /* Process the playlist items. */
        if (importPlaylist)
            yield this.processPlaylistItems(action);
        else if (hasPlaylistItems)
            this._xmlParser.skipNextElement();

        /* Delete playlist if it's empty. */
        if (this._import && this._playlist)
        {
            if (this._playlist.isEmpty)
            {
                this._library.remove(this._playlist);
                this._playlist = null;
            }
        }

        /* Compute and store the Songbird playlist signature. */
        if (this._import && this._playlist && (action != "keep"))
        {
            sigGen = {};
            yield this.generateSBPlaylistSig(this._playlist, sigGen);
            sigGen = sigGen.value;
            signature = sigGen.getSignature();
            sigGen.storeSignature(playlistID, signature);
        }

        /* Clear current playlist object. */
        this._playlist = null;
    },


    /*
     * isDirtyPlaylist
     *
     *   <-- aIsDirty               True if the current Songbird playlist is
     *                              dirty.
     *
     *   This function checks if the current Songbird playlist has been changed
     * since the last time it was imported from iTunes.  If it has, this
     * function returns true in aIsDirty; otherwise, it returns false.
     */

    isDirtyPlaylist: function(aIsDirty)
    {
        var                         sigGen;
        var                         signature;
        var                         storedSignature;
        var                         dirtyPlaylist;

        /* Compute the playlist signature. */
        sigGen = {};
        yield this.generateSBPlaylistSig(this._playlist, sigGen);
        sigGen = sigGen.value;
        signature = sigGen.getSignature();

        /* Get the stored signature. */
        storedSignature = sigGen.retrieveSignature(this._playlist.guid);

        /* If the computed signature is not the same as */
        /* the stored signature, the playlist is dirty. */
        if (signature != storedSignature)
            dirtyPlaylist = true;
        else
            dirtyPlaylist = false;

        aIsDirty.value = dirtyPlaylist;
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
        if (this._dirtyPlaylistAction)
            return (this._dirtyPlaylistAction);

        /* Send a dirty playlist event and get the action to take. */
        applyAll = {};
        action = this._listener.onDirtyPlaylist(playlistName, applyAll);
        applyAll = applyAll.value;

        /* If the user selected to apply action */
        /* to all playlists, save the action.   */
        if (applyAll)
            this._dirtyPlaylistAction = action;

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
            this._xmlParser.getNextTag(tag, tagPreText);

            /* If it's a "key" tag, process the playlist info. */
            if (tag.value == "key")
            {
                /* Get the key name. */
                this._xmlParser.getNextTag(tag, tagPreText);
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
                    this._xmlParser.getNextTag(tag, tagPreText);
                    if (tag.value.charCodeAt(tag.value.length - 1) ==
                        this.mSlashCharCode)
                    {
                        keyValue = tag.value.substring(0, tag.value.length - 1);
                    }
                    else
                    {
                        this._xmlParser.getNextTag(tag, tagPreText);
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
        var                         tag = {};
        var                         tagPreText = {};
        var                         track;
        var                         trackID;
        var                         trackGUID;
        var                         updateSBPlaylist;
        var                         done;

        /* Check if the Songbird playlist should be updated. */
        if (this._import && (action != "keep"))
            updateSBPlaylist = true;
        else
            updateSBPlaylist = false;

        /* Process each playlist item. */
        while (true)
        {
            /* Process add playlist track batch if it's full. */
            if (this.addPlaylistTrackBatchFull())
                this.addPlaylistTrackBatchProcess();

            /* Get the next tag. */
            this._xmlParser.getNextTag(tag, tagPreText);

            /* If it's a "dict" tag, process the playlist item. */
            /* If it's a "/array" tag, there are no more items. */
            if (tag.value == "dict")
            {
                /* Skip the key and integer tags  */
                /* and get the track ID and GUID. */
                this._xmlParser.getNextTag(tag, tagPreText);
                this._xmlParser.getNextTag(tag, tagPreText);
                this._xmlParser.getNextTag(tag, tagPreText);
                this._xmlParser.getNextTag(tag, tagPreText);
                trackID = tagPreText.value;
                if (updateSBPlaylist)
                    trackGUID = this._trackIDMap[trackID];

                /* Add the iTunes track persistent ID */
                /* to the iTunes library signature.   */
                /*zzz get ID value. */
                this._iTunesLibSig.update("Persistent ID");

                /* Add the track to the playlist. */
                if (updateSBPlaylist && trackGUID)
                {
                    try
                    {
                        track = this._library.getItemByGuid(trackGUID);
                    }
                    catch (e)
                    {
                        track = null;
                    }
                    if (track)
                        this.addPlaylistTrackBatchAdd(this._playlist, track);
                }
            }
            else if (tag.value == "/array")
            {
                break;
            }

            /* Yield if thread should. */
            yield this.yieldIfShouldWithStatusUpdate();
        }

        /* Process the remaining add playlist track batch. */
        if (!this.addPlaylistTrackBatchEmpty())
            this.addPlaylistTrackBatchProcess();
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
     *   --> aPlaylist              Songbird playlist.
     *   <-- aSigGen                Signature generator.
     *
     *   This function generates a signature for the Songbird playlist specified
     * by playlist and returns a signature generator object for it in aSigGen.
     */

    generateSBPlaylistSig: function(aPlaylist, aSigGen)
    {
        var                         sigGen;
        var                         signature = null;
        var                         playlistLength;
        var                         i, j;

        /* Create a signature generator. */
        sigGen = new ITSig();

        /* Generate a signature over all entries. */
        playlistLength = aPlaylist.length;
        for (i = 0; i < playlistLength; i++)
        {
            /* Update the signature with the media GUID. */
            sigGen.update(aPlaylist.getItemByIndex(i).guid);

            /* Yield if thread should. */
            yield this.yieldIfShouldWithStatusUpdate();
        }

        aSigGen.value = sigGen;
    },


  //----------------------------------------------------------------------------
  //
  // Importer track services.
  //
  //----------------------------------------------------------------------------

  /**
   * Process the iTunes library track list.
   */

  _processTrackList: function sbITunesImporter__processTrackList() {
    // Find the track list.
    yield this.findKey("Tracks");

    // Skip the "dict" tag.
    this._xmlParser.getNextTag();
    if (this._xmlParser.tag != "dict")
      throw new Error("Error parsing iTunes library XML file.");

    // Process each track in list and import in batches.
    var trackBatch = [];
    while (true) {
      // Get the next tag.
      this._xmlParser.getNextTag();
      var tag = this._xmlParser.tag;

      // If tag is a "/key" tag, process the track and add it to the batch.
      // If tag is a "/dict" tag, there are no more tracks.
      // Otherwise, process the next tag.
      if (tag == "/key") {
        var trackInfo = this._processTrack();
        if (trackInfo)
          trackBatch.push(trackInfo);
      }
      else if (tag == "/dict")
        break;
      else
        continue;

      // Process batch if full.
      if (trackBatch.length >= this.addTrackBatchSize) {
        // Import tracks.
        if (this._import)
          yield this._importTracks(trackBatch);
        trackBatch = [];

        // Yield if thread should.
        yield this.yieldIfShouldWithStatusUpdate();
      }
    }

    // Import remaining tracks.
    if ((trackBatch.length > 0) && (this._import))
      yield this._importTracks(trackBatch);

    // Wait for the mapping database to synchronize.
    while (!ITDB.sync())
      yield;

    // Update status.
    ITStatus.mProgress =
               (100 * this._xmlParser.tell()) / this._xmlFile.fileSize;
  },


  /**
   * Process the next track in the iTunes library and return an object
   * containing track information.
   *
   * \return                    Track information or null if track media is not
   *                            supported.
   */

  _processTrack: function sbITunesImporter__processTrack() {
    // One more track.
    this._trackCount++;

    // Skip the "dict" tag.
    this._xmlParser.getNextTag();
    if (this._xmlParser.tag != "dict")
      throw new Error("Error parsing iTunes library XML file.");

    // Get the track iTunes information.
    var trackInfo = {};
    this._getTrackITunesInfo(trackInfo);

    // Get the track URI.
    this._getTrackURI(trackInfo);

    // Log progress.
    Log("1: processTrack \"" + trackInfo.uri.spec + "\"\n");

    // Get the track properties.
    this._getTrackProperties(trackInfo);

    // Get the track file and check if the track media exists.
    var trackFile = null;
    var trackExists = false;
    if (trackInfo.uri instanceof Ci.nsIFileURL) {
      try {
        trackFile = trackInfo.uri.file;
        trackExists = trackFile.exists();
      } catch (ex) {
        Log("processTrack: File protocol error " + ex + "\n");
        Log(trackInfo.uri.spec + "\n");
      }
      if (!trackExists)
        this._nonExistentMediaCount++;
    }

    // Add the track content length property.
    if (trackExists && trackFile) {
      trackInfo.propertyArray.appendProperty(SBProperties.contentLength,
                                             trackFile.fileSize.toString());
    }

    // Check if the track media is supported and add result to the iTunes
    // library signature.  This ensures the signature changes if support for the
    // track media is added (e.g., by installing an extension).
    var supported = this._typeSniffer.isValidMediaURL(trackInfo.uri);
    if (!supported)
      this._unsupportedMediaCount++;
    this._iTunesLibSig.update("supported" + supported);

    // Get the track iTunes persistent ID and add it to the iTunes library
    // signature.
    var iTunesTrackID = trackInfo.iTunes["Persistent ID"];
    this._iTunesLibSig.update("Persistent ID" + iTunesTrackID);

    // Return track info if track is supported.
    if (!supported)
      return null;
    return trackInfo;
  },


  /**
   * Get the iTunes track info from the next track in the iTunes library and
   * return it in aTrackInfo.
   *
   * \param aTrackInfo          Track information table.
   */

  _getTrackITunesInfo:
    function sbITunesImporter__getTrackITunesInfo(aTrackInfo) {
    // Initialize the track iTunes info.
    aTrackInfo.iTunes = {};

    // Get the track iTunes info.
    while (true) {
      // Get the next tag.
      this._xmlParser.getNextTag();
      tag = this._xmlParser.tag;
      tagPreText = this._xmlParser.tagPreText;

      // If tag is a "key" tag, process the track info.
      // If tag is a "/dict" tag, the track info is done.
      if (tag == "key") {
        // Get the key name.
        this._xmlParser.getNextTag();
        var keyName = this._xmlParser.tagPreText;

        // Get the key value.  If the next tag is empty, use its name as the key
        // value.
        var keyValue;
        this._xmlParser.getNextTag();
        var tag = this._xmlParser.tag;
        if (tag.substring(tag.length - 1) == "/") {
          keyValue = tag.substring(0, tag.length - 1);
        } else {
          this._xmlParser.getNextTag();
          keyValue = this._xmlParser.tagPreText;
        }

        // Save the track info.
        aTrackInfo.iTunes[keyName] = keyValue;
      } else if (tag == "/dict") {
        break;
      }
    }
  },


  /**
   * Get the track URI from the track information specified by aTrackInfo and
   * save it there.
   *
   * \param aTrackInfo          Track information.
   */

  _getTrackURI: function sbITunesImporter__getTrackURI(aTrackInfo) {
    // Get the track media location.
    var location = aTrackInfo.iTunes["Location"];

    // If the track location prefix is "file://localhost//", convert path as a
    // UNC network path.
    location = location.replace(/^file:\/\/localhost\/\//, "file://///");

    // If the track location prefix is "file://localhost/", convert path as a
    // local file path.
    location = location.replace(/^file:\/\/localhost\//, "file:///");

    // Strip trailing "/" that iTunes sometimes adds.
    location = location.replace(/\/$/, "");

    // If location contains a bare DOS drive letter, set file scheme.
    if (location.match(/^\w:/))
      location = "file:///" + location;

    // If no scheme is specified, set file scheme with a UNC network path.
    if (!location.match(/^\w*:/))
      location = "file:////" + url;

    // Convert to lower case on Windows since it's case-insensitive.
    if (this._osType == "Windows")
      location = location.toLowerCase();

    // Add file location to iTunes library signature.
    this._iTunesLibSig.update("location" + location);

    // Get the track URI.
    aTrackInfo.uri = this._ioService.newURI(location, null, null);
  },


  /**
   * Get the track Songbird properties from the iTunes track information in the
   * track information specified by aTrackInfo.
   *
   * \param aTrackInfo          Track information.
   */

  _getTrackProperties:
    function sbITunesImporter__getTrackProperties(aTrackInfo) {
    // Initialize the track property array.
    var propertyArray =
          Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
            .createInstance(Ci.sbIMutablePropertyArray);
    aTrackInfo.propertyArray = propertyArray;

    // Convert between iTunes metadata and Songbird properties.
    for (var i = 0; i < this.metaDataTable.length; i++) {
      // Get the next metadata table entry.
      var metaDataEntry = this.metaDataTable[i];

      // Get the track iTunes metadata.  Skip if no metadata.
      var iTunesMetaData = aTrackInfo.iTunes[metaDataEntry.iTunes];
      if (!iTunesMetaData)
        continue;

      // Get the Songbird property from the iTunes metadata.
      var propertyValue =
            this._convertMetaValue(iTunesMetaData, metaDataEntry.convertFunc);
      propertyArray.appendProperty(metaDataEntry.songbird, propertyValue);

      // Add the Songbird property to the iTunes library signature.
      this._iTunesLibSig.update(metaDataEntry.songbird + propertyValue);
    }
  },


  /**
   * Convert the iTunes metadata value specified by aITunesMetaValue to a
   * Songbird property value using the conversion function specified by
   * aConvertFunc and return the result.
   *
   * \param aITunesMetaValue    iTunes metadata value.
   * \param aConvertFunc        Metadata conversion function.
   *
   * \return                    Songbird property value.
   */

  _convertMetaValue:
    function sbITunesImporter__convertMetaValue(aITunesMetaValue,
                                                aConvertFunc) {
    // If a conversion function was provided, use it.  Otherwise, just copy the
    // iTunes metadata value to the Songbird property value.
    var sbPropertyValue;
    if (aConvertFunc)
      sbPropertyValue = this[aConvertFunc](aITunesMetaValue);
    else
      sbPropertyValue = aITunesMetaValue;

    return sbPropertyValue;
  },


  /**
   * Convert the iTunes rating value specified by aITunesMetaValue to a Songbird
   * rating property value and return the result.
   *
   * \param aITunesMetaValue    iTunes metadata value.
   *
   * \return                    Songbird property value.
   */

  _convertRating: function sbITunesImporter__convertRating(aITunesMetaValue) {
    var rating = Math.floor((parseInt(aITunesMetaValue) + 10) / 20);
    return rating.toString();
  },


  /**
   * Convert the iTunes duration value specified by aITunesMetaValue to a
   * Songbird duration property value and return the result.
   *
   * \param aITunesMetaValue    iTunes metadata value.
   *
   * \return                    Songbird property value.
   */

  _convertDuration:
    function sbITunesImporter__convertDuration(aITunesMetaValue) {
    var duration = parseInt(aITunesMetaValue) * 1000;
    return duration.toString();
  },


  /**
   * Convert the iTunes date/time value specified by aITunesMetaValue to a
   * Songbird date/time property value and return the result.
   *
   * \param aITunesMetaValue    iTunes metadata value.
   *
   * \return                    Songbird property value.
   */

  _convertDateTime:
    function sbITunesImporter__convertDateTime(aITunesMetaValue) {
    // Convert "1970-01-01T00:00:00Z" to "1970/01/01 00:00:00 UTC".
    var iTunesDateTime = aITunesMetaValue.replace(/-/g, "/");
    iTunesDateTime = iTunesDateTime.replace(/T/, " ");
    iTunesDateTime = iTunesDateTime.replace(/Z/, " UTC");

    // Parse the date/time string into epoch time.
    sbDateTime = Date.parse(iTunesDateTime);

    return sbDateTime.toString();
  },


  /**
   * Import the batch of tracks specified by aTrackBatch into Songbird.
   *
   * \param aTrackBatch         Batch of tracks to import.
   */

  _importTracks: function sbITunesImporter__importTracks(aTrackBatch) {
    // Get the media items for previously imported tracks.
    this._getTrackMediaItems(aTrackBatch);

    // Create media items for new tracks.
    yield this._createTrackMediaItems(aTrackBatch);

    // Add tracks to the track ID map.
    for (var i = 0; i < aTrackBatch.length; i++) {
      // Get the track ID's.
      var trackInfo = aTrackBatch[i];
      var iTunesTrackID = trackInfo.iTunes["Track ID"];
      if (!trackInfo.mediaItem)
        continue;
      var guid = trackInfo.mediaItem.guid;

      // Add the track ID's to the map.
      this._trackIDMap[iTunesTrackID] = guid;
    }
  },


  /**
   * Get the media items for the tracks in the batch specified by aTrackBatch
   * that have already been imported.
   *
   * \param aTrackBatch         Batch of tracks for which to get media items.
   */

  _getTrackMediaItems:
    function sbITunesImporter__getTrackMediaItems(aTrackBatch) {
    // Get the media item for each previously imported track.
    for (var i = 0; i < aTrackBatch.length; i++) {
      // Get the track info.
      var trackInfo = aTrackBatch[i];
      var iTunesTrackID = trackInfo.iTunes["Track ID"];

      // Get the mapped GUID.  Skip track if it hasn't been previously imported.
      guid = ITDB.getSBIDFromITID(this._iTunesLibID, iTunesTrackID);
      if (!guid)
        continue;

      // Get the track media item.
      trackInfo.mediaItem = this._library.getMediaItem(guid);
    }
  },


  /**
   * Create media items for the tracks in the batch specified by aTrackBatch.
   *
   * \param aTrackBatch         Batch of tracks for which to create media items.
   */

  _createTrackMediaItems:
    function sbITunesImporter__createTrackMediaItems(aTrackBatch) {
    // Set up arrays of track URIs and properties.  Do nothing more if list is
    // empty.
    var uriArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                     .createInstance(Ci.nsIMutableArray);
    var propertyArrayArray =
          Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
            .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < aTrackBatch.length; i++) {
      // Get track info.  Skip tracks that have previously been imported.
      var trackInfo = aTrackBatch[i];
      if (trackInfo.mediaItem)
        continue;

      // Get the track URI and properties.
      uriArray.appendElement(trackInfo.uri, false);
      propertyArrayArray.appendElement(trackInfo.propertyArray, false);
    }
    if (uriArray.length == 0)
      return;

    // Create the track media items.
    var createdMediaItems;
    var createResult;
    var createComplete = false;
    var listener = {
      onProgress: function(aIndex) {},
      onComplete: function(aMediaItems, aResult) {
        createdMediaItems = aMediaItems;
        createResult = aResult;
        createComplete = true;
      }
    };
    this._library.batchCreateMediaItemsAsync(listener,
                                             uriArray,
                                             propertyArrayArray,
                                             false);

    // Wait for media item creation to complete.
    while (!createComplete)
      yield this.yieldWithStatusUpdate();

    // Do nothing more if creation failed.
    if (!Components.isSuccessCode(createResult)) {
      Log("Error creating media items " + createResult + "\n");
      return;
    }

    // Create a table mapping iTunes track ID's to track info objects.
    var iTunesTrackIDTable = {};
    for (var i = 0; i < aTrackBatch.length; i++) {
      var trackInfo = aTrackBatch[i];
      var iTunesTrackID = trackInfo.iTunes["Persistent ID"];
      iTunesTrackIDTable[iTunesTrackID] = trackInfo;
    }

    // Add the created media items to each corresponding track info object.
    var createCount = createdMediaItems.length;
    for (var i = 0; i < createCount; i++) {
      // Get the media item.
      var mediaItem = createdMediaItems.queryElementAt(i, Ci.sbIMediaItem);
      var iTunesGUID = mediaItem.getProperty(CompConfig.iTunesGUIDProperty);

      // Add the media item to the corresponding track info object.
      var trackInfo = iTunesTrackIDTable[iTunesGUID];
      if (trackInfo)
        trackInfo.mediaItem = mediaItem;
    }

    // batchCreateMediaItemsAsync won't return media items for duplicate tracks.
    // In order to get the corresponding media item for each duplicate track,
    // createMediaItem must be called.  Thus, for each track that does not have
    // a corresponding media item, call createMediaItem.
    for (var i = 0; i < aTrackBatch.length; i++) {
      // Get the track info and skip tracks that have media items.
      var trackInfo = aTrackBatch[i];
      if (trackInfo.mediaItem)
        continue;

      // Create the track media item.
      var mediaItem = this._library.createMediaItem(trackInfo.uri,
                                                    trackInfo.propertyArray);

      // Add the media item to the track info.
      if (mediaItem)
        trackInfo.mediaItem = mediaItem;
      else
        Log("Error creating track.\n");
    }

    // Map the Songbird track GUID's to iTunes track persistent ID's.
    for (var i = 0; i < aTrackBatch.length; i++) {
      // Get the track ID's.
      var trackInfo = aTrackBatch[i];
      if (!trackInfo.mediaItem)
        continue;
      var sbGUID = trackInfo.mediaItem.guid;
      var iTunesTrackID = trackInfo.iTunes["Persistent ID"];

      // Add track ID's to map.
      ITDB.mapID(this._iTunesLibID, iTunesTrackID, sbGUID);
    }
  }
};


/*******************************************************************************
 *******************************************************************************
 *
 * iTunes importer component module services.
 *
 *******************************************************************************
 ******************************************************************************/

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbITunesImporter]);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// XML parser services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Construct an XML parser object.
 */

function ITXMLParser(filePath) {
  // Initialize a file object.
  this._file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  this._file.initWithPath(filePath);

  // Initialize a file input stream.
  var fileInputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                          .createInstance(Ci.nsIFileInputStream);
  fileInputStream.init(this._file, 1, 0, 0);

  // Get a seekable input stream.
  this._seekableStream = fileInputStream.QueryInterface(Ci.nsISeekableStream);

  // Initialize a scriptable input stream.
  this._inputStream = Cc["@mozilla.org/intl/converter-input-stream;1"]
                        .createInstance(Ci.nsIConverterInputStream);
  this._inputStream.init
    (fileInputStream,
     "UTF-8",
     this.readSize,
     Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
}


//
// ITXMLParser prototype object.
//

ITXMLParser.prototype = {
  // Set the constructor.
  constructor: ITXMLParser,

  //----------------------------------------------------------------------------
  //
  // XML parser configuration.
  //
  //----------------------------------------------------------------------------

  //
  // readSize                   Number of bytes to read when filling stream
  //                            buffer.
  // tag                        Last parsed tag.
  // tagPreText                 Text before last parsed tag.
  //

  readSize: 16384,
  tag: null,
  tagPreText: null,


  //----------------------------------------------------------------------------
  //
  // XML parser fields.
  //
  //----------------------------------------------------------------------------

  //
  // _file                      File from which to read.
  // _seekableStream            Seekable stream from which to read.
  // _inputStream               Input stream from which to read.
  // _buffer                    Temporary buffer to hold data.
  // _tagList                   List of available tags.
  // _nextTagIndex              Index of next tag to get from tag list.
  //

  _file: null,
  _seekableStream: null,
  _inputStream: null,
  _buffer: "",
  _tagList: [],
  _nextTagIndex: 0,


  //----------------------------------------------------------------------------
  //
  // Public XML parser methods.
  //
  //----------------------------------------------------------------------------

  /**
   * Close the XML parser.
   */

  close: function ITXMLParser_close() {
    // Close the input stream.
    this._inputStream.close();
  },


  /**
   * Get the next tag and return it in aTag.  Return the text between the
   * previous tag and the returned tag in aTagPreText.
   *
   * \param aTag                Returned tag string.  Optional.
   * \param aPreText            Returned text before tag.  Optional.
   */

  getNextTag: function ITXMLParser_getNextTag(aTag, aTagPreText) {
    // If the end of the tag list is reached, read more tags.
    if (this._nextTagIndex >= this._tagList.length) {
      this._tagList.length = 0;
      this._nextTagIndex = 0;
      this._readTags();
    }

    // Return the next tag.
    if (this._tagList.length > 0) {
      var tagSplit = this._tagList[this._nextTagIndex].split("<");
      this._nextTagIndex++;
      this.tag = tagSplit[1];
      this.tagPreText = this._decodeEntities(tagSplit[0]);
    } else {
      this.tag = null;
      this.tagPreText = null;
    }

    // Return tag and pre-text.
    if (aTag)
      aTag.value = this.tag;
    if (aTagPreText)
      aTagPreText.value = this.tagPreText;
  },


  /**
   * Skip the next element and all its descendents.
   */

  skipNextElement: function ITXMLParser_skipNextElement() {
    // Loop down through the next element until its end.
    var depth = 0;
    do {
      // Get the next tag.
      var tag = {};
      var tagPreText = {};
      this.getNextTag(tag, tagPreText);
      tag = tag.value;

      // If the tag starts with a "/", decrement the depth.
      if (tag.charCodeAt(0) == "/".charCodeAt(0)) {
        depth--;
      }

      // Otherwise, if the tag does not end with a "/" (empty element),
      // increment the depth.
      else if (tag.charCodeAt(tag.length - 1) != "/".charCodeAt(0)) {
        depth++;
      }
    } while (depth > 0);
  },


  /**
   * Return the current offset within the input stream.
   *
   * \return                    Current offset within the input stream.
   */

  tell: function ITXMLParser_tell() {
    // Get the current input stream offset and subtract the current buffer size.
    var offset = this._seekableStream.tell() - this._buffer.length;

    return offset;
  },


  /**
   * Return the nsIFile object being parsed.
   *
   * \return                    Data file.
   */

  getFile: function ITXMLParser_getFile() {
    return this._file.QueryInterface(Components.interfaces.nsIFile);
  },


  //----------------------------------------------------------------------------
  //
  // Private XML parser methods.
  //
  //----------------------------------------------------------------------------

  /**
   * Read data from the input stream and divide it into the tag list _tagList.
   */

  _readTags: function ITXMLParser__readTags() {
    // Read more data into buffer.
    var readData = {};
    var readCount = this._inputStream.readString(this.readSize, readData);
    readData = readData.value
    if (readCount > 0)
      this._buffer += readData;

    // Read from the stream until a tag is found.
    while ((this._tagList.length == 0) && (this._buffer)) {
      // Split stream buffer at the tag ends.
      this._tagList = this._buffer.split(">");

      // Pop off remaining data after last tag end.  If no tag was found, read
      // more data into the buffer.
      if (this._tagList.length > 1) {
        this._buffer = this._tagList.pop();
      } else {
        // No tags were found.
        this._tagList = [];

        // Read more data.  If no more data is available, empty buffer.
        readData = {};
        readCount = this._inputStream.readString(this.readSize, readData);
        readData = readData.value;
        if (readCount > 0)
          this._buffer += readData;
        else
          this._buffer = "";
      }
    }
  },


  /**
   * Search for XML entitites within the string specified by aStr, decode and
   * replace them, and return the resulting string.
   *
   * \param aStr                String to decode.
   *
   * \return                    String with decoded entitites.
   */

  _decodeEntities: function ITXMLParser__decodeEntities(aStr) {
    return aStr.replace(/&#([0-9]*);/g, this._replaceEntity);
  },


  /**
   * Function to be used as a replacement function for the string object replace
   * method.  Replace XML entity strings.  The entity string is specified by
   * aEntityStr, and the entity value is specified by aEntityVal.  Return the
   * string represented by the entity.
   *
   * \param aEntityStr          Entity string.
   * \param aEntityVal          Entity value.
   *
   * \return                    Decoded entity string.
   */

  _replaceEntity: function ITXMLParser__replaceEntity(aEntityStr, aEntityVal) {
    return String.fromCharCode(aEntityVal);
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
     * mJob                         Importer job object.
     * mStatusDataRemote            Data remote for displaying status.
     * mStageText                   Stage status text.
     * mProgressText                Progress within stage text.
     * mProgress                    Progress within stage percentage.
     * mDone                        Done status.
     */

    mJob: null,
    mStatusDataRemote: null,
    mStageText: "",
    mProgressText: "",
    mProgress: 0,
    mDone: false,


    /***************************************************************************
     *
     * Public status UI methods.
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

        /* Initialize the job object. */
        if (this.mJob)
        {
            var titleText = SBFormattedString("import_library.job.title_text",
                                              [ "iTunes" ]);
            this.mJob.setTitleText(titleText);
            this.mJob.setStatusText(SBString("import_library.job.status_text"));
        }
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

        /* Update job object. */
        if (this.mJob)
        {
            this.mJob.setProgress(this.mProgress);
            this.mJob.setTotal(100);

            if (this.mDone)
                this.mJob.setStatus(Ci.sbIJobProgress.STATUS_SUCCEEDED);
        }

        /* Log status. */
        Log("Status: " + statusText + "\n");
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
 * LogException
 *
 *   --> aException             Exception to log.
 *
 *   This function logs the exception specified by aException.
 */

function LogException(aException)
{
    LogSvc.Log("Exception: " + aException +
               " at " + aException.fileName +
               ", line " + aException.lineNumber + "\n");
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


//------------------------------------------------------------------------------
//
// iTunes importer job object.
//
//------------------------------------------------------------------------------

// Songbird imports.
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");

/**
 * Construct an iTunes importer job object.
 */

function sbITunesImporterJob() {
  // Call super-class constructor.
  SBJobUtils.JobBase.call(this);
}

// Define the object.
sbITunesImporterJob.prototype = {
  // Inherit prototype.
  __proto__: SBJobUtils.JobBase.prototype,

  //
  // iTunes importer job configuration.
  //
  //   _updateProgressDelay     Time in milliseconds to delay job progress
  //                            updates.
  //

  _updateProgressDelay: 50,


  //
  // iTunes importer job fields.
  //
  //   _jobThread               Importer job thread object.
  //   _updateProgressTimer     Timer used to delay job progress updates to
  //                            limit update rate.
  //

  _jobThread: null,
  _updateProgressTimer: null,


  //----------------------------------------------------------------------------
  //
  // Public importer job services.
  //
  //----------------------------------------------------------------------------

  /**
   * Set the job status to the sbIJobProgress status value specified by aStatus.
   *
   * \param aStatus             Job status.
   */

  setStatus: function sbITunesImporterJob_setStatus(aStatus) {
    this._status = aStatus;
    this._updateProgress();
  },


  /**
   * Set the job status text to the value specified by aStatusText.
   *
   * \param aStatusText         Job status text.
   */

  setStatusText: function sbITunesImporterJob_setStatusText(aStatusText) {
    this._statusText = aStatusText;
    this._updateProgress();
  },


  /**
   * Set the job title text to the value specified by aTitleText.
   *
   * \param aTitleText          Job title text.
   */

  setTitleText: function sbITunesImporterJob_setTitleText(aTitleText) {
    this._titleText = aTitleText;
    this._updateProgress();
  },


  /**
   * Set the job work units completed progress value to the value specified by
   * aProgress.
   *
   * \param aProgress           Job work units completed progress.
   */

  setProgress: function sbITunesImporterJob_setProgress(aProgress) {
    this._progress = aProgress;
    this._updateProgress();
  },


  /**
   * Set the job total work units to complete value to the value specified by
   * aTotal.
   *
   * \param aTotal              Total job work units to complete.
   */

  setTotal: function sbITunesImporterJob_setTotal(aTotal) {
    this._total = aTotal;
    this._updateProgress();
  },


  /**
   * Set the importer job thread object.
   *
   * \param aJobThread          Importer job thread object.
   */

  setJobThread: function sbITunesImporterJob_setJobThread(aJobThread) {
    this._jobThread = aJobThread;
    this._canCancel = true;
  },


  //----------------------------------------------------------------------------
  //
  // Importer job sbIJobCancelable services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Attempt to cancel the job
   * Throws NS_ERROR_FAILURE if canceling fails
   */

  cancel: function sbITunesImporterJob_cancel() {
    // Terminate the job thread.
    if (this._jobThread)
      this._jobThread.terminate();
    this._jobThread = null;

    // Update import status.
    ITStatus.mStageText = SBString("import_library.job.status.cancelled");
    ITStatus.mDone = true;
    ITStatus.update();
  },


  //----------------------------------------------------------------------------
  //
  // Internal importer job services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the job progress.  This function delays a bit before updating to
   * limit the update rate.
   */

  _updateProgress: function sbITunesImporterJob__updateProgress() {
    // Do nothing if update progress timer is already running.
    if (this._updateProgressTimer)
      return;

    // Delay before updating progress to limit progress update rate.
    var _this = this;
    var func = function() { _this._updateProgressDelayed(); };
    this._updateProgressTimer =
           Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._updateProgressTimer.initWithCallback(func,
                                               this._updateProgressDelay,
                                               Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _updateProgressDelayed:
    function sbITunesImporterJob__updateProgressDelayed() {
    this._updateProgressTimer = null;
    this.notifyJobProgressListeners();
  }
};


