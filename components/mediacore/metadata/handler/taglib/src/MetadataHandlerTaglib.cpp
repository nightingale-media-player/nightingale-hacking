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
 * Taglib metadata handler.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  MetadataHandlerTaglib.cpp
* \brief Songbird MetadataHandlerTaglib implementation.
*/

/*******************************************************************************
 *
 * Taglib metadata handler imported services.
 *
 ******************************************************************************/

/* Local file imports. */
#include "MetadataHandlerTaglib.h"

/* Local module imports. */
#include "TaglibChannelFileIO.h"

/* Songbird utility classes */
#include "sbStringUtils.h"
#include <sbProxiedComponentManager.h>

/* Songbird interfaces */
#include "sbStandardProperties.h"
#include "sbIPropertyArray.h"
#include "sbPropertiesCID.h"

/* Mozilla imports. */
#include <necko/nsIURI.h>
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIStandardURL.h>
#include <nsICharsetDetector.h>
#include <nsIUTF8ConverterService.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <prprf.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsMemory.h>

#include <flacfile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <mp4file.h>
#include <vorbisfile.h>

/* C++ std imports. */
#include <sstream>

/* Windows Specific */
#if defined(XP_WIN)
  #include <windows.h>
#endif

#define WRITE_PROPERTY(result, SB_PROPERTY, method)         \
  PR_BEGIN_MACRO                                            \
  result = mpMetadataPropertyArray->GetPropertyValue(       \
    NS_LITERAL_STRING(SB_PROPERTY), propertyValue);         \
  if (NS_SUCCEEDED(result)) {                               \
    f.tag()->set##method(TagLib::String(                    \
      NS_ConvertUTF16toUTF8(propertyValue).BeginReading(),  \
      TagLib::String::UTF8));                               \
  }                                                         \
  PR_END_MACRO

#define WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY, method) \
  PR_BEGIN_MACRO                                            \
  result = mpMetadataPropertyArray->GetPropertyValue(       \
    NS_LITERAL_STRING(SB_PROPERTY), propertyValue);         \
  if (NS_SUCCEEDED(result)) {                               \
    int method;                                             \
    int numRead = PR_sscanf(                                \
      NS_ConvertUTF16toUTF8(propertyValue).BeginReading(),  \
      "%d",                                                 \
      &method);                                             \
    if (numRead == 1) {                                     \
      f.tag()->set##method(method);                         \
    }                                                       \
    else {                                                  \
      f.tag()->set##method(0);                              \
    }                                                       \
  }                                                         \
  PR_END_MACRO

/*******************************************************************************
 *
 * Taglib metadata handler logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbMetadataHandlerTaglib");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


// the minimum number of chracters to feed into the charset detector
#define GUESS_CHARSET_MIN_CHAR_COUNT 256

PRLock* sbMetadataHandlerTaglib::sBusyLock = nsnull;
PRLock* sbMetadataHandlerTaglib::sBackgroundLock = nsnull;
PRBool sbMetadataHandlerTaglib::sBusyFlag = PR_FALSE;

/*
 *
 * Taglib metadata handler nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS3(sbMetadataHandlerTaglib, sbIMetadataHandler,
                                            sbISeekableChannelListener,
                                            nsICharsetDetectionObserver)


/*******************************************************************************
 *
 * Taglib metadata handler sbIMetadataHandler implementation.
 *
 ******************************************************************************/

/**
* \brief Vote to be the handler returned for the given url
*
* The sbIMetadataManager will instantiate one of every sbIMetadataHandler
* subclass and ask it to vote on the given url.  Whichever handler returns
* the highest vote will be used as the handler for the url.
*
* Values less than zero cause that handler to be ignored.
*
* At the moment, our handlers return -1, 0, or 1 (for "no," "maybe," and
* "yes").
*
* \param url The url upon which one should vote
* \return The vote
* \sa sbIMetadataManager
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Vote(
    const nsAString             &url,
    PRInt32                     *pVote)
{
    nsString                    _url(url);
    PRInt32                     vote = 0;

    /* Convert the URL to lower case. */
    ToLowerCase(_url);

    /* Check for supported files. */
    if (   (_url.Find(".flac", PR_TRUE) != -1)
        || (_url.Find(".mpc", PR_TRUE) != -1)
        || (_url.Find(".mp3", PR_TRUE) != -1)
        || (_url.Find(".m4a", PR_TRUE) != -1)
        || (_url.Find(".m4p", PR_TRUE) != -1)
        || (_url.Find(".oga", PR_TRUE) != -1)
        || (_url.Find(".wv", PR_TRUE) != -1)
        || (_url.Find(".spx", PR_TRUE) != -1)
        || (_url.Find(".tta", PR_TRUE) != -1)
        || (_url.Find(".oga", PR_TRUE) != -1)
        || (_url.Find(".ogg", PR_TRUE) != -1))
    {
        vote = 1;
    }

    /* Check for unsupported files. */
    else if (   (_url.Find(".avi", PR_TRUE) != -1)
             || (_url.Find(".wma", PR_TRUE) != -1)
             || (_url.Find(".wmv", PR_TRUE) != -1)
             || (_url.Find(".asf", PR_TRUE) != -1)
             || (_url.Find(".wav", PR_TRUE) != -1))
    {
        vote = -1;
    }
    
    /* we can only handle things with mozilla protocol handlers */
    if (vote >= 0) {
        nsresult rv;
        nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIProtocolHandler> protocolHandler;
        nsCString scheme;
        
        rv = ios->ExtractScheme(NS_ConvertUTF16toUTF8(_url), scheme);
        if (NS_SUCCEEDED(rv)) {
            rv = ios->GetProtocolHandler(scheme.BeginReading(),
                                         getter_AddRefs(protocolHandler));
        }
        if (NS_FAILED(rv)) {
            // can't deal with the url
            LOG(("sbMetadataHandlerTaglib::Vote - can't handle scheme %s\n",
                 scheme.BeginReading()));
            vote = -1;
        }
    }

    /* Return results. */
    *pVote = vote;
    LOG(("sbMetadataHandlerTaglib::Vote - voting to %d\n", vote));

    return (NS_OK);
}


