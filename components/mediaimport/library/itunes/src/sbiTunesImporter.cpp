/*
 //
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbiTunesImporter.h"

// C/C++ includes
#include <algorithm>

// Mozilla includes
#include <nsAppDirectoryServiceDefs.h>
#include <nsArrayUtils.h>
#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsIArray.h>
#include <nsIBufferedStreams.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIInputStream.h>
#include <nsIIOService.h>
#include <nsILineInputStream.h>
#include <nsILocalFile.h>
#include <nsIProperties.h>
#include <nsISimpleEnumerator.h>
#include <nsIURI.h>
#include <nsIXULRuntime.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsThreadUtils.h>

// NSPR includes
#include <prtime.h>

// Nightingale includes
#include <sbIAlbumArtFetcherSet.h>
#include <sbIAlbumArtListener.h>
#include <sbFileUtils.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaList.h>
#include <sbIPropertyArray.h>
#include <sbLibraryUtils.h>
#include <sbIMediacoreTypeSniffer.h>
#include <sbPrefBranch.h>
#include <sbPropertiesCID.h>
#include <sbStringUtils.h>
#include <sbStandardProperties.h>
#ifdef SB_ENABLE_TEST_HARNESS
#include <sbITimingService.h>
#endif

#include "sbiTunesImporterAlbumArtListener.h"
#include "sbiTunesImporterJob.h"
#include "sbiTunesImporterStatus.h"

char const SB_ITUNES_LIBRARY_IMPORT_PREF_PREFIX[] = "library_import.itunes";
char const SB_ITUNES_LIBRARY_IMPORTER_PREF_PREFIX[] = "nightingale.library_importer.";

#ifdef PR_LOGGING
static PRLogModuleInfo* giTunesImporter = nsnull;
#define TRACE(args) \
  PR_BEGIN_MACRO \
  if (!giTunesImporter) \
  giTunesImporter = PR_NewLogModule("sbiTunesImporter"); \
  PR_LOG(giTunesImporter, PR_LOG_DEBUG, args); \
  PR_END_MACRO
#define LOG(args) \
  PR_BEGIN_MACRO \
  if (!giTunesImporter) \
  giTunesImporter = PR_NewLogModule("sbiTunesImporter"); \
  PR_LOG(giTunesImporter, PR_LOG_WARN, args); \
  PR_END_MACRO


#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * typedef for conversion functions used to convert values from iTunes to 
 * Nightingale
 */
typedef nsString (*ValueConversion)(nsAString const & aValue);

/**
 * Convert the iTunes rating value specified by aITunesMetaValue to a Nightingale
 * rating property value and return the result. 1 star is the same as 20 in 
 * iTunes
 *
 * \param aRating iTunes rating
 *
 * \return Nightingale property value.
 */

nsString ConvertRating(nsAString const & aRating) {
  nsresult rv;
  if (aRating.IsEmpty()) {
    return nsString();
  }
  PRInt32 rating = aRating.ToInteger(&rv, 10);
  nsString result;
  if (NS_SUCCEEDED(rv)) {
    result.AppendInt((rating + 10) / 20, 10);
  }
  return result;
}

/**
 * Convert the iTunes duration value specified by aITunesMetaValue to a
 * Nightingale duration property value and return the result.
 *
 * \param aDuration iTunes duration in seconds.
 *
 * \return Nightingale property value.
 */

nsString ConvertDuration(nsAString const & aDuration) {
  nsresult rv;
  if (aDuration.IsEmpty()) {
    return nsString();
  }
  PRInt32 const duration = aDuration.ToInteger(&rv, 10);
  nsString result;
  if (NS_SUCCEEDED(rv)) {
    result.AppendInt(duration * 1000);
  }
  return result;
}

/**
 * Convert the iTunes date/time value specified by aITunesMetaValue to a
 * Nightingale date/time property value and return the result.
 *
 * \param aDateTime iTunes date/time value.
 *
 * \return Nightingale property value.
 */

nsString ConvertDateTime(nsAString const & aDateTime) {
  // If empty just return empty
  if (aDateTime.IsEmpty()) {
    return nsString();
  }

  // Convert "1970-01-31T00:00:00Z" to "01-31-1970 00:00:00 GMT" for parsing
  // by PR_ParseTimeString

  // Split off "Z" and anything after (time zone spec?)
  nsCAutoString dateTime = ::NS_LossyConvertUTF16toASCII(aDateTime);
  nsTArray<nsCString> splitString;
  nsCString_Split(dateTime, NS_LITERAL_CSTRING("Z"), splitString);
  NS_ENSURE_TRUE(splitString.Length() > 0, nsString());
  dateTime = splitString[0];

  // Split up the date and time strings
  nsCString_Split(dateTime, NS_LITERAL_CSTRING("T"), splitString);
  NS_ENSURE_TRUE(splitString.Length() > 1, nsString());
  nsCAutoString dateString = splitString[0];
  nsCAutoString timeString = splitString[1];

  // Split up the day, month, and year strings
  nsCString_Split(dateString, NS_LITERAL_CSTRING("-"), splitString);
  NS_ENSURE_TRUE(splitString.Length() > 2, nsString());
  nsCAutoString yearString = splitString[0];
  nsCAutoString monthString = splitString[1];
  nsCAutoString dayString = splitString[2];

  // Produce the "01-31-1970 00:00:00 GMT" format
  char cDateTime[128];
  cDateTime[0] = '\0';
  PR_snprintf(cDateTime,
              sizeof(cDateTime),
              "%s-%s-%s %s GMT",
              monthString.get(),
              dayString.get(),
              yearString.get(),
              timeString.get());

  // Parse the date/time string into epoch time
  PRTime prTime;
  PRStatus status = PR_ParseTimeString(cDateTime, PR_TRUE, &prTime);

  // On failure just return an emptry string
  if (status == PR_FAILURE) {
    NS_WARNING("Failed to parse time in sbiTunesImporter ConvertDateTime");
    return nsString();
  }

  // Convert time to microseconds
  prTime /= PR_USEC_PER_MSEC;

  return sbAutoString(static_cast<PRUint64>(prTime));
}

/**
 * Convert the iTunes 'kind' value specified by aITunesMetaValue to a
 * Nightingale contentType property value and return the result.
 *
 * \param aKind iTunes media kind value.
 *
 * \return Nightingale property value.
 */

nsString ConvertKind(nsAString const & aKind) {
  nsString result;
  
  if (aKind.Find("video") != -1) {
    result = NS_LITERAL_STRING("video");
  }
  else if (aKind.Find("audio") != -1) {
    result = NS_LITERAL_STRING("audio");
  }
  else if (aKind.EqualsLiteral("true")) {
    result = NS_LITERAL_STRING("podcast");
  }
 
  return result;
}

struct PropertyMap {
  char const * SBProperty;
  char const * ITProperty;
  ValueConversion mConversion;
};

/**
 * Mapping between Nightingale properties and iTunes
 */
