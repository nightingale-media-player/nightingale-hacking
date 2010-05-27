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

#include "sbDeviceProperties.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceProperties, sbIDeviceProperties)

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsIProperty.h>
#include <prlog.h>
#include <prprf.h>
#include <prtime.h>

#include <sbStandardDeviceProperties.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceProperties:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDevicePropertiesLog = nsnull;
#define TRACE(args) PR_LOG(gDevicePropertiesLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDevicePropertiesLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbDeviceProperties::sbDeviceProperties() :
isInitialized(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gDevicePropertiesLog) {
    gDevicePropertiesLog = PR_NewLogModule("sbDeviceProperties");
  }
#endif
  mLock = nsAutoLock::NewLock("sbDevicePropertiesLock");
  // Intialize our properties container
  mProperties2 =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1");
  mProperties = do_QueryInterface(mProperties2);

  TRACE(("sbDeviceProperties[0x%.8x] - Constructed", this));

}

sbDeviceProperties::~sbDeviceProperties()
{
  TRACE(("sbDeviceProperties[0x%.8x] - Destructed", this));
  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
    mLock = nsnull;
  }
}

NS_IMETHODIMP
sbDeviceProperties::InitFriendlyName(const nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  nsresult rv =
    mProperties2->SetPropertyAsAString(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME),
                                       aFriendlyName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitVendorName(const nsAString & aVendorName)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv =
    mProperties2->SetPropertyAsAString(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                                      aVendorName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitModelNumber(nsIVariant *aModelNumber)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv =
    mProperties->SetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                             aModelNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitSerialNumber(nsIVariant *aSerialNumber)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv =
    mProperties->SetProperty(
                            NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                            aSerialNumber);
  NS_ENSURE_SUCCESS(rv, rv);

return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitFirmwareVersion(const nsAString &aFirmwareVersion)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv =
    mProperties2->SetPropertyAsAString(
                         NS_LITERAL_STRING(SB_DEVICE_PROPERTY_FIRMWARE_VERSION),
                         aFirmwareVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitDeviceLocation(nsIURI *aDeviceLocationUri)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mDeviceLocation = aDeviceLocationUri;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitDeviceIcon(nsIURI *aDeviceIconUri)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mDeviceIcon = aDeviceIconUri;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitDeviceProperties(nsIPropertyBag2 *aProperties)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsresult rv;

  // Copy the properties since the other init methods may have
  // been called
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = aProperties->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperty> property;
  nsString name;
  nsCOMPtr<nsIVariant> value;

  PRBool more;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
    rv = enumerator->GetNext(getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mProperties->SetProperty(name, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitDone()
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  isInitialized = PR_TRUE;
  return NS_OK;
}


static nsresult
GetProperty(nsIPropertyBag2 * aProperties,
            nsAString const & aProp,
            nsAString & aValue)
{
  nsString value;
  value.SetIsVoid(PR_TRUE);

  nsresult rv = aProperties->GetPropertyAsAString(aProp,
                                                  value);
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }
  aValue = value;
  return NS_OK;
}

static nsresult
GetProperty(nsIPropertyBag * aProperties,
            nsAString const & aProp,
            nsIVariant ** aValue)
{
  nsresult rv = aProperties->GetProperty(aProp, aValue);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *aValue = nsnull;
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetFriendlyName(nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  return GetProperty(mProperties2,
                     NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME),
                     aFriendlyName);
}

NS_IMETHODIMP
sbDeviceProperties::SetFriendlyName(const nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  nsresult rv =
    mProperties2->SetPropertyAsAString(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_NAME),
                                      aFriendlyName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetVendorName(nsAString & aVendorName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);

  return GetProperty(mProperties2,
                     NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                     aVendorName);;
}

NS_IMETHODIMP
sbDeviceProperties::GetModelNumber(nsIVariant * *aModelNumber)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aModelNumber);

  nsAutoLock lock(mLock);

  return GetProperty(mProperties,
                     NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                     aModelNumber);
}

NS_IMETHODIMP
sbDeviceProperties::GetSerialNumber(nsIVariant * *aSerialNumber)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aSerialNumber);

  nsAutoLock lock(mLock);

  return GetProperty(mProperties,
                     NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                     aSerialNumber);
}

NS_IMETHODIMP
sbDeviceProperties::GetFirmwareVersion(nsAString &aFirmwareVersion)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);

  return GetProperty(mProperties2,
                     NS_LITERAL_STRING(SB_DEVICE_PROPERTY_FIRMWARE_VERSION),
                     aFirmwareVersion);
}

NS_IMETHODIMP
sbDeviceProperties::GetUri(nsIURI * *aUri)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aUri);

  nsAutoLock lock(mLock);

  NS_IF_ADDREF( *aUri = mDeviceLocation );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetIconUri(nsIURI * *aIconUri)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aIconUri);

  nsAutoLock lock(mLock);

  NS_IF_ADDREF( *aIconUri = mDeviceIcon );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetProperties(nsIPropertyBag2 * *aProperties)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsAutoLock lock(mLock);

  NS_IF_ADDREF( *aProperties = mProperties2 );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::SetHidden(PRBool aHidden)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  nsresult rv =
    mProperties2->SetPropertyAsBool(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_HIDDEN),
                                    aHidden);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetHidden(PRBool *aHidden)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aHidden);

  nsAutoLock lock(mLock);
  nsresult rv = mProperties2->GetPropertyAsBool(
      NS_LITERAL_STRING(SB_DEVICE_PROPERTY_HIDDEN),
      aHidden);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