/**
* \brief Start the read operation
*
* After getting a handler from the sbIMetadataManager, the user code usually
* calls read upon it, immediately.  The implementor may choose to handle the
* request immediately or asynchronously.
*
* \return -1 if operating asynchronously, otherwise the number of metadata
*            values read (0 on failure)
*/
NS_IMETHODIMP sbMetadataHandlerTaglib::Read(
    PRInt32                     *pReadCount)
{
  nsresult rv;
  AcquireTaglibLock();
  rv = ReadInternal(pReadCount); 
  ReleaseTaglibLock(); 
  // note that although ReleaseTaglibLock() has a return value, it always succeeds
  // so we're really we're most interested in the return value from ReadInternal();
  
  return rv;
}

nsresult sbMetadataHandlerTaglib::AcquireTaglibLock() 
{
  // TagLib is not thread safe so we must do this elaborate dance
  // to ensure that only one thread toches taglib at a time as well
  // as make sure the main thread can process events while waiting
  // its turn.  This is done by busy-waiting around sBusyFlag.  If
  // you are the main thread, process events while busy waiting.

  nsCOMPtr<nsIThread> mainThread;
  if (NS_IsMainThread()) {
    mainThread = do_GetMainThread();
  }
  else {
    // To prevent an army of busy waiting background threads,
    // use a lock here so only one background thread can enter
    // this section at a time.
    PR_Lock(sBackgroundLock);
  }
  
  PRBool gotLock = PR_FALSE;
  while (!gotLock) {
    // Scope the lock
    {
      nsAutoLock lock(sBusyLock);
      if (!sBusyFlag) {
        sBusyFlag = PR_TRUE;
        gotLock = PR_TRUE;
      }
    }
    
    if (mainThread && !gotLock) {
      NS_ProcessPendingEvents(mainThread, 100);
    }
    
  }
  return NS_OK;
}

nsresult sbMetadataHandlerTaglib::ReleaseTaglibLock() {
  // let another background thread into the busywait.
  if (!NS_IsMainThread()) {
    PR_Unlock(sBackgroundLock);
  }

  // Scope the lock
  {
    nsAutoLock lock(sBusyLock);
    sBusyFlag = PR_FALSE;
  }

  return NS_OK;
}

nsresult sbMetadataHandlerTaglib::ReadInternal(
    PRInt32                     *pReadCount)
{
    nsCOMPtr<nsIStandardURL>    pStandardURL;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIFile>           pFile;
    nsCString                   urlSpec;
    nsCString                   urlScheme;
    nsAutoString                filePath;
    PRUint32                    unsignedReadCount = 0;
    PRInt32                     readCount = 0;
    nsresult                    result = NS_OK;

    /* Get the TagLib sbISeekableChannel file IO manager. */
    mpTagLibChannelFileIOManager =
            do_GetService
                ("@songbirdnest.com/Songbird/sbTagLibChannelFileIOManager;1",
                 &result);

    /* Initialize the metadata values. */
    if (NS_SUCCEEDED(result))
    {
        mpMetadataPropertyArray = 
          do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &result);

        result = mpMetadataPropertyArray->SetStrict(PR_FALSE);
        NS_ENSURE_SUCCESS(result, result);
    }

    /* Get the channel URL info. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->GetURI(getter_AddRefs(pURI));
    if (NS_SUCCEEDED(result))
    {
        pStandardURL = do_CreateInstance("@mozilla.org/network/standard-url;1",
                                         &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = pStandardURL->Init(pStandardURL->URLTYPE_STANDARD,
                                    0,
                                    NS_LITERAL_CSTRING(""),
                                    nsnull,
                                    pURI);
    }
    if (NS_SUCCEEDED(result))
        mpURL = do_QueryInterface(pStandardURL, &result);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetSpec(urlSpec);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetScheme(urlScheme);
    
    LOG(("sbMetadataHandlerTaglib::ReadInternal - spec is %s\n",
         urlSpec.BeginReading()));

#ifdef MOZ_CRASHREPORTER
    /* Add crash reporter annotation. */
    if (NS_SUCCEEDED(result))
    {
        mpCrashReporter = do_GetService("@mozilla.org/xre/app-info;1", &result);
        if (NS_FAILED(result))
        {
            mpCrashReporter = nsnull;
            result = NS_OK;
        }
        if (mpCrashReporter)
        {
            mpCrashReporter->AnnotateCrashReport
                                        (NS_LITERAL_CSTRING("TaglibReadSpec"),
                                         urlSpec);
        }
    }
#endif

    /* If the channel URL scheme is for a local file, try reading  */
    /* synchronously.  If successful, metadata read operation will */
    /* be completed.                                               */
    if (urlScheme.Equals(NS_LITERAL_CSTRING("file")))
    {
        /* Get the metadata local file path. */
        if (NS_SUCCEEDED(result))
        {
            result = mpFileProtocolHandler->GetFileFromURLSpec
                                                        (urlSpec,
                                                         getter_AddRefs(pFile));
        }

        if (NS_SUCCEEDED(result))
          result = pFile->GetPath(mMetadataPath);

        /* Read the metadata. */
        if (NS_SUCCEEDED(result))
            result = ReadMetadata();
    }

    /* If metadata reading is not complete, start an */
    /* asynchronous read with the metadata channel.  */
    if (NS_SUCCEEDED(result) && !mCompleted)
    {
        /* Create a metadata channel. */
        mpSeekableChannel =
            do_CreateInstance("@songbirdnest.com/Songbird/SeekableChannel;1",
                              &result);

        /* Add the metadata channel for use with the */
        /* TagLib nsIChannel  file I/O services.     */
        if (NS_SUCCEEDED(result))
        {
            /* Allocate a metadata channel ID    */
            /* and use it for the metadata path. */
            PR_AtomicIncrement((PRInt32 *) &sNextChannelID);
            mMetadataPath.AssignLiteral("metadata_channel://");
            mMetadataPath.AppendInt(sNextChannelID);
            mMetadataChannelID = mMetadataPath;

            /* Add the metadata channel. */
            result = mpTagLibChannelFileIOManager->AddChannel
                                                            (mMetadataChannelID,
                                                             mpSeekableChannel);
        }

        /* Open the metadata channel to start reading. */
        if (NS_SUCCEEDED(result))
            mpSeekableChannel->Open(mpChannel, this);

        /* Indicate that reading is being done asynchronously. */
        if (NS_SUCCEEDED(result))
            readCount = -1;
    }

    /* If the read operation is complete, get the number of read tags. */
    if (NS_SUCCEEDED(result) && mCompleted) {
        result = mpMetadataPropertyArray->GetLength(&unsignedReadCount);
        readCount = (PRInt32)unsignedReadCount; // (sigh)
    }

    /* Complete read operation on error. */
    if (!NS_SUCCEEDED(result))
    {
        CompleteRead();
        readCount = 0;
    }

    /* Return results. */
    *pReadCount = readCount;

    return result;
}


