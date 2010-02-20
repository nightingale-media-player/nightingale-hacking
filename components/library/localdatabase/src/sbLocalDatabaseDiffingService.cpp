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

#include "sbLocalDatabaseDiffingService.h"

#include "sbLocalDatabaseCID.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIProgrammingLanguage.h>
#include <nsIStringEnumerator.h>

#include <sbIMediaListView.h>

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsDataHashtable.h>
#include <nsHashKeys.h>
#include <nsTHashtable.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsTArray.h>
#include <nsXPCOMCID.h>

#include <sbLibraryChangeset.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

NS_IMPL_THREADSAFE_ADDREF(sbLocalDatabaseDiffingService)
NS_IMPL_THREADSAFE_RELEASE(sbLocalDatabaseDiffingService)

NS_INTERFACE_MAP_BEGIN(sbLocalDatabaseDiffingService)
  NS_IMPL_QUERY_CLASSINFO(sbLocalDatabaseDiffingService)
  NS_INTERFACE_MAP_ENTRY(sbILibraryDiffingService)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbILibraryDiffingService)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER1(sbLocalDatabaseDiffingService,
                             sbILibraryDiffingService)

NS_DECL_CLASSINFO(sbLocalDatabaseDiffingService)
NS_IMPL_THREADSAFE_CI(sbLocalDatabaseDiffingService)

sbLocalDatabaseDiffingService::sbLocalDatabaseDiffingService()
{
}

sbLocalDatabaseDiffingService::~sbLocalDatabaseDiffingService()
{
}

