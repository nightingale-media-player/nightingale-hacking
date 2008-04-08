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

#include <prlog.h>
#include <prprf.h>
#include <prtime.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceContent:5
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
isInitialized(false)
{
#ifdef PR_LOGGING
  if (!gDevicePropertiesLog) {
    gDevicePropertiesLog = PR_NewLogModule("sbDeviceProperties");
  }
#endif
  TRACE(("sbDeviceProperties[0x%.8x] - Constructed", this));
}

sbDeviceProperties::~sbDeviceProperties()
{
  TRACE(("sbDeviceProperties[0x%.8x] - Destructed", this));
}

NS_IMETHODIMP
sbDeviceProperties::InitFriendlyName(const nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mFriendlyName = aFriendlyName;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitVendorName(const nsAString & aVendorName)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mVendorName = aVendorName;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitModelNumber(nsIVariant *aModelNumber)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mModelNumber = aModelNumber;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitSerialNumber(nsIVariant *aSerialNumber)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  mSerialNumber = aSerialNumber;
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
  mProperties = aProperties;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::InitDone()
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);
  
  isInitialized = true;
  return NS_OK;
}


NS_IMETHODIMP
sbDeviceProperties::GetFriendlyName(nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  
  aFriendlyName = mFriendlyName;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::SetFriendlyName(const nsAString & aFriendlyName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  mFriendlyName = aFriendlyName;
  
  /* Now set it on the device */
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetVendorName(nsAString & aVendorName)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  aVendorName = mVendorName;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetModelNumber(nsIVariant * *aModelNumber)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF( *aModelNumber = mModelNumber );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetSerialNumber(nsIVariant * *aSerialNumber)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF( *aSerialNumber = mSerialNumber );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetUri(nsIURI * *aUri)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF( *aUri = mDeviceLocation );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetIconUri(nsIURI * *aIconUri)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);


  NS_IF_ADDREF( *aIconUri = mDeviceIcon );
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceProperties::GetProperties(nsIPropertyBag2 * *aProperties)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF( *aProperties = mProperties );
  return NS_OK;
}

