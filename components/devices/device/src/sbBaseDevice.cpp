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

#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbLibraryListenerHelpers.h"

NS_IMPL_ISUPPORTS0(sbBaseDevice::TransferRequest)

class RequestDeallocator : public nsDequeFunctor
{
  void* operator()(void* anObject)
  {
    ((sbBaseDevice::TransferRequest*)anObject)->Release();
    return nsnull;
  }
};

sbBaseDevice::sbBaseDevice()
{
  mRequestDeallocator = new RequestDeallocator();
  NS_ASSERTION(mRequestDeallocator, "Failed to create queue deallocator");
  mRequests.SetDeallocator(mRequestDeallocator);
}

sbBaseDevice::~sbBaseDevice()
{
  delete mRequestDeallocator;
}

nsresult sbBaseDevice::PushRequest(const int aType,
                                   sbIMediaItem* aItem,
                                   sbIMediaList* aList)
{
  NS_ENSURE_TRUE(aType != TransferRequest::REQUEST_RESERVED,
                 NS_ERROR_INVALID_ARG);

  TransferRequest* req = new TransferRequest();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  req->type = aType;
  req->item = aItem;
  req->list = aList;
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
