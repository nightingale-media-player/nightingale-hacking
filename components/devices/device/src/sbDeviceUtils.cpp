/* vim: set sw=2 :miv */
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

#include "sbDeviceUtils.h"

#include <algorithm>

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
#include <nsISupportsArray.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIWindowWatcher.h>
#include <nsThreadUtils.h>

#include "sbBaseDevice.h"
#include "sbIDeviceCapabilities.h"
#include "sbIDeviceContent.h"
#include "sbIDeviceErrorMonitor.h"
#include "sbIDeviceHelper.h"
#include "sbIDeviceLibrary.h"
#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbIMediaListListener.h"
#include <sbITranscodeProfile.h>
#include <sbITranscodeManager.h>
#include <sbIPrompter.h>
#include "sbIWindowWatcher.h"
#include "sbLibraryUtils.h"
#include <sbProxiedComponentManager.h>
#include "sbStandardProperties.h"
#include "sbStringUtils.h"
#include "sbProxyUtils.h"
#include <sbVariantUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbMemoryUtils.h>
#include <sbArray.h>
#include <sbIMediaInspector.h>

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceUtilsLog = NULL;
#define LOG(args) \
  PR_BEGIN_MACRO \
  if (!gDeviceUtilsLog) \
    gDeviceUtilsLog = PR_NewLogModule("deviceutils"); \
  PR_LOG(gDeviceUtilsLog, PR_LOG_WARN, args); \
  PR_END_MACRO
#define TRACE(args) \
  PR_BEGIN_MACRO \
  if (!gDeviceUtilsLog) \
    gDeviceUtilsLog = PR_NewLogModule("deviceutils"); \
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
    rv = ownerLibrary->Equals(deviceLib, &equalsLibrary);
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
  rv = errMonitor->DeviceHasErrors(aDevice, EmptyString(), &hasErrors);
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
nsresult sbDeviceUtils::SyncCheckLinkedPartner(sbIDevice* aDevice,
                                               PRBool     aRequestPartnerChange,
                                               PRBool*    aIsLinkedLocally)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aIsLinkedLocally);

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

  // If device is not linked locally, request that its sync partner be changed.
  if (!isLinkedLocally && aRequestPartnerChange) {
    // Request that the device sync partner be changed.
    PRBool partnerChangeGranted;
    rv = SyncRequestPartnerChange(aDevice, &partnerChangeGranted);
    NS_ENSURE_SUCCESS(rv, rv);

    // Change the sync partner if the request was granted.
    if (partnerChangeGranted) {
      rv = aDevice->SetPreference(NS_LITERAL_STRING("SyncPartner"),
                                  sbNewVariant(localSyncPartnerID));
      NS_ENSURE_SUCCESS(rv, rv);
      isLinkedLocally = PR_TRUE;
    }
  }

  // Return results.
  *aIsLinkedLocally = isLinkedLocally;

  return NS_OK;
}

