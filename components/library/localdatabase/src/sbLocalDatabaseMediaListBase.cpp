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

#include <nsIProgrammingLanguage.h>
#include <nsIProperty.h>
#include <nsIPropertyBag.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsIVariant.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>

#include <sbIMediaList.h>
#include <sbIPropertyArray.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabasePropertyCache.h"

#define DEFAULT_SORT_PROPERTY \
  NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#ordinal")
#define DEFAULT_FETCH_SIZE 1000

#define SB_CONTINUE_IF_FALSE(_expr)                        \
  PR_BEGIN_MACRO                                           \
    if (!(_expr)) {                                        \
      NS_WARNING("SB_CONTINUE_IF_FALSE triggered");        \
      continue;                                            \
    }                                                      \
  PR_END_MACRO

#define SB_CONTINUE_IF_FAILED(_rv)                         \
  SB_CONTINUE_IF_FALSE(NS_SUCCEEDED(_rv))

NS_IMPL_ISUPPORTS4(sbLocalDatabaseMediaListBase,
                   sbILibraryResource,
                   sbIMediaItem,
                   sbIMediaList,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLocalDatabaseMediaListBase,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbIMediaList)

sbLocalDatabaseMediaListBase::sbLocalDatabaseMediaListBase(sbILibrary* aLibrary,
                                                           const nsAString& aGuid) :
  mLibrary(aLibrary),
  mGuid(aGuid)
{
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetName(nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetName(const nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItems(nsISimpleEnumerator** aItems)
{
  nsCOMPtr<nsISimpleEnumerator> items =
    new sbGUIDArrayEnumerator(mLibrary, mFullArray);
  NS_ENSURE_TRUE(items, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aItems = items);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetLength(PRUint32* aLength)
{
  nsresult rv;
  rv = mFullArray->GetLength(aLength);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemByGuid(const nsAString& aGuid,
                                            sbIMediaItem** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemByIndex(PRUint32 aIndex,
                                             sbIMediaItem** _retval)
{
  nsresult rv;

  nsAutoString guid;
  rv = mFullArray->GetByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mLibrary->GetMediaItem(guid, getter_AddRefs(item));

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemsByPropertyValue(const nsAString& aName,
                                                      const nsAString& aValue,
                                                      nsISimpleEnumerator** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

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

  // Make a new GUID array to talk to the database.
  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray;
  nsresult rv = mFullArray->Clone(getter_AddRefs(guidArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // Clone copies the filters... which we don't want.
  rv = guidArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Set the filter.
  rv = guidArray->AddFilter(aName, valueEnum, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And make an enumerator to return the filtered items.
  nsCOMPtr<nsISimpleEnumerator> items =
    new sbGUIDArrayEnumerator(mLibrary, guidArray);
  NS_ENSURE_TRUE(items, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = items);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetItemsByPropertyValues(sbIPropertyArray* aProperties,
                                                       nsISimpleEnumerator** _retval)
{
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 propertyCount;
  nsresult rv = aProperties->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // It doesn't make sense to call this method without specifying any properties
  // so it is probably a caller error if we have none.
  NS_ENSURE_STATE(propertyCount);

  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray;
  rv = mFullArray->Clone(getter_AddRefs(guidArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // Clone copies the filters... which we don't want.
  rv = guidArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Now that our hash table is set up we call AddFilter for each property name
  // and all its associated values.
  PRUint32 filterCount =
    propertyHash.EnumerateRead(AddFilterToGUIDArrayEnumerator, guidArray);
  
  // Make sure we actually added some filters here. Otherwise something went
  // wrong and the results are not going to be what the caller expects.
  PRUint32 hashCount = propertyHash.Count();
  NS_ENSURE_STATE(filterCount == hashCount);

  // Finally make an enumerator to return the filtered items.
  nsCOMPtr<nsISimpleEnumerator> items =
    new sbGUIDArrayEnumerator(mLibrary, guidArray);
  NS_ENSURE_TRUE(items, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = items);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::IndexOf(sbIMediaItem* aMediaItem,
                                      PRUint32 aStartFrom,
                                      PRUint32* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::LastIndexOf(sbIMediaItem* aMediaItem,
                                          PRUint32 aStartFrom,
                                          PRUint32* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Contains(sbIMediaItem* aMediaItem,
                                       PRBool* _retval)
{
  /* virtual */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetIsEmpty(PRBool* aIsEmpty)
{
  nsresult rv;
  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsEmpty = length == 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Add(sbIMediaItem* aMediaItem)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::AddAll(sbIMediaList* aMediaList)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::InsertBefore(PRUint32 aIndex,
                                           sbIMediaItem* aMediaItem)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::MoveBefore(PRUint32 aFromIndex,
                                         PRUint32 aToIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::MoveLast(PRUint32 aIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Remove(sbIMediaItem* aMediaItem)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::RemoveByIndex(PRUint32 aIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::Clear()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetTreeView(nsITreeView** aTreeView)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::AddListener(sbIMediaListListener* aListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::RemoveListner(sbIMediaListListener* aListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// sbIMediaItem
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetLibrary(sbILibrary** aLibrary)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetOriginLibrary(sbILibrary** aOriginLibrary)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetIsMutable(PRBool* IsMutable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetGuid(nsAString& aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetMediaCreated(PRInt32* MediaCreated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetMediaCreated(PRInt32 aMediaCreated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetMediaUpdated(PRInt32* MediaUpdated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetMediaUpdated(PRInt32 aMediaUpdated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::TestIsAvailable(nsIObserver* Observer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetContentSrc(nsIURI** aContentSrc)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetContentSrc(nsIURI * aContentSrc)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetContentLength(PRInt32* ContentLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetContentLength(PRInt32 aContentLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetContentType(nsAString& aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetContentType(const nsAString& aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::OpenInputStream(PRUint32 aOffset,
                                              nsIInputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::OpenOutputStream(PRUint32 aOffset,
                                               nsIOutputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::ToString(nsAString& _retval)
{
  nsAutoString buff;

  buff.AppendLiteral("MediaList {guid: ");
  buff.Append(mGuid);
  buff.AppendLiteral("}");

  _retval = buff;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetUri(nsIURI** aUri)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetCreated(PRInt32* aCreated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetUpdated(PRInt32* aUpdated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetProperty(const nsAString& aName,
                                          nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::SetProperty(const nsAString& aName,
                                          const nsAString& aValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseMediaListBase)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetHelperForLanguage(PRUint32 language,
                                                   nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetFlags(PRUint32 *aFlags)
{
// not yet  *aFlags = nsIClassInfo::THREADSAFE;
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListBase::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

/**
 * This method enumerates a hash table and calls AddFilter on a GUIDArray once 
 * for each key. It constructs a string enumerator for the string array that
 * the hash table contains.
 *
 * This method expects to be handed an sbILocalDatabaseGUIDArray pointer as its
 * aUserData parameter.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListBase::AddFilterToGUIDArrayEnumerator(nsStringHashKey::KeyType aKey,
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

