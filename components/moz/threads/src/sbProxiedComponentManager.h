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

inline nsresult
do_GetProxyForObject(nsIEventTarget *target,
                     REFNSIID aIID,
                     nsISupports* aObj,
                     PRInt32 proxyType,
                     void** aProxyObject)
{
  nsresult rv;
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
    do_ProxiedGetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = proxyObjMgr->GetProxyForObject(target,
                                      aIID,
                                      aObj,
                                      proxyType,
                                      aProxyObject);
  return rv;
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

#endif /* __SB_PROXIEDCOMPONENTMANAGER_H__ */
