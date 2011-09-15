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

#ifndef __SB_PROXIEDCOMPONENTMANAGER_H__
#define __SB_PROXIEDCOMPONENTMANAGER_H__

#include <nsIRunnable.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsIProxyObjectManager.h>
#include <nsXPCOMCIDInternal.h>
#include <nsProxyRelease.h>

class sbProxiedComponentManagerRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  sbProxiedComponentManagerRunnable(PRBool aIsService,
                                    const nsCID& aCID,
                                    const char* aContractID,
                                    const nsIID& aIID) :
    mIsService(aIsService),
    mCID(aCID),
    mContractID(aContractID),
    mIID(aIID) {}

  PRBool mIsService;
  const nsCID& mCID;
  const char* mContractID;
  const nsIID& mIID;
  nsCOMPtr<nsISupports> mSupports;
  nsresult mResult;
};

class NS_COM_GLUE sbCreateProxiedComponent : public nsCOMPtr_helper
{
public:
  sbCreateProxiedComponent(const nsCID& aCID,
                           PRBool aIsService,
                           nsresult* aErrorPtr)
    : mCID(aCID),
      mContractID(nsnull),
      mIsService(aIsService),
      mErrorPtr(aErrorPtr)
  {
  }

  sbCreateProxiedComponent(const char* aContractID,
                           PRBool aIsService,
                           nsresult* aErrorPtr)
    : mCID(NS_GET_IID(nsISupports)),
      mContractID(aContractID),
      mIsService(aIsService),
      mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  const nsCID& mCID;
  const char*  mContractID;
  PRBool       mIsService;
  nsresult*    mErrorPtr;
};

inline
const sbCreateProxiedComponent
do_ProxiedCreateInstance(const nsCID& aCID, nsresult* error = 0)
{
  return sbCreateProxiedComponent(aCID, PR_FALSE, error);
}

inline
const sbCreateProxiedComponent
do_ProxiedGetService(const nsCID& aCID, nsresult* error = 0)
{
  return sbCreateProxiedComponent(aCID, PR_TRUE, error);
}

inline
const sbCreateProxiedComponent
do_ProxiedCreateInstance(const char* aContractID, nsresult* error = 0)
{
  return sbCreateProxiedComponent(aContractID, PR_FALSE, error);
}

inline
const sbCreateProxiedComponent
do_ProxiedGetService(const char* aContractID, nsresult* error = 0)
{
  return sbCreateProxiedComponent(aContractID, PR_TRUE, error);
}

/* Get a proxy using a proxied nsIProxyObjectManager (acquired with
   do_ProxiedGetService()).
   This is useful because though USING a proxy may spin the event loop,
   acquiring one via this function does not (assuming you've acquired the
   proxy object manager earlier, where spinning the event loop is acceptable).
 */
inline nsresult
do_GetProxyForObjectWithManager(nsIProxyObjectManager *aProxyObjMgr,
                                nsIEventTarget *aTarget,
                                REFNSIID aIID,
                                nsISupports* aObj,
                                PRInt32 aProxyType,
                                void** aProxyObject)
{
  nsresult rv;
  /* The proxied aProxyObjMgr can only actually proxy real target objects, so
     the special magic values available for 'target' must be resolved here.
   */
  nsCOMPtr<nsIThread> thread;
  nsCOMPtr<nsIEventTarget> target;
  if (aTarget == NS_PROXY_TO_CURRENT_THREAD) {
    rv = NS_GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_SUCCESS(rv, rv);

    target = thread;
  }
  else if (aTarget == NS_PROXY_TO_MAIN_THREAD) {
    rv = NS_GetMainThread(getter_AddRefs(thread));
    NS_ENSURE_SUCCESS(rv, rv);

    target = thread;
  }
  else {
    target = aTarget;
  }

  rv = aProxyObjMgr->GetProxyForObject(target,
                                       aIID,
                                       aObj,
                                       aProxyType,
                                       aProxyObject);
  return rv;
}

inline nsresult
do_GetProxyForObject(nsIEventTarget *aTarget,
                     REFNSIID aIID,
                     nsISupports* aObj,
                     PRInt32 aProxyType,
                     void** aProxyObject)
{
  nsresult rv;
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
    do_ProxiedGetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = do_GetProxyForObjectWithManager(proxyObjMgr,
                                       aTarget,
                                       aIID,
                                       aObj,
                                       aProxyType,
                                       aProxyObject);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

template <class T>
inline nsresult
do_GetProxyForObject(nsIEventTarget * aTarget,
                     T * aObj,
                     PRInt32 aProxyType,
                     void ** aProxyObject)
{
  return do_GetProxyForObject(aTarget,
                              NS_GET_TEMPLATE_IID(T),
                              aObj,
                              aProxyType,
                              aProxyObject);
}


/**
 * Class used by do_MainThreadQueryInterface.
 */

class NS_COM_GLUE sbMainThreadQueryInterface : public nsCOMPtr_helper
{
public:
  sbMainThreadQueryInterface(nsISupports* aSupports,
                             nsresult*    aResult) :
    mSupports(aSupports),
    mResult(aResult)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  nsISupports* mSupports;
  nsresult*    mResult;
};


/**
 * Return a QI'd version of the object specified by aSupports whose methods will
 * execute on the main thread.  If this function is called on the main thread,
 * the returned object will simply be the QI'd version of aSupports.  Otherwise,
 * the returned object will by a main thread synchronous proxy QI'd version of
 * aSupports.  The result code may optionally be returned in aResult.
 *
 * \param aSupports             Object for which to get QI'd main thread object.
 * \param aResult               Optional returned result.
 *
 * \return                      QI'd main thread object.
 *
 * Example:
 *
 *   nsresult rv;
 *   nsCOMPtr<nsIURI> mainThreadURI = do_MainThreadQueryInterface(uri, &rv);
 *   NS_ENSURE_SUCCESS(rv, rv);
 */

inline sbMainThreadQueryInterface
do_MainThreadQueryInterface(nsISupports* aSupports,
                            nsresult*    aResult = nsnull)
{
  return sbMainThreadQueryInterface(aSupports, aResult);
}


template <class T> inline void
do_MainThreadQueryInterface(already_AddRefed<T>& aSupports,
                            nsresult*            aResult = nsnull)
{
  // This signature exists solely to _stop_ you from doing the bad thing.
  // Saying |do_MainThreadQueryInterface()| on a pointer that is not otherwise
  // owned by someone else is an automatic leak.  See
  // <http://bugzilla.mozilla.org/show_bug.cgi?id=8221>.
}


#endif /* __SB_PROXIEDCOMPONENTMANAGER_H__ */

