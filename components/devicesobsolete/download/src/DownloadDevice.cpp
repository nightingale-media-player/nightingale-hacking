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

/* *****************************************************************************
 *******************************************************************************
 *
 * Download device.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  sbDownloadDevice.cpp
* \brief Songbird DownloadDevice Component Implementation.
*/

/* *****************************************************************************
 *
 * Download device imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include "DownloadDevice.h"

/* Mozilla imports. */
#include <nsArrayUtils.h>
#include <mozilla/Mutex.h>
#include <mozilla/Monitor.h>
#include <nsComponentManagerUtils.h>
#include <nsCRT.h>
#include <nsIDOMWindow.h>
#include <nsILocalFile.h>
#include <nsINetUtil.h>
#include <nsIProperties.h>
#include <nsIStandardURL.h>
#include <nsIFileURL.h>
#include <nsISupportsPrimitives.h>
#include <nsITreeView.h>
#include <nsISupportsPrimitives.h>
#include <nsIURL.h>
#include <nsIUTF8ConverterService.h>
#include <nsIWindowWatcher.h>
#include <nsIResumableChannel.h>
#include <nsServiceManagerUtils.h>
#include <nsNetUtil.h>
#include <nsUnicharUtils.h>

#include <prmem.h>

/* Songbird imports. */
#include <sbILibraryManager.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIFileMetadataService.h>
#include <sbIJobProgress.h>
#include <sbIPropertyManager.h>
#include <sbStandardProperties.h>


/* *****************************************************************************
 *
 * Download device configuration.
 *
 ******************************************************************************/

/*
 * SB_DOWNLOAD_DEVICE_CATEGORY  Download device category name.
 * SB_DOWNLOAD_DEVICE_ID        Download device identifier.
 * SB_DOWNLOAD_DIR_DR           Default download directory data remote.
 * SB_TMP_DIR                   Songbird temporary directory name.
 * SB_DOWNLOAD_TMP_DIR          Download device temporary directory name.
 * SB_DOWNLOAD_LIST_NAME        Download device media list name.
 * SB_STRING_BUNDLE_CHROME_URL  URL for Songbird string bundle.
 * SB_DOWNLOAD_COL_SPEC         Default download device playlist column spec.
 * SB_PREF_DOWNLOAD_LIBRARY     Download library preference name.
 * SB_PREF_WEB_LIBRARY          Web library GUID preference name.
 * SB_DOWNLOAD_PROGRESS_UPDATE_PERIOD_MS
 *                              Update period for download progress.
 * SB_DOWNLOAD_IDLE_TIMEOUT_MS  How long to wait after progress before 
 *                              considering a download to be failed.
 * SB_DOWNLOAD_PROGRESS_TIMER_MS
 *                              How often should we update progress.
 *                 
 */

#define SB_DOWNLOAD_DEVICE_CATEGORY                                            \
                            NS_LITERAL_STRING("Songbird Download Device").get()
#define SB_DOWNLOAD_DEVICE_ID   "download"
#define SB_DOWNLOAD_DIR_DR "download.folder"
#define SB_TMP_DIR "Songbird"
#define SB_DOWNLOAD_TMP_DIR "DownloadDevice"
#define SB_DOWNLOAD_LIST_NAME                                                  \
                "&chrome://songbird/locale/songbird.properties#device.download"
#define SB_STRING_BUNDLE_CHROME_URL                                            \
                                "chrome://songbird/locale/songbird.properties"
#define SB_DOWNLOAD_COL_SPEC \
  "http://songbirdnest.com/data/1.0#trackName 179 http://songbirdnest.com/data/1.0#artistName 115 http://songbirdnest.com/data/1.0#albumName 115 http://songbirdnest.com/data/1.0#originPageImage 43 http://songbirdnest.com/data/1.0#downloadDetails 266 http://songbirdnest.com/data/1.0#downloadButton 73"

#define SB_PREF_DOWNLOAD_MEDIALIST "songbird.library.download"
#define SB_PREF_WEB_LIBRARY     "songbird.library.web"
#define SB_DOWNLOAD_CUSTOM_TYPE "download"
#define SB_DOWNLOAD_PROGRESS_UPDATE_PERIOD_MS   1000
#define SB_DOWNLOAD_IDLE_TIMEOUT_MS (60*1000)
#define SB_DOWNLOAD_PROGRESS_TIMER_MS 1000


/* *****************************************************************************
 *
 * Download device logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbDownloadDevice");
#define TRACE(args) PR_LOG(gLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// XXXAus: this should be moved into a base64 utilities class.
/* XXXErik: comment out to prevent unused warnings
static unsigned char *base = (unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 */

// XXXAus: this should be moved into a base64 utilities class.
static PRInt32 codetovalue( unsigned char c )
{
  if( (c >= (unsigned char)'A') && (c <= (unsigned char)'Z') )
  {
    return (PRInt32)(c - (unsigned char)'A');
  }
  else if( (c >= (unsigned char)'a') && (c <= (unsigned char)'z') )
  {
    return ((PRInt32)(c - (unsigned char)'a') +26);
  }
  else if( (c >= (unsigned char)'0') && (c <= (unsigned char)'9') )
  {
    return ((PRInt32)(c - (unsigned char)'0') +52);
  }
  else if( (unsigned char)'+' == c )
  {
    return (PRInt32)62;
  }
  else if( (unsigned char)'/' == c )
  {
    return (PRInt32)63;
  }
  else
  {
    return -1;
  }
}

// XXXAus: this should be moved into a base64 utilities class.
static PRStatus decode4to3( const unsigned char    *src,
                           unsigned char          *dest )
{
  PRUint32 b32 = (PRUint32)0;
  PRInt32 bits;
  PRIntn i;

  for( i = 0; i < 4; i++ )
  {
    bits = codetovalue(src[i]);
    if( bits < 0 )
    {
      return PR_FAILURE;
    }

    b32 <<= 6;
    b32 |= bits;
  }

  dest[0] = (unsigned char)((b32 >> 16) & 0xFF);
  dest[1] = (unsigned char)((b32 >>  8) & 0xFF);
  dest[2] = (unsigned char)((b32      ) & 0xFF);

  return PR_SUCCESS;
}

