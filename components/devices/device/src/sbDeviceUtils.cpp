/* vim: set sw=2 :miv */
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

#include "sbDeviceUtils.h"

#include <algorithm>

#include <nsAlgorithm.h>
#include <nsArrayUtils.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCRT.h>
#include <nsComponentManagerUtils.h>
#include <nsIDOMWindow.h>
#include <nsIDialogParamBlock.h>
#include <nsIMutableArray.h>
#include <nsIFile.h>
#include <nsISimpleEnumerator.h>
#include <nsIStandardURL.h>
#include <nsISupportsArray.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIWindowWatcher.h>
#include <nsNetCID.h>
#include <nsThreadUtils.h>

#include "sbBaseDevice.h"
#include <sbDeviceCapsCompatibility.h>
#include "sbIDeviceCapabilities.h"
#include "sbIDeviceContent.h"
#include "sbIDeviceErrorMonitor.h"
#include "sbIDeviceHelper.h"
#include "sbIDeviceLibrary.h"
#include "sbIDeviceLibraryMediaSyncSettings.h"
#include "sbIDeviceLibrarySyncSettings.h"
#include "sbIDeviceRegistrar.h"
#include "sbIMediaItem.h"
#include <sbIMediaItemDownloader.h>
#include <sbIMediaItemDownloadService.h>
#include "sbIMediaList.h"
#include "sbIMediaListListener.h"
#include <sbITranscodeProfile.h>
#include <sbITranscodeManager.h>
#include <sbIPrompter.h>
#include "sbIWindowWatcher.h"
#include "sbLibraryUtils.h"
#include <sbPrefBranch.h>
#include <sbProxiedComponentManager.h>
#include "sbStandardProperties.h"
#include "sbStringUtils.h"
#include <sbVariantUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbMemoryUtils.h>
#include <sbArray.h>
#include <sbIMediaInspector.h>

#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceUtilsLog = NULL;
#define LOG(args) \
  PR_BEGIN_MACRO \
  if (!gDeviceUtilsLog) \
    gDeviceUtilsLog = PR_NewLogModule("sbDeviceUtils"); \
  PR_LOG(gDeviceUtilsLog, PR_LOG_WARN, args); \
  PR_END_MACRO
#define TRACE(args) \
  PR_BEGIN_MACRO \
  if (!gDeviceUtilsLog) \
    gDeviceUtilsLog = PR_NewLogModule("sbDeviceUtils"); \
  PR_LOG(gDeviceUtilsLog, PR_LOG_DEBUG, args); \
  PR_END_MACRO
#else
#define LOG(args) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define TRACE(args) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#endif

class sbDeviceUtilsQueryUserSpaceExceeded : public sbICallWithWindowCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICALLWITHWINDOWCALLBACK

  nsresult Query(sbIDevice*        aDevice,
                 sbIDeviceLibrary* aLibrary,
                 PRInt64           aSpaceNeeded,
                 PRInt64           aSpaceAvailable,
                 PRBool*           aAbort);

private:
  nsCOMPtr<sbIDevice>        mDevice;
  nsCOMPtr<sbIDeviceLibrary> mLibrary;
  PRBool                     mSync;
  PRInt64                    mSpaceNeeded;
  PRInt64                    mSpaceAvailable;
  PRBool*                    mAbort;
};