/**
* \brief Start the write operation
*
* After getting a handler from the sbIMetadataManager, the user code may set
* an sbIMetadataValues object into the handler and then call write to write
* the abstract metadata map into the specific metadata requirements of the
* file format supported by the handler.
*
* Note that the number of items written may not always equal the number of
* items in the sbIMetadataValues object if the underlying file format does
* not support the given keys.
*
* \return -1 if operating asynchronously, otherwise the number of metadata
*            values written (0 on failure)
*/
NS_IMETHODIMP sbMetadataHandlerTaglib::Write(
    PRInt32                     *pWriteCount)
{
  nsresult rv;
  AcquireTaglibLock();
  rv = WriteInternal(pWriteCount);
  ReleaseTaglibLock();

  return rv;
}

nsresult sbMetadataHandlerTaglib::WriteInternal(
    PRInt32                     *pWriteCount)
{
    nsCOMPtr<nsIStandardURL>    pStandardURL;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIFile>           pFile;
    nsCString                   urlSpec;
    nsCString                   urlScheme;
    nsAutoString                filePath;
    nsresult                    result = NS_OK;
  
    // TODO must ensure metadata is set before writing
    NS_ENSURE_TRUE(mpMetadataPropertyArray, NS_ERROR_NOT_INITIALIZED);

    /* Get the TagLib sbISeekableChannel file IO manager. */
    mpTagLibChannelFileIOManager =
            do_GetService
                ("@songbirdnest.com/Songbird/sbTagLibChannelFileIOManager;1",
                 &result);

    /* Get the channel URL info. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->GetURI(getter_AddRefs(pURI));
    if (NS_SUCCEEDED(result))
    {
        pStandardURL = do_CreateInstance("@mozilla.org/network/standard-url;1",
                                         &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = pStandardURL->Init(pStandardURL->URLTYPE_STANDARD,
                                    0,
                                    NS_LITERAL_CSTRING(""),
                                    nsnull,
                                    pURI);
    }
    if (NS_SUCCEEDED(result))
        mpURL = do_QueryInterface(pStandardURL, &result);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetSpec(urlSpec);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetScheme(urlScheme);

    if (!urlScheme.EqualsLiteral("file"))
    {
      return NS_ERROR_NOT_IMPLEMENTED;
    }
     
    /* Get the metadata local file path. */
    if (NS_SUCCEEDED(result))
    {
      result = mpFileProtocolHandler->GetFileFromURLSpec(
        urlSpec, 
        getter_AddRefs(pFile)
      );
    }

    if (NS_SUCCEEDED(result))
      result = pFile->GetPath(mMetadataPath);

    /* WRITE the metadata. */
    if (NS_SUCCEEDED(result)) {
      // on windows, we leave this as an nsAString
      // beginreading doesn't promise to return a null terminated string
      // this is RFB
#if XP_WIN
      nsAString &filePath = mMetadataPath;
#else
      nsCAutoString filePath = NS_ConvertUTF16toUTF8(mMetadataPath);
#endif

      TagLib::FileRef f(filePath.BeginReading());
      NS_ENSURE_TRUE(f.file(), NS_ERROR_FAILURE);
      NS_ENSURE_TRUE(f.file()->isOpen(), NS_ERROR_FAILURE);
      
      nsAutoString propertyValue;
      
      // WRITE_PROPERTY is a natty macro
      WRITE_PROPERTY(result, SB_PROPERTY_TRACKNAME, Title);
      WRITE_PROPERTY(result, SB_PROPERTY_ARTISTNAME, Artist);
      WRITE_PROPERTY(result, SB_PROPERTY_ALBUMARTISTNAME, AlbumArtist);
      WRITE_PROPERTY(result, SB_PROPERTY_ALBUMNAME, Album);
      WRITE_PROPERTY(result, SB_PROPERTY_COMMENT, Comment);
      WRITE_PROPERTY(result, SB_PROPERTY_LYRICS, Lyrics);
      WRITE_PROPERTY(result, SB_PROPERTY_GENRE, Genre);
      WRITE_PROPERTY(result, SB_PROPERTY_PRODUCERNAME, Producer);
      WRITE_PROPERTY(result, SB_PROPERTY_COMPOSERNAME, Composer);
      WRITE_PROPERTY(result, SB_PROPERTY_CONDUCTORNAME, Conductor);
      WRITE_PROPERTY(result, SB_PROPERTY_LYRICISTNAME, Lyricist);
      WRITE_PROPERTY(result, SB_PROPERTY_RECORDLABELNAME, RecordLabel);
      WRITE_PROPERTY(result, SB_PROPERTY_RATING, Rating);
      WRITE_PROPERTY(result, SB_PROPERTY_LANGUAGE, Language);
      WRITE_PROPERTY(result, SB_PROPERTY_KEY, Key);
      WRITE_PROPERTY(result, SB_PROPERTY_COPYRIGHT, License);
      WRITE_PROPERTY(result, SB_PROPERTY_COPYRIGHTURL, LicenseUrl);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_YEAR, Year);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_TRACKNUMBER, Track);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_TOTALTRACKS, TotalTracks);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_DISCNUMBER, Disc);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_TOTALDISCS, TotalDiscs);
      WRITE_NUMERIC_PROPERTY(result, SB_PROPERTY_BPM, Bpm);
      // todo: isCompilation! + Tests!

      // Attempt to save the metadata
      if (f.save()) {
        result = NS_OK;
      } else {
        result = NS_ERROR_FAILURE;      
      } 
    }
    
    // TODO need to set pWriteCount
  
    return result;
}


