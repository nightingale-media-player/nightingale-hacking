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

// Self imports.
#include "sbCDDevice.h"

// Local imports
#include "sbCDLog.h"

// Songbird imports.
#include <sbArray.h>
#include <sbDeviceUtils.h>
#include <sbIDeviceEvent.h>
#include <sbLibraryUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbMediaListEnumArrayHelper.h>
#include <sbProxiedComponentManager.h>
#include <sbVariantUtils.h>

// Mozilla imports.
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
#include <nsNetUtil.h>
#include <nsThreadUtils.h>
#include <nsComponentManagerUtils.h>

#define SB_CD_DEVICE_AUTO_INVOKE(aName, aMethod)                              \
  SB_AUTO_NULL_CLASS(aName, sbCDDevice*, mValue->aMethod)

//
// Auto-disposal class wrappers.
//
//   sbCDAutoFalse             Wrapper to automatically set a boolean to false.
//   sbCDAutoUnignoreItem      Wrapper to auto unignore changes to a media
//                              item.
//

SB_AUTO_NULL_CLASS(sbCDAutoFalse, PRBool*, *mValue = PR_FALSE);
SB_AUTO_CLASS2(sbCDAutoUnignoreItem,
               sbCDDevice*,
               sbIMediaItem*,
               mValue != nsnull,
               mValue->UnignoreMediaItem(mValue2),
               mValue = nsnull);


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

  // Get the next request batch.  If a complete batch is not available, do
  // nothing more.
  sbBaseDevice::Batch requestBatch;
  rv = PopRequest(requestBatch);
  NS_ENSURE_SUCCESS(rv, rv);
  if (requestBatch.empty())
    return NS_OK;

  // Set up to automatically set the state to STATE_IDLE on exit.  Any device
  // operation must change the state to not be idle.  This ensures that the
  // device is not prematurely removed (e.g., ejected) while the device content
  // is being accessed.
  sbDeviceStatusAutoState autoState(&mStatus, STATE_IDLE);

  // If we're not waiting and the batch isn't empty process the batch
  while (!requestBatch.empty()) {

    // Process each request in the batch
    Batch::iterator const end = requestBatch.end();
    Batch::iterator iter = requestBatch.begin();
    while (iter != end) {
      TransferRequest * request = iter->get();

      // Dispatch processing of request.
      LOG(("sbCDDevice::ReqHandleRequestAdded 0x%08x\n", request->type));
      switch(request->type)
      {
        case TransferRequest::REQUEST_MOUNT :
          mStatus.ChangeState(STATE_MOUNTING);
          ReqHandleMount(request);
        break;

        case TransferRequest::REQUEST_READ :
          mStatus.ChangeState(STATE_COPYING);
          // XXX TODO: ReqHandleRead(request);
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

    // Get the next request batch.
    rv = PopRequest(requestBatch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Safely reset the abort request flag
  IsRequestAborted();

  return NS_OK;
}

void
sbCDDevice::ReqHandleMount(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aRequest, /* void */);

  // Function variables.
  nsresult rv;

  // Log progress.
  LOG(("Enter sbCDDevice::ReqHandleMount \n"));

  // Update status and set for auto-failure.
  sbDeviceStatusAutoOperationComplete autoStatus(
                                              &mStatus,
                                              sbDeviceStatusHelper::OPERATION_TYPE_MOUNT,
                                              aRequest);

  // Update the device library contents.
  rv = UpdateDeviceLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Add the library to the device statistics.
  rv = mDeviceStatistics->AddLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Show the device library.
  rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                   NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Update status and clear auto-failure.
  autoStatus.SetResult(NS_OK);

  // Indicate that the device is now ready.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_READY,
                         sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));

  // Log progress.
  LOG(("Exit sbCDDevice::ReqHandleMount\n"));
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

  // Update the library with the new media files.
  nsCOMPtr<nsIArray> mediaItemList;
  rv = UpdateDeviceLibrary(newFileURIList);
  NS_ENSURE_SUCCESS(rv, rv);

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
    entry = do_QueryElementAt(tracks, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 trackNumber;
    rv = entry->GetTrackNumber(&trackNumber);
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
sbCDDevice::UpdateDeviceLibrary(nsIArray* aFileURIList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aFileURIList);

  // Function variables.
  nsresult rv;

  // Create media items for the files.
  nsCOMPtr<nsIArray> mediaItemList;
  rv = mDeviceLibrary->BatchCreateMediaItems(aFileURIList,
                                             nsnull,
                                             PR_TRUE,
                                             getter_AddRefs(mediaItemList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the number of created media items.
  PRUint32 mediaItemCount;
  rv = mediaItemList->GetLength(&mediaItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXX TODO: Metadata retrieval goes here

  return NS_OK;
}

