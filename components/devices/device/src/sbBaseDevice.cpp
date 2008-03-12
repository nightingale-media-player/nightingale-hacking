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
#include <vector>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsIPropertyBag2.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

#include "sbDeviceLibrary.h"
#include "sbIDeviceEvent.h"
#include "sbIDeviceManager.h"
#include "sbILibrary.h"
#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbLibraryListenerHelpers.h"
#include "sbStandardProperties.h"

NS_IMPL_THREADSAFE_ISUPPORTS0(sbBaseDevice::TransferRequest)

/*
 * NOTE: Since nsDeque deletes the base class, which has no virtual destructor,
 * our destructor will never get called.  Hence, this class must not have any
 * destructors - and no member variables either.  This also means we cannot use
 * MOZ_COUNT_CTOR / MOZ_COUNT_DTOR for leak checking, since the DTOR will never
 * get called and it will look like things are leaking.
 */
class RequestDeallocator : public nsDequeFunctor
{
public:
  void* operator()(void* anObject)
  {
    ((sbBaseDevice::TransferRequest*)anObject)->Release();
    return nsnull;
  }
};

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

sbBaseDevice::TransferRequest * sbBaseDevice::TransferRequest::New()
{
  return new TransferRequest();
}

sbBaseDevice::sbBaseDevice() : mAbortCurrentRequest(PR_FALSE)
{
  nsDequeFunctor* deallocator = new RequestDeallocator;
  NS_ASSERTION(deallocator, "Failed to create queue deallocator");
  mRequests.SetDeallocator(deallocator);
  /* the deque owns the deallocator */
  
  mStateLock = nsAutoLock::NewLock(__FILE__ "::mStateLock");
  NS_ASSERTION(mStateLock, "Failed to allocate state lock");

  mRequestLock = nsAutoLock::NewLock(__FILE__ "::mRequestLock");
  NS_ASSERTION(mRequestLock, "Failed to allocate request lock");
}

sbBaseDevice::~sbBaseDevice()
{
  if (mStateLock)
    nsAutoLock::DestroyLock(mStateLock);

  mRequests.SetDeallocator(nsnull);
}

nsresult sbBaseDevice::PushRequest(const int aType,
                                   sbIMediaItem* aItem,
                                   sbIMediaList* aList,
                                   PRUint32 aIndex,
                                   PRUint32 aOtherIndex)
{
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(aType != TransferRequest::REQUEST_RESERVED,
                 NS_ERROR_INVALID_ARG);

  TransferRequest* req = TransferRequest::New();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  req->type = aType;
  req->item = aItem;
  req->list = aList;
  req->index = aIndex;
  req->otherIndex = aOtherIndex;
  req->batchCount = 1;
  req->batchIndex = 1;
  req->itemTransferID = 0;

  return PushRequest(req);
}

nsresult sbBaseDevice::PushRequest(TransferRequest *aRequest)
{
  NS_ENSURE_ARG_POINTER(aRequest);

  { /* scope for request lock */
    nsAutoLock lock(mRequestLock);

    /* figure out the batch count */
    nsDequeIterator begin = mRequests.Begin();
    nsDequeIterator lastIt = mRequests.End();
    TransferRequest* last = nsnull;
    if (mRequests.GetSize() > 0) {
      last = static_cast<sbBaseDevice::TransferRequest*>(lastIt.GetCurrent());
    }

    if (last) {
      aRequest->itemTransferID = last->itemTransferID + 1;
    }
    
    // when calculating batch counts, we skip over invalid requests and updates
    // (since they are not presented to the user anyway)
    if (aRequest->type != TransferRequest::REQUEST_RESERVED &&
        aRequest->type != TransferRequest::REQUEST_UPDATE)
    {
      while (last && (last->type == TransferRequest::REQUEST_RESERVED ||
                      last->type == TransferRequest::REQUEST_UPDATE))
      {
        --lastIt;
        last = static_cast<sbBaseDevice::TransferRequest*>(lastIt.GetCurrent());
        if (begin == lastIt) {
          break;
        }
      }
    }
  
    if (last && last->type == aRequest->type) {
      // same type of request, batch them
      aRequest->batchCount += last->batchCount;
      aRequest->batchIndex = aRequest->batchCount;
  
      nsDequeIterator it = mRequests.End(); 

      for(; /* see loop */; --it) {
        TransferRequest* oldReq =
          static_cast<sbBaseDevice::TransferRequest*>(it.GetCurrent());
        if (!oldReq) {
          // no request
          break;
        }
        if (oldReq->type == TransferRequest::REQUEST_RESERVED ||
            oldReq->type == TransferRequest::REQUEST_UPDATE) {
          // invalid request, or update only (doesn't matter to the user), skip
          if (begin == it) {
            // start of the queue, nothing left
            break;
          }
          continue;
        }
        if (oldReq->type != aRequest->type) {
          /* differernt request type */
          break;
        }
        NS_ASSERTION(oldReq->batchCount == aRequest->batchCount - 1,
          "Unexpected batch count in old request");
        ++(oldReq->batchCount);

          if (begin == it) {
          /* no requests left */
          break;
        }
      }
    }

    NS_ADDREF(aRequest);
    mRequests.Push(aRequest);
  } /* end scope for request lock */

  return ProcessRequest();
}

nsresult sbBaseDevice::PopRequest(sbBaseDevice::TransferRequest** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mRequestLock);
  for(;;) {
    *_retval = static_cast<sbBaseDevice::TransferRequest*>(mRequests.PopFront());
    if (!*_retval) {
      // no requests left
      return NS_OK;
    }
    if ((*_retval)->type != TransferRequest::REQUEST_RESERVED) {
      // this is a valid request (transfer reference from queue to caller)
      return NS_OK;
    }
    
    // skip over this request (and delete it)
    NS_RELEASE(*_retval);
  }
  
  NS_NOTREACHED("Breaking out of infinite loop");
}