/**
* \brief Be thou informst that one's sbIMetadataChannel has just received data
*
* Every time the underlying nsIChannel dumps data on the sbIMetadataChannel,
* plus once more for when the nsIChannel reports a stop condition.
*
* This is a chance for the handler code to attempt to parse the datastream.
*
* \param pChannel The sbIMetadataChannel delivering data.  You'll have to QI for
*                 it.
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::OnChannelData(
    nsISupports                 *pChannel)
{
    return (NS_OK);
}


/**
* \brief Close down the internals of the handler, stop any downloads, free any
*        allocations
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Close()
{
    /* If a metadata channel is being used, remove it from */
    /* use with the TagLib nsIChannel file I/O services.   */
    if (!mMetadataChannelID.IsEmpty())
    {
        mpTagLibChannelFileIOManager->RemoveChannel(mMetadataChannelID);
        mMetadataChannelID.Truncate();
    }

    /* Close the metadata channel. */
    if (mpSeekableChannel)
    {
        mpSeekableChannel->Close();
        mpSeekableChannel = nsnull;
    }

    /* Complete read operation. */
    CompleteRead();

    return (NS_OK);
}


/*
 * Getters/setters
 */

NS_IMETHODIMP sbMetadataHandlerTaglib::GetProps(
    sbIMutablePropertyArray     **ppPropertyArray)
{
    NS_ENSURE_ARG_POINTER(ppPropertyArray);
    NS_ENSURE_STATE(ppPropertyArray);
    NS_ADDREF(*ppPropertyArray = mpMetadataPropertyArray);
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::SetProps(
    sbIMutablePropertyArray     *ppPropertyArray)
{
    mpMetadataPropertyArray = ppPropertyArray;
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::GetCompleted(
    PRBool                      *pCompleted)
{
    NS_ENSURE_ARG_POINTER(pCompleted);
    *pCompleted = mCompleted;
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::GetChannel(
    nsIChannel                  **ppChannel)
{
    NS_ENSURE_ARG_POINTER(ppChannel);
    NS_IF_ADDREF(*ppChannel = mpChannel);
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::SetChannel(
    nsIChannel                  *pChannel)
{
    mpChannel = pChannel;
    return (NS_OK);
}


/*******************************************************************************
 *
 * Taglib metadata handler sbISeekableChannel implementation.
 *
 ******************************************************************************/

/**
* \brief Be thou informst that one's sbISeekableChannel has just received data
*
* Every time the underlying nsIChannel dumps data on the sbISeekableChannel,
* plus once more for when the nsIChannel reports a stop condition.
*
* This is a chance for the handler code to attempt to parse the datastream.
*
* \param pChannel The sbISeekableChannel delivering data.  You'll have to QI for
*                 it.
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::OnChannelDataAvailable(
    sbISeekableChannel          *pChannel)
{
    PRBool                      channelCompleted;
    nsresult                    result = NS_OK;

    /* Do nothing if the metadata reading is complete. */
    if (mCompleted)
        return (result);

    /* Process channel data and complete processing on any exception. */
    try
    {
        /* Clear channel restart condition. */
        mMetadataChannelRestart = PR_FALSE;

        /* Read the metadata. */
        ReadMetadata();

        /* Check for metadata channel completion. */
        if (!mCompleted)
        {
            result = mpSeekableChannel->GetCompleted(&channelCompleted);
            if (NS_SUCCEEDED(result) && channelCompleted)
                CompleteRead();
        }
    }
    catch(...)
    {
        printf("1: OnChannelDataAvailable exception\n");
        CompleteRead();
    }

    return (result);
}


/*******************************************************************************
 *
 * Public taglib metadata handler services.
 *
 ******************************************************************************/

/*
 * sbMetadataHandlerTaglib
 *
 *   This method is the constructor for the taglib metadata handler class.
 */

sbMetadataHandlerTaglib::sbMetadataHandlerTaglib()
:
    mMetadataChannelRestart(PR_FALSE),
    mCompleted(PR_FALSE)
{
}


/*
 * ~sbMetadataHandlerTaglib
 *
 *   This method is the destructor for the taglib metadata handler class.
 */

sbMetadataHandlerTaglib::~sbMetadataHandlerTaglib()
{
    /* Ensure the taglib metadata handler is closed. */
    Close();
}


/*
 * FactoryInit
 *
 *   This function is called by the component factory to initialize the
 * component.
 */

nsresult sbMetadataHandlerTaglib::Init()
{
    nsresult                    rv;

    /* Create a file protocol handler instance. */
    mpFileProtocolHandler =
            do_CreateInstance("@mozilla.org/network/protocol;1?name=file", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mProxiedServices =
      do_GetService("@songbirdnest.com/moz/proxied-services;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}

/* static */
nsresult sbMetadataHandlerTaglib::ModuleConstructor(nsIModule* aSelf)
{
  sbMetadataHandlerTaglib::sBusyLock =
    nsAutoLock::NewLock("sbMetadataHandlerTaglib::sBusyLock");
  NS_ENSURE_TRUE(sbMetadataHandlerTaglib::sBusyLock, NS_ERROR_OUT_OF_MEMORY);

  sbMetadataHandlerTaglib::sBackgroundLock =
    nsAutoLock::NewLock("sbMetadataHandlerTaglib::sBackgroundLock");
  NS_ENSURE_TRUE(sbMetadataHandlerTaglib::sBackgroundLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

/* static */
void sbMetadataHandlerTaglib::ModuleDestructor(nsIModule* aSelf)
{
  if (sbMetadataHandlerTaglib::sBusyLock) {
    nsAutoLock::DestroyLock(sbMetadataHandlerTaglib::sBusyLock);
  }

  if (sbMetadataHandlerTaglib::sBackgroundLock) {
    nsAutoLock::DestroyLock(sbMetadataHandlerTaglib::sBackgroundLock);
  }
}

/*******************************************************************************
 *
 * Private taglib metadata handler class services.
 *
 ******************************************************************************/

/*
 * Static field initializers.
 */

PRUint32 sbMetadataHandlerTaglib::sNextChannelID = 0;


/*******************************************************************************
 *
 * Private taglib metadata handler ID3v2 services.
 *
 ******************************************************************************/

/*
 * ID3v2Map                     Map of ID3v2 tag names to Songbird metadata
 *                              names.
 */

static char *ID3v2Map[][2] =
{
    { "TENC", SB_PROPERTY_SOFTWAREVENDOR },
    { "TIT3", SB_PROPERTY_SUBTITLE },
//    { "TMED", "mediatype" },
//    { "TOAL", "original_album" },
//    { "TOPE", "original_artist" },
//    { "TORY", "original_year" },
//    { "TPE2", "accompaniment" },
//    { "TPE4", "interpreter_remixer" },
    { "UFID", SB_PROPERTY_METADATAUUID },
//    { "USER", "terms_of_use" },
//    { "WCOM", "commercialinfo_url" },
//    { "WOAR", "artist_url" },
//    { "WOAS", "source_url" },
//    { "WORS", "netradio_url" },
//    { "WPAY", "payment_url" },
//    { "WPUB", "publisher_url" },
//    { "WXXX", "user_url" }
      { "XXXX", "junq" }
};


/*
 * ReadID3v2Tags
 *
 *   --> pTag                   ID3v2 tag object.
 *   --> aCharset               The character set the tags are assumed to be in
 *                              Pass in null to do no conversion
 *
 *   This function reads ID3v2 tags from the ID3v2 tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadID3v2Tags(
  TagLib::ID3v2::Tag          *pTag,
  const char                  *aCharset)
{
  TagLib::ID3v2::FrameListMap frameListMap;
  int                         numMetadataEntries;
  int                         i;

  /* Do nothing if no tags are present. */
  if (!pTag) {
      return;
  }

  frameListMap = pTag->frameListMap();

  /* Add the metadata entries. */
  numMetadataEntries = sizeof(ID3v2Map) / sizeof(ID3v2Map[0]);
  for (i = 0; i < numMetadataEntries; i++) {
    TagLib::ID3v2::FrameList frameList = frameListMap[ID3v2Map[i][0]];
    if(!frameList.isEmpty()) {
      TagLib::String value = ConvertCharset(frameList.front()->toString(), aCharset);
      AddMetadataValue(ID3v2Map[i][1], value);
    }
  }
  
  // Specialcase for TLEN which we keep in microseconds and ID3v2 stores in ms
  TagLib::ID3v2::FrameList frameList = frameListMap["TLEN"];
  if (!frameList.isEmpty())
  {
    PRInt32 length;
    PR_sscanf(frameList.front()->toString().toCString(true), "%d", &length);
    AddMetadataValue(ID3v2Map[i][1], length * 1000);
  }
  return;
}


/*******************************************************************************
 *
 * Private taglib metadata handler APE services.
 *
 ******************************************************************************/

/*
 * APEMap                     Map of APE tag names to Songbird metadata
 *                            names.
 */

// see http://wiki.hydrogenaudio.org/index.php?title=APE_key
static char *APEMap[][2] =
{
    { "Subtitle", SB_PROPERTY_SUBTITLE },
//    { "Debut album", "original_album" },
//    { "File", "source_url" },
};


/*
 * ReadAPETags
 *
 *   --> pTag                   APE tag object.
 *
 *   This function reads APE tags from the APE tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadAPETags(
    TagLib::APE::Tag          *pTag)
{
    TagLib::APE::ItemListMap  itemListMap;
    int                       numMetadataEntries;
    int                       i;

    /* Do nothing if no tags are present. */
    if (!pTag) {
        return;
    }

    /* Get the item list map. */
    itemListMap = pTag->itemListMap();

    /* Add the metadata entries. */
    numMetadataEntries = sizeof(APEMap) / sizeof(APEMap[0]);
    for (i = 0; i < numMetadataEntries; i++) {
      TagLib::APE::Item item = itemListMap[APEMap[i][0]];
      if (!item.isEmpty()) {
        AddMetadataValue(APEMap[i][1], item.toString());
      }
    }
}


/*******************************************************************************
 *
 * Private taglib metadata handler Xiph services.
 *
 ******************************************************************************/

/*
 * ReadXiphTags
 *
 *   --> pTag                   Xiph tag object.
 *
 *   This function reads Xiph tags from the Xiph tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadXiphTags(
    TagLib::Ogg::XiphComment    *pTag)
{
  // nothing to do here. all the xiph properties we're interested in are already being read
}

/*******************************************************************************
 *
 * Private taglib metadata handler services.
 *
 ******************************************************************************/

/*
 * ReadMetadata
 *
 *   This function attempts to read the metadata.  If the metadata is available,
 * this function extracts the metadata values and sets the metadata reading
 * complete condition to true.
 */

nsresult sbMetadataHandlerTaglib::ReadMetadata()
{
    nsCString                   fileExt;
    PRBool                      isValid = PR_FALSE;
    PRBool                      decodedFileExt = PR_FALSE;
    nsresult                    result = NS_OK;

    /* Get the metadata file extension. */
    result = mpURL->GetFileExtension(fileExt);
    if (NS_SUCCEEDED(result))
        ToLowerCase(fileExt);

    /* Read the metadata using the file extension */
    /* to determine the metadata format.          */
    if (NS_SUCCEEDED(result))
    {
        decodedFileExt = PR_TRUE;
        if (fileExt.Equals(NS_LITERAL_CSTRING("flac")))
            isValid = ReadFLACFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("mpc")))
            isValid = ReadMPCFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("mp3")))
            isValid = ReadMPEGFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("m4a")))
            isValid = ReadMP4File(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("m4p")))
            isValid = ReadMP4File(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("ogg")))
            isValid = ReadOGGFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("oga")))
            isValid = ReadOGGFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("ogv")))
            isValid = ReadOGGFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("ogm")))
            isValid = ReadOGGFile(mMetadataPath);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("ogx")))
            isValid = ReadOGGFile(mMetadataPath);
        else
            decodedFileExt = PR_FALSE;
    }

    /* If the file extension was not decoded, try */
    /* reading the metadata in different formats. */
    if (   NS_SUCCEEDED(result)
        && !decodedFileExt
        && !isValid
        && !mMetadataChannelRestart)
    {
        isValid = ReadMPEGFile(mMetadataPath);
    }

    /* Check if the metadata reading is complete. */
    if (isValid && !mMetadataChannelRestart)
        CompleteRead();

    /* Complete metadata reading upon error. */
    if (!NS_SUCCEEDED(result))
        CompleteRead();

    return (result);
}


