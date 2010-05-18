/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

// Self imports.
#include "sbCDDevice.h"
#include "sbCDDeviceDefines.h"

// Local imports
#include "sbCDLog.h"

// Songbird imports.
#include <sbArray.h>
#include <sbDeviceTranscoding.h>
#include <sbDeviceUtils.h>
#include <sbFileUtils.h>
#include <sbIDeviceEvent.h>
#include <sbIAlbumArtScanner.h>
#include <sbIJobCancelable.h>
#include <sbIJobProgress.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediaManagementService.h>
#include <sbITranscodeAlbumArt.h>
#include <sbLibraryUtils.h>
#include <sbPrefBranch.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbPropertiesCID.h>
#include <sbStringUtils.h>
#include <sbMemoryUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbStatusPropertyValue.h>
#include <sbTranscodeProgressListener.h>
#include <sbVariantUtils.h>
#include <sbStandardProperties.h>
#include <sbWatchFolderUtils.h>

// Mozilla imports.
#include <nsCRT.h>
#include <nsArrayUtils.h>
#include <nsIBufferedStreams.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsILocalFile.h>
#include <nsIProxyObjectManager.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsIDOMWindow.h>
#include <nsIWindowWatcher.h>
#include <nsNetUtil.h>
#include <nsThreadUtils.h>
#include <nsComponentManagerUtils.h>

#define SB_CD_DEVICE_AUTO_INVOKE(aName, aMethod)                              \
  SB_AUTO_NULL_CLASS(aName, sbCDDevice*, mValue->aMethod)

//
// Auto-disposal class wrappers.
//
//   sbCDAutoFalse             Wrapper to automatically set a boolean to false.
//   sbCDAutoUnignoreItem      Wrapper to auto ignore changes to a media
//                             item.
//

SB_AUTO_NULL_CLASS(sbCDAutoFalse, PRBool*, *mValue = PR_FALSE);
SB_AUTO_CLASS2(sbCDAutoIgnoreItem,
               sbCDDevice*,
               sbIMediaItem*,
               mValue != nsnull,
               mValue->UnignoreMediaItem(mValue2),
               mValue = nsnull);

//
// Auto-sbCDAutoDeviceLocker unlocking class.
//
//   sbCDAutoDeviceUnlock   Wrapper class to automatically lock and unlock
//                          a device.
//
class sbCDAutoDeviceLocker
{
public:
  sbCDAutoDeviceLocker(sbICDDevice *aCDDevice)
    : mCDDevice(aCDDevice)
  {
    // Only lock the device if it isn't already locked.
    if (mCDDevice) {
      nsresult rv;
      PRBool isLocked = PR_FALSE;
      rv = mCDDevice->GetIsDeviceLocked(&isLocked);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "Could not get the device lock state!");

      if (!isLocked) {
        rv = mCDDevice->LockDevice();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not lock the cd device!");
      }
      else {
        // Set the device to |nsnull| here so we don't unlock a device that
        // we didn't lock.
        mCDDevice = nsnull;
      }
    }
  }

  virtual ~sbCDAutoDeviceLocker()
  {
    // Only unlock the device if it is already locked.
    if (mCDDevice) {
      nsresult rv;
      PRBool isLocked = PR_FALSE;
      rv = mCDDevice->GetIsDeviceLocked(&isLocked);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "Could not get the device lock state!");

      if (isLocked) {
        rv = mCDDevice->UnlockDevice();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not lock the cd device!");
      }
    }
  }

private:
  nsCOMPtr<sbICDDevice> mCDDevice;
};

//------------------------------------------------------------------------------