/* static */
nsresult sbDeviceUtils::SyncRequestPartnerChange
                          (sbIDevice* aDevice,
                           PRBool*    aPartnerChangeGranted)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aPartnerChangeGranted);

  // Function variables.
  nsresult rv;

  // Get the device name.
  nsString deviceName;
  rv = aDevice->GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the main library name.
  nsCOMPtr<sbILibrary> mainLibrary;
  nsString             libraryName;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mainLibrary->GetName(libraryName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a prompter that waits for a window.
  nsCOMPtr<sbIPrompter> prompter =
                          do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ensure that the library name is not empty.
  if (libraryName.IsEmpty()) {
    libraryName = SBLocalizedString("servicesource.library");
  }

  // Get the prompt title.
  nsAString const& title =
    SBLocalizedString("device.dialog.sync_confirmation.change_library.title");

  // Get the prompt message.
  nsTArray<nsString> formatParams;
  formatParams.AppendElement(deviceName);
  nsAString const& message =
    SBLocalizedString("device.dialog.sync_confirmation.change_library.msg",
                      formatParams);

  // Configure the buttons.
  PRUint32 buttonFlags = 0;

  // Configure the no button as button 1.
  nsAString const& noButton =
    SBLocalizedString
      ("device.dialog.sync_confirmation.change_library.no_button");
  buttonFlags += (nsIPromptService::BUTTON_POS_1 *
                  nsIPromptService::BUTTON_TITLE_IS_STRING);

  // Configure the sync button as button 0.
  nsAString const& syncButton =
    SBLocalizedString
      ("device.dialog.sync_confirmation.change_library.sync_button");
  buttonFlags += (nsIPromptService::BUTTON_POS_0 *
                  nsIPromptService::BUTTON_TITLE_IS_STRING) +
                 nsIPromptService::BUTTON_POS_0_DEFAULT;
  PRInt32 grantPartnerChangeIndex = 0;

  // XXX lone> see mozbug 345067, there is no way to tell the prompt service
  // what code to return when the titlebar's X close button is clicked, it is
  // always 1, so we have to make the No button the second button.

  // Query the user to determine whether the device sync partner should be
  // changed to the local partner.
  PRInt32 buttonPressed;
  rv = prompter->ConfirmEx(nsnull,
                           title.BeginReading(),
                           message.BeginReading(),
                           buttonFlags,
                           syncButton.BeginReading(),
                           noButton.BeginReading(),
                           nsnull,                      // No button 2.
                           nsnull,                      // No check message.
                           nsnull,                      // No check result.
                           &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if partner change request was granted.
  if (buttonPressed == grantPartnerChangeIndex)
    *aPartnerChangeGranted = PR_TRUE;
  else
    *aPartnerChangeGranted = PR_FALSE;

  return NS_OK;
}

SB_AUTO_CLASS(sbAutoNSMemoryPtr, void*, !!mValue, nsMemory::Free(mValue), mValue = nsnull);

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
  { "mp3",  "audio/mpeg",      "id3",  "mp3",    sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wma",  "audio/x-ms-wma",  "asf",  "wmav2",  sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aac",  "audio/aac",       "mov",  "aac",    sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "m4a",  "audio/aac",       "mov",  "aac",    sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aa",   "audio/audible",   "",     "",       sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "oga",  "application/ogg", "ogg",  "flac",   sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "ogg",  "application/ogg", "ogg",  "vorbis", sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "flac", "audio/x-flac",    "",     "flac",   sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wav",  "audio/x-wav",     "wav",  "pcm-int",sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aiff", "audio/x-aiff",    "aiff", "pcm-int",sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aif",  "audio/x-aiff",    "aiff", "pcm-int",sbIDeviceCapabilities::CONTENT_AUDIO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },

  /* video */
  { "mp4",  "video/mp4",       "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mpg",  "video/mpeg",      "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "mpeg", "video/mpeg",      "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "wmv",  "video/x-ms-wmv",  "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "avi",  "video/x-msvideo", "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "3gp",  "video/3gpp",      "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },
  { "3g2",  "video/3gpp",      "",    "",      sbIDeviceCapabilities::CONTENT_VIDEO, sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO },

  /* images */
  { "png",  "image/png",      "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jpg",  "image/jpeg",     "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "gif",  "image/gif",      "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "bmp",  "image/bmp",      "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "ico",  "image/x-icon",   "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "tiff", "image/tiff",     "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "tif",  "image/tiff",     "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "wmf",  "application/x-msmetafile", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jp2",  "image/jp2",      "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jpx",  "image/jpx",      "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "fpx",  "application/vnd.netfpx", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "pcd",  "image/x-photo-cd", "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "pict", "image/pict",     "", "", sbIDeviceCapabilities::CONTENT_IMAGE, sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },

  /* playlists */
  { "m3u",  "audio/x-mpegurl", "", "", sbIDeviceCapabilities::CONTENT_PLAYLIST, sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN }
};

PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH =
  NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);

const PRUint32 K = 1000;

/**
 * Convert the string bit rate to an integer
 * \param aBitRate the string version of the bit rate
 * \return the integer version of the bit rate or 0 if it fails
 */
static PRUint32 ParseBitRate(nsAString const & aBitRate)
{
  TRACE(("%s: %s", __FUNCTION__, NS_LossyConvertUTF16toASCII(aBitRate).get()));
  nsresult rv;

  if (aBitRate.IsEmpty()) {
    return 0;
  }
  PRUint32 rate = aBitRate.ToInteger(&rv, 10);
  if (NS_FAILED(rv)) {
    rate = 0;
  }
  return rate * K;
}

/**
 * Convert the string sample rate to an integer
 * \param aSampleRate the string version of the sample rate
 * \return the integer version of the sample rate or 0 if it fails
 */
static PRUint32 ParseSampleRate(nsAString const & aSampleRate) {
  TRACE(("%s: %s", __FUNCTION__, NS_LossyConvertUTF16toASCII(aSampleRate).get()));
  nsresult rv;
  if (aSampleRate.IsEmpty()) {
    return 0;
  }
  PRUint32 rate = aSampleRate.ToInteger(&rv, 10);
  if (NS_FAILED(rv)) {
    rate = 0;
  }
  return rate;
}

/**
 * Returns the formatting information for an item
 * \param aItem The item we want the format stuff for
 * \param aFormatType the formatting map entry for the item
 * \param aBitRate The bit rate for the item
 * \param aSampleRate the sample rate for the item
 */
nsresult
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
  aBitRate = ParseBitRate(bitRate);

  // Get the sample rate.
  nsString sampleRate;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                          sampleRate);
  NS_ENSURE_SUCCESS(rv, rv);
  aSampleRate = ParseSampleRate(sampleRate);

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
    for (PRUint32 index = 0;
         index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
         ++index) {
      sbExtensionToContentFormatEntry_t const & entry =
        MAP_FILE_EXTENSION_CONTENT_FORMAT[index];
      if (fileExtension.EqualsLiteral(entry.Extension)) {
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
 * \param aFormatType the formatting map entry for the MIME type
 */
/* static */ nsresult
sbDeviceUtils::GetFormatTypeForMimeType
                 (const nsAString&                   aMimeType,
                  sbExtensionToContentFormatEntry_t& aFormatType)
{
  TRACE(("%s", __FUNCTION__));

  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
       ++index) {
    sbExtensionToContentFormatEntry_t const & entry =
      MAP_FILE_EXTENSION_CONTENT_FORMAT[index];
    if (aMimeType.EqualsLiteral(entry.MimeType)) {
      TRACE(("%s: ext %s type %s container %s codec %s",
             __FUNCTION__, entry.Extension, entry.MimeType,
             entry.ContainerFormat, entry.Codec));
      aFormatType = entry;
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
      NS_WARNING("Unexpected content type in "
                 "sbDefaultBaseDeviceCapabilitiesRegistrar::"
                 "GetSupportedTranscodeProfiles");
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
sbDeviceUtils::GetSupportedTranscodeProfiles(sbIDevice * aDevice,
                                             nsIArray **aProfiles)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aProfiles);

  nsresult rv;

  nsCOMPtr<sbITranscodeManager> tcManager = do_ProxiedGetService(
          "@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> supportedProfiles = do_CreateInstance(
          SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> profiles;
  rv = tcManager->GetTranscodeProfiles(getter_AddRefs(profiles));
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
    PRUint32 formatsLength;
    char ** formats;
    rv = devCaps->GetSupportedFormats(contentType,
                                      &formatsLength,
                                      &formats);
    // Not found error is expected, we'll not do anything in that case, but we
    // need to finish out processing and not return early
    if (rv != NS_ERROR_NOT_AVAILABLE) {
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (NS_SUCCEEDED(rv)) {
      for (PRUint32 formatIndex = 0;
           formatIndex < formatsLength && NS_SUCCEEDED(rv);
           ++formatIndex)
      {
        nsString format;
        format.AssignLiteral(formats[formatIndex]);
        nsMemory::Free(formats[formatIndex]);

        nsCOMPtr<nsISupports> formatTypeSupports;
        devCaps->GetFormatType(format, getter_AddRefs(formatTypeSupports));
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
              rv = supportedProfiles->AppendElement(profile, PR_FALSE);
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }
        }
      }
      nsMemory::Free(formats);
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

  PRUint32 formatsLength;
  char ** formats;
  rv = devCaps->GetSupportedFormats(devCapContentType,
                                    &formatsLength,
                                    &formats);
  // If we know of transcoding formats than check them
  if (NS_SUCCEEDED(rv) && formatsLength > 0) {
    aNeedsTranscoding = true;
    for (PRUint32 formatIndex = 0;
         formatIndex < formatsLength;
         ++formatIndex) {

      NS_ConvertASCIItoUTF16 format(formats[formatIndex]);
      nsCOMPtr<nsISupports> formatType;
      rv = devCaps->GetFormatType(format, getter_AddRefs(formatType));
      if (NS_SUCCEEDED(rv)) {
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
          // Compare the various attributes, if bit rate and sample rate are
          // not specified then they always match
          if (containerFormat.Equals(itemContainerFormat) &&
              codec.Equals(itemCodec) &&
              (!aBitRate || IsValueInRange(aBitRate, bitRateRange)) &&
              (!aSampleRate || IsValueInRange(aSampleRate, sampleRateRange)))
          {
            TRACE(("%s: no transcoding needed, matches format %s "
                   "container %s codec %s",
                   __FUNCTION__, formats[formatIndex],
                   NS_LossyConvertUTF16toASCII(containerFormat).get(),
                   NS_LossyConvertUTF16toASCII(codec).get()));
            aNeedsTranscoding = false;
            break;
          }
        }
      }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(formatsLength, formats);
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

  nsCOMPtr<sbIMediaFormatContainer> container;
  rv =aMediaFormat->GetContainer(getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString containerType;
  rv = container->GetContainerType(containerType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaFormatVideo> formatVideo;
  rv = aMediaFormat->GetVideoStream(getter_AddRefs(formatVideo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get bitrate and ignore errors
  PRInt32 bitRate = 0;
  formatVideo->GetBitRate(&bitRate);

  nsCOMPtr<sbIMediaFormatAudio> formatAudio;
  rv = aMediaFormat->GetAudioStream(getter_AddRefs(formatAudio));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get sample rate and ignore errors
  PRInt32 sampleRate = 0;
  formatAudio->GetSampleRate(&sampleRate);

  nsString itemVideoType;
  rv = formatVideo->GetVideoType(itemVideoType);
  NS_ENSURE_SUCCESS(rv, rv);


  nsString itemAudioType;
  rv = formatAudio->GetAudioType(itemAudioType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 formatsLength;
  char ** formats;
  rv = devCaps->GetSupportedFormats(devCapContentType,
                                    &formatsLength,
                                    &formats);
  // If we know of transcoding formats than check them
  if (NS_SUCCEEDED(rv) && formatsLength > 0) {
    aNeedsTranscoding = true;
    for (PRUint32 formatIndex = 0;
         formatIndex < formatsLength;
         ++formatIndex) {

      NS_ConvertASCIItoUTF16 format(formats[formatIndex]);
      nsCOMPtr<nsISupports> formatType;
      rv = devCaps->GetFormatType(format, getter_AddRefs(formatType));
      if (NS_SUCCEEDED(rv)) {
        nsString containerFormat;
        nsString videoType;
        nsString audioType;
        nsString codec; // Not used

        nsCOMPtr<sbIDevCapRange> bitRateRange;
        nsCOMPtr<sbIDevCapRange> sampleRateRange;

        // TODO: XXX This will need to be improved as we finish out the
        // implementation of the media inspector
        rv = GetContainerFormatAndCodec(formatType,
                                        devCapContentType,
                                        containerFormat,
                                        videoType,
                                        audioType,
                                        codec,
                                        getter_AddRefs(bitRateRange),
                                        getter_AddRefs(sampleRateRange));
        if (NS_SUCCEEDED(rv)) {
          // Compare the various attributes, if bit rate and sample rate are
          // not specified then they always match
          if (containerFormat.Equals(containerType) &&
              videoType.Equals(itemVideoType) &&
              audioType.Equals(itemAudioType) &&
              (!bitRate || IsValueInRange(bitRate, bitRateRange)) &&
              (!sampleRate || IsValueInRange(sampleRate, sampleRateRange)))
          {
            TRACE(("%s: no transcoding needed, matches format %s "
                   "container %s codec %s",
                   __FUNCTION__, formats[formatIndex],
                   NS_LossyConvertUTF16toASCII(containerFormat).get(),
                   NS_LossyConvertUTF16toASCII(codec).get()));
            aNeedsTranscoding = false;
            break;
          }
        }
      }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(formatsLength, formats);
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
                                               nsCString &aCodec)
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

PRUint32
sbDeviceUtils::GetTranscodeType(sbIMediaItem * aMediaItem)
{
  nsresult rv;

  nsString contentType;
  rv = aMediaItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN);

  if (contentType.Equals(NS_LITERAL_STRING("audio"))) {
    return sbITranscodeProfile::TRANSCODE_TYPE_AUDIO;
  }
  else if (contentType.Equals(NS_LITERAL_STRING("video"))) {
    return sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO;
  }
  else if (contentType.Equals(NS_LITERAL_STRING("image"))) {
    return sbITranscodeProfile::TRANSCODE_TYPE_VIDEO;
  }
  NS_WARNING("sbDeviceUtils::GetTranscodeType: "
             "returning unknown transcoding type");
  return sbITranscodeProfile::TRANSCODE_TYPE_UNKNOWN;
}

PRUint32
sbDeviceUtils::GetDeviceCapsMediaType(sbIMediaItem * aMediaItem)
{
  nsresult rv;

  nsString contentType;
  rv = aMediaItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, sbIDeviceCapabilities::CONTENT_UNKNOWN);

  if (contentType.Equals(NS_LITERAL_STRING("audio"))) {
    return sbIDeviceCapabilities::CONTENT_AUDIO;
  }
  else if (contentType.Equals(NS_LITERAL_STRING("video"))) {
    return sbIDeviceCapabilities::CONTENT_VIDEO;
  }
  else if (contentType.Equals(NS_LITERAL_STRING("image"))) {
    return sbIDeviceCapabilities::CONTENT_IMAGE;
  }
  NS_WARNING("sbDeviceUtils::GetTranscodeType: "
             "returning unknown transcoding type");
  return sbIDeviceCapabilities::CONTENT_UNKNOWN;
};
