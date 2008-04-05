/* vim: set sw=2 : */
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

#include "sbLibraryUtils.h"

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsStringAPI.h>

#include <sbILibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <sbIPropertyArray.h>

#include <sbPropertiesCID.h>
#include "sbMediaListEnumSingleItemHelper.h"
#include <sbStandardProperties.h>

/* static */
nsresult sbLibraryUtils::GetItemInLibrary(/* in */  sbIMediaItem*  aItem,
                                          /* in */  sbILibrary*    aLibrary,
                                          /* out */ sbIMediaItem** _retval)
{
  /* this is an internal method, just assert/crash, don't be nice */
  NS_ASSERTION(aItem, "no item to look up!");
  NS_ASSERTION(aLibrary, "no library to search in!");
  NS_ASSERTION(_retval, "null return value pointer!");

  nsresult rv;

  nsCOMPtr<sbILibrary> itemLibrary;
  rv = aItem->GetLibrary(getter_AddRefs(itemLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  {
    PRBool isSameLib;
    rv = itemLibrary->Equals(aLibrary, &isSameLib);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isSameLib) {
      /* okay, so we want to find the same item... */
      *_retval = aItem;
      return NS_OK;
    }
  }
  

  // check if aItem originally was from aLibrary
  nsString originLibraryGuid, originItemGuid;
  nsString sourceLibraryGuid, sourceItemGuid;
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                            originLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                            originItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsString targetLibraryGuid;
    rv = aLibrary->GetGuid(targetLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    if (targetLibraryGuid.Equals(originLibraryGuid)) {
      // the media item originally came from the target library
      // find the media item with the matching guid
      rv = aLibrary->GetMediaItem(originItemGuid, _retval);
      if (NS_FAILED(rv)) {
        *_retval = nsnull;
      }
      return NS_OK;
    }
  }


  // check if some item in aLibrary was originally a copy of aItem
  {
    rv = itemLibrary->GetGuid(sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = aItem->GetGuid(sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                                   sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                   sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsRefPtr<sbMediaListEnumSingleItemHelper> helper =
      sbMediaListEnumSingleItemHelper::New();
    NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);
  
    rv = aLibrary->EnumerateItemsByProperties(propArray,
                                              helper,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIMediaItem> item = helper->GetItem();
    if (item) {
      item.forget(_retval);
      return NS_OK;
    }
  }
  
  
  // check if there is some item in aLibrary that was a copy of the same thing
  // that aItem was a copy of
  if (!originLibraryGuid.IsEmpty() && !originItemGuid.IsEmpty()) {
    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                                   sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                   sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsRefPtr<sbMediaListEnumSingleItemHelper> helper =
      sbMediaListEnumSingleItemHelper::New();
    NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);

    rv = aLibrary->EnumerateItemsByProperties(propArray,
                                              helper,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIMediaItem> item = helper->GetItem();
    if (item) {
      item.forget(_retval);
      return NS_OK;
    }
  }

  // give up
  *_retval = nsnull;
  return NS_OK;
}