nsresult
sbCDDevice::ReqHandleRequestAdded()
{
  // Check for abort.
  NS_ENSURE_FALSE(ReqAbortActive(), NS_ERROR_ABORT);

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Function variables.
  nsresult rv;

  // Prevent re-entrancy.  This can happen if some API waits and runs events on
  // the request thread.  Do nothing if already handling requests.  Otherwise,
  // indicate that requests are being handled and set up to automatically set
  // the handling requests flag to false on exit.  This check is only done on
  // the request thread, so locking is not required.
  if (mIsHandlingRequests) {
    return NS_OK;
  }
  mIsHandlingRequests = PR_TRUE;
  sbCDAutoFalse autoIsHandlingRequests(&mIsHandlingRequests);

  // Start processing of the next request batch and set to automatically
  // complete the current request on exit.
  sbBaseDevice::Batch requestBatch;
  rv = StartNextRequest(requestBatch);
  NS_ENSURE_SUCCESS(rv, rv);
  if (requestBatch.empty())
    return NS_OK;
  sbAutoDeviceCompleteCurrentRequest autoComplete(this);

  // Set up to automatically set the state to STATE_IDLE on exit.  Any device
  // operation must change the state to not be idle.  This ensures that the
  // device is not prematurely removed (e.g., ejected) while the device content
  // is being accessed.
  sbDeviceStatusAutoState autoState(&mStatus, STATE_IDLE);

  // Snapshot the preferences to be used by the operations in this batch
  sbPrefBranch prefBranch(PREF_CDDEVICE_RIPBRANCH, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mPrefAutoEject = prefBranch.GetBoolPref(PREF_CDDEVICE_AUTOEJECT, PR_FALSE);
  mPrefNotifySound = prefBranch.GetBoolPref(PREF_CDDEVICE_NOTIFYSOUND,
                                            PR_FALSE);

  // If the batch isn't empty, process the batch
  while (!requestBatch.empty()) {
    // Process each request in the batch
    Batch::iterator const end = requestBatch.end();
    Batch::iterator iter = requestBatch.begin();
    while (iter != end) {
      // Check for abort.
      if (ReqAbortActive()) {
        rv = RemoveLibraryItems(iter, end);
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool isDeviceLocked = PR_FALSE;
        rv = mCDDevice->GetIsDeviceLocked(&isDeviceLocked);
        NS_ENSURE_SUCCESS(rv, rv);
        if (isDeviceLocked) {
          rv = mCDDevice->UnlockDevice();
          NS_ENSURE_SUCCESS(rv, rv);
        }

        // Don't keep a cached profile after an abort.
        mTranscodeProfile = nsnull;
        return NS_ERROR_ABORT;
      }

      TransferRequest * request = iter->get();

      // Dispatch processing of request.
      LOG(("sbCDDevice::ReqHandleRequestAdded 0x%08x\n", request->type));
      switch(request->type)
      {
        case TransferRequest::REQUEST_MOUNT :
          mStatus.ChangeState(STATE_MOUNTING);
          rv = ReqHandleMount(request);
          NS_ENSURE_SUCCESS(rv, rv);
        break;

        case TransferRequest::REQUEST_READ :
          mStatus.ChangeState(STATE_TRANSCODE);

          rv = ReqHandleRead(request);
          if (rv != NS_ERROR_ABORT) {
            // Warn of any errors
            NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
              "Could not read a track off of disc!");
          }
          break;

        case TransferRequest::REQUEST_EJECT:
          rv = Eject();
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not eject the CD!");
          break;

        case sbICDDeviceEvent::REQUEST_CDLOOKUP:
          rv = AttemptCDLookup();
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not lookup CD data!");
          break;

        case TransferRequest::REQUEST_UPDATE:
          rv = ReqHandleUpdate(request);
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not update CD data!");
          break;

        default :
          NS_WARNING("Unsupported request type.");
          break;
      }
      if (iter != end) {
        requestBatch.erase(iter);
        iter = requestBatch.begin();
      }
    }

    PRBool isAborted = (rv == NS_ERROR_ABORT);

    // If the device is currently locked, unlock it here.
    // Always unlock before checking the result of the read request.
    PRBool isDeviceLocked = PR_FALSE;
    rv = mCDDevice->GetIsDeviceLocked(&isDeviceLocked);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isDeviceLocked) {
      rv = mCDDevice->UnlockDevice();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // If one of the above operatons returned |NS_ERROR_ABORT|, we need to
    // bail out of this method.
    if (isAborted) {
      // Don't keep a cached profile after an abort.
      mTranscodeProfile = nsnull;
      return NS_ERROR_ABORT;
    }

    // Complete the current request and forget auto-completion.
    CompleteCurrentRequest();
    autoComplete.forget();

    // Start processing of the next request batch and set to automatically
    // complete the current request on exit.
    rv = StartNextRequest(requestBatch);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!requestBatch.empty())
      autoComplete.Set(this);
  }

  return NS_OK;
}

nsresult
sbCDDevice::ReqHandleMount(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRequest);

  // Function variables.
  nsresult rv;

  // Log progress.
  LOG(("Enter sbCDDevice::ReqHandleMount \n"));

  // Set up to auto-disconnect in case of error.
  SB_CD_DEVICE_AUTO_INVOKE(AutoDisconnect, Disconnect()) autoDisconnect(this);

  // Update status and set for auto-failure.
  sbDeviceStatusAutoOperationComplete autoStatus(
                                    &mStatus,
                                    sbDeviceStatusHelper::OPERATION_TYPE_MOUNT,
                                    aRequest);

  // Get the volume to mount.
  nsRefPtr<sbBaseDeviceVolume> volume;
  rv = GetVolumeForItem(aRequest->list, getter_AddRefs(volume));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume info.
  nsRefPtr<sbDeviceStatistics> deviceStatistics;
  rv = volume->GetStatistics(getter_AddRefs(deviceStatistics));
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the device library contents.
  rv = UpdateDeviceLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the library to the device statistics.
  rv = deviceStatistics->AddLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the device library CD disc hash.
  nsAutoString cdDiscHash;
  rv = GetCDDiscHash(mCDDevice, cdDiscHash);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CDDISCHASH),
                                   cdDiscHash);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update status and clear auto-failure.
  autoStatus.SetResult(NS_OK);

  // Cancel auto-disconnect.
  autoDisconnect.forget();

  // Indicate that the Device is now ready.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_READY,
                         sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));

  LOG(("Exit sbCDDevice::ReqHandleMount\n"));
  return NS_OK;
}


