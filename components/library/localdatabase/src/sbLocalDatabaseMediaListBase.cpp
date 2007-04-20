/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include "sbLocalDatabaseMediaListBase.h"

#include <nsIProperty.h>
#include <nsIPropertyBag.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <sbICascadeFilterSet.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIFilterableMediaList.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbISearchableMediaList.h>
#include <sbISortableMediaList.h>

#include <DatabaseQuery.h>
#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsHashKeys.h>
#include <nsMemory.h>
#include <sbSQLBuilderCID.h>
#include <sbTArrayStringEnumerator.h>
#include <sbPropertiesCID.h>
#include "sbLocalDatabaseCascadeFilterSet.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabasePropertyCache.h"

NS_IMPL_ADDREF(sbLocalDatabaseMediaListBase)
NS_IMPL_RELEASE(sbLocalDatabaseMediaListBase)

NS_INTERFACE_MAP_BEGIN(sbLocalDatabaseMediaListBase)
  NS_INTERFACE_MAP_ENTRY(sbIMediaList)
NS_INTERFACE_MAP_END_INHERITING(sbLocalDatabaseMediaItem)

sbLocalDatabaseMediaListBase::sbLocalDatabaseMediaListBase()
: mFullArrayMonitor(nsnull),
  mLockedEnumerationActive(PR_FALSE)
{
}

sbLocalDatabaseMediaListBase::~sbLocalDatabaseMediaListBase()
{
  if (mFullArrayMonitor) {
    nsAutoMonitor::DestroyMonitor(mFullArrayMonitor);
  }
}

nsresult
sbLocalDatabaseMediaListBase::Init(sbILocalDatabaseLibrary* aLibrary,
                                   const nsAString& aGuid)
{
  mFullArrayMonitor =
    nsAutoMonitor::NewMonitor("sbLocalDatabaseMediaListBase::mFullArrayMonitor");
  NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_OUT_OF_MEMORY);

  // Initialize our base classes
  nsresult rv = sbLocalDatabaseMediaListListener::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbLocalDatabaseMediaItem::Init(aLibrary, aGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Adds multiple filters to a GUID array.
 *
 * This method enumerates a hash table and calls AddFilter on a GUIDArray once 
 * for each key. It constructs a string enumerator for the string array that
 * the hash table contains.
 *
 * This method expects to be handed an sbILocalDatabaseGUIDArray pointer as its
 * aUserData parameter.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListBase::AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                                           sbStringArray* aEntry,
                                                           void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");
  
  // Make a string enumerator for the string array.
  nsCOMPtr<nsIStringEnumerator> valueEnum =
    new sbTArrayStringEnumerator(aEntry);
  
  // If we failed then we're probably out of memory. Hope we do better on the
  // next key?
  NS_ENSURE_TRUE(valueEnum, PL_DHASH_NEXT);

  // Unbox the guidArray.
  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray =
    NS_STATIC_CAST(sbILocalDatabaseGUIDArray*, aUserData);
  
  // Set the filter.
  nsresult rv = guidArray->AddFilter(aKey, valueEnum, PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "AddFilter failed!");

  return PL_DHASH_NEXT;
}

/**
 * Internal method that may be inside the monitor.
 */
nsresult
sbLocalDatabaseMediaListBase::EnumerateAllItemsInternal(sbIMediaListEnumerationListener* aEnumerationListener)
{
  sbGUIDArrayEnumerator enumerator(mLibrary, mFullArray);
  return EnumerateItemsInternal(&enumerator, aEnumerationListener);
}

/**
 * Internal method that may be inside the monitor.
 */