PropertyMap gPropertyMap[] = {
  { SB_PROPERTY_ITUNES_GUID,      "Persistent ID", 0 },
  { SB_PROPERTY_ALBUMARTISTNAME,  "Album Artist", 0 },
  { SB_PROPERTY_ALBUMNAME,        "Album", 0 },
  { SB_PROPERTY_ARTISTNAME,       "Artist", 0 },
  { SB_PROPERTY_BITRATE,          "Bit Rate", 0 },
  { SB_PROPERTY_BPM,              "BPM", 0 },
  { SB_PROPERTY_COMMENT,          "Comments", 0 },
  { SB_PROPERTY_COMPOSERNAME,     "Composer", 0 },
  { SB_PROPERTY_CONTENTTYPE,      "Kind", ConvertKind },
  { SB_PROPERTY_DISCNUMBER,       "Disc Number", 0 },
  { SB_PROPERTY_DURATION,         "Total Time", ConvertDuration },
  { SB_PROPERTY_GENRE,            "Genre", 0},
  { SB_PROPERTY_LASTPLAYTIME,     "Play Date UTC", ConvertDateTime },
  { SB_PROPERTY_LASTSKIPTIME,     "Skip Date", ConvertDateTime },
  { SB_PROPERTY_PLAYCOUNT,        "Play Count", 0 },
  { SB_PROPERTY_CONTENTTYPE,      "Podcast", ConvertKind },
  { SB_PROPERTY_RATING,           "Rating", ConvertRating },
  { SB_PROPERTY_SAMPLERATE,       "Sample Rate", 0 },
  { SB_PROPERTY_SKIPCOUNT,        "Skip Count", 0 },
  { SB_PROPERTY_TOTALDISCS,       "Disc Count", 0 },
  { SB_PROPERTY_TOTALTRACKS,      "Track Count", 0 },
  { SB_PROPERTY_TRACKNAME,        "Name", 0 },
  { SB_PROPERTY_TRACKNUMBER,      "Track Number", 0 },
  { SB_PROPERTY_YEAR,             "Year", 0 },
};

NS_IMPL_ISUPPORTS2(sbiTunesImporter, sbILibraryImporter,
                                     sbIiTunesXMLParserListener)

sbiTunesImporter::sbiTunesImporter() : mBatchEnded(PR_FALSE),
                                       mDataFormatVersion(DATA_FORMAT_VERSION),
                                       mFoundChanges(PR_FALSE),
                                       mImportPlaylists(PR_TRUE),
                                       mMissingMediaCount(0),
                                       mOSType(UNINITIALIZED),
                                       mTrackCount(0),
                                       mUnsupportedMediaCount(0)
  
{
  mTrackBatch.reserve(BATCH_SIZE);
}

sbiTunesImporter::~sbiTunesImporter()
{
  // Just make sure we were finalized
  Finalize();
}

nsresult
sbiTunesImporter::Cancel() {
  nsresult rv;
  nsString msg;
  rv = 
    SBGetLocalizedString(msg,
                         NS_LITERAL_STRING("import_library.job.status.cancelled"));
  if (NS_FAILED(rv)) { 
    // Show at least something
    msg = NS_LITERAL_STRING("Library import cancelled");
  }
  mStatus->SetStatusText(msg);
  mStatus->Done();
  mStatus->Update();
  return NS_OK;
}

/* readonly attribute AString libraryType; */
NS_IMETHODIMP 
sbiTunesImporter::GetLibraryType(nsAString & aLibraryType)
{
  aLibraryType = NS_LITERAL_STRING("iTunes");
  return NS_OK;
}

/* readonly attribute AString libraryReadableType; */
NS_IMETHODIMP
sbiTunesImporter::GetLibraryReadableType(nsAString & aLibraryReadableType)
{
  aLibraryReadableType = NS_LITERAL_STRING("iTunes");
  return NS_OK;
}

/* readonly attribute AString libraryDefaultFileName; */
NS_IMETHODIMP
sbiTunesImporter::GetLibraryDefaultFileName(nsAString & aLibraryDefaultFileName)
{
  aLibraryDefaultFileName = NS_LITERAL_STRING("iTunes Music Library.xml");
  return NS_OK;
}

