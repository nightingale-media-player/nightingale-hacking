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
          convertFunc: "convertDuration" },
        { songbird: SBProperties.genre,            iTunes: "Genre" },
        { songbird: SBProperties.lastPlayTime,     iTunes: "Play Date UTC",
          convertFunc: "convertDateTime" },
        { songbird: SBProperties.lastSkipTime,     iTunes: "Skip Date",
          convertFunc: "convertDateTime" },
        { songbird: SBProperties.playCount,        iTunes: "Play Count" },
        { songbird: SBProperties.rating,           iTunes: "Rating",
          convertFunc: "convertRating" },
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
     * mOSType                      OS type.
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
     * mTypeSniffer                 Mediacore Type Sniffer component.
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
     * mLibPrevPathDR               Saved path of previously imported library.
     * mVersionDR                   Importer data format version.
     * mInLibraryBatch              True if beginLibraryBatch has been called
     * mTimingService               sbITimingService, if enabled
     * mTimingIdentifier            Identifier used with the timing service
     */

    mOSType: null,
    mListener: null,
    mExcludedPlaylists: "",
    mHandleImportReqFunc: null,
    mHandleAppStartupReqFunc: null,
    mXMLParser: null,
    mXMLFile: null,
    mIOService: null,
    mFileProtocolHandler: null,
    mTypeSniffer: null,
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
    mLibPrevPathDR: null,
    mVersionDR: null,
    mInLibraryBatch: false,
    mTimingService: null,
    mTimingIdentifier: null,


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

    get libraryPreviouslyImported()
    {
        if (this.getPref("lib_prev_mod_time", "").length == 0)
            return false;
        return true;
    },

    get libraryPreviousImportPath()
    {
        return this.mLibPrevPathDR.stringValue;
    },


    /**
     * \brief Initialize the library importer.
     */

    initialize: function()
    {
        /* Get the OS type. */
        this.mOSType = this.getOSType();

        /* Get the list of excluded playlists. */
        this.mExcludedPlaylists =
          SBString("import_library.itunes.excluded_playlists", "");

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
        this.mTypeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                              .createInstance(Ci.sbIMediacoreTypeSniffer); 

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
        this.mLibPrevPathDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mLibPrevPathDR.init(this.prefPrefix + ".lib_prev_path", null);
        this.mVersionDR = Components.
                            classes["@songbirdnest.com/Songbird/DataRemote;1"].
                            createInstance(Components.interfaces.sbIDataRemote);
        this.mVersionDR.init(this.prefPrefix + ".version", null);

        /* Set up timing if enabled */
        if ("@songbirdnest.com/Songbird/TimingService;1" in Cc) {
          this.mTimingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
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

        this.mHandleImportReqFunc = function(libFilePath,
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
            (aLibFilePath == this.mLibPrevPathDR.stringValue))
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
        if (this.mTimingService) {
            this.mTimingIdentifier = "ITunesImport-" + Date.now();
            this.mTimingService.startPerfTimer(this.mTimingIdentifier);
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
        this.mListener = aListener;
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
            if (this.mTimingService) {
                this.mTimingService.stopPerfTimer(this.mTimingIdentifier);
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
        if (this.mInLibraryBatch) {
            return;
        }
        if (this.mLibrary instanceof Ci.sbILocalDatabaseLibrary) {
            this.mLibrary.forceBeginUpdateBatch();
            this.mInLibraryBatch = true;
        }
    },

    /*
     * endLibraryBatch
     *
     *   Attempt to undo a forced library batch if one is in progress
     */
    endLibraryBatch: function() {
        if (!this.mInLibraryBatch) {
            return;
        }
        if (this.mLibrary instanceof Ci.sbILocalDatabaseLibrary) {
            this.mLibrary.forceEndUpdateBatch();
            this.mInLibraryBatch = false;
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
            this.mListener.onImportError();
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
            this.mImportPlaylists = !this.mDontImportPlaylistsPref.boolValue;
        }
        else
        {
            this.mImportPlaylists = false;
        }

        /* Update status. */
        if (checkForChanges)
            ITStatus.mStageText = "Checking for changes in library";
        else
            ITStatus.mStageText = "Importing library";

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

        /* We are about to do a lot of work, so
           force the library into a batch state */
        this.beginLibraryBatch();

        /* Find the iTunes library ID key. */
        yield this.findKey("Library Persistent ID");

        /* Get the iTunes library ID. */
        this.mXMLParser.getNextTag(tag, tagPreText);
        this.mXMLParser.getNextTag(tag, tagPreText);
        this.mITunesLibID = tagPreText.value;

        /* Add the iTunes library ID to the iTunes library signature. */
        this.mITunesLibSig.update(  "Library Persistent ID"
                                  + this.mITunesLibID);

        /* Process the library track list. */
        yield this.processTrackList();

        /* Process the library playlist list. */
        yield this.processPlaylistList();

        /* End the library batch, if one is in progress */
        this.endLibraryBatch();

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
            this.setPref("lib_prev_mod_time",
                         this.mXMLFile.lastModifiedTime.toString());
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

        if (!this.mTypeSniffer)
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
            this.mXMLParser.getNextTag(tag, tagPreText);

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
        if (this.mXMLParser)
        {
            ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                                 / this.mXMLFile.fileSize;
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
        var                         tag = {};
        var                         tagPreText = {};

        /* Find the track list. */
        yield this.findKey("Tracks");

        /* Skip the "dict" tag. */
        this.mXMLParser.getNextTag(tag, tagPreText);

        /* Process each track in list. */
        while (true)
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
                    break;
            }
            else
            {
                /* Process add track batch. */
                yield this.addTrackBatchProcess();

                /* Yield if thread should. */
                yield this.yieldIfShouldWithStatusUpdate();
            }
        }

        /* Process the remaining add track batch. */
        if (!this.addTrackBatchEmpty())
            yield this.addTrackBatchProcess();

        /* Wait for the database services to synchronize. */
        while (!ITDB.sync())
        {
            yield;
        }

        /* Update status. */
        ITStatus.mProgress =   (100 * this.mXMLParser.tell())
                             / this.mXMLFile.fileSize;
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
        supported = this.mTypeSniffer.isValidMediaURL(url);
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
        var                         addTrack;
        var                         guidList = [];
        var                         guid;
        var                         uriList = [];
        var                         uri;
        var                         iTunesTrackIDList = [];
        var                         propertyName;
        var                         i, j;

        /* Get the list of Songbird track GUIDs */
        /* from the list of iTunes track IDs.   */
        this.addTrackBatchGetGUIDs();

        /* Get the list of existing track media items. */
        this.addTrackBatchGetMediaItems();

        /* Create track media items for non-existent tracks. */
        yield this.addTrackBatchCreateMediaItems();

        /*XXXeps should sync any updated properties in existing tracks. */

        /* Add the tracks to the track ID map. */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            addTrack = this.mAddTrackList[i];
            this.mTrackIDMap[addTrack.trackInfoTable["Track ID"]] =
                                                                addTrack.guid;
        }

        /* Clear add track list. */
        this.mAddTrackList = [];
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
        var                         createMediaItemsListener;
        var                         addTrackMediaItemList;
        var                         uriList;
        var                         propertyArrayArray;

        var                         addTrack;
        var                         mediaItemList = [];
        var                         mediaItem;
        var                         iTunesTrackIDList = [];
        var                         guidList = [];
        var                         trackURI;
        var                         i, j;

        /* Set up a list of track URIs and           */
        /* properties.  Do nothing if list is empty. */
        uriList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                      .createInstance(Ci.nsIMutableArray);
        propertyArrayArray =
                Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                    .createInstance(Ci.nsIMutableArray);
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
        if (!uriList.length)
            return;

        /* Create the track media items. */
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
                                                 propertyArrayArray, false);

        /* Wait for media item creation to complete. */
        while (!createMediaItemsListener.complete)
            yield this.yieldWithStatusUpdate();

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
            return;
        }

        /* Get the list of media items added by the batch create. */
        mediaItemList = this.getJSArray(addTrackMediaItemList);

        /* Create a hash table for the add track list. */
        var addTrackMap = {};
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            addTrack = this.mAddTrackList[i];
            addTrackMap[addTrack.iTunesTrackID] = addTrack;
        }

        /* Add the added media items to the add track list. */
        for (i = 0; i < mediaItemList.length; i++)
        {
            /* Get the next media item and its associated iTunes GUID. */
            mediaItem = mediaItemList[i];
            var iTunesGUID = mediaItem.getProperty
                                                (CompConfig.iTunesGUIDProperty);

            /* Find the matching track in the add track */
            /* list and add the media item to it.       */
            addTrack = addTrackMap[iTunesGUID];
            if (addTrack)
            {
                addTrack.mediaItem = mediaItem;
                addTrack.guid = mediaItem.guid;
                iTunesTrackIDList.push(addTrack.iTunesTrackID);
                guidList.push(addTrack.guid);
            }
        }

        /* The batch create won't return a media item for any tracks that */
        /* are duplicates.  For any track in the add track list that      */
        /* doesn't have a media item, create a single media item and add  */
        /* the returned media item to the add track list.  This shouldn't */
        /* add a new media item; it should just return the matching,      */
        /* duplicate media item.  Ideally, there would be a more direct   */
        /* way to get a media item for a duplicate URL.                   */
        for (i = 0; i < this.mAddTrackList.length; i++)
        {
            /* Add a media item for each track */
            /* to add without a media item.    */
            addTrack = this.mAddTrackList[i];
            if (!addTrack.mediaItem)
            {
                /* Get the add track URI and properties. */
                var uri = addTrack.trackURI;
                var properties = addTrack.propertyArray;

                /* Try creating a media item.  This should */
                /* return a duplicate for the add track.   */
                mediaItem = this.mLibrary.createMediaItem(uri, properties);

                /* Add the media item to the add track. */
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

        /* Map the Songbird track GUIDs to iTunes track IDs. */
        for (i = 0; i < iTunesTrackIDList.length; i++)
        {
            ITDB.mapID(this.mITunesLibID,
                       iTunesTrackIDList[i],
                       guidList[i]);
        }
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

        /* If the track URI has an "http:" scheme, set the                   */
        /* downloadStatusTarget property to prevent the auto-downloader from */
        /* trying to download it.                                            */
        if (trackURI.scheme.match(/^http/))
        {
            propertyArray.appendProperty(SBProperties.downloadStatusTarget,
                                         "stream");
        }

        /* Add the track properties to the property array array. */
        aPropertyArrayArray.appendElement(propertyArray, false);

        /* Add the track URI and track properties to the add track object. */
        aAddTrack.trackURI = trackURI;
        aAddTrack.propertyArray = propertyArray;
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
        this.mXMLParser.getNextTag(tag, tagPreText);

        /* Process each playlist in list. */
        while (true)
        {
            /* Get the next tag. */
            this.mXMLParser.getNextTag(tag, tagPreText);

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
        else if (   this.mExcludedPlaylists.indexOf(":" + playlistName + ":")
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

        /* Get the Songbird playlist. */
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
                    this.mPlaylist = this.mLibrary.getItemByGuid(playlistID);
                }
                catch (e)
                {
                    this.mPlaylist = null;
                }
            }
        }

        /* Wait for the database services to synchronize. */
        while (!ITDB.sync())
        {
            yield;
        }

        /* Check for dirty playlist. */
        if (importPlaylist && this.mPlaylist)
        {
            dirtyPlaylist = {};
            yield this.isDirtyPlaylist(dirtyPlaylist);
            dirtyPlaylist = dirtyPlaylist.value;
        }

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

        /* Wait for the database services to synchronize. */
        while (!ITDB.sync())
        {
            yield;
        }

        /* Process the playlist items. */
        if (importPlaylist)
            yield this.processPlaylistItems(action);
        else if (hasPlaylistItems)
            this.mXMLParser.skipNextElement();

        /* Delete playlist if it's empty. */
        if (this.mImport && this.mPlaylist)
        {
            if (this.mPlaylist.isEmpty)
            {
                this.mLibrary.remove(this.mPlaylist);
                this.mPlaylist = null;
            }
        }

        /* Compute and store the Songbird playlist signature. */
        if (this.mImport && this.mPlaylist && (action != "keep"))
        {
            sigGen = {};
            yield this.generateSBPlaylistSig(this.mPlaylist, sigGen);
            sigGen = sigGen.value;
            signature = sigGen.getSignature();
            sigGen.storeSignature(playlistID, signature);
        }

        /* Clear current playlist object. */
        this.mPlaylist = null;
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
        yield this.generateSBPlaylistSig(this.mPlaylist, sigGen);
        sigGen = sigGen.value;
        signature = sigGen.getSignature();

        /* Get the stored signature. */
        storedSignature = sigGen.retrieveSignature(this.mPlaylist.guid);

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
        var                         tag = {};
        var                         tagPreText = {};
        var                         track;
        var                         trackID;
        var                         trackGUID;
        var                         updateSBPlaylist;
        var                         done;

        /* Check if the Songbird playlist should be updated. */
        if (this.mImport && (action != "keep"))
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
  return XPCOMUtils.generateModule([Component]);
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
///

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
  //

  readSize: 16384,


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
   * \param aTag                Tag string.
   * \param aPreText            Text before tag.
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
      aTag.value = tagSplit[1];
      aTagPreText.value = this._decodeEntities(tagSplit[0]);
    } else {
      aTag.value = null;
      aTagPreText.value = null;
    }
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


