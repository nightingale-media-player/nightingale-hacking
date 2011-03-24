/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbBaseDevice.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <vector>

#include <prtime.h>

#include <nsIDOMParser.h>
#include <nsIFileURL.h>
#include <nsIMutableArray.h>
#include <nsIPropertyBag2.h>
#include <nsITimer.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch.h>
#include <nsIProxyObjectManager.h>

#include <nsAppDirectoryServiceDefs.h>
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsCRT.h>
#include <nsDirectoryServiceUtils.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMNodeList.h>
#include <nsIDOMWindow.h>
#include <nsIPromptService.h>
#include <nsIScriptSecurityManager.h>
#include <nsISupportsPrimitives.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>
#include <nsIXMLHttpRequest.h>

#include <sbIDeviceContent.h>
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceInfoRegistrar.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncDiff.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceHelper.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceProperties.h>
#include <sbIDownloadDevice.h>
#include <sbIJobCancelable.h>
#include <sbILibrary.h>
#include <sbILibraryDiffingService.h>
#include <sbILibraryUtils.h>
#include <sbILocalDatabaseSmartMediaList.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediaFileManager.h>
#include <sbIMediaInspector.h>
#include <sbIMediaItem.h>
#include <sbIMediaItemController.h>
#include <sbIMediaItemDownloader.h>
#include <sbIMediaItemDownloadJob.h>
#include <sbIMediaItemDownloadService.h>
#include <sbIMediaList.h>
#include <sbIOrderableMediaList.h>
#include <sbIPrompter.h>
#include <sbIPropertyManager.h>
#include <sbIPropertyUnitConverter.h>
#include <nsISupportsPrimitives.h>
#include <sbITranscodeManager.h>
#include <sbITranscodeAlbumArt.h>
#include <sbITranscodingConfigurator.h>

#include <sbArray.h>
#include <sbArrayUtils.h>
#include <sbDeviceLibrary.h>
#include <sbDeviceStatus.h>
#include <sbFileUtils.h>
#include <sbJobUtils.h>
#include <sbLibraryUtils.h>
#include <sbLocalDatabaseCID.h>
#include <sbMemoryUtils.h>
#include <sbPrefBranch.h>
#include <sbPropertiesCID.h>
#include <sbPropertyBagUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardDeviceProperties.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbTranscodeUtils.h>
#include <sbURIUtils.h>
#include <sbVariantUtils.h>
#include <sbWatchFolderUtils.h>

#include "sbDeviceEnsureSpaceForWrite.h"
#include "sbDeviceProgressListener.h"
#include "sbDeviceStatusHelper.h"
#include "sbDeviceStreamingHandler.h"
#include "sbDeviceSupportsItemHelper.h"
#include "sbDeviceTranscoding.h"
#include "sbDeviceImages.h"
#include "sbDeviceUtils.h"
#include "sbDeviceXMLCapabilities.h"
#include "sbDeviceXMLInfo.h"
#include "sbLibraryListenerHelpers.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseDevice:5
 */
#undef LOG
#undef TRACE
#ifdef PR_LOGGING
PRLogModuleInfo* gBaseDeviceLog = nsnull;
#define LOG(args)   PR_LOG(gBaseDeviceLog, PR_LOG_WARN,  args)
#define TRACE(args) PR_LOG(gBaseDeviceLog, PR_LOG_DEBUG, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define LOG(args)  do{ } while(0)
#define TRACE(args) do { } while(0)
#endif

// preference names
#define PREF_DEVICE_PREFERENCES_BRANCH "songbird.device."
#define PREF_DEVICE_LIBRARY_BASE "library."
#define PREF_WARNING "warning."
#define PREF_ORGANIZE_PREFIX "media_management.library."
#define PREF_ORGANIZE_ENABLED "media_management.library.enabled"
#define PREF_ORGANIZE_DIR_FORMAT "media_management.library.format.dir"
#define PREF_ORGANIZE_FILE_FORMAT "media_management.library.format.file"
#define SB_PROPERTY_UILIMITTYPE "http://songbirdnest.com/data/1.0#uiLimitType"
#define RANDOM_LISTNAME "device.error.not_enough_freespace.random_playlist_name"

// default preferences
#define PREF_ORGANIZE_DIR_FORMAT_DEFAULT \
  SB_PROPERTY_ARTISTNAME "," FILE_PATH_SEPARATOR "," SB_PROPERTY_ALBUMNAME

#define BATCH_TIMEOUT 200 /* number of milliseconds to wait for batching */
#define BYTES_PER_10MB (10 * 1000 * 1000)

// List of supported device content folders.
static const PRUint32 sbBaseDeviceSupportedFolderContentTypeList[] =
{
  sbIDeviceCapabilities::CONTENT_AUDIO,
  sbIDeviceCapabilities::CONTENT_VIDEO,
  sbIDeviceCapabilities::CONTENT_PLAYLIST,
  sbIDeviceCapabilities::CONTENT_IMAGE
};

