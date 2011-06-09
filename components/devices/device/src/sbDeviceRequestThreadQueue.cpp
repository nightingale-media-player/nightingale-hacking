/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "sbDeviceRequestThreadQueue.h"

// Local includes
#include "sbBaseDevice.h"
#include "sbRequestItem.h"

// Mozilla includes
#include <nsArrayUtils.h>

// Songbird includes
#include <sbDebugUtils.h>
#include <sbPropertiesCID.h>
#include <sbVariantUtils.h>

// Songbird interfaces
#include <sbIDeviceEvent.h>

sbDeviceRequestThreadQueue * sbDeviceRequestThreadQueue::New()
{
  sbDeviceRequestThreadQueue * newObject;
  NS_NEWXPCOM(newObject, sbDeviceRequestThreadQueue);
  return newObject;
}

nsresult sbDeviceRequestThreadQueue::Start(sbBaseDevice * aBaseDevice)
{
  TRACE_FUNCTION("");

  NS_ENSURE_ARG_POINTER(aBaseDevice);
  sbIDevice * baseDevice = mBaseDevice;
  NS_IF_RELEASE(baseDevice);
  mBaseDevice = aBaseDevice;
  NS_ADDREF(static_cast<sbIDevice *>(mBaseDevice));
  nsresult rv = sbRequestThreadQueue::Start();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

sbDeviceRequestThreadQueue::sbDeviceRequestThreadQueue() :
    mBaseDevice(nsnull)
{
  SB_PRLOG_SETUP(sbDeviceRequestThreadQueue);
}

sbDeviceRequestThreadQueue::~sbDeviceRequestThreadQueue()
{
  TRACE_FUNCTION("");
  sbIDevice * device = mBaseDevice;
  mBaseDevice = nsnull;
  NS_IF_RELEASE(device);
}

/**
 * Static helper class for convenience
 */
class sbBaseDeviceRequestDupeCheck
{
public:
  typedef sbBaseDevice::TransferRequest TransferRequest;

  /**
   * This compares two items, handling null values, returning true if they are
   * equal
   */
  static bool CompareItems(sbIMediaItem * aLeft, sbIMediaItem * aRight)
  {
    PRBool same = PR_FALSE;
    // They're the same if neither is specified or both are and they are "equal"
    return (!aLeft && !aRight) ||
           (aLeft && aRight && NS_SUCCEEDED(aLeft->Equals(aRight, &same)) &&
            same);
  }

  /**
     * This compares both the list and item of aRequest and mRequest
     * \param aRequest the other request to be compared
     * \return True if the two requests refer to the same item and list
     */
  static bool CompareRequestItems(sbBaseDevice::TransferRequest * aRequest1,
                                  sbBaseDevice::TransferRequest * aRequest2)
  {
    NS_ENSURE_TRUE(aRequest1, false);
    NS_ENSURE_TRUE(aRequest2, false);

    return CompareItems(aRequest1->item, aRequest2->item) &&
           CompareItems(aRequest1->list, aRequest2->list);
  }

  /**
   * This is the primary function that does the work. It compares the two
   * requests and if they are a dupe returns true in aIsDupe. The return value
   * is whether the caller should continue through the queue or not.
   * \param aQueueRequest the request that is on the queue
   * \param aNewRequest The request that is about to be added to the queue
   * \param aIsDupe out parameter denoting whether the new request is a dupe
   *                of the request on the queue
   * \return Returns false if the caller should stop and not continue with
   *         the rest of the items on the queue. For instance if we see
   *         new playlist request and we have a delete request we're adding
   *         for that same playlist, then it's not a dupe, but there's no
   *         need to look further.
   */
  static bool DupeCheck(sbBaseDevice::TransferRequest * aQueueRequest,
                        sbBaseDevice::TransferRequest * aNewRequest,
                        bool & aIsDupe);
};

bool sbBaseDeviceRequestDupeCheck::DupeCheck(
                                  sbBaseDevice::TransferRequest * aQueueRequest,
                                  sbBaseDevice::TransferRequest * aNewRequest,
                                  bool & aIsDupe)
{
  PRUint32 queueType = aQueueRequest->GetType();
  PRUint32 newType = aNewRequest->GetType();

  // Default to not a duplicate
  aIsDupe = false;
  // Check the type of the request being added
  switch (newType) {
    case TransferRequest::REQUEST_WRITE: {
      // is this a write to a playlist?
      if (aNewRequest->IsPlaylist()) {
        // And the request on queue is a playlist operation
        if (aQueueRequest->IsPlaylist()) {
          switch (queueType) {

            // If the queue request is a playlist operation for the same
            // playlist dupe it
            case TransferRequest::REQUEST_WRITE:
            case TransferRequest::REQUEST_MOVE:
            case TransferRequest::REQUEST_DELETE:
              aIsDupe = CompareItems(aNewRequest->list, aQueueRequest->list);
              return aIsDupe;
            default:
              return false;
          }
        }

        // queue request wasn't a playlist operation so check for creation,
        // deletion, and updating of playlists
        switch (queueType) {
          // If we find a delete playlist, look no further, but it is not a dupe
          case TransferRequest::REQUEST_DELETE:
            aIsDupe = false;
            return CompareItems(aNewRequest->list, aQueueRequest->item);

          // If we find a new playlist request for the same playlist dupe it
          case TransferRequest::REQUEST_NEW_PLAYLIST:

          // If we find an update playlist request for the same playlist
          // dupe it
          case TransferRequest::REQUEST_UPDATE:
            aIsDupe = CompareItems(aNewRequest->list, aQueueRequest->item);
            return aIsDupe;

          default:
            return false;
        }
      }

      // Not a playlist operation, if it's the same item and
      // type then dupe it
      aIsDupe = (queueType == TransferRequest::REQUEST_WRITE) &&
                CompareRequestItems(aQueueRequest, aNewRequest);
      return aIsDupe;
    }
    case TransferRequest::REQUEST_DELETE: {
      // If we're dealing with a remove item from playlist
      if (aNewRequest->IsPlaylist()) {
        // If the playlist on the queue is the same as this one
        if (CompareItems(aNewRequest->list, aQueueRequest->list)) {
          switch (queueType) {
            case TransferRequest::REQUEST_MOVE:
            case TransferRequest::REQUEST_DELETE:
            case TransferRequest::REQUEST_UPDATE:
            case TransferRequest::REQUEST_WRITE: {
              aIsDupe = true;
              return true;
            }
            default:
              return false;
            break;
          }
        }
        return false;
      }
      // If the item we're deleting has a previous request in the queue
      // remove it unless it's another delete then dupe it
      if (CompareRequestItems(aQueueRequest, aNewRequest)) {
        switch (queueType) {
          case TransferRequest::REQUEST_NEW_PLAYLIST:
          case TransferRequest::REQUEST_WRITE:
          case TransferRequest::REQUEST_UPDATE:
            // Stop, but not a dupe
            return true;
          case TransferRequest::REQUEST_DELETE:
            aIsDupe = true;
            return true;
          default:
            return false;
        }
      }
      return false;
    }
    case TransferRequest::REQUEST_NEW_PLAYLIST: {
      // If we have new playlist request for the same item dupe this one
      if (queueType == newType) {
         aIsDupe = CompareItems(aNewRequest->item, aQueueRequest->item);
         return aIsDupe;
      }
      // If we find a delete request for this new playlist then we want to
      // stop looking in the queue
      return (queueType == TransferRequest::REQUEST_DELETE) &&
             CompareItems(aNewRequest->item, aQueueRequest->item);
    }
    case TransferRequest::REQUEST_MOVE: {
      if (aNewRequest->IsPlaylist()) {
        switch (queueType) {
          // Move after creation is unnecessary, dupe it
          case TransferRequest::REQUEST_NEW_PLAYLIST:
            aIsDupe = CompareItems(aNewRequest->list, aNewRequest->item);
            return aIsDupe;
          // Move after writing is unnecessary, dupe it
          case TransferRequest::REQUEST_WRITE:
            aIsDupe = aNewRequest->IsPlaylist() &&
                   CompareItems(aNewRequest->list, aQueueRequest->list);
            return aIsDupe;
          // Move after update is unnecessary, dupe it
          case TransferRequest::REQUEST_UPDATE: {
            aIsDupe = CompareItems(aNewRequest->list, aQueueRequest->item);
            return aIsDupe;
          }
          default:
            return false;
        }
      }
      return false;
    }
    case TransferRequest::REQUEST_UPDATE: {
      // If the queue item is a playlist operation then this request can
      // be treated as a dupe
      if (aQueueRequest->IsPlaylist()) {
        aIsDupe = CompareItems(aNewRequest->item, aQueueRequest->list);
        return aIsDupe;
      }
      switch (queueType) {
        case TransferRequest::REQUEST_WRITE:
        case TransferRequest::REQUEST_NEW_PLAYLIST:
        case TransferRequest::REQUEST_UPDATE:
          aIsDupe = CompareRequestItems(aQueueRequest, aNewRequest);
          return aIsDupe;

        case TransferRequest::REQUEST_DELETE:
          aIsDupe = CompareRequestItems(aQueueRequest, aNewRequest);
          return aIsDupe;

        default:
          return false;
      }
    }
    case TransferRequest::REQUEST_EJECT:
    case TransferRequest::REQUEST_FORMAT:
    case TransferRequest::REQUEST_MOUNT:
    case TransferRequest::REQUEST_READ:
    case TransferRequest::REQUEST_SUSPEND:
    case TransferRequest::REQUEST_SYNC:
    case TransferRequest::REQUEST_WIPE:
    default: {
      aIsDupe = CompareRequestItems(aQueueRequest, aNewRequest) &&
                newType == queueType;
      return aIsDupe;
    }
  }
  // We should never get here, but some compilers erroneously throw a warning
  // if we don't have a return
  return false;
}

nsresult
sbDeviceRequestThreadQueue::IsDuplicateRequest(sbRequestItem * aQueueRequest,
                                               sbRequestItem * aRequest,
                                               bool & aIsDuplicate,
                                               bool & aContinueChecking)
{
  NS_ENSURE_ARG_POINTER(aQueueRequest);
  NS_ENSURE_ARG_POINTER(aRequest);

  nsresult rv;

  sbBaseDevice::TransferRequest * queueRequest =
      static_cast<sbBaseDevice::TransferRequest*>(aQueueRequest);

  sbBaseDevice::TransferRequest * request =
      static_cast<sbBaseDevice::TransferRequest*>(aRequest);

  PRUint32 type = request->GetType();

  // reverse search through the queue for a comparable request
  bool isDuplicate = PR_FALSE;
  PRBool continueChecking =
           sbBaseDeviceRequestDupeCheck::DupeCheck(queueRequest,
                                                   request,
                                                   isDuplicate);


  // If the requests are item update requests, merge the updated properties.
  if (isDuplicate) {
    if (type == sbBaseDevice::TransferRequest::REQUEST_UPDATE &&
      !request->IsPlaylist()) {
      // Create a new, merged property array.
      nsCOMPtr<sbIMutablePropertyArray>
        mergedPropertyList =
          do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // Add the duplicate in the queue properties.
      nsCOMPtr<sbIPropertyArray> propertyList;
      propertyList = do_QueryInterface(queueRequest->data, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mergedPropertyList->AppendProperties(propertyList, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);

      // Add the new request properties.
      propertyList = do_QueryInterface(request->data, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mergedPropertyList->AppendProperties(propertyList, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);

      // Change the duplicate in the queue properties to the merged set.
      queueRequest->data = mergedPropertyList;
    }
    // If we found a duplicate playlist request, we may need to change it to
     // update request. Currently all devices recreate playlists on any
     // modification.
    else if (request->IsPlaylist()) {
      PRUint32 queueType = aRequest->GetType();

       // If the previous request was a write or move then change it to an update
      switch (queueType) {
        case sbBaseDevice::TransferRequest::REQUEST_WRITE:
        case sbBaseDevice::TransferRequest::REQUEST_MOVE: {
          aQueueRequest->SetType(sbBaseDevice::TransferRequest::REQUEST_UPDATE);

          queueRequest->item = queueRequest->list;
          nsCOMPtr<sbILibrary> library;
          queueRequest->list->GetLibrary(getter_AddRefs(library));
          queueRequest->list = library;
        }
        break;
      }
    }
  }
  aIsDuplicate = isDuplicate;
  aContinueChecking = continueChecking;

  return NS_OK;
}

void sbDeviceRequestThreadQueue::CompleteRequests() {

  sbRequestThreadQueue::CompleteRequests();

  {
    nsAutoLock lock(mLock);

    nsresult rv = mBaseDevice->ChangeState(sbIDevice::STATE_IDLE);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to clear cancel state of aborted device");
    }
  }
}

nsresult sbDeviceRequestThreadQueue::CleanupBatch(Batch & aBatch)
{
  TRACE_FUNCTION("");

  nsresult rv;
  nsInterfaceHashtable<nsISupportsHashKey, nsIMutableArray> groupedItems;
  groupedItems.Init();

  // Accumulate all the items that have not been processed so we can remove
  // their corresponding items from the library.
  const Batch::const_iterator end = aBatch.end();
  for (Batch::const_iterator iter = aBatch.begin();
       iter != end;
       ++iter) {
    sbBaseDevice::TransferRequest * request =
      static_cast<sbBaseDevice::TransferRequest *>(*iter);

    // If the request was processed nothing to cleanup
    bool processed = request->GetIsProcessed();
    if (processed) {
      continue;
    }

    PRUint32 type = request->GetType();

    // If this is a request that adds an item to the device we need to remove
    // it from the device since it never was copied. We store these via
    // media list so that we can use the batch logic to efficiently remove
    // them.
    switch (type) {
      case sbBaseDevice::TransferRequest::REQUEST_WRITE:
      case sbBaseDevice::TransferRequest::REQUEST_READ: {
        if (request->item) {
          nsCOMPtr<nsIMutableArray> items;
          groupedItems.Get(request->list, getter_AddRefs(items));
          if (!items) {
            items = do_CreateInstance(
                             "@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                             &rv);
            NS_ENSURE_TRUE(groupedItems.Put(request->list, items),
                           NS_ERROR_OUT_OF_MEMORY);
          }
          rv = items->AppendElement(request->item, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      continue;
    }
  }
  // If there are no items, don't bother do anything.
  if (groupedItems.Count() > 0 && mBaseDevice->mLibraryListener) {
    sbBaseDevice::AutoListenerIgnore ignore(mBaseDevice);
    groupedItems.Enumerate(RemoveLibraryEnumerator, mBaseDevice);
  }

  return NS_OK;
}

nsresult sbDeviceRequestThreadQueue::OnThreadStop()
{
  TRACE_FUNCTION("");

  nsresult rv;

  if (mBaseDevice) {
    rv = mBaseDevice->DeviceSpecificDisconnect();
    NS_ENSURE_SUCCESS(rv, rv);

    // Now notify everyone else that we have been removed.
    mBaseDevice->CreateAndDispatchDeviceManagerEvent(
      sbIDeviceEvent::EVENT_DEVICE_REMOVED,
      sbNewVariant(static_cast<sbIDevice*>(mBaseDevice)));

    sbIDevice * device = mBaseDevice;
    mBaseDevice = nsnull;
    NS_IF_RELEASE(device);
  }
  else {
    NS_WARNING("mBaseDevice is null in "
               "sbDeviceRequestThreadActions::OnThreadShutdown");
  }
  return NS_OK;
}

PLDHashOperator
sbDeviceRequestThreadQueue::RemoveLibraryEnumerator(
                                             nsISupports * aList,
                                             nsCOMPtr<nsIMutableArray> & aItems,
                                             void * aUserArg)
{
  NS_ENSURE_TRUE(aList, PL_DHASH_NEXT);
  NS_ENSURE_TRUE(aItems, PL_DHASH_NEXT);

  sbBaseDevice * const device =
    static_cast<sbBaseDevice*>(aUserArg);

  NS_ASSERTION(device->mLibraryListener,
               "sbDeviceRequestThreadQueue::RemoveLibraryEnumerator called "
               "with a device that does not have mLibraryListener set");
  sbBaseDevice::AutoListenerIgnore ignore(device);

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

nsresult
sbDeviceRequestThreadQueue::ProcessBatch(Batch & aBatch)
{
  TRACE_FUNCTION("");

  NS_ENSURE_STATE(mBaseDevice);

  nsresult rv;

  rv = mBaseDevice->ProcessBatch(aBatch);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