/*static*/
NS_METHOD sbLocalDatabaseDiffingService::RegisterSelf(
                          nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char* aLoaderStr,
                          const char* aType,
                          const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                         SB_LOCALDATABASE_DIFFINGSERVICE_DESCRIPTION,
                                         "service," SB_LOCALDATABASE_DIFFINGSERVICE_CONTRACTID,
                                         PR_TRUE,
                                         PR_TRUE,
                                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::Init()
{
  nsresult rv;

  mPropertyManager = do_CreateInstance(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::GetPropertyIDs(nsIStringEnumerator **aPropertyIDs)
{
  NS_ENSURE_ARG_POINTER(aPropertyIDs);
  NS_ENSURE_STATE(mPropertyManager);

  nsCOMPtr<nsIStringEnumerator> ids;

  nsresult rv = mPropertyManager->GetPropertyIDs(getter_AddRefs(ids));
  NS_ENSURE_SUCCESS(rv, rv);

  ids.forget(aPropertyIDs);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangeFromItems(
                                sbIMediaItem *aSourceItem,
                                sbIMediaItem *aDestinationItem,
                                sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestinationItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIPropertyArray> sourceProperties;
  nsCOMPtr<sbIPropertyArray> destinationProperties;

  nsresult rv = aSourceItem->GetProperties(nsnull,
                                           getter_AddRefs(sourceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationItem->GetProperties(nsnull,
                                       getter_AddRefs(destinationProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propertyChanges;
  rv = CreatePropertyChangesFromProperties(sourceProperties,
                                           destinationProperties,
                                           getter_AddRefs(propertyChanges));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                     0,
                                     aSourceItem,
                                     aDestinationItem,
                                     propertyChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChange = libraryChange);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateItemAddedLibraryChange(
                                sbIMediaItem *aSourceItem,
                                sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIPropertyArray> properties;
  nsresult rv = aSourceItem->GetProperties(nsnull, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyCount = 0;
  rv = properties->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString strPropertyID;
  nsString strPropertyValue;
  nsCOMPtr<sbIProperty> property;

  for(PRUint32 current = 0; current < propertyCount; ++current) {

    rv = properties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(strPropertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(strPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbPropertyChange> propertyChange;
    NS_NEWXPCOM(propertyChange, sbPropertyChange);
    NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

    rv = propertyChange->InitWithValues(sbIChangeOperation::ADDED,
                                        strPropertyID,
                                        EmptyString(),
                                        strPropertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> element =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propertyChanges->AppendElement(element,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = libraryChange->InitWithValues(sbIChangeOperation::ADDED,
                                     0,
                                     aSourceItem,
                                     nsnull,
                                     propertyChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChange = libraryChange);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateItemMovedLibraryChange(sbIMediaItem *aSourceItem,
                                                            PRUint32 aItemOrdinal,
                                                            sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbPropertyChange> propertyChange;
  NS_NEWXPCOM(propertyChange, sbPropertyChange);
  NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

  nsString strPropertyValue;
  strPropertyValue.AppendInt(aItemOrdinal);

  rv = propertyChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                      NS_LITERAL_STRING(SB_PROPERTY_ORDINAL),
                                      EmptyString(),
                                      strPropertyValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> element =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyChanges->AppendElement(element,
                                      PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChange->InitWithValues(sbIChangeOperation::MOVED,
                                     0,
                                     aSourceItem,
                                     nsnull,
                                     propertyChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChange = libraryChange);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateItemDeletedLibraryChange(sbIMediaItem *aDestinationItem,
                                                              sbILibraryChange **aLibraryChange)
{
  NS_ENSURE_ARG_POINTER(aDestinationItem);
  NS_ENSURE_ARG_POINTER(aLibraryChange);

  nsRefPtr<sbLibraryChange> libraryChange;
  NS_NEWXPCOM(libraryChange, sbLibraryChange);
  NS_ENSURE_TRUE(libraryChange, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = libraryChange->InitWithValues(sbIChangeOperation::DELETED,
                                              0,
                                              nsnull,
                                              aDestinationItem,
                                              nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChange = libraryChange);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreatePropertyChangesFromProperties(
                                sbIPropertyArray *aSourceProperties,
                                sbIPropertyArray *aDestinationProperties,
                                nsIArray **aPropertyChanges)
{
  NS_ENSURE_ARG_POINTER(aSourceProperties);
  NS_ENSURE_ARG_POINTER(aDestinationProperties);
  NS_ENSURE_ARG_POINTER(aPropertyChanges);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> propertyChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sourceLength;
  rv = aSourceProperties->GetLength(&sourceLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength;
  rv = aDestinationProperties->GetLength(&destinationLength);

  nsCOMPtr<sbIProperty> property;
  nsTHashtable<nsStringHashKey> sourcePropertyNamesFoundInDestination;

  PRBool success = sourcePropertyNamesFoundInDestination.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // These properties are excluded from checking since they
  // are automatically maintained and do not reflect actual
  // metadata differences in the items.
  nsTHashtable<nsStringHashKey> propertyExclusionList;
  success = propertyExclusionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsStringHashKey* successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CREATED));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_UPDATED));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_GUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  // Sadly we can't use content length because on a device the length may be
  // different
  successHashkey =
    propertyExclusionList.PutEntry(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH));
  NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

  nsString propertyId;
  nsString propertyValue;
  nsString propertyDestinationValue;

  // First, we process the sourceProperties.
  for(PRUint32 current = 0; current < sourceLength; ++current)
  {
    rv = aSourceProperties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if(propertyExclusionList.GetEntry(propertyId)) {
      continue;
    }

    rv = aDestinationProperties->GetPropertyValue(propertyId,
                                                  propertyDestinationValue);
    // Property has been added.
    if(rv == NS_ERROR_NOT_AVAILABLE) {
      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::ADDED,
                                          propertyId,
                                          EmptyString(),
                                          propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISupports> element =
        do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);

      rv = propertyChanges->AppendElement(element,
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {

      NS_ENSURE_SUCCESS(rv, rv);

      // Didn't fail, it should be present in both source and destination.
      successHashkey = sourcePropertyNamesFoundInDestination.PutEntry(propertyId);
      NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

      if (propertyId.EqualsLiteral(SB_PROPERTY_CONTENTURL)) {
        nsString originURL;
        rv = aDestinationProperties->GetPropertyValue(
                                       NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                       originURL);
        if (NS_SUCCEEDED(rv) && !originURL.IsEmpty()) {
          if (propertyValue.Equals(originURL)) {
            continue;
          }
        }
      }
      // Check if the duration is the same in seconds, that's good enough
      else if (propertyId.EqualsLiteral(SB_PROPERTY_DURATION)) {
        PRUint64 const sourceDuration = nsString_ToUint64(propertyValue, &rv);
        if (NS_SUCCEEDED(rv)) {
          PRUint64 const destDuration =
            nsString_ToUint64(propertyDestinationValue, &rv);
          // If the duration was parsed and the difference less than a second
          // then treat it as unchanged
          if (NS_SUCCEEDED(rv)
              && labs(sourceDuration - destDuration) < PR_USEC_PER_SEC) {
            continue;
          }
        }
      }
      // Property values are the same, nothing changed,
      // continue onto the next property.
      if(propertyValue.Equals(propertyDestinationValue)) {
        continue;
      }

      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::MODIFIED,
                                          propertyId,
                                          propertyDestinationValue,
                                          propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISupports> element =
        do_QueryInterface(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange), &rv);
      rv = propertyChanges->AppendElement(element,
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Second, we process the destinationProperties.
  // This will enable us to determine which properties were removed
  // from the source.
  for(PRUint32 current = 0; current < destinationLength; ++current) {
    rv = aDestinationProperties->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(propertyDestinationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if(propertyExclusionList.GetEntry(propertyId)) {
      continue;
    }

    if(!sourcePropertyNamesFoundInDestination.GetEntry(propertyId)) {

      // We couldn't find the property in the source properties, this means
      // the property must've been removed.
      nsRefPtr<sbPropertyChange> propertyChange;
      NS_NEWXPCOM(propertyChange, sbPropertyChange);
      NS_ENSURE_TRUE(propertyChange, NS_ERROR_OUT_OF_MEMORY);

      rv = propertyChange->InitWithValues(sbIChangeOperation::DELETED,
                                          propertyId,
                                          propertyDestinationValue,
                                          EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);

      rv = propertyChanges->AppendElement(NS_ISUPPORTS_CAST(sbIPropertyChange *, propertyChange),
                                          PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  PRUint32 propertyChangesCount = 0;
  rv = propertyChanges->GetLength(&propertyChangesCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aPropertyChanges = propertyChanges);

  return propertyChangesCount ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromLists(
                                sbIMediaList *aSourceList,
                                sbIMediaList *aDestinationList,
                                sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceList);
  NS_ENSURE_ARG_POINTER(aDestinationList);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> libraryChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseDiffingServiceEnumerator> sourceEnum;
  NS_NEWXPCOM(sourceEnum, sbLocalDatabaseDiffingServiceEnumerator);
  NS_ENSURE_TRUE(sourceEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = sourceEnum->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseDiffingServiceEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLocalDatabaseDiffingServiceEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = destinationEnum->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aSourceList->EnumerateAllItems(sourceEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationList->EnumerateAllItems(destinationEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> sourceArray;
  rv = sourceEnum->GetArray(getter_AddRefs(sourceArray));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> destinationArray;
  rv = destinationEnum->GetArray(getter_AddRefs(destinationArray));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sourceLength = 0;
  rv = sourceArray->GetLength(&sourceLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength = 0;
  rv = destinationArray->GetLength(&destinationLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
    EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
    EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;

  // Items present in the source (in the form of contentURL and originURL for each).
  nsTHashtable<nsStringHashKey> itemsInSource;

  PRBool success = itemsInSource.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // We need to keep track of item counts so we can create multiple
  // added changes since items may be added N times into the media list.
  nsDataHashtable<nsStringHashKey, PRUint32> itemsInSourceCount;
  nsDataHashtable<nsStringHashKey, PRUint32> itemsInSourceAvailableOccurrences;

  success = itemsInSourceCount.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = itemsInSourceAvailableOccurrences.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // For each item in the source, verify presence in destination media list.
  for(PRUint32 current = 0; current < sourceLength; ++current) {

    // We verify its presence using contentURL and originURL.
    item = do_QueryElementAt(sourceArray, current, &rv);

    // Bad item
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
       propertyCount != 2) {
      continue;
    }

    nsString strPropertyID;
    nsString strPropertyValue;
    nsString contentURL;

    PRBool hasFoundItem = PR_FALSE;
    nsCOMPtr<nsIArray> foundItems;
    nsCOMPtr<sbIProperty> property;
    nsCOMPtr<sbIMediaItem> itemDestination;

    for(PRUint32 currentProperty = 0;
        currentProperty < propertyCount && !hasFoundItem;
        ++currentProperty) {

      rv = currentArray->GetPropertyAt(currentProperty,
                                       getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetId(strPropertyID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // If there is no value, and we're not dealing with the origin URL then skip
      if(strPropertyValue.IsEmpty() &&
          (contentURL.IsEmpty() ||
           !strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL))) {
        continue;
      }

      // Need to determine if the origin URL is blank. We'll default to
      // false since this may not even be the origin URL. We need to know
      // if we're dealing with a blank origin URL below, where we check the
      // source library.
      PRBool const propertyWasBlank = strPropertyValue.IsEmpty();

      // If this is the content URL save off the value for the origin URL
      if(strPropertyID.EqualsLiteral(SB_PROPERTY_CONTENTURL)) {
        contentURL = strPropertyValue;
      }
      // We've got an origin URL switch to the value content URL
      // if there is no origin URL
      else if(strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL)) {
        // Determine if the originURL is blank
        if (propertyWasBlank) {
          strPropertyValue = contentURL;
        }
      }

      // Try and find it.
      rv = aDestinationList->GetItemsByProperty(strPropertyID,
                                                strPropertyValue,
                                                getter_AddRefs(foundItems));
      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);

      // Hooray, we found it. Add it the found list.
      nsStringHashKey *successHashkey = itemsInSource.PutEntry(strPropertyValue);
      NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

      PRUint32 foundItemsCount = 0;
      rv = foundItems->GetLength(&foundItemsCount);
      NS_ENSURE_SUCCESS(rv, rv);

      if(!foundItemsCount) {
          continue;
      }

      // If we're dealing with the origin URL and the source one is blank
      // we need to switch back to the content URL to lookup the source
      if (strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL) &&
          propertyWasBlank) {
        strPropertyID.AssignLiteral(SB_PROPERTY_CONTENTURL);
      }

      // Figure out how many occurrences of the item we have
      // in the source, this will help us figure out if we should
      // mark an item "added" instead of "modified" when its
      // present more than once.
      nsCOMPtr<nsIArray> foundItemsInSource;
      rv = aSourceList->GetItemsByProperty(strPropertyID,
                                           strPropertyValue,
                                           getter_AddRefs(foundItemsInSource));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 foundItemsInSourceCount = 0;
      rv = foundItemsInSource->GetLength(&foundItemsInSourceCount);
      NS_ENSURE_SUCCESS(rv, rv);

      // Is this the first time we've found this item?
      if(foundItemsInSourceCount > 1) {

        PRUint32 itemTotal = 0;
        if(itemsInSourceAvailableOccurrences.Get(strPropertyValue, &itemTotal)){
          // We've already found this item, only continue if it has occurrences
          // left to use.
          if(itemTotal > 1) {
            success =
              itemsInSourceAvailableOccurrences.Put(strPropertyValue, itemTotal - 1);
          }
          else {
            // No more valid occurrences of this item left, skip.
            break;
          }
        }
        else {
          // First time seeing this item, record the amount of occurrences.
          success = itemsInSourceCount.Put(strPropertyValue, foundItemsInSourceCount);
          NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

          // Also put it in a counter that we decrement. This will prevent us
          // from going over the amount of occurrences for the item.
          success = itemsInSourceAvailableOccurrences.Put(strPropertyValue, foundItemsInSourceCount - 1);
        }
      }

      // No matter which occurrence of the item we are using, the item is the same.
      itemDestination = do_QueryElementAt(foundItems, 0, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // We have something that seems like a pretty good match.
      hasFoundItem = PR_TRUE;
    }

    if(hasFoundItem) {
      // Present in destination, verify for property changes.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateLibraryChangeFromItems(item, itemDestination, getter_AddRefs(libraryChange));

      // Item did not change.
      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }

      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      // Not present in destination, get all properties for item and indicate it was added.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemAddedLibraryChange(item, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsDataHashtable<nsStringHashKey, PRUint32> itemsInDestinationCount;
  success = itemsInDestinationCount.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // For each item in the destination
  for(PRUint32 current = 0; current < destinationLength; ++current) {
    // Verify if present in source library.
    item = do_QueryElementAt(destinationArray, current, &rv);
    // Bad item.
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
      propertyCount != 2) {
        continue;
    }

    nsString strPropertyValue;

    // We verify its presence using contentURL and originURL.
    for(PRUint32 currentProperty = 0; currentProperty < propertyCount; ++currentProperty) {

      nsCOMPtr<sbIProperty> property;
      rv = currentArray->GetPropertyAt(currentProperty, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // Present in source, check to see if we've run out of occurrences
      // for this item or not.
      if(itemsInSource.GetEntry(strPropertyValue)) {

        PRUint32 destCount = 0;
        PRUint32 sourceCount = 0;

        if(itemsInSourceCount.Get(strPropertyValue, &sourceCount)) {

          if(itemsInDestinationCount.Get(strPropertyValue, &destCount)) {

            // We ran out of occurrences for this item.
            // It should be marked as deleted.
            if(destCount >= sourceCount) {
              // Not present in source, indicate that the item was removed from the source.
              nsCOMPtr<sbILibraryChange> libraryChange;
              rv = CreateItemDeletedLibraryChange(item, getter_AddRefs(libraryChange));
              NS_ENSURE_SUCCESS(rv, rv);

              rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
              NS_ENSURE_SUCCESS(rv, rv);
            }

          }
          else {
            // Item is present in source and has occurrences left for use.
            success = itemsInDestinationCount.Put(strPropertyValue, destCount + 1);
            NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

            break;
          }
        }
        else {
          // Single occurrence and it's present, simply continue onto the next item.
          break;
        }
      }
      else {
        // Not present in source, indicate that the item was removed from the source.
        nsCOMPtr<sbILibraryChange> libraryChange;
        rv = CreateItemDeletedLibraryChange(item, getter_AddRefs(libraryChange));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        break;
      }
    }
  }

  // Ensure that all items present will be in the correct order
  // by explicity including a move operation for each item present
  // the source.

  PRUint32 sourceListLength = 0;
  rv = aSourceList->GetLength(&sourceListLength);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 i = 0; i < sourceListLength; ++i) {
    rv = aSourceList->GetItemByIndex(i, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibraryChange> libraryChange;
    rv = CreateItemMovedLibraryChange(item, i, getter_AddRefs(libraryChange));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // That's it, we should have a valid changeset.
  nsCOMPtr<nsIMutableArray> sources =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sources->AppendElement(aSourceList, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChangeset->InitWithValues(sources,
                                        aDestinationList,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChangeset = libraryChangeset);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromLibraries(
                                sbILibrary *aSourceLibrary,
                                sbILibrary *aDestinationLibrary,
                                sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceLibrary);
  NS_ENSURE_ARG_POINTER(aDestinationLibrary);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> libraryChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseDiffingServiceEnumerator> sourceEnum;
  NS_NEWXPCOM(sourceEnum, sbLocalDatabaseDiffingServiceEnumerator);
  NS_ENSURE_TRUE(sourceEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = sourceEnum->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseDiffingServiceEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLocalDatabaseDiffingServiceEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = destinationEnum->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aSourceLibrary->EnumerateAllItems(sourceEnum,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationLibrary->EnumerateAllItems(destinationEnum,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> sourceArray;
  rv = sourceEnum->GetArray(getter_AddRefs(sourceArray));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> destinationArray;
  rv = destinationEnum->GetArray(getter_AddRefs(destinationArray));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sourceLength = 0;
  rv = sourceArray->GetLength(&sourceLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength = 0;
  rv = destinationArray->GetLength(&destinationLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                     EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;

  // Items present in the source (in the form of contentURL and originURL for each).
  nsTHashtable<nsStringHashKey> itemsInSource;

  PRBool success = itemsInSource.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // For each item in the source, verify presence in destination library.
  for(PRUint32 current = 0; current < sourceLength; ++current) {

    // We verify its presence using contentURL and originURL.
    item = do_QueryElementAt(sourceArray, current, &rv);

    // Bad item
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
       propertyCount != 2) {
      continue;
    }

    nsString strPropertyID;
    nsString strPropertyValue;
    nsString contentURL;

    PRBool hasFoundItem = PR_FALSE;
    nsCOMPtr<nsIArray> foundItems;
    nsCOMPtr<sbIProperty> property;
    nsCOMPtr<sbIMediaItem> itemDestination;

    for(PRUint32 currentProperty = 0;
        currentProperty < propertyCount && !hasFoundItem;
        ++currentProperty) {
      rv = currentArray->GetPropertyAt(currentProperty,
                                       getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetId(strPropertyID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // If there is no value, and we're not dealing with the origin URL then skip
      if(strPropertyValue.IsEmpty() &&
          (contentURL.IsEmpty() ||
           !strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL))) {
        continue;
      }

      // If this is the content URL save off the value for the origin URL
      if(strPropertyID.EqualsLiteral(SB_PROPERTY_CONTENTURL) &&
         strPropertyValue.IsEmpty()) {
        contentURL = strPropertyValue;
      }
      // We've got an origin URL switch to the value content URL if there
      // is no origin URL
      else if(strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL) &&
              strPropertyValue.IsEmpty()) {
        strPropertyValue = contentURL;
      }

      // Try and find it.
      rv = aDestinationLibrary->GetItemsByProperty(strPropertyID,
                                                   strPropertyValue,
                                                   getter_AddRefs(foundItems));

      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }

      NS_ENSURE_SUCCESS(rv, rv);

      // Hooray, we found it. Add it the found list.
      nsStringHashKey *successHashkey = itemsInSource.PutEntry(strPropertyValue);
      NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

      // There's no way to be certain which item is correct if we get multiple matches.
      // Because of this, we will reject multiple matches on contentURL and originURL
      // until we can create hashes of files to enable identifying them uniquely.
      PRUint32 foundItemsCount = 0;
      rv = foundItems->GetLength(&foundItemsCount);
      if(NS_FAILED(rv) ||
         foundItemsCount != 1) {
        continue;
      }

      itemDestination = do_QueryElementAt(foundItems, 0, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // We have something that seems like a pretty good match.
      hasFoundItem = PR_TRUE;
    }

    if(hasFoundItem) {
      // Present in destination, verify for property changes.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateLibraryChangeFromItems(item, itemDestination, getter_AddRefs(libraryChange));

      // Item did not change.
      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }

      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      // Not present in destination, get all properties for item and indicate it was added.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemAddedLibraryChange(item, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // For each item in the destination
  for(PRUint32 current = 0; current < destinationLength; ++current) {
    // Verify if present in source library.
    item = do_QueryElementAt(destinationArray, current, &rv);
    // Bad item.
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
      propertyCount != 2) {
        continue;
    }

    nsString strPropertyValue;

    // We verify its presence using contentURL and originURL.
    PRBool found = PR_FALSE;
    for(PRUint32 currentProperty = 0; currentProperty < propertyCount; ++currentProperty) {

      nsCOMPtr<sbIProperty> property;
      rv = currentArray->GetPropertyAt(currentProperty, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      if(itemsInSource.GetEntry(strPropertyValue)) {
        // Present in source, continue with next item instead.
        found = PR_TRUE;
        break;
      }
    }
    if (!found) {
      // Not present in source, indicate that the item was removed from the source.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemDeletedLibraryChange(item, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // That's it, we should have a valid changeset.
  nsCOMPtr<nsIMutableArray> sources =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sources->AppendElement(aSourceLibrary, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryChangeset->InitWithValues(sources,
                                        aDestinationLibrary,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChangeset = libraryChangeset);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::CreateLibraryChangesetFromListsToLibrary(
                                  nsIArray *aSourceLists,
                                  sbILibrary *aDestinationLibrary,
                                  sbILibraryChangeset **aLibraryChangeset)
{
  NS_ENSURE_ARG_POINTER(aSourceLists);
  NS_ENSURE_ARG_POINTER(aDestinationLibrary);
  NS_ENSURE_ARG_POINTER(aLibraryChangeset);

  // First, we build a hashtable of all the unique GUIDs.
  PRUint32 sourcesLength = 0;
  nsresult rv = aSourceLists->GetLength(&sourcesLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> sourceList;
  nsCOMPtr<sbIMediaItem> sourceItem;
  nsString sourceItemGUID;

  nsTArray<nsString> uniqueItemGUIDs;
  nsInterfaceHashtable<nsStringHashKey, sbIMediaItem> uniqueItems;
  PRBool success = uniqueItems.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Items present in the source (in the form of contentURL, originURL and
  // originGUID for each).
  nsTHashtable<nsStringHashKey> itemsInSource;
  success = itemsInSource.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                     EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                     EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 currentSource = 0;
      currentSource < sourcesLength;
      ++currentSource) {

    sourceItem = do_QueryElementAt(aSourceLists, currentSource, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    sourceList = do_QueryInterface(sourceItem, &rv);
    // Not media list.
    if (NS_FAILED(rv)) {
      rv = AddToUniqueItemList(sourceItem,
                               propertyArray,
                               uniqueItems,
                               uniqueItemGUIDs,
                               itemsInSource);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      PRUint32 sourceLength = 0;
      rv = sourceList->GetLength(&sourceLength);
      NS_ENSURE_SUCCESS(rv, rv);

      for(PRUint32 currentItem = 0; currentItem < sourceLength; ++currentItem) {
        rv = sourceList->GetItemByIndex(currentItem,
                                        getter_AddRefs(sourceItem));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = AddToUniqueItemList(sourceItem,
                                 propertyArray,
                                 uniqueItems,
                                 uniqueItemGUIDs,
                                 itemsInSource);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Add source list to the unique item list if it's not a library.
      nsCOMPtr<sbILibrary> sourceListIsLibrary = do_QueryInterface(sourceList);
      if (!sourceListIsLibrary) {
        rv = AddToUniqueItemList(sourceList,
                                 propertyArray,
                                 uniqueItems,
                                 uniqueItemGUIDs,
                                 itemsInSource);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  nsRefPtr<sbLibraryChangeset> libraryChangeset;
  NS_NEWXPCOM(libraryChangeset, sbLibraryChangeset);
  NS_ENSURE_TRUE(libraryChangeset, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIMutableArray> libraryChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseDiffingServiceEnumerator> destinationEnum;
  NS_NEWXPCOM(destinationEnum, sbLocalDatabaseDiffingServiceEnumerator);
  NS_ENSURE_TRUE(destinationEnum, NS_ERROR_OUT_OF_MEMORY);

  rv = destinationEnum->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDestinationLibrary->EnumerateAllItems(destinationEnum,
    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> destinationArray;
  rv = destinationEnum->GetArray(getter_AddRefs(destinationArray));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 destinationLength = 0;
  rv = destinationArray->GetLength(&destinationLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  nsCOMPtr<sbIMediaItem> itemDestination;

  PRUint32 sourceLength = uniqueItemGUIDs.Length();

  // For each item in the source, verify presence in destination library.
  for(PRUint32 current = 0; current < sourceLength; ++current) {

    // We verify its presence using contentURL and originURL.
    success = uniqueItems.Get(uniqueItemGUIDs[current],
                              getter_AddRefs(item));

    // Bad item
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
       propertyCount != 3) {
      continue;
    }

    nsString strPropertyID;
    nsString strPropertyValue;
    nsString contentURL;

    PRBool hasFoundItem = PR_FALSE;
    nsCOMPtr<nsIArray> foundItems;
    nsCOMPtr<sbIProperty> property;
    nsCOMPtr<sbIMediaItem> itemDestination;

    for(PRUint32 currentProperty = 0;
        currentProperty < propertyCount && !hasFoundItem;
        ++currentProperty) {
      rv = currentArray->GetPropertyAt(currentProperty,
                                       getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetId(strPropertyID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // If there is no value, and we're not dealing with the origin URL then skip
      if(strPropertyValue.IsEmpty() &&
          (contentURL.IsEmpty() ||
           !strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL))) {
        continue;
      }

      // If this is the content URL save off the value for the origin URL
      if(strPropertyID.EqualsLiteral(SB_PROPERTY_CONTENTURL)) {
        contentURL = strPropertyValue;
      }
      // We've got an origin URL switch to the value content URL if there
      // is no origin URL
      else if(strPropertyID.EqualsLiteral(SB_PROPERTY_ORIGINURL) &&
              strPropertyValue.IsEmpty()) {
        strPropertyValue = contentURL;
      }

      // Try and find it.
      rv = aDestinationLibrary->GetItemsByProperty(strPropertyID,
                                                   strPropertyValue,
                                                   getter_AddRefs(foundItems));

      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }

      NS_ENSURE_SUCCESS(rv, rv);

      // Hooray, we found it. Add it the found list.
      nsStringHashKey *successHashkey = itemsInSource.PutEntry(strPropertyValue);
      NS_ENSURE_TRUE(successHashkey, NS_ERROR_OUT_OF_MEMORY);

      // There's no way to be certain which item is correct if we get multiple matches.
      // Because of this, we will reject multiple matches on contentURL and originURL
      // until we can create hashes of files to enable identifying them uniquely.
      PRUint32 foundItemsCount = 0;
      rv = foundItems->GetLength(&foundItemsCount);
      if(NS_FAILED(rv) ||
        foundItemsCount > 1 ||
        !foundItemsCount) {
          continue;
      }

      itemDestination = do_QueryElementAt(foundItems, 0, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // We have something that seems like a pretty good match.
      hasFoundItem = PR_TRUE;
    }

    if(hasFoundItem) {
      // Present in destination, verify for property changes.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateLibraryChangeFromItems(item, itemDestination, getter_AddRefs(libraryChange));

      // Item did not change.
      if(rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }

      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      // Not present in destination, get all properties for item and indicate it was added.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemAddedLibraryChange(item, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // For each item in the destination
  for(PRUint32 current = 0; current < destinationLength; ++current) {

    // Verify if present in source library.
    item = do_QueryElementAt(destinationArray, current, &rv);
    // Bad item.
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> currentArray;
    rv = item->GetProperties(propertyArray, getter_AddRefs(currentArray));

    // Couldn't get properties required.
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyCount = 0;
    rv = currentArray->GetLength(&propertyCount);

    // Couldn't get the property count or property count
    // is unexpected, skip item and continue.
    if(NS_FAILED(rv) ||
       propertyCount != 3) {
      continue;
    }

    nsString strPropertyValue;

    // We verify its presence using contentURL and originURL.
    PRBool found = PR_FALSE;
    for(PRUint32 currentProperty = 0; currentProperty < propertyCount; ++currentProperty) {

      nsCOMPtr<sbIProperty> property;
      rv = currentArray->GetPropertyAt(currentProperty, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = property->GetValue(strPropertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      if(itemsInSource.GetEntry(strPropertyValue)) {
        // Present in source, continue with next item instead.
        found = PR_TRUE;
        break;
      }
    }
    if (!found) {
      // Not present in source, indicate that the item was removed from the source.
      nsCOMPtr<sbILibraryChange> libraryChange;
      rv = CreateItemDeletedLibraryChange(item, getter_AddRefs(libraryChange));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = libraryChanges->AppendElement(libraryChange, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // That's it, we should have a valid changeset.
  rv = libraryChangeset->InitWithValues(aSourceLists,
                                        aDestinationLibrary,
                                        libraryChanges);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aLibraryChangeset = libraryChangeset);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingService::AddToUniqueItemList(
  sbIMediaItem*                                        aItem,
  sbIPropertyArray*                                    aUniquePropSet,
  nsInterfaceHashtable<nsStringHashKey, sbIMediaItem>& aUniqueItemList,
  nsTArray<nsString>&                                  aUniqueItemGUIDList,
  nsTHashtable<nsStringHashKey>&                       aUniqueItemPropTable)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aUniquePropSet);

  PRBool success;
  nsresult rv;

  nsCOMPtr<sbIPropertyArray> currentArray;
  rv = aItem->GetProperties(aUniquePropSet, getter_AddRefs(currentArray));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString contentURL;
  rv = currentArray->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                      contentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Already in the list, done.
  if(aUniqueItemPropTable.GetEntry(contentURL)) {
    return NS_OK;
  }

  // It's ok for this property to not be available.
  nsString originURL;
  rv = currentArray->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                      originURL);
  if(rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Already in the list, done.
  if(!originURL.IsEmpty() &&
     aUniqueItemPropTable.GetEntry(originURL)) {
    return NS_OK;
  }

  // It's ok for this property to not be available.
  nsString originGUID;
  rv = currentArray->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                      originGUID);
  if(rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Already in the list, done.
  if(!originGUID.IsEmpty() &&
     aUniqueItemPropTable.GetEntry(originGUID)) {
    return NS_OK;
  }

  // contentURL not in the list yet, add it. We don't need to check
  // for empty with this property value since it's _required_.
  nsStringHashKey *hashKey = aUniqueItemPropTable.PutEntry(contentURL);
  NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);

  // originURL not in the list yet, add it.
  if(!originURL.IsEmpty()) {
    hashKey = aUniqueItemPropTable.PutEntry(originURL);
    NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);
  }

  // originGUID not in the list yet, add it.
  if(!originGUID.IsEmpty()) {
    hashKey = aUniqueItemPropTable.PutEntry(originGUID);
    NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);
  }

  nsString sourceItemGUID;
  rv = aItem->GetGuid(sourceItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // We are finally certain that the item not in final list, add it.
  if(!aUniqueItemList.Get(sourceItemGUID, nsnull)) {
    success = aUniqueItemList.Put(sourceItemGUID, aItem);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    nsString *element = aUniqueItemGUIDList.AppendElement(sourceItemGUID);
    NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

/* sbILibraryChangeset createChangeset (in sbIMediaList aSource, in sbIMediaList aDestination); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateChangeset(sbIMediaList *aSource,
                                                             sbIMediaList *aDestination,
                                                             sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbILibrary> sourceLibrary = do_QueryInterface(aSource);
  nsCOMPtr<sbILibrary> destinationLibrary = do_QueryInterface(aDestination);

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<sbILibraryChangeset> changeset;

  if(sourceLibrary && destinationLibrary) {

    rv = CreateLibraryChangesetFromLibraries(sourceLibrary,
                                             destinationLibrary,
                                             getter_AddRefs(changeset));

  }
  else {
    rv = CreateLibraryChangesetFromLists(aSource,
                                         aDestination,
                                         getter_AddRefs(changeset));
  }

  NS_ENSURE_SUCCESS(rv, rv);
  changeset.forget(_retval);

  return NS_OK;
}

/* sbILibraryChangeset createMultiChangeset (in nsIArray aSources, in sbIMediaList aDestination); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateMultiChangeset(nsIArray *aSources,
                                                                  sbIMediaList *aDestination,
                                                                  sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;

  // Destination must be a library.
  nsCOMPtr<sbILibrary> destinationLibrary =
    do_QueryInterface(aDestination);
  NS_ENSURE_TRUE(destinationLibrary, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbILibraryChangeset> changeset;
  rv = CreateLibraryChangesetFromListsToLibrary(aSources,
                                                destinationLibrary,
                                                getter_AddRefs(changeset));
  NS_ENSURE_SUCCESS(rv, rv);

  changeset.forget(_retval);

  return NS_OK;
}

/* AString createChangesetAsync (in sbIMediaList aSource, in sbIMediaList aDestination, [optional] in nsIObserver aObserver); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateChangesetAsync(sbIMediaList *aSource,
                                                                  sbIMediaList *aDestination,
                                                                  nsIObserver *aObserver,
                                                                  nsAString & _retval)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDestination);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbILibraryChangeset createMultiChangesetAsync (in nsIArray aSources, in sbIMediaList aDestination, [optional] in nsIObserver aObserver); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::CreateMultiChangesetAsync(nsIArray *aSources,
                                                                       sbIMediaList *aDestination,
                                                                       nsIObserver *aObserver,
                                                                       sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbILibraryChangeset getChangeset (in AString aChangesetCookie); */
NS_IMETHODIMP sbLocalDatabaseDiffingService::GetChangeset(const nsAString & aChangesetCookie,
                                                          sbILibraryChangeset **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// sbLocalDatabaseDiffingServiceComparator
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseDiffingServiceEnumerator,
                              sbIMediaListEnumerationListener)

sbLocalDatabaseDiffingServiceEnumerator::sbLocalDatabaseDiffingServiceEnumerator()
{
}

sbLocalDatabaseDiffingServiceEnumerator::~sbLocalDatabaseDiffingServiceEnumerator()
{
}

nsresult
sbLocalDatabaseDiffingServiceEnumerator::Init()
{
  NS_ENSURE_FALSE(mArray, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;
  mArray = do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseDiffingServiceEnumerator::GetArray(nsIArray **aArray)
{
  NS_ENSURE_TRUE(mArray, NS_ERROR_NOT_AVAILABLE);

  NS_ADDREF(*aArray = mArray);

  return NS_OK;
}

/* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbLocalDatabaseDiffingServiceEnumerator::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                            PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbLocalDatabaseDiffingServiceEnumerator::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                          sbIMediaItem *aMediaItem,
                                                          PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = mArray->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbLocalDatabaseDiffingServiceEnumerator::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                          nsresult aStatusCode)
{
  return NS_OK;
}
