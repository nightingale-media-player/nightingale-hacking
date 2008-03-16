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

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCRT.h>
#include <nsComponentManagerUtils.h>
#include <nsIMutableArray.h>
#include <nsIFile.h>
#include <nsISimpleEnumerator.h>
#include <nsIURI.h>
#include <nsIURL.h>

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
 *        marks all items in a library unavailable.
 */
class sbDeviceUtilsMarkUnavailableEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnEnumerationBegin(sbIMediaList *aMediaList, PRUint16 *_retval) {
      NS_ENSURE_ARG_POINTER(_retval);
      *_retval = sbIMediaListEnumerationListener::CONTINUE;
      return NS_OK;
  }

  NS_IMETHODIMP OnEnumeratedItem(sbIMediaList *aMediaList, sbIMediaItem *aItem, PRUint16 *_retval) {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_ARG_POINTER(_retval);

    nsresult rv = aItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY), 
                                     NS_LITERAL_STRING("0"));
    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return NS_OK;
  }

  NS_IMETHODIMP OnEnumerationEnd(sbIMediaList *aMediaList, nsresult aStatusCode) {
    return NS_OK;
  }

protected:
  virtual ~sbDeviceUtilsMarkUnavailableEnumerationListener() {};
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtilsMarkUnavailableEnumerationListener,
                              sbIMediaListEnumerationListener);

/*static*/
nsresult sbDeviceUtils::MarkAllItemsUnavailable(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsRefPtr<sbDeviceUtilsMarkUnavailableEnumerationListener> listener;
  NS_NEWXPCOM(listener, sbDeviceUtilsMarkUnavailableEnumerationListener);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  return aMediaList->EnumerateAllItems(listener, sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
}


class sbDeviceUtilsDeleteUnavailableEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS

    NS_IMETHODIMP OnEnumerationBegin(sbIMediaList *aMediaList, PRUint16 *_retval) {
      NS_ENSURE_ARG_POINTER(_retval);
      *_retval = sbIMediaListEnumerationListener::CONTINUE;
      return NS_OK;
  }

  NS_IMETHODIMP OnEnumeratedItem(sbIMediaList *aMediaList, sbIMediaItem *aItem, PRUint16 *_retval) {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_ARG_POINTER(_retval);
    
    nsresult rv = mArray->AppendElement(aItem, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = sbIMediaListEnumerationListener::CONTINUE;

    return NS_OK;
  }

  NS_IMETHODIMP OnEnumerationEnd(sbIMediaList *aMediaList, nsresult aStatusCode) {
    return NS_OK;
  }

  nsresult Init() {
    nsresult rv;
    mArray = do_CreateInstance("@mozilla.org/array;1", &rv);
    return rv;
  }

  nsresult GetArray(nsIArray **aArray) {
    NS_ENSURE_ARG_POINTER(aArray);
    return CallQueryInterface(mArray, aArray);
  }

protected:
  virtual ~sbDeviceUtilsDeleteUnavailableEnumerationListener() {};
  nsCOMPtr<nsIMutableArray> mArray;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceUtilsDeleteUnavailableEnumerationListener,
                              sbIMediaListEnumerationListener);

/*static*/
nsresult sbDeviceUtils::DeleteUnavailableItems(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  nsRefPtr<sbDeviceUtilsDeleteUnavailableEnumerationListener> listener;
  NS_NEWXPCOM(listener, sbDeviceUtilsDeleteUnavailableEnumerationListener);
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
