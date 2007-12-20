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

#include "sbLocalDatabaseCascadeFilterSet.h"

#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseMediaListView.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSchemaInfo.h"

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIArray.h>
#include <nsIMutableArray.h>
#include <nsIObjectOutputStream.h>
#include <nsIObjectInputStream.h>
#include <nsISimpleEnumerator.h>
#include <nsITreeView.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>

#include <DatabaseQuery.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIFilterableMediaListView.h>
#include <sbILibraryConstraints.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>
#include <sbISearchableMediaListView.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStandardProperties.h>
#include <sbTArrayStringEnumerator.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseCascadeFilterSet:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* sFilterSetLog = nsnull;
#define TRACE(args) if (sFilterSetLog) PR_LOG(sFilterSetLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (sFilterSetLog) PR_LOG(sFilterSetLog, PR_LOG_WARN, args)
#else /* PR_LOGGING */
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbLocalDatabaseCascadeFilterSet::sbLocalDatabaseCascadeFilterSet(sbLocalDatabaseMediaListView* aMediaListView) :
  mMediaListView(aMediaListView)
{
  MOZ_COUNT_CTOR(sbLocalDatabaseCascadeFilterSet);
  NS_ASSERTION(aMediaListView, "aMediaListView is null");
#ifdef PR_LOGGING
  if (!sFilterSetLog) {
    sFilterSetLog = PR_NewLogModule("sbLocalDatabaseCascadeFilterSet");
  }
#endif
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Constructed", this));
}

sbLocalDatabaseCascadeFilterSet::~sbLocalDatabaseCascadeFilterSet()
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Destructed", this));
  MOZ_COUNT_DTOR(sbLocalDatabaseCascadeFilterSet);

  if (mMediaList) {
    mMediaList->RemoveListener(this);
  }
}

NS_IMPL_ISUPPORTS3(sbLocalDatabaseCascadeFilterSet,
                   sbICascadeFilterSet,
                   sbIMediaListListener,
                   nsISupportsWeakReference);

