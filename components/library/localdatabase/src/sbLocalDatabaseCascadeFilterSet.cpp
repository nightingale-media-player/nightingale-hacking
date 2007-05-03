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

#include "sbLocalDatabaseCascadeFilterSet.h"

#include <nsISimpleEnumerator.h>
#include <nsITreeView.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIFilterableMediaList.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISearchableMediaList.h>
#include <sbISQLBuilder.h>

#include <DatabaseQuery.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseTreeView.h"
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbTArrayStringEnumerator.h>

NS_IMPL_ISUPPORTS1(sbLocalDatabaseCascadeFilterSet,
                   sbICascadeFilterSet);

nsresult
sbLocalDatabaseCascadeFilterSet::Init(sbILocalDatabaseLibrary* aLibrary,
                                      sbIMediaListView* aMediaListView,
                                      sbILocalDatabaseAsyncGUIDArray* aProtoArray)
{
  NS_ENSURE_ARG_POINTER(aMediaListView);
  NS_ENSURE_ARG_POINTER(aProtoArray);

  nsresult rv;

  mLibrary = aLibrary;

  mMediaListView = aMediaListView;

  mProtoArray = aProtoArray;

  rv = mProtoArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->SetIsDistinct(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mListeners.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetLength(PRUint16* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = mFilters.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetProperty(PRUint16 aIndex,
                                             nsAString& _retval)
{
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  _retval = mFilters[aIndex].property;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::IsSearch(PRUint16 aIndex,
                                          PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  *_retval = mFilters[aIndex].isSearch;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AppendFilter(const nsAString& aProperty,
                                              PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  sbFilterSpec* fs = mFilters.AppendElement();
  NS_ENSURE_TRUE(fs, NS_ERROR_OUT_OF_MEMORY);

  fs->isSearch = PR_FALSE;
  fs->property = aProperty;

  rv = mProtoArray->CloneAsyncArray(getter_AddRefs(fs->array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fs->array->AddSort(aProperty, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ConfigureArray(mFilters.Length() - 1);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mFilters.Length() - 1;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AppendSearch(const PRUnichar** aPropertyArray,
                                              PRUint32 aPropertyArrayCount,
                                              PRUint16 *_retval)
{
  if (aPropertyArrayCount) {
    NS_ENSURE_ARG_POINTER(aPropertyArray);
  }
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

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

  rv = mProtoArray->CloneAsyncArray(getter_AddRefs(fs->array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fs->array->AddSort(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created"),
                          PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ConfigureArray(mFilters.Length() - 1);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mFilters.Length() - 1;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::Remove(PRUint16 aIndex)
{
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  nsresult rv;

  mFilters.RemoveElementAt(aIndex);

  // Update the filters downstream from the removed filter
  for (PRUint32 i = aIndex; i < mFilters.Length(); i++) {
    rv = ConfigureArray(i);
    NS_ENSURE_SUCCESS(rv, rv);

    // Notify listeners
    mListeners.EnumerateEntries(OnValuesChangedCallback, &i);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::Set(PRUint16 aIndex,
                                     const PRUnichar** aValueArray,
                                     PRUint32 aValueArrayCount)
{
  if (aValueArrayCount) {
    NS_ENSURE_ARG_POINTER(aValueArray);
  }
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  nsresult rv;

  nsCOMPtr<sbIPropertyArray> filter =
    do_CreateInstance(SB_PROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbFilterSpec& fs = mFilters[aIndex];
  fs.values.Clear();

  for (PRUint32 i = 0; i < aValueArrayCount; i++) {
    if (aValueArray[i]) {
      nsString* value = fs.values.AppendElement(aValueArray[i]);
      NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);

      // If this is a search, we need to add this search value for each
      // property being searched
      if (fs.isSearch) {
        for (PRUint32 j = 0; j < fs.propertyList.Length(); j++) {
          rv = filter->AppendProperty(fs.propertyList[j], *value);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        rv = filter->AppendProperty(fs.property, *value);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      NS_WARNING("Null pointer passed in array");
    }
  }

  // Update downstream filters
  for (PRUint32 i = aIndex + 1; i < mFilters.Length(); i++) {
    rv = ConfigureArray(i);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mFilters[i].treeView) {
      sbLocalDatabaseTreeView* view = NS_STATIC_CAST(sbLocalDatabaseTreeView*, mFilters[i].treeView.get());
      rv = view->Rebuild();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Notify listeners
    mListeners.EnumerateEntries(OnValuesChangedCallback, &i);
  }

  // Update the associated view with the new filter or search setting
  if (fs.isSearch) {
    nsCOMPtr<sbISearchableMediaList> searchable =
      do_QueryInterface(mMediaListView, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aValueArrayCount == 0) {
      rv = searchable->ClearSearch();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = searchable->SetSearch(filter);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    nsCOMPtr<sbIFilterableMediaList> filterable =
      do_QueryInterface(mMediaListView, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aValueArrayCount == 0) {
      rv = filter->AppendProperty(fs.property, EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = filterable->SetFilters(filter);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetValues(PRUint16 aIndex,
                                           nsIStringEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

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
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  mFilters[aIndex].array->GetSortPropertyValueByIndex(aValueIndex, aValue);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetTreeView(PRUint16 aIndex,
                                             nsITreeView **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  sbFilterSpec& fs = mFilters[aIndex];

  if (fs.isSearch) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!fs.treeView) {

    nsresult rv;

    nsCOMPtr<sbILocalDatabasePropertyCache> propertyCache;
    rv = mLibrary->GetPropertyCache(getter_AddRefs(propertyCache));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fs.array->SetPropertyCache(propertyCache);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<sbLocalDatabaseTreeView> treeView(new sbLocalDatabaseTreeView());
    NS_ENSURE_TRUE(treeView, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIPropertyArray> propArray =
      do_CreateInstance("@songbirdnest.com/Songbird/Properties/PropertyArray;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propArray->AppendProperty(fs.property, NS_LITERAL_STRING("a"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = treeView->Init(mMediaListView, fs.array, propArray);
    NS_ENSURE_SUCCESS(rv, rv);

    fs.treeView = treeView.forget();
  }

  NS_ADDREF(*_retval = fs.treeView);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetValueCount(PRUint16 aIndex,
                                               PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  nsresult rv = mFilters[aIndex].array->GetLength(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::AddListener(sbICascadeFilterSetListener* aListener)
{
  nsISupportsHashKey* success = mListeners.PutEntry(aListener);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::RemoveListener(sbICascadeFilterSetListener* aListener)
{

  mListeners.RemoveEntry(aListener);

  return NS_OK;
}

nsresult
sbLocalDatabaseCascadeFilterSet::ConfigureArray(PRUint32 aIndex)
{
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  nsresult rv;

  const sbFilterSpec& fs = mFilters[aIndex];
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
            new sbTArrayStringEnumerator(NS_CONST_CAST(sbStringArray*,
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
          new sbTArrayStringEnumerator(NS_CONST_CAST(sbStringArray*,
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

PLDHashOperator PR_CALLBACK
sbLocalDatabaseCascadeFilterSet::OnValuesChangedCallback(nsISupportsHashKey* aKey,
                                                         void* aUserData)
{
  NS_ASSERTION(aKey && aUserData, "Args should not be null!");

  nsresult rv;
  nsCOMPtr<sbICascadeFilterSetListener> listener =
    do_QueryInterface(aKey->GetKey(), &rv);

  if (NS_SUCCEEDED(rv)) {
    PRUint32* index = NS_STATIC_CAST(PRUint32*, aUserData);
    rv = listener->OnValuesChanged(*index);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnValuesChanged returned a failure code");
  }
  return PL_DHASH_NEXT;
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

