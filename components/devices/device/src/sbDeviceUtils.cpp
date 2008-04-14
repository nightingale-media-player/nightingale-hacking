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
#include "sbIDeviceLibrary.h"
#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbIMediaListListener.h"
#include "sbStandardProperties.h"
#include "sbStringUtils.h"

/*static*/
nsresult sbDeviceUtils::GetOrganizedPath(/* in */ nsIFile *aParent,
                                         /* in */ sbIMediaItem *aItem,
                                         nsIFile **_retval)
{
  NS_ENSURE_ARG_POINTER(aParent);
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  
  nsString kIllegalChars = NS_LITERAL_STRING(FILE_ILLEGAL_CHARACTERS);
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
nsresult sbDeviceUtils::DeleteUnavailableItems(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  nsRefPtr<sbDeviceUtils::SnapshotEnumerationListener> listener =
    new sbDeviceUtils::SnapshotEnumerationListener();
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = listener->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aMediaList->EnumerateItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                                            NS_LITERAL_STRING("0"),
                                            listener, sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> array;
  rv = listener->GetArray(getter_AddRefs(array));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = array->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  return aMediaList->RemoveSome(enumerator);
}

/*static*/ 
nsresult sbDeviceUtils::CreateStatusFromRequest(/* in */ const nsAString &aDeviceID,
                                                /* in */ sbBaseDevice::TransferRequest *aRequest,
                                                /* out */ sbDeviceStatus **aStatus)
{
  NS_ENSURE_ARG_POINTER(aRequest);
  NS_ENSURE_ARG_POINTER(aStatus);

  nsRefPtr<sbDeviceStatus> status = sbDeviceStatus::New(aDeviceID);
  NS_ENSURE_TRUE(status, NS_ERROR_OUT_OF_MEMORY);

  switch(aRequest->type) {
    /* read requests */
    case sbIDevice::REQUEST_MOUNT:
      status->CurrentOperation(NS_LITERAL_STRING("mounting"));
    break;
    
    case sbIDevice::REQUEST_READ:
      status->CurrentOperation(NS_LITERAL_STRING("reading"));
    break;

    case sbIDevice::REQUEST_EJECT:
      status->CurrentOperation(NS_LITERAL_STRING("ejecting"));
    break;

    case sbIDevice::REQUEST_SUSPEND:
      status->CurrentOperation(NS_LITERAL_STRING("suspending"));
    break;

    /* write requests */
    case sbIDevice::REQUEST_WRITE:
      status->CurrentOperation(NS_LITERAL_STRING("copying"));
    break;
    
    case sbIDevice::REQUEST_DELETE:
      status->CurrentOperation(NS_LITERAL_STRING("deleting"));
    break;

    case sbIDevice::REQUEST_SYNC:
      status->CurrentOperation(NS_LITERAL_STRING("syncing"));
    break;

    /* delete all files */
    case sbIDevice::REQUEST_WIPE:
      status->CurrentOperation(NS_LITERAL_STRING("wiping"));
    break;

    /* move an item in one playlist */
    case sbIDevice::REQUEST_MOVE:
      status->CurrentOperation(NS_LITERAL_STRING("moving"));
    break;
    case sbIDevice::REQUEST_UPDATE:
      status->CurrentOperation(NS_LITERAL_STRING("updating"));
    break;
   
    case sbIDevice::REQUEST_NEW_PLAYLIST:
      status->CurrentOperation(NS_LITERAL_STRING("creating_playlist"));
    break;

    default:
      return NS_ERROR_FAILURE;
  }

  status.forget(aStatus);

  return NS_OK;
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

NS_IMETHODIMP sbDeviceUtils::SnapshotEnumerationListener
                           ::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CANCEL;
  
  // clear the list, so we can re-use the enumerator if appropriate
  nsresult rv = mArray->Clear();
  NS_ENSURE_SUCCESS(rv, rv);
  
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP sbDeviceUtils::SnapshotEnumerationListener
                           ::OnEnumeratedItem(sbIMediaList *aMediaList,
                                              sbIMediaItem *aItem,
                                              PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv = mArray->AppendElement(aItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP sbDeviceUtils::SnapshotEnumerationListener
                           ::OnEnumerationEnd(sbIMediaList *aMediaList,
                                              nsresult aStatusCode)
{
  NS_ENSURE_SUCCESS(aStatusCode, aStatusCode);
  return NS_OK;
}

nsresult sbDeviceUtils::SnapshotEnumerationListener::Init()
{
  nsresult rv;
  mArray = do_CreateInstance("@mozilla.org/array;1", &rv);
  return rv;
}

nsresult sbDeviceUtils::SnapshotEnumerationListener::GetArray(nsIArray **aArray)
{
  NS_ENSURE_ARG_POINTER(aArray);
  return CallQueryInterface(mArray, aArray);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtils::SnapshotEnumerationListener,
                              sbIMediaListEnumerationListener);
