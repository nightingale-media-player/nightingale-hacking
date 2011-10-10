/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#include "sbDeviceFirmwareSupport.h"

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIMutableArray.h>
#include <nsISimpleEnumerator.h>
#include <nsISupportsPrimitives.h>

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsXPCOMCID.h>

NS_IMPL_THREADSAFE_ADDREF(sbDeviceFirmwareSupport)
NS_IMPL_THREADSAFE_RELEASE(sbDeviceFirmwareSupport)

NS_IMPL_QUERY_INTERFACE2_CI(sbDeviceFirmwareSupport,
                            sbIDeviceFirmwareSupport,
                            nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER1(sbDeviceFirmwareSupport,
                             sbIDeviceFirmwareSupport)

NS_DECL_CLASSINFO(sbDeviceFirmwareSupport)
NS_IMPL_THREADSAFE_CI(sbDeviceFirmwareSupport)

sbDeviceFirmwareSupport::sbDeviceFirmwareSupport()
: mMonitor(nsnull)
{
}

sbDeviceFirmwareSupport::~sbDeviceFirmwareSupport()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

/* readonly attribute AString deviceFriendlyName; */
NS_IMETHODIMP 
sbDeviceFirmwareSupport::GetDeviceFriendlyName(nsAString & aDeviceFriendlyName)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  aDeviceFriendlyName = mDeviceFriendlyName;

  return NS_OK;
}

/* readonly attribute unsigned long deviceVendorID; */
NS_IMETHODIMP 
sbDeviceFirmwareSupport::GetDeviceVendorID(PRUint32 *aDeviceVendorID)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDeviceVendorID);

  nsAutoMonitor mon(mMonitor);
  *aDeviceVendorID = mDeviceVendorID;

  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator deviceProductIDs; */
NS_IMETHODIMP 
sbDeviceFirmwareSupport::GetDeviceProductIDs(nsISimpleEnumerator * *aDeviceProductIDs)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDeviceProductIDs);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = mDeviceProductIDs->Enumerate(aDeviceProductIDs);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void init (in AString aDeviceName, in unsigned long aVendorID, in nsISimpleEnumerator aDeviceProductIDs); */
NS_IMETHODIMP 
sbDeviceFirmwareSupport::Init(const nsAString & aDeviceName, 
                              PRUint32 aVendorID, 
                              nsISimpleEnumerator *aDeviceProductIDs)
{
  NS_ENSURE_ARG_POINTER(aDeviceProductIDs);
  NS_ENSURE_FALSE(mMonitor, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_FALSE(mDeviceProductIDs, NS_ERROR_ALREADY_INITIALIZED);

  mMonitor = nsAutoMonitor::NewMonitor("sbDeviceFirmwareSupport::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  mDeviceFriendlyName = aDeviceName;
  mDeviceVendorID = aVendorID;

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRBool hasMore = PR_FALSE;

  nsCOMPtr<nsIMutableArray> productIDs =
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  while(NS_SUCCEEDED(rv = aDeviceProductIDs->HasMoreElements(&hasMore)) &&
        hasMore) {
    nsCOMPtr<nsISupports> element;
    rv = aDeviceProductIDs->GetNext(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsPRUint32> productID = do_QueryInterface(element, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = productIDs->AppendElement(productID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mDeviceProductIDs = productIDs;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareSupport::SimpleInit(const nsAString & aDeviceName, 
                                    PRUint32 aVendorID, 
                                    PRUint32 aProductID)
{
  NS_ENSURE_FALSE(mMonitor, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_FALSE(mDeviceProductIDs, NS_ERROR_ALREADY_INITIALIZED);

  mMonitor = nsAutoMonitor::NewMonitor("sbDeviceFirmwareSupport::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_ERROR_UNEXPECTED;

  mDeviceProductIDs =
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDeviceFriendlyName = aDeviceName;
  mDeviceVendorID = aVendorID;

  nsCOMPtr<nsISupportsPRUint32> productID = 
    do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = productID->SetData(aProductID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDeviceProductIDs->AppendElement(productID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareSupport::AppendProductID(PRUint32 aProductID) 
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);

  nsCOMPtr<nsISupportsPRUint32> productID = 
    do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = productID->SetData(aProductID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDeviceProductIDs->AppendElement(productID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
