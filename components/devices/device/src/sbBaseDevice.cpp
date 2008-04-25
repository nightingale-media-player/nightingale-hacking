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

#include "sbBaseDevice.h"

#include <algorithm>
#include <set>
#include <vector>

#include <nsIPropertyBag2.h>
#include <nsITimer.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch.h>

#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

#include <sbIDeviceContent.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceHelper.h>
#include <sbIDeviceManager.h>
#include <sbILibrary.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

#include "sbDeviceLibrary.h"
#include "sbDeviceStatus.h"
#include "sbDeviceUtils.h"
#include "sbLibraryListenerHelpers.h"
#include "sbLibraryUtils.h"
#include "sbStandardDeviceProperties.h"
#include "sbStandardProperties.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseDevice:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseDeviceLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gBaseDeviceLog, PR_LOG_WARN, args)


#define DEFAULT_COLUMNSPEC_DEVICE_LIBRARY SB_PROPERTY_TRACKNAME " 265 "     \
                                          SB_PROPERTY_DURATION " 43 "       \
                                          SB_PROPERTY_ARTISTNAME " 177 a "  \
                                          SB_PROPERTY_ALBUMNAME " 159 "     \
                                          SB_PROPERTY_GENRE " 53 "          \
                                          SB_PROPERTY_RATING   " 80"        \

#define PREF_DEVICE_PREFERENCES_BRANCH "songbird.device.preferences."
#define PREF_WARNING "warning."

NS_IMPL_THREADSAFE_ISUPPORTS0(sbBaseDevice::TransferRequest)

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

sbBaseDevice::TransferRequest * sbBaseDevice::TransferRequest::New()
{
  return new TransferRequest();
}

PRBool sbBaseDevice::TransferRequest::IsPlaylist() const
{
  if (!list)
    return PR_FALSE;
  // Is this a playlist
  nsCOMPtr<sbILibrary> libTest = do_QueryInterface(list);
  return libTest ? PR_FALSE : PR_TRUE;      
}

PRBool sbBaseDevice::TransferRequest::IsCountable() const
{
  return !IsPlaylist() && 
         type != sbIDevice::REQUEST_UPDATE;
}


/**
 * Utility function to check a transfer request queue for proper batching
 */
#if DEBUG
static void CheckRequestBatch(std::deque<nsRefPtr<sbBaseDevice::TransferRequest> >::const_iterator aBegin,
                              std::deque<nsRefPtr<sbBaseDevice::TransferRequest> >::const_iterator aEnd)
{
  PRUint32 lastIndex = 0, lastCount = 0;
  int lastType = 0;
  
  for (;aBegin != aEnd; ++aBegin) {
    if (!(*aBegin)->IsCountable()) {
      // we don't care about any non-countable things
      continue;
    }
    if (lastType == 0 || lastType != (*aBegin)->type) {
      // type change
      NS_ASSERTION(lastIndex == lastCount, "type change with missing items");
      NS_ASSERTION((lastType == 0) || ((*aBegin)->batchIndex == 1),
                   "batch does not start with 1");
      NS_ASSERTION((*aBegin)->batchCount > 0, "empty batch");
      lastType = (*aBegin)->type;
      lastCount = (*aBegin)->batchCount;
      lastIndex = (*aBegin)->batchIndex;
    } else {
      // continue batch
      NS_ASSERTION(lastCount == (*aBegin)->batchCount,
                   "mismatched batch count");
      NS_ASSERTION(lastIndex + 1 == (*aBegin)->batchIndex,
                   "unexpected index");
      lastIndex = (*aBegin)->batchIndex;
    }
  }
  
  // check end of queue too
  NS_ASSERTION(lastIndex == lastCount, "end of queue with missing items");
}
#endif /* DEBUG */

sbBaseDevice::sbBaseDevice() :
  mLastTransferID(0),
  mLastRequestPriority(PR_INT32_MIN),
  mAbortCurrentRequest(PR_FALSE)
#ifdef DEBUG
  , mMediaListListenerIgnored(PR_FALSE),
  mLibraryListenerIgnored(PR_FALSE)