/*
 * CompleteRead
 *
 *   This function completes a metadata read operation.
 */

void sbMetadataHandlerTaglib::CompleteRead()
{
    /* Indicate that the metadata read operation has completed. */
    mCompleted = PR_TRUE;

#ifdef MOZ_CRASHREPORTER
    /* Remove crash reporter annotation. */
    if (mpCrashReporter)
    {
        mpCrashReporter->AnnotateCrashReport
                                        (NS_LITERAL_CSTRING("TaglibReadSpec"),
                                         NS_LITERAL_CSTRING(""));
    }
#endif
}


/*
 * ReadFile
 *
 *   --> pTagFile               File from which to read.
 *   --> aCharset               The character encoding to use for reading
 *                              May be empty string (to mean no conversion)
 *                              On Windows, may also be "CP_ACP"
 *
 *   <--                        True if file has valid metadata.
 *
 *   This function reads a base set of metadata from the file specified by
 * pTagFile.
 */

PRBool sbMetadataHandlerTaglib::ReadFile(
    TagLib::File                *pTagFile,
    const char                  *aCharset)
{
  TagLib::Tag                 *pTag;
  TagLib::AudioProperties     *pAudioProperties;

  /* We want to be sure we have a legit file ref. */
  if (!pTagFile || !pTagFile->isValid()) {
    return false; // not valid!
  }
  
  pTag = pTagFile->tag();
  if (pTag) {
    // yay random charset guessing!
    AddMetadataValue(SB_PROPERTY_TRACKNAME,       ConvertCharset(pTag->title(), aCharset));
    AddMetadataValue(SB_PROPERTY_ARTISTNAME,      ConvertCharset(pTag->artist(), aCharset));
    AddMetadataValue(SB_PROPERTY_ALBUMARTISTNAME, ConvertCharset(pTag->albumArtist(), aCharset));
    AddMetadataValue(SB_PROPERTY_ALBUMNAME,       ConvertCharset(pTag->album(), aCharset));
    AddMetadataValue(SB_PROPERTY_COMMENT,         ConvertCharset(pTag->comment(), aCharset));
    AddMetadataValue(SB_PROPERTY_LYRICS,          ConvertCharset(pTag->lyrics(), aCharset));
    AddMetadataValue(SB_PROPERTY_GENRE,           ConvertCharset(pTag->genre(), aCharset));
    AddMetadataValue(SB_PROPERTY_PRODUCERNAME,    ConvertCharset(pTag->producer(), aCharset));
    AddMetadataValue(SB_PROPERTY_COMPOSERNAME,    ConvertCharset(pTag->composer(), aCharset));
    AddMetadataValue(SB_PROPERTY_CONDUCTORNAME,   ConvertCharset(pTag->conductor(), aCharset));
    AddMetadataValue(SB_PROPERTY_LYRICISTNAME,    ConvertCharset(pTag->lyricist(), aCharset));
    AddMetadataValue(SB_PROPERTY_RECORDLABELNAME, ConvertCharset(pTag->recordLabel(), aCharset));
    AddMetadataValue(SB_PROPERTY_RATING,          ConvertCharset(pTag->rating(), aCharset));
    AddMetadataValue(SB_PROPERTY_LANGUAGE,        ConvertCharset(pTag->language(), aCharset));
    AddMetadataValue(SB_PROPERTY_KEY,             ConvertCharset(pTag->key(), aCharset));
    AddMetadataValue(SB_PROPERTY_COPYRIGHT,       ConvertCharset(pTag->license(), aCharset));
    AddMetadataValue(SB_PROPERTY_COPYRIGHTURL,    ConvertCharset(pTag->licenseUrl(), aCharset));
    AddMetadataValue(SB_PROPERTY_YEAR,            pTag->year());
    AddMetadataValue(SB_PROPERTY_TRACKNUMBER,     pTag->track());
    AddMetadataValue(SB_PROPERTY_TOTALTRACKS,     pTag->totalTracks());
    AddMetadataValue(SB_PROPERTY_DISCNUMBER,      pTag->disc());
    AddMetadataValue(SB_PROPERTY_TOTALDISCS,      pTag->totalDiscs());
    AddMetadataValue(SB_PROPERTY_BPM,             pTag->bpm());
    // todo: compilation!
  }
  
  pAudioProperties = pTagFile->audioProperties();
  if (pAudioProperties)
  {
      AddMetadataValue(SB_PROPERTY_BITRATE, pAudioProperties->bitrate());
      AddMetadataValue(SB_PROPERTY_SAMPLERATE, pAudioProperties->sampleRate());
      AddMetadataValue(SB_PROPERTY_DURATION, pAudioProperties->length() * 1000000);
  }
  
  return true; // file was valid
}