nsresult
sbCDDevice::UpdateDeviceLibrary(sbIDeviceLibrary* aLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Ignore library changes and set up to automatically stop ignoring.
  SetIgnoreLibraryListener(PR_TRUE);
  SetIgnoreMediaListListeners(PR_TRUE);
  SB_CD_DEVICE_AUTO_INVOKE
    (AutoIgnoreLibraryListener,
     SetIgnoreLibraryListener(PR_FALSE)) autoIgnoreLibraryListener(this);
  SB_CD_DEVICE_AUTO_INVOKE
    (AutoIgnoreMediaListListeners,
     SetIgnoreMediaListListeners(PR_FALSE)) autoIgnoreMediaListListeners(this);

  // Get the current CD disc hash.
  nsAutoString cdDiscHash;
  rv = GetCDDiscHash(mCDDevice, cdDiscHash);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the previous CD disc hash.
  nsAutoString prevCDDiscHash;
  rv = aLibrary->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CDDISCHASH),
                             prevCDDiscHash);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    prevCDDiscHash.Truncate();
    rv = NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // If the previous CD is being remounted, just set the friendly name and
  // return.
  if (cdDiscHash.Equals(prevCDDiscHash)) {
    nsAutoString albumName;
    rv = aLibrary->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                               albumName);
    if (NS_SUCCEEDED(rv) && !albumName.IsEmpty()) {
      rv = mProperties->SetFriendlyName(albumName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = mProperties->SetFriendlyName
                          (SBLocalizedString("cdrip.lookup.default_albumname"));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  // Mark the entire device library items as unavailable.
  rv = sbDeviceUtils::BulkSetProperty
                        (aLibrary,
                         NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                         NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of new media files.
  nsCOMPtr<nsIArray> newFileURIList;
  rv = GetMediaFiles(getter_AddRefs(newFileURIList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of new properties for each item.
  nsCOMPtr<nsIArray> newPropsArray;
  rv = GetMediaProperties(getter_AddRefs(newPropsArray));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_FALSE(ReqAbortActive(), NS_ERROR_ABORT);

  // Clear out any previous information.
  rv = mDeviceLibrary->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the library with the new media files.
  nsCOMPtr<nsIArray> mediaItemList;
  rv = mDeviceLibrary->BatchCreateMediaItems(newFileURIList,
                                             newPropsArray,
                                             PR_TRUE,
                                             getter_AddRefs(mediaItemList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the number of created media items.
  PRUint32 mediaItemCount;
  rv = mediaItemList->GetLength(&mediaItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set a pref to indicate to the media view that it needs to perform a lookup
  // once the view has been loaded.
  sbPrefBranch prefBranch(PREF_CDDEVICE_RIPBRANCH, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString deviceLibraryGuid;
  rv = mDeviceLibrary->GetGuid(deviceLibraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  deviceLibraryGuid.AppendLiteral(".needsLookup");
  prefBranch.SetBoolPref(NS_ConvertUTF16toUTF8(deviceLibraryGuid).get(),
                         PR_TRUE);

  return NS_OK;
}

nsresult
sbCDDevice::GetMediaFiles(nsIArray ** aURIList)
{
  nsresult rv;

  nsCOMPtr<nsIMutableArray> list =
        do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                          &rv);
      NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> toc;
  rv = mCDDevice->GetDiscTOC(getter_AddRefs(toc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!toc) {
    // If no TOC this would be really odd to occur, so we'll just return
    return NS_OK;
  }
  nsCOMPtr<nsIArray> tracks;
  rv = toc->GetTracks(getter_AddRefs(tracks));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> ioservice =
      do_ProxiedGetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOCEntry> entry;

  PRUint32 length;
  rv = tracks->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < length; ++index) {
    NS_ENSURE_FALSE(ReqAbortActive(), NS_ERROR_ABORT);
    entry = do_QueryElementAt(tracks, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Only append audio tracks to the library list.
    PRInt16 curTrackMode;
    rv = entry->GetTrackMode(&curTrackMode);
    if (NS_FAILED(rv) || curTrackMode != sbICDTOCEntry::TRACKMODE_AUDIO) {
      continue;
    }

    if (NS_SUCCEEDED(rv)) {
      nsString uriSpec;
      rv = entry->GetTrackURI(uriSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIURI> uri;
      rv = ioservice->NewURI(NS_LossyConvertUTF16toASCII(uriSpec),
                             nsnull,
                             nsnull,
                             getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = list->AppendElement(uri, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = CallQueryInterface(list, aURIList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbCDDevice::GetMediaProperties(nsIArray ** aPropertyList)
{
  nsresult rv;

  nsCOMPtr<nsIMutableArray> newPropsArray =
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> toc;
  rv = mCDDevice->GetDiscTOC(getter_AddRefs(toc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!toc) {
    // If no TOC this would be really odd to occur, so we'll just return
    return NS_OK;
  }
  nsCOMPtr<nsIArray> tracks;
  rv = toc->GetTracks(getter_AddRefs(tracks));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOCEntry> entry;

  PRUint32 length;
  rv = tracks->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < length; ++index) {
    NS_ENSURE_FALSE(ReqAbortActive(), NS_ERROR_ABORT);
    entry = do_QueryElementAt(tracks, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 trackNumber;
    rv = entry->GetTrackNumber(&trackNumber);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<sbIMutablePropertyArray> propList =
        do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // By default mark all library items as "Should Rip".
      rv = propList->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_SHOULDRIP),
                                    NS_LITERAL_STRING("1"));
      NS_ENSURE_SUCCESS(rv, rv);

      // number each track starting at 1
      nsAutoString trackNumberStr;
      trackNumberStr.AppendInt(trackNumber+1);
      rv = propList->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                                    trackNumberStr);
      NS_ENSURE_SUCCESS(rv, rv);

      // CDDA tracks always have 2 channels
      rv = propList->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CHANNELS),
                                    NS_LITERAL_STRING("2"));
      NS_ENSURE_SUCCESS(rv, rv);

      // and are always 44100 samples per second
      rv = propList->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                                    NS_LITERAL_STRING("44100"));
      NS_ENSURE_SUCCESS(rv, rv);

      // but have varying lengths (microseconds)
      PRTime duration;
      rv = entry->GetLength(&duration);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString durationStr;
      AppendInt(durationStr, duration);
      rv = propList->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                                    durationStr);
      NS_ENSURE_SUCCESS(rv, rv);

      newPropsArray->AppendElement(propList, false);
    }
  }

  rv = CallQueryInterface(newPropsArray, aPropertyList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbCDDevice::ProxyCDLookup() {
  nsresult rv;

  // Update the status
  rv = mStatus.ChangeState(sbICDDeviceEvent::STATE_LOOKINGUPCD);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Dispatch the event to notify listeners that we're about to start
  // metadata lookup
  CreateAndDispatchEvent(sbICDDeviceEvent::EVENT_CDLOOKUP_INITIATED,
                         sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));

  // Get the metadata manager and the default provider
  nsCOMPtr<sbIMetadataLookupManager> mlm =
    do_GetService("@songbirdnest.com/Songbird/MetadataLookup/manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  nsCOMPtr<sbIMetadataLookupProvider> provider;
  rv = mlm->GetDefaultProvider(getter_AddRefs(provider));

  // If there isn't a default provider, complete the CD lookup as if no results
  // were available and return.
  if (NS_FAILED(rv) || !provider) {
    rv = CompleteCDLookup(nsnull);
    NS_ENSURE_SUCCESS(rv, /* void */);
    return;
  }

  // Get our TOC
  nsCOMPtr<sbICDTOC> toc;
  rv = mCDDevice->GetDiscTOC(getter_AddRefs(toc));
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Initiate the metadata lookup
  LOG(("Querying metadata lookup provider for disc"));
  nsCOMPtr<sbIMetadataLookupJob> job;
  rv = provider->QueryDisc(toc, getter_AddRefs(job));
  if (NS_SUCCEEDED(rv) && job) {
    // Check the state of the job, if the state reflects success already, then
    // just invoke the progress listener directly, otherwise add the listener
    // to the job.
    PRUint16 jobStatus;
    rv = job->GetStatus(&jobStatus);
    NS_ENSURE_SUCCESS(rv, /* void */);
    if (jobStatus == sbIJobProgress::STATUS_SUCCEEDED ||
        jobStatus == sbIJobProgress::STATUS_FAILED)
    {
      rv = this->OnJobProgress(job);
      NS_ENSURE_SUCCESS(rv, /* void */);
    }
    else {
      rv = job->AddJobProgressListener((sbIJobProgressListener*)this);
      NS_ENSURE_SUCCESS(rv, /* void */);
    }
  }
  else {
    // If the metadata lookup provider failed to provide a job, complete CD
    // lookup as if no results were available.
    rv = CompleteCDLookup(nsnull);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

nsresult
sbCDDevice::ShowMetadataLookupDialog(const char *aLookupDialogURI,
                                     nsISimpleEnumerator *aLookupResultsEnum,
                                     PRBool aShouldReportEvents)
{
  NS_ENSURE_ARG_POINTER(aLookupDialogURI);

  // Only append the results to the arg array if a valid ptr was passed in.
  nsresult rv;
  nsCOMPtr<nsIDOMWindow> parentWindow;
  nsCOMPtr<nsIDOMWindow> domWindow;
  nsCOMPtr<nsIWindowWatcher> windowWatcher =
    do_ProxiedGetService("@mozilla.org/embedcomp/window-watcher;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = windowWatcher->GetActiveWindow(getter_AddRefs(parentWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  // Prevent the device from being ejected while the metadata results dialog
  // is getting shown. See bug 18354.
  sbCDAutoDeviceLocker cdDeviceLocker(mCDDevice);

  // Build out the dialog arguments.
  nsCOMPtr<nsIMutableArray> args =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Always send over the device's library.
  rv = args->AppendElement(mDeviceLibrary, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Not all dialogs need metadata results dialogs.
  if (aLookupResultsEnum) {
    rv = args->AppendElement(aLookupResultsEnum, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Throw up one of the metadata dialogs and prompt the user for
  // the right artist/album info and populate each track with it.
  rv = windowWatcher->OpenWindow(parentWindow,
      aLookupDialogURI,
      nsnull,
      "centerscreen,chrome,modal,titlebar,resizable,scrollbars=yes",
      args,
      getter_AddRefs(domWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aShouldReportEvents) {
    // This method has been instructed to report events, do that now.
    CreateAndDispatchEvent(sbICDDeviceEvent::EVENT_CDLOOKUP_METADATA_COMPLETE,
                           sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));
  }

  return NS_OK;
}

nsresult
sbCDDevice::AttemptCDLookup()
{
  nsresult rv;

  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThreadManager> threadMgr =
      do_GetService("@mozilla.org/thread-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIThread> mainThread;
    rv = threadMgr->GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbCDDevice, this, ProxyCDLookup);
    NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

    rv = mainThread->Dispatch(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    ProxyCDLookup();
  }

  return NS_OK;
}

nsresult
sbCDDevice::CompleteCDLookup(sbIJobProgress *aJob)
{
  nsresult rv;

  rv = mStatus.ChangeState(STATE_IDLE);

  PRUint16 numResults = 0;
  nsCOMPtr<nsISimpleEnumerator> metadataResultsEnum;
  if (aJob) {
    aJob->RemoveJobProgressListener(this);

    nsCOMPtr<sbIMetadataLookupJob> metalookupJob = do_QueryInterface(aJob, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = metalookupJob->GetMlNumResults(&numResults);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = metalookupJob->GetMetadataResults(getter_AddRefs(metadataResultsEnum));
    NS_ENSURE_SUCCESS(rv, rv);

    // Dispatch the event to notify listeners that we've finished cdlookup
    CreateAndDispatchEvent(sbICDDeviceEvent::EVENT_CDLOOKUP_COMPLETED,
                           sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));
  }

  LOG(("Number of metadata lookup results found: %d", numResults));
  // 3 cases to match up
  if (numResults == 1) {
    // Exactly 1 match found, automatically populate all the tracks with
    // the metadata found in this result

    // There is only one returned item, so no need to loop through the
    // enumerator here.
    PRBool hasMore = PR_FALSE;
    rv = metadataResultsEnum->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasMore, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsISupports> curItem;
    rv = metadataResultsEnum->GetNext(getter_AddRefs(curItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMetadataAlbumDetail> albumDetail =
      do_QueryInterface(curItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Update the device library and friendly name with the new album name.
    nsCOMPtr<sbIMutablePropertyArray> albumProperties;
    rv = albumDetail->GetProperties(getter_AddRefs(albumProperties));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString albumName;
    rv = albumProperties->GetPropertyValue(
        NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME), albumName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                     albumName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mProperties->SetFriendlyName(albumName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> trackPropResults;
    rv = albumDetail->GetTracks(getter_AddRefs(trackPropResults));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length = 0;
    rv = trackPropResults->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIMutablePropertyArray> curTrackPropArray =
        do_QueryElementAt(trackPropResults, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 propCount = 0;
      rv = curTrackPropArray->GetLength(&propCount);
      if (NS_FAILED(rv) || propCount == 0) {
        continue;
      }

      // Get the media item in this device library that has the same track
      // number (we add one due to the tracknumber being indexed from 1)
      nsString indexStr;
      indexStr.AppendInt(i+1);
      nsCOMPtr<nsIArray> tracks;
      rv = mDeviceLibrary->GetItemsByProperty(NS_LITERAL_STRING(
                                                   SB_PROPERTY_TRACKNUMBER),
                                              indexStr,
                                              getter_AddRefs(tracks));
      if(NS_FAILED(rv)) {
        continue;
      }

      // we should only ever have 1 matching track
      PRUint32 length = 0;
      rv = tracks->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(length == 1,
                   "More than one track in the device library with same #");

      // Append the new properties to the media item.
      nsCOMPtr<sbIMediaItem> curLibraryItem = do_QueryElementAt(tracks, 0, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = curLibraryItem->SetProperties(curTrackPropArray);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    rv = ShowMetadataLookupDialog(
        (numResults == 0) ? NO_CD_INFO_FOUND_DIALOG_URI
                          : MULTI_CD_INFO_FOUND_DIALOG_URI,
        metadataResultsEnum,
        aJob ? PR_FALSE : PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If the device library album name is unknown, set the device friendly name
  // to the default album name.
  nsAutoString albumName;
  PRBool       albumNameUnknown = PR_FALSE;
  rv = mDeviceLibrary->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                   albumName);
  if ((rv == NS_ERROR_NOT_AVAILABLE) || albumName.IsEmpty()) {
    albumNameUnknown = PR_TRUE;
    rv = mProperties->SetFriendlyName
                        (SBLocalizedString("cdrip.lookup.default_albumname"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If the album name is known, start an artwork scan.
  if (!albumNameUnknown) {
    nsCOMPtr<sbIAlbumArtScanner> artworkScanner =
      do_CreateInstance("@songbirdnest.com/Songbird/album-art/scanner;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Update any artwork already present.
    rv = artworkScanner->SetUpdateArtwork(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = artworkScanner->ScanListForArtwork(mDeviceLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Now that the metadata has been sorted out, post the metadata lookup
  // complete event.
  CreateAndDispatchEvent(sbICDDeviceEvent::EVENT_CDLOOKUP_METADATA_COMPLETE,
                         sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));

  return NS_OK;
}

nsresult
sbCDDevice::ReqHandleUpdate(TransferRequest * aRequest)
{
  nsresult rv;

  // See if the updated item is the device library.
  nsCOMPtr<sbILibrary> library = do_QueryInterface(aRequest->item);

  // Set the device friendly name to the album name if the library was updated.
  //XXXeps should only do this if the album name changed.
  if (library) {
    nsAutoString albumName;
    rv = library->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                              albumName);
    if (NS_SUCCEEDED(rv) && !albumName.IsEmpty()) {
      rv = mProperties->SetFriendlyName(albumName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

/*****
 * sbIJobProgressListener
 *****/
NS_IMETHODIMP
sbCDDevice::OnJobProgress(sbIJobProgress *aJob)
{
  NS_ENSURE_ARG_POINTER(aJob);

  nsresult rv;
  PRUint16 jobStatus;
  rv = aJob->GetStatus(&jobStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore still-running jobs
  if (jobStatus == sbIJobProgress::STATUS_RUNNING)
    return NS_OK;

  // Complete the CD lookup
  rv = CompleteCDLookup(aJob);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbCDDevice::GenerateFilename(sbIMediaItem *aItem,
                             nsACString & aOutFilename)
{
  NS_ENSURE_ARG_POINTER(aItem);

  // The format for the filename should be "<track#> - <title>"
  // I.e. "01 - DJFAIL MIX1"

  nsresult rv;
  nsString trackNumProp;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                          trackNumProp);
  NS_ENSURE_SUCCESS(rv, rv);

  // Pad at least one zero if we need to.
  if (trackNumProp.Length() == 1) {
    trackNumProp.Insert(NS_LITERAL_STRING("0"), 0);
  }

  nsString trackNameProp;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                          trackNameProp);
  NS_ENSURE_SUCCESS(rv, rv);

  aOutFilename.Append(NS_ConvertUTF16toUTF8(trackNumProp));
  aOutFilename.AppendLiteral(" - ");
  aOutFilename.Append(NS_ConvertUTF16toUTF8(trackNameProp));

  // Ensure we generate a valid filename by stripping illegal characters */
  aOutFilename.StripChars(FILE_ILLEGAL_CHARACTERS);
  // And path separators
  aOutFilename.StripChars(FILE_PATH_SEPARATOR);
  // And on win32, we cannot start or end with a space or dot.
  // Do this everywhere (rather than just win32) for consistency
  aOutFilename.Trim(" .", PR_TRUE, PR_TRUE);

  // Now give it a 'dummy' file extension. We'll replace this with the correct
  // extension for the format we transcode to, but we don't want to mistake
  // something else in the filename for the file extension.
  aOutFilename.AppendLiteral(".cdda");

  return NS_OK;
}

nsresult
sbCDDevice::ReqHandleRead(TransferRequest * aRequest)
{
  NS_ENSURE_ARG_POINTER(aRequest);

  LOG(("Enter sbMSCDeviceBase::ReqHandleRead\n"));

  nsresult rv;

  sbDeviceStatusAutoOperationComplete autoComplete
                                     (&mStatus,
                                      sbDeviceStatusHelper::OPERATION_TYPE_READ,
                                      aRequest);

  // Ensure that the device is locked during a read operation.
  sbCDAutoDeviceLocker autoDeviceLocker(mCDDevice);

  nsCOMPtr<sbIMediaItem> source = do_QueryInterface(aRequest->data, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbStatusPropertyValue value;
  value.SetMode(sbStatusPropertyValue::eNone);
  rv = source->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CDRIP_STATUS),
                           value.GetValue());
  NS_ENSURE_SUCCESS(rv, rv);

  sbIMediaItem * destination = aRequest->item;

  // We're creating a copy of the file so we want to break the link back to
  // the CD device library item, especially since it's semi-transient
  SBVoidString voidString;
  destination->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                           voidString);
  destination->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                           voidString);

  // Find the preferred audio transcoding profile.
  if (!mTranscodeProfile)
  {
    rv = GetDeviceTranscoding()->SelectTranscodeProfile(
                                      sbITranscodeProfile::TRANSCODE_TYPE_AUDIO,
                                      getter_AddRefs(mTranscodeProfile));
    NS_ENSURE_SUCCESS(rv, rv);

    // Cache the bitrate (as a string) so that it can be applied to
    // the tracks as they're created. This avoids the problem where
    // tracks are marked with the wrong bitrate if the user changes
    // his transcoding prefs after inserting a cd.
    PRUint32 bitrate;
    GetBitrateFromProfile(&bitrate);
    mTranscodeBitrateStr.Truncate();
    mTranscodeBitrateStr.AppendInt(bitrate/1000);
  }

  destination->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_BITRATE), mTranscodeBitrateStr);

  nsCOMPtr<nsIURI> sourceContentURI;
  rv = source->GetContentSrc(getter_AddRefs(sourceContentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Retrieve the managed path if the media management service is enabled.
  nsCOMPtr<nsIURI> musicFolderURI;
  rv = RegenerateMediaURL(destination, getter_AddRefs(musicFolderURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> musicFolderURL = do_QueryInterface(musicFolderURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString extension;
  rv = sbDeviceUtils::GetTranscodedFileExtension(mTranscodeProfile, extension);
  NS_ENSURE_SUCCESS(rv, rv);

  musicFolderURL->SetFileExtension(extension);

  // Create a unique media file and set it up for auto removal on error.
  nsCOMPtr<nsIFile> mediaFile;
  rv = CreateUniqueMediaFile(musicFolderURL,
                             getter_AddRefs(mediaFile),
                             getter_AddRefs(musicFolderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoRemoveFile autoRemoveMediaFile(mediaFile);

  // Ignore item update block
  {
    // Prevent notification while we set the content source on the item
    sbCDAutoIgnoreItem autoUnignore(this, destination);

    // Update the content URI to point to where the item will be organized.
    nsCOMPtr<nsIURI> contentURI;
    rv = sbLibraryUtils::GetContentURI(musicFolderURI,
                                       getter_AddRefs(contentURI));
    rv = destination->SetContentSrc(contentURI);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbITranscodeJob> tcJob = do_CreateInstance(
          "@songbirdnest.com/Songbird/Mediacore/Transcode/GStreamer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITranscodeJob> proxiedJob;
  rv = do_GetProxyForObject(target,
                            tcJob.get(),
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedJob));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxiedJob->SetProfile(mTranscodeProfile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString URISpec;
  rv = musicFolderURI->GetSpec(URISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Setup the ignore rule w/ the watch folder service.
  nsRefPtr<sbAutoIgnoreWatchFolderPath> autoWFPathIgnore =
    new sbAutoIgnoreWatchFolderPath();
  NS_ENSURE_TRUE(autoWFPathIgnore, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIFileURL> destFileURL =
    do_QueryInterface(musicFolderURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> destFileSpec;
  rv = destFileURL->GetFile(getter_AddRefs(destFileSpec));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString destFilePath;
  rv = destFileSpec->GetPath(destFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = autoWFPathIgnore->Init(destFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxiedJob->SetDestURI(NS_ConvertUTF8toUTF16(URISpec));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sourceContentURI->GetSpec(URISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxiedJob->SetSourceURI(NS_ConvertUTF8toUTF16(URISpec));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the metadata for the job
  nsCOMPtr<sbIPropertyArray> metadata;
  rv = aRequest->item->GetProperties(nsnull, getter_AddRefs(metadata));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxiedJob->SetMetadata(metadata);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITranscodeAlbumArt> albumArt = do_CreateInstance(
          SONGBIRD_TRANSCODEALBUMART_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> imageFormats = do_CreateInstance(
          SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Failure in album art transcoding is not fatal, we just skip the
  // art in that case.

  // When passed a zero-length array of imageFormats, this just passes through
  // the image data when we ask for the transcoded art.
  rv = albumArt->Init(aRequest->item, imageFormats);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIInputStream> imageStream;
    rv = albumArt->GetTranscodedArt(getter_AddRefs(imageStream));
    if (imageStream && NS_SUCCEEDED(rv)) {
      rv = proxiedJob->SetMetadataImage(imageStream);
      if (NS_FAILED(rv))
        NS_WARNING("Setting metadata image failed");
    }
  }

  typedef sbTranscodeProgressListener::StatusProperty StatusProperty;

  StatusProperty statusProperty(source,
                                NS_LITERAL_STRING(SB_PROPERTY_CDRIP_STATUS));

  nsCOMPtr<sbIJobCancelable> cancel = do_QueryInterface(proxiedJob);

  // Create our listener for transcode progress.
  nsRefPtr<sbTranscodeProgressListener> listener =
    sbTranscodeProgressListener::New(this,
                                     &mStatus,
                                     aRequest->item,
                                     mReqWaitMonitor,
                                     statusProperty,
                                     cancel);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  // Setup the progress listener.
  nsCOMPtr<sbIJobProgress> progress = do_QueryInterface(proxiedJob, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = progress->AddJobProgressListener(listener);
  NS_ENSURE_SUCCESS(rv, rv);

  // Setup the mediacore event listener.
  nsCOMPtr<sbIMediacoreEventTarget> eventTarget = do_QueryInterface(proxiedJob,
                                                                    &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = eventTarget->AddListener(listener);
  NS_ENSURE_SUCCESS(rv, rv);

  // This is async.
  rv = proxiedJob->Transcode();
  NS_ENSURE_SUCCESS(rv, rv);

  // Wait until the transcode job is complete.
  PRBool isComplete = PR_FALSE;
  while (!isComplete) {
    // Operate within the request wait monitor.
    nsAutoMonitor monitor(mReqWaitMonitor);

    // Check if the job is complete.
    isComplete = listener->IsComplete();

    // If not complete, wait for completion.
    if (!isComplete)
      monitor.Wait();
  }

  if (listener->IsAborted()) {
    return NS_ERROR_ABORT;
  }

  // Check the transcode status.
  PRUint16 status;
  rv = progress->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  // Log any errors.
#ifdef PR_LOGGING
  if (status != sbIJobProgress::STATUS_SUCCEEDED) {
    // Get an enumerator of the error messages.
    nsCOMPtr<nsIStringEnumerator> errorMessageEnum;
    rv = progress->GetErrorMessages(getter_AddRefs(errorMessageEnum));

    // Log each error.
    if (NS_SUCCEEDED(rv)) {
      PRBool hasMore;
      rv = errorMessageEnum->HasMore(&hasMore);
      if (NS_FAILED(rv))
        hasMore = PR_FALSE;
      while (hasMore) {
        // Get the next error message.
        nsAutoString errorMessage;
        rv = errorMessageEnum->GetNext(errorMessage);
        if (NS_FAILED(rv))
          break;

        // Log the error message.
        LOG(("sbCDDevice::ReqTranscodeWrite error %s\n",
             NS_ConvertUTF16toUTF8(errorMessage).get()));

        // Check for more error messages.
        rv = errorMessageEnum->HasMore(&hasMore);
        if (NS_FAILED(rv))
          hasMore = PR_FALSE;
      }
    }
  }
#endif

  if (status == sbIJobProgress::STATUS_SUCCEEDED) {
    // Show the item.
    {
      IgnoreMediaItem(aRequest->item);
      sbCDAutoIgnoreItem autoUnignore(this, aRequest->item);
      rv = aRequest->item->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                       NS_LITERAL_STRING("0"));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Clear auto-removal of media file.
    autoRemoveMediaFile.forget();

    autoComplete.SetResult(NS_OK);
  }
  else {
    // Return failure after cleaning up at the bottom of this method.
    rv = NS_ERROR_FAILURE;
  }

  // We need to check if this is the last item to be ripped, if so check for
  // AutoEject.
  if (aRequest->batchIndex == aRequest->batchCount) {
    nsresult rv2;
    mTranscodeProfile = nsnull;

    rv2 = HandleRipEnd();
    NS_ENSURE_SUCCESS(rv2, rv2);
  }

  return rv;
}

nsresult sbCDDevice::GetBitrateFromProfile(PRUint32 *bitrate)
{
  nsresult rv;

  nsCOMPtr<nsIArray> properties;
  rv = mTranscodeProfile->GetAudioProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> propEnumerator;
  rv = properties->Enumerate(getter_AddRefs(propEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_FALSE;
  rv = propEnumerator->HasMoreElements(&more);
  NS_ENSURE_SUCCESS(rv, rv);

  while (more)
  {
    nsCOMPtr<sbITranscodeProfileProperty> property;
    rv = propEnumerator->GetNext(getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString profilePropName;
    rv = property->GetPropertyName(profilePropName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (profilePropName.Equals(NS_LITERAL_STRING("bitrate")))
    {
      nsCOMPtr<nsIVariant> valueVariant;
      rv = property->GetValue(getter_AddRefs(valueVariant));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = valueVariant->GetAsUint32(bitrate);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = propEnumerator->HasMoreElements(&more);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