// XXXAus: this should be moved into a base64 utilities class.
static PRStatus decode3to2( const unsigned char    *src,
                           unsigned char          *dest )
{
  PRUint32 b32 = (PRUint32)0;
  PRInt32 bits;
  PRUint32 ubits;

  bits = codetovalue(src[0]);
  if( bits < 0 )
  {
    return PR_FAILURE;
  }

  b32 = (PRUint32)bits;
  b32 <<= 6;

  bits = codetovalue(src[1]);
  if( bits < 0 )
  {
    return PR_FAILURE;
  }

  b32 |= (PRUint32)bits;
  b32 <<= 4;

  bits = codetovalue(src[2]);
  if( bits < 0 )
  {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 |= (ubits >> 2);

  dest[0] = (unsigned char)((b32 >> 8) & 0xFF);
  dest[1] = (unsigned char)((b32     ) & 0xFF);

  return PR_SUCCESS;
}

// XXXAus: this should be moved into a base64 utilities class.
static PRStatus decode2to1( const unsigned char    *src,
                            unsigned char          *dest )
{
  PRUint32 b32;
  PRUint32 ubits;
  PRInt32 bits;

  bits = codetovalue(src[0]);
  if( bits < 0 )
  {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 = (ubits << 2);

  bits = codetovalue(src[1]);
  if( bits < 0 )
  {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 |= (ubits >> 4);

  dest[0] = (unsigned char)b32;

  return PR_SUCCESS;
}

// XXXAus: this should be moved into a base64 utilities class.
static PRStatus decode( const unsigned char    *src,
                        PRUint32                srclen,
                        unsigned char          *dest )
{
  PRStatus rv = PR_SUCCESS;

  while( srclen >= 4 )
  {
    rv = decode4to3(src, dest);
    if( PR_SUCCESS != rv )
    {
      return PR_FAILURE;
    }

    src += 4;
    dest += 3;
    srclen -= 4;
  }

  switch( srclen )
  {
  case 3:
    rv = decode3to2(src, dest);
    break;
  case 2:
    rv = decode2to1(src, dest);
    break;
  case 1:
    rv = PR_FAILURE;
    break;
  case 0:
    rv = PR_SUCCESS;
    break;
  default:
    PR_NOT_REACHED("coding error");
  }

  return rv;
}

// XXXAus: this should be moved into a base64 utilities class.
char * SB_Base64Decode( const char *src,
                        PRUint32    srclen,
                        char       *dest )
{
  PRStatus status;
  PRBool allocated = PR_FALSE;

  if( (char *)0 == src )
  {
    return (char *)0;
  }

  if( 0 == srclen )
  {
    srclen = strlen(src);
  }

  if( srclen && (0 == (srclen & 3)) )
  {
    if( (char)'=' == src[ srclen-1 ] )
    {
      if( (char)'=' == src[ srclen-2 ] )
      {
        srclen -= 2;
      }
      else
      {
        srclen -= 1;
      }
    }
  }

  if( (char *)0 == dest )
  {
    PRUint32 destlen = ((srclen * 3) / 4);
    dest = (char *)PR_MALLOC(destlen + 1);
    if( (char *)0 == dest )
    {
      return (char *)0;
    }
    dest[ destlen ] = (char)0; /* null terminate */
    allocated = PR_TRUE;
  }

  status = decode((const unsigned char *)src, srclen, (unsigned char *)dest);
  if( PR_SUCCESS != status )
  {
    if( PR_TRUE == allocated )
    {
      PR_DELETE(dest);
    }

    return (char *)0;
  }

  return dest;
}

// XXXAus: this should be moved into a MIME utilities class.
nsCString GetContentDispositionFilename(const nsACString &contentDisposition)
{
  NS_NAMED_LITERAL_CSTRING(DISPOSITION_ATTACHEMENT, "attachment");
  NS_NAMED_LITERAL_CSTRING(DISPOSITION_FILENAME, "filename=");

  nsCAutoString unicodeDisposition(contentDisposition);
  unicodeDisposition.StripWhitespace();

  PRInt32 pos = unicodeDisposition.Find(DISPOSITION_ATTACHEMENT,
    CaseInsensitiveCompare);
  if(pos == -1 )
    return EmptyCString();

  pos = unicodeDisposition.Find(DISPOSITION_FILENAME,
    CaseInsensitiveCompare);
  if(pos == -1)
    return EmptyCString();

  pos += DISPOSITION_FILENAME.Length();

  // if the string is quoted, we look for the next quote to know when the
  // filename ends.
  PRInt32 endPos = -1;
  if(unicodeDisposition.CharAt(pos) == '"') {

    pos++;
    endPos = unicodeDisposition.FindChar('\"', pos);

    if(endPos == -1)
      return EmptyCString();
  }
  // if not, we find the next ';' or we take the rest.
  else {
    endPos = unicodeDisposition.FindChar(';', pos);

    if(endPos == -1)  {
      endPos = unicodeDisposition.Length();
    }
  }

  nsCString filename(Substring(unicodeDisposition, pos, endPos - pos));

  // string is encoded in a different character set.
  if(StringBeginsWith(filename, NS_LITERAL_CSTRING("=?")) &&
     StringEndsWith(filename, NS_LITERAL_CSTRING("?="))) {

    nsresult rv;
    nsCOMPtr<nsIUTF8ConverterService> convServ =
      do_GetService("@mozilla.org/intl/utf8converterservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, EmptyCString());

    pos = 2;
    endPos = filename.FindChar('?', pos);

    if(endPos == -1)
      return EmptyCString();

    // found the charset
    nsCAutoString charset(Substring(filename, pos, endPos - pos));
    pos = endPos + 1;

    // find what encoding for the character set is used.
    endPos = filename.FindChar('?', pos);

    if(endPos == -1)
      return EmptyCString();

    nsCAutoString encoding(Substring(filename, pos, endPos - pos));
    pos = endPos + 1;

    ToLowerCase(encoding);

    // bad encoding.
    if(!encoding.EqualsLiteral("b") &&
       !encoding.EqualsLiteral("q")) {
      return EmptyCString();
    }

    // end of actual string to decode marked by ?=
    endPos = filename.FindChar('?', pos);
    // didn't find end, bad string
    if(endPos == -1 ||
       filename.CharAt(endPos + 1) != '=')
      return EmptyCString();

    nsCAutoString convertedFilename;
    nsCAutoString filenameToDecode(Substring(filename, pos, endPos - pos));

    if(encoding.EqualsLiteral("b")) {
      char *str = SB_Base64Decode(filenameToDecode.get(), filenameToDecode.Length(), nsnull);

      nsDependentCString strToConvert(str);
      rv = convServ->ConvertStringToUTF8(strToConvert, charset.get(), PR_TRUE, convertedFilename);

      PR_Free(str);
    }
    else if(encoding.EqualsLiteral("q")) {
      NS_WARNING("XXX: No support for Q encoded strings!");
    }

    if(NS_SUCCEEDED(rv)) {
      filename = convertedFilename;
    }
  }

  ReplaceChars(filename, nsDependentCString(FILE_ILLEGAL_CHARACTERS), '_');

  return filename;
}

/* *****************************************************************************
 *
 * Download device nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS4(sbDownloadDevice,
                   nsIObserver,
                   sbIDeviceBase,
                   sbIDownloadDevice,
                   sbIMediaListListener)


/* *****************************************************************************
 *
 * Download device services.
 *
 ******************************************************************************/

/*
 * sbDownloadDevice
 *
 *   This method is the constructor for the download device class.
 */

sbDownloadDevice::sbDownloadDevice()
:
    sbDeviceBase(),
    mpDeviceMonitor(nsnull)
{
}


/*
 * ~sbDownloadDevice
 *
 *   This method is the destructor for the download device class.
 */

sbDownloadDevice::~sbDownloadDevice()
{
}


/* *****************************************************************************
 *
 * Download device nsIObserver implementation.
 *
 ******************************************************************************/

/**
 * \brief watch for the app quitting to cancel the downloads
 */

NS_IMETHODIMP sbDownloadDevice::Observe(
    nsISupports                 *aSubject,
    const char                  *aTopic,
    const PRUnichar             *aData)
{
    nsresult rv;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aTopic);

    if (!strcmp("quit-application-granted", aTopic)) {
        /* Shutdown the session. */
        if (mpDownloadSession)
        {
            mpDownloadSession->Shutdown();
            mpDownloadSession = nsnull;
        }

        // remember to remove the observer too
        nsCOMPtr<nsIObserverService> obsSvc =
              do_GetService("@mozilla.org/observer-service;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = obsSvc->RemoveObserver(this, aTopic);
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
}


/* *****************************************************************************
 *
 * Download device sbIDeviceBase implementation.
 *
 ******************************************************************************/

/**
 * \brief Initialize the device category handler.
 */

NS_IMETHODIMP sbDownloadDevice::Initialize()
{
    nsCOMPtr<sbIPropertyManager>
                                pPropertyManager;
    nsCOMPtr<sbIURIPropertyInfo>
                                pURIPropertyInfo;
    nsCOMPtr<sbILibraryManager> pLibraryManager;
    nsresult                    rv;

    /* Initialize the base services. */
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

    /* Set the download device identifier. */
    mDeviceIdentifier = NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID);

    /* Initialize the device state. */
    rv = InitDeviceState(mDeviceIdentifier);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the IO service. */
    mpIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the pref service. */
    mpPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the library manager. */
    pLibraryManager =
            do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the string bundle. */
    {
        nsCOMPtr<nsIStringBundleService>
                                    pStringBundleService;

        /* Get the download device string bundle. */
        pStringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = pStringBundleService->CreateBundle(SB_STRING_BUNDLE_CHROME_URL,
                                                getter_AddRefs(mpStringBundle));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Get some strings. */
    rv = mpStringBundle->GetStringFromName
                            (NS_LITERAL_STRING("device.download.queued").get(),
                             getter_Copies(mQueuedStr));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the main library. */
    rv = pLibraryManager->GetMainLibrary(getter_AddRefs(mpMainLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the web library. */
    {
        nsCOMPtr<nsISupportsString> pSupportsString;
        nsAutoString                webLibraryGUID;

        /* Read the web library GUID from the preferences. */
        rv = mpPrefBranch->GetComplexValue(SB_PREF_WEB_LIBRARY,
                                           NS_GET_IID(nsISupportsString),
                                           getter_AddRefs(pSupportsString));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = pSupportsString->GetData(webLibraryGUID);
        NS_ENSURE_SUCCESS(rv, rv);

        /* Get the web library. */
        rv = pLibraryManager->GetLibrary(webLibraryGUID,
                                         getter_AddRefs(mpWebLibrary));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Initialize the download media list. */
    rv = InitializeDownloadMediaList();
    NS_ENSURE_SUCCESS(rv, rv);

    /* Listen to the main library to detect     */
    /* when the download media list is deleted. */
    rv = mpMainLibrary->AddListener
                                (this,
                                 PR_FALSE,
                                   sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED
                                 | sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                                 nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create the device transfer queue. */
    rv = CreateTransferQueue(mDeviceIdentifier);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create temporary download directory. */
    {
        nsCOMPtr<nsIProperties>     pProperties;
        PRUint32                    permissions;
        PRBool                      exists;

        /* Get the system temporary directory. */
        pProperties = do_GetService("@mozilla.org/file/directory_service;1",
                                    &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = pProperties->Get("TmpD",
                              NS_GET_IID(nsIFile),
                              getter_AddRefs(mpTmpDownloadDir));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mpTmpDownloadDir->GetPermissions(&permissions);
        NS_ENSURE_SUCCESS(rv, rv);

        /* Get the Songbird temporary directory and make sure it exists. */
        rv = mpTmpDownloadDir->Append(NS_LITERAL_STRING(SB_TMP_DIR));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mpTmpDownloadDir->Exists(&exists);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!exists)
        {
            rv = mpTmpDownloadDir->Create(nsIFile::DIRECTORY_TYPE, permissions);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        /* Get the download temporary directory and make sure it's empty.  If */
        /* directory cannot be deleted, don't propagate error (bug #6453).    */
        rv = mpTmpDownloadDir->Append(NS_LITERAL_STRING(SB_DOWNLOAD_TMP_DIR));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mpTmpDownloadDir->Exists(&exists);
        NS_ENSURE_SUCCESS(rv, rv);
        if (exists)
        {
            rv = mpTmpDownloadDir->Remove(PR_TRUE);
            if (NS_SUCCEEDED(rv))
                exists = PR_FALSE;
        }
        if (!exists)
        {
            rv = mpTmpDownloadDir->Create(nsIFile::DIRECTORY_TYPE, permissions);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    /* Watch for the app quitting so we can gracefully abort. */
    {
        nsCOMPtr<nsIObserverService> obsSvc =
                        do_GetService("@mozilla.org/observer-service;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = obsSvc->AddObserver(this, "quit-application-granted", PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    // Need thread pool to process file moves off the main thread.
    nsCOMPtr<nsIThreadPool> threadPool = 
      do_CreateInstance("@mozilla.org/thread-pool;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = threadPool->SetIdleThreadLimit(0);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = threadPool->SetIdleThreadTimeout(60000);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = threadPool->SetThreadLimit(1);
    NS_ENSURE_SUCCESS(rv, rv);

    // Thread pool is setup.
    threadPool.forget(getter_AddRefs(mFileMoveThreadPool));

    /* Resume incomplete transfers. */
    ResumeTransfers();

    return NS_OK;
}


/**
 * \brief Finalize usage of the device category handler.
 *
 * This effectively prepares the device category handler for
 * application shutdown.
 */

NS_IMETHODIMP sbDownloadDevice::Finalize()
{
  {
    /* Lock the download device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Dispose of any outstanding download sessions. */
    if (mpDownloadSession)
    {
        mpDownloadSession->Shutdown();
        mpDownloadSession = nsnull;
    }

    /* Remove the device transfer queue. */
    RemoveTransferQueue(mDeviceIdentifier);

    /* Remove main library listener. */
    if (mpMainLibrary)
        mpMainLibrary->RemoveListener(this);

    /* Finalize the download media list. */
    FinalizeDownloadMediaList();
  }

  mpMainLibrary = nsnull;
  mpWebLibrary = nsnull;

  return (NS_OK);
}


/**
 * \brief Add a device category handler callback.
 *
 * Enables you to get callbacks when devices are connected and when
 * transfers are initiated.
 *
 * \param aCallback The device category handler callback you wish to add.
 */

NS_IMETHODIMP sbDownloadDevice::AddCallback(
    sbIDeviceBaseCallback       *aCallback)
{
    return (sbDeviceBase::AddCallback(aCallback));
}


/**
 * \brief Remove a device category handler callback.
 *
 * \param aCallback The device category handler callback you wish to remove.
 */

NS_IMETHODIMP sbDownloadDevice::RemoveCallback(
    sbIDeviceBaseCallback       *aCallback)
{
    return (sbDeviceBase::RemoveCallback(aCallback));
}


/**
 * \brief Get the device library representing the content
 * available on the device.
 *
 * \param aDeviceIdentifier The unique device identifier.
 * \return The library for the specified device.
 */

NS_IMETHODIMP sbDownloadDevice::GetLibrary(
    const nsAString             &aDeviceIdentifier,
    sbILibrary                  **aLibrary)
{
    return (GetLibraryForDevice(mDeviceIdentifier, aLibrary));
}


/**
 * \brief Get the device's current state.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The current state of the device.
 */

NS_IMETHODIMP sbDownloadDevice::GetDeviceState(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aState)
{
    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Return results. */
    return (sbDeviceBase::GetDeviceState(mDeviceIdentifier, aState));
}


/**
 * \brief Get preferred transfer location for item
 *
 * Get a transfer location for the specified media item. This enables
 * the device to determine where best to put this media item based on
 * it's own set of criteria.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItem The media item that is about to be transferred.
 * \return The transfer location for the item.
 */

NS_IMETHODIMP sbDownloadDevice::GetTransferLocation(
    const nsAString             &aDeviceIdentifier,
    sbIMediaItem                *aMediaItem,
    nsIURI                      **aTransferLocationURI)
{
    LOG(("1: GetTransferLocation\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Transfer items to a library or destination.
 *
 * If you call this method with a library only, the device
 * will attempt to read the library's preferred transfer
 * destination and use it.
 *
 * If you call this method with a destination only, the device
 * will attempt to transfer the items to that destination only.
 *
 * If you call this method with a library and destination path, the
 * device will attempt to simply pass a destination if you do not wish to add
 * the item to a library after the transfer is complete.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItems An array of media items to transfer.
 * \param aDestinationPath The desired destination path, path may include full filename.
 * \param aDeviceOperation The desired operation (upload, download, move).
 * \param aBeginTransferNow Begin the transfer operation immediately?
 * \param aDestinationLibrary The desired destination library.
 * \return Number of items actually queued for transfer.
 */

NS_IMETHODIMP sbDownloadDevice::TransferItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    nsIURI                      *aDestinationPath,
    PRUint32                    aDeviceOperation,
    PRBool                      aBeginTransferNow,
    sbILibrary                  *aDestinationLibrary,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount;
    PRUint32                    i;
    nsresult                    rv;

    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(aMediaItems);

    /* Do nothing unless operation is upload.  Uploading an item to the   */
    /* download device will download the item to the destination library. */
    if (aDeviceOperation != OP_UPLOAD)
        return (NS_ERROR_NOT_IMPLEMENTED);

    /* Clear completed items. */
    ClearCompletedItems();

    /* Add all the media items to the transfer queue.  If any error    */
    /* occurs on an item, remove it and continue with remaining items. */
    rv = aMediaItems->GetLength(&itemCount);
    NS_ENSURE_SUCCESS(rv, rv);
    for (i = 0; i < itemCount; i++)
    {
        /* Get the next media item.  Skip to next item on error. */
        pMediaItem = do_QueryElementAt(aMediaItems, i, &rv);
        if (NS_FAILED(rv))
            continue;

        /* Enqueue the media item for download. */
        rv = EnqueueItem(pMediaItem);

        /* Remove item on error. */
        if (NS_FAILED(rv))
        {
            mpDeviceLibraryListener->SetIgnoreListener(PR_TRUE);
            mpDownloadMediaList->Remove(pMediaItem);
            mpDeviceLibraryListener->SetIgnoreListener(PR_FALSE);
        }
    }

    /* Run the transfer queue. */
    rv = RunTransferQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/**
 * \brief Update items on the device.
 */

NS_IMETHODIMP sbDownloadDevice::UpdateItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aItemCount)
{
  //NS_WARNING("UpdateItems not implemented");
  return NS_OK;
}


/**
 * \brief Delete items from the device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItems An array of sbIMediaItem objects.
 *
 * \return Number of items actually deleted from the device.
 */

NS_IMETHODIMP sbDownloadDevice::DeleteItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    arrayLength;
    PRUint32                    i;
    PRBool                      equals;
    PRBool                      cancelSession = PR_FALSE;
    PRUint32                    itemCount = 0;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aMediaItems);
    NS_ENSURE_ARG_POINTER(aItemCount);

    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Remove the items. */
    result = aMediaItems->GetLength(&arrayLength);
    for (i = 0; (NS_SUCCEEDED(result) && (i < arrayLength)); i++)
    {
        /* Get the media item to remove. */
        pMediaItem = do_QueryElementAt(aMediaItems, i, &result);

        /* Remove the item from the transfer queue. */
        if (NS_SUCCEEDED(result))
        {
            result1 = RemoveItemFromTransferQueue(mDeviceIdentifier,
                                                  pMediaItem);
            if (NS_SUCCEEDED(result1))
                itemCount++;
        }

        /* Reset the status target to new */
        if (NS_SUCCEEDED(result))
        {
          nsCOMPtr<sbIMediaItem> statusTarget;
          nsresult rv = GetStatusTarget(pMediaItem,
                                        getter_AddRefs(statusTarget));
          NS_ENSURE_SUCCESS(rv, rv);
          sbAutoDownloadButtonPropertyValue property(pMediaItem, statusTarget);
          if (property.value->GetMode() !=
              sbDownloadButtonPropertyValue::eComplete) {
            property.value->SetMode(sbDownloadButtonPropertyValue::eNew);
          }
        }

        /* Check if current session should be deleted. */
        if (NS_SUCCEEDED(result) && mpDownloadSession && !cancelSession)
        {
            result1 = pMediaItem->Equals(mpDownloadSession->mpMediaItem,
                                         &equals);
            if (NS_SUCCEEDED(result1) && equals)
                cancelSession = PR_TRUE;
        }
    }

    /* Cancel the session if needed.  This counts as another deleted */
    /* item since the item would not be in the transfer queue.       */
    if (cancelSession)
    {
        result1 = CancelSession();
        if (NS_SUCCEEDED(result1))
            itemCount++;
    }

    /* Return results. */
    *aItemCount = itemCount;

    return (result);
}


/**
 * \brief Delete all items from the device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of items actually deleted from the device.
 */

NS_IMETHODIMP sbDownloadDevice::DeleteAllItems(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount = 0;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aItemCount);

    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Remove all the items in the queue. */
    while (GetNextTransferItem(getter_AddRefs(pMediaItem)))
        itemCount++;

    /* Cancel active download session. */
    if (mpDownloadSession)
    {
        result1 = CancelSession();
        if (NS_SUCCEEDED(result1))
            itemCount++;
    }

    /* Return results. */
    *aItemCount = itemCount;

    return (result);
}


/**
 * \brief Begin transfer operations.
 * \return The media item that began transferring.
 */

NS_IMETHODIMP sbDownloadDevice::BeginTransfer(
    const nsAString             &aDeviceIdentifier,
    sbIMediaItem                **aMediaItem)
{
    LOG(("1: BeginTransfer\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Cancel a transfer by removing it from the queue.
 *
 * Cancel a transfer by removing it from the queue. If the item
 * has already been transfered this function has no effect.
 *
 * \param aDeviceIdentifier The device unique identifier
 * \param aMediaItems An array of media items.
 * \return The number of transfers actually cancelled.
 */

NS_IMETHODIMP sbDownloadDevice::CancelTransfer(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aNumItems)
{
    LOG(("1: CancelTransfer\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Suspend all transfers.
 *
 * Suspend all active transfers.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of transfer items actually suspended.
 */

NS_IMETHODIMP sbDownloadDevice::SuspendTransfer(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aNumItems)
{
    PRUint32                    numItems = 0;
    nsresult                    rv;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aNumItems);

    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* If there's a download session, suspend it. */
    if (mpDownloadSession)
    {
        rv = mpDownloadSession->Suspend();
        NS_ENSURE_SUCCESS(rv, rv);
        rv = SetDeviceState(mDeviceIdentifier, STATE_DOWNLOAD_PAUSED);
        NS_ENSURE_SUCCESS(rv, rv);
        numItems = 1;
    }

    /* Return results. */
    *aNumItems = numItems;

    return (NS_OK);
}


/**
 * \brief Resume pending transfers.
 *
 * Resume pending transfers for a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of transfer items actually resumed.
 */

NS_IMETHODIMP sbDownloadDevice::ResumeTransfer(
   const nsAString              &aDeviceIdentifier,
   PRUint32                     *aNumItems)
{
    PRUint32                    numItems = 0;
    nsresult                    rv;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aNumItems);

    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* If there's a download session, resume it. */
    if (mpDownloadSession)
    {
        rv = mpDownloadSession->Resume();
        NS_ENSURE_SUCCESS(rv, rv);
        rv = SetDeviceState(mDeviceIdentifier, STATE_DOWNLOADING);
        NS_ENSURE_SUCCESS(rv, rv);
        numItems = 1;
    }

    /* Return results. */
    *aNumItems = numItems;

    return (NS_OK);
}


/**
 * \brief Get the amount of used space from a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The amount of used space on the device, in bytes.
 */

NS_IMETHODIMP sbDownloadDevice::GetUsedSpace(
    const nsAString             &aDeviceIdentifier,
    PRInt64                     *aUsedSpace)
{
    LOG(("1: GetUsedSpace\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Get the amount of available space from a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The amount of available space, in bytes.
 */

NS_IMETHODIMP sbDownloadDevice::GetAvailableSpace(
    const nsAString             &aDeviceIdentifier,
    PRInt64                     *aAvailableSpace)
{
    LOG(("1: GetAvailableSpace\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Returns a list of file extensions representing the
 * formats supported by a specific device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return An array of nsISupportsString containing the supported
 * formats in file extension format.
 */

NS_IMETHODIMP sbDownloadDevice::GetSupportedFormats(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    **aSupportedFormats)
{
    LOG(("1: GetSupportedFormats\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Download is to copy a track from the device to the host.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsDownloadSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aDownloadSupported)
{
    LOG(("1: IsDownloadSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is uploading supported on this device?
 *
 * Upload is to copy a track from host to the device
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsUploadSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aUploadSupported)
{
    LOG(("1: IsUploadSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is it possible to delete items from the device?
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsDeleteSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aDeleteSupported)
{
    LOG(("1: IsDeleteSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is it possible to update items directly on the device?
 *
 * This method could be used for updating tracks on a device
 * or applying CDDB match for a CD.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsUpdateSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aUpdateSupported)
{
    LOG(("1: IsUpdateSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is eject or unmount supported by the device?.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsEjectSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aEjectSupported)
{
    LOG(("1: IsEjectSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Eject or unmount the device from the system.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::EjectDevice(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aEjected)
{
    LOG(("1: EjectDevice\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/*
 * sbIDeviceBase attribute getters/setters.
 */

NS_IMETHODIMP sbDownloadDevice::GetName(
    nsAString                   &aName)
{
    LOG(("1: GetName\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::SetName(
    const nsAString             &aName)
{
    LOG(("1: SetName\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceCategory(
    nsAString                   &aDeviceCategory)
{
    aDeviceCategory.Assign(SB_DOWNLOAD_DEVICE_CATEGORY);
    return (NS_OK);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceIdentifiers(
    nsIStringEnumerator         **aDeviceIdentifiers)
{
    LOG(("1: GetDeviceIdentifiers\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceCount(
    PRUint32                    *aDeviceCount)
{
    LOG(("1: GetDeviceCount\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/* *****************************************************************************
 *
 * Download device sbIDownloadDevice implementation.
 *
 ******************************************************************************/

/**
 * \brief Clear all completed items from download device.
 */

NS_IMETHODIMP sbDownloadDevice::ClearCompletedItems()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount;
    PRInt32                     i;
    nsresult                    rv;

    /* Get the number of download items. */
    rv = mpDownloadMediaList->GetLength(&itemCount);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Add all the media items to the transfer queue.  If any  */
    /* error occurs on an item, continue with remaining items. */
    for (i = itemCount - 1; i >= 0; i--)
    {
        /* Get the next item.  Skip to next item on error. */
        rv= mpDownloadMediaList->GetItemByIndex(i, getter_AddRefs(pMediaItem));
        if (NS_FAILED(rv))
            continue;

        /* Remove item from download device library if complete. */
        sbAutoDownloadButtonPropertyValue property(pMediaItem, nsnull, PR_TRUE);
        if (   property.value->GetMode()
            == sbDownloadButtonPropertyValue::eComplete)
        {
            mpDeviceLibraryListener->SetIgnoreListener(PR_TRUE);
            rv = mpDownloadMediaList->Remove(pMediaItem);
            mpDeviceLibraryListener->SetIgnoreListener(PR_FALSE);

            /* Warn if item could not be removed. */
            if (NS_FAILED(rv))
                NS_WARNING("Failed to remove completed download item.\n");
        }
    }

    return (NS_OK);
}


/*
 * sbIDownloadDevice attribute getters/setters.
 */

NS_IMETHODIMP sbDownloadDevice::GetDownloadMediaList(
    sbIMediaList                **aDownloadMediaList)
{
    nsresult                    rv;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aDownloadMediaList);

    /* Ensure the download media list is initialized. */
    rv = InitializeDownloadMediaList();
    NS_ENSURE_SUCCESS(rv, rv);

    /* Return results. */
    NS_ADDREF(*aDownloadMediaList = mpDownloadMediaList);

    return (NS_OK);
}

NS_IMETHODIMP sbDownloadDevice::GetCompletedItemCount(
    PRUint32                    *aCompletedItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount;
    PRUint32                    completedItemCount = 0;
    PRUint32                    i;
    nsresult                    rv;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aCompletedItemCount);

    /* Get the number of download items. */
    rv = mpDownloadMediaList->GetLength(&itemCount);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Count the number of completed items. */
    /*XXXeps there must be a quicker way to do this. */
    for (i = 0; i < itemCount; i++)
    {
        /* Get the next item. */
        rv= mpDownloadMediaList->GetItemByIndex(i, getter_AddRefs(pMediaItem));
        NS_ENSURE_SUCCESS(rv, rv);

        /* Increment item completed count if item has completed. */
        sbAutoDownloadButtonPropertyValue property(pMediaItem, nsnull, PR_TRUE);
        if (   property.value->GetMode()
            == sbDownloadButtonPropertyValue::eComplete)
        {
            completedItemCount++;
        }
    }

    /* Return results. */
    *aCompletedItemCount = completedItemCount;

    return (NS_OK);
}


/* *****************************************************************************
 *
 * Download device sbIMediaListListener implementation.
 *
 ******************************************************************************/

/**
 * \brief Called when a media item is added to the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The new media item.
 * \return True if you do not want any further onItemAdded notifications for
 *         the current batch.  If there is no current batch, the return value
 *         is ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnItemAdded(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRUint32                    aIndex,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_TRUE;
    return (NS_OK);
}


/**
 * \brief Called before a media item is removed from the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The media item to be removed
 * \return True if you do not want any further onBeforeItemRemoved
 *         notifications for the current batch.  If there is no current batch,
 *         the return value is ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnBeforeItemRemoved(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRUint32                    aIndex,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_TRUE;
    return (NS_OK);
}


/**
 * \brief Called after a media item is removed from the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The removed media item.
 * \return True if you do not want any further onAfterItemRemoved for the
 *         current batch.  If there is no current batch, the return value is
 *         ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnAfterItemRemoved(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRUint32                    aIndex,
    PRBool                      *_retval)
{
    PRBool                      isEqual;
    nsresult                    rv;

    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(aMediaItem);
    NS_ENSURE_ARG_POINTER(_retval);

    /* If the download device media list was removed, re-initialize it. */
    rv = mpDownloadMediaList->Equals(aMediaItem, &isEqual);
    if (NS_SUCCEEDED(rv) && isEqual)
        InitializeDownloadMediaList();

    *_retval = PR_FALSE;
    return (NS_OK);
}


/**
 * \brief Called when a media item is changed.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The item that has changed.
 * \param aProperties Array of properties that were updated.  Each property's
 *        value is the previous value of the property
 * \return True if you do not want any further onItemUpdated notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnItemUpdated(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    sbIPropertyArray            *aProperties,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_TRUE;
    return (NS_OK);
}

NS_IMETHODIMP sbDownloadDevice::OnItemMoved(
    sbIMediaList                *aMediaList,
    PRUint32                    aFromIndex,
    PRUint32                    aToIndex,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_TRUE;
    return (NS_OK);
}

/**
 * \Brief Called when a media list is cleared.
 * \param sbIMediaList aMediaList The list that was cleared.
 * \param aExcludeLists If true, only media items, not media lists, were
 *                      cleared.
 * \return True if you do not want any further onListCleared notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnListCleared(
    sbIMediaList                *aMediaList,
    PRBool                      aExcludeLists,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    /* Re-initialize the download device media list. */
    InitializeDownloadMediaList();

    *_retval = PR_FALSE;
    return (NS_OK);
}

/**
 * \Brief Called before a media list is cleared.
 * \param sbIMediaList aMediaList The list that is about to be cleared.
 * \param aExcludeLists If true, only media items, not media lists, are being
 *                      cleared.
 * \return True if you do not want any further onBeforeListCleared notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP sbDownloadDevice::OnBeforeListCleared(
    sbIMediaList                *aMediaList,
    PRBool                      aExcludeLists,
    PRBool                      *_retval)
{
    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_FALSE;
    return (NS_OK);
}

NS_IMETHODIMP
sbDownloadDevice::CreatePlaylists(const nsAString &aDeviceIdentifier,
                                  nsIArray *aMediaLists,
                                  PRUint32 *aItemCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::DeletePlaylists(const nsAString &aDeviceIdentifier,
                                  nsIArray *aMediaLists,
                                  PRUint32 *aItemCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::AddToPlaylist(const nsAString &aDeviceIdentifier,
                                sbIMediaList *aMediaList,
                                nsIArray *aMediaLists,
                                PRUint32 aBeforeIndex,
                                PRUint32 *aItemCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::RemoveFromPlaylist(const nsAString &aDeviceIdentifier,
                                     sbIMediaList *aMediaList,
                                     sbIMediaItem *aMediaItem,
                                     PRUint32 aIndex,
                                     PRUint32 *aItemCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::ClearPlaylist(const nsAString &aDeviceIdentifier,
                                sbIMediaList *aMediaList,
                                PRUint32 *aItemCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::MovePlaylistItem(const nsAString &aDeviceIdentifier,
                                   sbIMediaList *aMediaList,
                                   PRUint32 aFromIndex,
                                   PRUint32 aToIndex,
                                   PRUint32 *aItemCount)
{
  return NS_OK;
}

/**
 * \brief Called when the library is about to perform multiple operations at
 *        once.
 *
 * This notification can be used to optimize behavior. The consumer may
 * choose to ignore further onItemAdded or onItemRemoved notifications until
 * the onBatchEnd notification is received.
 *
 * \param aMediaList The list that has changed.
 */

NS_IMETHODIMP sbDownloadDevice::OnBatchBegin(
    sbIMediaList                *aMediaList)
{
    return (NS_OK);
}


/**
 * \brief Called when the library has finished performing multiple operations
 *        at once.
 *
 * This notification can be used to optimize behavior. The consumer may
 * choose to stop ignoring onItemAdded or onItemRemoved notifications after
 * receiving this notification.
 *
 * \param aMediaList The list that has changed.
 */

NS_IMETHODIMP sbDownloadDevice::OnBatchEnd(
    sbIMediaList                *aMediaList)
{
    return (NS_OK);
}


/* *****************************************************************************
 *
 * Download device media list services.
 *
 ******************************************************************************/

/*
 * InitializeDownloadMediaList
 *
 *   This function initializes the download device media list.  This function is
 * designed to be called multiple times to ensure that a valid download media
 * list is present and initialized.
 *   If the media list has already been initialized and is still valid, this
 * function does nothing.  If the media list already exists, this function
 * installs listeners and performs other initialization.  If the media list does
 * not exist, this function will create it and ensure that no items are being
 * downloaded.
 */

nsresult sbDownloadDevice::InitializeDownloadMediaList()
{
    nsAutoString                downloadMediaListGUID;
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    nsresult                    rv;

    /* Lock the download device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Handle case where download media list has already been initialized. */
    if (mpDownloadMediaList)
    {
        /* Do nothing if a valid download media list is already initialized. */
        rv = mpDownloadMediaList->GetGuid(downloadMediaListGUID);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mpMainLibrary->GetMediaItem(downloadMediaListGUID,
                                         getter_AddRefs(pMediaItem));
        if (NS_SUCCEEDED(rv))
            return (NS_OK);

        /* Finalize download media list. */
        FinalizeDownloadMediaList();
    }

    /* Get any existing download media list. */
    GetDownloadMediaList();

    /* Create the download media list if one does not exist. */
    if (!mpDownloadMediaList)
    {
        PRUint32                    itemCount;

        /* Ensure all items are deleted from device. */
        rv = DeleteAllItems(mDeviceIdentifier, &itemCount);
        NS_ENSURE_SUCCESS(rv, rv);

        /* Create the download media list. */
        rv = CreateDownloadMediaList();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Update the download media list. */
    rv = UpdateDownloadMediaList();
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create a download media list listener. */
    mpDeviceLibraryListener = new sbDeviceBaseLibraryListener;
    NS_ENSURE_TRUE(mpDeviceLibraryListener, NS_ERROR_OUT_OF_MEMORY);
    rv = mpDeviceLibraryListener->Init(mDeviceIdentifier, this);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Add the download device media list listener.*/
    rv = mpDownloadMediaList->AddListener
                                (mpDeviceLibraryListener,
                                 PR_FALSE,
                                   sbIMediaList::LISTENER_FLAGS_ITEMADDED
                                 | sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED
                                 | sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                                 nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = SetListenerForDeviceLibrary(mDeviceIdentifier,
                                     mpDeviceLibraryListener);
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/*
 * FinalizeDownloadMediaList
 *
 *   This function finalizes the download media list.  It removes listeners and
 * undoes any other initialization.  It does not delete the download media list.
 */

void sbDownloadDevice::FinalizeDownloadMediaList()
{
    /* Remove download media list listener. */
    if (mpDownloadMediaList && mpDeviceLibraryListener)
        mpDownloadMediaList->RemoveListener(mpDeviceLibraryListener);
    mpDownloadMediaList = nsnull;
    mpDeviceLibraryListener = nsnull;
}


/*
 * CreateDownloadMediaList
 *
 *   This function creates the download device media list.
 */

nsresult sbDownloadDevice::CreateDownloadMediaList()
{
    nsAutoString                downloadMediaListGUID;
    nsresult                    rv;

    /* Create the download media list. */
    rv = mpMainLibrary->CreateMediaList(NS_LITERAL_STRING("simple"),
                                        nsnull,
                                        getter_AddRefs(mpDownloadMediaList));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Save the media list GUID in the main      */
    /* library properties so others can find it. */
    rv = mpDownloadMediaList->GetGuid(downloadMediaListGUID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mpMainLibrary->SetProperty
             (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_MEDIALIST_GUID),
              downloadMediaListGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/*
 * GetDownloadMediaList
 *
 *   This function gets the download device media list.
 */

void sbDownloadDevice::GetDownloadMediaList()
{
    nsCOMPtr<nsISupportsString> pSupportsString;
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    nsAutoString                downloadMediaListGUID;
    nsresult                    rv;

    /* Read the download media list GUID. */
    rv = mpMainLibrary->GetProperty
             (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_MEDIALIST_GUID),
              downloadMediaListGUID);
    if (NS_FAILED(rv) || downloadMediaListGUID.IsEmpty())
    {
        /* For backward compatibility, read the download */
        /* media list GUID from the preferences.         */
        rv = mpPrefBranch->GetComplexValue(SB_PREF_DOWNLOAD_MEDIALIST,
                                           NS_GET_IID(nsISupportsString),
                                           getter_AddRefs(pSupportsString));
        if (NS_FAILED(rv)) return;
        rv = pSupportsString->GetData(downloadMediaListGUID);
        if (NS_FAILED(rv)) return;

        /* Set the download media list GUID main library property. */
        mpMainLibrary->SetProperty
            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_MEDIALIST_GUID),
             downloadMediaListGUID);
    }

    /* Get the download media list from the main library. */
    rv = mpMainLibrary->GetMediaItem(downloadMediaListGUID,
                                     getter_AddRefs(pMediaItem));
    if (NS_FAILED(rv)) return;
    mpDownloadMediaList = do_QueryInterface(pMediaItem, &rv);
    if (NS_FAILED(rv))
        mpDownloadMediaList = nsnull;
}


/*
 * UpdateDownloadMediaList
 *
 *   This function updates the download device media list.
 */

nsresult sbDownloadDevice::UpdateDownloadMediaList()
{
    nsresult rv;

    /* Set the download device media list name. */
    rv = mpDownloadMediaList->SetName(NS_LITERAL_STRING(SB_DOWNLOAD_LIST_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Set the download device media list default column spec. */
    nsAutoString downloadColSpec;
    downloadColSpec.AppendLiteral(SB_DOWNLOAD_COL_SPEC);
    rv = mpDownloadMediaList->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                             downloadColSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Set the download device media list custom type property. */
    rv = mpDownloadMediaList->SetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                                 NS_LITERAL_STRING(SB_DOWNLOAD_CUSTOM_TYPE));

    /* Set the sortable property to false. */
    rv = mpDownloadMediaList->SetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE),
                                 NS_LITERAL_STRING("0"));

    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}


/* *****************************************************************************
 *
 * Download device transfer services.
 *
 ******************************************************************************/

/*
 * EnqueueItem
 *
 *   --> apMediaItem            Media item to enqueue.
 *
 *   This function enqueues the media item specified by apMediaItem for
 * download.
 */

nsresult sbDownloadDevice::EnqueueItem(
    sbIMediaItem                *apMediaItem)
{
    nsresult                    rv;

    /* Set the item transfer destination. */
    rv = SetTransferDestination(apMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Update the download button property to reflect the stating state. */
    nsCOMPtr<sbIMediaItem> statusTarget;
    rv = GetStatusTarget(apMediaItem, getter_AddRefs(statusTarget));
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoDownloadButtonPropertyValue property(apMediaItem, statusTarget);
    property.value->SetMode(sbDownloadButtonPropertyValue::eStarting);

    /* Mark the download detail property as queued. */
    rv = apMediaItem->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             mQueuedStr);
    NS_ENSURE_SUCCESS(rv, rv);
    if (statusTarget)
    {
        rv = statusTarget->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             mQueuedStr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Add item to the transfer queue. */
    {
        mozilla::MonitorAutoLock mon(mpDeviceMonitor);
        rv = AddItemToTransferQueue(mDeviceIdentifier, apMediaItem);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return (NS_OK);
}


/*
 * RunTransferQueue
 *
 *   This function runs the transfer queue.  It will start transferring the next
 * item in the transfer queue.
 */

nsresult sbDownloadDevice::RunTransferQueue()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRBool                      initiated = PR_FALSE;
    nsresult                    result = NS_OK;

    /* Lock the device. */
    mozilla::MonitorAutoLock mon(mpDeviceMonitor);

    /* Initiate transfers until queue is busy or empty. */
    while (   !mpDownloadSession
           && GetNextTransferItem(getter_AddRefs(pMediaItem)))
    {
        /* Initiate item transfer. */
        mpDownloadSession = new sbDownloadSession(this, pMediaItem);
        if (!mpDownloadSession)
            result = NS_ERROR_OUT_OF_MEMORY;
        if (NS_SUCCEEDED(result))
            result = mpDownloadSession->Initiate();
        if (NS_SUCCEEDED(result))
            initiated = PR_TRUE;
        else
            initiated = PR_FALSE;

        /* Send notification that the transfer started. */
        if (initiated)
            DoTransferStartCallback(pMediaItem);

        /* Release the download session if not initiated. */
        if (!initiated && mpDownloadSession)
            mpDownloadSession = nsnull;
    }

    /* Update device state. */
    if (mpDownloadSession)
    {
        if (!mpDownloadSession->IsSuspended())
            SetDeviceState(mDeviceIdentifier, STATE_DOWNLOADING);
        else
            SetDeviceState(mDeviceIdentifier, STATE_DOWNLOAD_PAUSED);
    }
    else
    {
        SetDeviceState(mDeviceIdentifier, STATE_IDLE);
    }

    return result;
}


/*
 * GetNextTransferItem
 *
 *   <-- appMediaItem           Next media item to transfer.
 *
 *   <-- True                   The next media item to transfer was returned.
 *       False                  The transfer queue is busy or no media is
 *                              available.
 *
 *   This function returns in appMediaItem the next media item to transfer.  If
 * a media item is available for transfer and the transfer queue is not busy,
 * this function returns true; otherwise, it returns false.
 */

PRBool sbDownloadDevice::GetNextTransferItem(
    sbIMediaItem                **appMediaItem)
{
    nsCOMPtr<sbIMediaItem>          pMediaItem;
    nsresult                        result = NS_OK;

    /* Get the next media item to transfer. */
    result = GetNextItemFromTransferQueue(mDeviceIdentifier,
                                          getter_AddRefs(pMediaItem));
    if (NS_SUCCEEDED(result))
        result = RemoveItemFromTransferQueue(mDeviceIdentifier, pMediaItem);

    /* Return results. */
    if (NS_SUCCEEDED(result))
    {
        NS_ADDREF(*appMediaItem = pMediaItem);
        return (PR_TRUE);
    }
    else
    {
        return (PR_FALSE);
    }
}


/*
 * ResumeTransfers
 *
 *   This function resumes all uncompleted transfers in the download device
 * library.
 */

nsresult sbDownloadDevice::ResumeTransfers()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount;
    PRUint32                    queuedCount = 0;
    PRUint32                    i;
    nsresult                    itemResult;
    nsresult                    result = NS_OK;

    result = mpDownloadMediaList->GetLength(&itemCount);

    /* Resume each incomplete item in list. */
    for (i = 0; (NS_SUCCEEDED(result) && (i < itemCount)); i++)
    {
        /* Get the next item. */
        itemResult = mpDownloadMediaList->GetItemByIndex
                                          (i, getter_AddRefs(pMediaItem));
        NS_ENSURE_SUCCESS(itemResult, itemResult);

        sbAutoDownloadButtonPropertyValue property(pMediaItem, nsnull, PR_TRUE);

        /* Add item to transfer queue if not complete. */
        if (property.value->GetMode() !=
            sbDownloadButtonPropertyValue::eComplete) {

            mozilla::MonitorAutoLock mon(mpDeviceMonitor);
            itemResult = AddItemToTransferQueue(mDeviceIdentifier, pMediaItem);
            if (NS_SUCCEEDED(itemResult))
                queuedCount++;
        }

    }

    /* Run the transfer queue if any items were queued. */
    if (queuedCount > 0)
        RunTransferQueue();

    return (result);
}


/*
 * SetTransferDestination
 *
 *   --> pMediaItem             Media item for which to set transfer
 *                              destination.
 *
 *   This function sets the transfer destination property for the media item
 * specified by pMediaItem.  If the transfer destination is already set, this
 * function does nothing.
 */

nsresult sbDownloadDevice::SetTransferDestination(
    nsCOMPtr<sbIMediaItem>      pMediaItem)
{
    nsString                    dstProp;
    nsCOMPtr<nsIFile>           pDstFile;
    nsCOMPtr<nsIURI>            pDstURI;
    nsCOMPtr<sbIDownloadDeviceHelper>
                                pDownloadHelper;
    nsCString                   dstSpec;
    nsresult                    propertyResult;
    nsAutoString                contentType;
    nsresult                    result = NS_OK;

    /* Do nothing if destination is already set. */
    propertyResult = pMediaItem->GetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     dstProp);
    if (NS_SUCCEEDED(propertyResult) && !dstProp.IsEmpty())
        return (result);

    /* Get the destination folder from the download helper. This will pop the */
    /* dialog to the user if needed. It will throw an error if the user       */
    /* cancels the dialog.                                                    */
    if (NS_SUCCEEDED(result))
    {
        pDownloadHelper = do_GetService
                           ("@songbirdnest.com/Songbird/DownloadDeviceHelper;1",
                            &result);
    }

    if (NS_SUCCEEDED(result))
        result = pMediaItem->GetContentType (contentType);

    if (NS_SUCCEEDED(result))
        result = pDownloadHelper->GetDownloadFolder(contentType,
                                                    getter_AddRefs(pDstFile));

    /* Create a unique local destination file object.    */
    /* note that we only record the directory, we do not */
    /* append the filename until when the file finishes  */
    /* to download                                       */
    /* Get the destination URI spec. */
    if (NS_SUCCEEDED(result))
        result = mpIOService->NewFileURI(pDstFile, getter_AddRefs(pDstURI));

    if (NS_SUCCEEDED(result))
        result = pDstURI->GetSpec(dstSpec);

    /* Set the destination property. */
    if (NS_SUCCEEDED(result))
    {
        result = pMediaItem->SetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     NS_ConvertUTF8toUTF16(dstSpec));
    }

    return (result);
}


/*
 * CancelSession
 *
 *   This function cancels the active download session and runs the transfer
 * queue.
 */

nsresult sbDownloadDevice::CancelSession()
{
    nsresult                    result = NS_OK;

    /* Shutdown the session. */
    if (mpDownloadSession)
    {
        mpDownloadSession->Shutdown();
        mpDownloadSession = nsnull;
    }

    /* Run the transfer queue. */
    RunTransferQueue();

    return (result);
}


/*
 * SessionCompleted
 *
 *   --> apDownloadSession      Completed session.
 *   --> aStatus                Final session status.
 *
 *   This function is called when the download session specified by
 * apDownloadSession completes with the final status specified by aStatus.
 */

void sbDownloadDevice::SessionCompleted(
    sbDownloadSession           *apDownloadSession,
    PRInt32                     aStatus)
{
    /* Complete session. */
    {
        /* Lock the device. */
        mozilla::MonitorAutoLock mon(mpDeviceMonitor);

        /* Deliver transfer completion callbacks. */
        DoTransferCompleteCallback(apDownloadSession->mpMediaItem, aStatus);

        /* Release the download session. */
        if (apDownloadSession == mpDownloadSession)
            mpDownloadSession = nsnull;
    }

    /* Run the transfer queue. */
    RunTransferQueue();
}


/* *****************************************************************************
 *
 * Private download device services.
 *
 ******************************************************************************/

/*
 * GetTmpFile
 *
 *   <-- ppTmpFile              Temporary file.
 *
 *   This function returns in ppTmpFile an nsIFile object for a unique temporary
 * file.  This function does not create the file.
 */

nsresult sbDownloadDevice::GetTmpFile(
    nsIFile                     **ppTmpFile)
{
    nsCOMPtr<nsIFile>           pTmpFile;
    nsString                    tmpFileName;
    PRInt32                     fileNum;
    PRBool                      exists;
    nsresult                    result = NS_OK;

    /* Get a unique temporary download file. */
    fileNum = 1;
    do
    {
        /* Start at the temporary download directory. */
        result = mpTmpDownloadDir->Clone(getter_AddRefs(pTmpFile));

        /* Append the temporary file name. */
        if (NS_SUCCEEDED(result))
        {
            tmpFileName.AssignLiteral("tmp");
            tmpFileName.AppendInt(fileNum++);
            result = pTmpFile->Append(tmpFileName);
        }

        /* Check if the file exists. */
        if (NS_SUCCEEDED(result))
            result = pTmpFile->Exists(&exists);
    }
    while (exists && NS_SUCCEEDED(result));

    /* Return results. */
    if (NS_SUCCEEDED(result))
        NS_ADDREF(*ppTmpFile = pTmpFile);

    return (result);
}


/*
 * MakeFileUnique
 *
 *   --> apFile                 File to make unique.
 *
 *   This function makes the file object specified by apFile refer to a unique,
 * non-existent file.  If the specified file does not exist, this function does
 * nothing.  Otherwise, it tries different file leaf names until a non-existent
 * file is found and sets the specified file object's leaf name accordingly.
 */

nsresult sbDownloadDevice::MakeFileUnique(
    nsIFile                     *apFile)
{
    nsCOMPtr<nsIFile>           pUniqueFile;
    nsAutoString                leafName;
    nsAutoString                uniqueLeafName;
    PRInt32                     extOffset = -1;
    nsAutoString                uniqueStr;
    PRInt32                     uniqueNum = 1;
    PRBool                      exists;
    nsresult                    result = NS_OK;

    /* Do nothing if file does not exist. */
    result = apFile->Exists(&exists);
    if (NS_FAILED(result) || !exists)
        return (result);

    /* Clone the file object. */
    if (NS_SUCCEEDED(result))
        result = apFile->Clone(getter_AddRefs(pUniqueFile));

    /* Get the file leaf name. */
    if (NS_SUCCEEDED(result))
        result = pUniqueFile->GetLeafName(leafName);
    if (NS_SUCCEEDED(result))
        extOffset = leafName.RFindChar('.');

    /* Find a non-existent file name. */
    while (NS_SUCCEEDED(result) && exists)
    {
        /* Try producing a unique string. */
        uniqueStr.AssignLiteral("_");
        uniqueStr.AppendInt(uniqueNum++);
        uniqueStr.AppendLiteral("_");

        /* Add the unique string to the file leaf name. */
        uniqueLeafName.Assign(leafName);
        if (extOffset == -1)
            uniqueLeafName.Append(uniqueStr);
        else
            uniqueLeafName.Insert(uniqueStr, extOffset);

        /* Check if the file exists. */
        result = pUniqueFile->SetLeafName(uniqueLeafName);
        if (NS_SUCCEEDED(result))
            result = pUniqueFile->Exists(&exists);

        /* Limit number of tries. */
        if (exists && (uniqueNum > 1000))
            result = NS_ERROR_FILE_TOO_BIG;
    }

    /* Update the file with a unique leaf name. */
    if (NS_SUCCEEDED(result))
        result = apFile->SetLeafName(uniqueLeafName);

    return (result);
}

/*
 * OpenDialog
 *
 *   --> aChromeURL             URL to dialog chrome.
 *   --> apDialogPB             Dialog parameter block.
 *
 *   This function opens a dialog window with the chrome specified by aChromeURL
 * and the parameter block specified by apDialogPB.
 */

nsresult sbDownloadDevice::OpenDialog(
    char                        *aChromeURL,
    nsIDialogParamBlock         *apDialogPB)
{
    nsCOMPtr<nsIWindowWatcher>  pWindowWatcher;
    nsCOMPtr<nsIDOMWindow>      pActiveWindow;
    nsCOMPtr<nsIDOMWindow>      pWindow;
    nsCOMPtr<sbIDataRemote>     pDataRemote;
    nsCAutoString               chromeFeatures;
    PRBool                      accessibilityEnabled;
    nsresult                    result = NS_OK;

    /* Get the window watcher services. */
    pWindowWatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &result);

    /* Get the active window. */
    if (NS_SUCCEEDED(result))
        result = pWindowWatcher->GetActiveWindow(getter_AddRefs(pActiveWindow));

    /* Check if accessibility is enabled. */
    if (NS_SUCCEEDED(result))
    {
        pDataRemote = do_CreateInstance
                                    ("@songbirdnest.com/Songbird/DataRemote;1",
                                     &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = pDataRemote->Init(NS_LITERAL_STRING("accessibility.enabled"),
                                   EmptyString());
    }
    if (NS_SUCCEEDED(result))
        result = pDataRemote->GetBoolValue(&accessibilityEnabled);

    /* Get the chrome feature set. */
    if (NS_SUCCEEDED(result))
    {
        chromeFeatures = NS_LITERAL_CSTRING
                                ("chrome,centerscreen,modal=yes,resizable=no");
        if (accessibilityEnabled)
            chromeFeatures.AppendLiteral(",titlebar=yes");
        else
            chromeFeatures.AppendLiteral(",titlebar=no");
    }

    /* Open the dialog. */
    if (NS_SUCCEEDED(result))
    {
        pWindowWatcher->OpenWindow(pActiveWindow,
                                   aChromeURL,
                                   nsnull,
                                   chromeFeatures.get(),
                                   apDialogPB,
                                   getter_AddRefs(pWindow));
    }

    return (result);
}


/* static */ nsresult sbDownloadDevice::GetStatusTarget(
    sbIMediaItem                *apMediaItem,
    sbIMediaItem                **apStatusTarget)
{
    NS_ASSERTION(apMediaItem, "apMediaItem is null");
    NS_ASSERTION(apStatusTarget, "apStatusTaret is null");

    nsresult rv;
    nsString target;
    rv = apMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_STATUS_TARGET),
                                  target);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 pos = target.FindChar(',');
    if (pos >= 0) {
        nsDependentSubstring mediaItemGuid(target, pos + 1);

        nsString originalItemGuid;
        rv = apMediaItem->GetGuid(originalItemGuid);
        NS_ENSURE_SUCCESS(rv, rv);

        if (originalItemGuid.Equals(mediaItemGuid)) {
          *apStatusTarget = nsnull;
          return NS_OK;
        }

        nsCOMPtr<sbILibraryManager> libraryManager =
          do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsDependentSubstring libraryGuid(target, 0, pos);
        nsCOMPtr<sbILibrary> library;
        rv = libraryManager->GetLibrary(libraryGuid, getter_AddRefs(library));
        if (rv != NS_ERROR_NOT_AVAILABLE) {
            NS_ENSURE_SUCCESS(rv, rv);
            rv = library->GetItemByGuid(mediaItemGuid, apStatusTarget);
            if (rv != NS_ERROR_NOT_AVAILABLE) {
                NS_ENSURE_SUCCESS(rv, rv);
            }
            else {
                NS_WARNING("Status target media item not found");
                *apStatusTarget = nsnull;
            }
        }
        else {
            NS_WARNING("Status target library not found");
            *apStatusTarget = nsnull;
        }
    }

    return NS_OK;
}

/* *****************************************************************************
 *******************************************************************************
 *
 * Download session.
 *
 *******************************************************************************
 ******************************************************************************/

/* *****************************************************************************
 *
 * Download session imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsIChannel.h>
#include <prprf.h>


/* *****************************************************************************
 *
 * Public download session services.
 *
 ******************************************************************************/

/*
 * sbDownloadSession
 *
 *   This method is the constructor for the download session class.
 */

sbDownloadSession::sbDownloadSession(
    sbDownloadDevice            *pDownloadDevice,
    sbIMediaItem                *pMediaItem)
:
    mpMediaItem(pMediaItem),
    mpSessionLock(nsnull),
    mpDownloadDevice(pDownloadDevice),
    mShutdown(PR_FALSE),
    mSuspended(PR_FALSE),
    mInitialProgressBytes(0),
    mLastProgressBytes(0)
{
    TRACE(("sbDownloadSession[0x%.8x] - ctor", this));
}


/*
 * ~sbDownloadSession
 *
 *   This method is the destructor for the download session class.
 */

sbDownloadSession::~sbDownloadSession()
{
    /* clean up any active downloads, if we still manage to have them */
    Shutdown();

    TRACE(("sbDownloadSession[0x%.8x] - dtor", this));
}


/*
 * Initiate
 *
 *   This function initiates the download session.
 */
nsresult sbDownloadSession::Initiate()
{
    TRACE(("sbDownloadSession[0x%.8x] - Initiate", this));
    nsCOMPtr<sbILibraryManager> pLibraryManager;
    nsCOMPtr<nsIURI>            pDstURI;
    nsString                    dstSpec;
    nsCString                   dstCSpec;
    nsCOMPtr<nsILocalFile>      pDstFile;
    nsCString                   fileName;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIStandardURL>    pStandardURL;
    nsresult                    rv;

    /* Get the library manager and utilities services. */
    mpLibraryUtils =
        do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    pLibraryManager = do_GetService
                            ("@songbirdnest.com/Songbird/library/Manager;1",
                             &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the string bundle. */
    nsCOMPtr<nsIStringBundleService>
                                pStringBundleService;

    /* Get the download device string bundle. */
    pStringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID,
                                         &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = pStringBundleService->CreateBundle
                                    (SB_STRING_BUNDLE_CHROME_URL,
                                     getter_AddRefs(mpStringBundle));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get some strings. */
    rv = mpStringBundle->GetStringFromName
                    (NS_LITERAL_STRING("device.download.complete").get(),
                     getter_Copies(mCompleteStr));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mpStringBundle->GetStringFromName
                    (NS_LITERAL_STRING("device.download.error").get(),
                     getter_Copies(mErrorStr));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get a unique temporary download file. */
    rv = mpDownloadDevice->GetTmpFile(getter_AddRefs(mpTmpFile));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Set the origin URL */
    // Check if the origin URL is already set, if not copy from ContentSrc
    // We do this so that we don't overwrite the originURL with a downloaded
    // file location, then we can still see the original originURL.
    nsAutoString currentOriginURL;
    mpMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                             currentOriginURL);
    if (currentOriginURL.IsEmpty()) {
      nsCOMPtr<nsIURI> pSrcURI;
      nsCString srcSpec;
      rv = mpMediaItem->GetContentSrc(getter_AddRefs(pSrcURI));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = pSrcURI->GetSpec(srcSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      mSrcURISpec = NS_ConvertUTF8toUTF16(srcSpec);
      rv = mpMediaItem->SetProperty
                              (NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                               mSrcURISpec);
      if (NS_FAILED(rv))
      {
        NS_WARNING("Failed to set originURL, this item may be duplicated later \
                    because it's origin cannot be tracked!");
        return rv;
      }
    }

    /* Get the status target, if any */
    rv = sbDownloadDevice::GetStatusTarget(mpMediaItem,
                                               getter_AddRefs(mpStatusTarget));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the destination download (directory) URI. */
    rv = mpMediaItem->GetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                 dstSpec);
    if (NS_SUCCEEDED(rv) && dstSpec.IsEmpty())
        rv = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewURI(getter_AddRefs(mpDstURI), dstSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    {
      /* Keep a reference to the nsIFile pointing at the directory */
      nsCOMPtr<nsIFileURL>        pFileURL;
      nsCOMPtr<nsIFile>           pFile;
      pFileURL = do_QueryInterface(mpDstURI, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = pFileURL->GetFile(getter_AddRefs(pFile));
      NS_ENSURE_SUCCESS(rv, rv);
      pDstFile = do_QueryInterface(pFile, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    /* set the destination directory */
    rv = pDstFile->Clone(getter_AddRefs(mpDstFile));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get the destination library. */
    rv = pLibraryManager->GetMainLibrary(getter_AddRefs(mpDstLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Get a URI for the source. */
    rv = mpMediaItem->GetContentSrc(getter_AddRefs(mpSrcURI));
    NS_ENSURE_SUCCESS(rv, rv);

    /* Start download */
    rv = SetUpRequest();
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create the idle timer */
    mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create the progress timer */
    mProgressTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
}

/*
 * SetUpRequest
 *
 *   Starts the download or resumes a previously cancelled download.
 */

nsresult sbDownloadSession::SetUpRequest()
{
    nsresult rv;

    /* Create a persistent download web browser. */
    mpWebBrowser = do_CreateInstance
                        ("@mozilla.org/embedding/browser/nsWebBrowserPersist;1",
                         &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Create channel to download from. */
    nsCOMPtr<nsIInterfaceRequestor> ir(do_QueryInterface(mpWebBrowser));
    rv = NS_NewChannel(getter_AddRefs(mpRequest), mpSrcURI, nsnull, nsnull, ir);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mEntityID.IsEmpty())
    {
        /* We are resuming a download, initialize the channel to resume at the correct position */
        nsCOMPtr<nsIFile> clone;
        if (NS_FAILED(mpTmpFile->Clone(getter_AddRefs(clone))) ||
            NS_FAILED(clone->GetFileSize(&mInitialProgressBytes)))
        {
            NS_WARNING("Restarting download instead of resuming: failed \
                        determining how much is already downloaded!");
            mInitialProgressBytes = 0;
        }

        if (mInitialProgressBytes)
        {
            nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(mpRequest));
            if (resumableChannel &&
                NS_SUCCEEDED(resumableChannel->ResumeAt(mInitialProgressBytes, mEntityID)))
            {
                /* We are resuming download, make sure to append to the existing file */
                rv = mpWebBrowser->SetPersistFlags(nsIWebBrowserPersist::PERSIST_FLAGS_APPEND_TO_FILE);
                NS_ENSURE_SUCCESS(rv, rv);
            }
            else
            {
                NS_WARNING("Restarting download instead of resuming: \
                            nsIResumableChannel.ResumeAt failed");
                mInitialProgressBytes = 0;
            }
        }
    }

    /* Set up the listener. */
    mLastUpdate = PR_Now();
    rv = mpWebBrowser->SetProgressListener(this);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Initiate the download. */
    rv = mpWebBrowser->SaveChannel(mpRequest, mpTmpFile);
    if (NS_FAILED(rv))
    {
        NS_WARNING("Failed initiating download");
        mpWebBrowser->SetProgressListener(nsnull);
    }
    return rv;
}

/*
 * Suspend
 *
 *   This function suspends the download session.
 */

nsresult sbDownloadSession::Suspend()
{
    TRACE(("sbDownloadSession[0x%.8x] - Suspend", this));
    NS_ENSURE_STATE(!mShutdown);

    nsresult                    rv;

    /* Lock the session. */
    mozilla::MutexAutoLock lock(mpSessionLock);

    /* Do nothing if already suspended. */
    if (mSuspended)
        return NS_OK;

    /* Suspend request. */
    mEntityID.Truncate();
    nsCOMPtr<nsIResumableChannel> resumable(do_QueryInterface(mpRequest));
    if (resumable)
        resumable->GetEntityID(mEntityID);

    if (!mEntityID.IsEmpty())
    {
        /* Channel is resumable, simply cancel current request */
        rv = mpWebBrowser->Cancel(NS_BINDING_ABORTED);
        NS_ENSURE_SUCCESS(rv, rv);

        mpRequest = nsnull;
        mpWebBrowser->SetProgressListener(nsnull);
        mpWebBrowser = nsnull;
    }
    else
    {
        /* Channel not resumable, try suspending */
        rv = mpRequest->Suspend();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Update the download media item download button property. */
    sbAutoDownloadButtonPropertyValue property(mpMediaItem, mpStatusTarget);
    property.value->SetMode(sbDownloadButtonPropertyValue::ePaused);
    
    StopTimers();

    /* Mark session as suspended. */
    mSuspended = PR_TRUE;

    return NS_OK;
}


/*
 * Resume
 *
 *   This function resumes the download session.
 */

nsresult sbDownloadSession::Resume()
{
    TRACE(("sbDownloadSession[0x%.8x] - Resume", this));
    NS_ENSURE_STATE(!mShutdown);

    nsresult                    rv;

    /* Lock the session. */
    mozilla::MutexAutoLock lock(mpSessionLock);

    /* Do nothing if not suspended. */
    if (!mSuspended)
        return NS_OK;

    /* Resume the request. */
    if (!mEntityID.IsEmpty())
    {
        rv = SetUpRequest();
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
        rv = mpRequest->Resume();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Update the download media item download button property. */
    sbAutoDownloadButtonPropertyValue property(mpMediaItem, mpStatusTarget);
    property.value->SetMode(sbDownloadButtonPropertyValue::eDownloading);

    StartTimers();

    /* Mark session as not suspended. */
    mSuspended = PR_FALSE;

    return NS_OK;
}


/*
 * Shutdown
 *
 *   This function shuts down the download session.
 */

void sbDownloadSession::Shutdown()
{
    TRACE(("sbDownloadSession[0x%.8x] - Shutdown", this));

    // If shutdown is called, this means the library is shutting down and
    // we shouldn't be touching the library any more
    mpMediaItem = nsnull;

    /* Lock the session. */
    mozilla::MutexAutoLock lock(mpSessionLock);

    /* Stop the timers */
    StopTimers();

    // Keep a ref to ourselves since the clean up code will release us too
    // early
    nsRefPtr<sbDownloadSession> kungFuDeathGrip(this);

    /* Mark session for shutdown. */
    mShutdown = PR_TRUE;

    mpRequest = nsnull;

    if (mpWebBrowser) {
      mpWebBrowser->CancelSave();
      mpWebBrowser->SetProgressListener(nsnull);
      mpWebBrowser = nsnull;
    }

}


/*
 * IsSuspended
 *
 *   <-- PR_TRUE                Download session is suspended.
 *
 *   This function returns PR_TRUE if the download session is suspended;
 * otherwise, it returns PR_FALSE.
 */

PRBool sbDownloadSession::IsSuspended()
{
    /* Lock the session. */
    mozilla::MutexAutoLock lock(mpSessionLock);

    return mSuspended;
}


/* *****************************************************************************
 *
 * Download session nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDownloadSession,
                              nsIWebProgressListener,
                              nsITimerCallback)

/* *****************************************************************************
 *
 * Download session nsIWebProgressListener services.
 *
 ******************************************************************************/

/**
 * Notification indicating the state has changed for one of the requests
 * associated with aWebProgress.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification
 * @param aRequest
 *        The nsIRequest that has changed state.
 * @param aStateFlags
 *        Flags indicating the new state.  This value is a combination of one
 *        of the State Transition Flags and one or more of the State Type
 *        Flags defined above.  Any undefined bits are reserved for future
 *        use.
 * @param aStatus
 *        Error status code associated with the state change.  This parameter
 *        should be ignored unless aStateFlags includes the STATE_STOP bit.
 *        The status code indicates success or failure of the request
 *        associated with the state change.  NOTE: aStatus may be a success
 *        code even for server generated errors, such as the HTTP 404 error.
 *        In such cases, the request itself should be queried for extended
 *        error information (e.g., for HTTP requests see nsIHttpChannel).
 */

NS_IMETHODIMP sbDownloadSession::OnStateChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRUint32                    aStateFlags,
    nsresult                    aStatus)
{
    TRACE(("sbDownloadSession[0x%.8x] - OnStateChange "
           "aWebProgress 0x%.8x aRequest 0x%.8x "
           "aStateFlags 0x%.8x aStatus 0x%.8x",
           this, aWebProgress, aRequest, aStateFlags, aStatus));

    PRBool                      complete = PR_FALSE;
    nsresult                    status = aStatus;
    nsresult                    result = NS_OK;

    // Keep a ref to ourselves since the clean up code will release us too
    // early
    nsRefPtr<sbDownloadSession> kungFuDeathGrip(this);

    /* Process state change with the session locked. */
    {
        /* Lock the session. */
        mozilla::MutexAutoLock lock(mpSessionLock);

        if (aStateFlags & STATE_START) {
          /* start the timer */
          StartTimers();
        } else if (aStateFlags & STATE_STOP) {
          /* stop the timer */
          StopTimers();
        }

        /* Do nothing if download has not stopped or if shutting down. */
        if (!(aStateFlags & STATE_STOP) || mShutdown)
            return NS_OK;

        /* Do nothing on abort. */
        /* XXXeps This is a workaround for the fact that shutdown */
        /* isn't called until after channel is aborted.          */
        if (status == NS_ERROR_ABORT)
            return NS_OK;

        /* Check HTTP response status. */
        if (NS_SUCCEEDED(status))
        {
            nsCOMPtr<nsIHttpChannel>    pHttpChannel;
            PRBool                      requestSucceeded;

            /* Try to get HTTP channel. */
            pHttpChannel = do_QueryInterface(aRequest, &result);

            /* Check if request succeeded. */
            if (NS_SUCCEEDED(result))
                result = pHttpChannel->GetRequestSucceeded(&requestSucceeded);
            if (NS_SUCCEEDED(result) && !requestSucceeded)
                status = NS_ERROR_UNEXPECTED;

	    /* Don't propagate errors from here. */
            result = NS_OK;
        }

        /* Complete the transfer on success. */
        if (NS_SUCCEEDED(result) && NS_SUCCEEDED(status))
        {
            result = CompleteTransfer(aRequest);
            if (NS_SUCCEEDED(result))
                complete = PR_TRUE;
        }
        
        if (complete) {
            // Set the progress to complete.
            sbAutoDownloadButtonPropertyValue property(mpMediaItem,
                                                       mpStatusTarget);
            property.value->SetMode(sbDownloadButtonPropertyValue::eComplete);
        } else {
            // Set the progress to error.
            sbAutoDownloadButtonPropertyValue property(mpMediaItem,
                                                       mpStatusTarget);
            property.value->SetMode(sbDownloadButtonPropertyValue::eFailed);
        }

        /* Set the final download status. */
        nsAutoString statusStr;
        if (complete)
            statusStr.Assign(mCompleteStr);
        else
            statusStr.Assign(mErrorStr);
        mpMediaItem->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             statusStr);
        if (mpStatusTarget)
        {
            mpStatusTarget->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             statusStr);
        }
    }

    /* Send notification of session completion.  Do this */
    /* outside of session lock to prevent deadlock.      */
    mpDownloadDevice->SessionCompleted(this, status);

    /* Clean up within a session lock. */
    {
        /* Lock the session. */
        mozilla::MutexAutoLock lock(mpSessionLock);

        /* Clean up. */
        mpRequest = nsnull;
        if (mpWebBrowser)
        {
            mpWebBrowser->CancelSave();
            mpWebBrowser->SetProgressListener(nsnull);
        }
        mpWebBrowser = nsnull;
        mpMediaItem = nsnull;
    }

    return NS_OK;
}


/**
 * Notification that the progress has changed for one of the requests
 * associated with aWebProgress.  Progress totals are reset to zero when all
 * requests in aWebProgress complete (corresponding to onStateChange being
 * called with aStateFlags including the STATE_STOP and STATE_IS_WINDOW
 * flags).
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new progress.
 * @param aCurSelfProgress
 *        The current progress for aRequest.
 * @param aMaxSelfProgress
 *        The maximum progress for aRequest.
 * @param aCurTotalProgress
 *        The current progress for all requests associated with aWebProgress.
 * @param aMaxTotalProgress
 *        The total progress for all requests associated with aWebProgress.
 *
 * NOTE: If any progress value is unknown, or if its value would exceed the
 * maximum value of type long, then its value is replaced with -1.
 *
 * NOTE: If the object also implements nsIWebProgressListener2 and the caller
 * knows about that interface, this function will not be called. Instead,
 * nsIWebProgressListener2::onProgressChange64 will be called.
 */

NS_IMETHODIMP sbDownloadSession::OnProgressChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRInt32                     aCurSelfProgress,
    PRInt32                     aMaxSelfProgress,
    PRInt32                     aCurTotalProgress,
    PRInt32                     aMaxTotalProgress)
{
    TRACE(("sbDownloadSession[0x%.8x] - OnProgressChange "
           "aWebProgress 0x%.8x aRequest 0x%.8x "
           "aCurSelfProgress %d aMaxSelfProgress %d "
           "aCurTotalProgress %d aMaxTotalProgress %d",
           this, aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress,
           aCurTotalProgress, aMaxTotalProgress));

    /* we got some progress, let's reset the idle timer */
    ResetTimers();

    /* Update progress. */
    UpdateProgress(aCurSelfProgress, aMaxSelfProgress);

    return NS_OK;
}


/**
 * Called when the location of the window being watched changes.  This is not
 * when a load is requested, but rather once it is verified that the load is
 * going to occur in the given window.  For instance, a load that starts in a
 * window might send progress and status messages for the new site, but it
 * will not send the onLocationChange until we are sure that we are loading
 * this new page here.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The associated nsIRequest.  This may be null in some cases.
 * @param aLocation
 *        The URI of the location that is being loaded.
 */

NS_IMETHODIMP sbDownloadSession::OnLocationChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    nsIURI                      *aLocation)
{
    TRACE(("sbDownloadSession[0x%.8x] - OnLocationChange "
           "aWebProgress 0x%.8x aRequest 0x%.8x aLocation 0x%.8x",
           this, aWebProgress, aRequest, aLocation));

    return (NS_OK);
}


/**
 * Notification that the status of a request has changed.  The status message
 * is intended to be displayed to the user (e.g., in the status bar of the
 * browser).
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new status.
 * @param aStatus
 *        This value is not an error code.  Instead, it is a numeric value
 *        that indicates the current status of the request.  This interface
 *        does not define the set of possible status codes.  NOTE: Some
 *        status values are defined by nsITransport and nsISocketTransport.
 * @param aMessage
 *        Localized text corresponding to aStatus.
 */

NS_IMETHODIMP sbDownloadSession::OnStatusChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    nsresult                    aStatus,
    const PRUnichar             *aMessage)
{
    TRACE(("sbDownloadSession[0x%.8x] - OnStatusChange "
           "aWebProgress 0x%.8x aRequest 0x%.8x aStatus 0x%.8x aMessage '%s'",
           this, aWebProgress, aRequest, aStatus, aMessage));

    return (NS_OK);
}


/**
 * Notification called for security progress.  This method will be called on
 * security transitions (eg HTTP -> HTTPS, HTTPS -> HTTP, FOO -> HTTPS) and
 * after document load completion.  It might also be called if an error
 * occurs during network loading.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new security state.
 * @param aState
 *        A value composed of the Security State Flags and the Security
 *        Strength Flags listed above.  Any undefined bits are reserved for
 *        future use.
 *
 * NOTE: These notifications will only occur if a security package is
 * installed.
 */

NS_IMETHODIMP sbDownloadSession::OnSecurityChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRUint32                    aState)
{
    TRACE(("sbDownloadSession[0x%.8x] - OnSecurityChange "
           "aWebProgress 0x%.8x aRequest 0x%.8x aState 0x%.8x",
           this, aWebProgress, aRequest, aState));

    return (NS_OK);
}


/* *****************************************************************************
 *
 * Download session nsITimerCallback services.
 *
 ******************************************************************************/
NS_IMETHODIMP sbDownloadSession::Notify(nsITimer* aTimer)
{
    if (aTimer == mIdleTimer) {
      /* Once the idle timer has timed out we should abort the request.
       * Our onStateChange handler will take care of reflecting the state
       * into the UI. */
      mpRequest->Cancel(NS_BINDING_ABORTED);
    } else if (aTimer == mProgressTimer) {
      UpdateProgress(mLastProgressBytes, mLastProgressBytesMax);
    }
    return NS_OK;
}

/* *****************************************************************************
 *
 * Private download session services.
 *
 ******************************************************************************/

/*
 * CompleteTransfer
 *
 *   This function completes transfer of the download file into the destination
 * library.
 */

nsresult sbDownloadSession::CompleteTransfer(nsIRequest* aRequest)
{
    nsCOMPtr<nsIFile>           pFileDir;
    nsString                    fileName;
    nsCString                   srcSpec;
    nsCOMPtr<nsIURI>            pSrcURI;
    nsCOMPtr<sbIMediaList>      pDstMediaList;
    nsresult                    result = NS_OK;
    PRBool                      bIsDirectory;
    PRBool                      bChangedDstFile = PR_FALSE;

    // Get the the file name, if any, supplied in the MIME header:
    nsCString mimeFilename;
    nsCOMPtr<nsIHttpChannel> httpChannel =
      do_QueryInterface(aRequest, &result);
    if (NS_SUCCEEDED(result)) {
      nsCAutoString contentDisposition;
      result = httpChannel->GetResponseHeader(
                              NS_LITERAL_CSTRING("content-disposition"),
                              contentDisposition);
      if (NS_SUCCEEDED(result)) {
        mimeFilename = GetContentDispositionFilename(contentDisposition);
      }
    }

    /* Check if the destination is a directory.  If the destination */
    /* does not exist, assume it's a file about to be created.      */
    result = mpDstFile->IsDirectory(&bIsDirectory);
    if (result == NS_ERROR_FILE_NOT_FOUND || result == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
    {
        bIsDirectory = PR_FALSE;
        result = NS_OK;
    }
    NS_ENSURE_SUCCESS(result, result);

    if (bIsDirectory)
    {
        // the destination is a directory; make a complete filename.
        // Try the MIME file name first. If there isn't any, we'll fall back
        // to the source url filename.

        nsCString escFileName(mimeFilename);
        if (escFileName.IsEmpty()) {
          // make a filename based on the source URL
          nsCOMPtr<nsIURI> pFinalSrcURI;

          nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &result);
          NS_ENSURE_SUCCESS(result, result);

          result = channel->GetURI(getter_AddRefs(pFinalSrcURI));
          NS_ENSURE_SUCCESS(result, result);

          /* convert the uri into a URL */
          nsCOMPtr<nsIURL> pFinalSrcURL = do_QueryInterface(pFinalSrcURI, &result);
          NS_ENSURE_SUCCESS(result, result);

          result = pFinalSrcURL->GetFileName(escFileName);
          NS_ENSURE_SUCCESS(result, result);
        }

        /* Get the unescaped file name from the URL. */
        nsCOMPtr<nsINetUtil> netUtil;
        netUtil = do_GetService("@mozilla.org/network/util;1", &result);
        NS_ENSURE_SUCCESS(result, result);
        nsCString leafCName;
        result = netUtil->UnescapeString(escFileName,
          nsINetUtil::ESCAPE_URL_SKIP_CONTROL,
          leafCName);
        NS_ENSURE_SUCCESS(result, result);
        
        /* convert the leaf name to UTF 16 (since it can be invalid UTF8) */
        nsString leafName = NS_ConvertUTF8toUTF16(leafCName);
        if (leafName.IsEmpty()) {
          // not valid UTF8; use the escaped version instead :(
          leafName.Assign(NS_ConvertUTF8toUTF16(escFileName));
          if (leafName.IsEmpty()) {
            // still invalid; hard code something crappy
            leafName.AssignLiteral("unnamed");
          }
        }

        /* strip out characters not valid in file names */
        nsString illegalChars(NS_ConvertASCIItoUTF16(FILE_ILLEGAL_CHARACTERS));
        illegalChars.AppendLiteral(FILE_PATH_SEPARATOR);
        ReplaceChars(leafName, illegalChars, PRUnichar('_'));

        /* append to the path */
        result = mpDstFile->Append(leafName);
        NS_ENSURE_SUCCESS(result, result);

        /* ensure the filename is unique */
        result = sbDownloadDevice::MakeFileUnique(mpDstFile);
        NS_ENSURE_SUCCESS(result, result);

        bChangedDstFile = PR_TRUE;
    }

    // If the file name has no extension, use the one, if any, supplied
    // in the MIME header:
    result = mpDstFile->GetLeafName(fileName);
    NS_ENSURE_SUCCESS(result, result);
    if (fileName.RFindChar('.') == -1) {
      PRInt32 extension = mimeFilename.RFindChar('.');
      if (extension != -1) {
        fileName.Append(
          NS_ConvertUTF8toUTF16(Substring(mimeFilename, extension)));
        result = mpDstFile->SetLeafName(fileName);
        NS_ENSURE_SUCCESS(result, result);

        bChangedDstFile = PR_TRUE;
      }
    }

    if (bChangedDstFile) {
        /* Get the destination URI spec. */
        nsCOMPtr<nsIURI> pDstURI;
        result = mpLibraryUtils->GetFileContentURI(mpDstFile,
                                                   getter_AddRefs(mpDstURI));
        NS_ENSURE_SUCCESS(result, result);
        
        nsCString dstCSpec;
        result = mpDstURI->GetSpec(dstCSpec);
        NS_ENSURE_SUCCESS(result, result);

        /* Set the destination property. */
        mDstURISpec = NS_ConvertUTF8toUTF16(dstCSpec);
        result = mpMediaItem->SetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     mDstURISpec);
        NS_ENSURE_SUCCESS(result, result);
    }

    /* Save the content URL. */
    if (NS_SUCCEEDED(result))
      result = mpMediaItem->GetContentSrc(getter_AddRefs(pSrcURI));
    if (NS_SUCCEEDED(result))
      result = pSrcURI->GetSpec(srcSpec);

    /* Update the download media item content source property. */
    if (NS_SUCCEEDED(result)) {
      result = mpMediaItem->SetContentSrc(mpDstURI);
    }

    /* Add the download media item to the destination library. */
    if (NS_SUCCEEDED(result))
      pDstMediaList = do_QueryInterface(mpDstLibrary, &result);
    if (NS_SUCCEEDED(result))
      result = pDstMediaList->Add(mpMediaItem);

    /* Move the temporary download file to the final location. */
    if (NS_SUCCEEDED(result))
        result = mpDstFile->GetParent(getter_AddRefs(pFileDir));
    if (NS_SUCCEEDED(result)) {
      nsRefPtr<sbDownloadSessionMoveHandler> moveHandler;
      moveHandler = new sbDownloadSessionMoveHandler(mpTmpFile, 
                                                     pFileDir, 
                                                     fileName,
                                                     mpMediaItem);
      NS_ENSURE_TRUE(moveHandler, NS_ERROR_OUT_OF_MEMORY);

      result = mpDownloadDevice->mFileMoveThreadPool->Dispatch(moveHandler,
                                                               nsIEventTarget::DISPATCH_NORMAL);
    }

    /* Update the web library with the local downloaded file URL. */
    if (NS_SUCCEEDED(result))
    {
        nsRefPtr<WebLibraryUpdater> pWebLibraryUpdater;
        nsCOMPtr<sbIMediaList>      pWebMediaList;

        /* Get the web library media list. */
        if (NS_SUCCEEDED(result))
        {
            pWebMediaList = do_QueryInterface(mpDownloadDevice->mpWebLibrary,
                                              &result);
        }

        /* Update the web library. */
        if (NS_SUCCEEDED(result))
        {
            pWebLibraryUpdater = new WebLibraryUpdater(this);
            if (!pWebLibraryUpdater)
                result = NS_ERROR_OUT_OF_MEMORY;
        }
        if (NS_SUCCEEDED(result))
        {
            result = pWebMediaList->EnumerateItemsByProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     NS_ConvertUTF8toUTF16(srcSpec),
                                     pWebLibraryUpdater,
                                     sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        }

        /* Don't propagate errors from here. */
        result = NS_OK;
    }

    return (result);
}


/*
 * UpdateDstLibraryMetadata
 *
 *   This function updates the download media item metadata in the destination
 * library.  If the metadata is already set in the media item, this function
 * does nothing.
 */

nsresult sbDownloadSession::UpdateDstLibraryMetadata()
{
    nsCOMPtr<sbIMediaList>      pDstMediaList;
    nsCString                   dstSpec;
    nsRefPtr<LibraryMetadataUpdater> 
                                pLibraryMetadataUpdater;
    nsString                    durationStr;
    PRInt32                     duration = 0;
    PRBool                      updateDstLibraryMetadata = PR_TRUE;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Check if the download media item has metadata. */
    result1 = mpMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                                       durationStr);
    if (NS_SUCCEEDED(result1) && durationStr.IsEmpty())
        result = NS_ERROR_FAILURE;

    if (NS_SUCCEEDED(result1))
        duration = durationStr.ToInteger(&result1);

    if (NS_SUCCEEDED(result1) && (duration > 0))
        updateDstLibraryMetadata = PR_FALSE;

    /* Update the destination library metadata if needed. */
    if (updateDstLibraryMetadata)
    {
        /* Get the destination URI spec. */
        result = mpDstURI->GetSpec(dstSpec);

        /* Create a library metadata updater. */
        if (NS_SUCCEEDED(result))
        {
            pLibraryMetadataUpdater = new LibraryMetadataUpdater();
            if (!pLibraryMetadataUpdater)
                result = NS_ERROR_OUT_OF_MEMORY;
        }

        /* Update the download media item metadata in the library. */
        if (NS_SUCCEEDED(result))
            pDstMediaList = do_QueryInterface(mpDstLibrary, &result);
        if (NS_SUCCEEDED(result))
        {
            result = pDstMediaList->EnumerateItemsByProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     NS_ConvertUTF8toUTF16(dstSpec),
                                     pLibraryMetadataUpdater,
                                     sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        }
    }

    return (result);
}


/*
 * UpdateProgress
 *
 *   --> aProgress              Current progress value.
 *   --> aProgressMax           Maximum progress value.
 *
 *   This function updates the download session progress status with the
 * progress values specified by aProgress and aProgressMax.
 */

void sbDownloadSession::UpdateProgress(
    PRUint64                    aProgress,
    PRUint64                    aProgressMax)
{
    /* Correct progress values in case we are resuming a download */
    aProgress += mInitialProgressBytes;
    aProgressMax += mInitialProgressBytes;

    /* Update the download button property. */
    sbAutoDownloadButtonPropertyValue property(mpMediaItem, mpStatusTarget);
    property.value->SetMode(sbDownloadButtonPropertyValue::eDownloading);
    property.value->SetCurrent(aProgress);
    property.value->SetTotal(aProgressMax);

    /* Update the download details. */
    UpdateDownloadDetails(aProgress, aProgressMax);
}


/*
 * UpdateDownloadDetails
 *
 *   --> aProgress              Current progress value.
 *   --> aProgressMax           Maximum progress value.
 *
 *   This function updates the download details with the download session
 * progress values specified by aProgress and aProgressMax.
 */

void sbDownloadSession::UpdateDownloadDetails(
    PRUint64                    aProgress,
    PRUint64                    aProgressMax)
{
    nsAutoString                progressStr;
    PRTime                      now;
    PRUint64                    elapsedUSecs;
    PRUint32                    remainingSecs;
    nsresult                    rv;

    /* Determine the elapsed time since the last update. */
    now = PR_Now();
    elapsedUSecs = now - mLastUpdate;

    /* Limit the update rate. */
    if (   mLastUpdate
        && (elapsedUSecs < (  SB_DOWNLOAD_PROGRESS_UPDATE_PERIOD_MS
                            * PR_USEC_PER_MSEC)))
    {
        return;
    }

    /* Update the download rate. */
    UpdateDownloadRate(aProgress, elapsedUSecs);

    /* Compute the remaining number of seconds to download. */
    if (mRate)
    {
        remainingSecs = (PRUint32)
                    ((double(aProgressMax) - double(aProgress)) / mRate + 0.5);
    }
    else
    {
        remainingSecs = 0;
    }

    /* Produce the formatted progress string. */
    rv = FormatProgress(progressStr,
                        aProgress,
                        aProgressMax,
                        mRate,
                        remainingSecs);
   if (NS_FAILED(rv))
       progressStr.AssignLiteral("???");

    /* Update the download details property.  Nothing to do on error. */
    mpMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             progressStr);
    if (mpStatusTarget)
    {
        mpStatusTarget->SetProperty
                            (NS_LITERAL_STRING(SB_PROPERTY_DOWNLOAD_DETAILS),
                             progressStr);
    }

    /* Save update state. */
    mLastUpdate = now;
    mLastProgressBytes = aProgress;
    mLastProgressBytesMax = aProgressMax;
}


/*
 * UpdateDownloadRate
 *
 *   --> aProgress              Number of bytes downloaded.
 *   --> aElapsedUSecs          Number of microseconds elapsed since last
 *                              update.
 *
 *   This function updates the download rate computed by the number of bytes
 * downloaded specified by aProgress and the elapsed time since the last update
 * specified by aElapsedUSecs.
 */

void sbDownloadSession::UpdateDownloadRate(
    PRUint64                    aProgress,
    PRUint64                    aElapsedUSecs)
{
    double                      elapsedSecs;
    PRUint64                    dlByteCount;
    double                      rate;

    /* Get the number of elapsed seconds. */
    /* Do nothing if no time has elapsed. */
    elapsedSecs = double(aElapsedUSecs) / PR_USEC_PER_SEC;
    if (elapsedSecs <= 0.0)
        return;

    /* Compute the current download rate. */
    dlByteCount = aProgress - mLastProgressBytes;
    rate = double(dlByteCount) / elapsedSecs;

    /* Calculate smoothed download rate average. */
    if (mLastProgressBytes)
        mRate = 0.9 * mRate + 0.1 * rate;
    else
        mRate = rate;
}


/*
 * FormatProgress
 *
 *   <-- aProgressStr           Formatted progress string.
 *   --> aProgress              Current progress value.
 *   --> aProgressMax           Maximum progress value.
 *   --> aRate                  Download rate.
 *   --> aRemSeconds            Reaminig download seconds.
 *
 *   This function returns in aProgressStr a formatted string containing the
 * progress values specified by aProgress, aProgressMax, aRate, and aRemSeconds.
 */

nsresult sbDownloadSession::FormatProgress(
    nsString                    &aProgressStr,
    PRUint64                    aProgress,
    PRUint64                    aProgressMax,
    double                      aRate,
    PRUint32                    aRemSeconds)
{
    nsAutoString                byteProgressStr;
    nsAutoString                rateStr;
    nsAutoString                timeStr;
    const PRUnichar             *stringList[3];
    nsresult                    rv;

    /* Format byte progress. */
    rv = FormatByteProgress(byteProgressStr, aProgress, aProgressMax);
    NS_ENSURE_SUCCESS(rv, rv);
    stringList[0] = byteProgressStr.get();

    /* Format download rate. */
    rv = FormatRate(rateStr, aRate);
    NS_ENSURE_SUCCESS(rv, rv);
    stringList[1] = rateStr.get();

    /* Format remaining download time. */
    rv = FormatTime(timeStr, aRemSeconds);
    NS_ENSURE_SUCCESS(rv, rv);
    stringList[2] = timeStr.get();

    /* Format download progress. */
    rv = mpStringBundle->FormatStringFromName
                    (NS_LITERAL_STRING("device.download.statusFormat").get(),
                     stringList,
                     3,
                     getter_Copies(aProgressStr));
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/*
 * FormatRate
 *
 *   <-- aRateStr               Formatted rate string.
 *   --> aRate                  Download rate.
 *
 *   This function returns in aRateStr a formatted string containing the
 * download rate specified by aRate.
 */

nsresult sbDownloadSession::FormatRate(
    nsString                    &aRateStr,
    double                      aRate)
{
    char                        rateStr[32];

    /* Format the download rate. */
    PR_snprintf(rateStr, sizeof(rateStr), "%.1f", aRate / 1024.0 + 0.05);
    aRateStr.AssignLiteral(rateStr);

    return (NS_OK);
}


/*
 * FormatByteProgress
 *
 *   <-- aBytesProgressStr      Formatted byte progress string.
 *   --> aBytes                 Number of bytes downloaded.
 *   --> aBytesMax              Maximum number of download bytes.
 *
 *   This function returns in aBytesProgressStr a formatted string containing
 * the download byte progress values specified by aBytes and aBytesMax.
 */

nsresult sbDownloadSession::FormatByteProgress(
    nsString                    &aByteProgressStr,
    PRUint64                    aBytes,
    PRUint64                    aBytesMax)
{
    double                      bytesKB,
                                bytesMB;
    double                      bytesMaxKB,
                                bytesMaxMB;
    double                      bytesVal;
    double                      bytesMaxVal;
    char                        bytesValStr[32];
    char                        bytesMaxValStr[32];
    nsAutoString                bytesValNSStr;
    nsAutoString                bytesMaxValNSStr;
    nsAutoString                stringName;
    const PRUnichar             *stringList[2];
    nsresult                    rv;

    /* Get the number of download kilobytes and megabytes. */
    bytesKB = double(aBytes) / 1024.0;
    bytesMB = bytesKB / 1024.0;

    /* Get the number of max download kilobytes and megabytes. */
    bytesMaxKB = double(aBytesMax) / 1024.0;
    bytesMaxMB = bytesMaxKB / 1024.0;

    /* Determine format of byte progress status. */
    if (bytesMB >= 1.0)
    {
        stringName.AssignLiteral("device.download.statusFormatMBMB");
        bytesVal = bytesMB;
        bytesMaxVal = bytesMaxMB;
    }
    else if (bytesMaxMB >= 1.0)
    {
        stringName.AssignLiteral("device.download.statusFormatKBMB");
        bytesVal = bytesKB;
        bytesMaxVal = bytesMaxMB;
    }
    else
    {
        stringName.AssignLiteral("device.download.statusFormatKBKB");
        bytesVal = bytesKB;
        bytesMaxVal = bytesMaxKB;
    }

    /* Format the download bytes status. */
    PR_snprintf(bytesValStr, sizeof(bytesValStr), "%.1f", bytesVal);
    bytesValNSStr.AssignLiteral(bytesValStr);
    stringList[0] = bytesValNSStr.get();

    /* Format the max download bytes status. */
    PR_snprintf(bytesMaxValStr, sizeof(bytesMaxValStr), "%.1f", bytesMaxVal);
    bytesMaxValNSStr.AssignLiteral(bytesMaxValStr);
    stringList[1] = bytesMaxValNSStr.get();

    /* Format the download progress status. */
    rv = mpStringBundle->FormatStringFromName(stringName.get(),
                                              stringList,
                                              2,
                                              getter_Copies(aByteProgressStr));
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/*
 * FormatTime
 *
 *   <-- aTimeStr               Formatted time string.
 *   --> aSeconds               Number of seconds.
 *
 *   This function returns in aTimeStr a formatted time string for the number of
 * seconds specified by aSeconds.
 */

nsresult sbDownloadSession::FormatTime(
    nsString                    &aTimeStr,
    PRUint32                    aSeconds)
{
    nsAutoString                stringName;
    PRUint32                    hours,
                                minutes,
                                seconds;
    nsAutoString                hoursStr,
                                minutesStr,
                                secondsStr;
    const PRUnichar             *stringList[3];
    nsresult                    rv;

    /* Initialize the time seconds field. */
    seconds = aSeconds;

    /* Extract the hours field. */
    hours = aSeconds / 3600;
    hoursStr.AppendInt(hours);
    seconds -= hours * 3600;

    /* Extract the minutes field. */
    minutes = aSeconds / 60;
    if (hours && minutes < 10)
        minutesStr.AssignLiteral("0");
    minutesStr.AppendInt(minutes);
    seconds -= minutes * 60;

    /* Extract the seconds field. */
    if (seconds < 10)
        secondsStr.AssignLiteral("0");
    secondsStr.AppendInt(seconds);

    /* Determine the time format. */
    if (hours)
    {
        stringName.AssignLiteral("device.download.longTimeFormat");
        stringList[0] = hoursStr.get();
        stringList[1] = minutesStr.get();
        stringList[2] = secondsStr.get();
    }
    else
    {
        stringName.AssignLiteral("device.download.shortTimeFormat");
        stringList[0] = minutesStr.get();
        stringList[1] = secondsStr.get();
    }

    /* Format the time string. */
    rv = mpStringBundle->FormatStringFromName(stringName.get(),
                                              stringList,
                                              3,
                                              getter_Copies(aTimeStr));
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/**
 * \brief Start the idle and progress timers.
 */
nsresult sbDownloadSession::StartTimers()
{
    mProgressTimer->Cancel();
    mProgressTimer->InitWithCallback(this, SB_DOWNLOAD_PROGRESS_TIMER_MS,
        nsITimer::TYPE_REPEATING_SLACK);

    mIdleTimer->Cancel();
    mIdleTimer->InitWithCallback(this, SB_DOWNLOAD_IDLE_TIMEOUT_MS,
        nsITimer::TYPE_ONE_SHOT);

    return NS_OK;
}


/**
 * \brief Stop the idle and progress timers.
 */
nsresult sbDownloadSession::StopTimers()
{
  if(mProgressTimer) {
    mProgressTimer->Cancel();
  }
  
  if(mIdleTimer) {
    mIdleTimer->Cancel();
  }
  
  return NS_OK;
}


/**
 * \brief Reset the idle and progress timers.
 */
nsresult sbDownloadSession::ResetTimers()
{
    mProgressTimer->Cancel();
    mProgressTimer->InitWithCallback(this, SB_DOWNLOAD_PROGRESS_TIMER_MS,
        nsITimer::TYPE_REPEATING_SLACK);

    mIdleTimer->Cancel();
    return mIdleTimer->InitWithCallback(this, SB_DOWNLOAD_IDLE_TIMEOUT_MS,
        nsITimer::TYPE_ONE_SHOT);

    return NS_OK;
}


/* *****************************************************************************
 *
 * Download device LibraryMetadataUpdater services.
 *
 *   This class updates a library items' metadata by scanning a local,
 * downloaded file.
 *
 ******************************************************************************/

/* LibraryMetadataUpdater nsISupports implementation. */
NS_IMPL_ISUPPORTS1(sbDownloadSession::LibraryMetadataUpdater,
                   sbIMediaListEnumerationListener)


/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return true to begin enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumerationBegin(
    sbIMediaList                *aMediaList,
    PRUint16                    *_retval)
{
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(_retval);

    /* Create an array to contain the media items to scan. */
    mpMediaItemArray = do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &result);

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *_retval = sbIMediaListEnumerationListener::CONTINUE;
    else
        *_retval = sbIMediaListEnumerationListener::CANCEL;

    return (result);
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return true to continue enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumeratedItem(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRUint16                    *_retval)
{
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(_retval);

    /* Put the media item into the scanning array. */
    result = mpMediaItemArray->AppendElement(aMediaItem, PR_FALSE);

    /* Return results. */
    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return (NS_OK);
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumerationEnd(
    sbIMediaList                *aMediaList,
    nsresult                    aStatusCode)
{
    nsCOMPtr<sbIFileMetadataService>
                                pMetadataService;
    nsCOMPtr<sbIJobProgress>    pMetadataJob;
    nsresult                    result = NS_OK;

    /* Start a metadata scanning job. */
    pMetadataService = do_GetService
                            ("@songbirdnest.com/Songbird/FileMetadataService;1",
                             &result);
    if (NS_SUCCEEDED(result))
    {
        result = pMetadataService->Read(mpMediaItemArray,
                                        getter_AddRefs(pMetadataJob));
    }

    return (result);
}


/* *****************************************************************************
 *
 * Download session WebLibraryUpdater services.
 *
 *   This class updates web library items' content source properties with
 * downloaded local URLs.
 *
 ******************************************************************************/

/* WebLibraryUpdater nsISupports implementation. */
NS_IMPL_ISUPPORTS1(sbDownloadSession::WebLibraryUpdater,
                   sbIMediaListEnumerationListener)


/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return true to begin enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumerationBegin(
    sbIMediaList                *aMediaList,
    PRUint16                    *_retval)
{
    nsresult                    result = NS_OK;

    /* Return results. */
    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return (result);
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return true to continue enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumeratedItem(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRUint16                    *_retval)
{
    nsresult                    result = NS_OK;

    /* Update the content source. */
    aMediaItem->SetContentSrc(mpDownloadSession->mpDstURI);

    /* Return results. */
    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return (result);
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumerationEnd(
    sbIMediaList                *aMediaList,
    nsresult                    aStatusCode)
{
    nsresult                    result = NS_OK;

    return (result);
}

sbAutoDownloadButtonPropertyValue::sbAutoDownloadButtonPropertyValue(sbIMediaItem* aMediaItem,
                                                                     sbIMediaItem* aStatusTarget,
                                                                     PRBool aReadOnly) :
  value(nsnull),
  mMediaItem(aMediaItem),
  mStatusTarget(aStatusTarget),
  mReadOnly(aReadOnly)
{
  NS_ASSERTION(mMediaItem, "mMediaItem is null");

  MOZ_COUNT_CTOR(sbAutoDownloadButtonPropertyValue);

  nsString propertyValue;
  mMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOADBUTTON),
                          propertyValue);
  value = new sbDownloadButtonPropertyValue(propertyValue);
  NS_ASSERTION(value, "value is null");
}

sbAutoDownloadButtonPropertyValue::~sbAutoDownloadButtonPropertyValue()
{
  if (!mReadOnly && value) {
    nsString propertyValue;
    value->GetValue(propertyValue);

    nsresult rv;
    rv = mMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOADBUTTON),
                                 propertyValue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetProperty failed");

    if (mStatusTarget) {
      rv = mStatusTarget->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DOWNLOADBUTTON),
                                      propertyValue);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetProperty failed");
    }
  }
  MOZ_COUNT_DTOR(sbAutoDownloadButtonPropertyValue);
}


NS_IMPL_THREADSAFE_ISUPPORTS1(sbDownloadSessionMoveHandler, 
                              nsIRunnable)

NS_IMETHODIMP sbDownloadSessionMoveHandler::Run() {
  nsresult rv = NS_ERROR_UNEXPECTED;
  
  rv = mSourceFile->MoveTo(mDestinationPath, mDestinationFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIFileMetadataService> metadataService;
  nsCOMPtr<sbIJobProgress>         metadataJob;

  /* Create an array to contain the media items to scan. */
  nsCOMPtr<nsIMutableArray> itemArray = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

  rv = itemArray->AppendElement(mDestinationItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Start a metadata scanning job. */
  metadataService = 
    do_GetService("@songbirdnest.com/Songbird/FileMetadataService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return metadataService->Read(itemArray,
                               getter_AddRefs(metadataJob));
}