nsresult
sbLocalDatabaseCascadeFilterSet::Init(sbLocalDatabaseLibrary* aLibrary,
                                      sbILocalDatabaseAsyncGUIDArray* aProtoArray,
                                      sbLocalDatabaseCascadeFilterSetState* aState)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Init", this));
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aProtoArray);
  NS_ENSURE_STATE(mMediaListView);

  nsresult rv;

  mLibrary = aLibrary;

  // Set up our prototype array
  mProtoArray = aProtoArray;

  rv = mProtoArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->SetIsDistinct(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mListeners.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = mMediaListView->GetMediaList(getter_AddRefs(mMediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  // If there is any state to restore, restore it
  if (aState) {
#ifdef DEBUG
    {
      nsString buff;
      aState->ToString(buff);
      nsCOMPtr<nsISupports> supports = do_QueryInterface(aState);
      TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - restoring state [0x%.8x] %s",
             this, supports.get(), NS_LossyConvertUTF16toASCII(buff).get()));
    }
#endif

    for (PRUint32 i = 0; i < aState->mFilters.Length(); i++) {

      sbLocalDatabaseCascadeFilterSetState::Spec& spec = aState->mFilters[i];

      sbFilterSpec* fs = mFilters.AppendElement();
      NS_ENSURE_TRUE(fs, NS_ERROR_OUT_OF_MEMORY);

      fs->isSearch = spec.isSearch;
      fs->property = spec.property;
      nsString* added = fs->propertyList.AppendElements(spec.propertyList);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
      added = fs->values.AppendElements(spec.values);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

      if (spec.isSearch) {
        rv = ConfigureFilterArray(fs, NS_LITERAL_STRING(SB_PROPERTY_CREATED));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        rv = ConfigureFilterArray(fs, spec.property);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = ConfigureArray(i);
      NS_ENSURE_SUCCESS(rv, rv);

      if (spec.treeViewState) {
        nsRefPtr<sbLocalDatabaseTreeView> treeView =
          new sbLocalDatabaseTreeView();
        NS_ENSURE_TRUE(treeView, NS_ERROR_OUT_OF_MEMORY);

        rv = treeView->Init(mMediaListView,
                            fs->array,
                            nsnull,
                            spec.treeViewState);
        NS_ENSURE_SUCCESS(rv, rv);

        fs->treeView = treeView;
      }
    }
  }

  rv = UpdateListener(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetLength(PRUint16* aLength)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetLength", this));
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = mFilters.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetProperty(PRUint16 aIndex,
                                             nsAString& _retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetProperty", this));
  PRUint32 filterLength = mFilters.Length();
  NS_ENSURE_TRUE(filterLength, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_MAX(aIndex, filterLength - 1);

  _retval = mFilters[aIndex].property;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::IsSearch(PRUint16 aIndex,
                                          PRBool* _retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - IsSearch", this));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  *_retval = mFilters[aIndex].isSearch;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AppendFilter(const nsAString& aProperty,
                                              PRUint16 *_retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - AppendFilter", this));
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  sbFilterSpec* fs = mFilters.AppendElement();
  NS_ENSURE_TRUE(fs, NS_ERROR_OUT_OF_MEMORY);

  fs->isSearch = PR_FALSE;
  fs->property = aProperty;

  rv = ConfigureFilterArray(fs, aProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ConfigureArray(mFilters.Length() - 1);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mFilters.Length() - 1;

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AppendSearch(const PRUnichar** aPropertyArray,
                                              PRUint32 aPropertyArrayCount,
                                              PRUint16 *_retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - AppendSearch", this));
  if (aPropertyArrayCount) {
    NS_ENSURE_ARG_POINTER(aPropertyArray);
  }
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // A filter set can only have one search
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    if (mFilters[i].isSearch) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  sbFilterSpec* fs = mFilters.AppendElement();
  NS_ENSURE_TRUE(fs, NS_ERROR_OUT_OF_MEMORY);

  fs->isSearch = PR_TRUE;

  for (PRUint32 i = 0; i < aPropertyArrayCount; i++) {
    if (aPropertyArray[i]) {
      nsString* success = fs->propertyList.AppendElement(aPropertyArray[i]);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_WARNING("Null pointer passed in array");
    }
  }

  rv = ConfigureFilterArray(fs, NS_LITERAL_STRING(SB_PROPERTY_CREATED));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ConfigureArray(mFilters.Length() - 1);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mFilters.Length() - 1;

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::Remove(PRUint16 aIndex)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Remove", this));
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  nsresult rv;

  sbFilterSpec& fs = mFilters[aIndex];
  if (fs.arrayListener)
    fs.array->RemoveAsyncListener(fs.arrayListener);

  mFilters.RemoveElementAt(aIndex);

  // Update the filters downstream from the removed filter
  for (PRUint32 i = aIndex; i < mFilters.Length(); i++) {
    rv = ConfigureArray(i);
    NS_ENSURE_SUCCESS(rv, rv);

    // Notify listeners
    mListeners.EnumerateEntries(OnValuesChangedCallback, &i);
  }

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::ChangeFilter(PRUint16 aIndex,
                                              const nsAString& aProperty)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - ChangeFilter", this));
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  nsresult rv;

  sbFilterSpec& fs = mFilters[aIndex];
  if (fs.isSearch)
    return NS_ERROR_INVALID_ARG;

  fs.property = aProperty;
  fs.invalidationPending = PR_FALSE;

  rv = fs.array->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fs.array->AddSort(aProperty, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  fs.values.Clear();
  rv = ConfigureArray(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  if (fs.treeView) {
    nsCOMPtr<nsITreeSelection> selection;
    rv = fs.treeView->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = selection->ClearSelection();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fs.treeView->Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Tell the view to update its configuration.  It will first apply its
  // filters and then ask us for ours
  if (mMediaListView) {
    rv = mMediaListView->UpdateViewArrayConfiguration(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // And notify the view's listeners
    mMediaListView->NotifyListenersFilterChanged();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::Set(PRUint16 aIndex,
                                     const PRUnichar** aValueArray,
                                     PRUint32 aValueArrayCount)
{
#ifdef DEBUG
  {
    nsString buff;
    for (PRUint32 i = 0; i < aValueArrayCount && aValueArray; i++) {
      buff.Append(aValueArray[i]);
      if (i + 1 < aValueArrayCount) {
        buff.AppendLiteral(", ");
      }
    }
    if (!aValueArrayCount) {
      buff.AssignLiteral("no values");
    }
    TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Set %d, %d [%s]",
           this, aIndex, aValueArrayCount,
           NS_LossyConvertUTF16toASCII(buff).get()));
  }
#endif
  if (aValueArrayCount) {
    NS_ENSURE_ARG_POINTER(aValueArray);
  }
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  nsresult rv;

  sbFilterSpec& fs = mFilters[aIndex];
  fs.values.Clear();

  for (PRUint32 i = 0; i < aValueArrayCount; i++) {
    if (aValueArray[i]) {
      nsString* value = fs.values.AppendElement(aValueArray[i]);
      NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_WARNING("Null pointer passed in array");
    }
  }

  // Update downstream filters
  for (PRUint32 i = aIndex + 1; i < mFilters.Length(); i++) {

    // We want to clear the downstream filter since the upstream filter has
    // changed
    sbFilterSpec& downstream = mFilters[i];
    downstream.values.Clear();

    rv = ConfigureArray(i);
    NS_ENSURE_SUCCESS(rv, rv);

    if (downstream.treeView) {
      nsCOMPtr<nsITreeSelection> selection;
      rv = downstream.treeView->GetSelection(getter_AddRefs(selection));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = selection->ClearSelection();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = downstream.treeView->Rebuild();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Notify listeners
    mListeners.EnumerateEntries(OnValuesChangedCallback, &i);
  }

  // Tell the view to update its configuration.  It will first apply its
  // filters and then ask us for ours
  if (mMediaListView) {
    rv = mMediaListView->UpdateViewArrayConfiguration(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // And notify the view's listeners
    if (fs.isSearch) {
      mMediaListView->NotifyListenersSearchChanged();
    }
    else {
      mMediaListView->NotifyListenersFilterChanged();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::ClearAll()
{
  nsresult rv;

  PRBool filterChanged = PR_FALSE, searchChanged = PR_FALSE;

  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];

    if (fs.isSearch) {
      if (!searchChanged) {
        searchChanged = PR_TRUE;
      }
    }
    else if (!filterChanged) {
      filterChanged = PR_TRUE;
    }

    fs.values.Clear();
    rv = ConfigureArray(i);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mMediaListView) {
    rv = mMediaListView->UpdateViewArrayConfiguration(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // And notify the view's listeners
    if (filterChanged) {
      mMediaListView->NotifyListenersFilterChanged();
    }
    if (searchChanged) {
      mMediaListView->NotifyListenersSearchChanged();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetValues(PRUint16 aIndex,
                                           nsIStringEnumerator **_retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetValues", this));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  sbGUIDArrayPrimarySortEnumerator* values =
    new sbGUIDArrayPrimarySortEnumerator(mFilters[aIndex].array);
  NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = values);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetValueAt(PRUint16 aIndex,
                                            PRUint32 aValueIndex,
                                            nsAString& aValue)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetValueAt", this));
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  mFilters[aIndex].array->GetSortPropertyValueByIndex(aValueIndex, aValue);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetTreeView(PRUint16 aIndex,
                                             nsITreeView **_retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetTreeView", this));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_STATE(mMediaListView);

  sbFilterSpec& fs = mFilters[aIndex];

  if (fs.isSearch) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!fs.treeView) {

    nsresult rv;

    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propArray->SetStrict(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propArray->AppendProperty(fs.property, NS_LITERAL_STRING("a"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbLocalDatabaseTreeView> newTreeView =
      new sbLocalDatabaseTreeView();
    NS_ENSURE_TRUE(newTreeView, NS_ERROR_OUT_OF_MEMORY);

    rv = newTreeView->Init(mMediaListView, fs.array, propArray, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    fs.treeView = newTreeView;
  }

  NS_ADDREF(*_retval = fs.treeView);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetValueCount(PRUint16 aIndex,
                                               PRBool aUseCache,
                                               PRUint32 *_retval)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetValueCount", this));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  if (aUseCache) {
    *_retval = mFilters[aIndex].cachedValueCount;
  }
  else {
    PRUint32 valueCount;
    nsresult rv = mFilters[aIndex].array->GetLength(&valueCount);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = OnGetLength(aIndex, valueCount);
    NS_ENSURE_SUCCESS(rv, rv);
    *_retval = valueCount;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AddListener(sbICascadeFilterSetListener* aListener)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - AddListener", this));
  nsISupportsHashKey* success = mListeners.PutEntry(aListener);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::RemoveListener(sbICascadeFilterSetListener* aListener)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - RemoveListener", this));

  mListeners.RemoveEntry(aListener);

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::AddConfiguration(sbILocalDatabaseGUIDArray* mArray)
{
  NS_ENSURE_ARG_POINTER(mArray);
  nsresult rv;

  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    const sbFilterSpec& filter = mFilters[i];

    // Skip filters and searches that have no values
    if (filter.values.Length() == 0) {
      continue;
    }

    if (filter.isSearch) {

      // A search can be over many properties (held in propertyList).  Add a
      // search filter for each property being searched.  The value being
      // searched for is the first element of the values array.  Note that
      // we make the values to search for sortable
      PRUint32 length = filter.propertyList.Length();
      for (PRUint32 j = 0; j < length; j++) {
        nsCOMPtr<sbIPropertyInfo> info;
        rv = propMan->GetPropertyInfo(filter.propertyList[j],
                                      getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 valuesLength = filter.values.Length();
        sbStringArray valueArray(valuesLength);

        for(PRUint32 k = 0; k < valuesLength; k++) {
          nsString sortableValue;
          rv = info->MakeSortable(filter.values[k], sortableValue);
          NS_ENSURE_SUCCESS(rv, rv);
          nsString* successString = valueArray.AppendElement(sortableValue);
          NS_ENSURE_TRUE(successString, NS_ERROR_OUT_OF_MEMORY);
        }

        nsCOMPtr<nsIStringEnumerator> valueEnum =
          new sbTArrayStringEnumerator(&valueArray);
        NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

        rv = mArray->AddFilter(filter.propertyList[j], valueEnum, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      nsCOMPtr<sbIPropertyInfo> info;
      rv = propMan->GetPropertyInfo(filter.property, getter_AddRefs(info));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length = filter.values.Length();
      sbStringArray sortableValueArray(length);
      for (PRUint32 i = 0; i < length; i++) {
        nsAutoString sortableValue;
        if (SB_IsTopLevelProperty(filter.property)) {
          sortableValue = filter.values[i];
        }
        else {
          rv = info->MakeSortable(filter.values[i], sortableValue);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        nsString* appended = sortableValueArray.AppendElement(sortableValue);
        NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
      }

      nsCOMPtr<nsIStringEnumerator> valueEnum =
        new sbTArrayStringEnumerator(&sortableValueArray);
      NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

      rv = mArray->AddFilter(filter.property, valueEnum, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::AddFilters(sbILibraryConstraintBuilder* aBuilder,
                                            PRBool* aChanged)
{
  NS_ENSURE_ARG_POINTER(aBuilder);
  NS_ENSURE_ARG_POINTER(aChanged);
  nsresult rv;

  *aChanged = PR_FALSE;
  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    const sbFilterSpec& filter = mFilters[i];

    if (!filter.isSearch && filter.values.Length()) {
      *aChanged = PR_TRUE;

      rv = aBuilder->Intersect(nsnull);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIStringEnumerator> values =
        new sbTArrayStringEnumerator(&filter.values);
      NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

      rv = aBuilder->IncludeList(filter.property, values, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::AddSearches(sbILibraryConstraintBuilder* aBuilder,
                                             PRBool* aChanged)
{
  NS_ENSURE_ARG_POINTER(aBuilder);
  NS_ENSURE_ARG_POINTER(aChanged);
  nsresult rv;

  *aChanged = PR_FALSE;
  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    const sbFilterSpec& filter = mFilters[i];

    if (filter.isSearch && filter.values.Length()) {

      PRUint32 numProperties = filter.propertyList.Length();
      PRUint32 numValues = filter.values.Length();
      for (PRUint32 j = 0; j < numValues; j++) {
        *aChanged = PR_TRUE;
        for (PRUint32 k = 0; k < numProperties; k++) {
          rv = aBuilder->Include(filter.propertyList[k],
                                 filter.values[j],
                                 nsnull);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        if (j + 1 < numValues) {
          rv = aBuilder->Intersect(nsnull);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::ClearFilters()
{
  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    sbFilterSpec& filter = mFilters[i];

    if (!filter.isSearch) {
      filter.values.Clear();
    }
  }
  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::ClearSearches()
{
  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    sbFilterSpec& filter = mFilters[i];

    if (filter.isSearch) {
      filter.values.Clear();
    }
  }
  return NS_OK;
}

void
sbLocalDatabaseCascadeFilterSet::ClearMediaListView()
{
  mMediaListView = nsnull;
}

nsresult
sbLocalDatabaseCascadeFilterSet::GetState(sbLocalDatabaseCascadeFilterSetState** aState)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - GetState", this));
  NS_ASSERTION(aState, "aState is null");

  nsRefPtr<sbLocalDatabaseCascadeFilterSetState> state =
    new sbLocalDatabaseCascadeFilterSetState();
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    sbFilterSpec& fs = mFilters[i];

    sbLocalDatabaseCascadeFilterSetState::Spec* spec =
      state->mFilters.AppendElement();
    NS_ENSURE_TRUE(spec, NS_ERROR_OUT_OF_MEMORY);

    spec->isSearch = fs.isSearch;
    spec->property = fs.property;

    nsString* added = spec->propertyList.AppendElements(fs.propertyList);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

    added = spec->values.AppendElements(fs.values);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

      if (fs.treeView) {
        rv = fs.treeView->GetState(getter_AddRefs(spec->treeViewState));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

#ifdef DEBUG
    {
      nsString buff;
      state->ToString(buff);
      nsCOMPtr<nsISupports> supports = do_QueryInterface(state);
      TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - returning state [0x%.8x] %s",
             this, supports.get(), NS_LossyConvertUTF16toASCII(buff).get()));
    }
#endif

  NS_ADDREF(*aState = state);
  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::OnGetLength(PRUint32 aIndex,
                                             PRUint32 aLength)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - OnGetLength(%d, %d)",
         this, aIndex, aLength));
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  PRUint32 index = aIndex;
  if (aLength != mFilters[index].cachedValueCount) {
    mFilters[index].cachedValueCount = aLength;

    // Notify listeners
    mListeners.EnumerateEntries(OnValuesChangedCallback, &index);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::ConfigureArray(PRUint32 aIndex)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - ConfigureArray", this));
  NS_ENSURE_TRUE(aIndex < mFilters.Length(), NS_ERROR_INVALID_ARG);

  nsresult rv;

  sbFilterSpec& fs = mFilters[aIndex];
  fs.arrayListener->mIndex = aIndex;

  // Clear this filter since our upstream filters have changed
  rv = fs.array->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Apply the filters of each upstream filter to this filter
  for (PRUint32 i = 0; i < aIndex; i++) {
    const sbFilterSpec& upstream = mFilters[i];

    if (upstream.values.Length() > 0) {

      if(upstream.isSearch) {

        for (PRUint32 j = 0; j < upstream.propertyList.Length(); j++) {

          nsCOMPtr<sbIPropertyInfo> info;
          rv = propMan->GetPropertyInfo(upstream.propertyList[j],
                                        getter_AddRefs(info));
          NS_ENSURE_SUCCESS(rv, rv);

          sbStringArray sortableValues;
          for (PRUint32 k = 0; k < upstream.values.Length(); k++) {
            nsAutoString sortableValue;
            rv = info->MakeSortable(upstream.values[k], sortableValue);
            NS_ENSURE_SUCCESS(rv, rv);

            nsString* success = sortableValues.AppendElement(sortableValue);
            NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
          }

          nsCOMPtr<nsIStringEnumerator> values =
            new sbTArrayStringEnumerator(const_cast<sbStringArray*>(
                                                       &sortableValues));
          NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

          rv = fs.array->AddFilter(upstream.propertyList[j],
                                   values,
                                   PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);

        }

      }
      else {
        nsCOMPtr<sbIPropertyInfo> info;
        rv = propMan->GetPropertyInfo(upstream.property, getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);

        sbStringArray sortableValues;
        for (PRUint32 k = 0; k < upstream.values.Length(); k++) {
          nsAutoString sortableValue;
          rv = info->MakeSortable(upstream.values[k], sortableValue);
          NS_ENSURE_SUCCESS(rv, rv);

          nsString* success = sortableValues.AppendElement(sortableValue);
          NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
        }

        nsCOMPtr<nsIStringEnumerator> values =
          new sbTArrayStringEnumerator(const_cast<sbStringArray*>(
                                                     &sortableValues));
        NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

        rv = fs.array->AddFilter(upstream.property,
                                 values,
                                 PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::ConfigureFilterArray(sbFilterSpec* aSpec,
                                                      const nsAString& aSortProperty)
{
  NS_ASSERTION(aSpec, "aSpec is null");

  nsresult rv;

  rv = mProtoArray->CloneAsyncArray(getter_AddRefs(aSpec->array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aSpec->array->AddSort(aSortProperty, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NEWXPCOM(aSpec->arrayListener, sbLocalDatabaseCascadeFilterSetArrayListener);
  NS_ENSURE_TRUE(aSpec->arrayListener, NS_ERROR_OUT_OF_MEMORY);
  rv = aSpec->arrayListener->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aSpec->array->AddAsyncListener(aSpec->arrayListener);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabasePropertyCache> propertyCache;
  rv = mLibrary->GetPropertyCache(getter_AddRefs(propertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aSpec->array->SetPropertyCache(propertyCache);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::InvalidateFilter(sbFilterSpec& aFilter)
{
  LOG(("sbLocalDatabaseCascadeFilterSet[0x%.8x] - Invalidating %s",
       this, NS_ConvertUTF16toUTF8(aFilter.property).get()));

  nsresult rv = aFilter.array->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aFilter.treeView) {
    rv = aFilter.treeView->Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  aFilter.invalidationPending = PR_FALSE;

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::UpdateListener(PRBool aRemoveListener)
{
  NS_ENSURE_STATE(mMediaList);

  nsresult rv;

  nsCOMPtr<sbIMediaListListener> listener =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaListListener*, this));

  if (aRemoveListener) {
    rv = mMediaList->RemoveListener(listener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIMutablePropertyArray> filter =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString voidString;
  voidString.SetIsVoid(PR_TRUE);

  // Filter this listener on the properties of all the filters
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];
    if (!fs.isSearch) {
      rv = filter->AppendProperty(mFilters[i].property, voidString);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Use a weak reference here since we have an owning reference to the view
  // which has an owning reference to the list
  rv = mMediaList->AddListener(listener,
                               PR_TRUE,
                               sbIMediaList::LISTENER_FLAGS_ALL &
                                 ~sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED,
                               filter);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbIMediaListListener
NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnItemAdded(sbIMediaList* aMediaList,
                                             sbIMediaItem* aMediaItem,
                                             PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // If we are in a batch, set all the filters to invalidate and ignore
  // any future adds in this batch
  if (mBatchHelper.IsActive()) {
    for (PRUint32 i = 0; i < mFilters.Length(); i++) {
      mFilters[i].invalidationPending = PR_TRUE;
    }
    *aNoMoreForBatch = PR_TRUE;
    return NS_OK;
  }

  // Only need to invalidate a filter list if the new item has a value for
  // its property
  nsresult rv;
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];

    nsAutoString junk;
    rv = aMediaItem->GetProperty(fs.property, junk);
    if (NS_SUCCEEDED(rv) && !junk.IsVoid()) {
      rv = InvalidateFilter(fs);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                     sbIMediaItem* aMediaItem,
                                                     PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                    sbIMediaItem* aMediaItem,
                                                    PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // If we are in a batch, set all the filters to invalidate the ignore
  // any future adds in this batch
  if (mBatchHelper.IsActive()) {
    for (PRUint32 i = 0; i < mFilters.Length(); i++) {
      mFilters[i].invalidationPending = PR_TRUE;
    }
    *aNoMoreForBatch = PR_TRUE;
    return NS_OK;
  }

  // Invalidate a filter only when the removed item has a value in its
  // property
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];
    nsAutoString junk;
    nsresult rv = aMediaItem->GetProperty(fs.property, junk);
    if (NS_SUCCEEDED(rv) && !junk.IsVoid()) {
      rv = InvalidateFilter(fs);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnItemUpdated(sbIMediaList* aMediaList,
                                               sbIMediaItem* aMediaItem,
                                               sbIPropertyArray* aProperties,
                                               PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  nsresult rv;

  // Invalidate any filters whose property was updated
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];

    nsAutoString junk;
    rv = aProperties->GetPropertyValue(fs.property, junk);
    if (NS_SUCCEEDED(rv)) {
      if (mBatchHelper.IsActive()) {
        fs.invalidationPending = PR_TRUE;
      }
      else {
        rv = InvalidateFilter(fs);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnListCleared(sbIMediaList* aMediaList,
                                               PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Invalidate all filters
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];
    if (mBatchHelper.IsActive()) {
      fs.invalidationPending = PR_TRUE;
    }
    else {
      nsresult rv = InvalidateFilter(fs);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnBatchBegin(sbIMediaList* aMediaList)
{
  mBatchHelper.Begin();
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::OnBatchEnd(sbIMediaList* aMediaList)
{
  mBatchHelper.End();

  // Do all pending invalidations
  for (PRUint32 i = 0; i < mFilters.Length(); i++) {
    sbFilterSpec& fs = mFilters[i];
    if (fs.invalidationPending) {
      nsresult rv = InvalidateFilter(fs);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}


PLDHashOperator PR_CALLBACK
sbLocalDatabaseCascadeFilterSet::OnValuesChangedCallback(nsISupportsHashKey* aKey,
                                                         void* aUserData)
{
  TRACE(("sbLocalDatabaseCascadeFilterSet[static] - OnValuesChangedCallback"));
  NS_ASSERTION(aKey && aUserData, "Args should not be null!");

  nsresult rv;
  nsCOMPtr<sbICascadeFilterSetListener> listener =
    do_QueryInterface(aKey->GetKey(), &rv);

  if (NS_SUCCEEDED(rv)) {
    PRUint32* index = static_cast<PRUint32*>(aUserData);
    rv = listener->OnValuesChanged(*index);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnValuesChanged returned a failure code");
  }
  return PL_DHASH_NEXT;
}

NS_IMPL_ISUPPORTS2(sbLocalDatabaseCascadeFilterSetArrayListener,
                   nsISupportsWeakReference,
                   sbILocalDatabaseAsyncGUIDArrayListener)

nsresult sbLocalDatabaseCascadeFilterSetArrayListener::Init
                            (sbLocalDatabaseCascadeFilterSet *aCascadeFilterSet)
{
  nsresult rv;

  mWeakCascadeFilterSet =
    do_GetWeakReference
      (NS_ISUPPORTS_CAST(sbICascadeFilterSet*, aCascadeFilterSet), &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mCascadeFilterSet = aCascadeFilterSet;

  return NS_OK;
}

// sbILocalDatabaseAsyncGUIDArrayListener
NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetArrayListener::OnGetLength(PRUint32 aLength,
                                                          nsresult aResult)
{
  TRACE(("sbLocalDatabaseCascadeFilterSetArrayListener[0x%.8x] - "
         "OnGetLength(%d)", this, aLength));

  nsresult rv;

  if (NS_SUCCEEDED(aResult)) {
    nsCOMPtr<nsISupports> cascadeFilterSetRef =
                                  do_QueryReferent(mWeakCascadeFilterSet, &rv);
    if (cascadeFilterSetRef) {
#ifdef DEBUG
      nsCOMPtr<nsISupports> classPtr = do_QueryInterface
              (NS_ISUPPORTS_CAST(sbICascadeFilterSet*, mCascadeFilterSet), &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(classPtr == cascadeFilterSetRef,
                   "Weak reference and class pointer are out of sync!");
#endif
      rv = mCascadeFilterSet->OnGetLength(mIndex, aLength);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetArrayListener::OnGetGuidByIndex
                                                        (PRUint32 aIndex,
                                                         const nsAString& aGUID,
                                                         nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetArrayListener::OnGetSortPropertyValueByIndex
                                                        (PRUint32 aIndex,
                                                         const nsAString& aGUID,
                                                         nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetArrayListener::OnGetMediaItemIdByIndex
                                                        (PRUint32 aIndex,
                                                         PRUint32 aMediaItemId,
                                                         nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetArrayListener::OnStateChange(PRUint32 aState)
{
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(sbGUIDArrayPrimarySortEnumerator, nsIStringEnumerator)

sbGUIDArrayPrimarySortEnumerator::sbGUIDArrayPrimarySortEnumerator(sbILocalDatabaseAsyncGUIDArray* aArray) :
  mArray(aArray),
  mNextIndex(0)
{
}

NS_IMETHODIMP
sbGUIDArrayPrimarySortEnumerator::HasMore(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  PRUint32 length;
  rv = mArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mNextIndex < length;
  return NS_OK;
}

NS_IMETHODIMP
sbGUIDArrayPrimarySortEnumerator::GetNext(nsAString& _retval)
{
  nsresult rv;

  rv = mArray->GetSortPropertyValueByIndex(mNextIndex, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  mNextIndex++;
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(sbLocalDatabaseCascadeFilterSetState,
                   nsISerializable);

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetState::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  mFilters.Clear();

  PRUint32 length;
  rv = aStream->Read32(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    Spec* spec = mFilters.AppendElement();
    NS_ENSURE_TRUE(spec, NS_ERROR_OUT_OF_MEMORY);

    rv = aStream->ReadBoolean(&spec->isSearch);
      NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->ReadString(spec->property);
      NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyListLength;
    rv = aStream->Read32(&propertyListLength);
    for (PRUint32 j = 0; j < propertyListLength; j++) {
      nsString property;
      rv = aStream->ReadString(property);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString* added = spec->propertyList.AppendElement(property);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }

    PRUint32 valuesength;
    rv = aStream->Read32(&valuesength);
    for (PRUint32 j = 0; j < valuesength; j++) {
      nsString value;
      rv = aStream->ReadString(value);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString* added = spec->values.AppendElement(value);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }

    PRBool hasTreeViewState;
    rv = aStream->ReadBoolean(&hasTreeViewState);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasTreeViewState) {
      spec->treeViewState = new sbLocalDatabaseTreeViewState();
      NS_ENSURE_TRUE(spec->treeViewState, NS_ERROR_OUT_OF_MEMORY);

      rv = spec->treeViewState->Init();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = spec->treeViewState->Read(aStream);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSetState::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  PRUint32 length = mFilters.Length();
  rv = aStream->Write32(length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    Spec& fs = mFilters[i];

    rv = aStream->WriteBoolean(fs.isSearch);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->WriteWStringZ(fs.property.BeginReading());
      NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyListLength = fs.propertyList.Length();
    rv = aStream->Write32(propertyListLength);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 j = 0; j < propertyListLength; j++) {
      rv = aStream->WriteWStringZ(fs.propertyList[j].BeginReading());
        NS_ENSURE_SUCCESS(rv, rv);
      }

    PRUint32 valuesLength = fs.values.Length();
    rv = aStream->Write32(valuesLength);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 j = 0; j < valuesLength; j++) {
      rv = aStream->WriteWStringZ(fs.values[j].BeginReading());
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (fs.treeViewState) {
      rv = aStream->WriteBoolean(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = fs.treeViewState->Write(aStream);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = aStream->WriteBoolean(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSetState::ToString(nsAString& aStr)
{
  nsresult rv;
  nsString buff;

  PRUint32 numFilters = mFilters.Length();
  for (PRUint32 i = 0; i < numFilters; i++) {
    Spec& fs = mFilters[i];

    if (fs.isSearch) {
      buff.AppendLiteral("search [[");
      PRUint32 propertyListLength = fs.propertyList.Length();
      for (PRUint32 j = 0; j < propertyListLength; j++) {
        buff.Append(fs.propertyList[j]);
        if (j + 1 < propertyListLength) {
          buff.AppendLiteral(", ");
        }
      }
      buff.AppendLiteral("] ");
    }
    else {
      buff.AppendLiteral("filter [");
      buff.Append(fs.property);
      buff.AppendLiteral(" ");
    }

    buff.AppendLiteral("values [");

    PRUint32 valesLength = fs.values.Length();
    for (PRUint32 j = 0; j < valesLength; j++) {
      buff.Append(fs.values[j]);
      if (j + 1 < valesLength) {
        buff.AppendLiteral(", ");
      }
      }
      buff.AppendLiteral("]");

    if (fs.treeViewState) {
      buff.AppendLiteral("treeView: ");
      nsString tmp;
      rv = fs.treeViewState->ToString(tmp);
      NS_ENSURE_SUCCESS(rv, rv);
      buff.Append(tmp);
    }

    if (i + 1 < numFilters) {
      buff.AppendLiteral(", ");
    }
  }

  aStr = buff;

  return NS_OK;
}