/*
 * GuessCharset
 *
 *   --> pTag                   The tags to look at
 *
 *   <--                        The best guess at the encoding the tags are in
 *
 *   This function looks at a a set of tags and tries to guess what charset
 *   (encoding) the tags are in.  Returns empty string to mean don't convert
 *   (valid Unicode), and the literal "CP_ACP" if on Windows and best guess is
 *   whatever the system locale is.
 */

void sbMetadataHandlerTaglib::GuessCharset(
    TagLib::Tag*                pTag,
    nsACString&                 _retval)
{
    nsresult rv;

    if (!pTag) {
        // no tag to read
        _retval.Truncate();
        return;
    }

    // first, build a string consisting of some tags
    TagLib::String tagString;
    tagString += pTag->album();
    tagString += pTag->artist();
    tagString += pTag->title();

    // comment and genre can end up confusing the detection; ignore them
    //tagString += pTag->comment();
    //tagString += pTag->genre();

    // first, figure out if this was from unicode - we do this by scanning the
    // string for things that had more than 8 bits.

    // XXXben We can't use the TagLib::String iterators here because TagLib was
    //        not compiled with -fshort_wchar whereas this component (and
    //        mozilla) are. Iterate manually instead.
    std::string data = tagString.toCString(true);
    NS_ConvertUTF8toUTF16 expandedData(data.c_str());

    const PRUnichar *begin, *end;
    expandedData.BeginReading(&begin, &end);

    PRBool is7Bit = PR_TRUE;
    while (begin < end) {
        PRUnichar character = *begin++;
        if (character & ~0xFF) {
            _retval.Truncate();
            return;
        }
        if (character & 0x80) {
            is7Bit = PR_FALSE;
        }
    }

    if (is7Bit) {
        _retval.AssignLiteral("us-ascii");
        return;
    }

    // XXXben The code below is going to run *slowly*, but hopefully we already
    //        exited for UTF16 and ASCII.

    // see if it's valid utf8; if yes, assume it _is_ indeed utf8
    std::string raw = tagString.toCString();
    if (IsLikelyUTF8(nsDependentCString(raw.c_str(), raw.length()))) {
        nsCOMPtr<nsIUTF8ConverterService> utf8Service;
        mProxiedServices->GetUtf8ConverterService(getter_AddRefs(utf8Service));
        if (utf8Service) {
            nsCString dataUTF8;
            rv = utf8Service->ConvertStringToUTF8(nsDependentCString(raw.c_str(), raw.length()),
                                                  "utf-8",
                                                  PR_FALSE,
                                                  dataUTF8);
            if (NS_SUCCEEDED(rv)) {
                // this was utf8
                _retval.AssignLiteral("utf-8");
                return;
            }
        }
    }

    // the metadata is in some 8-bit encoding; try to guess
    mLastConfidence = eNoAnswerYet;
    nsCOMPtr<nsICharsetDetector> detector = do_CreateInstance(
        NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector");
    if (detector) {
        nsCOMPtr<nsICharsetDetectionObserver> observer =
            do_QueryInterface( NS_ISUPPORTS_CAST(nsICharsetDetectionObserver*, this) );
        NS_ASSERTION(observer, "Um! We're supposed to be implementing this...");

        rv = detector->Init(observer);
        if (NS_SUCCEEDED(rv)) {
            PRBool isDone;
            // artificially inflate the buffer by repeating it a lot; this does
            // in fact help with the detection
            const PRUint32 chunkSize = tagString.size();
            PRUint32 currentSize = 0;
            while (currentSize < GUESS_CHARSET_MIN_CHAR_COUNT) {
                rv = detector->DoIt(raw.c_str(), chunkSize, &isDone);
                if (NS_FAILED(rv) || isDone) {
                    break;
                }
                currentSize += chunkSize;
            }
            if (NS_SUCCEEDED(rv)) {
                rv = detector->Done();
            }
            if (NS_SUCCEEDED(rv)) {
                if (eSureAnswer == mLastConfidence || eBestAnswer == mLastConfidence) {
                    _retval.Assign(mLastCharset);
                    return;
                }
            }
        }
    }

#if XP_WIN
    // we have no idea what charset this is, but we know it's bad.
    // for Windows only, assume CP_ACP

    // make the call fail if it's not valid CP_ACP
    int size = MultiByteToWideChar( CP_ACP,
                                    MB_ERR_INVALID_CHARS,
                                    data.c_str(),
                                    data.length(),
                                    nsnull,
                                    0 );
    if (size) {
        // okay, so CP_ACP is usable
        _retval.AssignLiteral("CP_ACP");
        return;
    }
#endif

    // we truely know nothing
    _retval.Truncate();
}

/* nsICharsetDetectionObserver */
NS_IMETHODIMP sbMetadataHandlerTaglib::Notify(
    const char                  *aCharset,
    nsDetectionConfident        aConf)
{
    mLastCharset.AssignLiteral(aCharset);
    mLastConfidence = aConf;
    return NS_OK;
}

TagLib::String sbMetadataHandlerTaglib::ConvertCharset(
    TagLib::String              aString,
    const char                  *aCharset)
{
    if (!aCharset || !*aCharset) {
        // no conversion
        return aString;
    }

    std::string data = aString.toCString(false);
#if XP_WIN
    if (strcmp("CP_ACP", aCharset) == 0) {
        // convert to CP_ACP
        int size = ::MultiByteToWideChar( CP_ACP,
                                          MB_ERR_INVALID_CHARS,
                                          data.c_str(),
                                          data.length(),
                                          nsnull,
                                          0 );
        if (size) {
            // convert to CP_ACP
            PRUnichar *wstr = reinterpret_cast< PRUnichar * >( NS_Alloc( (size + 1) * sizeof( PRUnichar ) ) );
            if (wstr) {
                int read = MultiByteToWideChar( CP_ACP,
                                                MB_ERR_INVALID_CHARS,
                                                data.c_str(),
                                                data.length(),
                                                wstr,
                                                size );
                NS_ASSERTION(size == read, "Win32 Current Codepage conversion failed.");

                wstr[size] = '\0';

                TagLib::String strValue(wstr);
                NS_Free(wstr);
                return strValue;
            }
        }
    }
#endif

    // convert via Mozilla
    nsCOMPtr<nsIUTF8ConverterService> utf8Service;
    mProxiedServices->GetUtf8ConverterService(getter_AddRefs(utf8Service));
    if (utf8Service) {
        nsDependentCString raw(data.c_str(), data.length());
        nsCString converted;
        nsresult rv = utf8Service->ConvertStringToUTF8(
            raw, aCharset, PR_FALSE, converted);
        if (NS_SUCCEEDED(rv))
            return TagLib::String(converted.BeginReading(), TagLib::String::UTF8);
    }

    // failed to convert :(
    return aString;
}


/*
 * ReadFLACFile
 *
 *   --> filePath               Path to FLAC file.
 *
 *   <--                        True if file has valid FLAC metadata.
 *
 *   This function reads metadata from the FLAC file with the file path
 * specified by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadFLACFile(
    nsAString                   &aFilePath)
{
    nsAutoPtr<TagLib::FLAC::File>   pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Get the file path in the proper format for the platform. */
#if XP_WIN
    nsAString &filePath = aFilePath;
#else
    nsCAutoString filePath = NS_ConvertUTF16toUTF8(aFilePath);
#endif

    /* Open and read the metadata file. */
    pTagFile = new TagLib::FLAC::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath.BeginReading());
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the Xiph comment metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadXiphTags(pTagFile->xiphComment());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

    return (isValid);
}


