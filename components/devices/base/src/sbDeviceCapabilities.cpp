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

#include "sbDeviceCapabilities.h"
#include "sbTArrayStringEnumerator.h"

#include <algorithm>

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>
#include <nsIProgrammingLanguage.h>
#include <nsServiceManagerUtils.h>

#include <sbDeviceUtils.h>
#include <sbMemoryUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDeviceCapabilities,
                              sbIDeviceCapabilities,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbDeviceCapabilities,
                             sbIDeviceCapabilities,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbDeviceCapabilities)
NS_IMPL_THREADSAFE_CI(sbDeviceCapabilities)

#include <prlog.h>
#include <prprf.h>
#include <prtime.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceCapabilities:5
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
isInitialized(false),
isConfigured(false)
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
  // Release all the format types...
  PRInt32 count = mContentFormatTypes.Count();
  for(PRInt32 i = 0; i < count; i++) {
    FormatTypes *formatType =
      static_cast<FormatTypes *>(mContentFormatTypes.ElementAt(i));
    delete formatType;
  }
  mContentFormatTypes.Clear();
}

NS_IMETHODIMP
sbDeviceCapabilities::Init()
{
  NS_ENSURE_TRUE(!isInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv = mContentTypes.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSupportedMimeTypes.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = sbIDeviceCapabilities::CONTENT_UNKNOWN;
       i < sbIDeviceCapabilities::CONTENT_MAX_TYPES;
       i++)
  {
    FormatTypes *newFormatTypes = new FormatTypes;
    if (!newFormatTypes)
      NS_ENSURE_SUCCESS(rv, NS_ERROR_OUT_OF_MEMORY);
    rv = newFormatTypes->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mContentFormatTypes.AppendElement(newFormatTypes) ?
            NS_OK : NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mContentFormatTypes.Compact();

  isInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::ConfigureDone()
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  /* set this so we are not called again */
  isConfigured = true;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::SetFunctionTypes(PRUint32 *aFunctionTypes,
                                       PRUint32 aFunctionTypesCount)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  for (PRUint32 arrayCounter = 0; arrayCounter < aFunctionTypesCount; ++arrayCounter) {
    if (mFunctionTypes.IndexOf(aFunctionTypes[arrayCounter]) ==
        mFunctionTypes.NoIndex) {
      mFunctionTypes.AppendElement(aFunctionTypes[arrayCounter]);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::SetEventTypes(PRUint32 *aEventTypes,
                                    PRUint32 aEventTypesCount)
{
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  for (PRUint32 arrayCounter = 0; arrayCounter < aEventTypesCount; ++arrayCounter) {
    if (mSupportedEvents.IndexOf(aEventTypes[arrayCounter]) ==
        mSupportedEvents.NoIndex) {
      mSupportedEvents.AppendElement(aEventTypes[arrayCounter]);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
sbDeviceCapabilities::AddContentTypes(PRUint32 aFunctionType,
                                      PRUint32 *aContentTypes,
                                      PRUint32 aContentTypesCount)
{
  NS_ENSURE_ARG_POINTER(aContentTypes);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  nsTArray<PRUint32> * nContentTypes = nsnull;
  PRBool const found = mContentTypes.Get(aFunctionType, &nContentTypes);
  if (!found) {
    nContentTypes = new nsTArray<PRUint32>(aContentTypesCount);
  }
  NS_ASSERTION(nContentTypes, "nContentTypes should not be null");
  for (PRUint32 arrayCounter = 0; arrayCounter < aContentTypesCount; ++arrayCounter) {
    if (nContentTypes->IndexOf(aContentTypes[arrayCounter]) ==
        nContentTypes->NoIndex) {
      nContentTypes->AppendElement(aContentTypes[arrayCounter]);
    }
  }

  if (!found) {
    mContentTypes.Put(aFunctionType, nContentTypes);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::AddMimeTypes(PRUint32 aContentType,
                                   const char * *aMimeTypes,
                                   PRUint32 aMimeTypesCount)
{
  NS_ENSURE_ARG_POINTER(aMimeTypes);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  nsTArray<nsCString> * nMimeTypes = nsnull;
  PRBool const found = mSupportedMimeTypes.Get(aContentType, &nMimeTypes);
  if (!found) {
    nMimeTypes = new nsTArray<nsCString>(aMimeTypesCount);
  }
  NS_ASSERTION(nMimeTypes, "nMimeTypes should not be null");
  for (PRUint32 arrayCounter = 0; arrayCounter < aMimeTypesCount; ++arrayCounter) {
    nsCString mimeType(aMimeTypes[arrayCounter]);
    if (nMimeTypes->IndexOf(mimeType) == nMimeTypes->NoIndex) {
      nMimeTypes->AppendElement(aMimeTypes[arrayCounter]);
    }
  }

  if (!found) {
    mSupportedMimeTypes.Put(aContentType, nMimeTypes);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::AddFormatType(PRUint32 aContentType,
                                    nsAString const & aMimeType,
                                    nsISupports * aFormatType)
{
  NS_ENSURE_ARG_POINTER(aFormatType);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceCapabilities::CONTENT_UNKNOWN,
                      sbIDeviceCapabilities::CONTENT_MAX_TYPES - 1);

  FormatTypes *formatType =
    static_cast<FormatTypes *>(mContentFormatTypes.SafeElementAt(aContentType));
  if (!formatType)
    return NS_ERROR_NULL_POINTER;

  nsTArray<nsCOMPtr<nsISupports> > * formatTypes;
  PRBool const found = formatType->Get(aMimeType, &formatTypes);
  if (!found) {
    formatTypes = new nsTArray<nsCOMPtr<nsISupports> >(1);
  }

  formatTypes->AppendElement(aFormatType);

  if (!found) {
    PRBool const added = formatType->Put(aMimeType, formatTypes);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::AddCapabilities(sbIDeviceCapabilities *aCapabilities)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!isConfigured, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;

  PRUint32* functionTypes;
  PRUint32  functionTypesCount;
  rv = aCapabilities->GetSupportedFunctionTypes(&functionTypesCount,
                                                &functionTypes);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemPtr functionTypesPtr(functionTypes);

  rv = SetFunctionTypes(functionTypes, functionTypesCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 functionTypeIndex = 0;
       functionTypeIndex < functionTypesCount;
       ++functionTypeIndex) {
    PRUint32 functionType = functionTypes[functionTypeIndex];

    PRUint32* contentTypes;
    PRUint32  contentTypesCount;
    rv = aCapabilities->GetSupportedContentTypes(functionType,
                                                 &contentTypesCount,
                                                 &contentTypes);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoNSMemPtr contentTypesPtr(contentTypes);

    rv = AddContentTypes(functionType, contentTypes, contentTypesCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 contentTypeIndex = 0;
         contentTypeIndex < contentTypesCount;
         ++contentTypeIndex) {
      PRUint32 contentType = contentTypes[contentTypeIndex];

      char**   mimeTypes;
      PRUint32 mimeTypesCount;
      rv = aCapabilities->GetSupportedMimeTypes(contentType,
                                                &mimeTypesCount,
                                                &mimeTypes);
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);
      sbAutoNSArray<char*> autoMimeTypes(mimeTypes, mimeTypesCount);

      rv = AddMimeTypes(contentType,
                        const_cast<const char**>(mimeTypes),
                        mimeTypesCount);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 mimeTypeIndex = 0;
           mimeTypeIndex < mimeTypesCount;
           ++mimeTypeIndex) {
        nsAutoString mimeType;
        mimeType.AssignLiteral(mimeTypes[mimeTypeIndex]);

        nsISupports** formatTypes;
        PRUint32 formatTypeCount;
        rv = aCapabilities->GetFormatTypes(contentType,
                                           mimeType,
                                           &formatTypeCount,
                                           &formatTypes);
        NS_ENSURE_SUCCESS(rv, rv);
        sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                             formatTypes); 
        for (PRUint32 formatIndex = 0;
             formatIndex < formatTypeCount;
             formatIndex++)
        {
          nsCOMPtr<nsISupports> formatType = formatTypes[formatIndex];
          rv = AddFormatType(contentType, mimeType, formatType);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  PRUint32* supportedEvents;
  PRUint32  supportedEventsCount;
  rv = aCapabilities->GetSupportedEvents(&supportedEventsCount,
                                         &supportedEvents);
  NS_ENSURE_SUCCESS(rv, rv);
  SetEventTypes(supportedEvents, supportedEventsCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedFunctionTypes(PRUint32 *aArrayCount,
                                                PRUint32 **aFunctionTypes)
{
  NS_ENSURE_ARG_POINTER(aArrayCount);
  NS_ENSURE_ARG_POINTER(aFunctionTypes);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(isConfigured, NS_ERROR_NOT_INITIALIZED);

  PRUint32 arrayLen = mFunctionTypes.Length();
  PRUint32* outArray = (PRUint32*)nsMemory::Alloc(arrayLen * sizeof(PRUint32));
  NS_ENSURE_TRUE(outArray, NS_ERROR_OUT_OF_MEMORY);

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
  NS_ENSURE_ARG_POINTER(aArrayCount);
  NS_ENSURE_ARG_POINTER(aContentTypes);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(isConfigured, NS_ERROR_NOT_INITIALIZED);

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
sbDeviceCapabilities::GetSupportedMimeTypes(PRUint32 aContentType,
                                            PRUint32 *aArrayCount,
                                            char ***aSupportedMimeTypes)
{
  NS_ENSURE_ARG_POINTER(aArrayCount);
  NS_ENSURE_ARG_POINTER(aSupportedMimeTypes);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(isConfigured, NS_ERROR_NOT_INITIALIZED);

  nsTArray<nsCString>* supportedMimeTypes;

  if (!mSupportedMimeTypes.Get(aContentType, &supportedMimeTypes)) {
    NS_WARNING("Requseted content type is not available for this device.");
    return NS_ERROR_NOT_AVAILABLE;
  }

  PRUint32 arrayLen = supportedMimeTypes->Length();
  char** outArray = reinterpret_cast<char**>(NS_Alloc(arrayLen * sizeof(char*)));
  if (!outArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRUint32 arrayCounter = 0; arrayCounter < arrayLen; arrayCounter++) {
    nsCString const & mimeType = supportedMimeTypes->ElementAt(arrayCounter);
    outArray[arrayCounter] = ToNewCString(mimeType);
  }

  *aArrayCount = arrayLen;
  *aSupportedMimeTypes = outArray;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedEvents(PRUint32 *aArrayCount,
                                         PRUint32 **aSupportedEvents)
{
  NS_ENSURE_ARG_POINTER(aArrayCount);
  NS_ENSURE_ARG_POINTER(aSupportedEvents);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(isConfigured, NS_ERROR_NOT_INITIALIZED);

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

NS_IMETHODIMP
sbDeviceCapabilities::SupportsContent(PRUint32 aFunctionType,
                                      PRUint32 aContentType,
                                      PRBool *aSupported)
{
  NS_ENSURE_ARG_POINTER(aSupported);
  NS_ENSURE_TRUE(isInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(isConfigured, NS_ERROR_NOT_INITIALIZED);

  *aSupported = PR_FALSE;

  nsresult rv;
  PRUint32 *functionTypes;
  PRUint32 functionTypesLength;
  rv = GetSupportedFunctionTypes(&functionTypesLength,
                                 &functionTypes);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemPtr functionTypesPtr(functionTypes);
  PRUint32 *end = functionTypes + functionTypesLength;
  // function type not available. not supported.
  if (std::find(functionTypes, end, aFunctionType) == end)
    return NS_OK;

  PRUint32 *contentTypes;
  PRUint32 contentTypesLength;
  rv = GetSupportedContentTypes(aFunctionType,
                                &contentTypesLength,
                                &contentTypes);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemPtr contentTypesPtr(contentTypes);
  end = contentTypes + contentTypesLength;
  *aSupported = std::find(contentTypes, end, aContentType) != end;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceCapabilities::GetSupportedFileExtensions(sbIDevice *aDevice,
                                                 PRUint32 aContentType,
                                                 nsIStringEnumerator **_retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  nsresult rv;

  nsTArray<nsString> allExtensions;
  rv = sbDeviceUtils::AddSupportedFileExtensions(aDevice,
                                                 aContentType,
                                                 allExtensions);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringEnumerator> allExtensionsEnum =
    new sbTArrayStringEnumerator(&allExtensions);

  NS_ENSURE_TRUE(allExtensionsEnum, NS_ERROR_OUT_OF_MEMORY);

  allExtensionsEnum.forget(_retval);

  return NS_OK;
}

/**
 * Returns the list of constraints for the format
 */
NS_IMETHODIMP
sbDeviceCapabilities::GetFormatTypes(PRUint32 aContentType,
                                     nsAString const & aMimeType,
                                     PRUint32 *aArrayCount,
                                     nsISupports *** aSupportedFormats)
{
  NS_ENSURE_ARG_POINTER(aArrayCount);
  NS_ENSURE_ARG_POINTER(aSupportedFormats);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceCapabilities::CONTENT_UNKNOWN,
                      sbIDeviceCapabilities::CONTENT_MAX_TYPES - 1);

  FormatTypes *formatType =
    static_cast<FormatTypes *>(mContentFormatTypes.SafeElementAt(aContentType));
  if (!formatType)
    return NS_ERROR_NULL_POINTER;

  nsTArray<nsCOMPtr<nsISupports> > * formats;
  PRBool const found = formatType->Get(aMimeType, &formats);
  PRUint32 count = 0;
  if (found) {
    count = formats->Length();
  }

  nsISupports **formatsArray = static_cast<nsISupports **>(NS_Alloc(
              sizeof(nsISupports *) * count));
  NS_ENSURE_TRUE(formatsArray, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < count; i++) {
    formatsArray[i] = formats->ElementAt(i);
    NS_ADDREF(formatsArray[i]);
  }

  *aArrayCount = count;
  *aSupportedFormats = formatsArray;

  return NS_OK;
}

/*******************************************************************************
 * sbImageSize
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbImageSize, sbIImageSize, nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbImageSize,
                             sbIImageSize,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbImageSize)
NS_IMPL_THREADSAFE_CI(sbImageSize)

sbImageSize::~sbImageSize()
{
  /* destructor code */
}

NS_IMETHODIMP
sbImageSize::Initialize(PRInt32 aWidth,
                        PRInt32 aHeight) {
  mWidth = aWidth;
  mHeight = aHeight;

  return NS_OK;
}

/* readonly attribute long width; */
NS_IMETHODIMP
sbImageSize::GetWidth(PRInt32 *aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  *aWidth = mWidth;
  return NS_OK;
}

/* readonly attribute long height; */
NS_IMETHODIMP
sbImageSize::GetHeight(PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  *aHeight = mHeight;
  return NS_OK;
}

/*******************************************************************************
 * sbDevCapRange
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDevCapRange, sbIDevCapRange, nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbDevCapRange,
                             sbIDevCapRange,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbDevCapRange)
NS_IMPL_THREADSAFE_CI(sbDevCapRange)

sbDevCapRange::~sbDevCapRange()
{
  /* destructor code */
}

NS_IMETHODIMP sbDevCapRange::Initialize(PRInt32 aMin,
                                        PRInt32 aMax,
                                        PRInt32 aStep) {
  mMin = aMin;
  mMax = aMax;
  mStep = aStep;
  mValues.Clear();
  return NS_OK;
}

/* readonly attribute nsIArray values; */
NS_IMETHODIMP
sbDevCapRange::GetValue(PRUint32 aIndex, PRInt32 * aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);

  *aValue = mValues[aIndex];
  return NS_OK;
}

NS_IMETHODIMP
sbDevCapRange::AddValue(PRInt32 aValue)
{
  NS_ENSURE_TRUE(mValues.AppendElement(aValue), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbDevCapRange::GetValueCount(PRUint32 * aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = mValues.Length();
  return NS_OK;
}

/* readonly attribute long min; */
NS_IMETHODIMP
sbDevCapRange::GetMin(PRInt32 *aMin)
{
  NS_ENSURE_ARG_POINTER(aMin);

  *aMin = mMin;
  return NS_OK;
}

/* readonly attribute long max; */
NS_IMETHODIMP
sbDevCapRange::GetMax(PRInt32 *aMax)
{
  NS_ENSURE_ARG_POINTER(aMax);

  *aMax = mMax;
  return NS_OK;
}

/* readonly attribute long step; */
NS_IMETHODIMP
sbDevCapRange::GetStep(PRInt32 *aStep)
{
  NS_ENSURE_ARG_POINTER(aStep);

  *aStep = mStep;
  return NS_OK;
}

NS_IMETHODIMP
sbDevCapRange::IsValueInRange(PRInt32 aValue, PRBool * aInRange) {
  NS_ENSURE_ARG_POINTER(aInRange);

  if (mValues.Length() > 0) {
    *aInRange = mValues.Contains(aValue);
  }
  else {
    *aInRange = aValue <= mMax && aValue >= mMin &&
               (mStep == 0 || ((aValue - mMin) % mStep == 0));
  }
  return NS_OK;
}

/*******************************************************************************
 * sbFormatTypeConstraint
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbFormatTypeConstraint,
                              sbIFormatTypeConstraint,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbFormatTypeConstraint,
                             sbIFormatTypeConstraint,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbFormatTypeConstraint)
NS_IMPL_THREADSAFE_CI(sbFormatTypeConstraint)

sbFormatTypeConstraint::~sbFormatTypeConstraint()
{
  /* destructor code */
}

NS_IMETHODIMP
sbFormatTypeConstraint::Initialize(nsAString const & aConstraintName,
                                   nsIVariant * aMinValue,
                                   nsIVariant * aMaxValue) {
  NS_ENSURE_ARG_POINTER(aMinValue);
  NS_ENSURE_ARG_POINTER(aMaxValue);

  mConstraintName = aConstraintName;
  mMinValue = aMinValue;
  mMaxValue = aMaxValue;
  return NS_OK;
}

/* readonly attribute AString constraintName; */
NS_IMETHODIMP
sbFormatTypeConstraint::GetConstraintName(nsAString & aConstraintName)
{
  aConstraintName = mConstraintName;
  return NS_OK;
}

/* readonly attribute nsIVariant constraintMinValue; */
NS_IMETHODIMP
sbFormatTypeConstraint::GetConstraintMinValue(nsIVariant * *aConstraintMinValue)
{
  NS_ENSURE_ARG_POINTER(aConstraintMinValue);

  *aConstraintMinValue = mMinValue.get();
  NS_IF_ADDREF(*aConstraintMinValue);
  return NS_OK;
}

/* readonly attribute nsIVariant constraintMaxValue; */
NS_IMETHODIMP
sbFormatTypeConstraint::GetConstraintMaxValue(nsIVariant * *aConstraintMaxValue)
{
  NS_ENSURE_ARG_POINTER(aConstraintMaxValue);

  *aConstraintMaxValue = mMaxValue.get();
  NS_IF_ADDREF(*aConstraintMaxValue);
  return NS_OK;
}

/*******************************************************************************
 * Image format type implementation
 */

/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS2(sbImageFormatType,
                              sbIImageFormatType,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbImageFormatType,
                             sbIImageFormatType,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbImageFormatType)
NS_IMPL_THREADSAFE_CI(sbImageFormatType)

sbImageFormatType::~sbImageFormatType()
{
  /* destructor code */
}

NS_IMETHODIMP
sbImageFormatType::Initialize(nsACString const & aImageFormat,
                              nsIArray * aSupportedExplicitSizes,
                              sbIDevCapRange * aSupportedWidths,
                              sbIDevCapRange * aSupportedHeights) {
  NS_ENSURE_ARG_POINTER(aSupportedExplicitSizes);

  mImageFormat = aImageFormat;
  mSupportedExplicitSizes = aSupportedExplicitSizes;
  mSupportedWidths = aSupportedWidths;
  mSupportedHeights = aSupportedHeights;
  return NS_OK;
}

/* readonly attribute ACString imageFormat; */
NS_IMETHODIMP
sbImageFormatType::GetImageFormat(nsACString & aImageFormat)
{
  aImageFormat = mImageFormat;
  return NS_OK;
}

/* readonly attribute nsIArray supportedExplicitSizes; */
NS_IMETHODIMP
sbImageFormatType::GetSupportedExplicitSizes(nsIArray * *aSupportedExplicitSizes)
{
  NS_ENSURE_ARG_POINTER(aSupportedExplicitSizes);

  *aSupportedExplicitSizes = mSupportedExplicitSizes;
  NS_IF_ADDREF(*aSupportedExplicitSizes);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedWidths; */
NS_IMETHODIMP
sbImageFormatType::GetSupportedWidths(sbIDevCapRange * *aSupportedWidths)
{
  NS_ENSURE_ARG_POINTER(aSupportedWidths);

  *aSupportedWidths = mSupportedWidths;
  NS_IF_ADDREF(*aSupportedWidths);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedHeights; */
NS_IMETHODIMP
sbImageFormatType::GetSupportedHeights(sbIDevCapRange * *aSupportedHeights)
{
  NS_ENSURE_ARG_POINTER(aSupportedHeights);

  *aSupportedHeights = mSupportedHeights;
  NS_IF_ADDREF(*aSupportedHeights);
  return NS_OK;
}

/*******************************************************************************
 * Audio format type
 */

/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS2(sbAudioFormatType,
                              sbIAudioFormatType,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbAudioFormatType,
                             sbIAudioFormatType,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbAudioFormatType)
NS_IMPL_THREADSAFE_CI(sbAudioFormatType)

sbAudioFormatType::~sbAudioFormatType()
{
  /* destructor code */
}

NS_IMETHODIMP
sbAudioFormatType::Initialize(nsACString const & aContainerFormat,
                              nsACString const & aAudioCodec,
                              sbIDevCapRange * aSupportedBitrates,
                              sbIDevCapRange * aSupportedSampleRates,
                              sbIDevCapRange * aSupportedChannels,
                              nsIArray * aFormatSpecificConstraints) {
  mContainerFormat = aContainerFormat;
  mAudioCodec = aAudioCodec;
  mSupportedBitrates = aSupportedBitrates;
  mSupportedSampleRates = aSupportedSampleRates;
  mSupportedChannels = aSupportedChannels;
  mFormatSpecificConstraints = aFormatSpecificConstraints;

  return NS_OK;
}

/* readonly attribute ACString containerFormat; */
NS_IMETHODIMP
sbAudioFormatType::GetContainerFormat(nsACString & aContainerFormat)
{
  aContainerFormat = mContainerFormat;
  return NS_OK;
}

/* readonly attribute ACString audioCodec; */
NS_IMETHODIMP
sbAudioFormatType::GetAudioCodec(nsACString & aAudioCodec)
{
  aAudioCodec = mAudioCodec;
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedBitrates; */
NS_IMETHODIMP
sbAudioFormatType::GetSupportedBitrates(sbIDevCapRange * *aSupportedBitrates)
{
  NS_ENSURE_ARG_POINTER(aSupportedBitrates);
  *aSupportedBitrates = mSupportedBitrates;
  NS_IF_ADDREF(*aSupportedBitrates);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedSampleRates; */
NS_IMETHODIMP
sbAudioFormatType::GetSupportedSampleRates(sbIDevCapRange * *aSupportedSampleRates)
{
  NS_ENSURE_ARG_POINTER(aSupportedSampleRates);
  *aSupportedSampleRates = mSupportedSampleRates;
  NS_IF_ADDREF(*aSupportedSampleRates);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedChannels; */
NS_IMETHODIMP
sbAudioFormatType::GetSupportedChannels(sbIDevCapRange * *aSupportedChannels)
{
  NS_ENSURE_ARG_POINTER(aSupportedChannels);
  *aSupportedChannels = mSupportedChannels;
  NS_IF_ADDREF(*aSupportedChannels);
  return NS_OK;
}

/* readonly attribute nsIArray formatSpecificConstraints; */
NS_IMETHODIMP
sbAudioFormatType::GetFormatSpecificConstraints(nsIArray * *aFormatSpecificConstraints)
{
  NS_ENSURE_ARG_POINTER(aFormatSpecificConstraints);
  *aFormatSpecificConstraints = mFormatSpecificConstraints;
  NS_IF_ADDREF(*aFormatSpecificConstraints);
  return NS_OK;
}

/*******************************************************************************
 * Video format video stream
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDevCapVideoStream,
                              sbIDevCapVideoStream,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbDevCapVideoStream,
                             sbIDevCapVideoStream,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbDevCapVideoStream)
NS_IMPL_THREADSAFE_CI(sbDevCapVideoStream)

sbDevCapVideoStream::sbDevCapVideoStream()
{
}

sbDevCapVideoStream::~sbDevCapVideoStream()
{
}

static nsresult
CopyAndParseToFractions(const char ** aStrings,
                        PRUint32 aStringCount,
                        nsTArray<sbFraction> & aFractions)
{
  nsresult rv;

  for (PRUint32 index = 0; index < aStringCount; ++index)
  {
    sbFraction fraction;
    rv = sbFractionFromString(NS_ConvertASCIItoUTF16(aStrings[index]),
                              fraction);
    if (NS_SUCCEEDED(rv)) {
      aFractions.AppendElement(fraction);
    }
    else {
      nsCString msg("CopyAndParseToFractions: Unable to parse fraction ");
      msg.Append(aStrings[index]);
      NS_WARNING(msg.BeginReading());
    }
  }
  return NS_OK;
}

static nsresult
CopyAndParseFromFractions(const nsTArray<sbFraction> & aFractions,
                          PRUint32 aStringCount,
                          char ** &aStrings)
{
  aStrings = reinterpret_cast<char**>(NS_Alloc(aStringCount * sizeof(char*)));
  NS_ENSURE_TRUE(aStrings, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 index = 0; index < aStringCount; ++index)
  {
    nsString string = sbFractionToString(aFractions[index]);
    aStrings[index] = ToNewCString(NS_ConvertUTF16toUTF8(string));
    if (!aStrings[index]) {
      // allocation failure
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(index, aStrings);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}

/* void initialize (in ACString aType, in nsIArray aExplicitSizes, in sbIDevCapRange aWidths, in sbIDevCapRange aHeights, in unsigned long aVideoParsCount, [array, size_is (aVideoParsCount)] in string aVideoPars, in unsigned long aFrameRateCount, [array, size_is (aFrameRateCount)] in string aFrameRates, in sbIDevCapRange aBitRates); */
NS_IMETHODIMP sbDevCapVideoStream::Initialize(const nsACString & aType,
                                              nsIArray *aExplicitSizes,
                                              sbIDevCapRange *aWidths,
                                              sbIDevCapRange *aHeights,
                                              PRUint32 aVideoPARCount,
                                              const char **aVideoPARs,
                                              PRUint32 aFrameRateCount,
                                              const char **aFrameRates,
                                              sbIDevCapRange *aBitRates)
{
  mType = aType;
  mExplicitSizes = aExplicitSizes;
  mWidths = aWidths;
  mHeights = aHeights;
  CopyAndParseToFractions(aVideoPARs, aVideoPARCount, mVideoPARs);
  CopyAndParseToFractions(aFrameRates, aFrameRateCount, mFrameRates);
  mBitRates = aBitRates;

  return NS_OK;
}

/* readonly attribute ACString type; */
NS_IMETHODIMP sbDevCapVideoStream::GetType(nsACString & aType)
{
  aType = mType;
  return NS_OK;
}

/* readonly attribute nsIArray supportedExplicitSizes; */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedExplicitSizes(nsIArray * *aSupportedExplicitSizes)
{
  NS_IF_ADDREF(*aSupportedExplicitSizes = mExplicitSizes);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedWidths; */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedWidths(sbIDevCapRange * *aSupportedWidths)
{
  NS_IF_ADDREF(*aSupportedWidths = mWidths);
  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedHeights; */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedHeights(sbIDevCapRange * *aSupportedHeights)
{
  NS_IF_ADDREF(*aSupportedHeights = mHeights);
  return NS_OK;
}

/* void getSupportedVideoPARs (out unsigned long aCount, [array, size_is (aCount)] out string aVideoPARs); */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedVideoPARs(PRUint32 *aCount, char ***aVideoPARs)
{
  *aCount = mVideoPARs.Length();

  nsresult rv = CopyAndParseFromFractions(mVideoPARs, *aCount, *aVideoPARs);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void getSupportedFrameRates (out unsigned long aCount, [array, size_is (aCount)] out string aFrameRates); */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedFrameRates(PRUint32 *aCount, char ***aFrameRates)
{
  *aCount = mFrameRates.Length();

  nsresult rv = CopyAndParseFromFractions(mFrameRates, *aCount, *aFrameRates);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* readonly attribute sbIDevCapRange supportedBitRates; */
NS_IMETHODIMP sbDevCapVideoStream::GetSupportedBitRates(sbIDevCapRange * *aSupportedBitRates)
{
  NS_IF_ADDREF(*aSupportedBitRates = mBitRates);
  return NS_OK;
}

/*******************************************************************************
 * Video format audio stream
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDevCapAudioStream,
                              sbIDevCapAudioStream,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbDevCapAudioStream,
                             sbIDevCapAudioStream,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbDevCapAudioStream)
NS_IMPL_THREADSAFE_CI(sbDevCapAudioStream)

sbDevCapAudioStream::sbDevCapAudioStream()
{
}

sbDevCapAudioStream::~sbDevCapAudioStream()
{
}

NS_IMETHODIMP
sbDevCapAudioStream::Initialize(const nsACString & aType,
                                sbIDevCapRange *aBitRates,
                                sbIDevCapRange *aSampleRates,
                                sbIDevCapRange *aChannels)
{
  mType = aType;
  mBitRates = aBitRates;
  mSampleRates = aSampleRates;
  mChannels = aChannels;

  return NS_OK;
}

NS_IMETHODIMP sbDevCapAudioStream::GetType(nsACString & aType)
{
  aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
sbDevCapAudioStream::GetSupportedBitRates(sbIDevCapRange * *aBitRates)
{
  return sbReturnCOMPtr(mBitRates, aBitRates);
}

/* readonly attribute sbIDevCapRange supportedSampleRates; */
NS_IMETHODIMP
sbDevCapAudioStream::GetSupportedSampleRates(sbIDevCapRange * *aSampleRates)
{
  return sbReturnCOMPtr(mSampleRates, aSampleRates);
}

/* readonly attribute sbIDevCapRange supportedChannels; */
NS_IMETHODIMP
sbDevCapAudioStream::GetSupportedChannels(sbIDevCapRange * *aChannels)
{
  return sbReturnCOMPtr(mChannels, aChannels);
}

/*******************************************************************************
 * Video format type
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbVideoFormatType,
                              sbIVideoFormatType,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbVideoFormatType,
                             sbIVideoFormatType,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbVideoFormatType)
NS_IMPL_THREADSAFE_CI(sbVideoFormatType)

sbVideoFormatType::sbVideoFormatType()
{
  /* member initializers and constructor code */
}

sbVideoFormatType::~sbVideoFormatType()
{
  /* destructor code */
}

NS_IMETHODIMP
sbVideoFormatType::Initialize(const nsACString & aContainerType,
                                    sbIDevCapVideoStream *aVideoStream,
                                    sbIDevCapAudioStream *aAudioStream)
{
  mContainerType = aContainerType;
  mVideoStream = aVideoStream;
  mAudioStream = aAudioStream;

  return NS_OK;
}

NS_IMETHODIMP
sbVideoFormatType::GetContainerType(nsACString & aContainerType)
{
  aContainerType = mContainerType;
  return NS_OK;
}

NS_IMETHODIMP
sbVideoFormatType::GetVideoStream(sbIDevCapVideoStream * *aVideoStream)
{
  return sbReturnCOMPtr(mVideoStream, aVideoStream);
}

NS_IMETHODIMP
sbVideoFormatType::GetAudioStream(sbIDevCapAudioStream * *aAudioStream)
{
  return sbReturnCOMPtr(mAudioStream, aAudioStream);
}
