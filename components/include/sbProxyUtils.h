#ifndef __SBPROXYUTILS_H__
#define __SBPROXYUTILS_H__

#include <nsIEventTarget.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>

/**
 * \brief Returns a proxy for the given object.
 *
 * See NS_GetProxyForObject in nsIProxyObjectManager.idl for details.
 */
static nsresult
SB_GetProxyForObject(nsIEventTarget *target,
                     REFNSIID aIID,
                     nsISupports* aObj,
                     PRInt32 proxyType,
                     void** aProxyObject)
{
  nsresult rv;
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
    do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return proxyObjMgr->GetProxyForObject(target, aIID, aObj, proxyType,
                                        aProxyObject);
}

#endif /* __SBPROXYUTILS_H__ */
