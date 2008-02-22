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

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>

#include "sbDeviceLibrary.h"
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

sbBaseDevice::sbBaseDevice()
{
  nsDequeFunctor* deallocator = new RequestDeallocator;
  NS_ASSERTION(deallocator, "Failed to create queue deallocator");
  mRequests.SetDeallocator(deallocator);
  /* the deque owns the deallocator */
  
  mRequestLock = nsAutoLock::NewLock(__FILE__ "::mRequestLock");
}

sbBaseDevice::~sbBaseDevice()
{
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

  { /* scope for request lock */
    nsAutoLock lock(mRequestLock);
  
    /* figure out the batch count */
    TransferRequest* last =
      static_cast<sbBaseDevice::TransferRequest*>(mRequests.Peek());
  
    if (last && last->type == aType) {
      // same type of request, batch them
      req->batchCount += last->batchCount;
  
      nsDequeIterator begin = mRequests.Begin();
      nsDequeIterator it = mRequests.End(); 
  
      for(; /* see loop */; --it) {
        TransferRequest* oldReq =
          static_cast<sbBaseDevice::TransferRequest*>(it.GetCurrent());
        if (!oldReq) {
          // no request
          break;
        }
        if (oldReq->type == TransferRequest::REQUEST_RESERVED) {
          // invalid request, skip
          if (begin == it) {
            // start of the queue, nothing left
            break;
          }
          continue;
        }
        if (oldReq->type != aType) {
          /* differernt request type */
          break;
        }
        NS_ASSERTION(oldReq->batchCount == req->batchCount - 1,
                     "Unexpected batch count in old request");
        ++(oldReq->batchCount);
  
        if (begin == it) {
          /* no requests left */
          break;
        }
      }
    }

    NS_ADDREF(req);
    mRequests.Push(req);
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

nsresult sbBaseDevice::ClearRequests()
{
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mRequestLock);
  mRequests.Erase();
  return NS_OK;
}

/* readonly attribute unsigned long state; */
NS_IMETHODIMP sbBaseDevice::GetState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = mState;
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
  nsresult rv = devLib->Init(aId);
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