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

#include <nsArrayUtils.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCRT.h>
#include <nsComponentManagerUtils.h>
#include <nsIMutableArray.h>
#include <nsIFile.h>
#include <nsISimpleEnumerator.h>
#include <nsIURI.h>
#include <nsIURL.h>

#include "sbBaseDevice.h"
#include "sbIDeviceContent.h"
#include "sbIDeviceHelper.h"
#include "sbIDeviceLibrary.h"
#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbIMediaListListener.h"
#include <sbIPrompter.h>
#include "sbIWindowWatcher.h"
#include "sbLibraryUtils.h"
#include "sbStandardProperties.h"
#include "sbStringUtils.h"
#include <sbVariantUtils.h>

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
                                                  const nsAString& aValue)
  : mId(aId),
    mValue(aValue)
  {}

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnEnumerationBegin(sbIMediaList *aMediaList, PRUint16 *_retval) {
      NS_ENSURE_ARG_POINTER(_retval);
      *_retval = sbIMediaListEnumerationListener::CONTINUE;
      return NS_OK;
  }

  NS_IMETHODIMP OnEnumeratedItem(sbIMediaList *aMediaList, sbIMediaItem *aItem, PRUint16 *_retval) {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_ARG_POINTER(_retval);

    nsresult rv = aItem->SetProperty(mId, mValue);
    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return NS_OK;
  }

  NS_IMETHODIMP OnEnumerationEnd(sbIMediaList *aMediaList, nsresult aStatusCode) {
    return NS_OK;
  }

protected:
  nsString mId;
  nsString mValue;
  virtual ~sbDeviceUtilsBulkSetPropertyEnumerationListener() {}
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtilsBulkSetPropertyEnumerationListener,
                              sbIMediaListEnumerationListener);

/*static*/
nsresult sbDeviceUtils::BulkSetProperty(sbIMediaList *aMediaList,
                                        const nsAString& aPropertyId,
                                        const nsAString& aPropertyValue,
                                        sbIPropertyArray* aPropertyFilter)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsRefPtr<sbDeviceUtilsBulkSetPropertyEnumerationListener> listener =
    new sbDeviceUtilsBulkSetPropertyEnumerationListener(aPropertyId,
                                                        aPropertyValue);
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
  for (PRInt32 i = 0; i < length; i++) {
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