/*
 * ReadMPCFile
 *
 *   --> filePath               Path to MPC file.
 *
 *   <--                        True if file has valid MPC metadata.
 *
 *   This function reads metadata from the MPC file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMPCFile(
    nsAString                   &aFilePath)
{
    nsAutoPtr<TagLib::MPC::File>    pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Get the file path in the proper format for the platform. */
#if XP_WIN
    nsAString &filePath = aFilePath;
#else
    nsCAutoString filePath = NS_ConvertUTF16toUTF8(aFilePath);
#endif

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MPC::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath.BeginReading());
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the APE metadata */
    if (NS_SUCCEEDED(result) && isValid)
        ReadAPETags(pTagFile->APETag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

    return (isValid);
}


/*
 * ReadMPEGFile
 *
 *   --> filePath               Path to MPEG file.
 *
 *   <--                        True if file has valid MPEG metadata.
 *
 *   This function reads metadata from the MPEG file with the file path
 * specified by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMPEGFile(
    nsAString                   &aFilePath)
{
    nsAutoPtr<TagLib::MPEG::File>   pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Get the file path in the proper format for the platform. */
#if XP_WIN
    nsAString &filePath = aFilePath;
#else
    nsCAutoString filePath = NS_ConvertUTF16toUTF8(aFilePath);
