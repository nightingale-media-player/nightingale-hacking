/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbDefaultBaseDeviceCapabilitiesRegistrar.h"

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsISupportsPrimitives.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <prlog.h>

// Songbird includes
#include <sbDeviceXMLCapabilities.h>
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbIMediaItem.h>
#include <sbITranscodeManager.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDefaultBaseDeviceCapabilitiesRegistrar:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDefaultBaseDeviceCapabilitiesRegistrarLog = nsnull;
#define TRACE(args) PR_LOG(gDefaultBaseDeviceCapabilitiesRegistrarLog , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDefaultBaseDeviceCapabilitiesRegistrarLog , PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

sbDefaultBaseDeviceCapabilitiesRegistrar::
  sbDefaultBaseDeviceCapabilitiesRegistrar()
{
  #ifdef PR_LOGGING
    if (!gDefaultBaseDeviceCapabilitiesRegistrarLog)
      gDefaultBaseDeviceCapabilitiesRegistrarLog =
        PR_NewLogModule("sbDefaultBaseDeviceCapabilitiesRegistrar");
  #endif
}

sbDefaultBaseDeviceCapabilitiesRegistrar::
  ~sbDefaultBaseDeviceCapabilitiesRegistrar()
{
}

/* readonly attribute PRUint32 type; */
NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::GetType(PRUint32 *aType)
{
  TRACE(("%s: default", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aType);

  *aType = sbIDeviceCapabilitiesRegistrar::DEFAULT;
  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::
  AddCapabilities(sbIDevice *aDevice,
                  sbIDeviceCapabilities *aCapabilities) {
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv;

  // Look for capabilities settings in the device preferences
  nsCOMPtr<nsIVariant> capabilitiesVariant;
  rv = aDevice->GetPreference(NS_LITERAL_STRING("capabilities"),
                              getter_AddRefs(capabilitiesVariant));
  if (NS_SUCCEEDED(rv)) {
    PRUint16 dataType;
    rv = capabilitiesVariant->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    if ((dataType == nsIDataType::VTYPE_INTERFACE) ||
        (dataType == nsIDataType::VTYPE_INTERFACE_IS)) {
      nsCOMPtr<nsISupports>           capabilitiesISupports;
      nsCOMPtr<sbIDeviceCapabilities> capabilities;
      rv = capabilitiesVariant->GetAsISupports
                                  (getter_AddRefs(capabilitiesISupports));
      NS_ENSURE_SUCCESS(rv, rv);
      capabilities = do_QueryInterface(capabilitiesISupports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aCapabilities->AddCapabilities(capabilities);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  // Add capabilities from the default device capabilities document.
  rv = sbDeviceXMLCapabilities::AddCapabilities
         (aCapabilities,
          "chrome://songbird/content/devices/sbDefaultDeviceCapabilities.xml");
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::InterestedInDevice(sbIDevice *aDevice,
                                                             PRBool *retval)
{
  NS_ENSURE_ARG_POINTER(retval);
  *retval = PR_FALSE;
  return NS_OK;
}

