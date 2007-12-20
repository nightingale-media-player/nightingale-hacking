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