#endif

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MPEG::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath.BeginReading());
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Guess the charset */
    nsCString charset;
    GuessCharset(pTagFile->tag(), charset);

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile, charset.BeginReading());

    /* Read the ID3v2 metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadID3v2Tags(pTagFile->ID3v2Tag(), charset.BeginReading());

    /* Read the APE metadata */
    if (NS_SUCCEEDED(result) && isValid)
        ReadAPETags(pTagFile->APETag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

    return (isValid);
}


/*
 * ReadMP4File
 *
 *   --> filePath               Path to MP4 file.
 *
 *   <--                        True if file has valid MP4 metadata.
 *
 *   This function reads metadata from the MP4 file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMP4File(
    nsAString                   &aFilePath)
{
    nsAutoPtr<TagLib::MP4::File>    pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Get the file path in the proper format for the platform. */
#if XP_WIN
    nsAString &filePath = aFilePath;
#else
    nsCAutoString filePath = NS_ConvertUTF16toUTF8(aFilePath);
#endif

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MP4::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath.BeginReading());
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

    return (isValid);
}


/*
 * ReadOGGFile
 *
 *   --> filePath               Path to OGG file.
 *
 *   <--                        True if file has valid OGG metadata.
 *
 *   This function reads metadata from the OGG file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadOGGFile(
    nsAString                   &aFilePath)
{
    nsAutoPtr<TagLib::Vorbis::File> pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Get the file path in the proper format for the platform. */
#if XP_WIN
    nsAString &filePath = aFilePath;
#else
    nsCAutoString filePath = NS_ConvertUTF16toUTF8(aFilePath);
#endif

    /* Open and read the metadata file. */
    pTagFile = new TagLib::Vorbis::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath.BeginReading());
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the Xiph metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadXiphTags(pTagFile->tag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

    return (isValid);
}


/*
 * AddMetadataValue
 *
 *   --> name                   Metadata name.
 *   --> value                  Metadata value.
 *
 *   This function adds the metadata value specified by value with the name
 * specified by name to the set of metadata values.
 */

nsresult sbMetadataHandlerTaglib::AddMetadataValue(
    const char                  *name,
    TagLib::String              value)
{
    nsresult                    result = NS_OK;

    /* Do nothing if no metadata value available. */
    if (value.isNull())
        return (result);

    /* Add the metadata value. */
    result = mpMetadataPropertyArray->AppendProperty
                        (NS_ConvertUTF8toUTF16(name),
                         NS_ConvertUTF8toUTF16(value.toCString(true)));

    return (result);
}


/*
 * AddMetadataValue
 *
 *   --> name                   Metadata name.
 *   --> value                  Metadata value.
 *
 *   This function adds the metadata value specified by value with the name
 * specified by name to the set of metadata values.
 */

nsresult sbMetadataHandlerTaglib::AddMetadataValue(
    const char                  *name,
    TagLib::uint                value)
{
    nsAutoString                valueString;
    nsresult                    result = NS_OK;

    //  Zero indicates no value
    if (value == 0) {
      return result;
    }

    /* Convert the integer value into a string. */    
    valueString.AppendInt(value);

    /* Add the metadata value. */
    result = mpMetadataPropertyArray->AppendProperty
                                     (NS_ConvertUTF8toUTF16(name),
                                      valueString);

    return (result);
}
