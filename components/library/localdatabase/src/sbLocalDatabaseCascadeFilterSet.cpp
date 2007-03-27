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

#include <DatabaseQuery.h>
#include <nsComponentManagerUtils.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbLocalDatabasePropertyCache.h>
#include <nsISimpleEnumerator.h>

NS_IMPL_ISUPPORTS1(sbLocalDatabaseCascadeFilterSet,
                   sbICascadeFilterSet);

NS_METHOD
sbLocalDatabaseCascadeFilterSet::Init(sbIMediaList* aMediaList,
                                      sbILocalDatabaseGUIDArray* aProtoArray)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aProtoArray);

  nsresult rv;

  mMediaList = aMediaList;

  mProtoArray = aProtoArray;

  rv = mProtoArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProtoArray->SetIsDistinct(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

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

  rv = mProtoArray->Clone(getter_AddRefs(fs->array));
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

  rv = mProtoArray->Clone(getter_AddRefs(fs->array));
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

  sbFilterSpec& fs = mFilters[aIndex];
  fs.values.Clear();

  for (PRUint32 i = 0; i < aValueArrayCount; i++) {
    if (aValueArray[i]) {
      nsString* success = fs.values.AppendElement(aValueArray[i]);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_WARNING("Null pointer passed in array");
    }
  }

  // Update downstream filters
  for (PRUint32 i = aIndex + 1; i < mFilters.Length(); i++) {
    rv = ConfigureArray(i);
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

  sbGUIDArrayPrimraySortEnumerator* values =
    new sbGUIDArrayPrimraySortEnumerator(mFilters[aIndex].array);
  NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = values);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::GetLength(PRUint16 aIndex,
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
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseCascadeFilterSet::RemoveListener(sbICascadeFilterSetListener* aListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
sbLocalDatabaseCascadeFilterSet::ConfigureArray(PRUint32 aIndex)
{
  NS_ENSURE_ARG_MAX(aIndex, mFilters.Length() - 1);

  nsresult rv;

  const sbFilterSpec& fs = mFilters[aIndex];
  rv = fs.array->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  // Apply the filters of each upstream filter to this filter
  for (PRUint32 i = 0; i < aIndex; i++) {
    const sbFilterSpec& upstream = mFilters[i];

    if (upstream.values.Length() > 0) {

      if(upstream.isSearch) {

        for (PRUint32 j = 0; j < upstream.propertyList.Length(); j++) {
          nsCOMPtr<nsIStringEnumerator> values =
            new sbTArrayStringEnumerator(NS_CONST_CAST(sbStringArray*,
                                                       &upstream.values));
          NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);
  
          rv = fs.array->AddFilter(upstream.propertyList[j],
                                   values,
                                   PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);

        }

      }
      else {
        nsCOMPtr<nsIStringEnumerator> values =
          new sbTArrayStringEnumerator(NS_CONST_CAST(sbStringArray*,
                                                     &upstream.values));
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

NS_IMPL_ISUPPORTS1(sbGUIDArrayPrimraySortEnumerator, nsIStringEnumerator)

sbGUIDArrayPrimraySortEnumerator::sbGUIDArrayPrimraySortEnumerator(sbILocalDatabaseGUIDArray* aArray) :
  mArray(aArray),
  mNextIndex(0)
{
}

NS_IMETHODIMP
sbGUIDArrayPrimraySortEnumerator::HasMore(PRBool *_retval)
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
sbGUIDArrayPrimraySortEnumerator::GetNext(nsAString& _retval)
{
  nsresult rv;

  rv = mArray->GetSortPropertyValueByIndex(mNextIndex, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  mNextIndex++;
  return NS_OK;
}