nsresult
sbLocalDatabaseMediaListBase::EnumerateItemsByPropertyInternal(const nsAString& aName,
                                                               nsIStringEnumerator* aValueEnum,
                                                               sbIMediaListEnumerationListener* aEnumerationListener)
{
  // Make a new GUID array to talk to the database.
  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray;
  nsresult rv = mFullArray->Clone(getter_AddRefs(guidArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // Clone copies the filters... which we don't want.
  rv = guidArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the filter.
  rv = guidArray->AddFilter(aName, aValueEnum, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And make an enumerator to return the filtered items.
  sbGUIDArrayEnumerator enumerator(mLibrary, guidArray);

  return EnumerateItemsInternal(&enumerator, aEnumerationListener);
}

/**
 * Internal method that may be inside the monitor.
 */
nsresult
sbLocalDatabaseMediaListBase::EnumerateItemsByPropertiesInternal(sbStringArrayHash* aPropertiesHash,
                                                                 sbIMediaListEnumerationListener* aEnumerationListener)
{
  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray;
  nsresult rv = mFullArray->Clone(getter_AddRefs(guidArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // Clone copies the filters... which we don't want.
  rv = guidArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  // Now that our hash table is set up we call AddFilter for each property
  // name and all its associated values.
  PRUint32 filterCount =
    aPropertiesHash->EnumerateRead(AddFilterToGUIDArrayCallback, guidArray);
  
  // Make sure we actually added some filters here. Otherwise something went
  // wrong and the results are not going to be what the caller expects.
  PRUint32 hashCount = aPropertiesHash->Count();
  NS_ENSURE_TRUE(filterCount == hashCount, NS_ERROR_UNEXPECTED);

  // Finally make an enumerator to return the filtered items.
  sbGUIDArrayEnumerator enumerator(mLibrary, guidArray);
  return EnumerateItemsInternal(&enumerator, aEnumerationListener);
}

nsresult
sbLocalDatabaseMediaListBase::MakeStandardQuery(sbIDatabaseQuery** _retval)
{
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseLocation;
  rv = mLibrary->GetDatabaseLocation(getter_AddRefs(databaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  if (databaseLocation) {
    rv = query->SetDatabaseLocation(databaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

/**
 * \brief Enumerates the items to the given listener.
 */
nsresult
sbLocalDatabaseMediaListBase::EnumerateItemsInternal(sbGUIDArrayEnumerator* aEnumerator,
                                                     sbIMediaListEnumerationListener* aListener)
{
  // Loop until we explicitly return.
  while (PR_TRUE) {

    PRBool hasMore;
    nsresult rv = aEnumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!hasMore) {
      return NS_OK;
    }
    
    nsCOMPtr<sbIMediaItem> item;
    rv = aEnumerator->GetNext(getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool continueEnumerating;
    rv = aListener->OnEnumeratedItem(this, item, &continueEnumerating);
    NS_ENSURE_SUCCESS(rv, rv);

    // Stop enumerating if the listener requested it.
    if (NS_SUCCEEDED(rv) && !continueEnumerating) {
      return NS_ERROR_ABORT;
    }
  }

  NS_NOTREACHED("Uh, how'd we get here?");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetName(nsAString& aName)
{
  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#mediaListName"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetName(const nsAString& aName)
{
  nsresult rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"), aName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
  nsAutoMonitor mon(mFullArrayMonitor);

  nsresult rv = mFullArray->GetLength(aLength);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemByGuid(const nsAString& aGuid,
                                            sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(aGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemByIndex(PRUint32 aIndex,
                                             sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString guid;
  {
    NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
    nsAutoMonitor mon(mFullArrayMonitor);

    rv = mFullArray->GetByIndex(aIndex, guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(guid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::EnumerateAllItems(sbIMediaListEnumerationListener* aEnumerationListener,
                                                PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  nsresult rv;

  switch (aEnumerationType) {

    case sbIMediaList::ENUMERATIONTYPE_LOCKING: {
      NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
      nsAutoMonitor mon(mFullArrayMonitor);

      // Don't reenter!
      NS_ENSURE_FALSE(mLockedEnumerationActive, NS_ERROR_FAILURE);
      mLockedEnumerationActive = PR_TRUE;

      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateAllItemsInternal(aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }

      mLockedEnumerationActive = PR_FALSE;

    } break; // ENUMERATIONTYPE_LOCKING

    case sbIMediaList::ENUMERATIONTYPE_SNAPSHOT: {
      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateAllItemsInternal(aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }
    } break; // ENUMERATIONTYPE_SNAPSHOT

    default: {
      NS_NOTREACHED("Invalid enumeration type");
      rv = NS_ERROR_INVALID_ARG;
    } break;
  }

  aEnumerationListener->OnEnumerationEnd(this, rv);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::EnumerateItemsByProperty(const nsAString& aName,
                                                       const nsAString& aValue,
                                                       sbIMediaListEnumerationListener* aEnumerationListener,
                                                       PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  // A property name must be specified.
  NS_ENSURE_TRUE(!aName.IsEmpty(), NS_ERROR_INVALID_ARG);

  // Make a single-item string array to hold our property value.
  sbStringArray valueArray(1);
  nsString* value = valueArray.AppendElement();
  NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

  value->Assign(aValue);

  // Make a string enumerator for it.
  nsCOMPtr<nsIStringEnumerator> valueEnum =
    new sbTArrayStringEnumerator(&valueArray);
  NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  switch (aEnumerationType) {

    case sbIMediaList::ENUMERATIONTYPE_LOCKING: {
      NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
      nsAutoMonitor mon(mFullArrayMonitor);

      // Don't reenter!
      NS_ENSURE_FALSE(mLockedEnumerationActive, NS_ERROR_FAILURE);
      mLockedEnumerationActive = PR_TRUE;

      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateItemsByPropertyInternal(aName, valueEnum,
                                                aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }

      mLockedEnumerationActive = PR_FALSE;

    } break; // ENUMERATIONTYPE_LOCKING

    case sbIMediaList::ENUMERATIONTYPE_SNAPSHOT: {
      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateItemsByPropertyInternal(aName, valueEnum,
                                                aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }
    } break; // ENUMERATIONTYPE_SNAPSHOT

    default: {
      NS_NOTREACHED("Invalid enumeration type");
      rv = NS_ERROR_INVALID_ARG;
    } break;
  }

  aEnumerationListener->OnEnumerationEnd(this, rv);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::EnumerateItemsByProperties(sbIPropertyArray* aProperties,
                                                         sbIMediaListEnumerationListener* aEnumerationListener,
                                                         PRUint16 aEnumerationType)
{
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aEnumerationListener);

  PRUint32 propertyCount;
  nsresult rv = aProperties->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // It doesn't make sense to call this method without specifying any properties
  // so it is probably a caller error if we have none.
  NS_ENSURE_STATE(propertyCount);

  // The guidArray needs AddFilter called only once per property with an
  // enumerator that contains all the values. We were given an array of
  // name/value pairs, so this is a little tricky. We make a hash table that
  // uses the property name for a key and an array of values as its data. Then
  // we load the arrays in a loop and finally call AddFilter as an enumeration
  // function.
  
  sbStringArrayHash propertyHash;
  
  // Init with the propertyCount as the number of buckets to create. This will
  // probably be too many, but it's likely less than the default of 16.
  propertyHash.Init(propertyCount);

  // Load the hash table with properties from the array.
  for (PRUint32 index = 0; index < propertyCount; index++) {

    // Get the property.
    nsCOMPtr<nsIProperty> property;
    rv = aProperties->GetPropertyAt(index, getter_AddRefs(property));
    SB_CONTINUE_IF_FAILED(rv);

    // Get the value.
    nsCOMPtr<nsIVariant> value;
    rv = property->GetValue(getter_AddRefs(value));
    SB_CONTINUE_IF_FAILED(rv);

    // Make sure the value is a string type, otherwise bail.
    PRUint16 dataType;
    rv = value->GetDataType(&dataType);
    SB_CONTINUE_IF_FAILED(rv);

    if (dataType != nsIDataType::VTYPE_ASTRING) {
      NS_WARNING("Wrong data type passed in a properties array!");
      continue;
    }

    // Get the name of the property. This will be the key for the hash table.
    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    SB_CONTINUE_IF_FAILED(rv);

    // Get the string array associated with the key. If it doesn't yet exist
    // then we need to create it.
    sbStringArray* stringArray;
    PRBool arrayExists = propertyHash.Get(propertyName, &stringArray);
    if (!arrayExists) {
      NS_NEWXPCOM(stringArray, sbStringArray);
      SB_CONTINUE_IF_FALSE(stringArray);

      // Try to add the array to the hash table.
      PRBool success = propertyHash.Put(propertyName, stringArray);
      if (!success) {
        NS_WARNING("Failed to add string array to property hash!");
        
        // Make sure to delete the new array, otherwise it will leak.
        NS_DELETEXPCOM(stringArray);
        continue;
      }
    }
    NS_ASSERTION(stringArray, "Must have a valid pointer here!");

    // Now we need a slot for the property value.
    nsString* valueString = stringArray->AppendElement();
    SB_CONTINUE_IF_FALSE(valueString);

    // And finally assign it.
    rv = value->GetAsAString(*valueString);
    SB_CONTINUE_IF_FAILED(rv);
  }

  switch (aEnumerationType) {

    case sbIMediaList::ENUMERATIONTYPE_LOCKING: {
      NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
      nsAutoMonitor mon(mFullArrayMonitor);

      // Don't reenter!
      NS_ENSURE_FALSE(mLockedEnumerationActive, NS_ERROR_FAILURE);
      mLockedEnumerationActive = PR_TRUE;

      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateItemsByPropertiesInternal(&propertyHash,
                                                  aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }

      mLockedEnumerationActive = PR_FALSE;

    } break; // ENUMERATIONTYPE_LOCKING

    case sbIMediaList::ENUMERATIONTYPE_SNAPSHOT: {
      PRBool beginEnumeration;
      rv = aEnumerationListener->OnEnumerationBegin(this, &beginEnumeration);

      if (NS_SUCCEEDED(rv)) {
        if (beginEnumeration) {
          rv = EnumerateItemsByPropertiesInternal(&propertyHash,
                                                  aEnumerationListener);
        }
        else {
          // The user cancelled the enumeration.
          rv = NS_ERROR_ABORT;
        }
      }
    } break; // ENUMERATIONTYPE_SNAPSHOT

    default: {
      NS_NOTREACHED("Invalid enumeration type");
      rv = NS_ERROR_INVALID_ARG;
    } break;
  }

  aEnumerationListener->OnEnumerationEnd(this, rv);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::IndexOf(sbIMediaItem* aMediaItem,
                                      PRUint32 aStartFrom,
                                      PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 count;

  NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
  nsAutoMonitor mon(mFullArrayMonitor);

  nsresult rv = mFullArray->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  // Do some sanity checking.
  NS_ENSURE_TRUE(count > 0, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_MAX(aStartFrom, count - 1);

  nsAutoString testGUID;
  rv = aMediaItem->GetGuid(testGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = aStartFrom; index < count; index++) {
    nsAutoString itemGUID;
    rv = mFullArray->GetByIndex(index, itemGUID);
    SB_CONTINUE_IF_FAILED(rv);

    if (testGUID.Equals(itemGUID)) {
      *_retval = index;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::LastIndexOf(sbIMediaItem* aMediaItem,
                                          PRUint32 aStartFrom,
                                          PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
  nsAutoMonitor mon(mFullArrayMonitor);

  PRUint32 count;
  nsresult rv = mFullArray->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  // Do some sanity checking.
  NS_ENSURE_TRUE(count > 0, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_MAX(aStartFrom, count - 1);

  nsAutoString testGUID;
  rv = aMediaItem->GetGuid(testGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = count - 1; index >= aStartFrom; index--) {
    nsAutoString itemGUID;
    rv = mFullArray->GetByIndex(index, itemGUID);
    SB_CONTINUE_IF_FAILED(rv);

    if (testGUID.Equals(itemGUID)) {
      *_retval = index;
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Contains(sbIMediaItem* aMediaItem,
                                       PRBool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetIsEmpty(PRBool* aIsEmpty)
{
  NS_ENSURE_ARG_POINTER(aIsEmpty);

  NS_ENSURE_TRUE(mFullArrayMonitor, NS_ERROR_FAILURE);
  nsAutoMonitor mon(mFullArrayMonitor);

  PRUint32 length;
  nsresult rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsEmpty = length == 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Add(sbIMediaItem* aMediaItem)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::AddAll(sbIMediaList* aMediaList)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Remove(sbIMediaItem* aMediaItem)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::RemoveByIndex(PRUint32 aIndex)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::RemoveSome(nsISimpleEnumerator* aMediaItems)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Clear()
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::AddListener(sbIMediaListListener* aListener)
{
  return sbLocalDatabaseMediaListListener::AddListener(aListener);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::RemoveListener(sbIMediaListListener* aListener)
{
  return sbLocalDatabaseMediaListListener::RemoveListener(aListener);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::CreateView(sbIMediaListView** _retval)
{
  NS_NOTREACHED("Not meant to be implemented in this base class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::BeginUpdateBatch()
{
  sbLocalDatabaseMediaListListener::NotifyListenersBatchBegin(this);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::EndUpdateBatch()
{
  sbLocalDatabaseMediaListListener::NotifyListenersBatchEnd(this);
  return NS_OK;
}