#endif
{
#ifdef PR_LOGGING
  if (!gBaseDeviceLog) {
    gBaseDeviceLog = PR_NewLogModule( "sbBaseDevice" );
  }
#endif /* PR_LOGGING */

  mStateLock = nsAutoLock::NewLock(__FILE__ "::mStateLock");
  NS_ASSERTION(mStateLock, "Failed to allocate state lock");

  mRequestMonitor = nsAutoMonitor::NewMonitor(__FILE__ "::mRequestMonitor");
  NS_ASSERTION(mRequestMonitor, "Failed to allocate request monitor");
}

sbBaseDevice::~sbBaseDevice()
{
  if (mRequestMonitor)
    nsAutoMonitor::DestroyMonitor(mRequestMonitor);
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

nsresult sbBaseDevice::PushRequest(const int aType,
                                   sbIMediaItem* aItem,
                                   sbIMediaList* aList,
                                   PRUint32 aIndex,
                                   PRUint32 aOtherIndex,
                                   PRInt32 aPriority)
{
  NS_ENSURE_TRUE(mRequestMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(aType != 0, NS_ERROR_INVALID_ARG);

  nsRefPtr<TransferRequest> req = TransferRequest::New();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  req->type = aType;
  req->item = aItem;
  req->list = aList;
  req->index = aIndex;
  req->otherIndex = aOtherIndex;
  req->priority = aPriority;

  return PushRequest(req);
}

nsresult sbBaseDevice::PushRequest(TransferRequest *aRequest)
{
  NS_ENSURE_ARG_POINTER(aRequest);
  
  #if DEBUG
    // XXX Mook: if writing, make sure that we're writing from a file
    if (aRequest->type == TransferRequest::REQUEST_WRITE) {
      NS_ASSERTION(aRequest->item, "writing with no item");
      nsCOMPtr<nsIURI> aSourceURI;
      aRequest->item->GetContentSrc(getter_AddRefs(aSourceURI));
      PRBool schemeIs = PR_FALSE;
      aSourceURI->SchemeIs("file", &schemeIs);
      NS_ASSERTION(schemeIs, "writing from device, but not from file!");
    }
  #endif

  { /* scope for request lock */
    nsAutoMonitor requestMon(mRequestMonitor);
    
    /* decide where this request will be inserted */
    // figure out which queue we're looking at
    PRInt32 priority = aRequest->priority;
    TransferRequestQueue& queue = mRequests[priority];
    
    #if DEBUG
      CheckRequestBatch(queue.begin(), queue.end());
    #endif /* DEBUG */
    
    // initialize some properties of the request
    aRequest->itemTransferID = mLastTransferID++;
    aRequest->batchIndex = 1;
    aRequest->batchCount = 1;
    
    /* figure out the batch count */
    TransferRequestQueue::iterator begin = queue.begin(),
                                   end = queue.end();

    nsRefPtr<TransferRequest> last;
    if (begin != end) {
      // the queue is not empty
      last = queue.back();
    }
    
    // when calculating batch counts, we skip over invalid requests and updates
    // (since they are not presented to the user anyway)
    if (aRequest->IsCountable())
    {
      while (last && !last->IsCountable())
      {
        --end;
        if (begin == end) {
          // nothing left in the queue
          last = nsnull;
          break;
        } else {
          last = *(end - 1);
        }
      }
  
      if (last && last->type == aRequest->type) {
        // same type of request, batch them
        aRequest->batchCount += last->batchCount;
        aRequest->batchIndex = aRequest->batchCount;
        
        while (begin != end) {
          nsRefPtr<TransferRequest> oldReq = *--end;
          if (!oldReq->IsCountable()) {
            // invalid request, or update only (doesn't matter to the user), skip
            continue;
          }
          if (oldReq->type != aRequest->type) {
            /* differernt request type */
            break;
          }
          NS_ASSERTION(oldReq->batchCount == aRequest->batchCount - 1,
                       "Unexpected batch count in old request");
          ++(oldReq->batchCount);
        }
      }
    }
    
    queue.push_back(aRequest);
    
    #if DEBUG
      CheckRequestBatch(queue.begin(), queue.end());
    #endif /* DEBUG */
    
  } /* end scope for request lock */
  
  return ProcessRequest();
}

nsresult sbBaseDevice::GetFirstRequest(PRBool aRemove,
                                       sbBaseDevice::TransferRequest** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mRequestMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor reqMon(mRequestMonitor);
  
  // Note: we shouldn't remove any empty queues from the map either, if we're
  // not going to pop the request.
  
  // try to find the last peeked request
  TransferRequestQueueMap::iterator mapIt =
    mRequests.find(mLastRequestPriority);
  if (mapIt == mRequests.end()) {
    mapIt = mRequests.begin();
  }

  while (mapIt != mRequests.end()) {
    // always pop the request from the first queue
    
    TransferRequestQueue& queue = mapIt->second;
    
    #if DEBUG
      CheckRequestBatch(queue.begin(), queue.end());
    #endif
    
    if (queue.empty()) {
      if (aRemove) {
        // this queue is empty, remove it
        mRequests.erase(mapIt);
        mapIt = mRequests.begin();
      } else {
        // go to the next queue
        ++mapIt;
      }
      continue;
    }

    nsRefPtr<TransferRequest> request = queue.front();
    
    if (aRemove) {
      // we want to pop the request
      queue.pop_front();
      if (queue.empty()) {
        mRequests.erase(mapIt);
        mapIt = mRequests.begin();
      } else {
        #if DEBUG
          CheckRequestBatch(queue.begin(), queue.end());
        #endif
      }
      // forget the last seen priority
      mLastRequestPriority = PR_INT32_MIN;
    } else {
      // record the last seen priority
      mLastRequestPriority = mapIt->first;
    }
    
    request.forget(_retval);
    return NS_OK;
  }
  // there are no queues left
  mLastRequestPriority = PR_INT32_MIN;
  *_retval = nsnull;
  return NS_OK;
}

nsresult sbBaseDevice::PopRequest(sbBaseDevice::TransferRequest** _retval)
{
  return GetFirstRequest(PR_TRUE, _retval);
}

nsresult sbBaseDevice::PeekRequest(sbBaseDevice::TransferRequest** _retval)
{
  return GetFirstRequest(PR_FALSE, _retval);
}

template <class T>
NS_HIDDEN_(PRBool) Compare(T* left, nsCOMPtr<T> right)
{
  nsresult rv = NS_OK;
  PRBool isEqual = (left == right) ? PR_TRUE : PR_FALSE;
  if (!isEqual && left != nsnull && right != nsnull)
    rv = left->Equals(right, &isEqual);
  return NS_SUCCEEDED(rv) && isEqual;
}

nsresult sbBaseDevice::RemoveRequest(const int aType,
                                     sbIMediaItem* aItem,
                                     sbIMediaList* aList)
{
  NS_ENSURE_TRUE(mRequestMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor reqMon(mRequestMonitor);

  // always pop the request from the first queue
  // can't just test for empty because we may have junk requests
  TransferRequestQueueMap::iterator mapIt = mRequests.begin(),
                                    mapEnd = mRequests.end();
  
  for (; mapIt != mapEnd; ++mapIt) {
    TransferRequestQueue& queue = mapIt->second;
    
    // more possibly dummy item accounting
    TransferRequestQueue::iterator queueIt = queue.begin(),
                                   queueEnd = queue.end();
    
    #if DEBUG
      CheckRequestBatch(queueIt, queueEnd);
    #endif

    for (; queueIt != queueEnd; ++queueIt) {
      nsRefPtr<TransferRequest> request = *queueIt;

      if (request->type == aType &&
          Compare(aItem, request->item) && Compare(aList, request->list))
      {
        // found; remove
        queue.erase(queueIt);

        #if DEBUG
          CheckRequestBatch(queue.begin(), queue.end());
        #endif /* DEBUG */

        return NS_OK;
      }
    }

    #if DEBUG
      CheckRequestBatch(queue.begin(), queue.end());
    #endif /* DEBUG */
    
  }

  // there are no queues left
  return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
}

typedef std::vector<nsRefPtr<sbBaseDevice::TransferRequest> > sbBaseDeviceTransferRequests;

/**
 * This iterates over the transfer requests and removes the Songbird library
 * items that were created for the requests.
 */
static nsresult RemoveLibraryItems(sbBaseDeviceTransferRequests const & items, sbBaseDevice *aBaseDevice)
{
  sbBaseDeviceTransferRequests::const_iterator const end = items.end();
  for (sbBaseDeviceTransferRequests::const_iterator iter = items.begin(); 
       iter != end;
       ++iter) {
    nsRefPtr<sbBaseDevice::TransferRequest> request = *iter; 
    // If this is a request that adds an item to the device we need to remove
    // it from the device since it never was copied
    if (request->type == sbBaseDevice::TransferRequest::REQUEST_WRITE) {
      if (request->list && request->item) {
        nsresult rv = aBaseDevice->DeleteItem(request->list, request->item);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  return NS_OK;
}

nsresult sbBaseDevice::ClearRequests(const nsAString &aDeviceID)
{
  nsresult rv;
  sbBaseDeviceTransferRequests requests;

  nsRefPtr<TransferRequest> request;
  PeekRequest(getter_AddRefs(request));

  rv = SetState(STATE_IDLE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(mRequestMonitor, NS_ERROR_NOT_INITIALIZED);
  {
    nsAutoMonitor reqMon(mRequestMonitor);
    
    if(!mRequests.empty()) {
      // Save off the library items that are pending to avoid any
      // potential reenterancy issues when deleting them.
      TransferRequestQueueMap::const_iterator mapIt = mRequests.begin(),
                                              mapEnd = mRequests.end();
      for (; mapIt != mapEnd; ++mapIt) {
        const TransferRequestQueue& queue = mapIt->second;
        TransferRequestQueue::const_iterator queueIt = queue.begin(),
                                             queueEnd = queue.end();
        
        #if DEBUG
          CheckRequestBatch(queueIt, queueEnd);
        #endif
        
        for (; queueIt != queueEnd; ++queueIt) {
          if ((*queueIt)->type == sbBaseDevice::TransferRequest::REQUEST_WRITE) {
            requests.push_back(*queueIt);
          }
        }
        
        #if DEBUG
          CheckRequestBatch(queue.begin(), queue.end());
        #endif
      }

      mAbortCurrentRequest = PR_TRUE;
      mRequests.clear();
    }
  }
  
  // must not hold onto the monitor while we create sbDeviceStatus objects
  // because that involves proxying to the main thread

  if (!requests.empty()) {
    rv = RemoveLibraryItems(requests, this);
    NS_ENSURE_SUCCESS(rv, rv);
  
    if (request) {
      nsRefPtr<sbDeviceStatus> status;
      rv = sbDeviceUtils::CreateStatusFromRequest(aDeviceID, 
                                                  request.get(), 
                                                  getter_AddRefs(status));
      NS_ENSURE_SUCCESS(rv, rv);
  
      // All done cancelling, make sure we set appropriate state.
      status->StateMessage(NS_LITERAL_STRING("Completed"));
      status->Progress(100);
  
      status->SetItem(request->item);
      status->SetList(request->list);
  
      status->WorkItemProgress(request->batchIndex);
      status->WorkItemProgressEndCount(request->batchCount);
  
      nsCOMPtr<nsIWritableVariant> var =
        do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
  
      rv = var->SetAsISupports(request->item);
      NS_ENSURE_SUCCESS(rv, rv);
      
      CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                             var,
                             PR_TRUE);
    }
  }

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

  // set state, checking if it changed
  {
    NS_ENSURE_TRUE(mStateLock, NS_ERROR_NOT_INITIALIZED);
    nsAutoLock lock(mStateLock);
    if (mState != aState) {
      mState = aState;
      stateChanged = PR_TRUE;
    }
  }

  // send state changed event.  do it outside of lock in case event handler gets
  // called immediately and tries to read the state
  if (stateChanged) {
    nsCOMPtr<nsIWritableVariant> var = do_CreateInstance("@songbirdnest.com/Songbird/Variant;1",
                                                         &rv);
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
  
  rv = devLib->QueryInterface(NS_GET_IID(sbIDeviceLibrary),
                              reinterpret_cast<void**>(_retval));
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
  
  rv = aDevLib->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                            NS_ConvertASCIItoUTF16(NS_LITERAL_CSTRING(DEFAULT_COLUMNSPEC_DEVICE_LIBRARY)));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbBaseDeviceLibraryListener> libListener = new sbBaseDeviceLibraryListener();
  NS_ENSURE_TRUE(libListener, NS_ERROR_OUT_OF_MEMORY);
  
  rv = libListener->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aDevLib->AddDeviceLibraryListener(libListener);
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

  libListener.swap(mLibraryListener);

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
  
  mMediaListListeners.Put(list, listener);
  return NS_OK;
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
  NS_ASSERTION((aIgnoreListener && !mMediaListListenerIgnored) || 
                 (!aIgnoreListener && mMediaListListenerIgnored), 
               "SetIgnoreMediaListListeners was called more than once with the same aIgnoreListener value");
#ifdef DEBUG
  mMediaListListenerIgnored = aIgnoreListener;
#endif
  mMediaListListeners.EnumerateRead(sbBaseDevice::EnumerateIgnoreMediaListListeners, 
                                    &aIgnoreListener);
  return NS_OK;
}

nsresult
sbBaseDevice::SetIgnoreLibraryListener(PRBool aIgnoreListener)
{
  NS_ASSERTION((aIgnoreListener && !mLibraryListenerIgnored) || 
                 (!aIgnoreListener && mLibraryListenerIgnored), 
               "SetIgnoreMediaListListeners was called more than once with the same aIgnoreListener value");
#ifdef DEBUG
  mLibraryListenerIgnored = aIgnoreListener;
#endif
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
sbBaseDevice::CreateTransferRequest(PRUint32 aRequest, 
                                    nsIPropertyBag2 *aRequestParameters,
                                    TransferRequest **aTransferRequest)
{
  NS_ENSURE_ARG_RANGE(aRequest, REQUEST_MOUNT, REQUEST_FACTORY_RESET);
  NS_ENSURE_ARG_POINTER(aRequestParameters);
  NS_ENSURE_ARG_POINTER(aTransferRequest);

  TransferRequest* req = TransferRequest::New();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  
  nsresult rv;

  nsCOMPtr<sbIMediaItem> item;
  nsCOMPtr<sbIMediaList> list;
  nsCOMPtr<nsISupports>  data;

  PRUint32 index = PR_UINT32_MAX;
  PRUint32 otherIndex = PR_UINT32_MAX;
  PRInt32 priority = TransferRequest::PRIORITY_DEFAULT;
  
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

  NS_WARN_IF_FALSE(item || list || data, "No data of any kind given in request. This request will most likely fail.");

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

  rv = aRequestParameters->GetPropertyAsInt32(NS_LITERAL_STRING("priority"),
                                               &priority);
  if(NS_FAILED(rv)) {
    priority = TransferRequest::PRIORITY_DEFAULT;
  }

  req->type = aRequest;
  req->item = item;
  req->list = list;
  req->data = data;
  req->index = index;
  req->otherIndex = otherIndex;
  req->priority = priority;

  NS_ADDREF(*aTransferRequest = req);

  return NS_OK;
}

nsresult sbBaseDevice::CreateAndDispatchEvent(PRUint32 aType,
                                              nsIVariant *aData,
                                              PRBool aAsync /*= PR_TRUE*/)
{
  nsresult rv;
  
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(aType, aData, static_cast<sbIDevice*>(this),
                            getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return DispatchEvent(deviceEvent, aAsync, nsnull);
}

// by COM rules, we are only guranteed that comparing nsISupports* is valid
struct COMComparator {
  bool operator()(nsISupports* aLeft, nsISupports* aRight) const {
    return nsCOMPtr<nsISupports>(do_QueryInterface(aLeft)) <
             nsCOMPtr<nsISupports>(do_QueryInterface(aRight));
  }
};

struct EnsureSpaceForWriteRemovalHelper {
  EnsureSpaceForWriteRemovalHelper(std::set<sbIMediaItem*,COMComparator>& aItemsToRemove)
    : mItemsToRemove(aItemsToRemove.begin(), aItemsToRemove.end())
  {
  }
  
  bool operator() (nsRefPtr<sbBaseDevice::TransferRequest> & aRequest) {
    // remove all requests where either the item or the list is to be removed
    return (mItemsToRemove.end() != mItemsToRemove.find(aRequest->item) ||
            mItemsToRemove.end() != mItemsToRemove.find(aRequest->list));
  }
  
  std::set<sbIMediaItem*, COMComparator> mItemsToRemove;
};

nsresult sbBaseDevice::EnsureSpaceForWrite(TransferRequestQueue& aQueue,
                                           PRBool * aRequestRemoved,
                                           nsIArray** aItemsToWrite)
{
  LOG(("                        sbBaseDevice::EnsureSpaceForWrite++\n"));
  
  // We will possibly want to remove some writes.  Unfortunately, this may be
  // followed by some other actions (add-to-list, etc.) dealing with those items.
  // and yet we need to keep everything that does get executed in the same order
  // as the original queue.
  // So, we keep a std::map where the key is all media items to be written
  // (since writing the same item twice should not result in the space being
  // used twice).  For convenience later, the value is the size of the file.
  
  nsresult rv;
  std::map<sbIMediaItem*, PRInt64> itemsToWrite; // not owning
  // all items in itemsToWrite should have a reference from the request queue
  // XXX Mook: Do we need to be holding nsISupports* instead?
  
  NS_ASSERTION(NS_IsMainThread(),
               "sbBaseDevice::EnsureSpaceForWrite expected to be on main thread");
  
  // we have a batch that has some sort of write
  // check to see if it will all fit on the device
  TransferRequestQueue::iterator batchStart = aQueue.begin();
  TransferRequestQueue::iterator end = aQueue.end();
  
  // shift batchStart to where we last were
  while (batchStart != end && *batchStart != mRequestBatchStart)
    ++batchStart;
  
  // get the total size of the request
  PRInt64 contentLength, totalLength = 0;
  nsCOMPtr<sbIDeviceLibrary> ownerLibrary;
  
  TransferRequestQueue::iterator request;
  for (request = batchStart; request != end; ++request) {
    if ((*request)->type != TransferRequest::REQUEST_WRITE)
      continue;
    
    nsCOMPtr<sbILibrary> requestLib =
      do_QueryInterface((*request)->list, &rv);
    if (NS_FAILED(rv) || !requestLib) {
      // this is an add to playlist request, don't worry about size
      continue;
    }
    
    if (!ownerLibrary) {
      rv = sbDeviceUtils::GetDeviceLibraryForItem(this,
                                                  (*request)->list,
                                                  getter_AddRefs(ownerLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(ownerLibrary);
    } else {
      #if DEBUG
        nsCOMPtr<sbIDeviceLibrary> newLibrary;
        rv = sbDeviceUtils::GetDeviceLibraryForItem(this,
                                                    (*request)->list,
                                                    getter_AddRefs(newLibrary));
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_STATE(newLibrary);
        NS_ENSURE_STATE(SameCOMIdentity(ownerLibrary, newLibrary));
      #endif /* DEBUG */
    }
    if (itemsToWrite.end() != itemsToWrite.find((*request)->item)) {
      // this item is already in the set, don't worry about it
      continue;
    }
    
    rv = sbLibraryUtils::GetContentLength((*request)->item,
                                          &contentLength);
    NS_ENSURE_SUCCESS(rv, rv);
    
    totalLength += contentLength;
    LOG(("r(%08x) i(%08x) sbBaseDevice::EnsureSpaceForWrite - size %lld\n",
         (void*) *request, (void*) (*request)->item, contentLength));
    
    itemsToWrite[(*request)->item] = contentLength;
  }
  
  LOG(("                        sbBaseDevice::EnsureSpaceForWrite - %u items = %lld bytes\n",
       itemsToWrite.size(), totalLength));

  if (itemsToWrite.size() < 1) {
    // there were no items to write (they were all add to playlist requests)
    if (aRequestRemoved) {
      // we didn't remove anything (there was nothing to remove)
      *aRequestRemoved = PR_FALSE;
    }
    if (aItemsToWrite) {
      // the caller wants to know which items we will end up copying
      // give it an empty list
      nsCOMPtr<nsIMutableArray> items =
        do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = CallQueryInterface(items, aItemsToWrite); // addrefs
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
  
  // ask the helper to figure out if we have space
  nsCOMPtr<sbIDeviceHelper> deviceHelper =
    do_GetService("@songbirdnest.com/Songbird/Device/Base/Helper;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint64 freeSpace;
  PRBool continueWrite;
  rv = deviceHelper->HasSpaceForWrite(totalLength,
                                      ownerLibrary,
                                      this,
                                      &freeSpace,
                                      &continueWrite);
  
  LOG(("                        sbBaseDevice::EnsureSpaceForWrite - %u items = %lld bytes / free %lld bytes\n",
       itemsToWrite.size(), totalLength, freeSpace));
  
  // there are three cases to go from here:
  // (1) fill up the device, with whatever fits, in random order (sync)
  // (2) fill up the device, with whatever fits, in given order (manual)
  // (3) user chose to abort
  
  // In the (1) case, if we first shuffle the list, it becomes case (2)
  // and case (2) is just case (3) with fewer files to remove
  
  // copy the set of items into a orderable list
  std::vector<sbIMediaItem*> itemList;
  std::map<sbIMediaItem*, PRInt64>::iterator itemsBegin = itemsToWrite.begin(),
                                             itemsEnd = itemsToWrite.end(),
                                             itemsNext;
  for (itemsNext = itemsBegin; itemsNext != itemsEnd; ++itemsNext) {
    itemList.push_back(itemsNext->first);
  }
  // and a set of items we can't fit
  std::set<sbIMediaItem*, COMComparator> itemsToRemove;
  
  if (!continueWrite) {
    // we need to remove everything
    itemsToRemove.insert(itemList.begin(), itemList.end());
    itemList.clear();
    if (aRequestRemoved) {
      *aRequestRemoved = PR_TRUE;
    }
  } else {
    PRUint32 mgmtType;
    rv = ownerLibrary->GetMgmtType(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (mgmtType != sbIDeviceLibrary::MGMT_TYPE_MANUAL) {
      // shuffle the list
      std::random_shuffle(itemList.begin(), itemList.end());
    }
    
    // fit as much of the list as possible
    std::vector<sbIMediaItem*>::iterator begin = itemList.begin(),
                                         end = itemList.end(),
                                         next;
    PRInt64 bytesRemaining = freeSpace;
    for (next = begin; next != end; ++next) {
      if (bytesRemaining > itemsToWrite[*next]) {
        // this will fit
        bytesRemaining -= itemsToWrite[*next];
      } else {
        // won't fit, remove it
        itemsToRemove.insert(*next);
        *next = nsnull; /* wipe out the sbIMediaItem* */
      }
    }
  }

  // locate all the items we don't want to copy and remove from the songbird
  // device library
  std::set<sbIMediaItem*,COMComparator>::iterator removalEnd = itemsToRemove.end();
  for (request = batchStart; request != end; ++request) {
    if ((*request)->type != TransferRequest::REQUEST_WRITE ||
        removalEnd == itemsToRemove.find((*request)->item))
    {
      // not a write we don't want
      continue;
    }
    
    rv = (*request)->list->Remove((*request)->item);
    NS_ENSURE_SUCCESS(rv, rv);
  
    // tell the user about it
    nsCOMPtr<nsIWritableVariant> var =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = var->SetAsISupports((*request)->item);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_NOT_ENOUGH_FREESPACE,
                                var,
                                PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  // remove the requests from the queue; however, since removing any one item
  // will invalidate the iterators, we first shift all the valid ones to the
  // start of the queue, then remove all the unwanted ones in a batch.  Good
  // thing nsRefPtr has useful assignment.
  // while doing this, we also use this as an opportunity to fix the batch
  // counts.
  { /* scope */
    TransferRequestQueue::iterator
      batchStart, // the start of the current batch
      batchNext,  // an iterator to calculate batch sizes
      nextFree,   // the place to insert the next item
      nextItem,   // the next item to examine
      queueEnd = aQueue.end();
    batchStart = nextFree = nextItem = aQueue.begin();
    /* the request queue:
      -------------------------------------------------------------------------
         ^- batch start    ^- next free          ^- next item        queue end-^
     */
    int lastType = 0; /* reserved */
    PRUint32 batchSize = 0;
    for (;; ++nextItem) {
      /* sanity checking; not really needed */
      NS_ASSERTION(batchStart <= nextFree, "invalid iterator state");
      NS_ASSERTION(nextFree   <= nextItem, "invalid iterator state");
      NS_ASSERTION(nextItem   <= queueEnd, "invalid iterator state");
      
      bool remove = (nextItem != queueEnd) &&
                      (removalEnd != itemsToRemove.find((*nextItem)->item) ||
                       removalEnd != itemsToRemove.find((*nextItem)->list));
      
      if (remove) {
        // go to the next item; don't fix up batch counts, so that we end up
        // merging requests
        continue;
      }
      
      int newType = 0;
      if (nextItem != queueEnd) {
        newType = (*nextItem)->IsCountable() ? (*nextItem)->type : 0;
      } else {
        // at the end of queue; force a type change
        newType = lastType + 1;
      }
      if ((lastType != 0) &&
          (newType != 0) &&
          (lastType != newType))
      {
        // the request type changed; need to fix up the batch counts
        PRUint32 batchIndex = 1;
        for (; batchStart != nextFree; ++batchStart) {
          if ((*batchStart)->IsCountable()) {
            NS_ASSERTION((*batchStart)->type == lastType,
                         "Unexpected item type");
            (*batchStart)->batchIndex = batchIndex;
            (*batchStart)->batchCount = batchSize;
            ++batchIndex;
          }
        }
        NS_ASSERTION(batchIndex == batchSize + 1,
                     "Unexpected ending batch index");
        batchSize = 0;
      }
      
      if (nextItem == queueEnd) {
        // end of queue
        break;
      }
      
      if (newType != 0) {
        // this is a countable request, possibly of a different type
        lastType = newType;
        ++batchSize;
      }
      
      if (nextFree != nextItem) {
        // shift the request, if necessary
        *nextFree = *nextItem;
      }
      
      ++nextFree;
    }
    
    LOG(("                        sbBaseDevice::EnsureSpaceForWrite - erasing %u items", queueEnd - nextFree));
    aQueue.erase(nextFree, queueEnd);
  }
  
  if (aItemsToWrite) {
    // the caller wants to know which items we will end up copying
    nsCOMPtr<nsIMutableArray> items =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    std::vector<sbIMediaItem*>::const_iterator begin = itemList.begin(),
                                               end = itemList.end();
    for (; begin != end; ++begin) {
      if (*begin) {
        // this is an actual item to write
        nsCOMPtr<nsISupports> supports = do_QueryInterface(*begin, &rv);
        if (NS_SUCCEEDED(rv) && supports) {
          rv = items->AppendElement(supports, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
    rv = CallQueryInterface(items, aItemsToWrite); // addrefs
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (aRequestRemoved) {
    *aRequestRemoved = !(itemsToRemove.empty());
  }
  
  LOG(("                        sbBaseDevice::EnsureSpaceForWrite--\n"));
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

void sbBaseDevice::Init()
{
  NS_ASSERTION(NS_IsMainThread(),
               "base device init not on main thread, implement proxying");
  if (!NS_IsMainThread()) {
    // we need to get the weak reference on the main thread because it is not
    // threadsafe, but we only ever use it from the main thread
    nsCOMPtr<nsIRunnable> event = new sbBaseDeviceInitHelper(this);
    NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
    return;
  }
  
  // get a weak ref of the device manager
  nsCOMPtr<nsISupportsWeakReference> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2");
  if (!manager)
    return;
  
  nsresult rv = manager->GetWeakReference(getter_AddRefs(mParentEventTarget));
  if (NS_FAILED(rv))
    mParentEventTarget = nsnull;
}

NS_IMETHODIMP sbBaseDevice::SetWarningDialogEnabled(const nsAString & aWarning, PRBool aEnabled)
{
  nsresult rv;

  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the key for this warning
  nsString prefKey(NS_LITERAL_STRING(PREF_WARNING));
  prefKey.Append(aWarning);

  // set the pref
  rv = prefBranch->SetBoolPref(NS_ConvertUTF16toUTF8(prefKey).get(), aEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbBaseDevice::GetWarningDialogEnabled(const nsAString & aWarning, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // get the pref branch for this device
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = GetPrefBranch(getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the key for this warning
  nsString prefKey(NS_LITERAL_STRING(PREF_WARNING));
  prefKey.Append(aWarning);

  // does the pref exist?
  PRBool hasValue;
  rv = prefBranch->PrefHasUserValue(NS_ConvertUTF16toUTF8(prefKey).get(), &hasValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasValue) {
    // get the pref
    rv = prefBranch->GetBoolPref(NS_ConvertUTF16toUTF8(prefKey).get(), _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // by default warnings are enabled
    *_retval = PR_TRUE;
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

nsresult sbBaseDevice::GetPrefBranch(nsIPrefBranch** aPrefBranch)
{
  NS_ENSURE_ARG_POINTER(aPrefBranch);

  nsresult rv;

  // get the prefs service
  nsCOMPtr<nsIPrefService> prefService = 
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // get id of this device
  nsID* id;
  rv = GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  // get that as a string
  char idString[NSID_LENGTH];
  id->ToProvidedString(idString);
  NS_Free(id);

  // create the pref key
  nsCString prefKey(PREF_DEVICE_PREFERENCES_BRANCH);
  prefKey.Append(idString);
  
  return prefService->GetBranch(prefKey.get(), aPrefBranch);

}