nsresult sbBaseDevice::PeekRequest(sbBaseDevice::TransferRequest** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mRequestLock);

  for(;;) {
    *_retval = static_cast<sbBaseDevice::TransferRequest*>(mRequests.PeekFront());
    if (!*_retval) {
      // no requests left
      return NS_OK;
    }
    if ((*_retval)->type != TransferRequest::REQUEST_RESERVED) {
      // this is a valid request
      break;
    }

    // skip over this request (deleting it while we're here)
    mRequests.PopFront();
    NS_RELEASE(*_retval);
  }
  
  // the queue still needs its reference, we need to add an extra one
  NS_ADDREF(*_retval);
  return NS_OK;
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
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mRequestLock);
  
  for (int index = 0; index < mRequests.GetSize(); ++index) {
    TransferRequest* request = (TransferRequest*)mRequests.ObjectAt(index);
    // we shouldn't be able to get here if the queue is already empty
    NS_ASSERTION(request, "Attempting to remove from empty queue!");
    if (request->type != aType)
      continue;

    if (Compare(aItem, request->item) && Compare(aList, request->list)) {
      // found; remove - except we don't have a removal method
      // so instead we mark it as useless and skip it when the time comes
      request->type = TransferRequest::REQUEST_RESERVED;
      break;
    }
  }
  return NS_OK;
}

typedef std::vector<sbBaseDevice::TransferRequest *> sbBaseDeviceTransferRequests;

/**
 * This iterates over the transfer requests and removes the Songbird library
 * items that were created for the requests.
 */
static nsresult RemoveLibraryItems(sbBaseDeviceTransferRequests const & items)
{
  sbBaseDeviceTransferRequests::const_iterator const end = items.end();
  for (sbBaseDeviceTransferRequests::const_iterator iter = items.begin(); 
       iter != end;
       ++iter) {
    sbBaseDevice::TransferRequest * request = *iter; 
    // If this is a request that adds an item to the device we need to remove
    // it from the device since it never was copied
    if (request->type == sbBaseDevice::TransferRequest::REQUEST_WRITE) {
      if (request->list && request->item) {
        nsresult rv = request->list->Remove(request->item);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    NS_RELEASE(request);
  }
  return NS_OK;
}

/**
 * This copies the add requests to the inserter
 */
template <class T>
inline
void CopyLibraryItems(nsDeque const & items, T inserter)
{
  nsDequeIterator end = items.End();
  for (nsDequeIterator iter = items.Begin(); iter != end; ++iter) {
    sbBaseDevice::TransferRequest * const request = static_cast<sbBaseDevice::TransferRequest*>(iter.GetCurrent());
    if (request->type == sbBaseDevice::TransferRequest::REQUEST_WRITE) {
      NS_ADDREF(request);
      inserter = request;
    }
  }
}

nsresult sbBaseDevice::ClearRequests()
{
  sbBaseDeviceTransferRequests requests;
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);
  {
    nsAutoLock lock(mRequestLock);
    requests.reserve(mRequests.GetSize());
    // Save off the library items that are pending to avoid any
    // potential reenterancy issues when deleting them.
    CopyLibraryItems(mRequests, std::back_inserter(requests));
    mAbortCurrentRequest = PR_TRUE;
    mRequests.Erase();
  }
  nsresult rv = RemoveLibraryItems(requests);
  NS_ENSURE_SUCCESS(rv, rv);
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
  
  if (!mMediaListListeners.IsInitialized()) {
    // we expect to be unintialized, but just in case...
    if (!mMediaListListeners.Init()) {
      return NS_ERROR_FAILURE;
    }
  }
  
  nsRefPtr<sbDeviceLibrary> devLib = new sbDeviceLibrary();
  NS_ENSURE_TRUE(devLib, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = devLib->Initialize(aId);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = devLib->QueryInterface(NS_GET_IID(sbIDeviceLibrary),
                              reinterpret_cast<void**>(_retval));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<sbBaseDeviceLibraryListener> libListener = new sbBaseDeviceLibraryListener();
  NS_ENSURE_TRUE(libListener, NS_ERROR_OUT_OF_MEMORY);
  
  rv = libListener->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = devLib->AddDeviceLibraryListener(libListener);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // hook up the media list listeners to the existing lists
  nsRefPtr<MediaListListenerAttachingEnumerator> enumerator =
    new MediaListListenerAttachingEnumerator(this);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  
  rv = devLib->EnumerateItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
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

nsresult 
sbBaseDevice::CreateTransferRequest(PRUint32 aRequest, 
                                    nsIPropertyBag2 *aRequestParameters,
                                    TransferRequest **aTransferRequest)
{
  NS_ENSURE_ARG_RANGE(aRequest, REQUEST_MOUNT, REQUEST_NEW_PLAYLIST);
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
  PRUint32 batchCount = 1;
  
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

  rv = aRequestParameters->GetPropertyAsUint32(NS_LITERAL_STRING("batchCount"),
                                               &batchCount);
  if(NS_FAILED(rv)) {
    batchCount = 1;
  }

  req->type = aRequest;
  req->item = item;
  req->list = list;
  req->data = data;
  req->index = index;
  req->otherIndex = otherIndex;
  req->batchCount = batchCount;

  NS_ADDREF(*aTransferRequest = req);

  return NS_OK;
}

nsresult sbBaseDevice::CreateAndDispatchEvent(PRUint32 aType,
                                              nsIVariant *aData)
{
  nsresult rv;
  
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(aType, aData, static_cast<sbIDevice*>(this),
                            getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return DispatchEvent(deviceEvent, PR_TRUE, nsnull);
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