/* readonly attribute AString libraryDefaultFilePath; */
NS_IMETHODIMP
sbiTunesImporter::GetLibraryDefaultFilePath(nsAString & aLibraryDefaultFilePath)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_CreateInstance("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIFile> libraryFile;

  nsString defaultLibraryName;
  rv = GetLibraryDefaultFileName(defaultLibraryName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  /* Search for an iTunes library database file. */
  switch (GetOSType())
  {
    case MAC_OS : {
      rv = directoryService->Get("Music",
                                 NS_GET_IID(nsIFile),
                                 getter_AddRefs(libraryFile));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = libraryFile->Append(NS_LITERAL_STRING("iTunes"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
    case WINDOWS_OS : {
      rv = directoryService->Get("Music",
                                 NS_GET_IID(nsIFile),
                                 getter_AddRefs(libraryFile));

      if (NS_FAILED(rv)) {
        rv = directoryService->Get("Pers",
                                   NS_GET_IID(nsIFile),
                                   getter_AddRefs(libraryFile));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = libraryFile->Append(NS_LITERAL_STRING("My Music"));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = libraryFile->Append(NS_LITERAL_STRING("iTunes"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
    default : {
      /* We _could_ use XDGMusic, but since Linux doesn't actually have iTunes
       * the user must have copied it manually - which means it wouldn't
       * actually make sense to try to be smart.
       */
      rv = directoryService->Get("Home",
                                 NS_GET_IID(nsIFile),
                                 getter_AddRefs(libraryFile));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }

  rv = libraryFile->Append(defaultLibraryName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  /* If the library file exists, get its path. */
  PRBool exists = PR_FALSE;
  rv = libraryFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (exists) {
    nsString path;
    rv = libraryFile->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aLibraryDefaultFilePath = path;
    return NS_OK;
  }
  
  return NS_OK;
}

/* readonly attribute AString libraryFileExtensionList; */
NS_IMETHODIMP 
sbiTunesImporter::GetLibraryFileExtensionList(
    nsAString & aLibraryFileExtensionList)
{
  aLibraryFileExtensionList = NS_LITERAL_STRING("xml");
  return NS_OK;
}

/* readonly attribute boolean libraryPreviouslyImported; */
NS_IMETHODIMP 
sbiTunesImporter::GetLibraryPreviouslyImported(
    PRBool *aLibraryPreviouslyImported)
{
  nsresult rv;
  sbPrefBranch prefs(SB_ITUNES_LIBRARY_IMPORT_PREF_PREFIX , &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCString const & temp = prefs.GetCharPref("lib_prev_mod_time", nsCString());
  *aLibraryPreviouslyImported = temp.IsEmpty() ? PR_FALSE : PR_TRUE;
  return NS_OK;
}

/* readonly attribute AString libraryPreviousImportPath; */
NS_IMETHODIMP
sbiTunesImporter::GetLibraryPreviousImportPath(
    nsAString & aLibraryPreviousImportPath)
{
  nsresult rv;
  sbPrefBranch prefs(SB_ITUNES_LIBRARY_IMPORT_PREF_PREFIX , &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  aLibraryPreviousImportPath = 
    ::NS_ConvertUTF8toUTF16(prefs.GetCharPref("lib_prev_path", 
                                              nsCString()));
  return NS_OK;
}

/* void initialize (); */
NS_IMETHODIMP
sbiTunesImporter::Initialize()
{
  nsresult rv = miTunesLibSig.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);
  
  mIOService = 
    do_CreateInstance("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mAlbumArtFetcher = 
    do_CreateInstance("@getnightingale.com/Nightingale/album-art-fetcher-set;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset flags to their default value
  mBatchEnded = PR_FALSE;
  mFoundChanges = PR_FALSE;
  mUnsupportedMediaCount = 0;
  mMissingMediaCount = 0;
  
  rv = GetMainLibrary(getter_AddRefs(mLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  mLDBLibrary = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
#ifdef SB_ENABLE_TEST_HARNESS
  // Ignore errors, we'll just not do timing
  mTimingService = do_GetService("@getnightingale.com/Nightingale/TimingService;1", &rv);
#endif
  
  rv = miTunesDBServices.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Read in the added results file
  do { /* scope, and to jump out of */
    nsCOMPtr<nsIFile> resultsFile;
    rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR,
                                getter_AddRefs(resultsFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = resultsFile->Append(NS_LITERAL_STRING("itunesexportresults.txt"));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success;
    rv = resultsFile->Exists(&success);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!success) {
      // no file, escape from scope
      break;
    }

    nsCOMPtr<nsIInputStream> inputStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), resultsFile);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(inputStream, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    do { // loop over lines
      nsCAutoString line;
      rv = lineStream->ReadLine(line, &success);
      NS_ENSURE_SUCCESS(rv, rv);

      if (StringBeginsWith(line, NS_LITERAL_CSTRING("["))) {
        // skip for now
        continue;
      }

      // line format is
      // <nightingale guid>=<library persistent id>,<track database id>
      // with only the last part be variable-length
      const int LEN_GUID = 36;
      const int LEN_PID = 16;

      if (line.Length() < LEN_GUID + LEN_PID + 2) {
        continue;
      }
      if (line[LEN_GUID] != '=' ||
          line[LEN_GUID + 1 + LEN_PID] != ',')
      {
        // invalidly formatted line
        continue;
      }

      NS_ConvertASCIItoUTF16 sbid(Substring(line, 0, LEN_GUID)),
                             libid(Substring(line, LEN_GUID + 1, LEN_PID)),
                             id(Substring(line, LEN_GUID + 2 + LEN_PID));
      miTunesDBServices.MapID(libid, id, sbid);
    } while (success);
    /* rv = */ inputStream->Close();
    /* rv = */ resultsFile->Remove(PR_FALSE);
  } while(false);
  
  mPlaylistBlacklist = 
    SBLocalizedString(NS_LITERAL_STRING("import_library.itunes.excluded_playlists"), 
                      nsString());
  return NS_OK;
}

/* void finalize (); */
NS_IMETHODIMP 
sbiTunesImporter::Finalize()
{
  if (!mBatchEnded) {
    mBatchEnded = PR_TRUE;
    if (mLDBLibrary) {
      mLDBLibrary->ForceEndUpdateBatch();
    }
  }
  mListener = nsnull;
  mLibrary = nsnull;
  mLDBLibrary = nsnull;
  if (mStatus.get()) {
    mStatus->Finalize();
  }
  return NS_OK;
}

nsresult sbiTunesImporter::DBModified(sbPrefBranch & aPrefs,
                                       nsAString const & aLibPath,
                                       PRBool * aModified) {
  *aModified = PR_TRUE;
  nsresult rv;
  
  // Check that the path is the same
  nsString prevPath;
  rv = GetLibraryPreviousImportPath(prevPath);
  if (NS_FAILED(rv) || !aLibPath.Equals(prevPath)) {
    return NS_OK;
  }
  
  // Check the last modified times
  nsCOMPtr<nsILocalFile> file = do_CreateInstance(NS_LOCALFILE_CONTRACTID);
  rv = file->InitWithPath(aLibPath);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }
  
  PRInt64 lastModified;
  rv = file->GetLastModifiedTime(&lastModified);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }
  nsCString const & temp = aPrefs.GetCharPref("lib_prev_mod_time", nsCString());
  if (temp.IsEmpty()) {
    return NS_OK;
  }
  PRInt64 prevLastModified = 
    nsString_ToInt64(NS_ConvertASCIItoUTF16(temp), &rv);
  if (NS_SUCCEEDED(rv)) {
    *aModified = lastModified != prevLastModified;
  }
  return NS_OK;
}
/* sbIJobProgress import (in AString aLibFilePath, 
 *                        in AString aGUID,
 *                        in boolean aCheckForChanges); */
NS_IMETHODIMP 
sbiTunesImporter::Import(const nsAString & aLibFilePath, 
                          const nsAString & aGUID, 
                          PRBool aCheckForChanges, 
                          sbIJobProgress ** aJobProgress)
{
  NS_ENSURE_ARG_POINTER(aJobProgress);
  TRACE(("sbiTunesImporter::Import(%s, %s, %s)",
         NS_LossyConvertUTF16toASCII(aLibFilePath).get(),
         NS_LossyConvertUTF16toASCII(aGUID).get(),
         (aCheckForChanges ? "true" : "false")));
  // Must be started on the main thread
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_FAILURE);
  
  nsresult rv;

  // reset members
  mFoundChanges = PR_FALSE;
  mMissingMediaCount = 0;
  mTrackCount = 0;
  mUnsupportedMediaCount = 0;

  mLibraryPath = aLibFilePath;
  mImport = aCheckForChanges ? PR_FALSE : PR_TRUE;
  sbPrefBranch prefs(SB_ITUNES_LIBRARY_IMPORT_PREF_PREFIX, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<sbiTunesImporterJob> jobProgress = sbiTunesImporterJob::New();
  mStatus = 
    std::auto_ptr<sbiTunesImporterStatus>(sbiTunesImporterStatus::New(jobProgress));
  NS_ENSURE_TRUE(mStatus.get(), NS_ERROR_FAILURE);

  mStatus->Initialize();

  mDataFormatVersion = prefs.GetIntPref("version", DATA_FORMAT_VERSION);
  // If we're checking for changes and the db wasn't modified, just exit
  PRBool modified;
  if (!mImport && NS_SUCCEEDED(DBModified(prefs, 
                                         mLibraryPath, 
                                         &modified)) && !modified) {
    /* Update status. */
    rv = mStatus->Reset();
    NS_ENSURE_SUCCESS(rv, rv);
    
    mStatus->SetStatusText(NS_LITERAL_STRING("No library changes found"));
    mStatus->Done();
    mStatus->Update();
    return NS_OK;
  }
  mImportPlaylists = PR_FALSE;
  mBatchEnded = PR_FALSE;  
  
  if (mImport) {
    sbPrefBranch userPrefs(SB_ITUNES_LIBRARY_IMPORTER_PREF_PREFIX, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mImportPlaylists = userPrefs.GetBoolPref("import_playlists", PR_FALSE);
  }
  
  mTypeSniffer = do_CreateInstance("@getnightingale.com/Nightingale/Mediacore/TypeSniffer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
#ifdef SB_ENABLE_TEST_HARNESS 
  if (mTimingService) {
      mTimingIdentifier = NS_LITERAL_STRING("ITunesImport-");
      mTimingIdentifer.AppendInt(time(0));
      mTimingService->StartPerfTimer(mTimingIdentifier);
  }
#endif
  
  rv = sbOpenInputStream(mLibraryPath, getter_AddRefs(mStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1", 
                                                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = file->InitWithPath(mLibraryPath);
  if (NS_SUCCEEDED(rv)) {
    PRInt64 size;
    rv = file->GetFileSize(&size);
    if (NS_SUCCEEDED(rv)) {
      mStatus->SetProgressMax(size);
    }
  }
  
  
  nsAString const & msg = 
    mImport ? NS_LITERAL_STRING("Importing library") :
              NS_LITERAL_STRING("Checking for changes in library");

  mStatus->SetStatusText(msg);

  // Scope batching
  {
    // Put the library in batch mode
    mLDBLibrary->ForceBeginUpdateBatch();
    
    mParser = sbiTunesXMLParser::New();


    rv = mParser->Parse(mStream, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aJobProgress = jobProgress;
  NS_IF_ADDREF(*aJobProgress);
  return NS_OK;
}

/* void setListener (in sbILibraryImporterListener aListener); */
NS_IMETHODIMP 
sbiTunesImporter::SetListener(sbILibraryImporterListener *aListener)
{
  NS_WARN_IF_FALSE(mListener == nsnull && aListener, "Listener was previously set");
  
  mListener = aListener;
  
  return NS_OK;
}

sbiTunesImporter::OSType sbiTunesImporter::GetOSType()
{
  if (mOSType == UNINITIALIZED) {
    nsresult rv;
    nsCOMPtr<nsIXULRuntime> appInfo = 
      do_CreateInstance("@mozilla.org/xre/app-info;1", &rv);
    NS_ENSURE_SUCCESS(rv, UNKNOWN_OS);
    
    nsCString osName;
    rv = appInfo->GetOS(osName);
    NS_ENSURE_SUCCESS(rv, UNKNOWN_OS);
    
    ToLowerCase(osName);
    
    /* Determine the OS type. */
    if (osName.Find("darwin") != -1) {
      mOSType = MAC_OS;
    }
    else if (osName.Find("linux") != -1) {
      mOSType = LINUX_OS;
    }
    else if (osName.Find("win") != -1) {
      mOSType = WINDOWS_OS;
    }
    else { 
      mOSType = UNKNOWN_OS;
    }
  }
  return mOSType;
}

// sbIiTunesXMLParserListener implementation

/* void onTopLevelProperties (in sbIStringMap aProperties); */
NS_IMETHODIMP sbiTunesImporter::OnTopLevelProperties(sbIStringMap *aProperties) {
  NS_ENSURE_ARG_POINTER(aProperties);
  
  nsresult rv = aProperties->Get(NS_LITERAL_STRING("Library Persistent ID"), 
                                 miTunesLibID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString id(NS_LITERAL_STRING("Library Persistent ID"));
  id.Append(miTunesLibID);
  rv = miTunesLibSig.Update(id);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP 
sbiTunesImporter::OnTrack(sbIStringMap *aProperties) {
  NS_ENSURE_ARG_POINTER(aProperties);
  
  if (mStatus->CancelRequested()) {
    Cancel();
    return NS_ERROR_ABORT;
  }
  nsresult rv = UpdateProgress();
  
  nsAutoPtr<iTunesTrack> track(new iTunesTrack);
  NS_ENSURE_TRUE(track, NS_ERROR_OUT_OF_MEMORY);

  rv  = track->Initialize(aProperties);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG  
  nsString uri16;
  if (track->mProperties.Get(NS_LITERAL_STRING("Location"), &uri16)) {
    LOG(("Importing Track %s\n", NS_ConvertUTF16toUTF8(uri16).get()));
  }
#endif
  // If there is no persistent ID, then skip it
  nsString persistentID;
  if (!track->mProperties.Get(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID), 
                              &persistentID)) {
    return NS_OK;
  }
  mTrackBatch.push_back(track.forget());
  if (mTrackBatch.size() == BATCH_SIZE) {
    ProcessTrackBatch();
  }
  return NS_OK;
}

NS_IMETHODIMP 
sbiTunesImporter::OnTracksComplete() {
  if (!mStatus->CancelRequested() && mTrackBatch.size() > 0) {
    ProcessTrackBatch();
  }
  return NS_OK;
}

PRBool 
sbiTunesImporter::ShouldImportPlaylist(sbIStringMap * aProperties) {
  
  nsString playlistName;
  nsresult rv = aProperties->Get(NS_LITERAL_STRING("Name"), playlistName);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  // If we've seen the nightingale folder check the parent ID of the playlist
  // and if the parent is Nightingale we don't need to import this playlist
  // since this is a Nightingale playlist
  if (!mNightingaleFolderID.IsEmpty()) {
    nsString parentID;
    rv = aProperties->Get(NS_LITERAL_STRING("Parent Persistent ID"), parentID);
    if (NS_FAILED(rv) || parentID.Equals(mNightingaleFolderID)) {
      return PR_FALSE;
    }
  }
  nsString master;
  // Don't care if this errors
  aProperties->Get(NS_LITERAL_STRING("Master"), master);
  
  nsString smartInfo;
  aProperties->Get(NS_LITERAL_STRING("Smart Info"), smartInfo);
  
  nsString isFolder;
  aProperties->Get(NS_LITERAL_STRING("Folder"), isFolder);
  
  nsString delimitedName;
  delimitedName.AppendLiteral(":");
  delimitedName.Append(playlistName);
  delimitedName.AppendLiteral(":");
  // If it has tracks, is not the master playlist, is not a folder, is not a 
  // smart playlist, and is not on the black list it should be imported
  return !master.EqualsLiteral("true") &&
         smartInfo.IsEmpty() &&
         !isFolder.EqualsLiteral("true") &&
         mPlaylistBlacklist.Find(delimitedName) == -1;
}

/**
 * Determines if this is the special Nightingale folder for playlists
 */
static PRBool 
IsNightingaleFolder(sbIStringMap * aProperties) {
  
  nsString playlistName;
  nsresult rv = aProperties->Get(NS_LITERAL_STRING("Name"), playlistName);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  nsString smartInfo;
  aProperties->Get(NS_LITERAL_STRING("Smart Info"), smartInfo);
  
  nsString isFolder;
  aProperties->Get(NS_LITERAL_STRING("Folder"), isFolder);

  return smartInfo.IsEmpty() &&
         isFolder.EqualsLiteral("true") &&
         playlistName.EqualsLiteral("Nightingale");
}

static nsresult
FindPlaylistByName(sbILibrary * aLibrary,
                   nsAString const & aPlaylistName,
                   sbIMediaList ** aMediaList) {
  NS_ENSURE_ARG_POINTER(aMediaList);
  // Clear out so if we fail to find we return an empty string  
  nsCOMArray<sbIMediaItem> mediaItems;
  nsresult rv = 
    sbLibraryUtils::GetItemsByProperty(aLibrary, 
                                       NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                                       aPlaylistName,
                                       mediaItems);
  if (NS_SUCCEEDED(rv) && mediaItems.Count() > 0) {
    NS_WARN_IF_FALSE(mediaItems.Count() > 1, "We got multiple playlists for "
                                             "the same name in iTunesImport");
    // nsCOMArray doesn't addref on access so no need for an nsCOMPtr here
    sbIMediaItem * mediaItem = mediaItems.ObjectAt(0);
    if (mediaItem) {
      rv = mediaItem->QueryInterface(NS_GET_IID(sbIMediaList),
                                     reinterpret_cast<void**>(aMediaList));
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }
  // Not found so return null
  *aMediaList = nsnull;
  return NS_OK;
}

nsresult
sbiTunesImporter::UpdateProgress() {
  mStatus->SetProgress(mBytesRead);
  return NS_OK;
}

NS_IMETHODIMP
sbiTunesImporter::OnPlaylist(sbIStringMap *aProperties, 
                             PRInt32 *aTrackIds, 
                             PRUint32 aTrackIdsCount)
{
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aTrackIds);
  
  if (mStatus->CancelRequested()) {
    Cancel();
    return NS_ERROR_ABORT;
  }
  nsresult rv = UpdateProgress();
  // If this is the Nightingale folder save its ID
  if (IsNightingaleFolder(aProperties)) {
    // Ignore errors, if not found it's left empty
    aProperties->Get(NS_LITERAL_STRING("Playlist Persistent ID"), 
                     mNightingaleFolderID);
  }
  else if (ShouldImportPlaylist(aProperties)) {
    nsString playlistID;
    rv = aProperties->Get(NS_LITERAL_STRING("Playlist Persistent ID"), 
                          playlistID);
    NS_ENSURE_SUCCESS(rv, rv);
          
    NS_NAMED_LITERAL_STRING(name, "Name");
    nsString playlistName;
    rv = aProperties->Get(name, playlistName);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("Importing playlist %s\n", NS_ConvertUTF16toUTF8(playlistName).get()));
    nsString text(name);
    text.Append(playlistName);
    
    rv = miTunesLibSig.Update(text);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (mImportPlaylists) {
      nsString playlistSBGUID;
      rv = miTunesDBServices.GetSBIDFromITID(miTunesLibID, playlistID, 
                                             playlistSBGUID);
      nsCOMPtr<sbIMediaList> mediaList; 
      if ((NS_FAILED(rv) || playlistSBGUID.IsEmpty()) 
          && mDataFormatVersion < 2) {
        rv = FindPlaylistByName(mLibrary,
                                playlistName, 
                                getter_AddRefs(mediaList));
        NS_ENSURE_SUCCESS(rv, rv);        
      }
      else if (!playlistSBGUID.IsEmpty()) {
        nsCOMPtr<sbIMediaItem> mediaListAsItem;
        // Get the media list as an item, if it fails then just continue with
        // mediaList being null
        rv = mLibrary->GetItemByGuid(playlistSBGUID, getter_AddRefs(mediaListAsItem));
        if (NS_SUCCEEDED(rv)) {
          // if this errors, mediaList will be null which is fine
          mediaList = do_QueryInterface(mediaListAsItem);
        }
      }
      ImportPlaylist(aProperties,
                     aTrackIds,
                     aTrackIdsCount,
                     mediaList);
    }
  }
  return NS_OK;
}

static nsresult
ComputePlaylistSignature(sbiTunesSignature & aSignature, 
                         sbIMediaList * aMediaList) {
  PRUint32 length;
  nsresult rv = aMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString guid;
  nsCOMPtr<sbIMediaItem> mediaItem;
  for (PRUint32 index = 0; index < length; ++index) {
    rv = aMediaList->GetItemByIndex(index, getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aSignature.Update(guid);
  }
  return NS_OK;
}

static nsresult
IsPlaylistDirty(sbIMediaList * aMediaList, 
                PRBool & aIsDirty) {
  sbiTunesSignature signature;
  nsresult rv = signature.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = ComputePlaylistSignature(signature, aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString computedSignature;
  rv = signature.GetSignature(computedSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString playlistGuid;
  rv = aMediaList->GetGuid(playlistGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString storedSignature;
  rv = signature.RetrieveSignature(playlistGuid, storedSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  aIsDirty = !computedSignature.Equals(storedSignature);
  
  return NS_OK;
}

nsresult
sbiTunesImporter::GetDirtyPlaylistAction(nsAString const & aPlaylistName,
                                         nsAString & aAction) {
  aAction = NS_LITERAL_STRING("replace");
  // If the user hasn't given an action for all yet, ask them again
  if (mPlaylistAction.IsEmpty()) { 
  
    PRBool applyAll;
    nsresult rv = mListener->OnDirtyPlaylist(aPlaylistName,
                                              &applyAll, 
                                              aAction);
    NS_ENSURE_SUCCESS(rv, rv);
    if (applyAll) {
      mPlaylistAction = aAction;
    }
  }
  else { // The user gave us an answer for the rest, so use that
    aAction = mPlaylistAction;
  }
  return NS_OK;
}

static nsresult
AddItemsToPlaylist(sbIMediaList * aMediaList,
                   nsIMutableArray * aItems) {
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = aItems->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aMediaList->AddSome(enumerator);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbiTunesImporter::ProcessPlaylistItems(sbIMediaList * aMediaList,
                                       PRInt32 * aTrackIds,
                                       PRUint32 aTrackIdsCount) {
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aTrackIds);
  
  nsresult rv;
  nsCOMPtr<nsIMutableArray> tracks = 
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  for (PRUint32 index = 0; index < aTrackIdsCount; ++index) {
    // If we're at a batch point
    if (((index + 1) % BATCH_SIZE) == 0) {
      rv = AddItemsToPlaylist(aMediaList, tracks);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = tracks->Clear();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    nsString trackID;
    trackID.AppendInt(aTrackIds[index], 10);
    
    // Add the iTuens track persistent ID to the iTunes library signature
    nsString text;
    text.AppendLiteral("Persistent ID");
    text.Append(miTunesLibID);
    text.Append(trackID);
    rv = miTunesLibSig.Update(text);
    NS_ENSURE_SUCCESS(rv, rv);
    
    TrackIDMap::const_iterator iter = mTrackIDMap.find(trackID);
    if (iter != mTrackIDMap.end()) {
      rv = mLibrary->GetItemByGuid(iter->second, getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = tracks->AppendElement(mediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  rv = AddItemsToPlaylist(aMediaList, tracks);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

static nsresult
StorePlaylistSignature(sbIMediaList * aMediaList) {
  
  sbiTunesSignature signature;
  nsresult rv = signature.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = ComputePlaylistSignature(signature, aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString theSignature;
  rv = signature.GetSignature(theSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString guid;
  rv = aMediaList->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);
    
  rv = signature.StoreSignature(guid, theSignature);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbiTunesImporter::ImportPlaylist(sbIStringMap *aProperties, 
                                 PRInt32 *aTrackIds, 
                                 PRUint32 aTrackIdsCount,
                                 sbIMediaList * aMediaList) {
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aTrackIds);
  
  nsresult rv;
  
  nsCOMPtr<sbIMediaList> mediaList(aMediaList);
  PRBool isDirty = PR_TRUE;
  if (mediaList) {
    rv = IsPlaylistDirty(mediaList, isDirty);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  nsString playlistiTunesID;
  rv = aProperties->Get(NS_LITERAL_STRING("Playlist Persistent ID"),
                        playlistiTunesID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString playlistName;
  rv = aProperties->Get(NS_LITERAL_STRING("Name"), playlistName);
  NS_ENSURE_SUCCESS(rv, rv);
    
  nsCString action("replace");
  if (!mImportPlaylists) {
    action = "keep";
  }
  else if (mediaList && isDirty) {   
    nsString userAction;
    rv = GetDirtyPlaylistAction(playlistName, userAction);
    NS_ENSURE_SUCCESS(rv, rv);
    action = NS_LossyConvertUTF16toASCII(userAction);
  }
  if (action.Equals("replace")) {
    mFoundChanges = PR_TRUE;
    if (mediaList) {
      nsString guid;
      rv = mediaList->GetGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mLibrary->Remove(mediaList);
      NS_ENSURE_SUCCESS(rv, rv);
      
      // Playlist is dead so no more references
      mediaList = nsnull;
      
      // Remove the old entry
      rv = miTunesDBServices.RemoveSBIDEntry(guid);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (aTrackIdsCount > 0) {
      
      // Setup the properties. We need to do properties so that when
      // export sees this it knows it's an iTunes playlist
      nsCOMPtr<sbIMutablePropertyArray> properties =
        do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                                      playlistName);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                                      playlistiTunesID);
      NS_ENSURE_SUCCESS(rv, rv);
     
      rv = mLibrary->CreateMediaList(NS_LITERAL_STRING("simple"), 
                                     properties, 
                                     getter_AddRefs(mediaList));
      NS_ENSURE_SUCCESS(rv, rv);
      
      // Now add the Nightingale and iTunes ID's to the map table
      nsString guid;
      rv = mediaList->GetGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = miTunesDBServices.MapID(miTunesLibID, playlistiTunesID, guid);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = ProcessPlaylistItems(mediaList,
                                aTrackIds,
                                aTrackIdsCount);
      NS_ENSURE_SUCCESS(rv, rv);
    
      StorePlaylistSignature(mediaList);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
sbiTunesImporter::OnPlaylistsComplete() {
  /* Update status. */
  mStatus->Reset();
  char const * completionMsg;
  if (mImport) {
    completionMsg = "Library import complete";
  }
  else {
    if (mFoundChanges) {
      completionMsg = "Found library changes";
    }
    else {
      completionMsg = "No library changes found";
    }
  }
  
  if (!mBatchEnded) {
    mLDBLibrary->ForceEndUpdateBatch();
    mBatchEnded = PR_TRUE;
  }
  
  mStatus->SetStatusText(NS_ConvertASCIItoUTF16(completionMsg));
  
  mStatus->Done();
  mStatus->Update();

#ifdef SB_ENABLE_TEST_HARNESS
  if (mTimingService) {
    mTimingService->StopPerfTimer(mTimingIdentifier);
  }
#endif
  
  /* If checking for changes and changes were */
  /* found, send a library changed event.     */
  if (!mImport && mFoundChanges) {
      mListener->OnLibraryChanged(mLibraryPath, miTunesLibID);
  }
  /* If non-existent media is encountered, send an event. */
  if (mImport) {
      nsresult rv;
      sbPrefBranch prefs(SB_ITUNES_LIBRARY_IMPORT_PREF_PREFIX, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      // Update the previous path preference
      prefs.SetCharPref("lib_prev_path", NS_ConvertUTF16toUTF8(mLibraryPath));
      
      // Update the last modified time
      nsCOMPtr<nsILocalFile> file = 
        do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = file->InitWithPath(mLibraryPath);
      NS_ENSURE_SUCCESS(rv, rv);
      
      PRInt64 lastModified;
      file->GetLastModifiedTime(&lastModified);
      sbAutoString lastModifiedStr(static_cast<PRUint64>(lastModified));
      prefs.SetCharPref("lib_prev_mod_time",
                        NS_ConvertUTF16toUTF8(lastModifiedStr));
    if (mMissingMediaCount > 0)  {
      mListener->OnNonExistentMedia(mMissingMediaCount,
                                    mTrackCount);
    }

    /* If unsupported media is encountered, send an event. */
    if (mUnsupportedMediaCount > 0) {
      mListener->OnUnsupportedMedia();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
sbiTunesImporter::OnError(const nsAString & aErrorMessage, PRBool *_retval)
{
  LOG(("XML Parsing error: %s\n", 
      ::NS_LossyConvertUTF16toASCII(aErrorMessage).get()));
  if (mStatus.get() && !mStatus->CancelRequested()) {
    mListener->OnImportError();
  }
  return NS_OK;
}

/**
 * This is a helper function that enumerates properties and compares the values
 * to the property array passed to the constructor. It builds a list of
 * properties that are different in mChangedProperties.
 */
struct sbiTunesImporterEnumeratePropertiesData
{
  /**
   * Initialize the internal properties and the changed property array
   */
  sbiTunesImporterEnumeratePropertiesData(sbIPropertyArray * aProperties, 
                                          nsresult * rv) :
    mProperties(aProperties) {
    mChangedProperties = do_CreateInstance("@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1", rv);
  }
  /**
   * Returns true if there are any changed properties
   */
  bool NeedsUpdating() const
  {
    PRUint32 length = 0;
    // In case of error, just let length be 0
    mChangedProperties->GetLength(&length);
    return length != 0;
  }
  /**
   * A list of properties to compare 
   */
  nsCOMPtr<sbIPropertyArray> mProperties;
  /**
   * The list of properties that have changed
   */
  nsCOMPtr<sbIMutablePropertyArray> mChangedProperties;
};

static PLDHashOperator
EnumReadFunc(nsAString const & aKey,
             nsString aValue,
             void* aUserArg) {
  NS_ENSURE_TRUE(aUserArg, PL_DHASH_STOP);
  
  sbiTunesImporterEnumeratePropertiesData * data = 
    reinterpret_cast<sbiTunesImporterEnumeratePropertiesData *>(aUserArg);
  nsString currentValue;
  data->mProperties->GetPropertyValue(aKey, currentValue);
  if (!aValue.Equals(currentValue)) {
    data->mChangedProperties->AppendProperty(aKey, aValue);
  }
  return PL_DHASH_NEXT;
}

nsresult sbiTunesImporter::ProcessUpdates() {
  nsresult rv;
  TrackBatch::iterator const end = mTrackBatch.end();
  for (TrackBatch::iterator iter = mTrackBatch.begin();
       iter != end;
       ++iter) {
    if (!*iter) {
      continue;
    }
    nsCOMPtr<nsIURI> uri;
    iTunesTrack * const track = *iter;
    nsString guid;
    rv = miTunesDBServices.GetSBIDFromITID(miTunesLibID, track->mTrackID, guid);
    if (NS_SUCCEEDED(rv) && !guid.IsEmpty()) {
      nsString name;
      track->mProperties.Get(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), &name);
      
      mTrackIDMap.insert(TrackIDMap::value_type(track->mTrackID, guid));
      track->mSBGuid = guid;
      nsCOMPtr<sbIMediaItem> mediaItem;
      rv = mLibrary->GetMediaItem(guid, getter_AddRefs(mediaItem));
      if (NS_SUCCEEDED(rv)) {
        mFoundChanges = PR_TRUE;
        *iter = nsnull;
      
        nsCOMPtr<sbIPropertyArray> properties;
        rv = mediaItem->GetProperties(nsnull, getter_AddRefs(properties));
        if (NS_SUCCEEDED(rv)) {
          sbiTunesImporterEnumeratePropertiesData data(properties, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
          // Get the content URL and compare against the track URL. If
          // we fail to get it continue on, and let the downstream code
          // deal with it.
          nsString contentURL;
          NS_NAMED_LITERAL_STRING(CONTENT_URL, SB_PROPERTY_CONTENTURL);
          rv = properties->GetPropertyValue(CONTENT_URL,
                                            contentURL);
          if (NS_SUCCEEDED(rv)) {
            // Get the track URI, compare to the nightingale URI, if it's changed
            // we need to add it into the changed property array
            track->GetTrackURI(GetOSType(), 
                               mIOService,
                               miTunesLibSig, 
                               getter_AddRefs(uri));
            nsCOMPtr<nsIURI> fixedUri;
            rv = sbLibraryUtils::GetContentURI(uri, getter_AddRefs(fixedUri));
            NS_ENSURE_SUCCESS(rv, rv);
            nsCString trackCURI;
            rv = fixedUri->GetSpec(trackCURI);
            if (NS_SUCCEEDED(rv)) {      
              nsString const & trackURI = NS_ConvertUTF8toUTF16(trackCURI);
              if (!trackURI.Equals(contentURL)) {
                data.mChangedProperties->AppendProperty(CONTENT_URL,
                                                        trackURI);
              }
            }
          }
          // Enumerate the track properties and compare them to the Nightingale
          // ones and build a property array of the ones that are different
          track->mProperties.EnumerateRead(EnumReadFunc, &data);
          if (data.NeedsUpdating()) {
            rv = mediaItem->SetProperties(data.mChangedProperties);
            NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
                             "Failed to set a property on iTunes import");
          }
        }
      }
    }
  }
  return NS_OK;
}

nsresult 
sbiTunesImporter::ProcessNewItems(
  TracksByID & aTrackMap,
  nsIArray ** aNewItems) {
  NS_ENSURE_ARG_POINTER(aNewItems);
  
  nsresult rv;
  
  nsCOMPtr<nsIMutableArray> uriArray = 
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> propertyArrays = 
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIURI> uri;

  TrackBatch::iterator const begin = mTrackBatch.begin();
  TrackBatch::iterator const end = mTrackBatch.end();
  for (TrackBatch::iterator iter = begin; iter != end; ++iter) {
    // Skip if null (processed above)
    if (*iter) {
      nsString name;
      (*iter)->mProperties.Get(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), 
                               &name);

      nsString persistentID;
      PRBool ok = 
        (*iter)->mProperties.Get(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID), 
                                 &persistentID);
      NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
      
      aTrackMap.insert(TracksByID::value_type(persistentID, iter - begin));

      nsCOMPtr<nsIFile> file;
      // Get the location and create a URI object and add it to the array
      rv = (*iter)->GetTrackURI(GetOSType(), 
                                mIOService,
                                miTunesLibSig, 
                                getter_AddRefs(uri));
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIFileURL> trackFile = do_QueryInterface(uri, &rv);
        PRBool trackExists = PR_FALSE;
        if (NS_SUCCEEDED(rv)) {
          rv = trackFile->GetFile(getter_AddRefs(file));
          if (NS_SUCCEEDED(rv)) {
            file->Exists(&trackExists);
          }
          else {
            nsCString spec;
            uri->GetSpec(spec);
            LOG(("processTrack: File protocol error %d\n", rv));
            LOG(("%s\n", spec.BeginReading()));
          }
          if (!trackExists) {
            ++mMissingMediaCount;
          }
        }
        // Check if the track media is supported and add result to the iTunes
        // library signature.  This ensures the signature changes if support 
        // for the track media is added (e.g., by installing an extension).
        PRBool supported = PR_FALSE;
        // Ignore errors, default to not supported
        mTypeSniffer->IsValidMediaURL(uri, &supported);
        
        if (!supported) {
          ++mUnsupportedMediaCount;
        }
        nsString sig(NS_LITERAL_STRING("supported"));
        if (supported) {
          sig.AppendLiteral("true");
        }
        else {
          sig.AppendLiteral("false");
        }
        rv = miTunesLibSig.Update(sig);
        if (supported) {
          mFoundChanges = PR_TRUE;
          if (file) {
            // Add the track content length property.
            PRInt64 fileSize = 0;
            file->GetFileSize(&fileSize);
            (*iter)->mProperties.Put(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                                     sbAutoString(static_cast<PRUint64>(fileSize)));  
            NS_ENSURE_SUCCESS(rv, rv);
          }
          ++mTrackCount;
          rv = uriArray->AppendElement(uri, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
  
          // Add the track's property array to the array
          nsCOMPtr<sbIPropertyArray> propertyArray;
          rv = (*iter)->GetPropertyArray(getter_AddRefs(propertyArray));
          NS_ENSURE_SUCCESS(rv, rv);
          
          rv = propertyArrays->AppendElement(propertyArray, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }
  PRUint32 length;
  rv = propertyArrays->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (length > 0) {
    // Have to do synchronous to prevent playlist creation happening while
    // we're still creating its items.
    rv = mLibrary->BatchCreateMediaItems(uriArray, 
                                         propertyArrays, 
                                         PR_FALSE,
                                         aNewItems);
  }
  else {
    *aNewItems = nsnull;
  }
  return NS_OK;
}

nsresult 
sbiTunesImporter::ProcessCreatedItems(
  nsIArray * aCreatedItems,
  TracksByID const & aTrackMap) {
  NS_ENSURE_ARG_POINTER(aCreatedItems);
  
  PRUint32 length;
  nsresult rv = aCreatedItems->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  
  TracksByID::const_iterator trackMapEnd = aTrackMap.end();
  nsCOMPtr<sbIMediaItem> mediaItem;
  nsCOMPtr<sbIAlbumArtListener> albumArtListener = 
    sbiTunesImporterAlbumArtListener::New();
  for (PRUint32 index = 0; index < length; ++index) {
    
    mediaItem = do_QueryElementAt(aCreatedItems, 
                                  index, 
                                  &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString guid;
    rv = mediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString iTunesGUID;
    rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                                iTunesGUID);
    TracksByID::const_iterator iter = aTrackMap.find(iTunesGUID);
    if (iter != trackMapEnd) {
      mTrackBatch[iter->second]->mSBGuid = guid;
      mTrackIDMap.insert(TrackIDMap::value_type(
                           mTrackBatch[iter->second]->mTrackID,
                           guid));
    }
    mAlbumArtFetcher->SetFetcherType(sbIAlbumArtFetcherSet::TYPE_LOCAL);
    mAlbumArtFetcher->FetchAlbumArtForTrack(mediaItem,
                                            albumArtListener);
  }
  // batchCreateMediaItemsAsync won't return media items for duplicate tracks.
  // In order to get the corresponding media item for each duplicate track,
  // createMediaItem must be called.  Thus, for each track that does not have
  // a corresponding media item, call createMediaItem.
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<sbIPropertyArray> propertyArray;
  TrackBatch::iterator const end = mTrackBatch.end();
  for (TrackBatch::iterator iter = mTrackBatch.begin();
       iter != end;
       ++iter) {
    if (!*iter) {
      continue;
    }
    iTunesTrack * const track = *iter;
    // This can be null if it's an update
    if (track->mSBGuid.IsEmpty()) {
      rv = track->GetTrackURI(GetOSType(), 
                              mIOService,
                              miTunesLibSig, 
                              getter_AddRefs(uri));
      // If we can't get the URI then just skip it
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = track->GetPropertyArray(getter_AddRefs(propertyArray));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mLibrary->CreateMediaItem(uri, 
                                     propertyArray, 
                                     PR_FALSE, 
                                     getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mediaItem->GetGuid(track->mSBGuid);
      NS_ENSURE_SUCCESS(rv, rv);      
      mTrackIDMap.insert(TrackIDMap::value_type(track->mTrackID, 
                                                track->mSBGuid));
    }
    if (!track->mSBGuid.IsEmpty()) {
      rv = miTunesDBServices.MapID(miTunesLibID,
                                   track->mTrackID,
                                   track->mSBGuid);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

inline
void sbiTunesImporter::DestructiTunesTrack(iTunesTrack * aTrack) {
  delete aTrack;
}

nsresult sbiTunesImporter::ProcessTrackBatch() {
  nsresult rv;

  rv = ProcessUpdates();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> newItems;
  TracksByID trackMap;
  rv = ProcessNewItems(trackMap,
                       getter_AddRefs(newItems));
  NS_ENSURE_SUCCESS(rv, rv);

  // Only process created items if any new items where added.
  if (newItems) {
    rv = ProcessCreatedItems(newItems, trackMap);
    NS_ENSURE_SUCCESS(rv, rv);
  
    std::for_each(mTrackBatch.begin(), mTrackBatch.end(), DestructiTunesTrack);
  }
  
  mTrackBatch.clear();
  return NS_OK;
}

sbiTunesImporter::iTunesTrack::iTunesTrack() {
  MOZ_COUNT_CTOR(iTunesTrack);
}
sbiTunesImporter::iTunesTrack::~iTunesTrack() {
  MOZ_COUNT_DTOR(iTunesTrack);
}

nsresult 
sbiTunesImporter::iTunesTrack::Initialize(sbIStringMap * aProperties) {
  NS_ENSURE_ARG_POINTER(aProperties);
  
  nsresult rv = aProperties->Get(NS_LITERAL_STRING("Track ID"), mTrackID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool ok = mProperties.Init(32);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
    
  NS_NAMED_LITERAL_STRING(location, "Location");
  nsString URI;
  
  rv = aProperties->Get(location, URI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mProperties.Put(location, URI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  for (int index = 0; index < NS_ARRAY_LENGTH(gPropertyMap); ++index) {
    PropertyMap const & propertyMapEntry = gPropertyMap[index];
    nsString value;
    rv = aProperties->Get(NS_ConvertASCIItoUTF16(propertyMapEntry.ITProperty),
                          value);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "sbIStringMap::Get failed");
    if (!value.IsVoid()) {
      if (propertyMapEntry.mConversion) {
        value = propertyMapEntry.mConversion(value);
      }
      mProperties.Put(NS_ConvertASCIItoUTF16(propertyMapEntry.SBProperty),
                      value);
    }
  }
  return NS_OK;
}

static PLDHashOperator
ConvertToPropertyArray(nsAString const & aKey,
                       nsString aValue,
                       void * aUserArg) {
  NS_ENSURE_TRUE(aUserArg, PL_DHASH_STOP);
  
  sbIMutablePropertyArray * array = 
    reinterpret_cast<sbIMutablePropertyArray*>(aUserArg);
  nsresult rv = array->AppendProperty(aKey, aValue);
  NS_ENSURE_SUCCESS(rv, ::PL_DHASH_STOP);
  return PL_DHASH_NEXT;
}

nsresult 
sbiTunesImporter::iTunesTrack::GetPropertyArray(
    sbIPropertyArray ** aPropertyArray) {
  NS_ENSURE_ARG_POINTER(aPropertyArray);
  
  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> array = 
    do_CreateInstance("@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mProperties.EnumerateRead(ConvertToPropertyArray, array.get());
  nsCOMPtr<sbIPropertyArray> propertyArray = do_QueryInterface(array);
  propertyArray.forget(aPropertyArray);
  return NS_OK;
}

/** nsIRunnable implementation **/
nsresult 
sbiTunesImporter::iTunesTrack::GetTrackURI(
  sbiTunesImporter::OSType aOSType,
  nsIIOService * aIOService,
  sbiTunesSignature & aSignature,
  nsIURI ** aTrackURI) {
  NS_ENSURE_ARG_POINTER(aIOService);
  NS_ENSURE_ARG_POINTER(aTrackURI);

  if (mURI) {
    *aTrackURI = mURI.get();
    NS_ADDREF(*aTrackURI);
    return NS_OK;
  }
  nsString uri16;
  if (!mProperties.Get(NS_LITERAL_STRING("Location"), &uri16)) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  if (uri16.IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  // Convert to UTF8
  nsCString uri = NS_ConvertUTF16toUTF8(uri16);
  
  nsCString adjustedURI;
  
  if (uri[uri.Length() - 1] == '/') {
    uri.Cut(uri.Length() -1, 1);
  }
  
  if (uri.Find("file://localhost//", CaseInsensitiveCompare) == 0) {
    adjustedURI.AssignLiteral("file://///");
    uri.Cut(0, 18);
  }
  else if (uri.Find("file://localhost/", CaseInsensitiveCompare) == 0) {
    adjustedURI.AssignLiteral("file:///");
    uri.Cut(0,17);
  }
  else {
    char const c = uri[0];
    if (uri.Length() > 3 && 
        (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') &&
        uri[1] == ':' &&
        uri[2] == '/') {
      adjustedURI = "file:///";
      uri.Cut(0, 3);
    }
    else {
      adjustedURI = "file:////";
    }
  }

  adjustedURI.Append(uri);

  // Convert to lower case on Windows since it's case-insensitive.
  if (aOSType == WINDOWS_OS) {
    ToLowerCase(adjustedURI);
  }

  nsString locationSig;
  locationSig.AssignLiteral("Location");
  locationSig.AppendLiteral(adjustedURI.BeginReading());
  // Add file location to iTunes library signature.
  nsresult rv = aSignature.Update(locationSig);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Get the track URI.
  rv = aIOService->NewURI(adjustedURI, nsnull, nsnull, getter_AddRefs(mURI));
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aTrackURI = mURI.get();
  NS_ADDREF(*aTrackURI);
  
  return NS_OK;
}

/* void onDataAvailable (in nsIRequest aRequest, in nsISupports aContext, in nsIInputStream aInputStream, in unsigned long aOffset, in unsigned long aCount); */
NS_IMETHODIMP 
sbiTunesImporter::OnProgress(PRInt64 aBytesResad)
{
  mBytesRead = aBytesResad;
  return NS_OK;
}
