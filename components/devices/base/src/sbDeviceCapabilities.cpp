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

#include "sbDeviceCapabilities.h"

#include <nsIMutableArray.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsArrayUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceCapabilities, sbIDeviceCapabilities)

#include <prlog.h>
#include <prprf.h>
#include <prtime.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceContent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceCapabilitiesLog = nsnull;
#define TRACE(args) PR_LOG(gDeviceCapabilitiesLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDeviceCapabilitiesLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbDeviceCapabilities::sbDeviceCapabilities() :
isInitialized(false)
{
#ifdef PR_LOGGING
  if (!gDeviceCapabilitiesLog) {
    gDeviceCapabilitiesLog = PR_NewLogModule("sbDeviceCapabilities");
  }
#endif
  TRACE(("sbDeviceCapabilities[0x%.8x] - Constructed", this));
}

sbDeviceCapabilities::~sbDeviceCapabilities()
{
  TRACE(("sbDeviceCapabilities[0x%.8x] - Destructed", this));
}


NS_IMETHODIMP
sbDeviceCapabilities::InitDone()
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  /* set this so we are not called again */
  isInitialized = true;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::SetFunctionTypes(PRUint32 *aFunctionTypes,
                                       PRUint32 aFunctionTypesCount)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  PRUint32 arrayCounter;
  for (PRUint32 arrayCounter = 0; arrayCounter < aFunctionTypesCount; arrayCounter++) {
    mFunctionTypes[arrayCounter] = aFunctionTypes[arrayCounter];
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::SetEventTypes(PRUint32 *aEventTypes,
                                    PRUint32 aEventTypesCount)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  for (PRUint32 arrayCounter = 0; arrayCounter < aEventTypesCount; arrayCounter++) {
    mSupportedEvents[arrayCounter] = aEventTypes[arrayCounter];
  }

  return NS_OK;
}


NS_IMETHODIMP
sbDeviceCapabilities::AddContentTypes(PRUint32 aFunctionType,
                                      PRUint32 *aContentTypes,
                                      PRUint32 aContentTypesCount)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsTArray<PRUint32> nContentTypes;
  
  for (PRUint32 arrayCounter = 0; arrayCounter < aContentTypesCount; arrayCounter++) {
    nContentTypes[arrayCounter] = aContentTypes[arrayCounter];
  }
  
  mContentTypes.Put(aFunctionType, &nContentTypes);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::AddFormats(PRUint32 aContentType,
                                 PRUint32 *aFormats,
                                 PRUint32 aFormatsCount)
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsTArray<PRUint32> nFormats;
  
  for (PRUint32 arrayCounter = 0; arrayCounter < aFormatsCount; arrayCounter++) {
    nFormats[arrayCounter] = aFormats[arrayCounter];
  }
  
  mSupportedFormats.Put(aContentType, &nFormats);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedFunctionTypes(PRUint32 *aArrayCount,
                                                PRUint32 **aFunctionTypes)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  PRUint32 arrayLen = mFunctionTypes.Length();
  PRUint32* outArray = (PRUint32*)nsMemory::Alloc(arrayLen * sizeof(PRUint32));
  if (!outArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  for (PRUint32 arrayCounter = 0; arrayCounter < arrayLen; arrayCounter++) {
    outArray[arrayCounter] = mFunctionTypes[arrayCounter];
  }

  *aArrayCount = arrayLen;
  *aFunctionTypes = outArray;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedContentTypes(PRUint32 aFunctionType,
                                               PRUint32 *aArrayCount,
                                               PRUint32 **aContentTypes)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  
  nsTArray<PRUint32>* contentTypes;

  if (!mContentTypes.Get(aFunctionType, &contentTypes)) {
    NS_WARNING("There are no content types for the requested function type.");
    return NS_ERROR_NOT_AVAILABLE;
  }

  PRUint32 arrayLen = contentTypes->Length();
  PRUint32* outArray = (PRUint32*)nsMemory::Alloc(arrayLen * sizeof(PRUint32));
  if (!outArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  for (PRUint32 arrayCounter = 0; arrayCounter < arrayLen; arrayCounter++) {
    outArray[arrayCounter] = contentTypes->ElementAt(arrayCounter);
  }

  *aArrayCount = arrayLen;
  *aContentTypes = outArray;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedFormats(PRUint32 aContentType,
                                          PRUint32 *aArrayCount,
                                          PRUint32 **aSupportedFormats)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  
  nsTArray<PRUint32>* supportedFormats;

  if (!mSupportedFormats.Get(aContentType, &supportedFormats)) {
    NS_WARNING("Requseted content type is not available for this device.");
    return NS_ERROR_NOT_AVAILABLE;
  }

  PRUint32 arrayLen = supportedFormats->Length();
  PRUint32* outArray = (PRUint32*)nsMemory::Alloc(arrayLen * sizeof(PRUint32));
  if (!outArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  for (PRUint32 arrayCounter = 0; arrayCounter < arrayLen; arrayCounter++) {
    outArray[arrayCounter] = supportedFormats->ElementAt(arrayCounter);
  }

  *aArrayCount = arrayLen;
  *aSupportedFormats = outArray;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedEvents(PRUint32 *aArrayCount,
                                         PRUint32 **aSupportedEvents)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);

  PRUint32 arrayLen = mSupportedEvents.Length();
  PRUint32* outArray = (PRUint32*)nsMemory::Alloc(arrayLen * sizeof(PRUint32));
  if (!outArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  for (PRUint32 arrayCounter = 0; arrayCounter < arrayLen; arrayCounter++) {
    outArray[arrayCounter] = mSupportedEvents[arrayCounter];
  }

  *aArrayCount = arrayLen;
  *aSupportedEvents = outArray;
  return NS_OK;
}