/*static*/
nsresult sbDeviceUtils::GetOrganizedPath(/* in */ nsIFile *aParent,
                                         /* in */ sbIMediaItem *aItem,
                                         nsIFile **_retval)
{
  NS_ENSURE_ARG_POINTER(aParent);
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsString kIllegalChars = NS_ConvertASCIItoUTF16(FILE_ILLEGAL_CHARACTERS);
  kIllegalChars.AppendLiteral(FILE_PATH_SEPARATOR);

  nsCOMPtr<nsIFile> file;
  rv = aParent->Clone(getter_AddRefs(file));

  nsString propValue;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                          propValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!propValue.IsEmpty()) {
    nsString_ReplaceChar(propValue, kIllegalChars, PRUnichar('_'));
    rv = file->Append(propValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                          propValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!propValue.IsEmpty()) {
    nsString_ReplaceChar(propValue, kIllegalChars, PRUnichar('_'));
    rv = file->Append(propValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIURI> itemUri;
  rv = aItem->GetContentSrc(getter_AddRefs(itemUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> itemUrl = do_QueryInterface(itemUri, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString fileCName;
  rv = itemUrl->GetFileName(fileCName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString fileName = NS_ConvertUTF8toUTF16(fileCName);
  nsString_ReplaceChar(fileName, kIllegalChars, PRUnichar('_'));
  rv = file->Append(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  file.swap(*_retval);

  return NS_OK;
}

/**
 * \class Device Utilities: Media List enumeration listener that
 *        marks all items in a library with a property
 */
class sbDeviceUtilsBulkSetPropertyEnumerationListener :
  public sbIMediaListEnumerationListener
{
public:
  sbDeviceUtilsBulkSetPropertyEnumerationListener(const nsAString& aId,
                                                  const nsAString& aValue,
                                                  PRInt32 *aAbortFlag = nsnull)
  : mId(aId),
    mValue(aValue),
    mAbortFlag(aAbortFlag)
  {}

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnEnumerationBegin(sbIMediaList *aMediaList, PRUint16 *_retval)
  {
    NS_ENSURE_ARG_POINTER(_retval);

    PRBool abortRequests = PR_FALSE;
    if (mAbortFlag)
      abortRequests = PR_AtomicAdd(mAbortFlag, 0);

    if (abortRequests) {
      *_retval = sbIMediaListEnumerationListener::CANCEL;
      return NS_OK;
    } else {
      *_retval = sbIMediaListEnumerationListener::CONTINUE;
      return NS_OK;
    }
  }

  NS_IMETHODIMP OnEnumeratedItem(sbIMediaList *aMediaList, sbIMediaItem *aItem,
                                 PRUint16 *_retval)
  {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_ARG_POINTER(_retval);

    PRBool abortRequests = PR_FALSE;
    if (mAbortFlag)
      abortRequests = PR_AtomicAdd(mAbortFlag, 0);

    if (abortRequests) {
      *_retval = sbIMediaListEnumerationListener::CANCEL;
      return NS_OK;
    }

    nsresult rv = aItem->SetProperty(mId, mValue);
    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return NS_OK;
  }

  NS_IMETHODIMP OnEnumerationEnd(sbIMediaList *aMediaList, nsresult aStatusCode)
  {
    return NS_OK;
  }

protected:
  nsString mId;
  nsString mValue;
  PRBool *mAbortFlag;
  virtual ~sbDeviceUtilsBulkSetPropertyEnumerationListener() {}
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtilsBulkSetPropertyEnumerationListener,
                              sbIMediaListEnumerationListener);

/*static*/
nsresult sbDeviceUtils::BulkSetProperty(sbIMediaList *aMediaList,
                                        const nsAString& aPropertyId,
                                        const nsAString& aPropertyValue,
                                        sbIPropertyArray* aPropertyFilter,
                                        PRInt32 *aAbortFlag)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsRefPtr<sbDeviceUtilsBulkSetPropertyEnumerationListener> listener =
    new sbDeviceUtilsBulkSetPropertyEnumerationListener(aPropertyId,
                                                        aPropertyValue,
                                                        aAbortFlag);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  if (!aPropertyFilter) {
    // set all items
    return aMediaList->EnumerateAllItems(listener,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  }

  // set some filtered set of items
  return aMediaList->EnumerateItemsByProperties(aPropertyFilter,
                                                listener,
                                                sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
}

/*static*/
nsresult sbDeviceUtils::DeleteByProperty(sbIMediaList *aMediaList,
                                         nsAString const & aProperty,
                                         nsAString const & aValue)
{
  nsresult rv;

  NS_ASSERTION(aMediaList, "Attempting to delete null media list");

  nsCOMPtr<nsIArray> array;
  rv = aMediaList->GetItemsByProperty(aProperty,
                                      aValue,
                                      getter_AddRefs(array));
  if (NS_SUCCEEDED(rv)) {

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = array->Enumerate(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(rv, rv);

    return aMediaList->RemoveSome(enumerator);
  }
  // No items is not an error
  else if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_OK;
  }
  // Return failure of GetItemsByProperty
  return rv;
}

/*static*/
nsresult sbDeviceUtils::DeleteUnavailableItems(sbIMediaList *aMediaList)
{
  return sbDeviceUtils::DeleteByProperty(aMediaList,
                                         NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                                         NS_LITERAL_STRING("0"));
}

/* static */
nsresult sbDeviceUtils::GetDeviceLibraryForItem(sbIDevice* aDevice,
                                                sbIMediaItem* aItem,
                                                sbIDeviceLibrary** _retval)
{
  NS_ASSERTION(aDevice, "Getting device library with no device");
  NS_ASSERTION(aItem, "Getting device library for nothing");
  NS_ASSERTION(_retval, "null retval");

  nsresult rv;

  nsCOMPtr<sbILibrary> ownerLibrary;
  rv = aItem->GetLibrary(getter_AddRefs(ownerLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDeviceLibraryForLibrary(aDevice, ownerLibrary, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::NewDeviceLibraryURI(sbIDeviceLibrary* aDeviceLibrary,
                                            const nsCString&  aSpec,
                                            nsIURI**          aURI)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);
  NS_ENSURE_ARG_POINTER(aURI);

  // Function variables.
  nsresult rv;

  // Get the device.
  nsCOMPtr<sbIDevice> device;
  rv = aDeviceLibrary->GetDevice(getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device ID.
  char               deviceIDString[NSID_LENGTH];
  sbAutoMemPtr<nsID> deviceID;
  rv = device->GetId(deviceID.StartAssignment());
  NS_ENSURE_SUCCESS(rv, rv);
  deviceID->ToProvidedString(deviceIDString);

  // Get the device library GUID.
  nsAutoString guid;
  rv = aDeviceLibrary->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the base URI spec.
  nsCAutoString baseURISpec;
  baseURISpec.Assign("x-device:///");
  baseURISpec.Append(deviceIDString);
  baseURISpec.Append("/");
  baseURISpec.Append(NS_ConvertUTF16toUTF8(guid));
  baseURISpec.Append("/");

  // Produce the base URI.
  nsCOMPtr<nsIStandardURL>
    baseStandardURL = do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = baseStandardURL->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY,
                             -1,
                             baseURISpec,
                             nsnull,
                             nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> baseURI = do_QueryInterface(baseStandardURL, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the requested URI.
  nsCOMPtr<nsIStandardURL>
    standardURL = do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = standardURL->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY,
                         -1,
                         aSpec,
                         nsnull,
                         baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  rv = CallQueryInterface(standardURL, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::GetDeviceLibraryForLibrary(sbIDevice* aDevice,
                                                   sbILibrary* aLibrary,
                                                   sbIDeviceLibrary** _retval)
{
  NS_ASSERTION(aDevice, "Getting device library with no device");
  NS_ASSERTION(aLibrary, "Getting device library for nothing");
  NS_ASSERTION(_retval, "null retval");

  nsresult rv;

  // mediaItem.library is not a sbIDeviceLibrary, test GUID :(
  nsCOMPtr<sbIDeviceContent> content;
  rv = aDevice->GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> libraries;
  rv = content->GetLibraries(getter_AddRefs(libraries));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryCount;
  rv = libraries->GetLength(&libraryCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < libraryCount; ++index) {
    nsCOMPtr<sbIDeviceLibrary> deviceLib =
      do_QueryElementAt(libraries, index, &rv);
    if (NS_FAILED(rv))
      continue;

    PRBool equalsLibrary;
    rv = aLibrary->Equals(deviceLib, &equalsLibrary);
    NS_ENSURE_SUCCESS(rv, rv);

    if (equalsLibrary) {
      deviceLib.forget(_retval);
      return NS_OK;
    }
  }

  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

/* static */
nsresult sbDeviceUtils::GetMediaItemByDevicePersistentId
                          (sbILibrary*      aLibrary,
                           const nsAString& aDevicePersistentId,
                           sbIMediaItem**   aItem)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  // get the library items with the device persistent ID
  nsCOMPtr<nsIArray> mediaItemList;
  rv = aLibrary->GetItemsByProperty
                   (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                    aDevicePersistentId,
                    getter_AddRefs(mediaItemList));
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_ERROR_NOT_AVAILABLE;
  NS_ENSURE_SUCCESS(rv, rv);

  // Return the first item with an exactly matching persistent ID.
  // GetItemsByProperty does not do an exact match (e.g., case insensitive,
  // number strings limited to 64-bit float precision; see bug 15640).
  PRUint32 length;
  rv = mediaItemList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < length; i++) {
    // get the next media item
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = mediaItemList->QueryElementAt(i,
                                       NS_GET_IID(sbIMediaItem),
                                       getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // check for an exact match
    nsAutoString devicePersistentId;
    rv = mediaItem->GetProperty
                      (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                       devicePersistentId);
    NS_ENSURE_SUCCESS(rv, rv);
    if (aDevicePersistentId.Equals(devicePersistentId)) {
      mediaItem.forget(aItem);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/* static */
nsresult sbDeviceUtils::GetOriginMediaItemByDevicePersistentId
                          (sbILibrary*      aLibrary,
                           const nsAString& aDevicePersistentId,
                           sbIMediaItem**   aItem)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  // get the device media item from the device persistent ID
  nsCOMPtr<sbIMediaItem> deviceMediaItem;
  rv = GetMediaItemByDevicePersistentId(aLibrary,
                                        aDevicePersistentId,
                                        getter_AddRefs(deviceMediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the original item from the device media item
  rv = sbLibraryUtils::GetOriginItem(deviceMediaItem, aItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::GetDeviceWriteLength(sbIDeviceLibrary* aDeviceLibrary,
                                             sbIMediaItem*     aMediaItem,
                                             PRUint64*         aWriteLength)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aWriteLength);

  // Function variables.
  nsresult rv;

  // Get the download service to check if the media item needs to be downloaded.
  // Even if the media item has a local content source file, a version that's
  // compatible with the device may still need to be downloaded (e.g., for DRM
  // purposes or for a compatible format).
  nsCOMPtr<sbIMediaItemDownloadService> downloadService =
    do_GetService("@songbirdnest.com/Songbird/MediaItemDownloadService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a downloader for the media item and target device library.
  nsCOMPtr<sbIMediaItemDownloader> downloader;
  rv = downloadService->GetDownloader(aMediaItem,
                                      aDeviceLibrary,
                                      getter_AddRefs(downloader));
  NS_ENSURE_SUCCESS(rv, rv);

  // If a downloader was returned, the media item needs to be downloaded.  Get
  // the download size.
  if (downloader) {
    rv = downloader->GetDownloadSize(aMediaItem,
                                     aDeviceLibrary,
                                     aWriteLength);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  // Try getting the content length directly from the media item.
  PRInt64 contentLength;
  rv = sbLibraryUtils::GetContentLength(aMediaItem, &contentLength);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(contentLength >= 0, NS_ERROR_FAILURE);

  // Return results.
  *aWriteLength = static_cast<PRUint64>(contentLength);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::QueryUserSpaceExceeded
                          (/* in */  sbIDevice*        aDevice,
                           /* in */  sbIDeviceLibrary* aLibrary,
                           /* in */  PRInt64           aSpaceNeeded,
                           /* in */  PRInt64           aSpaceAvailable,
                           /* out */ PRBool*           aAbort)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aAbort);

  nsresult rv;

  // create a query object and query the user
  nsRefPtr<sbDeviceUtilsQueryUserSpaceExceeded> query;
  query = new sbDeviceUtilsQueryUserSpaceExceeded();
  NS_ENSURE_TRUE(query, NS_ERROR_OUT_OF_MEMORY);
  rv = query->Query(aDevice, aLibrary, aSpaceNeeded, aSpaceAvailable, aAbort);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::QueryUserAbortRip(PRBool* aAbort)
{
  NS_ENSURE_ARG_POINTER(aAbort);

  nsresult rv;

  // By default assume the user says yes.
  *aAbort = PR_TRUE;

  // Get a prompter that does not wait for a window.
  nsCOMPtr<sbIPrompter> prompter =
                          do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prompter->SetWaitForWindow(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the prompt title.
  nsAString const& title =
    SBLocalizedString("device.dialog.cddevice.stopripping.title");

  // Get the prompt message.
  nsAString const& message =
    SBLocalizedString("device.dialog.cddevice.stopripping.msg");

  // Configure the buttons.
  PRUint32 buttonFlags = nsIPromptService::STD_YES_NO_BUTTONS;

  // Query the user if they wish to cancel the rip or not.
  PRInt32 buttonPressed;
  rv = prompter->ConfirmEx(nsnull,
                           title.BeginReading(),
                           message.BeginReading(),
                           buttonFlags,
                           nsnull,                      // "Yes" Button
                           nsnull,                      // "No" Button
                           nsnull,                      // No button 2.
                           nsnull,                      // No check message.
                           nsnull,                      // No check result.
                           &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);

  // Yes = 0, No = 1
  *aAbort = (buttonPressed == 0);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::QueryUserViewErrors(sbIDevice* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  nsresult rv;

  nsCOMPtr<sbIDeviceErrorMonitor> errMonitor =
      do_GetService("@songbirdnest.com/device/error-monitor-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasErrors;
  rv = errMonitor->DeviceHasErrors(aDevice, EmptyString(), 0, &hasErrors);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasErrors) {
    // Query the user if they wish to see the errors

    // Get a prompter that does not wait for a window.
    nsCOMPtr<sbIPrompter> prompter =
                            do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = prompter->SetWaitForWindow(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the prompt title.
    nsAString const& title =
      SBLocalizedString("device.dialog.cddevice.viewerrors.title");

    // Get the prompt message.
    nsAString const& message =
      SBLocalizedString("device.dialog.cddevice.viewerrors.msg");

    // Configure the buttons.
    PRUint32 buttonFlags = nsIPromptService::STD_YES_NO_BUTTONS;

    // Query the user if they wish to see the errors
    PRInt32 buttonPressed;
    rv = prompter->ConfirmEx(nsnull,
                             title.BeginReading(),
                             message.BeginReading(),
                             buttonFlags,
                             nsnull,                      // "Yes" Button
                             nsnull,                      // "No" Button
                             nsnull,                      // No button 2.
                             nsnull,                      // No check message.
                             nsnull,                      // No check result.
                             &buttonPressed);
    NS_ENSURE_SUCCESS(rv, rv);

    if (buttonPressed == 0) {
      ShowDeviceErrors(aDevice);
    }
  }

  return NS_OK;
}


/* static */
nsresult sbDeviceUtils::ShowDeviceErrors(sbIDevice* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  // We have to send an comglmeration of stuff into the OpenWindow to get
  // all the parameters the dialog needs to display the errors.
  //  nsISupports = nsIDialogParamBlock =
  //    String[0] = "" Bank string for options.
  //    String[1] = "Ripping" Localized string for operation message.
  //    Objects = nsIMutableArray =
  //      [0] = sbIDevice
  //      [1] = nsIArray of nsISupportStrings for error messages.

  // Start with the DialogParamBlock and add strings/objects to it
  nsCOMPtr<nsIDialogParamBlock> dialogBlock =
    do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // First add strings
  // Options string
  rv = dialogBlock->SetString(0, NS_LITERAL_STRING("").get());
  NS_ENSURE_SUCCESS(rv, rv);
  // Operation string
  rv = dialogBlock->SetString(1, NS_LITERAL_STRING("ripping").get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Now add Objects (nsIMutableArray)
  nsCOMPtr<nsIMutableArray> objects =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  // Append device
  rv = objects->AppendElement(aDevice, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  // Append error strings
  nsCOMPtr<sbIDeviceErrorMonitor> errMonitor =
      do_GetService("@songbirdnest.com/device/error-monitor-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIArray> errorStrings;
  rv = errMonitor->GetDeviceErrors(aDevice,
                                   EmptyString(),
                                   0,
                                   getter_AddRefs(errorStrings));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = objects->AppendElement(errorStrings, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  // Append the objects to the dialogBlock
  rv = dialogBlock->SetObjects(objects);
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert the DialogParamBlock to an nsISuports
  nsCOMPtr<nsISupports> arguments = do_QueryInterface(dialogBlock, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a prompter that does not wait for a window.
  nsCOMPtr<sbIPrompter> prompter =
                          do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prompter->SetWaitForWindow(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Display the dialog.
  nsCOMPtr<nsIDOMWindow> dialogWindow;
  rv = prompter->OpenDialog(nsnull, // Default to parent window
                            NS_LITERAL_STRING("chrome://songbird/content/xul/device/deviceErrorDialog.xul"),
                            NS_LITERAL_STRING("device_error_dialog"),
                            NS_LITERAL_STRING("chrome,centerscreen,model=yes,titlebar=no"),
                            arguments,
                            getter_AddRefs(dialogWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::SetLinkedSyncPartner(sbIDevice* aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  nsresult rv;

  // Get the device sync partner ID and determine if the device is linked to a
  // sync partner.
  PRBool               deviceIsLinked;
  nsCOMPtr<nsIVariant> deviceSyncPartnerIDVariant;
  nsAutoString         deviceSyncPartnerID;
  rv = aDevice->GetPreference(NS_LITERAL_STRING("SyncPartner"),
                              getter_AddRefs(deviceSyncPartnerIDVariant));
  if (NS_SUCCEEDED(rv)) {
    rv = deviceSyncPartnerIDVariant->GetAsAString(deviceSyncPartnerID);
    NS_ENSURE_SUCCESS(rv, rv);
    deviceIsLinked = PR_TRUE;
  } else {
    deviceIsLinked = PR_FALSE;
  }

  // Get the local sync partner ID.
  nsAutoString localSyncPartnerID;
  rv = GetMainLibraryId(localSyncPartnerID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if device is linked to local sync partner.
  PRBool isLinkedLocally = PR_FALSE;
  if (deviceIsLinked)
    isLinkedLocally = deviceSyncPartnerID.Equals(localSyncPartnerID);

  // If device is not linked locally, set its sync partner.
  if (!isLinkedLocally) {
    rv = aDevice->SetPreference(NS_LITERAL_STRING("SyncPartner"),
                                sbNewVariant(localSyncPartnerID));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

SB_AUTO_CLASS(sbAutoNSMemoryPtr, void*, !!mValue, NS_Free(mValue), mValue = nsnull);

bool sbDeviceUtils::ArePlaylistsSupported(sbIDevice * aDevice)
{
  nsCOMPtr<sbIDeviceCapabilities> capabilities;
  nsresult rv = aDevice->GetCapabilities(getter_AddRefs(capabilities));
  NS_ENSURE_SUCCESS(rv, false);

  bool supported = false;
  PRUint32 * functionTypes;
  PRUint32 functionTypesLength;
  rv = capabilities->GetSupportedFunctionTypes(&functionTypesLength,
                                               &functionTypes);
  NS_ENSURE_SUCCESS(rv, false);
  sbAutoNSMemoryPtr functionTypesPtr(functionTypes);
  for (PRUint32 functionType = 0;
       !supported && functionType < functionTypesLength;
       ++functionType) {
    PRUint32 * contentTypes;
    PRUint32 contentTypesLength;
    rv = capabilities->GetSupportedContentTypes(functionTypes[functionType],
                                                &contentTypesLength,
                                                &contentTypes);
    NS_ENSURE_SUCCESS(rv, false);
    sbAutoNSMemoryPtr contentTypesPtr(contentTypes);
    PRUint32 * const end = contentTypes + contentTypesLength;
    PRUint32 const CONTENT_PLAYLIST =
      static_cast<PRUint32>(sbIDeviceCapabilities::CONTENT_PLAYLIST);
    supported = std::find(contentTypes,
                          end,
                          CONTENT_PLAYLIST) != end;
  }
  return supported;
}

//------------------------------------------------------------------------------
//
// sbDeviceUtilsQueryUserSpaceExceeded class.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtilsQueryUserSpaceExceeded,
                              sbICallWithWindowCallback)

NS_IMETHODIMP
sbDeviceUtilsQueryUserSpaceExceeded::HandleWindowCallback(nsIDOMWindow* aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  nsresult rv;

  // get the device helper
  nsCOMPtr<sbIDeviceHelper> deviceHelper =
    do_GetService("@songbirdnest.com/Songbird/Device/Base/Helper;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // query the user
  PRBool proceed;
  rv = deviceHelper->QueryUserSpaceExceeded(aWindow,
                                            mDevice,
                                            mLibrary,
                                            mSpaceNeeded,
                                            mSpaceAvailable,
                                            &proceed);
  NS_ENSURE_SUCCESS(rv, rv);

  // return results
  *mAbort = !proceed;

  return NS_OK;
}

nsresult
sbDeviceUtilsQueryUserSpaceExceeded::Query(sbIDevice*        aDevice,
                                           sbIDeviceLibrary* aLibrary,
                                           PRInt64           aSpaceNeeded,
                                           PRInt64           aSpaceAvailable,
                                           PRBool*           aAbort)
{
  nsresult rv;

  // get the query parameters
  mDevice = aDevice;
  mLibrary = aLibrary;
  mSpaceNeeded = aSpaceNeeded;
  mSpaceAvailable = aSpaceAvailable;
  mAbort = aAbort;

  // wait to query user until a window is available
  nsCOMPtr<sbIWindowWatcher> windowWatcher;
  windowWatcher = do_GetService("@songbirdnest.com/Songbird/window-watcher;1",
                                &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = windowWatcher->CallWithWindow(NS_LITERAL_STRING("Songbird:Main"),
                                     this,
                                     PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* For most of these, the file extension -> container format mapping is fairly
   reliable, but the codecs can vary wildly. There is no way to do better
   without actually looking inside the files. The codecs listed are usually the
   most commonly found ones for this extension.
 */
sbExtensionToContentFormatEntry_t const
MAP_FILE_EXTENSION_CONTENT_FORMAT[] = {
  /* audio */
  { "mp3",  "audio/mpeg",      "audio/mpeg",  "audio/mpeg",     "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wma",  "audio/x-ms-wma",  "video/x-ms-asf",  "audio/x-ms-wma",   "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aac",  "audio/aac",       "",                 "audio/aac",     "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "m4a",  "audio/aac",       "video/mp4",  "audio/aac",     "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "3gp",  "audio/aac",       "video/3gpp",       "audio/aac",     "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aa",   "audio/audible",   "",     "",        "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aa",   "audio/x-pn-audibleaudio", "", "",    "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "ogg",  "application/ogg", "application/ogg",  "audio/x-vorbis",  "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "oga",  "application/ogg", "application/ogg",  "audio/x-flac",    "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "flac", "audio/x-flac",    "", "audio/x-flac",    "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wav",  "audio/x-wav",     "audio/x-wav",  "audio/x-pcm-int", "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wav",  "audio/x-adpcm",   "audio/x-wav",  "audio/x-adpcm", "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aiff", "audio/x-aiff",    "audio/x-aiff", "audio/x-pcm-int", "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aif",  "audio/x-aiff",    "audio/x-aiff", "audio/x-pcm-int", "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "ape",  "audio/x-ape",     "",             "",                "", "", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },

  /* video */
  { "mp4",  "video/mp4",       "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mov",  "video/quicktime", "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mp4",  "video/quicktime", "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mp4",  "image/jpeg",      "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mpg",  "video/mpeg",      "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mpeg", "video/mpeg",      "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "wmv",  "video/x-ms-asf",  "video/x-ms-asf",  "", "video/x-ms-wmv",    "audio/x-ms-wma",    sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "avi",  "video/x-msvideo", "",                "", "mpeg4",          "wma",            sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "divx", "video/x-msvideo", "",                "", "mpeg4",          "mp3",            sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "3gp",  "video/3gpp",      "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "3g2",  "video/3gpp",      "",                "", "",               "",               sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "ogv",  "application/ogg", "application/ogg", "", "video/x-theora", "audio/x-vorbis", sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  /* images */
  { "png",  "image/png",      "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "jpg",  "image/jpeg",     "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "jpeg", "image/jpeg",     "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "gif",  "image/gif",      "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "bmp",  "image/bmp",      "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "ico",  "image/x-icon",   "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "tiff", "image/tiff",     "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "tif",  "image/tiff",     "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "wmf",  "application/x-msmetafile", "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "jp2",  "image/jp2",      "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "jpx",  "image/jpx",      "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "fpx",  "application/vnd.netfpx", "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "pcd",  "image/x-photo-cd", "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },
  { "pict", "image/pict",     "", "", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_IMAGE },

  /* playlists */
  { "m3u",  "audio/x-mpegurl", "", "", "", "", sbIDeviceCapabilities::CONTENT_PLAYLIST, sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN },
  { "m3u8", "audio/x-mpegurl", "", "", "", "", sbIDeviceCapabilities::CONTENT_PLAYLIST, sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN }
};

PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH =
  NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);

const PRInt32 K = 1000;

/**
 * Convert the string value to an integer
 * \param aValue the string version of the value
 * \return the integer version of the value or 0 if it fails
 */
static PRInt32 ParseInteger(nsAString const & aValue) {
  TRACE(("%s: %s", __FUNCTION__, NS_LossyConvertUTF16toASCII(aValue).get()));
  nsresult rv;
  if (aValue.IsEmpty()) {
    return 0;
  }
  PRUint32 val = aValue.ToInteger(&rv, 10);
  if (NS_FAILED(rv)) {
    val = 0;
  }
  return val;
}

/**
 * Returns the formatting information for an item
 * \param aItem The item we want the format stuff for
 * \param aFormatType the formatting map entry for the item
 * \param aBitRate The bit rate for the item
 * \param aSampleRate the sample rate for the item
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypeForItem(
                 sbIMediaItem * aItem,
                 sbExtensionToContentFormatEntry_t & aFormatType,
                 PRUint32 & aBitRate,
                 PRUint32 & aSampleRate)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  // We do it with string manipulation here rather than proper URL objects due
  // to thread-unsafety of URLs.
  nsString contentURL;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                          contentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the format type.
  rv = GetFormatTypeForURL(contentURL, aFormatType);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return rv;
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the bit rate.
  nsString bitRate;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_BITRATE), bitRate);
  NS_ENSURE_SUCCESS(rv, rv);
  aBitRate = NS_MIN (ParseInteger(bitRate) * K, 0);

  // Get the sample rate.
  nsString sampleRate;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                          sampleRate);
  NS_ENSURE_SUCCESS(rv, rv);
  aSampleRate = NS_MIN (ParseInteger(sampleRate), 0);

  return NS_OK;
}

/**
 * Returns the formatting information for an item
 * \param aItem The item we want the format stuff for
 * \param aFormatType the formatting map entry for the item
 * \param aSampleRate the sample rate for the item
 * \param aChannels the number of channels for the item
 * \param aBitRate The bit rate for the item
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypeForItem(
                 sbIMediaItem * aItem,
                 sbExtensionToContentFormatEntry_t & aFormatType,
                 PRUint32 & aSampleRate,
                 PRUint32 & aChannels,
                 PRUint32 & aBitRate)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  // We do it with string manipulation here rather than proper URL objects due
  // to thread-unsafety of URLs.
  nsString contentURL;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                          contentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the format type.
  rv = GetFormatTypeForURL(contentURL, aFormatType);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return rv;
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the bit rate.
  nsString bitRate;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_BITRATE), bitRate);
  NS_ENSURE_SUCCESS(rv, rv);
  aBitRate = NS_MIN (ParseInteger(bitRate) * K, 0);

  // Get the sample rate.
  nsString sampleRate;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                          sampleRate);
  NS_ENSURE_SUCCESS(rv, rv);
  aSampleRate = NS_MIN (ParseInteger(sampleRate), 0);

  // Get the channel count.
  nsString channels;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CHANNELS),
                          channels);
  NS_ENSURE_SUCCESS(rv, rv);
  aChannels = NS_MIN (ParseInteger(channels), 0);

  return NS_OK;
}

/**
 * Returns the formatting information for a URI
 * \param aURI The URI we want the format stuff for
 * \param aFormatType the formatting map entry for the URI
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypeForURI
                 (nsIURI*                            aURI,
                  sbExtensionToContentFormatEntry_t& aFormatType)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv;

  // Get the URI spec.
  nsCAutoString uriSpec;
  rv = aURI->GetSpec(uriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the format type.
  return GetFormatTypeForURL(NS_ConvertUTF8toUTF16(uriSpec), aFormatType);
}

/**
 * Returns the formatting information for a URL
 * \param aURL The URL we want the format stuff for
 * \param aFormatType the formatting map entry for the URL
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypeForURL
                 (const nsAString&                   aURL,
                  sbExtensionToContentFormatEntry_t& aFormatType)
{
  TRACE(("%s", __FUNCTION__));

  PRInt32 const lastDot = aURL.RFind(NS_LITERAL_STRING("."));
  if (lastDot != -1) {
    nsDependentSubstring fileExtension(aURL,
                                       lastDot + 1,
                                       aURL.Length() - lastDot - 1);
    nsCAutoString extension = NS_ConvertUTF16toUTF8(fileExtension);
    ToLowerCase(extension);
    for (PRUint32 index = 0;
         index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
         ++index) {
      sbExtensionToContentFormatEntry_t const & entry =
        MAP_FILE_EXTENSION_CONTENT_FORMAT[index];
      if (extension.EqualsLiteral(entry.Extension)) {
        TRACE(("%s: ext %s type %s container %s codec %s",
               __FUNCTION__, entry.Extension, entry.MimeType,
               entry.ContainerFormat, entry.Codec));
        aFormatType = entry;
        return NS_OK;
      }
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/**
 * Returns the formatting information for a MIME type
 * \param aMimeType The MIME type we want the format stuff for
 * \param aFormatTypeList the formatting map entries for the MIME type
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypesForMimeType
                 (const nsAString&                             aMimeType,
                  const PRUint32                               aContentType,
                  nsTArray<sbExtensionToContentFormatEntry_t>& aFormatTypeList)
{
  TRACE(("%s", __FUNCTION__));

  aFormatTypeList.Clear();
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
       ++index) {
    sbExtensionToContentFormatEntry_t const & entry =
      MAP_FILE_EXTENSION_CONTENT_FORMAT[index];

    if (aMimeType.EqualsLiteral(entry.MimeType) &&
      aContentType == entry.ContentType) {
      TRACE(("%s: ext %s type %s container %s codec %s",
             __FUNCTION__, entry.Extension, entry.MimeType,
             entry.ContainerFormat, entry.Codec));
      NS_ENSURE_TRUE(aFormatTypeList.AppendElement(entry),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }

  return NS_OK;
}

/**
 * Return in aMimeType the audio MIME type for the container specified by
 * aContainer and codec specified by aCodec.
 *
 * \param aContainer            Container type.
 * \param aCodec                Codec type.
 * \param aMimeType             Returned MIME type.
 */
/* static */ nsresult
sbDeviceUtils::GetAudioMimeTypeForFormatTypes(const nsAString& aContainer,
                                              const nsAString& aCodec,
                                              nsAString&       aMimeType)
{
  TRACE(("%s", __FUNCTION__));

  // Search the content format map for a match.
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
       ++index) {
    // Get the next format entry.
    sbExtensionToContentFormatEntry_t const& entry =
      MAP_FILE_EXTENSION_CONTENT_FORMAT[index];

    // Skip entry if it's not audio.
    if (entry.ContentType != sbIDeviceCapabilities::CONTENT_AUDIO)
      continue;

    // Check for a match, returning if a match is found.
    if (aContainer.EqualsLiteral(entry.ContainerFormat) &&
        aCodec.EqualsLiteral(entry.Codec)) {
      TRACE(("%s: container %s codec %s mime type %s",
             __FUNCTION__, entry.ContainerFormat, entry.Codec, entry.MimeType));
      aMimeType.AssignLiteral(entry.MimeType);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/**
 * Maps between the device caps and the transcode profile content types
 */
static PRUint32 TranscodeToCapsContentTypeMap[] = {
  sbIDeviceCapabilities::CONTENT_UNKNOWN,
  sbIDeviceCapabilities::CONTENT_AUDIO,
  sbIDeviceCapabilities::CONTENT_IMAGE,
  sbIDeviceCapabilities::CONTENT_VIDEO
};

/**
 * Gets the format information for a format type
 */
static nsresult
GetContainerFormatAndCodec(nsISupports * aFormatType,
                           PRUint32 aContentType,
                           nsAString & aContainerFormat,
                           nsAString & aVideoType,
                           nsAString & aAudioType,
                           nsAString & aCodec,
                           sbIDevCapRange ** aBitRateRange = nsnull,
                           sbIDevCapRange ** aSampleRateRange = nsnull)
{
  nsresult rv;

  TRACE(("%s", __FUNCTION__));

  switch (aContentType) {
    case sbIDeviceCapabilities::CONTENT_AUDIO: {
      nsCOMPtr<sbIAudioFormatType> audioFormat =
        do_QueryInterface(aFormatType);
      if (audioFormat) {
        nsCString temp;
        audioFormat->GetContainerFormat(temp);
        aContainerFormat = NS_ConvertASCIItoUTF16(temp);
        audioFormat->GetAudioCodec(temp);
        aCodec = NS_ConvertASCIItoUTF16(temp);
        if (aBitRateRange) {
          audioFormat->GetSupportedBitrates(aBitRateRange);
        }
        if (aSampleRateRange) {
          audioFormat->GetSupportedSampleRates(aSampleRateRange);
        }
      }
    }
    break;
    case sbIDeviceCapabilities::CONTENT_IMAGE: {
      nsCOMPtr<sbIImageFormatType> imageFormat =
        do_QueryInterface(aFormatType);
      if (imageFormat) {
        nsCString temp;
        imageFormat->GetImageFormat(temp);
        aContainerFormat = NS_ConvertASCIItoUTF16(temp);
        if (aBitRateRange) {
          *aBitRateRange = nsnull;
        }
        if (aSampleRateRange) {
          *aSampleRateRange = nsnull;
        }
      }
    }
    break;
    case sbIDeviceCapabilities::CONTENT_VIDEO: {
      nsCOMPtr<sbIVideoFormatType> videoFormat =
        do_QueryInterface(aFormatType);
      if (videoFormat) {

        nsCOMPtr<sbIDevCapVideoStream> videoStream;
        rv = videoFormat->GetVideoStream(getter_AddRefs(videoStream));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDevCapAudioStream> audioStream;
        rv = videoFormat->GetAudioStream(getter_AddRefs(audioStream));
        nsCString videoType;
        if (aBitRateRange && videoStream) {
          videoStream->GetSupportedBitRates(aBitRateRange);
          rv = videoStream->GetType(videoType);
          NS_ENSURE_SUCCESS(rv, rv);
          aVideoType = NS_ConvertASCIItoUTF16(videoType);
        }
        nsCString audioType;
        if (aSampleRateRange && audioStream) {
          audioStream->GetSupportedSampleRates(aSampleRateRange);
          rv = audioStream->GetType(audioType);
          NS_ENSURE_SUCCESS(rv, rv);
          aAudioType = NS_ConvertASCIItoUTF16(audioType);
        }
      }
      if (aSampleRateRange) {
        *aSampleRateRange = nsnull;
      }
    }
    break;
    default: {
      NS_WARNING("Unexpected content type in GetContainerFormatAndCodec");
      if (aBitRateRange) {
        *aBitRateRange = nsnull;
      }
      if (aSampleRateRange) {
        *aSampleRateRange = nsnull;
      }
    }
    break;
  }
  return NS_OK;
}

nsresult
sbDeviceUtils::GetTranscodeProfiles(PRUint32 aType, nsIArray ** aProfiles)
{
  nsresult rv;

  nsCOMPtr<sbITranscodeManager> tcManager = do_ProxiedGetService(
          "@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = tcManager->GetTranscodeProfiles(aType, aProfiles);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceUtils::GetSupportedTranscodeProfiles(PRUint32 aType,
                                             sbIDevice * aDevice,
                                             nsIArray **aProfiles)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aProfiles);

  nsresult rv;

  nsCOMPtr<nsIMutableArray> supportedProfiles = do_CreateInstance(
          SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> profiles;
  rv = GetTranscodeProfiles(aType, getter_AddRefs(profiles));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceCapabilities> devCaps;
  rv = aDevice->GetCapabilities(getter_AddRefs(devCaps));
  NS_ENSURE_SUCCESS(rv, rv);

  // These are the content types we care about
  static PRUint32 contentTypes[] = {
    sbIDeviceCapabilities::CONTENT_AUDIO,
    sbIDeviceCapabilities::CONTENT_IMAGE,
    sbIDeviceCapabilities::CONTENT_VIDEO
  };

  // Process each content type
  for (PRUint32 contentTypeIndex = 0;
       contentTypeIndex < NS_ARRAY_LENGTH(contentTypes);
       ++contentTypeIndex)
  {
    PRUint32 const contentType = contentTypes[contentTypeIndex];
    TRACE(("%s: Finding profiles for content type %d", __FUNCTION__, contentType));
    PRUint32 mimeTypesLength;
    char ** mimeTypes;
    rv = devCaps->GetSupportedMimeTypes(contentType,
                                        &mimeTypesLength,
                                        &mimeTypes);
    // Not found error is expected, we'll not do anything in that case, but we
    // need to finish out processing and not return early
    if (rv != NS_ERROR_NOT_AVAILABLE) {
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (NS_SUCCEEDED(rv)) {
      TRACE(("%s: Checking %d mime types for type %d", __FUNCTION__,
             mimeTypesLength, contentType));

      for (PRUint32 mimeTypeIndex = 0;
           mimeTypeIndex < mimeTypesLength && NS_SUCCEEDED(rv);
           ++mimeTypeIndex)
      {
        TRACE(("%s: Checking mime type %s", __FUNCTION__,
               mimeTypes[mimeTypeIndex]));
        nsString mimeType;
        mimeType.AssignLiteral(mimeTypes[mimeTypeIndex]);
        NS_Free(mimeTypes[mimeTypeIndex]);

        nsISupports** formatTypes;
        PRUint32 formatTypeCount;
        rv = devCaps->GetFormatTypes(contentType,
                                     mimeType,
                                     &formatTypeCount,
                                     &formatTypes);
        NS_ENSURE_SUCCESS (rv, rv);
        sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                             formatTypes);

        for (PRUint32 formatIndex = 0;
             formatIndex < formatTypeCount;
             formatIndex++)
        {
          nsCOMPtr<nsISupports> formatTypeSupports = formatTypes[formatIndex];

          nsString containerFormat;
          nsString codec;
          nsString videoType; // Not used
          nsString audioType; // Not used
          rv = GetContainerFormatAndCodec(formatTypeSupports,
                                          contentType,
                                          containerFormat,
                                          videoType,
                                          audioType,
                                          codec);
          NS_ENSURE_SUCCESS(rv, rv);
          TRACE(("%s: Checking format %d with container %s, video %s, audio %s, codec %s",
                  __FUNCTION__, formatIndex,
                  NS_ConvertUTF16toUTF8(containerFormat).BeginReading(),
                  NS_ConvertUTF16toUTF8(videoType).BeginReading(),
                  NS_ConvertUTF16toUTF8(audioType).BeginReading(),
                  NS_ConvertUTF16toUTF8(codec).BeginReading()));

          // Look for a match among our transcoding profile
          PRUint32 length;
          rv = profiles->GetLength(&length);
          NS_ENSURE_SUCCESS(rv, rv);

          for (PRUint32 index = 0;
               index < length && NS_SUCCEEDED(rv);
               ++index)
          {
            nsCOMPtr<sbITranscodeProfile> profile = do_QueryElementAt(profiles,
                                                                      index,
                                                                      &rv);
            NS_ENSURE_SUCCESS(rv, rv);
            nsString profileContainerFormat;
            rv = profile->GetContainerFormat(profileContainerFormat);
            NS_ENSURE_SUCCESS(rv, rv);

            PRUint32 profileType;
            rv = profile->GetType(&profileType);
            NS_ENSURE_SUCCESS(rv, rv);

            nsString audioCodec;
            rv = profile->GetAudioCodec(audioCodec);
            NS_ENSURE_SUCCESS(rv, rv);

            nsString videoCodec;
            rv = profile->GetVideoCodec(videoCodec);

            if (TranscodeToCapsContentTypeMap[profileType] == contentType &&
                profileContainerFormat.Equals(containerFormat))
            {
              if ((contentType == sbIDeviceCapabilities::CONTENT_AUDIO &&
                   audioCodec.Equals(codec)) ||
                  (contentType == sbIDeviceCapabilities::CONTENT_VIDEO &&
                   videoCodec.Equals(codec)))
              {
                TRACE(("%s: Adding this format", __FUNCTION__));
                rv = supportedProfiles->AppendElement(profile, PR_FALSE);
                NS_ENSURE_SUCCESS(rv, rv);
              }
            }
          }
        }
      }
      NS_Free(mimeTypes);
    }
  }

  rv = CallQueryInterface(supportedProfiles.get(), aProfiles);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Helper to determine if a value is in the range
 * \param aValue the value to check for
 * \param aRange the range to check for the value
 * \return true if the value is in the range else false
 */
static
bool IsValueInRange(PRInt32 aValue, sbIDevCapRange * aRange) {
  if (!aValue || !aRange) {
    return true;
  }
  PRBool inRange;
  nsresult rv = aRange->IsValueInRange(aValue, &inRange);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return inRange != PR_FALSE;
}

/**
 * Determine if an item needs transcoding
 * NOTE: This is deprecated and will be replaced by the overloaded version below
 * \param aFormatType The format type mapping of the item
 * \param aBitRate the bit rate of the item
 * \param aDevice The device we're transcoding to
 * \param aNeedsTranscoding where we put our flag denoting it needs or does not
 *                          need transcoding
 */
nsresult
sbDeviceUtils::DoesItemNeedTranscoding(
                                sbExtensionToContentFormatEntry_t & aFormatType,
                                PRUint32 & aBitRate,
                                PRUint32 & aSampleRate,
                                sbIDevice * aDevice,
                                bool & aNeedsTranscoding)
{
  TRACE(("%s", __FUNCTION__));
  nsCOMPtr<sbIDeviceCapabilities> devCaps;
  nsresult rv = aDevice->GetCapabilities(getter_AddRefs(devCaps));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const devCapContentType =
                  TranscodeToCapsContentTypeMap[aFormatType.TranscodeType];

  nsString itemContainerFormat;
  itemContainerFormat.AssignLiteral(aFormatType.ContainerFormat);
  nsString itemCodec;
  itemCodec.AssignLiteral(aFormatType.Codec);

  LOG(("Determining if item needs transcoding\n\tItem Container: '%s'\n\tItem Codec: '%s'",
       NS_LossyConvertUTF16toASCII(itemContainerFormat).get(),
       NS_LossyConvertUTF16toASCII(itemCodec).get()));

  PRUint32 mimeTypesLength;
  char ** mimeTypes;
  rv = devCaps->GetSupportedMimeTypes(devCapContentType,
                                      &mimeTypesLength,
                                      &mimeTypes);
  // If we know of transcoding formats than check them
  if (NS_SUCCEEDED(rv) && mimeTypesLength > 0) {
    aNeedsTranscoding = true;
    for (PRUint32 mimeTypesIndex = 0;
         mimeTypesIndex < mimeTypesLength;
         ++mimeTypesIndex) {

      NS_ConvertASCIItoUTF16 mimeType(mimeTypes[mimeTypesIndex]);
      nsISupports** formatTypes;
      PRUint32 formatTypeCount;
      rv = devCaps->GetFormatTypes(devCapContentType,
                                   mimeType,
                                   &formatTypeCount,
                                   &formatTypes);
      NS_ENSURE_SUCCESS (rv, rv);
      sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                           formatTypes);

      for (PRUint32 formatIndex = 0;
           formatIndex < formatTypeCount;
           formatIndex++)
      {
        nsCOMPtr<nsISupports> formatType = formatTypes[formatIndex];
        nsString containerFormat;
        nsString codec;

        nsCOMPtr<sbIDevCapRange> bitRateRange;
        nsCOMPtr<sbIDevCapRange> sampleRateRange;
        nsString videoType; // Not used
        nsString audioType; // Not used
        rv = GetContainerFormatAndCodec(formatType,
                                        devCapContentType,
                                        containerFormat,
                                        videoType,
                                        audioType,
                                        codec,
                                        getter_AddRefs(bitRateRange),
                                        getter_AddRefs(sampleRateRange));
        if (NS_SUCCEEDED(rv)) {

          LOG(("Comparing container and codec\n\tCaps Container: '%s'\n\tCaps Codec: '%s'",
               NS_LossyConvertUTF16toASCII(containerFormat).get(),
               NS_LossyConvertUTF16toASCII(codec).get()));

          // Compare the various attributes, if bit rate and sample rate are
          // not specified then they always match
          if (containerFormat.Equals(itemContainerFormat) &&
              codec.Equals(itemCodec) &&
              (!aBitRate || IsValueInRange(aBitRate, bitRateRange)) &&
              (!aSampleRate || IsValueInRange(aSampleRate, sampleRateRange)))
          {
            TRACE(("%s: no transcoding needed, matches mime type '%s' "
                   "container '%s' codec '%s'",
                   __FUNCTION__, mimeTypes[mimeTypesIndex],
                   NS_LossyConvertUTF16toASCII(containerFormat).get(),
                   NS_LossyConvertUTF16toASCII(codec).get()));
            aNeedsTranscoding = false;
            break;
          }
        }
      }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mimeTypesLength, mimeTypes);
  }
  else { // We don't know the transcoding formats of the device so just copy
    TRACE(("%s: no information on device, assuming no transcoding needed",
           __FUNCTION__));
    aNeedsTranscoding = false;
  }
  TRACE(("%s: result %s", __FUNCTION__, aNeedsTranscoding ? "yes" : "no"));
  return NS_OK;
}

nsresult
sbDeviceUtils::DoesItemNeedTranscoding(PRUint32 aTranscodeType,
                                       sbIMediaFormat * aMediaFormat,
                                       sbIDevice* aDevice,
                                       bool & aNeedsTranscoding)
{
  TRACE(("%s", __FUNCTION__));
  nsCOMPtr<sbIDeviceCapabilities> devCaps;
  nsresult rv = aDevice->GetCapabilities(getter_AddRefs(devCaps));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const devCapContentType =
                  TranscodeToCapsContentTypeMap[aTranscodeType];

  nsCOMPtr<sbIDeviceCapsCompatibility> devCompatible =
    do_CreateInstance(SONGBIRD_DEVICECAPSCOMPATIBILITY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = devCompatible->Initialize(devCaps, aMediaFormat, devCapContentType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool compatible;
  rv = devCompatible->Compare(&compatible);
  NS_ENSURE_SUCCESS(rv, rv);

  aNeedsTranscoding = !compatible;
  return NS_OK;
}

// XXX this needs to be fixed to be not gstreamer specific
nsresult
sbDeviceUtils::GetTranscodingConfigurator(
        PRUint32 aTranscodeType,
        sbIDeviceTranscodingConfigurator **aConfigurator)
{
 nsresult rv;
 nsCOMPtr<sbIDeviceTranscodingConfigurator> configurator;
 if (aTranscodeType == sbITranscodeProfile::TRANSCODE_TYPE_AUDIO) {
    configurator = do_CreateInstance(
            "@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/"
            "Audio/GStreamer;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    configurator = do_CreateInstance(
            "@songbirdnest.com/Songbird/Mediacore/Transcode/Configurator/"
            "Device/GStreamer;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF (*aConfigurator = configurator);

  return NS_OK;
}

nsresult
sbDeviceUtils::ApplyPropertyPreferencesToProfile(sbIDevice* aDevice,
                                                 nsIArray*  aPropertyArray,
                                                 nsString   aPrefNameBase)
{
  nsresult rv;

  if(!aPropertyArray) {
    // Perfectly ok; this just means there aren't any properties.
    return NS_OK;
  }

  PRUint32 numProperties;
  rv = aPropertyArray->GetLength(&numProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < numProperties; i++) {
    nsCOMPtr<sbITranscodeProfileProperty> property =
        do_QueryElementAt(aPropertyArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propName;
    rv = property->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString prefName = aPrefNameBase;
    prefName.AppendLiteral(".");
    prefName.Append(propName);

    nsCOMPtr<nsIVariant> prefVariant;
    rv = aDevice->GetPreference(prefName, getter_AddRefs(prefVariant));
    NS_ENSURE_SUCCESS(rv, rv);

    // Only use the property if we have a real value (not a void/empty variant)
    PRUint16 dataType;
    rv = prefVariant->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataType != nsIDataType::VTYPE_VOID &&
        dataType != nsIDataType::VTYPE_EMPTY)
    {
      rv = property->SetValue(prefVariant);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbDeviceUtils::GetTranscodedFileExtension(sbITranscodeProfile *aProfile,
                                          nsCString &aExtension)
{
  NS_ENSURE_TRUE(aProfile, NS_ERROR_UNEXPECTED);

  nsString temp;
  nsresult rv = aProfile->GetContainerFormat(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ConvertUTF16toUTF8 containerFormat(temp);

  rv =  aProfile->GetAudioCodec(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ConvertUTF16toUTF8 codec(temp);

  for (PRUint32 index = 0;
       index < MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH;
       ++index) {
    sbExtensionToContentFormatEntry_t const & entry =
      MAP_FILE_EXTENSION_CONTENT_FORMAT[index];
    if (containerFormat.Equals(entry.ContainerFormat) &&
        codec.Equals(entry.Codec)) {
      aExtension.AssignLiteral(entry.Extension);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}


nsresult
sbDeviceUtils::GetCodecAndContainerForMimeType(nsCString aMimeType,
                                               nsCString &aContainer,
                                               nsCString &aCodec,
                                               nsCString &aVideoType,
                                               nsCString &aAudioType)
{
  for (PRUint32 index = 0;
       index < MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH;
       ++index)
  {
    sbExtensionToContentFormatEntry_t const & entry =
      MAP_FILE_EXTENSION_CONTENT_FORMAT[index];

    if (aMimeType.EqualsLiteral(entry.MimeType)) {
      aContainer.AssignLiteral(entry.ContainerFormat);
      aCodec.AssignLiteral(entry.Codec);
      aVideoType.AssignLiteral(entry.VideoType);
      aAudioType.AssignLiteral(entry.AudioType);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

bool
sbDeviceUtils::IsItemDRMProtected(sbIMediaItem * aMediaItem)
{
  nsresult rv;

  nsString isDRMProtected;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISDRMPROTECTED),
                               isDRMProtected);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv) || rv == NS_ERROR_NOT_AVAILABLE,
             "sbDeviceUtils::IsItemDRMProtected Unexpected error, "
             "returning false");
  return NS_SUCCEEDED(rv) && isDRMProtected.EqualsLiteral("1");
}

/* static */ nsresult
sbDeviceUtils::GetDeviceCapsTypeFromMediaItem(sbIMediaItem *aMediaItem,
                                              PRUint32 *aContentType,
                                              PRUint32 *aFunctionType)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aContentType);
  NS_ENSURE_ARG_POINTER(aFunctionType);

  nsresult rv;

  *aContentType = sbIDeviceCapabilities::CONTENT_UNKNOWN;
  *aFunctionType = sbIDeviceCapabilities::FUNCTION_UNKNOWN;

  nsString itemContentType;
  rv = aMediaItem->GetContentType(itemContentType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 contentType, functionType;
  if (itemContentType.Equals(NS_LITERAL_STRING("audio"))) {
    contentType = sbIDeviceCapabilities::CONTENT_AUDIO;
    functionType = sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK;
  }
  else if (itemContentType.Equals(NS_LITERAL_STRING("video"))) {
    contentType = sbIDeviceCapabilities::CONTENT_VIDEO;
    functionType = sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK;
  }
  else if (itemContentType.Equals(NS_LITERAL_STRING("image"))) {
    contentType = sbIDeviceCapabilities::CONTENT_IMAGE;
    functionType = sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY;
  }
  else {
    NS_WARNING("sbDeviceUtils::GetDeviceCapsTypeFromMediaItem: "
               "returning unknown device capabilities type");
    contentType = sbIDeviceCapabilities::CONTENT_UNKNOWN;
    functionType = sbIDeviceCapabilities::FUNCTION_UNKNOWN;
  }

  *aContentType = contentType;
  *aFunctionType = functionType;
  return NS_OK;
};

/* static */ nsresult
sbDeviceUtils::GetDeviceCapsTypeFromListContentType(PRUint16 aListContentType,
                                                    PRUint32 *aContentType,
                                                    PRUint32 *aFunctionType)
{
  NS_ENSURE_ARG_POINTER(aContentType);
  NS_ENSURE_ARG_POINTER(aFunctionType);

  PRUint32 contentType, functionType;
  // Map mix type list to audio.
  if (aListContentType & sbIMediaList::CONTENTTYPE_AUDIO) {
    contentType = sbIDeviceCapabilities::CONTENT_AUDIO;
    functionType = sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK;
  }
  else if (aListContentType == sbIMediaList::CONTENTTYPE_VIDEO) {
    contentType = sbIDeviceCapabilities::CONTENT_VIDEO;
    functionType = sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK;
  }
  else {
    NS_WARNING("sbDeviceUtils::GetDeviceCapsTypeFromListContentType: "
               "returning unknown device capabilities type");
    contentType = sbIDeviceCapabilities::CONTENT_UNKNOWN;
    functionType = sbIDeviceCapabilities::FUNCTION_UNKNOWN;
  }

  *aContentType = contentType;
  *aFunctionType = functionType;

  return NS_OK;
};

/* static */ PRBool
sbDeviceUtils::IsMediaItemSupported(sbIDevice *aDevice,
                                    sbIMediaItem *aMediaItem)
{
  NS_ENSURE_TRUE(aDevice, PR_FALSE);
  NS_ENSURE_TRUE(aMediaItem, PR_FALSE);

  nsresult rv;

  PRUint32 contentType, functionType;
  rv = GetDeviceCapsTypeFromMediaItem(aMediaItem,
                                      &contentType,
                                      &functionType);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsCOMPtr<sbIDeviceCapabilities> capabilities;
  rv = aDevice->GetCapabilities(getter_AddRefs(capabilities));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool isSupported;
  rv = capabilities->SupportsContent(functionType,
                                     contentType,
                                     &isSupported);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return isSupported;
}

/* static */ PRBool
sbDeviceUtils::IsMediaListContentTypeSupported(sbIDevice *aDevice,
                                               PRUint16 aListContentType)
{
  NS_ENSURE_TRUE(aDevice, PR_FALSE);

  nsresult rv;

  PRUint32 contentType, functionType;
  rv = GetDeviceCapsTypeFromListContentType(aListContentType,
                                            &contentType,
                                            &functionType);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv), PR_FALSE);

  nsCOMPtr<sbIDeviceCapabilities> capabilities;
  rv = aDevice->GetCapabilities(getter_AddRefs(capabilities));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool isSupported;
  rv = capabilities->SupportsContent(functionType,
                                     contentType,
                                     &isSupported);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return isSupported;
}

/*static*/
nsresult sbDeviceUtils::AddSupportedFileExtensions
                          (sbIDevice*          aDevice,
                           PRUint32            aContentType,
                           nsTArray<nsString>& aFileExtensionList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  nsresult rv;

  // Get the device capabilities.
  nsCOMPtr<sbIDeviceCapabilities> caps;
  rv = aDevice->GetCapabilities(getter_AddRefs(caps));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of supported MIME types and set it up for auto-disposal.  Just
  // return if content type is not available.
  char** mimeTypeList;
  PRUint32 mimeTypeCount;
  rv = caps->GetSupportedMimeTypes(aContentType, &mimeTypeCount, &mimeTypeList);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_OK;
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSArray<char*> autoMimeTypeList(mimeTypeList, mimeTypeCount);

  // Get the set of file extensions from the MIME type list.
  for (PRUint32 i = 0; i < mimeTypeCount; ++i) {
    // Get the format types for the MIME type.
    nsTArray<sbExtensionToContentFormatEntry_t> formatTypeList;
    rv = sbDeviceUtils::GetFormatTypesForMimeType
                          (NS_ConvertASCIItoUTF16(mimeTypeList[i]),
                           aContentType,
                           formatTypeList);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add each file extension to the list.
    for (PRUint32 j = 0; j < formatTypeList.Length(); ++j) {
      // Add the file extension to the list.
      NS_ConvertASCIItoUTF16 extension(formatTypeList[j].Extension);
      if (!aFileExtensionList.Contains(extension)) {
        aFileExtensionList.AppendElement(extension);
      }
    }
  }

  return NS_OK;
}

nsresult
sbDeviceUtils::GetMediaSettings(
                            sbIDeviceLibrary * aDevLib,
                            PRUint32 aMediaType,
                            sbIDeviceLibraryMediaSyncSettings ** aMediaSettings)
{
  NS_ASSERTION(aDevLib, "aDevLib is null");

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  nsresult rv = aDevLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_IMAGE,
                                      aMediaSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceUtils::GetMgmtTypeForMedia(sbIDeviceLibrary * aDevLib,
                                   PRUint32 aMediaType,
                                   PRUint32 & aMgmtType)
{
  NS_ASSERTION(aDevLib, "aDevLib is null");
  nsresult rv;

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  rv = GetMediaSettings(aDevLib,
                        sbIDeviceLibrary::MEDIATYPE_IMAGE,
                        getter_AddRefs(mediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediaSyncSettings->GetMgmtType(&aMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbDeviceUtils::GetDeviceLibrary(nsAString const & aDeviceLibGuid,
                                         sbIDevice * aDevice,
                                         sbIDeviceLibrary ** aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);

  nsresult rv;

  // mediaItem.library is not a sbIDeviceLibrary, test GUID :(
  nsCOMPtr<sbIDeviceContent> content;
  rv = aDevice->GetContent(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> libraries;
  rv = content->GetLibraries(getter_AddRefs(libraries));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryCount;
  rv = libraries->GetLength(&libraryCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < libraryCount; ++index) {
    nsCOMPtr<sbIDeviceLibrary> deviceLib =
      do_QueryElementAt(libraries, index, &rv);
    if (NS_FAILED(rv))
      continue;

    nsString deviceLibGuid;
    rv = deviceLib->GetGuid(deviceLibGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    if (deviceLibGuid.Equals(aDeviceLibGuid)) {
      deviceLib.forget(aDeviceLibrary);
      return NS_OK;
    }
  }

  *aDeviceLibrary = nsnull;
  return NS_OK;
}

nsresult sbDeviceUtils::GetDeviceLibrary(nsAString const & aDevLibGuid,
                                         nsID const * aDeviceID,
                                         sbIDeviceLibrary ** aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);

  nsresult rv;

  nsCOMPtr<sbIDeviceLibrary> deviceLibrary;

  nsCOMPtr<sbIDeviceRegistrar> deviceRegistrar =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);

  if (aDeviceID) {
    nsCOMPtr<sbIDevice> device;
    rv = deviceRegistrar->GetDevice(aDeviceID, getter_AddRefs(device));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetDeviceLibrary(aDevLibGuid, device, getter_AddRefs(deviceLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<nsIArray> devices;
    rv = deviceRegistrar->GetDevices(getter_AddRefs(devices));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbIDevice> device;
    PRUint32 length;
    rv = devices->GetLength(&length);
    for (PRUint32 index = 0; index < length && !deviceLibrary; ++index) {
      device = do_QueryElementAt(devices, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = GetDeviceLibrary(aDevLibGuid, device, getter_AddRefs(deviceLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  deviceLibrary.forget(aDeviceLibrary);

  return NS_OK;
}

nsresult sbDeviceUtils::GetSyncItemInLibrary(sbIMediaItem*  aMediaItem,
                                             sbILibrary*    aTargetLibrary,
                                             sbIMediaItem** aSyncItem)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aTargetLibrary);
  NS_ENSURE_ARG_POINTER(aSyncItem);

  // Function variables.
  nsresult rv;

  // Try getting the sync item directly in the target library.
  rv = sbLibraryUtils::GetItemInLibrary(aMediaItem, aTargetLibrary, aSyncItem);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aSyncItem)
    return NS_OK;

  // Get the source library.
  nsCOMPtr<sbILibrary> sourceLibrary;
  rv = aMediaItem->GetLibrary(getter_AddRefs(sourceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Try getting the sync item using the outer GUID property.
  nsAutoString outerGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_OUTERGUID),
                               outerGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!outerGUID.IsEmpty()) {
    // Get the outer media item.
    nsCOMPtr<sbIMediaItem> outerMediaItem;
    rv = sourceLibrary->GetMediaItem(outerGUID, getter_AddRefs(outerMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Try getting the sync item from the outer media item.
    rv = sbLibraryUtils::GetItemInLibrary(outerMediaItem,
                                          aTargetLibrary,
                                          aSyncItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (*aSyncItem)
      return NS_OK;
  }

  // Try getting the sync item using the storage GUID property.
  nsAutoString storageGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                               storageGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!storageGUID.IsEmpty()) {
    // Get the storage media item.
    nsCOMPtr<sbIMediaItem> storageMediaItem;
    rv = sourceLibrary->GetMediaItem(storageGUID,
                                     getter_AddRefs(storageMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Try getting the sync item from the storage media item.
    rv = sbLibraryUtils::GetItemInLibrary(storageMediaItem,
                                          aTargetLibrary,
                                          aSyncItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (*aSyncItem)
      return NS_OK;
  }

  // Could not find the sync item.
  *aSyncItem = nsnull;

  return NS_OK;
}

nsresult sbDeviceUtils::SetOriginIsInMainLibrary(sbIMediaItem * aMediaItem,
                                                 sbILibrary * aDevLibrary,
                                                 PRBool aMark)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  NS_NAMED_LITERAL_STRING(SB_PROPERTY_TRUE, "1");
  NS_NAMED_LITERAL_STRING(SB_PROPERTY_FALSE, "0");

  nsCOMPtr<sbIMediaItem> itemInDeviceLibrary;
  rv = sbDeviceUtils::GetSyncItemInLibrary(aMediaItem,
                                           aDevLibrary,
                                           getter_AddRefs(itemInDeviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  if (itemInDeviceLibrary) {
    nsString inMainLibrary;
    rv = itemInDeviceLibrary->GetProperty(
                     NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY),
                     inMainLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
    // Default to "false" if there is no property
    if (NS_FAILED(rv) || inMainLibrary.IsVoid()) {
      inMainLibrary = SB_PROPERTY_FALSE;
    }
    // If the flag is different than what we want change it
    const PRBool isMarked =
        !inMainLibrary.Equals(SB_PROPERTY_FALSE) ? PR_TRUE : PR_FALSE;
    if (aMark != isMarked) {
      rv = itemInDeviceLibrary->SetProperty(
                     NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY),
                     aMark ? SB_PROPERTY_TRUE : SB_PROPERTY_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}


sbDeviceListenerIgnore::sbDeviceListenerIgnore(sbBaseDevice * aDevice,
                       sbIMediaItem * aItem) :
                         mDevice(aDevice),
                         mIgnoring(PR_FALSE),
                         mListenerType(LIBRARY),
                         mMediaItem(aItem) {
  NS_ASSERTION(aItem, "aItem is null");
  NS_ADDREF(mMediaItem);
  mDevice->IgnoreMediaItem(aItem);
}

sbDeviceListenerIgnore::~sbDeviceListenerIgnore() {
  if (mMediaItem) {
    mDevice->UnignoreMediaItem(mMediaItem);
    NS_RELEASE(mMediaItem);
  }
  else {
    SetIgnore(PR_FALSE);
  }
}

/**
 * Turns the listners back on if they are turned off
 */
void sbDeviceListenerIgnore::SetIgnore(PRBool aIgnore) {
  // If we're changing the ignore state
  if (mIgnoring != aIgnore) {

    // If the library listener was specified
    if ((mListenerType & LIBRARY) != 0) {
      mDevice->SetIgnoreLibraryListener(aIgnore);
    }

    // If the media list listener was specified
    if ((mListenerType & MEDIA_LIST) != 0) {
        mDevice->SetIgnoreMediaListListeners(aIgnore);
    }
    mIgnoring = aIgnore;
  }
}


//------------------------------------------------------------------------------
// sbIDeviceCapabilities Logging functions
// NOTE: This is built only w/ PR_LOGGING turned on.

#ifdef PR_LOGGING
#define LOG_MODULE(aLogModule, args)   PR_LOG(aLogModule, PR_LOG_WARN, args)

#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */

static nsresult LogFormatType(PRUint32 aContentType,
                              const nsAString & aFormat,
                              sbIDeviceCapabilities *aDeviceCaps,
                              PRLogModuleInfo *aLogModule);

static nsresult LogImageFormatType(sbIImageFormatType *aImageFormatType,
                                   const nsAString & aFormat,
                                   PRLogModuleInfo *aLogModule);

static nsresult LogVideoFormatType(sbIVideoFormatType *aVideoFormatType,
                                   const nsAString & aFormat,
                                   PRLogModuleInfo *aLogModule);

static nsresult LogAudioFormatType(sbIAudioFormatType *aAudioFormatType,
                                   const nsAString & aFormat,
                                   PRLogModuleInfo *aLogModule);

static nsresult LogSizeArray(nsIArray *aSizeArray, PRLogModuleInfo *aLogModule);

static nsresult LogRange(sbIDevCapRange *aRange,
                         PRBool aIsMinMax,
                         PRLogModuleInfo *aLogModule);


/* static */ nsresult
sbDeviceUtils::LogDeviceCapabilities(sbIDeviceCapabilities *aDeviceCaps,
                                     PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aDeviceCaps);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;

  LOG_MODULE(aLogModule,
      ("DEVICE CAPS:\n===============================================\n"));

  PRUint32 functionTypeCount;
  PRUint32 *functionTypes;
  rv = aDeviceCaps->GetSupportedFunctionTypes(&functionTypeCount,
                                              &functionTypes);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemoryPtr functionTypesPtr(functionTypes);
  for (PRUint32 functionType = 0;
       functionType < functionTypeCount;
       functionType++)
  {
    PRUint32 *contentTypes;
    PRUint32 contentTypesLength;

    rv = aDeviceCaps->GetSupportedContentTypes(functionTypes[functionType],
                                               &contentTypesLength,
                                               &contentTypes);
    NS_ENSURE_SUCCESS(rv, rv);

    sbAutoNSMemoryPtr contentTypesPtr(contentTypes);

    for (PRUint32 contentType = 0;
         contentType < contentTypesLength;
         contentType++)
    {
      PRUint32 mimeTypesCount;
      char **mimeTypes;
      rv = aDeviceCaps->GetSupportedMimeTypes(contentTypes[contentType],
                                              &mimeTypesCount,
                                              &mimeTypes);
      NS_ENSURE_SUCCESS(rv, rv);
      sbAutoNSArray<char *> autoMimeTypesPtr(mimeTypes, mimeTypesCount);

      for (PRUint32 mimeType = 0; mimeType < mimeTypesCount; mimeType++) {
        rv = LogFormatType(contentTypes[contentType],
                           NS_ConvertUTF8toUTF16(mimeTypes[mimeType]),
                           aDeviceCaps,
                           aLogModule);
      }
    }
  }

  // Output style
  LOG_MODULE(aLogModule,
      ("\n===============================================\n"));

  return NS_OK;
}

/* static */ nsresult
LogFormatType(PRUint32 aContentType,
              const nsAString & aFormat,
              sbIDeviceCapabilities *aDeviceCaps,
              PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aDeviceCaps);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;
  nsISupports** formatTypes;
  PRUint32 formatTypeCount;
  rv = aDeviceCaps->GetFormatTypes(aContentType,
                                   aFormat,
                                   &formatTypeCount,
                                   &formatTypes);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                       formatTypes);

  for (PRUint32 formatIndex = 0;
       formatIndex < formatTypeCount;
       formatIndex++)
  {
    nsCOMPtr<nsISupports> formatSupports = formatTypes[formatIndex];

    // QI to find out which format this is.
    nsCOMPtr<sbIAudioFormatType> audioFormatType =
        do_QueryInterface(formatSupports, &rv);
    if (NS_SUCCEEDED(rv) && audioFormatType) {
      rv = LogAudioFormatType(audioFormatType, aFormat, aLogModule);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsCOMPtr<sbIVideoFormatType> videoFormatType =
          do_QueryInterface(formatSupports, &rv);
      if (NS_SUCCEEDED(rv) && videoFormatType) {
        rv = LogVideoFormatType(videoFormatType, aFormat, aLogModule);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        // Last chance, attempt image type.
        nsCOMPtr<sbIImageFormatType> imageFormatType =
            do_QueryInterface(formatSupports, &rv);
        if (NS_SUCCEEDED(rv) && imageFormatType) {
          rv = LogImageFormatType(imageFormatType, aFormat, aLogModule);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  return NS_OK;
}

/* static */ nsresult
LogImageFormatType(sbIImageFormatType *aImageFormatType,
                   const nsAString & aFormat,
                   PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aImageFormatType);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;

  nsString format(aFormat);
  LOG_MODULE(aLogModule, (" *** IMAGE FORMAT TYPE '%s' ***",
       NS_ConvertUTF16toUTF8(format).get()));

  nsCString imgFormat;
  rv = aImageFormatType->GetImageFormat(imgFormat);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> widths;
  rv = aImageFormatType->GetSupportedWidths(getter_AddRefs(widths));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> heights;
  rv = aImageFormatType->GetSupportedHeights(getter_AddRefs(heights));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> sizes;
  rv = aImageFormatType->GetSupportedExplicitSizes(getter_AddRefs(sizes));
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * image format: %s", imgFormat.get()));

  LOG_MODULE(aLogModule, (" * image widths:\n"));;
  rv = LogRange(widths, PR_TRUE, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * image heights:\n"));
  rv = LogRange(heights, PR_TRUE, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * image supported sizes:\n"));
  rv = LogSizeArray(sizes, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  // Output style
  LOG_MODULE(aLogModule, ("\n"));
  return NS_OK;
}

/* static */ nsresult
LogVideoFormatType(sbIVideoFormatType *aVideoFormatType,
                   const nsAString & aFormat,
                   PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aVideoFormatType);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;

  nsString format(aFormat);
  LOG_MODULE(aLogModule, (" *** VIDEO FORMAT TYPE '%s' ***",
        NS_ConvertUTF16toUTF8(format).get()));

  nsCString containerType;
  rv = aVideoFormatType->GetContainerType(containerType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapVideoStream> videoStream;
  rv = aVideoFormatType->GetVideoStream(getter_AddRefs(videoStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString videoStreamType;
  rv = videoStream->GetType(videoStreamType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> videoSizes;
  rv = videoStream->GetSupportedExplicitSizes(getter_AddRefs(videoSizes));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> videoWidths;
  rv = videoStream->GetSupportedWidths(getter_AddRefs(videoWidths));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> videoHeights;
  rv = videoStream->GetSupportedHeights(getter_AddRefs(videoHeights));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> videoBitrates;
  rv = videoStream->GetSupportedBitRates(getter_AddRefs(videoBitrates));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 num, den;


  LOG_MODULE(aLogModule, (" * videostream type = %s)", videoStreamType.get()));

  LOG_MODULE(aLogModule, (" * videostream supported widths:"));
  rv = LogRange(videoWidths, PR_TRUE, aLogModule);

  LOG_MODULE(aLogModule, (" * videosream supported heights:"));
  rv = LogRange(videoHeights, PR_TRUE, aLogModule);

  LOG_MODULE(aLogModule, (" * videostream bitrates:"));
  rv = LogRange(videoBitrates, PR_TRUE, aLogModule);

  PRBool supportsPARRange = PR_FALSE;
  rv = videoStream->GetDoesSupportPARRange(&supportsPARRange);
  if (NS_SUCCEEDED(rv) && supportsPARRange) {
    // Simply log the min and the max values here.
    nsCOMPtr<sbIDevCapFraction> minSupportedPAR;
    rv = videoStream->GetMinimumSupportedPAR(getter_AddRefs(minSupportedPAR));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDevCapFraction> maxSupportedPAR;
    rv = videoStream->GetMaximumSupportedPAR(getter_AddRefs(maxSupportedPAR));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = minSupportedPAR->GetNumerator(&num);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = minSupportedPAR->GetDenominator(&den);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG_MODULE(aLogModule, (" * videostream min PAR: %i/%i", num, den));

    rv = maxSupportedPAR->GetNumerator(&num);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = maxSupportedPAR->GetDenominator(&den);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG_MODULE(aLogModule, (" * videostream max PAR: %i/%i", num, den));
  }
  else {
    // Log out the list of PARs.
    nsCOMPtr<nsIArray> supportedPARs;
    rv = videoStream->GetSupportedPARs(getter_AddRefs(supportedPARs));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length = 0;
    rv = supportedPARs->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG_MODULE(aLogModule, (" * videostream PAR count: %i", length));

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIDevCapFraction> curFraction =
        do_QueryElementAt(supportedPARs, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = curFraction->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = curFraction->GetDenominator(&den);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG_MODULE(aLogModule, ("   - %i/%i", num, den));
    }
  }

  PRBool supportsFrameRange = PR_FALSE;
  rv = videoStream->GetDoesSupportFrameRateRange(&supportsFrameRange);
  if (NS_SUCCEEDED(rv) && supportsFrameRange) {
    nsCOMPtr<sbIDevCapFraction> minFrameRate;
    rv = videoStream->GetMinimumSupportedFrameRate(
        getter_AddRefs(minFrameRate));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDevCapFraction> maxFrameRate;
    rv = videoStream->GetMaximumSupportedFrameRate(
        getter_AddRefs(maxFrameRate));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = minFrameRate->GetNumerator(&num);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = minFrameRate->GetDenominator(&den);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG_MODULE(aLogModule, (" * videostream min framerate: %i/%i", num, den));

    rv = maxFrameRate->GetNumerator(&num);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = maxFrameRate->GetDenominator(&den);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG_MODULE(aLogModule, (" * videostream max framerate: %i/%i", num, den));
  }
  else {
    nsCOMPtr<nsIArray> frameRates;
    rv = videoStream->GetSupportedFrameRates(getter_AddRefs(frameRates));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length = 0;
    rv = frameRates->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG_MODULE(aLogModule, (" * videostream framerate count: %i", length));

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIDevCapFraction> curFraction =
        do_QueryElementAt(frameRates, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = curFraction->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = curFraction->GetDenominator(&den);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG_MODULE(aLogModule, ("   - %i/%i", num, den));
    }
  }

  nsCOMPtr<sbIDevCapAudioStream> audioStream;
  rv = aVideoFormatType->GetAudioStream(getter_AddRefs(audioStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString audioStreamType;
  rv = audioStream->GetType(audioStreamType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> audioBitrates;
  rv = audioStream->GetSupportedBitRates(getter_AddRefs(audioBitrates));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> audioSamplerates;
  rv = audioStream->GetSupportedSampleRates(getter_AddRefs(audioSamplerates));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> audioChannels;
  rv = audioStream->GetSupportedChannels(getter_AddRefs(audioChannels));
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * audiostream type = %s", audioStreamType.get()));

  LOG_MODULE(aLogModule, (" * audiostream bitrates:"));
  rv = LogRange(audioBitrates, PR_TRUE, aLogModule);

  LOG_MODULE(aLogModule, (" * audiostream samplerates:"));
  rv = LogRange(audioSamplerates, PR_FALSE, aLogModule);

  LOG_MODULE(aLogModule, (" * audiostream channels"));
  rv = LogRange(audioChannels, PR_TRUE, aLogModule);

  // Output style
  LOG_MODULE(aLogModule, ("\n"));
  return NS_OK;
}

/* static */ nsresult
LogAudioFormatType(sbIAudioFormatType *aAudioFormatType,
                   const nsAString & aFormat,
                   PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aAudioFormatType);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;

  nsString format(aFormat);
  LOG_MODULE(aLogModule, (" *** AUDIO FORMAT TYPE '%s' ***",
        NS_ConvertUTF16toUTF8(format).get()));

  nsCString audioCodec;
  rv = aAudioFormatType->GetAudioCodec(audioCodec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString containerFormat;
  rv = aAudioFormatType->GetContainerFormat(containerFormat);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> bitrates;
  rv = aAudioFormatType->GetSupportedBitrates(getter_AddRefs(bitrates));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> samplerates;
  rv = aAudioFormatType->GetSupportedSampleRates(getter_AddRefs(samplerates));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> channels;
  rv = aAudioFormatType->GetSupportedChannels(getter_AddRefs(channels));
  NS_ENSURE_SUCCESS(rv, rv);

  // Output
  LOG_MODULE(aLogModule, ("  * audioCodec = %s", audioCodec.get()));
  LOG_MODULE(aLogModule, ("  * containerFormat = %s", containerFormat.get()));

  LOG_MODULE(aLogModule, ("  * audio bitrates:"));
  rv = LogRange(bitrates, PR_TRUE, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * audio samplerates:"));
  rv = LogRange(samplerates, PR_FALSE, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_MODULE(aLogModule, (" * audio channels:"));
  rv = LogRange(channels, PR_TRUE, aLogModule);
  NS_ENSURE_SUCCESS(rv, rv);

  // Output style
  LOG_MODULE(aLogModule, ("\n"));
  return NS_OK;
}

/* static */ nsresult
LogSizeArray(nsIArray *aSizeArray, PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aSizeArray);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;
  PRUint32 arrayCount = 0;
  rv = aSizeArray->GetLength(&arrayCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < arrayCount; i++) {
    nsCOMPtr<sbIImageSize> curImageSize =
        do_QueryElementAt(aSizeArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 width, height;
    rv = curImageSize->GetWidth(&width);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = curImageSize->GetHeight(&height);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG_MODULE(aLogModule, ("   - %i x %i", width, height));
  }

  return NS_OK;
}

/* static */ nsresult
LogRange(sbIDevCapRange *aRange,
         PRBool aIsMinMax,
         PRLogModuleInfo *aLogModule)
{
  NS_ENSURE_ARG_POINTER(aRange);
  NS_ENSURE_ARG_POINTER(aLogModule);

  nsresult rv;
  if (aIsMinMax) {
    PRInt32 min, max, step;
    rv = aRange->GetMin(&min);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRange->GetMax(&max);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRange->GetStep(&step);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG_MODULE(aLogModule, ("   - min:  %i", min));
    LOG_MODULE(aLogModule, ("   - max:  %i", max));
    LOG_MODULE(aLogModule, ("   - step: %i", step));
  }
  else {
    PRUint32 valueCount = 0;
    rv = aRange->GetValueCount(&valueCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < valueCount; i++) {
      PRInt32 curValue = 0;
      rv = aRange->GetValue(i, &curValue);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG_MODULE(aLogModule, ("   - %i", curValue));
    }
  }

  return NS_OK;
}

#endif

bool sbDeviceUtils::ShouldLogDeviceInfo()
{
  const char * const DEVICE_PREF_BRANCH = "songbird.device.";
  const char * const LOG_DEVICE_INFO_PREF = "log_device_info";

  nsresult rv;
  sbPrefBranch prefBranch(DEVICE_PREF_BRANCH, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return prefBranch.GetBoolPref(LOG_DEVICE_INFO_PREF, PR_FALSE) != PR_FALSE;
}

nsCString sbDeviceUtils::GetDeviceIdentifier(sbIDevice * aDevice)
{
  nsresult rv;

  if (!aDevice) {
    return NS_LITERAL_CSTRING("Device Unknown");
  }
  nsCString result;

  nsString deviceIdentifier;
  rv = aDevice->GetName(deviceIdentifier);
  if (NS_FAILED(rv)) {
    deviceIdentifier.Truncate();
  }
  // Convert to ascii since this is going to be output to an ascii device
  result = NS_LossyConvertUTF16toASCII(deviceIdentifier);
  nsID *deviceID;
  rv = aDevice->GetId(&deviceID);
  sbAutoNSMemPtr autoDeviceID(deviceID);
  NS_ENSURE_SUCCESS(rv, result);

  char volumeGUID[NSID_LENGTH];
  deviceID->ToProvidedString(volumeGUID);

  if (!result.IsEmpty()) {
    result.Append(NS_LITERAL_CSTRING("-"));
  }
  result.Append(volumeGUID);

  return result;
}
