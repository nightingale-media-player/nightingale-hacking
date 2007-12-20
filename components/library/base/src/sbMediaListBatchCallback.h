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

#ifndef __SB_MEDIALIST_BATCH_CALLBACK_H__
#define __SB_MEDIALIST_BATCH_CALLBACK_H__

#include <sbIMediaList.h>

/**
 * Use this class for simple wrappers around a function pointer.
 */
class sbMediaListBatchCallback : public sbIMediaListBatchCallback
{
  typedef nsresult (*sbMediaListBatchCallbackFunc)(nsISupports* aUserData);
public:

  explicit
  sbMediaListBatchCallback(sbMediaListBatchCallbackFunc aCallbackFunc)
  : mCallbackFunc(aCallbackFunc)
  {
    NS_ASSERTION(aCallbackFunc, "Null function pointer!");
  }

  NS_IMETHOD_(nsrefcnt) AddRef()
  {
    NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
    NS_ASSERT_OWNINGTHREAD(sbMediaListBatchCallback);
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "sbMediaListBatchCallback", sizeof(*this));
    return mRefCnt;
  }

  NS_IMETHOD_(nsrefcnt) Release(void)
  {
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    NS_ASSERT_OWNINGTHREAD(sbMediaListBatchCallback);
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "sbMediaListBatchCallback");
    if (mRefCnt == 0) {
      mRefCnt = 1; /* stabilize */
      NS_DELETEXPCOM(this);
      return 0;
    }
    return mRefCnt;
  }

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr)
  {
    NS_ASSERTION(aInstancePtr,
                 "QueryInterface requires a non-NULL destination!");
    nsresult rv = NS_ERROR_FAILURE;
    NS_INTERFACE_TABLE1(sbMediaListBatchCallback, sbIMediaListBatchCallback)
    return rv;
  }

  NS_IMETHOD RunBatched(nsISupports* aUserData)
  {
    return (*mCallbackFunc)(aUserData);
  }

protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

private:
  sbMediaListBatchCallbackFunc mCallbackFunc;
};

#endif /* __SB_MEDIALIST_BATCH_CALLBACK_H__ */