static nsresult
GetPropertyBag(sbIDevice * aDevice, nsIPropertyBag2 ** aProperties)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsCOMPtr<sbIDeviceProperties> deviceProperties;
  nsresult rv = aDevice->GetProperties(getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceProperties->GetProperties(aProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


static nsresult
GetWritableDeviceProperties(sbIDevice * aDevice,
                            nsIWritablePropertyBag ** aProperties)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsresult rv;

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2>        roDeviceProperties;
  rv = GetPropertyBag(aDevice, getter_AddRefs(roDeviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(roDeviceProperties, aProperties);
}

class MediaListListenerAttachingEnumerator : public sbIMediaListEnumerationListener
{
public:
  MediaListListenerAttachingEnumerator(sbBaseDevice* aDevice)
   : mDevice(aDevice)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  sbBaseDevice* mDevice;
};

NS_IMPL_ISUPPORTS1(MediaListListenerAttachingEnumerator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP MediaListListenerAttachingEnumerator::OnEnumerationBegin(sbIMediaList*,
                                                                       PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP MediaListListenerAttachingEnumerator::OnEnumeratedItem(sbIMediaList*,
                                                                     sbIMediaItem* aItem,
                                                                     PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbIMediaList> list(do_QueryInterface(aItem, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDevice->ListenToList(list);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP MediaListListenerAttachingEnumerator::OnEnumerationEnd(sbIMediaList*,
                                                                     nsresult)
{
  return NS_OK;
}

class ShowMediaListEnumerator : public sbIMediaListEnumerationListener
{
public:
  explicit ShowMediaListEnumerator(PRBool aHideMediaLists);
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  PRBool    mHideMediaLists;
  nsString  mHideMediaListsStringValue;
};

NS_IMPL_ISUPPORTS1(ShowMediaListEnumerator ,
                   sbIMediaListEnumerationListener)


ShowMediaListEnumerator::ShowMediaListEnumerator(PRBool aHideMediaLists)
: mHideMediaLists(aHideMediaLists)
{
  mHideMediaListsStringValue = (mHideMediaLists == PR_TRUE) ?
                               NS_LITERAL_STRING("1") :
                               NS_LITERAL_STRING("0");
}

NS_IMETHODIMP ShowMediaListEnumerator::OnEnumerationBegin(sbIMediaList*,
                                                          PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP ShowMediaListEnumerator::OnEnumeratedItem(sbIMediaList*,
                                                        sbIMediaItem* aItem,
                                                        PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = aItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                          mHideMediaListsStringValue);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP ShowMediaListEnumerator::OnEnumerationEnd(sbIMediaList*,
                                                        nsresult)
{
  return NS_OK;
}

sbBaseDevice::TransferRequest *
sbBaseDevice::TransferRequest::New(PRUint32 aType,
                                   sbIMediaItem * aItem,
                                   sbIMediaList * aList,
                                   PRUint32 aIndex,
                                   PRUint32 aOtherIndex,
                                   nsISupports * aData)
{
  TransferRequest * request;
  NS_NEWXPCOM(request, TransferRequest);
  if (request) {
    request->SetType(aType);
    request->item = aItem;
    request->list = aList;
    request->index = aIndex;
    request->otherIndex = aOtherIndex;
    request->data = aData;
    // Delete, write, and read requests that aren't playlists are countable
    switch (aType) {
      case sbIDevice::REQUEST_DELETE:
      case sbIDevice::REQUEST_WRITE:
      case sbIDevice::REQUEST_READ:
        // These requests are countable unless they involve playlists
        if (!request->IsPlaylist()) {
          request->SetIsCountable(true);
        }
      default:
        break;
    }
  }
  return request;
}

PRBool sbBaseDevice::TransferRequest::IsPlaylist() const
{
  if (!list)
    return PR_FALSE;
  // Is this a playlist
  nsCOMPtr<sbILibrary> libTest = do_QueryInterface(list);
  return libTest ? PR_FALSE : PR_TRUE;
}

sbBaseDevice::TransferRequest::TransferRequest() :
  transcodeProfile(nsnull),
  contentSrcSet(PR_FALSE),
  destinationMediaPresent(PR_FALSE),
  destinationCompatibility(COMPAT_SUPPORTED),
  transcoded(PR_FALSE)
{
}

sbBaseDevice::TransferRequest::~TransferRequest()
{
  NS_IF_RELEASE(transcodeProfile);
}

sbBaseDevice::sbBaseDevice() :
  mIgnoreMediaListCount(0),
  mPerTrackOverhead(DEFAULT_PER_TRACK_OVERHEAD),
  mSyncState(sbBaseDevice::SYNC_STATE_NORMAL),
  mInfoRegistrarType(sbIDeviceInfoRegistrar::NONE),
  mPreferenceLock(nsnull),
  mMusicLimitPercent(100),
  mDeviceTranscoding(nsnull),
  mDeviceImages(nsnull),
  mCanTranscodeAudio(CAN_TRANSCODE_UNKNOWN),
  mCanTranscodeVideo(CAN_TRANSCODE_UNKNOWN),
  mSyncType(0),
  mEnsureSpaceChecked(false),
  mConnected(PR_FALSE),
  mVolumeLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gBaseDeviceLog) {
    gBaseDeviceLog = PR_NewLogModule( "sbBaseDevice" );
  }
#endif /* PR_LOGGING */

  #if defined(__GNUC__) && !defined(DEBUG)
    PRBool __attribute__((unused)) success;
  #else
    PRBool success;
  #endif

  mStateLock = nsAutoLock::NewLock(__FILE__ "::mStateLock");
  NS_ASSERTION(mStateLock, "Failed to allocate state lock");

  mPreviousStateLock = nsAutoLock::NewLock(__FILE__ "::mPreviousStateLock");
  NS_ASSERTION(mPreviousStateLock, "Failed to allocate state lock");

  mPreferenceLock = nsAutoLock::NewLock(__FILE__ "::mPreferenceLock");
  NS_ASSERTION(mPreferenceLock, "Failed to allocate preference lock");

  mConnectLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE,
                              __FILE__"::mConnectLock");
  NS_ASSERTION(mConnectLock, "Failed to allocate connection lock");

  // Create the volume lock.
  mVolumeLock = nsAutoLock::NewLock("sbBaseDevice::mVolumeLock");
  NS_ASSERTION(mVolumeLock, "Failed to allocate volume lock");

  // Initialize the track source table.
  success = mTrackSourceTable.Init();
  NS_ASSERTION(success, "Failed to initialize track source table");

  // Initialize the volume tables.
  success = mVolumeGUIDTable.Init();
  NS_ASSERTION(success, "Failed to initialize volume GUID table");

  success = mVolumeLibraryGUIDTable.Init();
  NS_ASSERTION(success, "Failed to initialize volume library GUID table");

  // the typical case is 1 library per device
  success = mOrganizeLibraryPrefs.Init(1);
  NS_ASSERTION(success, "Failed to initialize organize prefs hashtable");
}

/* virtual */
sbBaseDevice::~sbBaseDevice()
{
  if (mVolumeLock)
    nsAutoLock::DestroyLock(mVolumeLock);
  mVolumeLock = nsnull;

  mVolumeList.Clear();
  mTrackSourceTable.Clear();
  mVolumeGUIDTable.Clear();
  mVolumeLibraryGUIDTable.Clear();

  if (mPreferenceLock)
    nsAutoLock::DestroyLock(mPreferenceLock);

  if (mStateLock)
    nsAutoLock::DestroyLock(mStateLock);

  if (mPreviousStateLock)
    nsAutoLock::DestroyLock(mPreviousStateLock);

  if (mConnectLock)
    PR_DestroyRWLock(mConnectLock);
  mConnectLock = nsnull;

  if (mDeviceTranscoding) {
    delete mDeviceTranscoding;
  }

  if (mDeviceImages) {
    delete mDeviceImages;
  }

  if (mLibraryListener) {
    mLibraryListener->Destroy();
  }
}

NS_IMETHODIMP sbBaseDevice::Connect()
{
  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::Disconnect()
{
  // Cancel any pending deferred setup device timer.
  if (mDeferredSetupDeviceTimer) {
    mDeferredSetupDeviceTimer->Cancel();
    mDeferredSetupDeviceTimer = nsnull;
  }

  // Tell the thread to shutdown. The thread will then invoke the device
  // specific disconnect logic on the main thread.
  mRequestThreadQueue->Stop();

  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::SubmitRequest(PRUint32 aRequestType,
                                          nsIPropertyBag2 *aRequestParameters)
{
  nsRefPtr<TransferRequest> transferRequest;
  nsresult rv = CreateTransferRequest(aRequestType,
                                      aRequestParameters,
                                      getter_AddRefs(transferRequest));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mRequestThreadQueue->PushRequest(transferRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;

}

NS_IMETHODIMP sbBaseDevice::SyncLibraries()
{
  nsresult rv;

  nsCOMPtr<sbIDeviceContent> content;
  rv = GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> libraries;
  rv = content->GetLibraries(getter_AddRefs(libraries));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryCount;
  rv = libraries->GetLength(&libraryCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < libraryCount; ++index) {
    nsCOMPtr<sbIDeviceLibrary> deviceLib =
      do_QueryElementAt(libraries, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deviceLib->Sync();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbBaseDevice::PushRequest(const PRUint32 aType,
                                   sbIMediaItem* aItem,
                                   sbIMediaList* aList,
                                   PRUint32 aIndex,
                                   PRUint32 aOtherIndex,
                                   nsISupports * aData)
{
  NS_ENSURE_TRUE(aType != 0, NS_ERROR_INVALID_ARG);

  nsresult rv;

  nsRefPtr<TransferRequest> req = TransferRequest::New(aType,
                                                       aItem,
                                                       aList,
                                                       aIndex,
                                                       aOtherIndex,
                                                       aData);
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);

  // If we have an item get its content type and set the request data's
  // itemType
  if (req->item) {
    nsString contentType;
    rv = req->item->GetContentType(contentType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (contentType.Equals(NS_LITERAL_STRING("audio"))) {
      req->itemType = TransferRequest::REQUESTBATCH_AUDIO;
      mSyncType |= TransferRequest::REQUESTBATCH_AUDIO;
    }
    else if (contentType.Equals(NS_LITERAL_STRING("video"))) {
      req->itemType = TransferRequest::REQUESTBATCH_VIDEO;
      mSyncType |= TransferRequest::REQUESTBATCH_VIDEO;
    }
    else {
      req->itemType = TransferRequest::REQUESTBATCH_UNKNOWN;
    }
  }
  rv = mRequestThreadQueue->PushRequest(req);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbBaseDevice::BatchBegin()
{
  return mRequestThreadQueue->BatchBegin();
}

nsresult sbBaseDevice::BatchEnd()
{
  return mRequestThreadQueue->BatchEnd();
}

nsresult
sbBaseDevice::GetRequestTemporaryFileFactory
                (TransferRequest*          aRequest,
                 sbITemporaryFileFactory** aTemporaryFileFactory)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aTemporaryFileFactory);

  // Function variables.
  nsresult rv;

  // Get the request temporary file factory, creating one if necessary.
  nsCOMPtr<sbITemporaryFileFactory>
    temporaryFileFactory = aRequest->temporaryFileFactory;
  if (!temporaryFileFactory) {
    temporaryFileFactory = do_CreateInstance(SB_TEMPORARYFILEFACTORY_CONTRACTID,
                                             &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    aRequest->temporaryFileFactory = temporaryFileFactory;
  }

  // Return results.
  temporaryFileFactory.forget(aTemporaryFileFactory);

  return NS_OK;
}

/**
 * Dispatches a download error to the device
 */
static nsresult DispatchDownloadError(sbBaseDevice * aDevice,
                                      nsAString const & aMessage,
                                      sbIMediaItem * aItem)
{
  nsresult rv;

  NS_ASSERTION(aDevice, "aDevice null in DispatchDownloadError");

  // Create the error info.
  sbPropertyBagHelper errorInfo;
  errorInfo["message"] = nsString(aMessage);
  NS_ENSURE_SUCCESS(errorInfo.rv(), errorInfo.rv());
  errorInfo["item"] = aItem;
  NS_ENSURE_SUCCESS(errorInfo.rv(), errorInfo.rv());

  // Dispatch the error event.
  rv = aDevice->CreateAndDispatchEvent(
                                    sbIDeviceEvent::EVENT_DEVICE_DOWNLOAD_ERROR,
                                    sbNewVariant(errorInfo.GetBag()));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to dispatch download error");

  return NS_OK;
}

/**
 * Simple auto class to generate an error if we exit via an error path
 */
class sbDownloadAutoComplete : private sbDeviceStatusAutoOperationComplete
{
public:
  sbDownloadAutoComplete(sbDeviceStatusHelper * aStatus,
                         sbDeviceStatusHelper::Operation aOperation,
                         TransferRequest * aRequest,
                         PRUint32 aBatchCount,
                         sbBaseDevice * aDevice) :
                           sbDeviceStatusAutoOperationComplete(aStatus,
                                                               aOperation,
                                                               aRequest,
                                                               aBatchCount),
                           mDevice(aDevice),
                           mDownloadJob(nsnull),
                           mItem(aRequest->item)
  {

  }
  ~sbDownloadAutoComplete()
  {
    nsresult rv;

    if (mDevice && mItem) {
      nsString errorMessage;
      nsCOMPtr<nsIStringEnumerator> errorEnumerator;
      if (mDownloadJob) {
        PRUint32 errors;
        rv = mDownloadJob->GetErrorCount(&errors);
        if (NS_SUCCEEDED(rv) && errors) {
          rv = mDownloadJob->GetErrorMessages(getter_AddRefs(errorEnumerator));
          if (NS_SUCCEEDED(rv)) {
            PRBool more;
            rv = errorEnumerator->HasMore(&more);
            if (NS_SUCCEEDED(rv) && more) {
              nsString message;
              errorEnumerator->GetNext(message);
              if (!errorMessage.IsEmpty()) {
                errorMessage.Append(NS_LITERAL_STRING("\n"));
              }
              errorMessage.Append(message);
            }
          }
        }
      }
      // Return a generic error message if there is nothing specific
      if (errorMessage.IsEmpty()) {
        sbStringBundle bundle;
        errorMessage = bundle.Get("device.error.download",
                                  "Download of media failed.");
      }
      DispatchDownloadError(mDevice,
                            errorMessage,
                            mItem);
      // If we're dispatch the error then we don't want the parent auto class
      // to dispatch an error
      sbDeviceStatusAutoOperationComplete::SetResult(NS_OK);
    }
  }
  void SetDownloadJob(sbIMediaItemDownloadJob * aDownloadJob)
  {
    mDownloadJob = aDownloadJob;
  }
  void forget()
  {
    mDevice = nsnull;
  }
  void SetResult(nsresult aResult)
  {
    sbDeviceStatusAutoOperationComplete::SetResult(aResult);
    // If this is a success result we don't need to perform an error dispatch
    if (NS_SUCCEEDED(aResult)) {
      mDevice = nsnull;
    }
  }
private:
  // Non-owning reference, this can't outlive the device
  sbBaseDevice * mDevice;
  // We hold a reference, just to be safe, as this is an auto class and the
  // compiler might destroy the nsCOMPtr that holds this before our class.
  nsCOMPtr<sbIMediaItemDownloadJob> mDownloadJob;
  // Non-owning reference, we rely on the request to keep the item alive.
  sbIMediaItem * mItem;
};

nsresult
sbBaseDevice::DownloadRequestItem(TransferRequest*      aRequest,
                                  PRUint32              aBatchCount,
                                  sbDeviceStatusHelper* aDeviceStatusHelper)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aDeviceStatusHelper);

  // Function variables.
  nsresult rv;

  sbDownloadAutoComplete autoComplete(
                                  aDeviceStatusHelper,
                                  sbDeviceStatusHelper::OPERATION_TYPE_DOWNLOAD,
                                  aRequest,
                                  aBatchCount,
                                  this);

  // Get the request volume info.
  nsRefPtr<sbBaseDeviceVolume> volume;
  nsCOMPtr<sbIDeviceLibrary>   deviceLibrary;

  rv = GetVolumeForItem(aRequest->item, getter_AddRefs(volume));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = volume->GetDeviceLibrary(getter_AddRefs(deviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the download service.
  nsCOMPtr<sbIMediaItemDownloadService> downloadService =
    do_GetService("@songbirdnest.com/Songbird/MediaItemDownloadService;1",
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a downloader for the request item.  If none is returned, the request
  // item either doesn't need to be downloaded or cannot be downloaded.
  nsCOMPtr<sbIMediaItemDownloader> downloader;
  rv = downloadService->GetDownloader(aRequest->item,
                                      deviceLibrary,
                                      getter_AddRefs(downloader));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!downloader) {
    // Don't error, if item doesn't need to be downloaded
    autoComplete.forget();
    return NS_OK;
  }
  // Update status and set to auto-complete.
  aDeviceStatusHelper->ChangeState(STATE_DOWNLOADING);

  // Create a download job.
  // TODO: Add a writable temporaryFileFactory attribute to
  //       sbIMediaItemDownloadJob and set it to the request
  //       temporaryFileFactory.
  nsCOMPtr<sbIMediaItemDownloadJob> downloadJob;
  rv = downloader->CreateDownloadJob(aRequest->item,
                                     deviceLibrary,
                                     getter_AddRefs(downloadJob));
  NS_ENSURE_SUCCESS(rv, rv);

  autoComplete.SetDownloadJob(downloadJob);

  // Set the download job temporary file factory.
  nsCOMPtr<sbITemporaryFileFactory> temporaryFileFactory;
  rv = GetRequestTemporaryFileFactory(aRequest,
                                      getter_AddRefs(temporaryFileFactory));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = downloadJob->SetTemporaryFileFactory(temporaryFileFactory);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the download job progress.
  nsCOMPtr<sbIJobProgress>
    downloadJobProgress = do_MainThreadQueryInterface(downloadJob, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up to auto-cancel the job.
  nsCOMPtr<sbIJobCancelable> cancel = do_QueryInterface(downloadJobProgress);
  sbAutoJobCancel autoCancel(cancel);

  PRMonitor * stopWaitMonitor = mRequestThreadQueue->GetStopWaitMonitor();

  // Add a device job progress listener.
  nsRefPtr<sbDeviceProgressListener> listener;
  rv = sbDeviceProgressListener::New(getter_AddRefs(listener),
                                     stopWaitMonitor,
                                     aDeviceStatusHelper);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = downloadJobProgress->AddJobProgressListener(listener);
  NS_ENSURE_SUCCESS(rv, rv);

  // Start the download job.
  rv = downloadJob->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  // Wait for the download job to complete.
  PRBool isComplete = PR_FALSE;
  while (!isComplete) {
    // Operate within the request wait monitor.
    nsAutoMonitor monitor(stopWaitMonitor);

    // Check for abort.
    if (IsRequestAborted()) {
      return NS_ERROR_ABORT;
    }

    // Check if the job is complete.
    isComplete = listener->IsComplete();

    // If not complete, wait for completion.  If requests are cancelled, the
    // request wait monitor will be notified.
    if (!isComplete)
      monitor.Wait();
  }

  // Forget auto-cancel.
  autoCancel.forget();

  // Check for download errors.
  nsCOMPtr<nsIStringEnumerator> errorMessages;
  rv = downloadJob->GetErrorMessages(getter_AddRefs(errorMessages));
  NS_ENSURE_SUCCESS(rv, rv);
  if (errorMessages) {
    PRBool hasMore;
    rv = errorMessages->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasMore) {
      // Release the enumerator just ot be safe since the auto class will
      // be enumerating
      errorMessages = nsnull;
      autoComplete.SetResult(NS_ERROR_FAILURE);
      return NS_ERROR_FAILURE;
    }
  }

  // Get the downloaded file.
  rv = downloadJob->GetDownloadedFile(getter_AddRefs(aRequest->downloadedFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the downloaded media item properties.
  nsCOMPtr<sbIPropertyArray> properties;
  rv = downloadJob->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);
  {
    sbDeviceListenerIgnore ignore(this, aRequest->item);
    rv = aRequest->item->SetProperties(properties);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update the request item origin and content source.
  nsCOMPtr<nsIURI> downloadedFileURI;
  rv = sbNewFileURI(aRequest->downloadedFile,
                    getter_AddRefs(downloadedFileURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = UpdateOriginAndContentSrc(aRequest, downloadedFileURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Operation completed without error.
  autoComplete.SetResult(NS_OK);

  return NS_OK;
}

nsresult sbBaseDevice::GetPreferenceInternal(nsIPrefBranch *aPrefBranch,
                                             const nsAString & aPrefName,
                                             nsIVariant **_retval)
{
  NS_ENSURE_ARG_POINTER(aPrefBranch);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
  nsresult rv;

  NS_ConvertUTF16toUTF8 prefNameUTF8(aPrefName);

  // get tht type of the pref
  PRInt32 prefType;
  rv = aPrefBranch->GetPrefType(prefNameUTF8.get(), &prefType);
  NS_ENSURE_SUCCESS(rv, rv);

  // create a writable variant
  nsCOMPtr<nsIWritableVariant> writableVariant =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the value of our pref
  switch (prefType) {
    case nsIPrefBranch::PREF_INVALID: {
      rv = writableVariant->SetAsEmpty();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_STRING: {
      char* _value = NULL;
      rv = aPrefBranch->GetCharPref(prefNameUTF8.get(), &_value);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString value;
      value.Adopt(_value);

      // set the value of the variant to the value of the pref
      rv = writableVariant->SetAsACString(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_INT: {
      PRInt32 value;
      rv = aPrefBranch->GetIntPref(prefNameUTF8.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = writableVariant->SetAsInt32(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_BOOL: {
      PRBool value;
      rv = aPrefBranch->GetBoolPref(prefNameUTF8.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = writableVariant->SetAsBool(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
  }

  return CallQueryInterface(writableVariant, _retval);
}

/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP sbBaseDevice::GetPreference(const nsAString & aPrefName, nsIVariant **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
  nsresult rv;

  // special case device capabilities preferences
  if (aPrefName.Equals(NS_LITERAL_STRING("capabilities"))) {
    return GetCapabilitiesPreference(_retval);
  }

  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  return GetPreferenceInternal(prefBranch, aPrefName, _retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP sbBaseDevice::SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue)
{
  NS_ENSURE_ARG_POINTER(aPrefValue);
  NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
  nsresult rv;

  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  return SetPreferenceInternal(prefBranch, aPrefName, aPrefValue);
}

nsresult sbBaseDevice::SetPreferenceInternal(nsIPrefBranch *aPrefBranch,
                                             const nsAString & aPrefName,
                                             nsIVariant *aPrefValue)
{
  NS_ENSURE_ARG_POINTER(aPrefValue);
  NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
  nsresult rv;

  PRBool hasChanged = PR_FALSE;
  rv = SetPreferenceInternal(aPrefBranch, aPrefName, aPrefValue, &hasChanged);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasChanged) {
    // apply the preference
    ApplyPreference(aPrefName, aPrefValue);

    // fire the pref change event
    nsCOMPtr<sbIDeviceManager2> devMgr =
      do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED,
                                sbNewVariant(aPrefName),
                                PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbBaseDevice::SetPreferenceInternalNoNotify(
                         const nsAString& aPrefName,
                         nsIVariant*      aPrefValue,
                         PRBool*          aHasChanged)
{
  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  nsresult rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  return SetPreferenceInternal(prefBranch, aPrefName, aPrefValue, aHasChanged);
}

nsresult sbBaseDevice::SetPreferenceInternal(nsIPrefBranch*   aPrefBranch,
                                             const nsAString& aPrefName,
                                             nsIVariant*      aPrefValue,
                                             PRBool*          aHasChanged)
{
  NS_ENSURE_ARG_POINTER(aPrefValue);
  NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
  nsresult rv;

  NS_ConvertUTF16toUTF8 prefNameUTF8(aPrefName);

  // figure out what sort of variant we have
  PRUint16 dataType;
  rv = aPrefValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  // figure out what sort of data we used to have
  PRInt32 prefType;
  rv = aPrefBranch->GetPrefType(prefNameUTF8.get(), &prefType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasChanged = PR_FALSE;

  switch (dataType) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_UINT16:
    case nsIDataType::VTYPE_UINT32:
    case nsIDataType::VTYPE_UINT64:
    case nsIDataType::VTYPE_FLOAT:
    case nsIDataType::VTYPE_DOUBLE:
    {
      // some sort of number
      PRInt32 oldValue, value;
      rv = aPrefValue->GetAsInt32(&value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (prefType != nsIPrefBranch::PREF_INT) {
        hasChanged = PR_TRUE;
      } else {
        rv = aPrefBranch->GetIntPref(prefNameUTF8.get(), &oldValue);
        if (NS_SUCCEEDED(rv) && oldValue != value) {
          hasChanged = PR_TRUE;
        }
      }

      rv = aPrefBranch->SetIntPref(prefNameUTF8.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }

    case nsIDataType::VTYPE_BOOL:
    {
      // a bool pref
      PRBool oldValue, value;
      rv = aPrefValue->GetAsBool(&value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (prefType != nsIPrefBranch::PREF_BOOL) {
        hasChanged = PR_TRUE;
      } else {
        rv = aPrefBranch->GetBoolPref(prefNameUTF8.get(), &oldValue);
        if (NS_SUCCEEDED(rv) && oldValue != value) {
          hasChanged = PR_TRUE;
        }
      }

      rv = aPrefBranch->SetBoolPref(prefNameUTF8.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }

    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
    {
      // unset the pref
      if (prefType != nsIPrefBranch::PREF_INVALID) {
        rv = aPrefBranch->ClearUserPref(prefNameUTF8.get());
        NS_ENSURE_SUCCESS(rv, rv);
        hasChanged = PR_TRUE;
      }

      break;
    }

    default:
    {
      // assume a string
      nsCString value;
      rv = aPrefValue->GetAsACString(value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (prefType != nsIPrefBranch::PREF_STRING) {
        hasChanged = PR_TRUE;
      } else {
        char* oldValue;
        rv = aPrefBranch->GetCharPref(prefNameUTF8.get(), &oldValue);
        if (NS_SUCCEEDED(rv)) {
          if (!(value.Equals(oldValue))) {
            hasChanged = PR_TRUE;
          }
          NS_Free(oldValue);
        }
      }

      rv = aPrefBranch->SetCharPref(prefNameUTF8.get(), value.get());
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }
  }

  // return has changed status
  if (aHasChanged)
    *aHasChanged = hasChanged;

  return NS_OK;
}

nsresult sbBaseDevice::HasPreference(nsAString& aPrefName,
                                     PRBool*    aHasPreference)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aHasPreference);

  // Function variables.
  nsresult rv;

  // Try getting the preference.
  nsCOMPtr<nsIVariant> prefValue;
  rv = GetPreference(aPrefName, getter_AddRefs(prefValue));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!prefValue) {
    *aHasPreference = PR_FALSE;
    return NS_OK;
  }

  // Preference does not exist if it's empty or void.
  PRUint16 dataType;
  rv = prefValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);
  if ((dataType == nsIDataType::VTYPE_EMPTY) ||
      (dataType == nsIDataType::VTYPE_VOID)) {
    *aHasPreference = PR_FALSE;
    return NS_OK;
  }

  // Preference exists.
  *aHasPreference = PR_TRUE;

  return NS_OK;
}

/* readonly attribute boolean isDirectTranscoding; */
NS_IMETHODIMP sbBaseDevice::GetIsDirectTranscoding(PRBool *aIsDirect)
{
  *aIsDirect = PR_TRUE;
  return NS_OK;
}

/* readonly attribute boolean isBusy; */
NS_IMETHODIMP sbBaseDevice::GetIsBusy(PRBool *aIsBusy)
{
  NS_ENSURE_ARG_POINTER(aIsBusy);
  NS_ENSURE_TRUE(mStateLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mStateLock);
  switch (mState) {
    case STATE_IDLE:
    case STATE_DOWNLOAD_PAUSED:
    case STATE_UPLOAD_PAUSED:
      *aIsBusy = PR_FALSE;
    break;

    default:
      *aIsBusy = PR_TRUE;
    break;
  }
  return NS_OK;
}

/* readonly attribute boolean canDisconnect; */
NS_IMETHODIMP sbBaseDevice::GetCanDisconnect(PRBool *aCanDisconnect)
{
  NS_ENSURE_ARG_POINTER(aCanDisconnect);
  NS_ENSURE_TRUE(mStateLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mStateLock);
  switch(mState) {
    case STATE_IDLE:
    case STATE_MOUNTING:
    case STATE_DISCONNECTED:
    case STATE_DOWNLOAD_PAUSED:
    case STATE_UPLOAD_PAUSED:
      *aCanDisconnect = PR_TRUE;
    break;

    default:
      *aCanDisconnect = PR_FALSE;
    break;
  }
  return NS_OK;
}

/* readonly attribute unsigned long state; */
NS_IMETHODIMP sbBaseDevice::GetPreviousState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  NS_ENSURE_TRUE(mPreviousStateLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mPreviousStateLock);
  *aState = mPreviousState;
  return NS_OK;
}

nsresult sbBaseDevice::SetPreviousState(PRUint32 aState)
{
  // set state, checking if it changed
  NS_ENSURE_TRUE(mPreviousStateLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mPreviousStateLock);
  if (mPreviousState != aState) {
    mPreviousState = aState;
  }
  return NS_OK;
}

/* readonly attribute unsigned long state; */
NS_IMETHODIMP sbBaseDevice::GetState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  NS_ENSURE_TRUE(mStateLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mStateLock);
  *aState = mState;
  return NS_OK;
}

nsresult sbBaseDevice::SetState(PRUint32 aState)
{

  nsresult rv;
  PRBool stateChanged = PR_FALSE;
  PRUint32 prevState;

  // set state, checking if it changed
  {
    NS_ENSURE_TRUE(mStateLock, NS_ERROR_NOT_INITIALIZED);
    nsAutoLock lock(mStateLock);

    // Only allow the cancel state to transition to the idle state.  This
    // prevents the request processing code from changing the state from cancel
    // to some other operation before it has checked for a canceled request.
    if ((mState == STATE_CANCEL) && (aState != STATE_IDLE))
      return NS_OK;

    prevState = mState;
    if (mState != aState) {
      mState = aState;
      stateChanged = PR_TRUE;
    }
    // Update the previous state - we set it outside of the if loop, so
    // even if the current state isn't changing, the previous state still
    // gets updated to the correct previous state.
    SetPreviousState(prevState);
  }

  // send state changed event.  do it outside of lock in case event handler gets
  // called immediately and tries to read the state
  if (stateChanged) {
    nsCOMPtr<nsIWritableVariant> var =
        do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = var->SetAsUint32(aState);
    NS_ENSURE_SUCCESS(rv, rv);
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_STATE_CHANGED, var);
  }

  return NS_OK;
}

nsresult sbBaseDevice::CreateDeviceLibrary(const nsAString& aId,
                                           nsIURI* aLibraryLocation,
                                           sbIDeviceLibrary** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsRefPtr<sbDeviceLibrary> devLib = new sbDeviceLibrary(this);
  NS_ENSURE_TRUE(devLib, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = InitializeDeviceLibrary(devLib, aId, aLibraryLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(devLib.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbBaseDevice::InitializeDeviceLibrary
                         (sbDeviceLibrary* aDevLib,
                          const nsAString& aId,
                          nsIURI*          aLibraryLocation)
{
  NS_ENSURE_ARG_POINTER(aDevLib);

  if (!mMediaListListeners.IsInitialized()) {
    // we expect to be unintialized, but just in case...
    if (!mMediaListListeners.Init()) {
      return NS_ERROR_FAILURE;
    }
  }

  nsresult rv = aDevLib->Initialize(aId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Hide the library on creation. The device is responsible
  // for showing it is done mounting.
  rv = aDevLib->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                            NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDevLib->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE),
                            NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mLibraryListener) {
    nsRefPtr<sbBaseDeviceLibraryListener> libListener =
      new sbBaseDeviceLibraryListener();
    NS_ENSURE_TRUE(libListener, NS_ERROR_OUT_OF_MEMORY);

    rv = libListener->Init(this);
    NS_ENSURE_SUCCESS(rv, rv);

    libListener.swap(mLibraryListener);
  }

  rv = aDevLib->AddDeviceLibraryListener(mLibraryListener);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the library preferences.
  rv = InitializeDeviceLibraryPreferences(aDevLib);
  NS_ENSURE_SUCCESS(rv, rv);

  // hook up the media list listeners to the existing lists
  nsRefPtr<MediaListListenerAttachingEnumerator> enumerator =
    new MediaListListenerAttachingEnumerator(this);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  rv = aDevLib->EnumerateItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                         NS_LITERAL_STRING("1"),
                                         enumerator,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::InitializeDeviceLibraryPreferences(sbDeviceLibrary* aDevLib)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevLib);

  // Function variables.
  nsresult rv;

  // Get the base library preference name.
  nsAutoString libraryPreferenceBase;
  rv = GetLibraryPreferenceBase(aDevLib, libraryPreferenceBase);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if the library organize enabled preference has been set.
  PRBool       organizeEnabledSet;
  nsAutoString organizeEnabledPref = libraryPreferenceBase;
  organizeEnabledPref.Append(NS_LITERAL_STRING(PREF_ORGANIZE_ENABLED));
  rv = HasPreference(organizeEnabledPref, &organizeEnabledSet);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set default library organize preferences if they haven't been set yet.
  if (!organizeEnabledSet) {
    // Set the default organize directory format.
    nsAutoString organizeDirFormatPref = libraryPreferenceBase;
    organizeDirFormatPref.Append(NS_LITERAL_STRING(PREF_ORGANIZE_DIR_FORMAT));
    rv = SetPreference(organizeDirFormatPref,
                       sbNewVariant(PREF_ORGANIZE_DIR_FORMAT_DEFAULT));
    NS_ENSURE_SUCCESS(rv, rv);

    // Enable library organization by default.
    rv = SetPreference(organizeEnabledPref,
                       sbNewVariant(PR_TRUE, nsIDataType::VTYPE_BOOL));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void sbBaseDevice::FinalizeDeviceLibrary(sbIDeviceLibrary* aDevLib)
{
  // Finalize and clear the media list listeners.
  EnumerateFinalizeMediaListListenersInfo enumerateInfo;
  enumerateInfo.device = this;
  enumerateInfo.library = aDevLib;
  mMediaListListeners.Enumerate
                        (sbBaseDevice::EnumerateFinalizeMediaListListeners,
                         &enumerateInfo);

  // Finalize the device library.
  aDevLib->RemoveDeviceLibraryListener(mLibraryListener);
  aDevLib->Finalize();
}

nsresult sbBaseDevice::AddLibrary(sbIDeviceLibrary* aDevLib)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevLib);

  // Function variables.
  nsresult rv;

  // Check library access.
  rv = CheckAccess(aDevLib);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device content.
  nsCOMPtr<sbIDeviceContent> content;
  rv = GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the device library volume name.
  nsRefPtr<sbBaseDeviceVolume> volume;
  rv = GetVolumeForItem(aDevLib, getter_AddRefs(volume));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = UpdateVolumeName(volume);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the library to the device content.
  rv = content->AddLibrary(aDevLib);
  NS_ENSURE_SUCCESS(rv, rv);

  // Send a device library added event.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_LIBRARY_ADDED,
                         sbNewVariant(aDevLib));

  // If no default library has been set, use library as default.  Otherwise,
  // check default library preference.
  if (!mDefaultLibrary) {
    rv = UpdateDefaultLibrary(aDevLib);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Get the default library GUID.
    nsAutoString defaultLibraryGUID;
    nsCOMPtr<nsIVariant> defaultLibraryGUIDPref;
    rv = GetPreference(NS_LITERAL_STRING("default_library_guid"),
                       getter_AddRefs(defaultLibraryGUIDPref));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = defaultLibraryGUIDPref->GetAsAString(defaultLibraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the added library GUID.
    nsAutoString libraryGUID;
    rv = aDevLib->GetGuid(libraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    // If added library GUID matches default library preference GUID, set it as
    // the default.
    if (libraryGUID.Equals(defaultLibraryGUID)) {
      rv = UpdateDefaultLibrary(aDevLib);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Apply the library preferences.
  rv = ApplyLibraryPreference(aDevLib, SBVoidString(), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbBaseDevice::CheckAccess(sbIDeviceLibrary* aDevLib)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevLib);

  // Function variables.
  nsresult rv;

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2>     deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the access compatibility.
  nsAutoString accessCompatibility;
  rv = deviceProperties->GetPropertyAsAString
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY),
          accessCompatibility);
  if (NS_FAILED(rv))
    accessCompatibility.Truncate();

  // Do nothing more if access is not read-only.
  if (!accessCompatibility.Equals(NS_LITERAL_STRING("ro")))
    return NS_OK;

  // Prompt user.
  // Get a prompter.
  nsCOMPtr<sbIPrompter>
    prompter = do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Determine whether the access can be changed.
  PRBool canChangeAccess = PR_FALSE;
  rv = deviceProperties->GetPropertyAsBool
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY_MUTABLE),
          &canChangeAccess);
  if (NS_FAILED(rv))
    canChangeAccess = PR_FALSE;

  // Get the device name.
  nsAutoString deviceName;
  rv = GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the prompt title.
  SBLocalizedString title("device.dialog.read_only_device.title");

  // Get the prompt message.
  nsAutoString       msg;
  nsTArray<nsString> params;
  params.AppendElement(deviceName);
  if (canChangeAccess) {
    msg = SBLocalizedString("device.dialog.read_only_device.can_change.msg",
                            params);
  } else {
    msg = SBLocalizedString("device.dialog.read_only_device.cannot_change.msg",
                            params);
  }

  // Configure the buttons.
  PRUint32 buttonFlags = 0;
  PRInt32 changeAccessButtonIndex = -1;
  if (canChangeAccess) {
    changeAccessButtonIndex = 0;
    buttonFlags += nsIPromptService::BUTTON_POS_0 *
                   nsIPromptService::BUTTON_TITLE_IS_STRING;
    buttonFlags += nsIPromptService::BUTTON_POS_1 *
                   nsIPromptService::BUTTON_TITLE_IS_STRING;
  } else {
    buttonFlags += nsIPromptService::BUTTON_POS_0 *
                   nsIPromptService::BUTTON_TITLE_OK;
  }

  // Get the button labels.
  SBLocalizedString buttonLabel0("device.dialog.read_only_device.change");
  SBLocalizedString buttonLabel1("device.dialog.read_only_device.dont_change");

  // Prompt user.
  PRInt32 buttonPressed;
  rv = prompter->ConfirmEx(nsnull,             // Parent window.
                           title.get(),
                           msg.get(),
                           buttonFlags,
                           buttonLabel0.get(),
                           buttonLabel1.get(),
                           nsnull,             // Button 2 label.
                           nsnull,             // Check message.
                           nsnull,             // Check result.
                           &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);

  // Change access if user selected to do so.
  if (canChangeAccess && (buttonPressed == changeAccessButtonIndex)) {
    // Set the access compatibility property to read-write.
    nsCOMPtr<nsIWritablePropertyBag>
      writeDeviceProperties = do_QueryInterface(deviceProperties, &rv);
    accessCompatibility = NS_LITERAL_STRING("rw");
    NS_ENSURE_SUCCESS(rv, rv);
    writeDeviceProperties->SetProperty
      (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY),
       sbNewVariant(accessCompatibility));
  }

  return NS_OK;
}

nsresult sbBaseDevice::RemoveLibrary(sbIDeviceLibrary* aDevLib)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevLib);

  // Function variables.
  nsresult rv;

  // Get the device content.
  nsCOMPtr<sbIDeviceContent> content;
  rv = GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  // If the default library is being removed, change the default to the first
  // library.
  if (aDevLib == mDefaultLibrary) {
    // Get the list of device libraries.
    PRUint32           libraryCount;
    nsCOMPtr<nsIArray> libraries;
    rv = content->GetLibraries(getter_AddRefs(libraries));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = libraries->GetLength(&libraryCount);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the default library to the first library or null if no libraries.
    nsCOMPtr<sbIDeviceLibrary> defaultLibrary;
    for (PRUint32 i = 0; i < libraryCount; ++i) {
      nsCOMPtr<sbIDeviceLibrary> library = do_QueryElementAt(libraries, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      if (library != aDevLib) {
        defaultLibrary = library;
        break;
      }
    }
    rv = UpdateDefaultLibrary(defaultLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Send a device library removed event.
  nsAutoString guid;
  rv = aDevLib->GetGuid(guid);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get device library.");
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_LIBRARY_REMOVED,
                         sbNewVariant(guid));

  // Remove the device library from the device content.
  rv = content->RemoveLibrary(aDevLib);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::UpdateLibraryProperty(sbILibrary*      aLibrary,
                                    const nsAString& aPropertyID,
                                    const nsAString& aPropertyValue)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  nsresult rv;

  // Get the current property value.
  nsAutoString currentPropertyValue;
  rv = aLibrary->GetProperty(aPropertyID, currentPropertyValue);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the property value if it's changing.
  if (!aPropertyValue.Equals(currentPropertyValue)) {
    rv = aLibrary->SetProperty(aPropertyID, aPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseDevice::UpdateDefaultLibrary(sbIDeviceLibrary* aDevLib)
{
  nsresult rv;

  // Do nothing if default library is not changing.
  if (aDevLib == mDefaultLibrary)
    return NS_OK;

  // Update the default library and volume.
  nsRefPtr<sbBaseDeviceVolume> volume;
  if (aDevLib) {
    rv = GetVolumeForItem(aDevLib, getter_AddRefs(volume));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mDefaultLibrary = aDevLib;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    mDefaultVolume = volume;
  }

  // Handle the default library change.
  OnDefaultLibraryChanged();

  return NS_OK;
}

nsresult
sbBaseDevice::OnDefaultLibraryChanged()
{
  // Send a default library changed event.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_DEFAULT_LIBRARY_CHANGED,
                         sbNewVariant(mDefaultLibrary));

  return NS_OK;
}

nsresult sbBaseDevice::ListenToList(sbIMediaList* aList)
{
  NS_ENSURE_ARG_POINTER(aList);

  NS_ASSERTION(mMediaListListeners.IsInitialized(),
               "sbBaseDevice::ListenToList called before listener hash is initialized!");

  nsresult rv;

  #if DEBUG
    // check to make sure we're not listening to a library
    nsCOMPtr<sbILibrary> library = do_QueryInterface(aList);
    NS_ASSERTION(!library,
                 "Should not call sbBaseDevice::ListenToList on a library!");
  #endif

  // the extra QI to make sure we're at the canonical pointer
  // and not some derived interface
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // check for an existing listener
  if (mMediaListListeners.Get(list, nsnull)) {
    // we are already listening to the media list, don't re-add
    return NS_OK;
  }

  nsRefPtr<sbBaseDeviceMediaListListener> listener =
    new sbBaseDeviceMediaListListener();
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  rv = listener->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = list->AddListener(listener,
                         PR_FALSE, /* weak */
                         0, /* all */
                         nsnull /* filter */);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we're currently ignoring listeners then we need to ignore this one too
  // else we'll get out of balance
  if (mIgnoreMediaListCount > 0)
    listener->SetIgnoreListener(PR_TRUE);
  mMediaListListeners.Put(list, listener);

  return NS_OK;
}

PLDHashOperator sbBaseDevice::EnumerateFinalizeMediaListListeners
                                (nsISupports* aKey,
                                 nsRefPtr<sbBaseDeviceMediaListListener>& aData,
                                 void* aClosure)
{
  nsresult rv;

  // Get the device and library for which to finalize media list listeners.
  EnumerateFinalizeMediaListListenersInfo*
    enumerateInfo =
      static_cast<EnumerateFinalizeMediaListListenersInfo*>(aClosure);
  nsCOMPtr<sbILibrary> library = enumerateInfo->library;

  // Get the listener media list.
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aKey, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  // Do nothing if media list is contained in another library.
  nsCOMPtr<sbILibrary> mediaListLibrary;
  PRBool               equals;
  rv = mediaList->GetLibrary(getter_AddRefs(mediaListLibrary));
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  rv = mediaListLibrary->Equals(library, &equals);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  if (!equals)
    return PL_DHASH_NEXT;

  // Remove the media list listener.
  mediaList->RemoveListener(aData);

  return PL_DHASH_REMOVE;
}

PLDHashOperator sbBaseDevice::EnumerateIgnoreMediaListListeners(nsISupports* aKey,
                                                                nsRefPtr<sbBaseDeviceMediaListListener> aData,
                                                                void* aClosure)
{
  nsresult rv;
  PRBool *ignore = static_cast<PRBool *>(aClosure);

  rv = aData->SetIgnoreListener(*ignore);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

nsresult
sbBaseDevice::SetIgnoreMediaListListeners(PRBool aIgnoreListener)
{
  if (aIgnoreListener)
    PR_AtomicIncrement(&mIgnoreMediaListCount);
  else
    PR_AtomicDecrement(&mIgnoreMediaListCount);

  mMediaListListeners.EnumerateRead(sbBaseDevice::EnumerateIgnoreMediaListListeners,
                                    &aIgnoreListener);
  return NS_OK;
}

nsresult
sbBaseDevice::SetIgnoreLibraryListener(PRBool aIgnoreListener)
{
  return mLibraryListener->SetIgnoreListener(aIgnoreListener);
}

nsresult
sbBaseDevice::SetMediaListsHidden(sbIMediaList *aLibrary, PRBool aHidden)
{
  NS_ENSURE_ARG_POINTER(aLibrary);

  nsRefPtr<ShowMediaListEnumerator> enumerator = new ShowMediaListEnumerator(aHidden);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = aLibrary->EnumerateItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                                   NS_LITERAL_STRING("1"),
                                                   enumerator,
                                                   sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  return rv;
}

nsresult sbBaseDevice::IgnoreMediaItem(sbIMediaItem * aItem) {
  return mLibraryListener->IgnoreMediaItem(aItem);
}

nsresult sbBaseDevice::UnignoreMediaItem(sbIMediaItem * aItem) {
  return mLibraryListener->UnignoreMediaItem(aItem);
}

nsresult
sbBaseDevice::DeleteItem(sbIMediaList *aLibrary, sbIMediaItem *aItem)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aItem);

  NS_ENSURE_STATE(mLibraryListener);

  SetIgnoreMediaListListeners(PR_TRUE);
  mLibraryListener->SetIgnoreListener(PR_TRUE);

  nsresult rv = aLibrary->Remove(aItem);

  SetIgnoreMediaListListeners(PR_FALSE);
  mLibraryListener->SetIgnoreListener(PR_FALSE);

  return rv;
}

nsresult
sbBaseDevice::GetItemContentType(sbIMediaItem* aMediaItem,
                                 PRUint32*     aContentType)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aContentType);

  // Function variables.
  nsresult rv;

  // Get the format type.
  sbExtensionToContentFormatEntry_t formatType;
  PRUint32                          bitRate;
  PRUint32                          sampleRate;
  rv = sbDeviceUtils::GetFormatTypeForItem(aMediaItem,
                                           formatType,
                                           bitRate,
                                           sampleRate);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    // Expected error no need to log it to the console just pass it on
    return NS_ERROR_NOT_AVAILABLE;
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  *aContentType = formatType.ContentType;

  return NS_OK;
}

nsresult
sbBaseDevice::UpdateOriginAndContentSrc(TransferRequest* aRequest,
                                        nsIURI*          aURI)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aURI);

  // Function variables.
  nsresult rv;

  // Ignore changes to the request item while it's being updated.
  sbDeviceListenerIgnore ignore(this, aRequest->item);

  // If the content source has not already been updated, update the content
  // origin.
  if (!aRequest->contentSrcSet) {
    // Copy the current content source URL to the origin URL.
    nsAutoString originURL;
    rv = aRequest->item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     originURL);
    if (NS_SUCCEEDED(rv)) {
      rv = aRequest->item->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                       originURL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Update the content source.
  rv = aRequest->item->SetContentSrc(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // The content source has now been set.
  aRequest->contentSrcSet = PR_TRUE;

  return NS_OK;
}

nsresult
sbBaseDevice::CreateTransferRequest(PRUint32 aRequestType,
                                    nsIPropertyBag2 *aRequestParameters,
                                    TransferRequest **aTransferRequest)
{
  NS_ENSURE_ARG_POINTER(aRequestParameters);
  NS_ENSURE_ARG_POINTER(aTransferRequest);


  nsresult rv;

  nsCOMPtr<sbIMediaItem> item;
  nsCOMPtr<sbIMediaList> list;
  nsCOMPtr<nsISupports>  data;

  PRUint32 index = PR_UINT32_MAX;
  PRUint32 otherIndex = PR_UINT32_MAX;

  rv = aRequestParameters->GetPropertyAsInterface(NS_LITERAL_STRING("item"),
                                                  NS_GET_IID(sbIMediaItem),
                                                  getter_AddRefs(item));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "No item present in request parameters.");

  rv = aRequestParameters->GetPropertyAsInterface(NS_LITERAL_STRING("list"),
                                                  NS_GET_IID(sbIMediaList),
                                                  getter_AddRefs(list));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "No list present in request parameters.");

  rv = aRequestParameters->GetPropertyAsInterface(NS_LITERAL_STRING("data"),
                                                  NS_GET_IID(nsISupports),
                                                  getter_AddRefs(data));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "No data present in request parameters.");

  NS_WARN_IF_FALSE(item || list || data,
                   "No data of any kind given in request."
                   "This request will most likely fail.");

  rv = aRequestParameters->GetPropertyAsUint32(NS_LITERAL_STRING("index"),
                                               &index);
  if(NS_FAILED(rv)) {
    index = PR_UINT32_MAX;
  }

  rv = aRequestParameters->GetPropertyAsUint32(NS_LITERAL_STRING("otherIndex"),
                                               &otherIndex);
  if(NS_FAILED(rv)) {
    otherIndex = PR_UINT32_MAX;
  }

  nsRefPtr<TransferRequest> req = TransferRequest::New(aRequestType,
                                                       item,
                                                       list,
                                                       index,
                                                       otherIndex,
                                                       data);
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);

  req.forget(aTransferRequest);

  return NS_OK;
}

nsresult sbBaseDevice::CreateAndDispatchEvent
                         (PRUint32 aType,
                          nsIVariant *aData,
                          PRBool aAsync /*= PR_TRUE*/,
                          sbIDeviceEventTarget* aTarget /*= nsnull*/)
{
  nsresult rv;

  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceStatus> status;
  rv = GetCurrentStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 subState = sbIDevice::STATE_IDLE;
  if (status) {
    rv = status->GetCurrentSubState(&subState);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(aType,
                            aData,
                            static_cast<sbIDevice*>(this),
                            mState,
                            subState,
                            getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dispatched;
  if (aTarget)
    return aTarget->DispatchEvent(deviceEvent, aAsync, &dispatched);
  return DispatchEvent(deviceEvent, aAsync, &dispatched);
}

nsresult sbBaseDevice::CreateAndDispatchDeviceManagerEvent
                         (PRUint32 aType,
                          nsIVariant *aData,
                          PRBool aAsync /*= PR_TRUE*/)
{
  nsresult rv;

  // Use the device manager as the event target.
  nsCOMPtr<sbIDeviceEventTarget> eventTarget =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return CreateAndDispatchEvent(aType, aData, aAsync, eventTarget);
}

nsresult
sbBaseDevice::GenerateFilename(sbIMediaItem * aItem,
                               nsACString & aFilename) {
  nsresult rv;
  nsCString filename;
  nsCString extension;

  nsCOMPtr<nsIURI> sourceContentURI;
  rv = aItem->GetContentSrc(getter_AddRefs(sourceContentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> sourceContentURL = do_QueryInterface(sourceContentURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = sourceContentURL->GetFileBaseName(filename);
    NS_ENSURE_SUCCESS(rv, rv);
    // If the filename already contains the extension then don't ask the URI
    // for it
    rv = sourceContentURL->GetFileExtension(extension);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Last ditch effort to figure out the filename/extension
    nsCString spec;
    rv = sourceContentURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 lastSlash = spec.RFind("/");
    if (lastSlash == -1) {
      lastSlash = 0;
    }
    PRInt32 lastPeriod = spec.RFind(".");
    if (lastPeriod == -1 || lastPeriod < lastSlash) {
      lastPeriod = spec.Length();
    }
    filename = Substring(spec, lastSlash + 1, lastPeriod - lastSlash - 1);
    extension = Substring(spec, lastPeriod + 1, spec.Length() - lastPeriod - 1);
  }
  aFilename = filename;
  if (!extension.IsEmpty()) {
    aFilename += NS_LITERAL_CSTRING(".");
    aFilename += extension;
  }
  return NS_OK;
}

nsresult
sbBaseDevice::CreateUniqueMediaFile(nsIURI  *aURI,
                                    nsIFile **aUniqueFile,
                                    nsIURI  **aUniqueURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv;

  // Get a clone of the URI.
  nsCOMPtr<nsIURI> uniqueURI;
  rv = aURI->Clone(getter_AddRefs(uniqueURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file URL.
  nsCOMPtr<nsIFileURL> uniqueFileURL = do_QueryInterface(uniqueURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object, invalidating the cache beforehand.
  nsCOMPtr<nsIFile> uniqueFile;
  rv = sbInvalidateFileURLCache(uniqueFileURL);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = uniqueFileURL->GetFile(getter_AddRefs(uniqueFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if file already exists.
  PRBool alreadyExists;
  rv = uniqueFile->Exists(&alreadyExists);
  NS_ENSURE_SUCCESS(rv, rv);

  // Try different file names until a unique one is found.  Limit the number of
  // unique names to try to the same limit as nsIFile.createUnique.
  for (PRUint32 uniqueIndex = 1;
       alreadyExists && (uniqueIndex < 10000);
       ++uniqueIndex) {
    // Get a clone of the URI.
    rv = aURI->Clone(getter_AddRefs(uniqueURI));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the file URL.
    uniqueFileURL = do_QueryInterface(uniqueURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the file base name.
    nsCAutoString fileBaseName;
    rv = uniqueFileURL->GetFileBaseName(fileBaseName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add a unique index to the file base name.
    fileBaseName.Append(" (");
    fileBaseName.AppendInt(uniqueIndex);
    fileBaseName.Append(")");
    rv = uniqueFileURL->SetFileBaseName(fileBaseName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the file object, invalidating the cache beforehand.
    rv = sbInvalidateFileURLCache(uniqueFileURL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = uniqueFileURL->GetFile(getter_AddRefs(uniqueFile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Check if file already exists.
    rv = uniqueFile->Exists(&alreadyExists);
    NS_ENSURE_SUCCESS(rv, rv);

    // Create file if it doesn't already exist.
    if (!alreadyExists) {
      rv = uniqueFile->Create(nsIFile::NORMAL_FILE_TYPE,
                              SB_DEFAULT_FILE_PERMISSIONS);
      if (rv == NS_ERROR_FILE_ALREADY_EXISTS) {
        alreadyExists = PR_TRUE;
        rv = NS_OK;
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Return results.
  if (aUniqueFile)
    uniqueFile.forget(aUniqueFile);
  if (aUniqueURI)
    uniqueURI.forget(aUniqueURI);

  return NS_OK;
}

nsresult
sbBaseDevice::RegenerateMediaURL(sbIMediaItem *aItem,
                                 nsIURI **_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMediaFileManager> fileMan =
    do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Init with nsnull to get the default file management behavior for
  // items in the main library:
  rv = fileMan->Init(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the path the item would be organized
  nsCOMPtr<nsIFile> mediaPath;
  rv = fileMan->GetManagedPath(aItem,
                               sbIMediaFileManager::MANAGE_MOVE |
                                 sbIMediaFileManager::MANAGE_COPY |
                                 sbIMediaFileManager::MANAGE_RENAME,
                               getter_AddRefs(mediaPath));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> parentDir;
  rv = mediaPath->GetParent(getter_AddRefs(parentDir));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool fileExists = PR_FALSE;
  rv = parentDir->Exists(&fileExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!fileExists) {
    rv = parentDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIURI> mediaURI;
  rv = sbNewFileURI(mediaPath, getter_AddRefs(mediaURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> mediaURL;
  mediaURL = do_QueryInterface(mediaURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(mediaURL, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ReqProcessingStart()
{
  // Process any queued requests.
  nsresult rv = mRequestThreadQueue->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ReqProcessingStop(nsIRunnable * aShutdownAction)
{
  nsresult rv = mRequestThreadQueue->Stop();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

bool
sbBaseDevice::CheckAndResetRequestAbort()
{
  return mRequestThreadQueue->CheckAndResetRequestAbort();
}

nsresult
sbBaseDevice::ClearRequests()
{
  mRequestThreadQueue->ClearRequests();
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDevice::CancelRequests()
{
  mRequestThreadQueue->CancelRequests();
  return NS_OK;
}

PRBool
sbBaseDevice::IsRequestAborted()
{
  bool aborted = mRequestThreadQueue->CheckAndResetRequestAbort();
  if (aborted) {
    return PR_TRUE;
  }
  PRUint32 deviceState;
  return NS_FAILED(GetState(&deviceState)) ||
      deviceState == sbIDevice::STATE_DISCONNECTED ? PR_TRUE : PR_FALSE;
}

template <class T>
inline
T find_iterator(T start, T end, T target)
{
  while (start != target && start != end) {
    ++start;
  }
  return start;
}

nsresult sbBaseDevice::EnsureSpaceForWrite(Batch & aBatch, bool aInSync)
{
  LOG(("                        sbBaseDevice::EnsureSpaceForWrite++\n"));

  nsresult rv;

  sbDeviceEnsureSpaceForWrite esfw(this, aInSync, aBatch);

  rv = esfw.EnsureSpace();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* a helper class to proxy sbBaseDevice::Init onto the main thread
 * needed because sbBaseDevice multiply-inherits from nsISupports, so
 * AddRef gets confused
 */
class sbBaseDeviceInitHelper : public nsRunnable
{
public:
  sbBaseDeviceInitHelper(sbBaseDevice* aDevice)
    : mDevice(aDevice) {
      NS_ADDREF(NS_ISUPPORTS_CAST(sbIDevice*, mDevice));
    }

  NS_IMETHOD Run() {
    mDevice->Init();
    return NS_OK;
  }

private:
  ~sbBaseDeviceInitHelper() {
    NS_ISUPPORTS_CAST(sbIDevice*, mDevice)->Release();
  }
  sbBaseDevice* mDevice;
};

nsresult sbBaseDevice::Init()
{
  nsresult rv;

  NS_ASSERTION(NS_IsMainThread(),
               "base device init not on main thread, implement proxying");
  if (!NS_IsMainThread()) {
    // we need to get the weak reference on the main thread because it is not
    // threadsafe, but we only ever use it from the main thread
    nsCOMPtr<nsIRunnable> event = new sbBaseDeviceInitHelper(this);
    return NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
  }

  mRequestThreadQueue = sbDeviceRequestThreadQueue::New(this);

  // get a weak ref of the device manager
  nsCOMPtr<nsISupportsWeakReference> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = manager->GetWeakReference(getter_AddRefs(mParentEventTarget));
  if (NS_FAILED(rv)) {
    mParentEventTarget = nsnull;
    return rv;
  }

  rv = GetMainLibrary(getter_AddRefs(mMainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the media folder URL table.
  NS_ENSURE_TRUE(mMediaFolderURLTable.Init(), NS_ERROR_OUT_OF_MEMORY);

  // Initialize the device properties.
  rv = InitializeProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  // Perform derived class intialization
  rv = InitDevice();
  NS_ENSURE_SUCCESS(rv, rv);

  // Perform initial properties update.
  UpdateProperties();

  // get transcoding stuff
  mDeviceTranscoding = new sbDeviceTranscoding(this);
  NS_ENSURE_TRUE(mDeviceTranscoding, NS_ERROR_OUT_OF_MEMORY);

  // get image stuff
  mDeviceImages = new sbDeviceImages(this);
  NS_ENSURE_TRUE(mDeviceImages, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbBaseDevice::GetMusicFreeSpace(sbILibrary* aLibrary,
                                PRInt64*    aFreeMusicSpace)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aFreeMusicSpace);

  // Function variables.
  nsresult rv;

  // Get the available music space.
  PRInt64 musicAvailableSpace;
  rv = GetMusicAvailableSpace(aLibrary, &musicAvailableSpace);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2>     deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the music used space.
  PRInt64 musicUsedSpace;
  nsAutoString musicUsedSpaceStr;
  rv = aLibrary->GetProperty
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MUSIC_USED_SPACE),
          musicUsedSpaceStr);
  NS_ENSURE_SUCCESS(rv, rv);
  musicUsedSpace = nsString_ToInt64(musicUsedSpaceStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return result.
  if (musicAvailableSpace >= musicUsedSpace)
    *aFreeMusicSpace = musicAvailableSpace - musicUsedSpace;
  else
    *aFreeMusicSpace = 0;

  return NS_OK;
}

nsresult
sbBaseDevice::GetMusicAvailableSpace(sbILibrary* aLibrary,
                                     PRInt64*    aMusicAvailableSpace)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMusicAvailableSpace);

  // Function variables.
  nsresult rv;

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2>     deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the total capacity.
  PRInt64 capacity;
  nsAutoString capacityStr;
  rv = aLibrary->GetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_CAPACITY),
                             capacityStr);
  NS_ENSURE_SUCCESS(rv, rv);
  capacity = nsString_ToInt64(capacityStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Compute the amount of available music space.
  PRInt64 musicAvailableSpace;
  if (mMusicLimitPercent < 100) {
    musicAvailableSpace = (capacity * mMusicLimitPercent) /
                          static_cast<PRInt64>(100);
  } else {
    musicAvailableSpace = capacity;
  }

  // Return results.
  *aMusicAvailableSpace = musicAvailableSpace;

  return NS_OK;
}

nsresult
sbBaseDevice::SupportsMediaItemDRM(sbIMediaItem* aMediaItem,
                                   PRBool        aReportErrors,
                                   PRBool*       _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // DRM is not supported by default.  Subclasses can override this.
  if (aReportErrors) {
    rv = DispatchTranscodeErrorEvent
           (aMediaItem, SBLocalizedString("transcode.file.drmprotected"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  *_retval = PR_FALSE;

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Device volume services.
//
//------------------------------------------------------------------------------

nsresult
sbBaseDevice::AddVolume(sbBaseDeviceVolume* aVolume)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  nsresult rv;

  // Add the volume to the volume lists and tables.
  nsAutoString volumeGUID;
  rv = aVolume->GetGUID(volumeGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    NS_ENSURE_TRUE(mVolumeList.AppendElement(aVolume), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(mVolumeGUIDTable.Put(volumeGUID, aVolume),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  // If the device is currently marked as "hidden" and a new volume was added,
  // reset the hidden property.
  nsCOMPtr<sbIDeviceProperties> deviceProperties;
  rv = GetProperties(getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isHidden = PR_FALSE;
  rv = deviceProperties->GetHidden(&isHidden);
  if (NS_SUCCEEDED(rv) && isHidden) {
    rv = deviceProperties->SetHidden(PR_FALSE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not mark device as not hidden!");
  }

  return NS_OK;
}

nsresult
sbBaseDevice::RemoveVolume(sbBaseDeviceVolume* aVolume)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  nsresult rv;
  PRBool isVolumeListEmpty = PR_FALSE;

  // If the device volume has a device library, get the library GUID.
  nsAutoString               libraryGUID;
  nsCOMPtr<sbIDeviceLibrary> library;
  rv = aVolume->GetDeviceLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  if (library)
    library->GetGuid(libraryGUID);

  // Remove the volume from the volume lists and tables.
  nsAutoString volumeGUID;
  rv = aVolume->GetGUID(volumeGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    mVolumeList.RemoveElement(aVolume);
    mVolumeGUIDTable.Remove(volumeGUID);
    if (!libraryGUID.IsEmpty())
      mVolumeLibraryGUIDTable.Remove(libraryGUID);
    if (mPrimaryVolume == aVolume)
      mPrimaryVolume = nsnull;

    isVolumeListEmpty = mVolumeList.IsEmpty();
  }

  // If the last volume has been ejected, mark this device as hidden
  if (isVolumeListEmpty) {
    nsCOMPtr<sbIDeviceProperties> deviceProperties;
    rv = GetProperties(getter_AddRefs(deviceProperties));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deviceProperties->SetHidden(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseDevice::GetVolumeForItem(sbIMediaItem*        aItem,
                               sbBaseDeviceVolume** aVolume)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  nsresult rv;

  // Get the item's library's guid.
  nsAutoString libraryGUID;
  nsCOMPtr<sbILibrary> library;
  rv = aItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = library->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume from the volume library GUID table.
  nsRefPtr<sbBaseDeviceVolume> volume;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    PRBool present = mVolumeLibraryGUIDTable.Get(libraryGUID,
                                                 getter_AddRefs(volume));
    NS_ENSURE_TRUE(present, NS_ERROR_NOT_AVAILABLE);
  }

  // Return results.
  volume.forget(aVolume);

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Device settings services.
//
//------------------------------------------------------------------------------

nsresult
sbBaseDevice::GetDeviceSettingsDocument
                (nsIDOMDocument** aDeviceSettingsDocument)
{
  // No device settings document.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);
  *aDeviceSettingsDocument = nsnull;
  return NS_OK;
}

nsresult
sbBaseDevice::GetDeviceSettingsDocument
                (nsIFile*         aDeviceSettingsFile,
                 nsIDOMDocument** aDeviceSettingsDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsFile);
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);

  // Function variables.
  nsresult rv;

  // If the device settings file does not exist, just return null.
  PRBool exists;
  rv = aDeviceSettingsFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    *aDeviceSettingsDocument = nsnull;
    return NS_OK;
  }

  // Get the device settings file URI spec.
  nsCAutoString    deviceSettingsURISpec;
  nsCOMPtr<nsIURI> deviceSettingsURI;
  rv = NS_NewFileURI(getter_AddRefs(deviceSettingsURI), aDeviceSettingsFile);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceSettingsURI->GetSpec(deviceSettingsURISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create an XMLHttpRequest object.
  nsCOMPtr<nsIXMLHttpRequest>
    xmlHttpRequest = do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIScriptSecurityManager> ssm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrincipal> principal;
  rv = ssm->GetSystemPrincipal(getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->Init(principal, nsnull, nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the device settings file document.
  rv = xmlHttpRequest->OpenRequest(NS_LITERAL_CSTRING("GET"),
                                   deviceSettingsURISpec,
                                   PR_FALSE,                  // async
                                   SBVoidString(),            // user
                                   SBVoidString());           // password
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->Send(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->GetResponseXML(aDeviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::GetDeviceSettingsDocument
                (nsTArray<PRUint8>& aDeviceSettingsContent,
                 nsIDOMDocument**   aDeviceSettingsDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);

  // Function variables.
  nsresult rv;

  // Parse the device settings document from the content.
  nsCOMPtr<nsIDOMParser> domParser = do_CreateInstance(NS_DOMPARSER_CONTRACTID,
                                                       &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = domParser->ParseFromBuffer(aDeviceSettingsContent.Elements(),
                                  aDeviceSettingsContent.Length(),
                                  "text/xml",
                                  aDeviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettingsDocument()
{
  nsresult rv;

  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Get the device settings document.  Do nothing if device settings document
  // is not available.
  nsCOMPtr<nsIDOMDocument> deviceSettingsDocument;
  rv = GetDeviceSettingsDocument(getter_AddRefs(deviceSettingsDocument));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!deviceSettingsDocument)
    return NS_OK;

  // Apply the device settings.
  rv = ApplyDeviceSettings(deviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettings(nsIDOMDocument* aDeviceSettingsDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);

  // Function variables.
  nsresult rv;

  // Apply the device friendly name setting.
  rv = ApplyDeviceSettingsToProperty
         (aDeviceSettingsDocument,
          NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  // Apply device settings device info.
  rv = ApplyDeviceSettingsDeviceInfo(aDeviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  // Apply the device capabilities.
  rv = ApplyDeviceSettingsToCapabilities(aDeviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettingsToProperty
                (nsIDOMDocument*  aDeviceSettingsDocument,
                 const nsAString& aPropertyName)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);
  NS_ENSURE_TRUE(StringBeginsWith(aPropertyName,
                                  NS_LITERAL_STRING(SB_DEVICE_PROPERTY_BASE)),
                 NS_ERROR_INVALID_ARG);

  // Function variables.
  NS_NAMED_LITERAL_STRING(devPropNS, SB_DEVICE_PROPERTY_NS);
  nsCOMPtr<nsIDOMNode> dummyNode;
  nsresult             rv;

  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Get the device property name suffix.
  nsAutoString propertyNameSuffix(Substring(aPropertyName,
                                            strlen(SB_DEVICE_PROPERTY_BASE)));

  // Get the device setting element.
  nsCOMPtr<nsIDOMElement>  deviceSettingElement;
  nsCOMPtr<nsIDOMNodeList> elementList;
  nsCOMPtr<nsIDOMNode>     elementNode;
  PRUint32                 elementCount;
  rv = aDeviceSettingsDocument->GetElementsByTagNameNS
                                  (devPropNS,
                                   propertyNameSuffix,
                                   getter_AddRefs(elementList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = elementList->GetLength(&elementCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (elementCount > 0) {
    rv = elementList->Item(0, getter_AddRefs(elementNode));
    NS_ENSURE_SUCCESS(rv, rv);
    deviceSettingElement = do_QueryInterface(elementNode, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Do nothing if device settings element does not exist.
  if (!deviceSettingElement)
    return NS_OK;

  // Apply the device setting to the property.
  nsAutoString  propertyValue;
  rv = deviceSettingElement->GetAttribute(NS_LITERAL_STRING("value"),
                                          propertyValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ApplyDeviceSettingsToProperty(aPropertyName,
                                     sbNewVariant(propertyValue));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettingsToProperty(const nsAString& aPropertyName,
                                            nsIVariant*      aPropertyValue)
{
  // Nothing for the base class to do.
  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettingsDeviceInfo
                (nsIDOMDocument* aDeviceSettingsDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);

  // Function variables.
  nsAutoPtr<nsString> folderURL;
  PRBool              needMediaFolderUpdate = PR_FALSE;
  PRBool              success;
  nsresult            rv;

  // Get the device info from the device settings document.  Do nothing if no
  // device info present.
  nsAutoPtr<sbDeviceXMLInfo> deviceXMLInfo(new sbDeviceXMLInfo(this));
  PRBool                     present;
  NS_ENSURE_TRUE(deviceXMLInfo, NS_ERROR_OUT_OF_MEMORY);
  rv = deviceXMLInfo->Read(aDeviceSettingsDocument);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceXMLInfo->GetDeviceInfoPresent(&present);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!present)
    return NS_OK;

  // Get device folders.
  for (PRUint32 i = 0;
       i < NS_ARRAY_LENGTH(sbBaseDeviceSupportedFolderContentTypeList);
       ++i) {
    // Get the next folder content type.
    PRUint32 folderContentType = sbBaseDeviceSupportedFolderContentTypeList[i];

    // Allocate a folder URL.
    nsAutoPtr<nsString> folderURL(new nsString());
    NS_ENSURE_TRUE(folderURL, NS_ERROR_OUT_OF_MEMORY);

    // Get the device folder URL and add it to the media folder URL table.
    rv = deviceXMLInfo->GetDeviceFolder(folderContentType, *folderURL);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!folderURL->IsEmpty()) {
      success = mMediaFolderURLTable.Put(folderContentType, folderURL);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      folderURL.forget();
      needMediaFolderUpdate = PR_TRUE;
    }
  }

  nsString excludedFolders;
  rv = deviceXMLInfo->GetExcludedFolders(excludedFolders);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device properties.
  nsCOMPtr<nsIWritablePropertyBag> deviceProperties;
  rv = GetWritableDeviceProperties(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!excludedFolders.IsEmpty()) {
    LOG(("Excluded Folders: %s",
         NS_LossyConvertUTF16toASCII(excludedFolders).BeginReading()));

    rv = deviceProperties->SetProperty(
                           NS_LITERAL_STRING(SB_DEVICE_PROPERTY_EXCLUDED_FOLDERS),
                           sbNewVariant(excludedFolders));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get import rules:
  nsCOMPtr<nsIArray> importRules;
  rv = deviceXMLInfo->GetImportRules(getter_AddRefs(importRules));
  NS_ENSURE_SUCCESS(rv, rv);

  // Stow the rules, if any, in the device properties:
  if (importRules) {
    nsCOMPtr<nsIWritablePropertyBag2> devProps2 =
      do_QueryInterface(deviceProperties, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = devProps2->SetPropertyAsInterface(
                    NS_LITERAL_STRING(SB_DEVICE_PROPERTY_IMPORT_RULES),
                    importRules);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update media folders if needed.  Ignore errors if media folders fail to
  // update.
  if (needMediaFolderUpdate)
    UpdateMediaFolders();

  // Log the device folders.
  LogDeviceFolders();

  // Determine if the device supports format.
  PRBool supportsFormat;
  rv = deviceXMLInfo->GetDoesDeviceSupportReformat(&supportsFormat);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceProperties->SetProperty(
      NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SUPPORTS_REFORMAT),
      sbNewVariant(supportsFormat));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ApplyDeviceSettingsToCapabilities
                (nsIDOMDocument* aDeviceSettingsDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceSettingsDocument);

  // Function variables.
  nsresult rv;

  // Get the device capabilities from the device settings document.
  nsCOMPtr<sbIDeviceCapabilities> deviceCaps;
  rv = sbDeviceXMLCapabilities::GetCapabilities(getter_AddRefs(deviceCaps),
                                                aDeviceSettingsDocument,
                                                this);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the device settings document has device capabilities, use them;
  // otherwise, continue using current device capabilities.
  if (deviceCaps)
    mCapabilities = deviceCaps;

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Device properties services.
//
//------------------------------------------------------------------------------

nsresult
sbBaseDevice::InitializeProperties()
{
  return NS_OK;
}


/**
 * Update the device properties.
 */

nsresult
sbBaseDevice::UpdateProperties()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  // Update the device statistics properties.
  rv = UpdateStatisticsProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the volume names.
  rv = UpdateVolumeNames();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Update the device property specified by aName.
 *
 * \param aName                 Name of property to update.
 */

nsresult
sbBaseDevice::UpdateProperty(const nsAString& aName)
{
  return NS_OK;
}


/**
 * Update the device statistics properties.
 */

nsresult
sbBaseDevice::UpdateStatisticsProperties()
{
  nsresult rv;

  // Get the list of all volumes.
  nsTArray< nsRefPtr<sbBaseDeviceVolume> > volumeList;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    volumeList = mVolumeList;
  }

  TRACE(("%s[%p] (%u volumes)", __FUNCTION__, this, volumeList.Length()));

  // Update the statistics properties for all volumes.
  for (PRUint32 i = 0; i < volumeList.Length(); i++) {
    // Get the volume.
    nsRefPtr<sbBaseDeviceVolume> volume = volumeList[i];

    // Skip volume if it's not mounted.
    PRBool isMounted;
    rv = volume->GetIsMounted(&isMounted);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isMounted) {
      #if PR_LOGGING
        nsString volumeGuid;
        rv = volume->GetGUID(volumeGuid);
        if (NS_FAILED(rv)) volumeGuid.AssignLiteral("<unknown guid>");
        TRACE(("%s[%p] - volume %s not mounted",
               __FUNCTION__,
               this,
               NS_ConvertUTF16toUTF8(volumeGuid).get()));
      #endif /* PR_LOGGING */
      continue;
    }

    // Get the volume library and statistics.
    nsCOMPtr<sbIDeviceLibrary>   deviceLibrary;
    nsRefPtr<sbDeviceStatistics> deviceStatistics;
    rv = volume->GetDeviceLibrary(getter_AddRefs(deviceLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = volume->GetStatistics(getter_AddRefs(deviceStatistics));
    NS_ENSURE_SUCCESS(rv, rv);

    // Update the volume statistics properties.
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MUSIC_ITEM_COUNT),
            sbAutoString(deviceStatistics->AudioCount()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %u",
           __FUNCTION__,
           this,
           "AudioCount",
           deviceStatistics->AudioCount()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MUSIC_USED_SPACE),
            sbAutoString(deviceStatistics->AudioUsed()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %llu",
           __FUNCTION__,
           this,
           "AudioUsed",
           deviceStatistics->AudioUsed()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MUSIC_TOTAL_PLAY_TIME),
            sbAutoString(deviceStatistics->AudioPlayTime()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %llu",
           __FUNCTION__,
           this,
           "AudioPlayTime",
           deviceStatistics->AudioPlayTime()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_VIDEO_ITEM_COUNT),
            sbAutoString(deviceStatistics->VideoCount()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %u",
           __FUNCTION__,
           this,
           "VideoCount",
           deviceStatistics->VideoCount()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_VIDEO_USED_SPACE),
            sbAutoString(deviceStatistics->VideoUsed()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %llu",
           __FUNCTION__,
           this,
           "VideoUsed",
           deviceStatistics->VideoUsed()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_VIDEO_TOTAL_PLAY_TIME),
            sbAutoString(deviceStatistics->VideoPlayTime()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %llu",
           __FUNCTION__,
           this,
           "VideoPlayTime",
           deviceStatistics->VideoPlayTime()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_IMAGE_ITEM_COUNT),
            sbAutoString(deviceStatistics->ImageCount()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %u",
           __FUNCTION__,
           this,
           "ImageCount",
           deviceStatistics->ImageCount()));
    rv = UpdateLibraryProperty
           (deviceLibrary,
            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_IMAGE_USED_SPACE),
            sbAutoString(deviceStatistics->ImageUsed()));
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("%s[%p]: %s = %llu",
           __FUNCTION__,
           this,
           "ImageUsed",
           deviceStatistics->ImageUsed()));
  }

  return NS_OK;
}


/**
 * Update the names of all device volumes.
 */

nsresult
sbBaseDevice::UpdateVolumeNames()
{
  // Get the list of all volumes.
  nsTArray< nsRefPtr<sbBaseDeviceVolume> > volumeList;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    volumeList = mVolumeList;
  }

  // Update the names for all volumes.
  for (PRUint32 i = 0; i < volumeList.Length(); i++) {
    // Update the volume name, continuing on error.
    UpdateVolumeName(volumeList[i]);
  }

  return NS_OK;
}


/**
 * Update the name of the device volume specified by aVolume.
 *
 * \param aVolume               Device volume for which to update name.
 */

nsresult
sbBaseDevice::UpdateVolumeName(sbBaseDeviceVolume* aVolume)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  nsresult rv;

  // Get the volume device library.
  nsCOMPtr<sbIDeviceLibrary> deviceLibrary;
  rv = aVolume->GetDeviceLibrary(getter_AddRefs(deviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume capacity.
  nsAutoString displayCapacity;
  nsAutoString capacity;
  if (deviceLibrary) {
    rv = deviceLibrary->GetProperty
                          (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_CAPACITY),
                           capacity);
    if (NS_SUCCEEDED(rv) && !capacity.IsEmpty()) {
      // Convert the capacity to a display capacity.
      nsCOMPtr<sbIPropertyUnitConverter> storageConverter =
        do_CreateInstance(SB_STORAGEPROPERTYUNITCONVERTER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = storageConverter->AutoFormat(capacity, -1, 1, displayCapacity);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Check if the volume is removable.
  PRBool storageRemovable = PR_FALSE;
  PRInt32 removable;
  rv = aVolume->GetRemovable(&removable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (removable >= 0) {
    storageRemovable = (removable != 0);
  }
  else {
    // Assume first volume is internal and all others are removable.
    PRUint32 volumeIndex;
    {
      nsAutoLock autoVolumeLock(mVolumeLock);
      volumeIndex = mVolumeList.IndexOf(aVolume);
    }
    NS_ASSERTION(volumeIndex != mVolumeList.NoIndex, "Volume not found");
    storageRemovable = (volumeIndex != 0);
  }

  // Produce the library name.
  nsAutoString       libraryName;
  nsTArray<nsString> libraryNameParams;
  libraryNameParams.AppendElement(displayCapacity);
  if (!storageRemovable) {
    if (!displayCapacity.IsEmpty()) {
      libraryName =
        SBLocalizedString("device.volume.internal.name_with_capacity",
                          libraryNameParams);
    }
    else {
      libraryName = SBLocalizedString("device.volume.internal.name");
    }
  }
  else {
    if (!displayCapacity.IsEmpty()) {
      libraryName =
        SBLocalizedString("device.volume.removable.name_with_capacity",
                          libraryNameParams);
    }
    else {
      libraryName = SBLocalizedString("device.volume.removable.name");
    }
  }

  // Update the library name if necessary.
  if (deviceLibrary) {
    nsAutoString currentLibraryName;
    rv = deviceLibrary->GetName(currentLibraryName);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!currentLibraryName.Equals(libraryName)) {
      rv = deviceLibrary->SetName(libraryName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Device preference services.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP sbBaseDevice::SetWarningDialogEnabled(const nsAString & aWarning, PRBool aEnabled)
{
  nsresult rv;

  // get the key for this warning
  nsString prefKey(NS_LITERAL_STRING(PREF_WARNING));
  prefKey.Append(aWarning);

  // have a variant to set
  nsCOMPtr<nsIWritableVariant> var =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = var->SetAsBool(aEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetPreference(prefKey, var);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::GetWarningDialogEnabled(const nsAString & aWarning, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // get the key for this warning
  nsString prefKey(NS_LITERAL_STRING(PREF_WARNING));
  prefKey.Append(aWarning);

  nsCOMPtr<nsIVariant> var;
  rv = GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // does the pref exist?
  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_EMPTY ||
      dataType == nsIDataType::VTYPE_VOID)
  {
    // by default warnings are enabled
    *_retval = PR_TRUE;
  } else {
    rv = var->GetAsBool(_retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::ResetWarningDialogs()
{
  nsresult rv;

  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // the key for all warnings
  nsString prefKey(NS_LITERAL_STRING(PREF_WARNING));

  // clear the prefs
  rv = prefBranch->DeleteBranch(NS_ConvertUTF16toUTF8(prefKey).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::OpenInputStream(nsIURI*          aURI,
                                            nsIInputStream** retval)
{
  return NS_ERROR_NOT_AVAILABLE;
}

nsresult sbBaseDevice::GetPrefBranch(const char *aPrefBranchName,
                                     nsIPrefBranch** aPrefBranch)
{
  NS_ENSURE_ARG_POINTER(aPrefBranch);
  nsresult rv;

  // If we're not on the main thread proxy the service
  PRBool const isMainThread = NS_IsMainThread();

  // get the prefs service
  nsCOMPtr<nsIPrefService> prefService;

  if (!isMainThread) {
    prefService = do_ProxiedGetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the pref branch
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(aPrefBranchName, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // If we're not on the main thread proxy the pref branch
  if (!isMainThread) {
    nsCOMPtr<nsIPrefBranch> proxy;
    rv = do_GetProxyForObject(target,
                              NS_GET_IID(nsIPrefBranch),
                              prefBranch,
                              nsIProxyObjectManager::INVOKE_SYNC |
                              nsIProxyObjectManager::FORCE_PROXY_CREATION,
                              getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);
    prefBranch.swap(proxy);
  }

  prefBranch.forget(aPrefBranch);

  return rv;
}

nsresult sbBaseDevice::GetPrefBranchRoot(nsACString& aRoot)
{
  nsresult rv;

  // get id of this device
  nsID* id;
  rv = GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  // get that as a string
  char idString[NSID_LENGTH];
  id->ToProvidedString(idString);
  NS_Free(id);

  // create the pref branch root
  aRoot.Assign(PREF_DEVICE_PREFERENCES_BRANCH);
  aRoot.Append(idString);
  aRoot.AppendLiteral(".preferences.");

  return NS_OK;
}

nsresult sbBaseDevice::GetPrefBranch(nsIPrefBranch** aPrefBranch)
{
  NS_ENSURE_ARG_POINTER(aPrefBranch);
  nsresult rv;

  // get the pref branch root
  nsCAutoString root;
  rv = GetPrefBranchRoot(root);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetPrefBranch(root.get(), aPrefBranch);
}

nsresult sbBaseDevice::GetPrefBranch(sbIDeviceLibrary* aLibrary,
                                     nsIPrefBranch**   aPrefBranch)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aPrefBranch);
  nsresult rv;

  // start with the pref branch root
  nsCAutoString prefKey;
  rv = GetPrefBranchRoot(prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // add the library pref key
  nsAutoString libraryGUID;
  rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  prefKey.Append(".library.");
  prefKey.Append(NS_ConvertUTF16toUTF8(libraryGUID));
  prefKey.Append(".");

  return GetPrefBranch(prefKey.get(), aPrefBranch);
}

nsresult sbBaseDevice::ApplyPreference(const nsAString& aPrefName,
                                       nsIVariant*      aPrefValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPrefValue);

  // Function variables.
  nsresult rv;

  // Check if it's a library preference.
  PRBool isLibraryPreference = GetIsLibraryPreference(aPrefName);

  // Apply preference.
  if (isLibraryPreference) {
    // Get the library preference info.
    nsCOMPtr<sbIDeviceLibrary> library;
    nsAutoString               libraryPrefBase;
    nsAutoString               libraryPrefName;
    rv = GetPreferenceLibrary(aPrefName,
                              getter_AddRefs(library),
                              libraryPrefBase);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = GetLibraryPreferenceName(aPrefName, libraryPrefBase, libraryPrefName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Apply the library preference.
    rv = ApplyLibraryPreference(library, libraryPrefName, aPrefValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

PRBool sbBaseDevice::GetIsLibraryPreference(const nsAString& aPrefName)
{
  return StringBeginsWith(aPrefName,
                          NS_LITERAL_STRING(PREF_DEVICE_LIBRARY_BASE));
}

nsresult sbBaseDevice::GetPreferenceLibrary(const nsAString&   aPrefName,
                                            sbIDeviceLibrary** aLibrary,
                                            nsAString&         aLibraryPrefBase)
{
  // Function variables.
  nsresult rv;

  // Get the device content.
  nsCOMPtr<sbIDeviceContent> content;
  rv = GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIArray> libraryList;
  rv = content->GetLibraries(getter_AddRefs(libraryList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search the libraries for a match.  Return results if found.
  PRUint32 libraryCount;
  rv = libraryList->GetLength(&libraryCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < libraryCount; i++) {
    // Get the next library.
    nsCOMPtr<sbIDeviceLibrary> library = do_QueryElementAt(libraryList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the library info.
    nsAutoString guid;
    rv = library->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Return library if it matches pref.
    nsAutoString libraryPrefBase;
    rv = GetLibraryPreferenceBase(library, libraryPrefBase);
    NS_ENSURE_SUCCESS(rv, rv);
    if (StringBeginsWith(aPrefName, libraryPrefBase)) {
      if (aLibrary)
        library.forget(aLibrary);
      aLibraryPrefBase.Assign(libraryPrefBase);
      return NS_OK;
    }
  }

  // Library not found.
  return NS_ERROR_NOT_AVAILABLE;
}

nsresult sbBaseDevice::GetLibraryPreference(sbIDeviceLibrary* aLibrary,
                                            const nsAString&  aLibraryPrefName,
                                            nsIVariant**      aPrefValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Get the library preference base.
  nsAutoString libraryPrefBase;
  rv = GetLibraryPreferenceBase(aLibrary, libraryPrefBase);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetLibraryPreference(libraryPrefBase, aLibraryPrefName, aPrefValue);
}

nsresult sbBaseDevice::GetLibraryPreference(const nsAString& aLibraryPrefBase,
                                            const nsAString& aLibraryPrefName,
                                            nsIVariant**     aPrefValue)
{
  // Get the full preference name.
  nsAutoString prefName(aLibraryPrefBase);
  prefName.Append(aLibraryPrefName);

  return GetPreference(prefName, aPrefValue);
}

nsresult sbBaseDevice::ApplyLibraryPreference
                         (sbIDeviceLibrary* aLibrary,
                          const nsAString&  aLibraryPrefName,
                          nsIVariant*       aPrefValue)
{
  nsresult rv;

  // Operate under the preference lock.
  nsAutoLock preferenceLock(mPreferenceLock);

  // Get the library pref base.
  nsAutoString prefBase;
  rv = GetLibraryPreferenceBase(aLibrary, prefBase);
  NS_ENSURE_SUCCESS(rv, rv);

  // If no preference name is specified, read and apply all library preferences.
  PRBool applyAll = PR_FALSE;
  if (aLibraryPrefName.IsEmpty())
    applyAll = PR_TRUE;

  // Apply music limit preference.
  if (applyAll ||
      aLibraryPrefName.EqualsLiteral("music_limit_percent") ||
      aLibraryPrefName.EqualsLiteral("use_music_limit_percent"))
  {
    // First ensure that the music limit percentage pref is enabled.
    PRBool shouldLimitMusicSpace = PR_FALSE;
    rv = GetShouldLimitMusicSpace(prefBase, &shouldLimitMusicSpace);
    if (NS_SUCCEEDED(rv) && shouldLimitMusicSpace) {
      PRUint32 musicLimitPercent = 100;
      rv = GetMusicLimitSpacePercent(prefBase, &musicLimitPercent);
      if (NS_SUCCEEDED(rv)) {
        // Finally, apply the preference value.
        mMusicLimitPercent = musicLimitPercent;
      }
    }
    else {
      // Music space limiting is disabled, set the limit percent to 100%.
      mMusicLimitPercent = 100;
    }
  }

  return ApplyLibraryOrganizePreference(aLibrary,
                                        aLibraryPrefName,
                                        prefBase,
                                        aPrefValue);
}

nsresult sbBaseDevice::ApplyLibraryOrganizePreference
                         (sbIDeviceLibrary* aLibrary,
                          const nsAString&  aLibraryPrefName,
                          const nsAString&  aLibraryPrefBase,
                          nsIVariant*       aPrefValue)
{
  nsresult rv;
  PRBool applyAll = aLibraryPrefName.IsEmpty();

  if (!applyAll && !StringBeginsWith(aLibraryPrefName,
                                     NS_LITERAL_STRING(PREF_ORGANIZE_PREFIX)))
  {
    return NS_OK;
  }

  nsString prefBase(aLibraryPrefBase);
  if (prefBase.IsEmpty()) {
    // Get the library pref base.
    rv = GetLibraryPreferenceBase(aLibrary, prefBase);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsString guidString;
  rv = aLibrary->GetGuid(guidString);
  NS_ENSURE_SUCCESS(rv, rv);
  nsID libraryGuid;
  PRBool success =
    libraryGuid.Parse(NS_LossyConvertUTF16toASCII(guidString).get());
  NS_ENSURE_TRUE(success, NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA);

  nsAutoPtr<OrganizeData> libraryDataReleaser;
  OrganizeData* libraryData;
  PRBool found = mOrganizeLibraryPrefs.Get(libraryGuid, &libraryData);
  if (!found) {
    libraryData = new OrganizeData;
    libraryDataReleaser = libraryData;
  }
  NS_ENSURE_TRUE(libraryData, NS_ERROR_OUT_OF_MEMORY);

  // Get the preference value.
  nsCOMPtr<nsIVariant> prefValue = aPrefValue;

  if (applyAll ||
      aLibraryPrefName.EqualsLiteral(PREF_ORGANIZE_ENABLED))
  {
    if (applyAll || !prefValue) {
      rv = GetLibraryPreference(prefBase,
                                NS_LITERAL_STRING(PREF_ORGANIZE_ENABLED),
                                getter_AddRefs(prefValue));
      if (NS_FAILED(rv))
        prefValue = nsnull;
    }
    if (prefValue) {
      PRUint16 dataType;
      rv = prefValue->GetDataType(&dataType);
      if (NS_SUCCEEDED(rv) && dataType == nsIDataType::VTYPE_BOOL) {
        rv = prefValue->GetAsBool(&libraryData->organizeEnabled);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  if (applyAll ||
      aLibraryPrefName.EqualsLiteral(PREF_ORGANIZE_DIR_FORMAT))
  {
    if (applyAll || !prefValue) {
      rv = GetLibraryPreference(prefBase,
                                NS_LITERAL_STRING(PREF_ORGANIZE_DIR_FORMAT),
                                getter_AddRefs(prefValue));
      if (NS_FAILED(rv))
        prefValue = nsnull;
    }
    if (prefValue) {
      PRUint16 dataType;
      rv = prefValue->GetDataType(&dataType);
      if (NS_SUCCEEDED(rv) && dataType != nsIDataType::VTYPE_EMPTY) {
        rv = prefValue->GetAsACString(libraryData->dirFormat);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  if (applyAll ||
      aLibraryPrefName.EqualsLiteral(PREF_ORGANIZE_FILE_FORMAT))
  {
    if (applyAll || !prefValue) {
      rv = GetLibraryPreference(prefBase,
                                NS_LITERAL_STRING(PREF_ORGANIZE_FILE_FORMAT),
                                getter_AddRefs(prefValue));
      if (NS_FAILED(rv))
        prefValue = nsnull;
    }
    if (prefValue) {
      PRUint16 dataType;
      rv = prefValue->GetDataType(&dataType);
      if (NS_SUCCEEDED(rv) && dataType != nsIDataType::VTYPE_EMPTY) {
        rv = prefValue->GetAsACString(libraryData->fileFormat);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  if (!found) {
    success = mOrganizeLibraryPrefs.Put(libraryGuid, libraryData);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    libraryDataReleaser.forget();
  }

  return NS_OK;
}

nsresult sbBaseDevice::GetLibraryPreferenceName
                         (const nsAString& aPrefName,
                          nsAString&       aLibraryPrefName)
{
  nsresult rv;

  // Get the preference library preference base.
  nsAutoString               libraryPrefBase;
  rv = GetPreferenceLibrary(aPrefName, nsnull, libraryPrefBase);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetLibraryPreferenceName(aPrefName, libraryPrefBase, aLibraryPrefName);
}

nsresult sbBaseDevice::GetLibraryPreferenceName
                         (const nsAString&  aPrefName,
                          const nsAString&  aLibraryPrefBase,
                          nsAString&        aLibraryPrefName)
{
  // Validate pref name.
  NS_ENSURE_TRUE(StringBeginsWith(aPrefName, aLibraryPrefBase),
                 NS_ERROR_ILLEGAL_VALUE);

  // Extract the library preference name.
  aLibraryPrefName.Assign(Substring(aPrefName, aLibraryPrefBase.Length()));

  return NS_OK;
}

nsresult sbBaseDevice::GetLibraryPreferenceBase(sbIDeviceLibrary* aLibrary,
                                                nsAString&        aPrefBase)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Get the library info.
  nsAutoString guid;
  rv = aLibrary->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the library preference base.
  aPrefBase.Assign(NS_LITERAL_STRING(PREF_DEVICE_LIBRARY_BASE));
  aPrefBase.Append(guid);
  aPrefBase.AppendLiteral(".");

  return NS_OK;
}

nsresult
sbBaseDevice::GetCapabilitiesPreference(nsIVariant** aCapabilities)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv;

  // Get the device settings document.
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = GetDeviceSettingsDocument(getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the device capabilities from the device settings document.
  if (domDocument) {
    nsCOMPtr<sbIDeviceCapabilities> deviceCapabilities;
    rv = sbDeviceXMLCapabilities::GetCapabilities
                                    (getter_AddRefs(deviceCapabilities),
                                     domDocument,
                                     this);
    NS_ENSURE_SUCCESS(rv, rv);

    // Return any device capabilities from the device settings document.
    if (deviceCapabilities) {
      sbNewVariant capabilitiesVariant(deviceCapabilities);
      NS_ENSURE_TRUE(capabilitiesVariant.get(), NS_ERROR_FAILURE);
      NS_ADDREF(*aCapabilities = capabilitiesVariant.get());
      return NS_OK;
    }
  }

  // Return no capabilities.
  sbNewVariant capabilitiesVariant;
  NS_ENSURE_TRUE(capabilitiesVariant.get(), NS_ERROR_FAILURE);
  NS_ADDREF(*aCapabilities = capabilitiesVariant.get());

  return NS_OK;
}

nsresult sbBaseDevice::GetLocalDeviceDir(nsIFile** aLocalDeviceDir)
{
  NS_ENSURE_ARG_POINTER(aLocalDeviceDir);
  PRBool   exists;
  nsresult rv;

  // Get the root of all local device directories, creating it if needed.
  nsCOMPtr<nsIFile> localDeviceDir;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(localDeviceDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = localDeviceDir->Append(NS_LITERAL_STRING("devices"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = localDeviceDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = localDeviceDir->Create(nsIFile::DIRECTORY_TYPE,
                                SB_DEFAULT_DIRECTORY_PERMISSIONS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // get id of this device
  nsID* id;
  rv = GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  // get that as a string
  char idString[NSID_LENGTH];
  id->ToProvidedString(idString);
  NS_Free(id);

  // Produce this device's sub directory name.
  nsAutoString deviceSubDirName;
  deviceSubDirName.Assign(NS_LITERAL_STRING("device"));
  deviceSubDirName.Append(NS_ConvertUTF8toUTF16(idString + 1, NSID_LENGTH - 3));

  // Remove any illegal characters.
  PRUnichar *begin, *end;
  for (deviceSubDirName.BeginWriting(&begin, &end); begin < end; ++begin) {
    if (*begin & (~0x7F)) {
      *begin = PRUnichar('_');
    }
  }
  deviceSubDirName.StripChars(FILE_ILLEGAL_CHARACTERS
                              FILE_PATH_SEPARATOR
                              " _-{}");

  // Get this device's local directory, creating it if needed.
  rv = localDeviceDir->Append(deviceSubDirName);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = localDeviceDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = localDeviceDir->Create(nsIFile::DIRECTORY_TYPE,
                                SB_DEFAULT_DIRECTORY_PERMISSIONS);
    #if PR_LOGGING
      if (NS_FAILED(rv)) {
        nsString path;
        nsresult rv2 = localDeviceDir->GetPath(path);
        if (NS_FAILED(rv2)) path.AssignLiteral("<unknown>");
        TRACE(("%s[%p]: Failed to create device directory %s",
               __FUNCTION__,
               this,
               NS_ConvertUTF16toUTF8(path).BeginReading()));
      }
    #endif /* PR_LOGGING */
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  localDeviceDir.forget(aLocalDeviceDir);

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Device sync services.
//
//------------------------------------------------------------------------------

nsresult
sbBaseDevice::SendSyncCompleteRequest()
{
  nsresult rv;

  nsCOMPtr<nsIWritablePropertyBag2> syncCompleteParams =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRUint64> timestamp =
    do_CreateInstance(NS_SUPPORTS_PRUINT64_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 now = PR_Now();
  rv = timestamp->SetData(now);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = syncCompleteParams->SetPropertyAsInterface(NS_LITERAL_STRING("data"),
                                                  timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = syncCompleteParams->SetPropertyAsInterface(NS_LITERAL_STRING("list"),
                                                  NS_ISUPPORTS_CAST(sbIMediaList*, mDefaultLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SubmitRequest(sbIDevice::REQUEST_SYNC_COMPLETE,
                     syncCompleteParams);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::HandleSyncRequest(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);

  // Function variables.
  nsresult rv;

  rv = sbDeviceUtils::SetLinkedSyncPartner(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if we should cache this sync for later
  if (mSyncState != sbBaseDevice::SYNC_STATE_NORMAL) {
    mSyncState = sbBaseDevice::SYNC_STATE_PENDING;
    return NS_OK;
  }

  // Ensure enough space is available for operation.  Cancel operation if not.
  PRBool abort;
  rv = EnsureSpaceForSync(aRequest, &abort);
  NS_ENSURE_SUCCESS(rv, rv);
  if (abort) {
    return NS_OK;
  }

  if (IsRequestAborted()) {
    return NS_ERROR_ABORT;
  }

  // Produce the sync change set.
  nsCOMPtr<sbILibraryChangeset> exportChangeset;
  nsCOMPtr<sbILibraryChangeset> importChangeset;
  rv = SyncProduceChangeset(aRequest,
                            getter_AddRefs(exportChangeset),
                            getter_AddRefs(importChangeset));
  NS_ENSURE_SUCCESS(rv, rv);

  if (IsRequestAborted()) {
    return NS_ERROR_ABORT;
  }

  // Setup the device state for sync'ing
  rv = SetState(STATE_SYNCING);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceStatus> status;
  rv = GetCurrentStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = status->SetCurrentState(STATE_SYNCING);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = status->SetCurrentSubState(STATE_SYNCING);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset the sync type for the upcoming sync
  mSyncType = TransferRequest::REQUESTBATCH_UNKNOWN;

  nsCOMPtr<sbIDeviceLibrary> devLib = do_QueryInterface(aRequest->list, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExportToDevice(devLib, exportChangeset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLib;
  rv = GetMainLibrary(getter_AddRefs(mainLib));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get the main library");

  rv = ImportFromDevice(mainLib, importChangeset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SendSyncCompleteRequest();
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the sync types (audio/video/image) after queuing requests
  aRequest->itemType = mSyncType;

  nsCOMPtr<sbIDeviceCapabilities> capabilities;
  rv = GetCapabilities(getter_AddRefs(capabilities));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSupported;
  rv = capabilities->SupportsContent(
                       sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY,
                       sbIDeviceCapabilities::CONTENT_IMAGE,
                       &isSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = GetPreference(NS_LITERAL_STRING(PREF_IMAGESYNC_ENABLED),
                     getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 dataType = 0;
  rv = var->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool imageSyncEnabled = PR_FALSE;
  // The preference is only available after user changes the image sync settings
  if (dataType == nsIDataType::VTYPE_BOOL) {
    rv = var->GetAsBool(&imageSyncEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Do not proceed to image sync request submission if image is not supported
  // or not enabled at all.
  if (!isSupported || !imageSyncEnabled) {
    return NS_OK;
  }

  // If the user has enabled image sync, trigger it after the audio/video sync
  // If the user has disabled image sync, trigger it to do the removal.
  nsCOMPtr<nsIWritablePropertyBag2> requestParams =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = requestParams->SetPropertyAsInterface
                        (NS_LITERAL_STRING("list"),
                         NS_ISUPPORTS_CAST(sbIMediaList*, mDefaultLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SubmitRequest(sbIDevice::REQUEST_IMAGESYNC, requestParams);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::HandleSyncCompletedRequest(TransferRequest* aRequest)
{
  NS_ENSURE_ARG_POINTER(aRequest);
  nsresult rv;

  if (IsRequestAborted()) {
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsISupportsPRUint64> timestamp =
      do_QueryInterface(aRequest->data, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 ts = 0;
  rv = timestamp->GetData(&ts);
  NS_ENSURE_SUCCESS(rv, rv);

  // This list is actually the device library.
  nsCOMPtr<sbIMediaList> list = aRequest->list;
  NS_ENSURE_TRUE(list, NS_ERROR_FAILURE);

  sbAutoString timestampStr((PRUint64)(ts / PR_MSEC_PER_SEC));
  rv = list->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_TIME),
                         timestampStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::EnsureSpaceForSync(TransferRequest* aRequest,
                                 PRBool*          aAbort)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aAbort);

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Get the sync source and destination libraries.
  nsCOMPtr<sbILibrary> srcLib = do_QueryInterface(aRequest->item, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDeviceLibrary> dstLib = do_QueryInterface(aRequest->list, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Allocate a list of sync items and a hash table of their sizes.
  nsCOMArray<sbIMediaItem>                     syncItemList;
  nsDataHashtable<nsISupportsHashKey, PRInt64> syncItemSizeMap;
  success = syncItemSizeMap.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Get the sync item sizes and total size.
  PRInt64 totalSyncSize;
  rv = SyncGetSyncItemSizes(srcLib,
                            dstLib,
                            syncItemList,
                            syncItemSizeMap,
                            &totalSyncSize);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the space available for syncing.
  PRInt64 availableSpace;
  rv = SyncGetSyncAvailableSpace(dstLib, &availableSpace);
  NS_ENSURE_SUCCESS(rv, rv);

  // If not enough space is available, ask the user what action to take.  If the
  // user does not abort the operation, create and sync to a sync media list
  // that will fit in the available space.  Otherwise, set the management type
  // to manual and return with abort.
  if (availableSpace < totalSyncSize && !mEnsureSpaceChecked) {
    // Ask the user what action to take.
    PRBool abort;
    rv = sbDeviceUtils::QueryUserSpaceExceeded(this,
                                               dstLib,
                                               totalSyncSize,
                                               availableSpace,
                                               &abort);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set sync mode to manual upon abort. Also clear the listeners and reset
    // the read-only bit.
    if (abort) {
      nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
      rv = dstLib->GetSyncSettings(getter_AddRefs(syncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = dstLib->SetSyncSettings(syncSettings);
      NS_ENSURE_SUCCESS(rv, rv);

      *aAbort = PR_TRUE;
      return NS_OK;
    }

    // Save the flag so that we won't ask user multiple times in one
    // syncing operation.
    mEnsureSpaceChecked = true;

    // Create a sync media list and sync to it.
    rv = SyncCreateAndSyncToList(srcLib,
                                 dstLib,
                                 syncItemList,
                                 syncItemSizeMap,
                                 availableSpace);
    if (rv == NS_ERROR_ABORT) {
      *aAbort = PR_TRUE;
      return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Don't abort.
  *aAbort = PR_FALSE;

  return NS_OK;
}

nsresult
sbBaseDevice::SyncCreateAndSyncToList
  (sbILibrary*                                   aSrcLib,
   sbIDeviceLibrary*                             aDstLib,
   nsCOMArray<sbIMediaItem>&                     aSyncItemList,
   nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
   PRInt64                                       aAvailableSpace)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aSrcLib);
  NS_ENSURE_ARG_POINTER(aDstLib);

  // Function variables.
  nsresult rv;

  // Clear sync playlist to prevent syncing while creating the sync playlist.
  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = aDstLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    // Skip image type since we don't support it right now.
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    rv = syncSettings->GetMediaSettings(i, getter_AddRefs(mediaSyncSettings));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mediaSyncSettings->ClearSelectedPlaylists();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mediaSyncSettings->SetMgmtType(
           sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aDstLib->SetSyncSettings(syncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for abort.
  NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

  // Create a new source sync media list.
  nsCOMPtr<sbIMediaList> syncMediaList;
  rv = SyncCreateSyncMediaList(aSrcLib,
                               aDstLib,
                               aAvailableSpace,
                               getter_AddRefs(syncMediaList));
  if (rv == NS_ERROR_ABORT)
    return rv;
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for abort.
  NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

  // Sync to the sync media list.
  rv = SyncToMediaList(aDstLib, syncMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::SyncCreateSyncMediaList(sbILibrary*       aSrcLib,
                                      sbIDeviceLibrary* aDstLib,
                                      PRInt64           aAvailableSpace,
                                      sbIMediaList**    aSyncMediaList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aSrcLib);
  NS_ENSURE_ARG_POINTER(aSyncMediaList);

  // Function variables.
  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibrary> devLib;
  rv = GetDefaultLibrary(getter_AddRefs(devLib));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString guid;
  rv = devLib->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(
                        NS_LITERAL_STRING(SB_PROPERTY_DEVICE_LIBRARY_GUID),
                        guid);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = propertyArray->AppendProperty(
                        NS_LITERAL_STRING(SB_PROPERTY_LISTTYPE),
                        NS_LITERAL_STRING("2"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = propertyArray->AppendProperty(
                        NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                        NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> randomMediaList;
  rv = aSrcLib->GetItemsByProperties(propertyArray,
                                     getter_AddRefs(randomMediaList));
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 length;
  rv = randomMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_WARN_IF_FALSE(length < 2, "Multiple random sync'ing playlists");
  nsCOMPtr<sbIMediaList> syncMediaList;
  if (length > 0) {
    rv = randomMediaList->QueryElementAt(0,
                                         NS_GET_IID(sbIMediaList),
                                         getter_AddRefs(syncMediaList));
    NS_ENSURE_SUCCESS(rv, rv);

    // Calculate the smart list total item size. Return the list if the size
    // can fit on the device. Create a new one if not.
    PRInt64 totalSyncSize = 0;
    PRUint32 itemCount;
    rv = syncMediaList->GetLength(&itemCount);
    NS_ENSURE_SUCCESS(rv, rv);
    for (PRUint32 i = 0; i < itemCount; ++i) {
      // Get the sync item.
      nsCOMPtr<sbIMediaItem> mediaItem;
      rv = syncMediaList->GetItemByIndex(i, getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);

      // Media List inside of media list?
      nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem, &rv);
      if (NS_SUCCEEDED(rv))
        continue;

      // Get the item write length adding in the per track overhead.  Assume a
      // length of 0 on error.
      PRUint64 writeLength;
      rv = sbDeviceUtils::GetDeviceWriteLength(aDstLib,
                                               mediaItem,
                                               &writeLength);
      if (NS_FAILED(rv))
        writeLength = 0;
      writeLength += mPerTrackOverhead;

      totalSyncSize += writeLength;
    }

    if (totalSyncSize <= aAvailableSpace) {
      // smart playlist found.
      syncMediaList.forget(aSyncMediaList);
      return NS_OK;
    }
    else {
      // If not enough space is available to hold the existing smart list
      // content, ask the user what action to take. If the user does not abort
      // the operation, create another smart list and sync to it that will fit
      // in the available space.  Otherwise, set the management type to manual
      // and return.
      PRBool abort;
      rv = sbDeviceUtils::QueryUserSpaceExceeded(this,
                                                 aDstLib,
                                                 totalSyncSize,
                                                 aAvailableSpace,
                                                 &abort);
      NS_ENSURE_SUCCESS(rv, rv);
      if (abort)
        return NS_ERROR_ABORT;

      // Reset the library guid property to ignore for next over-capacity sync.
      // And create another smart list.
      rv = syncMediaList->SetProperty(
                            NS_LITERAL_STRING(SB_PROPERTY_DEVICE_LIBRARY_GUID),
                            NS_LITERAL_STRING(""));
    }
  }

  // Create a new source sync smart media list.
  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> proxiedLibrary;
  rv = do_GetProxyForObject(target,
                            aSrcLib,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxiedLibrary->CreateMediaList(NS_LITERAL_STRING("smart"),
                                       propertyArray,
                                       getter_AddRefs(syncMediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the device name
  nsString deviceName;
  rv = GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the sync media list name.
  nsString listName;
  nsTArray<nsString> listNameParams;
  listNameParams.AppendElement(deviceName);
  rv = SBGetLocalizedFormattedString(listName,
                                     NS_LITERAL_STRING(RANDOM_LISTNAME),
                                     listNameParams,
                                     NS_LITERAL_STRING("Autofill"),
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString uniqueName;
  rv = sbLibraryUtils::SuggestUniqueNameForPlaylist(aSrcLib,
                                                    listName,
                                                    uniqueName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = syncMediaList->SetName(uniqueName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSmartMediaList> randomList =
    do_QueryInterface(syncMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the smart list condition to be "media type is audio" with limited
  // storage capacity and select by random.
  nsCOMPtr<sbIPropertyOperator> equal;
  rv = sbLibraryUtils::GetEqualOperator(getter_AddRefs(equal));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSmartMediaListCondition> condition;
  rv = randomList->AppendCondition(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                   equal,
                                   NS_LITERAL_STRING("audio"),
                                   nsString(),
                                   nsString(),
                                   getter_AddRefs(condition));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = randomList->SetMatchType(sbILocalDatabaseSmartMediaList::MATCH_TYPE_ALL);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = randomList->SetLimitType(
                     sbILocalDatabaseSmartMediaList::LIMIT_TYPE_BYTES);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 syncSpace = aAvailableSpace * SYNC_PLAYLIST_AVAILABLE_PCT / 100;

  nsString uiLimitType;
  uiLimitType.AssignLiteral("GB");
  syncSpace = syncSpace / BYTES_PER_10MB * BYTES_PER_10MB;

  rv = randomList->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_UILIMITTYPE),
                               uiLimitType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = randomList->SetLimit(syncSpace);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = randomList->SetRandomSelection(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Rebuild after the conditions are set.
  rv = randomList->Rebuild();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  syncMediaList.forget(aSyncMediaList);

  return NS_OK;
}

nsresult
sbBaseDevice::SyncToMediaList(sbIDeviceLibrary* aDstLib,
                              sbIMediaList*     aMediaList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDstLib);
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Function variables.
  nsresult rv;

  PRUint16 contentType;
  rv = aMediaList->GetListContentType(&contentType);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(contentType > 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = aDstLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> audioMediaSyncSettings;
  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                      getter_AddRefs(audioMediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> videoMediaSyncSettings;
  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_VIDEO,
                                      getter_AddRefs(videoMediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> syncList =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = syncList->AppendElement(aMediaList, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  // Set to sync to the sync media list.
  if (contentType & sbIMediaList::CONTENTTYPE_AUDIO) {
    rv = audioMediaSyncSettings->SetSelectedPlaylists(syncList);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = videoMediaSyncSettings->ClearSelectedPlaylists();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (contentType & sbIMediaList::CONTENTTYPE_VIDEO) {
    rv = videoMediaSyncSettings->SetSelectedPlaylists(syncList);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = audioMediaSyncSettings->ClearSelectedPlaylists();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = audioMediaSyncSettings->SetMgmtType(
         sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mgmtType;
  if (contentType == sbIMediaList::CONTENTTYPE_AUDIO)
    mgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE;
  else
    mgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS;
  rv = videoMediaSyncSettings->SetMgmtType(mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDstLib->SetSyncSettings(syncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::SyncGetSyncItemSizes
  (sbILibrary*                                   aSrcLib,
   sbIDeviceLibrary*                             aDstLib,
   nsCOMArray<sbIMediaItem>&                     aSyncItemList,
   nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
   PRInt64*                                      aTotalSyncSize)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aSrcLib);
  NS_ENSURE_ARG_POINTER(aDstLib);
  NS_ENSURE_ARG_POINTER(aTotalSyncSize);

  // Function variables.
  nsresult rv;

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = aDstLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of sync media lists.
  nsCOMPtr<nsIArray> syncList;
  rv = syncSettings->GetSyncPlaylists(getter_AddRefs(syncList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Fill in the sync item size table and calculate the total sync size.
  PRInt64 totalSyncSize = 0;
  PRUint32 syncListLength;
  rv = syncList->GetLength(&syncListLength);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < syncListLength; i++) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the next sync media item.
    nsCOMPtr<sbIMediaItem> syncMI = do_QueryElementAt(syncList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the sync media list.
    nsCOMPtr<sbIMediaList> syncML = do_QueryInterface(syncMI, &rv);

    // Not media list.
    if (NS_FAILED(rv)) {
      // Get the size of the sync media item.
      rv = SyncGetSyncItemSizes(syncMI,
                                aDstLib,
                                aSyncItemList,
                                aSyncItemSizeMap,
                                &totalSyncSize);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      // Get the sizes of the sync media items.
      rv = SyncGetSyncItemSizes(syncML,
                                aDstLib,
                                aSyncItemList,
                                aSyncItemSizeMap,
                                &totalSyncSize);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Return results.
  *aTotalSyncSize = totalSyncSize;

  return NS_OK;
}

nsresult
sbBaseDevice::SyncGetSyncItemSizes
  (sbIMediaList*                                 aSyncML,
   sbIDeviceLibrary*                             aDstLib,
   nsCOMArray<sbIMediaItem>&                     aSyncItemList,
   nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
   PRInt64*                                      aTotalSyncSize)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aSyncML);
  NS_ENSURE_ARG_POINTER(aTotalSyncSize);

  // Function variables.
  nsresult rv;

  // Accumulate the sizes of all sync items.  Use GetItemByIndex like the
  // diffing service does.
  PRInt64  totalSyncSize = *aTotalSyncSize;
  PRUint32 itemCount;
  rv = aSyncML->GetLength(&itemCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < itemCount; i++) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the sync item.
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = aSyncML->GetItemByIndex(i, getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Ignore media lists.
    nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem, &rv);
    if (NS_SUCCEEDED(rv))
      continue;

    rv = SyncGetSyncItemSizes(mediaItem,
                              aDstLib,
                              aSyncItemList,
                              aSyncItemSizeMap,
                              &totalSyncSize);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  *aTotalSyncSize = totalSyncSize;

  return NS_OK;
}

nsresult
sbBaseDevice::SyncGetSyncItemSizes
  (sbIMediaItem*                                 aSyncMI,
   sbIDeviceLibrary*                             aDstLib,
   nsCOMArray<sbIMediaItem>&                     aSyncItemList,
   nsDataHashtable<nsISupportsHashKey, PRInt64>& aSyncItemSizeMap,
   PRInt64*                                      aTotalSyncSize)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aSyncMI);
  NS_ENSURE_ARG_POINTER(aTotalSyncSize);

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Accumulate the sizes of the sync item.
  PRInt64  totalSyncSize = *aTotalSyncSize;

  // Do nothing more if item is already in list.
  if (aSyncItemSizeMap.Get(aSyncMI, nsnull))
    return NS_OK;

  // Get the item write length adding in the per track overhead.  Assume a
  // length of 0 on error.
  PRUint64 writeLength;
  rv = sbDeviceUtils::GetDeviceWriteLength(aDstLib, aSyncMI, &writeLength);
  if (NS_FAILED(rv))
    writeLength = 0;
  writeLength += mPerTrackOverhead;

  // Add item.
  success = aSyncItemList.AppendObject(aSyncMI);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  success = aSyncItemSizeMap.Put(aSyncMI, writeLength);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  totalSyncSize += writeLength;

  // Return results.
  *aTotalSyncSize = totalSyncSize;

  return NS_OK;
}

nsresult
sbBaseDevice::SyncGetSyncAvailableSpace(sbILibrary* aLibrary,
                                        PRInt64*    aAvailableSpace)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aAvailableSpace);

  // Function variables.
  nsresult rv;

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2>     deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the free space.
  PRInt64 freeSpace;
  nsAutoString freeSpaceStr;
  rv = aLibrary->GetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_FREE_SPACE),
                             freeSpaceStr);
  NS_ENSURE_SUCCESS(rv, rv);
  freeSpace = nsString_ToInt64(freeSpaceStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the music used space.
  PRInt64 musicUsedSpace;
  nsAutoString musicUsedSpaceStr;
  rv = aLibrary->GetProperty
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MUSIC_USED_SPACE),
          musicUsedSpaceStr);
  NS_ENSURE_SUCCESS(rv, rv);
  musicUsedSpace = nsString_ToInt64(musicUsedSpaceStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add track overhead to the music used space.
  PRUint32 trackCount;
  rv = aLibrary->GetLength(&trackCount);
  NS_ENSURE_SUCCESS(rv, rv);
  musicUsedSpace += trackCount * mPerTrackOverhead;

  // Determine the total available space for syncing as the free space plus the
  // space used for music.
  PRInt64 availableSpace = freeSpace + musicUsedSpace;

  // Apply limit to the total space available for music.
  PRInt64 musicAvailableSpace;
  rv = GetMusicAvailableSpace(aLibrary, &musicAvailableSpace);
  NS_ENSURE_SUCCESS(rv, rv);
  if (availableSpace >= musicAvailableSpace)
    availableSpace = musicAvailableSpace;

  // Return results.
  *aAvailableSpace = availableSpace;

  return NS_OK;
}

namespace {

  nsresult GetMediaSettingsValues(sbIDeviceLibrarySyncSettings * aSyncSettings,
                                  PRUint32 aMediaType,
                                  PRUint32 * aMgmtType,
                                  PRBool * aImport,
                                  nsIMutableArray * aSelectedPlaylists)
  {
    NS_ENSURE_ARG_POINTER(aSyncSettings);
    NS_ENSURE_ARG_POINTER(aMgmtType);
    NS_ENSURE_ARG_POINTER(aImport);
    NS_ENSURE_ARG_POINTER(aSelectedPlaylists);

    nsresult rv;

    nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> settings;
    rv = aSyncSettings->GetMediaSettings(aMediaType,
                                         getter_AddRefs(settings));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = settings->GetMgmtType(aMgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (*aMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
      nsCOMPtr<nsIArray> playlists;
      rv = settings->GetSelectedPlaylists(getter_AddRefs(playlists));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = sbAppendnsIArray(playlists,
                            aSelectedPlaylists);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = settings->GetImport(aImport);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
}

nsresult
sbBaseDevice::SyncProduceChangeset(TransferRequest*      aRequest,
                                   sbILibraryChangeset** aExportChangeset,
                                   sbILibraryChangeset** aImportChangeset)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aExportChangeset);
  NS_ENSURE_ARG_POINTER(aImportChangeset);

  // Function variables.
  nsresult rv;

  // Get the sync source and destination libraries.
  nsCOMPtr<sbILibrary> mainLib = do_QueryInterface(aRequest->item, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDeviceLibrary> devLib = do_QueryInterface(aRequest->list, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = devLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> selectedPlaylists =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

  PRUint32 audioMgmtType;
  PRBool audioImport;
  rv = GetMediaSettingsValues(syncSettings,
                              sbIDeviceLibrary::MEDIATYPE_AUDIO,
                              &audioMgmtType,
                              &audioImport,
                              selectedPlaylists);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 videoMgmtType;
  PRBool videoImport;
  rv = GetMediaSettingsValues(syncSettings,
                              sbIDeviceLibrary::MEDIATYPE_VIDEO,
                              &videoMgmtType,
                              &videoImport,
                              selectedPlaylists);
  NS_ENSURE_SUCCESS(rv, rv);

  // Figure out the sync media types we're interested in
  PRUint32 syncMediaTypes = 0;
  if (audioMgmtType != sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE) {
    syncMediaTypes = sbIDeviceLibrarySyncDiff::SYNC_TYPE_AUDIO;
  }
  if (videoMgmtType != sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE) {
    syncMediaTypes |= sbIDeviceLibrarySyncDiff::SYNC_TYPE_VIDEO;
  }

  PRUint32 syncFlag = sbIDeviceLibrarySyncDiff::SYNC_FLAG_EXPORT_PLAYLISTS;
  // If we're sync'ing everything or everything of one but not the other there
  // is no need for playlists
  if ((audioMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL &&
         videoMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) ||
      (audioMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL &&
         videoMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE) ||
      (audioMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE &&
         videoMgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL)) {
    selectedPlaylists = nsnull;
    syncFlag = sbIDeviceLibrarySyncDiff::SYNC_FLAG_EXPORT_ALL;
  }

  // If we're importing either audio, video, or both set the import flag
  if (audioImport || videoImport) {
    syncFlag |= sbIDeviceLibrarySyncDiff::SYNC_FLAG_IMPORT;
  }

  PRBool useOriginForPlaylists;
  rv = GetUseOriginForPlaylists(&useOriginForPlaylists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibrarySyncDiff> syncDiff =
    do_CreateInstance(SONGBIRD_DEVICELIBRARYSYNCDIFF_CONTRACTID, &rv);

  rv = syncDiff->GenerateSyncLists(syncFlag,
                                   useOriginForPlaylists,
                                   syncMediaTypes,
                                   mainLib,
                                   devLib,
                                   selectedPlaylists,
                                   aExportChangeset,
                                   aImportChangeset);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDevice::ExportToDevice(sbIDeviceLibrary*    aDevLibrary,
                             sbILibraryChangeset* aChangeset)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevLibrary);
  NS_ENSURE_ARG_POINTER(aChangeset);

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Create some change list arrays.
  nsCOMArray<sbIMediaList>     addMediaListList;
  nsCOMArray<sbILibraryChange> mediaListChangeList;
  nsCOMPtr<nsIMutableArray>    addItemList =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  bool const playlistsSupported = sbDeviceUtils::ArePlaylistsSupported(this);

  // Get the list of all changes.
  nsCOMPtr<nsIArray> changeList;
  PRUint32           changeCount;
  rv = aChangeset->GetChanges(getter_AddRefs(changeList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = changeList->GetLength(&changeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Group changes for later processing but apply property updates immediately.
  for (PRUint32 i = 0; i < changeCount; i++) {
    if (IsRequestAborted()) {
      return NS_ERROR_ABORT;
    }
    // Get the next change.
    nsCOMPtr<sbILibraryChange> change = do_QueryElementAt(changeList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Put the change into the appropriate list.
    PRUint32 operation;
    rv = change->GetOperation(&operation);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add item to add media list list or add item list.
    PRBool itemIsList;
    rv = change->GetItemIsList(&itemIsList);
    NS_ENSURE_SUCCESS(rv, rv);

    // if this is a playlist and they're not supported ignore the change
    if (itemIsList && !playlistsSupported) {
      continue;
    }

    switch (operation)
    {
      case sbIChangeOperation::DELETED:
        {
          // We no longer remove items from the device
        } break;

      case sbIChangeOperation::ADDED:
        {
          // Get the source item to add.
          nsCOMPtr<sbIMediaItem> mediaItem;
          rv = change->GetSourceItem(getter_AddRefs(mediaItem));
          NS_ENSURE_SUCCESS(rv, rv);

          if (itemIsList) {
            nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem,
                                                                 &rv);
            NS_ENSURE_SUCCESS(rv, rv);
            rv = addMediaListList.AppendObject(mediaList);
            NS_ENSURE_SUCCESS(rv, rv);
          } else {
            rv = addItemList->AppendElement(mediaItem, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        } break;

      case sbIChangeOperation::MODIFIED:
        {
          // Get the source item that changed.
          nsCOMPtr<sbIMediaItem> mediaItem;
          rv = change->GetSourceItem(getter_AddRefs(mediaItem));
          NS_ENSURE_SUCCESS(rv, rv);

          // if it's hidden, don't sync it
          nsString hidden;
          rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                      hidden);
          if (rv != NS_ERROR_NOT_AVAILABLE) {
            NS_ENSURE_SUCCESS(rv, rv);
          }
          // If the change is to a media list, add it to the media list change
          // list.
          if (itemIsList) {
            success = mediaListChangeList.AppendObject(change);
            NS_ENSURE_SUCCESS(success, NS_ERROR_FAILURE);
          }

          // Update the item properties.
          rv = SyncUpdateProperties(change);
          NS_ENSURE_SUCCESS(rv, rv);
        } break;

      default:
        break;
    }
  }

  if (IsRequestAborted()) {
    return NS_ERROR_ABORT;
  }

  // Add items.
  nsCOMPtr<nsISimpleEnumerator> addItemEnum;
  rv = addItemList->Enumerate(getter_AddRefs(addItemEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDevLibrary->AddSome(addItemEnum);
  NS_ENSURE_SUCCESS(rv, rv);

  if (IsRequestAborted()) {
    rv = sbDeviceUtils::DeleteByProperty(aDevLibrary,
                                         NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                         NS_LITERAL_STRING("1"));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to remove partial added items");
    return NS_ERROR_ABORT;
  }

  // Add media lists.
  PRInt32 count = addMediaListList.Count();
  for (PRInt32 i = 0; i < count; i++) {
    if (IsRequestAborted()) {
      return NS_ERROR_ABORT;
    }
    rv = SyncAddMediaList(aDevLibrary, addMediaListList[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Sync the media lists.
  rv = SyncMediaLists(mediaListChangeList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::SyncAddMediaList(sbIDeviceLibrary* aDstLibrary,
                               sbIMediaList*     aMediaList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDstLibrary);
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Function variables.
  nsresult rv;

  // Don't sync media list if it shouldn't be synced.
  PRBool shouldSync = PR_FALSE;
  rv = ShouldSyncMediaList(aMediaList, &shouldSync);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!shouldSync)
    return NS_OK;

  // Create a main thread destination library proxy to add the media lists.
  // This ensures that the library notification for the added media list is
  // delivered and processed before media list items are added.  This ensures
  // that a device listener is added for the added media list in time to get
  // notifications for the added media list items.
  nsCOMPtr<sbIDeviceLibrary> proxyDstLibrary;
  rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(sbIDeviceLibrary),
                            aDstLibrary,
                            nsIProxyObjectManager::INVOKE_SYNC |
                            nsIProxyObjectManager::FORCE_PROXY_CREATION,
                            getter_AddRefs(proxyDstLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Copy the media list.  Don't use the add method because it disables
  // notifications until after the media list and its items are added.
  nsCOMPtr<sbIMediaList> mediaList;
  rv = proxyDstLibrary->CopyMediaList(NS_LITERAL_STRING("simple"),
                                      aMediaList,
                                      PR_FALSE,
                                      getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::SyncMediaLists(nsCOMArray<sbILibraryChange>& aMediaListChangeList)
{
  nsresult rv;

  // Just replace the destination media lists content with the sources
  // TODO: See if just applying changes is more efficient than clearing and
  // addAll

  // Sync each media list.
  PRInt32 count = aMediaListChangeList.Count();
  for (PRInt32 i = 0; i < count; i++) {
    // Get the media list change.
    nsCOMPtr<sbILibraryChange> change = aMediaListChangeList[i];

    // Get the destination media list item and library.
    nsCOMPtr<sbIMediaItem>     sourceMediaListItem;
    rv = change->GetSourceItem(getter_AddRefs(sourceMediaListItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> sourceMediaList =
        do_QueryInterface(sourceMediaListItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the destination media list item and library.
    nsCOMPtr<sbIMediaItem> destMediaListItem;
    rv = change->GetDestinationItem(getter_AddRefs(destMediaListItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> destMediaList =
        do_QueryInterface(destMediaListItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = destMediaList->Clear();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = destMediaList->AddAll(sourceMediaList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseDevice::SyncUpdateProperties(sbILibraryChange* aChange)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aChange);

  // Function variables.
  nsresult rv;

  // Get the item to update.
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = aChange->GetDestinationItem(getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of properties to update.
  nsCOMPtr<nsIArray> propertyList;
  PRUint32           propertyCount;
  rv = aChange->GetProperties(getter_AddRefs(propertyList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = propertyList->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update properties.
  for (PRUint32 i = 0; i < propertyCount; i++) {
    // Get the next property to update.
    nsCOMPtr<sbIPropertyChange>
      property = do_QueryElementAt(propertyList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property info.
    nsAutoString propertyID;
    nsAutoString propertyValue;
    nsAutoString oldPropertyValue;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = property->GetNewValue(propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = property->GetOldValue(oldPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    // Don't sync properties set by the device or transcoding.
    if (propertyID.Equals(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL)) ||
        propertyID.Equals
                     (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID)) ||
        propertyID.Equals(NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_PLAYCOUNT)) ||
        propertyID.Equals(NS_LITERAL_STRING(SB_PROPERTY_LAST_SYNC_SKIPCOUNT)) ||
        propertyID.Equals(NS_LITERAL_STRING(SB_PROPERTY_BITRATE))) {
      continue;
    }

    // Merge the property, ignoring errors.
    SyncMergeProperty(mediaItem, propertyID, propertyValue, oldPropertyValue);
  }

  return NS_OK;
}

/**
 * Merges the property value based on what property we're dealing with
 */
nsresult
sbBaseDevice::SyncMergeProperty(sbIMediaItem * aItem,
                                nsAString const & aPropertyId,
                                nsAString const & aNewValue,
                                nsAString const & aOldValue) {
  nsresult rv = NS_OK;

  nsString mergedValue = nsString(aNewValue);
  if (aPropertyId.Equals(NS_LITERAL_STRING(SB_PROPERTY_LASTPLAYTIME))) {
    rv = SyncMergeSetToLatest(aNewValue, aOldValue, mergedValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return aItem->SetProperty(aPropertyId, mergedValue);
}

/**
 * Returns the latest of the date/time. The dates are in milliseconds since
 * the JS Data's epoch date.
 */
nsresult
sbBaseDevice::SyncMergeSetToLatest(nsAString const & aNewValue,
                                   nsAString const & aOldValue,
                                   nsAString & aMergedValue) {
  nsresult rv;
  PRInt64 newDate;
  newDate = nsString_ToInt64(aNewValue, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 oldDate;
  oldDate = nsString_ToInt64(aOldValue, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  aMergedValue = newDate > oldDate ? aNewValue : aOldValue;
  return NS_OK;
}


nsresult
sbBaseDevice::ShouldSyncMediaList(sbIMediaList* aMediaList,
                                  PRBool*       aShouldSync)
{
  TRACE(("%s", __FUNCTION__));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aShouldSync);

  // Function variables.
  nsresult rv;

  // Default to should not sync.
  *aShouldSync = PR_FALSE;

  // Don't sync download media lists.
  nsAutoString customType;
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                               customType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (customType.EqualsLiteral("download"))
    return NS_OK;

  // Don't sync hidden lists.
  nsAutoString hidden;
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN), hidden);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hidden.EqualsLiteral("1"))
    return NS_OK;

  // Don't sync empty lists.
  PRBool isEmpty;
  rv = aMediaList->GetIsEmpty(&isEmpty);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isEmpty)
    return NS_OK;

  // Don't sync list that device doesn't support.
  PRUint16 listContentType;
  rv = sbLibraryUtils::GetMediaListContentType(aMediaList, &listContentType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!sbDeviceUtils::IsMediaListContentTypeSupported(this,
                                                      listContentType)) {
    return NS_OK;
  }

  // Don't sync media lists that are storage for other media lists (e.g., simple
  // media lists for smart media lists).
  nsAutoString outerGUID;
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_OUTERGUID),
                               outerGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!outerGUID.IsEmpty())
    return NS_OK;

  // Media list should be synced.
  *aShouldSync = PR_TRUE;

  return NS_OK;
}

nsresult
sbBaseDevice::SyncMainLibraryFlag(sbIMediaItem * aMediaItem)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_STATE(mMainLibrary);

  nsresult rv;

  // Get the guid of the original item, if any, that aMediaItem was
  // copied from.  The origin _library_ guid is ignored here to
  // avoid an extra check.  This is more efficient on configurations
  // where few items have been copied to the device from a library
  // other than the main library:
  nsAutoString originItemGuid;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originItemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the last saved value of SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY:
  nsAutoString wasInMainLibrary;
  rv = aMediaItem->GetProperty(
    NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY),
    wasInMainLibrary);
  NS_ENSURE_SUCCESS(rv, rv);
  // Normalize the value to facilitate the comparison below:
  if (wasInMainLibrary.IsEmpty())
  {
    wasInMainLibrary.AppendInt(0);
  }

  // Check whether the item is in the main library:
  nsAutoString isInMainLibrary;
  if (originItemGuid.IsEmpty()) {
    isInMainLibrary.AppendInt(0);
  }
  else {
    nsCOMPtr<sbIMediaItem> originItem;
    // GetMediaItem() returns an error if the item is not present, but
    // this behavior--and what error to expect--is undocumented.  Just
    // ignore the result code and test the returned item instead.
    mMainLibrary->GetMediaItem(originItemGuid, getter_AddRefs(originItem));
    isInMainLibrary.AppendInt(originItem != NULL);
  }

  // Update SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY if it changed:
  if (isInMainLibrary != wasInMainLibrary) {
    rv = aMediaItem->SetProperty(
      NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY),
      isInMainLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseDevice::PromptForEjectDuringPlayback(PRBool* aEject)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aEject);

  nsresult rv;

  sbPrefBranch prefBranch("songbird.device.dialog.", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hide_dialog = prefBranch.GetBoolPref("eject_while_playing", PR_FALSE);

  if (hide_dialog) {
    // if the dialog is disabled, continue as if the user had said yes
    *aEject = PR_TRUE;
    return NS_OK;
  }

  // get the prompter service and wait for a window to be available
  nsCOMPtr<sbIPrompter> prompter =
    do_GetService("@songbirdnest.com/Songbird/Prompter;1");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the stringbundle
  sbStringBundle bundle;

  // get the window title
  nsString const& title = bundle.Get("device.dialog.eject_while_playing.title");

  // get the device name
  nsString deviceName;
  rv = GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the message, based on the device name
  nsTArray<nsString> formatParams;
  formatParams.AppendElement(deviceName);
  nsString const& message =
    bundle.Format("device.dialog.eject_while_playing.message", formatParams);

  // get the text for the eject button
  nsString const& eject = bundle.Get("device.dialog.eject_while_playing.eject");

  // get the text for the checkbox
  nsString const& check =
    bundle.Get("device.dialog.eject_while_playing.dontask");

  // show the dialog box
  PRBool accept;
  rv = prompter->ConfirmEx(nsnull, title.get(), message.get(),
      (nsIPromptService::BUTTON_POS_0 *
       nsIPromptService::BUTTON_TITLE_IS_STRING) +
      (nsIPromptService::BUTTON_POS_1 *
       nsIPromptService::BUTTON_TITLE_CANCEL), eject.get(), nsnull, nsnull,
      check.get(), &hide_dialog, &accept);
  NS_ENSURE_SUCCESS(rv, rv);

  *aEject = !accept;

  // save the checkbox state
  rv = prefBranch.SetBoolPref("eject_while_playing", hide_dialog);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::UpdateMediaFolders()
{
  // Nothing for the base class to do.
  return NS_OK;
}

nsresult sbBaseDevice::GetDeviceWriteDestURI
                         (sbIMediaItem* aWriteDstItem,
                          nsIURI*       aContentSrcBaseURI,
                          nsIURI*       aWriteSrcURI,
                          nsIURI **     aContentSrc)
{
  TRACE(("%s", __FUNCTION__));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWriteDstItem);
  NS_ENSURE_ARG_POINTER(aContentSrcBaseURI);
  NS_ENSURE_ARG_POINTER(aContentSrc);

  // Function variables.
  nsString         kIllegalChars =
                     NS_ConvertASCIItoUTF16(FILE_ILLEGAL_CHARACTERS);
  nsCOMPtr<nsIURI> writeSrcURI = aWriteSrcURI;
  nsresult         rv;

  // If no write source URI is given, get it from the write source media item.
  if (!writeSrcURI) {
    // Get the origin item for the write destination item.
    nsCOMPtr<sbIMediaItem> writeSrcItem;
    rv = sbLibraryUtils::GetOriginItem(aWriteDstItem,
                                       getter_AddRefs(writeSrcItem));
    if (NS_FAILED(rv)) {
      // If there is not an existing origin for the write item, use the URI
      // of |aWriteDstItem|.
      rv = aWriteDstItem->GetContentSrc(getter_AddRefs(writeSrcURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      // Get the write source URI.
      rv = writeSrcItem->GetContentSrc(getter_AddRefs(writeSrcURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Convert our nsIURI to an nsIFileURL and nsIFile
  nsCOMPtr<nsIFile>    writeSrcFile;
  nsCOMPtr<nsIFileURL> writeSrcFileURL = do_QueryInterface(writeSrcURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    // Now get the nsIFile
    rv = writeSrcFileURL->GetFile(getter_AddRefs(writeSrcFile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Now check to make sure the source file actually exists
    PRBool fileExists = PR_FALSE;
    rv = writeSrcFile->Exists(&fileExists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!fileExists) {
      // Create the device error event and dispatch it
      nsCOMPtr<nsIVariant> var = sbNewVariant(aWriteDstItem).get();
      CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_FILE_MISSING,
                             var, PR_TRUE);

      // Remove item from library
      nsCOMPtr<sbILibrary> destLibrary;
      rv = aWriteDstItem->GetLibrary(getter_AddRefs(destLibrary));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = DeleteItem(destLibrary, aWriteDstItem);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  // Check if the item needs to be organized
  nsCOMPtr<sbILibrary> destLibrary;
  rv = aWriteDstItem->GetLibrary(getter_AddRefs(destLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString destLibGuidStr;
  rv = destLibrary->GetGuid(destLibGuidStr);
  NS_ENSURE_SUCCESS(rv, rv);
  nsID destLibGuid;
  PRBool success =
    destLibGuid.Parse(NS_LossyConvertUTF16toASCII(destLibGuidStr).get());
  OrganizeData* organizeData = nsnull;
  if (success) {
    success = mOrganizeLibraryPrefs.Get(destLibGuid, &organizeData);
  }

  nsCOMPtr<nsIFile> contentSrcFile;
  if (success && organizeData->organizeEnabled) {
    nsCOMPtr<nsIFileURL> baseFileUrl =
      do_QueryInterface(aContentSrcBaseURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> baseFile;
    rv = baseFileUrl->GetFile(getter_AddRefs(baseFile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the managed path
    nsCOMPtr<sbIMediaFileManager> fileMgr =
      do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_NAMED_LITERAL_STRING(KEY_MEDIA_FOLDER, "media-folder");
    NS_NAMED_LITERAL_STRING(KEY_FILE_FORMAT, "file-format");
    NS_NAMED_LITERAL_STRING(KEY_DIR_FORMAT, "dir-format");
    nsCOMPtr<nsIWritablePropertyBag2> writableBag =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1");
    NS_ENSURE_TRUE(writableBag, NS_ERROR_OUT_OF_MEMORY);
    rv = writableBag->SetPropertyAsInterface(KEY_MEDIA_FOLDER, baseFile);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = writableBag->SetPropertyAsACString(KEY_FILE_FORMAT, organizeData->fileFormat);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = writableBag->SetPropertyAsACString(KEY_DIR_FORMAT, organizeData->dirFormat);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = fileMgr->Init(writableBag);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fileMgr->GetManagedPath(aWriteDstItem,
                                 sbIMediaFileManager::MANAGE_COPY |
                                   sbIMediaFileManager::MANAGE_MOVE,
                                 getter_AddRefs(contentSrcFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> parentDir;
    rv = contentSrcFile->GetParent(getter_AddRefs(parentDir));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = parentDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (rv != NS_ERROR_FILE_ALREADY_EXISTS) {
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // Get the write source file name, unescape it, and replace illegal
    // characters.
    nsAutoString writeSrcFileName;
    if (writeSrcFile) {
      nsCOMPtr<nsIFile> canonicalFile;
      nsCOMPtr<sbILibraryUtils> libUtils =
        do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
      rv = libUtils->GetCanonicalPath(writeSrcFile,
                                      getter_AddRefs(canonicalFile));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = canonicalFile->GetLeafName(writeSrcFileName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsCOMPtr<nsIURL>
        writeSrcURL = do_MainThreadQueryInterface(writeSrcURI, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCAutoString fileName;
      rv = writeSrcURL->GetFileName(fileName);
      NS_ENSURE_SUCCESS(rv, rv);
      writeSrcFileName = NS_ConvertUTF8toUTF16(fileName);
    }

    // replace illegal characters
    nsString_ReplaceChar(writeSrcFileName, kIllegalChars, PRUnichar('_'));

    // Get a file object for the content base.
    nsCOMPtr<nsIFileURL>
      contentSrcBaseFileURL = do_QueryInterface(aContentSrcBaseURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> contentSrcBaseFile;
    rv = contentSrcBaseFileURL->GetFile(getter_AddRefs(contentSrcBaseFile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Start the content source at the base.
    rv = contentSrcBaseFile->Clone(getter_AddRefs(contentSrcFile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Append file name of the write source file.
    rv = contentSrcFile->Append(writeSrcFileName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Check if the content source file already exists.
  PRBool exists;
  rv = contentSrcFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a unique file if content source file already exists.
  if (exists) {
    // Get the permissions of the content source parent.
    PRUint32          permissions;
    nsCOMPtr<nsIFile> parent;
    rv = contentSrcFile->GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = parent->GetPermissions(&permissions);
    NS_ENSURE_SUCCESS(rv, rv);

    // Create a unique file.
    rv = contentSrcFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, permissions);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = sbNewFileURI(contentSrcFile, aContentSrc);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbBaseDevice::SetupDevice()
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;

  // Cancel any pending deferred setup device timer.
  if (mDeferredSetupDeviceTimer) {
    rv = mDeferredSetupDeviceTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
    mDeferredSetupDeviceTimer = nsnull;
  }

  // Defer device setup for a small time delay to allow device connection and
  // mounting to complete.  Since device setup presents a modal dialog,
  // presenting the dialog immediately may stop mounting of other device
  // volumes, preventing them from showing up in the device setup dialog.  In
  // addition, the dialog can prevent device connection from completing,
  // causing problems if the device is disconnected while the device setup
  // dialog is presented.
  mDeferredSetupDeviceTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDeferredSetupDeviceTimer->InitWithFuncCallback(DeferredSetupDevice,
                                                       this,
                                                       DEFER_DEVICE_SETUP_DELAY,
                                                       nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ void
sbBaseDevice::DeferredSetupDevice(nsITimer* aTimer,
                                  void*     aClosure)
{
  TRACE(("%s", __FUNCTION__));
  sbBaseDevice* baseDevice = reinterpret_cast<sbBaseDevice*>(aClosure);
  baseDevice->DeferredSetupDevice();
}

nsresult
sbBaseDevice::DeferredSetupDevice()
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;

  // Clear the deferred setup device timer.
  mDeferredSetupDeviceTimer = nsnull;

  // Present the setup device dialog.
  nsCOMPtr<sbIPrompter>
    prompter = do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMWindow> dialogWindow;
  rv = prompter->OpenDialog
         (nsnull,
          NS_LITERAL_STRING
            ("chrome://songbird/content/xul/device/deviceSetupDialog.xul"),
          NS_LITERAL_STRING("DeviceSetup"),
          NS_LITERAL_STRING("chrome,centerscreen,modal=yes,titlebar=no"),
          NS_ISUPPORTS_CAST(sbIDevice*, this),
          getter_AddRefs(dialogWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ProcessInfoRegistrars()
{
  TRACE(("%s", __FUNCTION__));
  // If we haven't built the registrars then do so
  if (mInfoRegistrarType != sbIDeviceInfoRegistrar::NONE) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = catMgr->EnumerateCategory(SB_DEVICE_INFO_REGISTRAR_CATEGORY,
                                 getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  // Enumerate the registrars and find the highest scoring one (Greatest type)
  PRBool hasMore;
  rv = enumerator->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);

  while(hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = enumerator->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsCString> data = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString entryName;
    rv = data->GetData(entryName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString contractId;
    rv = catMgr->GetCategoryEntry(SB_DEVICE_INFO_REGISTRAR_CATEGORY,
                                  entryName.get(),
                                  getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDeviceInfoRegistrar> infoRegistrar =
      do_CreateInstance(contractId.get(), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool interested;
    rv = infoRegistrar->InterestedInDevice(this, &interested);
    NS_ENSURE_SUCCESS(rv, rv);
    if (interested) {
      PRUint32 type;
      rv = infoRegistrar->GetType(&type);
      NS_ENSURE_SUCCESS(rv, rv);
      if (type >= mInfoRegistrarType) {
        mInfoRegistrar = infoRegistrar;
        mInfoRegistrarType = type;
      }
    }

    rv = enumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
sbBaseDevice::RegisterDeviceInfo()
{
  TRACE(("%s", __FUNCTION__));

  PRBool   success;
  nsresult rv;

  // Process the device info registrars.
  rv = ProcessInfoRegistrars();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device properties.
  nsCOMPtr<nsIWritablePropertyBag> deviceProperties;
  rv = GetWritableDeviceProperties(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Register device default name.
  nsString defaultName;
  rv = mInfoRegistrar->GetDefaultName(this, defaultName);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Default Name: %s", NS_ConvertUTF16toUTF8(defaultName).get()));

  if (!defaultName.IsEmpty()) {
    rv = deviceProperties->SetProperty(
                           NS_LITERAL_STRING(SB_DEVICE_PROPERTY_DEFAULT_NAME),
                           sbNewVariant(defaultName));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Register device folders.
  for (PRUint32 i = 0;
       i < NS_ARRAY_LENGTH(sbBaseDeviceSupportedFolderContentTypeList);
       ++i) {
    // Get the next folder content type.
    PRUint32 folderContentType = sbBaseDeviceSupportedFolderContentTypeList[i];

    // Allocate a folder URL.
    nsAutoPtr<nsString> folderURL(new nsString());
    NS_ENSURE_TRUE(folderURL, NS_ERROR_OUT_OF_MEMORY);

    // Get the device folder URL and add it to the media folder URL table.
    rv = mInfoRegistrar->GetDeviceFolder(this, folderContentType, *folderURL);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!folderURL->IsEmpty()) {
      success = mMediaFolderURLTable.Put(folderContentType, folderURL);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      folderURL.forget();
    }
  }

  nsString excludedFolders;
  rv = mInfoRegistrar->GetExcludedFolders(this, excludedFolders);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Excluded Folders: %s",
       NS_LossyConvertUTF16toASCII(excludedFolders).BeginReading()));

  if (!excludedFolders.IsEmpty()) {
    rv = deviceProperties->SetProperty(
                           NS_LITERAL_STRING(SB_DEVICE_PROPERTY_EXCLUDED_FOLDERS),
                           sbNewVariant(excludedFolders));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get import rules:
  nsCOMPtr<nsIArray> importRules;
  rv = mInfoRegistrar->GetImportRules(this, getter_AddRefs(importRules));
  NS_ENSURE_SUCCESS(rv, rv);

  // Stow the rules, if any, in the device properties:
  if (importRules) {
    nsCOMPtr<nsIWritablePropertyBag2> devProps2 =
      do_QueryInterface(deviceProperties, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = devProps2->SetPropertyAsInterface(
                    NS_LITERAL_STRING(SB_DEVICE_PROPERTY_IMPORT_RULES),
                    importRules);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Log the device folders.
  LogDeviceFolders();

  // Determine if the device supports format.
  PRBool supportsFormat;
  rv = mInfoRegistrar->GetDoesDeviceSupportReformat(this, &supportsFormat);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceProperties->SetProperty(
      NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SUPPORTS_REFORMAT),
      sbNewVariant(supportsFormat));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * The capabilities cannot be cached because the capabilities are not preserved
 * on all devices.
 */
nsresult
sbBaseDevice::RegisterDeviceCapabilities(sbIDeviceCapabilities * aCapabilities)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv;

  rv = ProcessInfoRegistrars();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mInfoRegistrar) {
    rv = mInfoRegistrar->AddCapabilities(this, aCapabilities);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDevice::SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                                sbIDeviceSupportsItemCallback* aCallback)
{
  // This is always async, so it's fine to always create a new runnable and
  // do anything there
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aCallback);

  nsresult rv;
  nsRefPtr<sbDeviceSupportsItemHelper> helper =
    new sbDeviceSupportsItemHelper();
  NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);
  rv = helper->Init(aMediaItem, this, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbDeviceSupportsItemHelper,
                             helper.get(),
                             RunSupportsMediaItem);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);
    rv = NS_DispatchToMainThread(runnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    helper->RunSupportsMediaItem();
  }
  return NS_OK;
}

nsresult
sbBaseDevice::SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                                sbDeviceSupportsItemHelper*    aCallback,
                                PRBool                         aReportErrors,
                                PRBool*                        _retval)
{
  // we will always need the aMediaItem and retval
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  if (NS_IsMainThread()) {
    // it is an error to call this on the main thread with no callback
    NS_ENSURE_ARG_POINTER(aCallback);
  }

  nsresult rv;

  // Handle image media items using file extensions.
  nsString contentType;
  rv = aMediaItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (contentType.Equals(NS_LITERAL_STRING("image"))) {
    // Get the media item file extension.
    nsCString        sourceFileExtension;
    nsCOMPtr<nsIURI> sourceURI;
    rv = aMediaItem->GetContentSrc(getter_AddRefs(sourceURI));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFileURL> sourceFileURL = do_QueryInterface(sourceURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = sourceFileURL->GetFileExtension(sourceFileExtension);
    NS_ENSURE_SUCCESS(rv, rv);
    ToLowerCase(sourceFileExtension);

    // Get the list of file extensions supported by the device.
    nsTArray<nsString> fileExtensionList;
    rv = sbDeviceUtils::AddSupportedFileExtensions
                          (this,
                           sbIDeviceCapabilities::CONTENT_IMAGE,
                           fileExtensionList);
    NS_ENSURE_SUCCESS(rv, rv);

    // Item is supported if its file extension is supported.
    *_retval = fileExtensionList.Contains
                                   (NS_ConvertUTF8toUTF16(sourceFileExtension));

    return NS_OK;
  }

  if (sbDeviceUtils::IsItemDRMProtected(aMediaItem)) {
    rv = SupportsMediaItemDRM(aMediaItem, aReportErrors, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  PRUint32 const transcodeType =
    GetDeviceTranscoding()->GetTranscodeType(aMediaItem);
  bool needsTranscoding = false;

  if (transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO &&
      mCanTranscodeAudio != CAN_TRANSCODE_UNKNOWN) {
    *_retval = (mCanTranscodeAudio == CAN_TRANSCODE_YES) ? PR_TRUE : PR_FALSE;
    return NS_OK;
  }
  else if(transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO &&
          mCanTranscodeVideo != CAN_TRANSCODE_UNKNOWN) {
    *_retval = (mCanTranscodeVideo == CAN_TRANSCODE_YES) ? PR_TRUE : PR_FALSE;
    return NS_OK;
  }

  // Try to check if we can transcode first, since it's cheaper than trying
  // to inspect the actual file (and both ways we get which files we can get
  // on the device).
  // XXX MOOK this needs to be fixed to be not gstreamer specific
  nsCOMPtr<nsIURI> inputUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(inputUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceTranscodingConfigurator> configurator;
  rv = sbDeviceUtils::GetTranscodingConfigurator(transcodeType,
                                               getter_AddRefs(configurator));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = configurator->SetInputUri(inputUri);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDevice> device =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIDevice*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = configurator->SetDevice(device);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = configurator->DetermineOutputType();
  if (NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;

    if (transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO) {
      mCanTranscodeAudio = CAN_TRANSCODE_YES;
    }
    else if(transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO) {
      mCanTranscodeVideo = CAN_TRANSCODE_YES;
    }

    return NS_OK;
  }

  // Can't transcode, check the media format as a fallback.
  if (aCallback) {
    // asynchronous
    nsCOMPtr<sbIMediaInspector> inspector;
    rv = GetDeviceTranscoding()->GetMediaInspector(getter_AddRefs(inspector));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aCallback->InitJobProgress(inspector, transcodeType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = inspector->InspectMediaAsync(aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_ERROR_IN_PROGRESS;
  }
  // synchronous
  nsCOMPtr<sbIMediaFormat> mediaFormat;
  rv = GetDeviceTranscoding()->GetMediaFormat(transcodeType,
                                              aMediaItem,
                                              getter_AddRefs(mediaFormat));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sbDeviceUtils::DoesItemNeedTranscoding(transcodeType,
                                              mediaFormat,
                                              this,
                                              needsTranscoding);

  *_retval = (NS_SUCCEEDED(rv) && !needsTranscoding);

  if (transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO) {
    mCanTranscodeAudio = (*_retval == PR_TRUE) ? CAN_TRANSCODE_YES : CAN_TRANSCODE_NO;
  }
  else if(transcodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO) {
    mCanTranscodeVideo = (*_retval == PR_TRUE) ? CAN_TRANSCODE_YES : CAN_TRANSCODE_NO;
  }

  return NS_OK;
}

nsresult
sbBaseDevice::UpdateStreamingItemSupported(Batch & aBatch)
{
  nsresult rv;

  // Iterate over the batch updating item transferable
  const Batch::const_iterator end = aBatch.end();
  for (Batch::const_iterator iter = aBatch.begin();
       iter != end;
       ++iter) {
    TransferRequest * const request = static_cast<TransferRequest*>(*iter);

    // Skip everything but read and write requests
    switch (request->GetType()) {
      case sbBaseDevice::TransferRequest::REQUEST_WRITE:
      case sbBaseDevice::TransferRequest::REQUEST_READ:
        break;
      default:
        continue;
    }

    nsCOMPtr<sbIMediaItem> mediaItem = request->item;

    nsString trackType;
    rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKTYPE),
                                trackType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (trackType.IsEmpty())
      continue;

    PRBool isSupported = PR_FALSE;
    if (!mTrackSourceTable.Get(trackType, &isSupported)) {
      PRMonitor * stopWaitMonitor = mRequestThreadQueue->GetStopWaitMonitor();

      // check transferable only once.
      nsRefPtr<sbDeviceStreamingHandler> listener;
      rv = sbDeviceStreamingHandler::New(getter_AddRefs(listener),
                                         mediaItem,
                                         stopWaitMonitor);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = listener->CheckTransferable();
      NS_ENSURE_SUCCESS(rv, rv);

      // Wait for the transferable check to complete.
      PRBool isComplete = PR_FALSE;
      while (!isComplete) {
        // Operate within the request wait monitor.
        nsAutoMonitor monitor(stopWaitMonitor);

        // Check for abort.
        NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

        // Check if the transferable check is complete.
        isComplete = listener->IsComplete();

        // If not complete, wait for completion. If requests are cancelled,
        // the request wait monitor will be notified.
        if (!isComplete)
          monitor.Wait();
      }

      isSupported = listener->IsStreamingItemSupported();
      NS_ENSURE_TRUE(mTrackSourceTable.Put(trackType, isSupported),
                     NS_ERROR_OUT_OF_MEMORY);
    }

    if (!isSupported) {
      request->destinationCompatibility =
        sbBaseDevice::TransferRequest::COMPAT_UNSUPPORTED;
    }
  }

  // Clear the table so next batch will be re-checked.
  mTrackSourceTable.Clear();

  return NS_OK;
}

/* attribute sbIDeviceLibrary defaultLibrary; */
NS_IMETHODIMP
sbBaseDevice::GetDefaultLibrary(sbIDeviceLibrary** aDefaultLibrary)
{
  NS_ENSURE_ARG_POINTER(aDefaultLibrary);
  NS_IF_ADDREF(*aDefaultLibrary = mDefaultLibrary);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDevice::SetDefaultLibrary(sbIDeviceLibrary* aDefaultLibrary)
{
  NS_ENSURE_ARG_POINTER(aDefaultLibrary);
  nsresult rv;

  // Do nothing if default library is not changing.
  if (mDefaultLibrary == aDefaultLibrary)
    return NS_OK;

  // Ensure library is in the device content.
  nsCOMPtr<nsIArray>         libraries;
  nsCOMPtr<sbIDeviceContent> content;
  PRUint32                   index;
  rv = GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = content->GetLibraries(getter_AddRefs(libraries));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = libraries->IndexOf(0, aDefaultLibrary, &index);
  if (rv == NS_ERROR_FAILURE) {
    // Library is not in device content.
    rv = NS_ERROR_ILLEGAL_VALUE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the default library preference.
  nsAutoString guid;
  rv = aDefaultLibrary->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetPreference(NS_LITERAL_STRING("default_library_guid"),
                     sbNewVariant(guid));
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the default library.
  rv = UpdateDefaultLibrary(aDefaultLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update properties for new default library.
  UpdateProperties();

  return NS_OK;
}

/* readonly attribute sbIDeviceLibrary primaryLibrary; */
NS_IMETHODIMP
sbBaseDevice::GetPrimaryLibrary(sbIDeviceLibrary** aPrimaryLibrary)
{
  NS_ENSURE_ARG_POINTER(aPrimaryLibrary);
  if (!mPrimaryVolume) {
    *aPrimaryLibrary = nsnull;
    return NS_OK;
  }
  return mPrimaryVolume->GetDeviceLibrary(aPrimaryLibrary);
}

nsresult
sbBaseDevice::GetSupportedTranscodeProfiles(PRUint32 aType,
                                            nsIArray **aSupportedProfiles)
{
  return GetDeviceTranscoding()->GetSupportedTranscodeProfiles(
          aType,
          aSupportedProfiles);
}

nsresult
sbBaseDevice::GetDeviceTranscodingProperty(PRUint32         aTranscodeType,
                                           const nsAString& aPropertyName,
                                           nsIVariant**     aPropertyValue)
{
  TRACE(("%s", __FUNCTION__));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPropertyValue);

  // Function variables.
  nsresult rv;

  // Get the device transcoding profile.
  nsCOMPtr<sbITranscodeProfile> transcodeProfile;
  rv = GetDeviceTranscoding()->SelectTranscodeProfile
                                 (aTranscodeType,
                                  getter_AddRefs(transcodeProfile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a property enumerator.
  nsCOMPtr<nsISimpleEnumerator> propEnumerator;
  nsCOMPtr<nsIArray>            properties;
  rv = transcodeProfile->GetAudioProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = properties->Enumerate(getter_AddRefs(propEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search for property.
  PRBool more = PR_FALSE;
  rv = propEnumerator->HasMoreElements(&more);
  NS_ENSURE_SUCCESS(rv, rv);
  while (more) {
    // Get the next property.
    nsCOMPtr<sbITranscodeProfileProperty> property;
    rv = propEnumerator->GetNext(getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property name.
    nsAutoString profilePropName;
    rv = property->GetPropertyName(profilePropName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Return property value if its name matches.
    if (profilePropName.Equals(aPropertyName)) {
      rv = property->GetValue(aPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }

    // Check for more properties.
    rv = propEnumerator->HasMoreElements(&more);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Property not found.
  *aPropertyValue = nsnull;

  return NS_OK;
}

nsresult
sbBaseDevice::DispatchTranscodeErrorEvent(sbIMediaItem*    aMediaItem,
                                          const nsAString& aErrorMessage)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bag->SetPropertyAsAString(NS_LITERAL_STRING("message"), aErrorMessage);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("item"), aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString srcUri;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                               srcUri);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<sbITranscodeError> transcodeError;
    rv = SB_NewTranscodeError(aErrorMessage, aErrorMessage, SBVoidString(),
                              srcUri,
                              aMediaItem,
                              getter_AddRefs(transcodeError));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("transcode-error"),
                                     NS_ISUPPORTS_CAST(sbITranscodeError*, transcodeError));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSCODE_ERROR,
                              sbNewVariant(NS_GET_IID(nsIPropertyBag2), bag));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static nsresult
AddAlbumArtFormats(PRUint32 aContentType,
                   sbIDeviceCapabilities *aCapabilities,
                   nsIMutableArray *aArray,
                   PRUint32 numMimeTypes,
                   char **mimeTypes)
{
  nsresult rv;

  for (PRUint32 i = 0; i < numMimeTypes; i++) {
    nsISupports** formatTypes;
    PRUint32 formatTypeCount;
    rv = aCapabilities->GetFormatTypes(aContentType,
                                       NS_ConvertASCIItoUTF16(mimeTypes[i]),
                                       &formatTypeCount,
                                       &formatTypes);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                         formatTypes);
    for (PRUint32 formatIndex = 0;
         formatIndex < formatTypeCount;
         formatIndex++)
    {
      nsCOMPtr<sbIImageFormatType> constraints =
          do_QueryInterface(formatTypes[formatIndex], &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aArray->AppendElement(constraints, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbBaseDevice::GetSupportedAlbumArtFormats(nsIArray * *aFormats)
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;
  nsCOMPtr<nsIMutableArray> formatConstraints =
      do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceCapabilities> capabilities;
  rv = GetCapabilities(getter_AddRefs(capabilities));
  NS_ENSURE_SUCCESS(rv, rv);

  char **mimeTypes;
  PRUint32 numMimeTypes;
  rv = capabilities->GetSupportedMimeTypes(sbIDeviceCapabilities::CONTENT_IMAGE,
          &numMimeTypes, &mimeTypes);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddAlbumArtFormats(sbIDeviceCapabilities::CONTENT_IMAGE,
                          capabilities,
                          formatConstraints,
                          numMimeTypes,
                          mimeTypes);
  /* Ensure everything is freed here before potentially returning; no
     magic destructors for this thing */
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(numMimeTypes, mimeTypes);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF (*aFormats = formatConstraints);
  return NS_OK;
}

nsresult
sbBaseDevice::GetShouldLimitMusicSpace(const nsAString & aPrefBase,
                                       PRBool *aOutShouldLimitSpace)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aOutShouldLimitSpace);
  *aOutShouldLimitSpace = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIVariant> shouldEnableVar;
  rv = GetLibraryPreference(aPrefBase,
                            NS_LITERAL_STRING("use_music_limit_percent"),
                            getter_AddRefs(shouldEnableVar));
  NS_ENSURE_SUCCESS(rv, rv);

  return shouldEnableVar->GetAsBool(aOutShouldLimitSpace);
}

nsresult
sbBaseDevice::GetMusicLimitSpacePercent(const nsAString & aPrefBase,
                                        PRUint32 *aOutLimitPercentage)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aOutLimitPercentage);
  *aOutLimitPercentage = 100;  // always default to 100

  nsresult rv;
  nsCOMPtr<nsIVariant> prefValue;
  rv = GetLibraryPreference(aPrefBase,
                            NS_LITERAL_STRING("music_limit_percent"),
                            getter_AddRefs(prefValue));
  NS_ENSURE_SUCCESS(rv, rv);

  return prefValue->GetAsUint32(aOutLimitPercentage);
}

/* void Eject(); */
NS_IMETHODIMP sbBaseDevice::Eject()
{
  TRACE(("%s", __FUNCTION__));

  nsresult rv;

  // Device has no default library. Just leave.
  if (!mDefaultLibrary)
    return NS_OK;

  // If the playback is on for the device item, stop the playback before
  // ejecting.
  nsCOMPtr<sbIMediacoreManager> mediacoreManager =
    do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = mediacoreManager->GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the currently playing media item.
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = sequencer->GetCurrentItem(getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // No current media item. Just leave.
  if (!mediaItem)
    return NS_OK;

  // Get the library that the media item lives in.
  nsCOMPtr<sbILibrary> library;
  rv = mediaItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  // Is that library our device library?
  PRBool equal;
  rv = mDefaultLibrary->Equals(library, &equal);
  NS_ENSURE_SUCCESS(rv, rv);

  if (equal) {
    nsCOMPtr<sbIMediacoreStatus> status;
    rv = mediacoreManager->GetStatus(getter_AddRefs(status));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 state = 0;
    rv = status->GetState(&state);
    NS_ENSURE_SUCCESS(rv, rv);

    // Not playing, nothing to do.
    if (state == sbIMediacoreStatus::STATUS_STOPPED ||
        state == sbIMediacoreStatus::STATUS_UNKNOWN) {
      return NS_OK;
    }

    // Confirm with user on whether to stop the playback and eject.
    PRBool eject;
    PromptForEjectDuringPlayback(&eject);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!eject)
      return NS_ERROR_ABORT;

    nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
    rv = mediacoreManager->GetPlaybackControl(getter_AddRefs(playbackControl));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = playbackControl->Stop();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* void Format(); */
NS_IMETHODIMP sbBaseDevice::Format()
{
  TRACE(("%s", __FUNCTION__));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean supportsReformat; */
NS_IMETHODIMP sbBaseDevice::GetSupportsReformat(PRBool *_retval)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIPropertyBag2> deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceProperties->GetPropertyAsBool(
      NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SUPPORTS_REFORMAT),
      _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* attribute boolean cacheSyncRequests; */
NS_IMETHODIMP
sbBaseDevice::GetCacheSyncRequests(PRBool *_retval)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = (mSyncState != sbBaseDevice::SYNC_STATE_NORMAL);
  return NS_OK;
}

/* attribute boolean cacheSyncRequests; */
NS_IMETHODIMP
sbBaseDevice::SetCacheSyncRequests(PRBool aCacheSyncRequests)
{
  TRACE(("%s", __FUNCTION__));

  if (aCacheSyncRequests &&
      mSyncState == sbBaseDevice::SYNC_STATE_NORMAL) {
    mSyncState = sbBaseDevice::SYNC_STATE_CACHE;
  }
  else if (!aCacheSyncRequests &&
           mSyncState == sbBaseDevice::SYNC_STATE_PENDING) {
    mSyncState = sbBaseDevice::SYNC_STATE_NORMAL;
    // Since we are turning off the CacheSyncRequests and a sync request came in
    // we need to start a sync.
    SyncLibraries();
  }
  else if (!aCacheSyncRequests) {
    mSyncState = sbBaseDevice::SYNC_STATE_NORMAL;
  }

  return NS_OK;
}

nsresult
sbBaseDevice::GetNameBase(nsAString& aName)
{
  TRACE(("%s", __FUNCTION__));
  PRBool   hasKey;
  nsresult rv;

  nsCOMPtr<nsIPropertyBag2> properties;
  rv = GetPropertyBag(this, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Try using the friendly name property and exit if successful.
  rv = properties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME), &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    rv = properties->GetPropertyAsAString
                        (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME), aName);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  // Try using the default name property and exit if successful.
  rv = properties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_DEFAULT_NAME),
                          &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    rv = properties->GetPropertyAsAString
                        (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_DEFAULT_NAME),
                         aName);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  // Use the product name.
  return GetProductName(aName);
}

nsresult
sbBaseDevice::GetProductNameBase(char const * aDefaultModelNumberString,
                                 nsAString& aProductName)
{
  TRACE(("%s [%s]", __FUNCTION__, aDefaultModelNumberString));
  NS_ENSURE_ARG_POINTER(aDefaultModelNumberString);

  nsAutoString productName;
  PRBool       hasKey;
  nsresult     rv;

  nsCOMPtr<nsIPropertyBag2> properties;
  rv = GetPropertyBag(this, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the vendor name.
  nsAutoString vendorName;
  rv = properties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                          &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    // Get the vendor name.
    rv = properties->GetPropertyAsAString
                        (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                         vendorName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get the device model number, using a default if one is not available.
  nsAutoString modelNumber;
  rv = properties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                          &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    // Get the model number.
    rv = properties->GetPropertyAsAString(
                              NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                              modelNumber);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (modelNumber.IsEmpty()) {
    // Get the default model number.
    modelNumber = SBLocalizedString(aDefaultModelNumberString);
  }

  // Produce the product name.
  if (!vendorName.IsEmpty() &&
      !StringBeginsWith(modelNumber, vendorName)) {
    nsTArray<nsString> params;
    NS_ENSURE_TRUE(params.AppendElement(vendorName), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(params.AppendElement(modelNumber), NS_ERROR_OUT_OF_MEMORY);
    productName.Assign(SBLocalizedString("device.product.name", params));
  } else {
    productName.Assign(modelNumber);
  }

  // Return results.
  aProductName.Assign(productName);

  return NS_OK;
}

nsresult
sbBaseDevice::IgnoreWatchFolderPath(nsIURI * aURI,
                                    sbAutoIgnoreWatchFolderPath ** aIgnorePath)
{
  nsresult rv;
  // Setup the ignore rule w/ the watch folder service.
  nsRefPtr<sbAutoIgnoreWatchFolderPath> autoWFPathIgnore =
    new sbAutoIgnoreWatchFolderPath();
  NS_ENSURE_TRUE(autoWFPathIgnore, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIFileURL> destURL =
    do_QueryInterface(aURI, &rv);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsCOMPtr<nsIFile> destFile;
  rv = destURL->GetFile(getter_AddRefs(destFile));
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsString destPath;
  rv = destFile->GetPath(destPath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = autoWFPathIgnore->Init(destPath);
  NS_ENSURE_SUCCESS(rv, rv);

  autoWFPathIgnore.forget(aIgnorePath);

  return NS_OK;
}

nsresult
sbBaseDevice::GetExcludedFolders(nsTArray<nsString> & aExcludedFolders)
{
  nsresult rv;

  nsCOMPtr<nsIPropertyBag2> deviceProperties;
  rv = GetPropertyBag(this, getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString excludedFolderStrings;
  rv = deviceProperties->GetPropertyAsAString(
                         NS_LITERAL_STRING(SB_DEVICE_PROPERTY_EXCLUDED_FOLDERS),
                         excludedFolderStrings);
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
    nsString_Split(excludedFolderStrings,
                   NS_LITERAL_STRING(","),
                   aExcludedFolders);
  }

  return NS_OK;
}

void
sbBaseDevice::LogDeviceFolders()
{
#ifdef PR_LOGGING
  LOG(("DEVICE FOLDERS:\n========================================\n"));
  mMediaFolderURLTable.EnumerateRead(LogDeviceFoldersEnum, nsnull);
#endif
}

/* static */ PLDHashOperator
sbBaseDevice::LogDeviceFoldersEnum(const unsigned int& aKey,
                                   nsString*           aData,
                                   void*               aUserArg)
{
#ifdef PR_LOGGING
  static const char* contentTypeNameList[] = {
    "CONTENT_UNKNOWN",
    "CONTENT_FILE",
    "CONTENT_FOLDER",
    "CONTENT_AUDIO",
    "CONTENT_IMAGE",
    "CONTENT_VIDEO",
    "CONTENT_PLAYLIST",
    "CONTENT_ALBUM"
  };

  // Get the content type name.
  const char* contentTypeName;
  char        contentTypeString[32];
  if (aKey < NS_ARRAY_LENGTH(contentTypeNameList)) {
    contentTypeName = contentTypeNameList[aKey];
  }
  else {
    sprintf(contentTypeString, "0x%08x", aKey);
    contentTypeName = contentTypeString;
  }

  // Log folder.
  if (aData)
    LOG(("  %s: %s\n", contentTypeName, NS_ConvertUTF16toUTF8(*aData).get()));
#endif

  return PL_DHASH_NEXT;
}

PLDHashOperator
sbBaseDevice::RemoveLibraryEnumerator(nsISupports * aList,
                                      nsCOMPtr<nsIMutableArray> & aItems,
                                      void * aUserArg)
{
  NS_ENSURE_TRUE(aList, PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aItems, PL_DHASH_NEXT);

  sbBaseDevice * const device =
    static_cast<sbBaseDevice*>(aUserArg);

  AutoListenerIgnore ignore(device);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = aItems->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aList);
  if (list) {
    rv = list->RemoveSome(enumerator);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);
  }

  return PL_DHASH_NEXT;
}

sbBaseDevice::AutoListenerIgnore::AutoListenerIgnore(sbBaseDevice * aDevice) :
  mDevice(aDevice)
{
  mDevice->SetIgnoreMediaListListeners(PR_TRUE);
  mDevice->mLibraryListener->SetIgnoreListener(PR_TRUE);
}

sbBaseDevice::AutoListenerIgnore::~AutoListenerIgnore()
{
  mDevice->SetIgnoreMediaListListeners(PR_FALSE);
  mDevice->mLibraryListener->SetIgnoreListener(PR_FALSE);
}

/**
 * Split out a batch into three separate batches so that items are processed
 * in proper order and the status reporting system reports status properly.
 */
void SBWriteRequestSplitBatches(const sbBaseDevice::Batch & aInput,
                                sbBaseDevice::Batch & aNonTranscodeItems,
                                sbBaseDevice::Batch & aTranscodeItems,
                                sbBaseDevice::Batch & aPlaylistItems)
{
  const sbBaseDevice::TransferRequest::CompatibilityType NEEDS_TRANSCODING =
    sbBaseDevice::TransferRequest::COMPAT_NEEDS_TRANSCODING;

  const sbBaseDevice::Batch::const_iterator end = aInput.end();
  for (sbBaseDevice::Batch::const_iterator iter = aInput.begin();
      iter != end;
      ++iter)
  {
    // We only want to split out certain requests, things like mount and format
    // will stay in the non transcode items. This really isn't ideal, but
    // but with the way our current status reporting system works we have to
    // break things out. Typically requests like format and mount will never be
    // in a batch with another request.
    switch ((*iter)->GetType()) {
      case sbBaseDevice::TransferRequest::REQUEST_WRITE:
      case sbBaseDevice::TransferRequest::REQUEST_READ:
      case sbBaseDevice::TransferRequest::REQUEST_MOVE:
      case sbBaseDevice::TransferRequest::REQUEST_UPDATE:
      case sbBaseDevice::TransferRequest::REQUEST_NEW_PLAYLIST:
      case sbBaseDevice::TransferRequest::REQUEST_DELETE: {
        sbBaseDevice::TransferRequest * request =
          static_cast<sbBaseDevice::TransferRequest*>(*iter);
        if (request->IsPlaylist()) {
          aPlaylistItems.push_back(*iter);
        }
        else if (request->destinationCompatibility == NEEDS_TRANSCODING) {
          aTranscodeItems.push_back(request);
        }
        else {
          aNonTranscodeItems.push_back(request);
        }
      }
      break;
      default: {
        aNonTranscodeItems.push_back(*iter);
      }
      break;
    }
  }
}
