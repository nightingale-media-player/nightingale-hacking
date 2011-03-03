/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
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


#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabasePropertyCache.h"

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCIDInternal.h>
#include <prlog.h>

#include <sbIPropertyInfo.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include "sbLocalDatabaseSQL.h"
#include <sbTArrayStringEnumerator.h>
#include <sbStringUtils.h>
#include <sbDebugUtils.h>

PRUint32 const BAG_HASHTABLE_SIZE = 20;

// sbILocalDatabaseResourcePropertyBag
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseResourcePropertyBag,
                              sbILocalDatabaseResourcePropertyBag)

sbLocalDatabaseResourcePropertyBag::sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                                                       PRUint32 aMediaItemId,
                                                                       const nsAString &aGuid)
: mCache(aCache)
, mGuid(aGuid)
, mMediaItemId(aMediaItemId)
{
  SB_PRLOG_SETUP(sbLocalDatabaseResourcePropertyBag);
}

sbLocalDatabaseResourcePropertyBag::~sbLocalDatabaseResourcePropertyBag()
{
}

nsresult
sbLocalDatabaseResourcePropertyBag::Init()
{
  nsresult rv;

  PRBool success = mValueMap.Init(BAG_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(BAG_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mIdService =
    do_GetService("@songbirdnest.com/Songbird/IdentityService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseResourcePropertyBag::PropertyBagKeysToArray(const PRUint32& aPropertyID,
                                                           sbPropertyData* aPropertyData,
                                                           void *aArg)
{
  nsTArray<PRUint32>* propertyIDs = static_cast<nsTArray<PRUint32>*>(aArg);
  if (propertyIDs->AppendElement(aPropertyID)) {
    return PL_DHASH_NEXT;
  }
  else {
    return PL_DHASH_STOP;
  }
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetGuid(nsAString &aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetMediaItemId(PRUint32* aMediaItemId)
{
  NS_ENSURE_ARG_POINTER(aMediaItemId);
  *aMediaItemId = mMediaItemId;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetIds(nsIStringEnumerator **aIDs)
{
  NS_ENSURE_ARG_POINTER(aIDs);

  nsTArray<PRUint32> propertyDBIDs;
  mValueMap.EnumerateRead(PropertyBagKeysToArray, &propertyDBIDs);

  PRUint32 len = mValueMap.Count();
  if (propertyDBIDs.Length() < len) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<nsString> propertyIDs;
  for (PRUint32 i = 0; i < len; i++) {
    nsString propertyID;
    PRBool success = mCache->GetPropertyID(propertyDBIDs[i], propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
    propertyIDs.AppendElement(propertyID);
  }

  *aIDs = new sbTArrayStringEnumerator(&propertyIDs);
  NS_ENSURE_TRUE(*aIDs, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aIDs);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetProperty(const nsAString& aPropertyID,
                                                nsAString& _retval)
{
  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);
  return GetPropertyByID(propertyDBID, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetPropertyByID(PRUint32 aPropertyDBID,
                                                    nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoMonitor mon(mCache->mMonitor);

    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {
      _retval.Assign(data->value);
      return NS_OK;
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetSortablePropertyByID(PRUint32 aPropertyDBID,
                                                            nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoMonitor mon(mCache->mMonitor);
    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {

      // Generate and cache the sortable value
      // only when needed
      if (data->sortableValue.IsEmpty()) {
        nsString propertyID;
        PRBool success = mCache->GetPropertyID(aPropertyDBID, propertyID);
        NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
        nsCOMPtr<sbIPropertyInfo> propertyInfo;
        nsresult rv = mPropertyManager->GetPropertyInfo(propertyID,
                                                 getter_AddRefs(propertyInfo));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = propertyInfo->MakeSortable(data->value, data->sortableValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      _retval.Assign(data->sortableValue);
      return NS_OK;
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::
  GetSearchablePropertyByID(PRUint32 aPropertyDBID,
                            nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoMonitor mon(mCache->mMonitor);
    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {

      // Generate and cache the searchable value
      // only when needed
      if (data->searchableValue.IsEmpty()) {
        nsString propertyID;
        PRBool success = mCache->GetPropertyID(aPropertyDBID, propertyID);
        NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
        nsCOMPtr<sbIPropertyInfo> propertyInfo;
        nsresult rv = mPropertyManager->GetPropertyInfo(propertyID,
                                                 getter_AddRefs(propertyInfo));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = propertyInfo->MakeSearchable(data->value, data->searchableValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      _retval.Assign(data->searchableValue);
      return NS_OK;
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::SetProperty(const nsAString & aPropertyID,
                                                const nsAString & aValue)
{
  nsresult rv;

  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);
  if(propertyDBID == 0) {
    rv = mCache->InsertPropertyIDInLibrary(aPropertyID, &propertyDBID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = mPropertyManager->GetPropertyInfo(aPropertyID,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool valid = PR_FALSE;
  rv = propertyInfo->Validate(aValue, &valid);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if(NS_UNLIKELY(!valid)) {
    LOG("Failed to set property id %s with value %s",
        NS_ConvertUTF16toUTF8(aPropertyID).get(),
        NS_ConvertUTF16toUTF8(aValue).get());
  }
#endif

  NS_ENSURE_TRUE(valid, NS_ERROR_ILLEGAL_VALUE);

  // Find all properties whose secondary sort depends on this
  // property
  nsCOMPtr<sbIPropertyArray> dependentProperties;
  rv = mPropertyManager->GetDependentProperties(aPropertyID,
            getter_AddRefs(dependentProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 dependentPropertyCount;
  rv = dependentProperties->GetLength(&dependentPropertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 previousDirtyCount = 0;
  {
    nsAutoMonitor mon(mCache->mMonitor);

    rv = PutValue(propertyDBID, aValue);
    NS_ENSURE_SUCCESS(rv, rv);

    previousDirtyCount = mDirty.Count();

    // Mark the property that changed as dirty
    mDirty.PutEntry(propertyDBID);
    // Mark the property that changed as dirty for invalidation of guid arrays.
    mDirtyForInvalidation.insert(propertyDBID);

    // Also mark as dirty any properties that use
    // the changed property in their secondary sort values
    if (dependentPropertyCount > 0) {
      for (PRUint32 i = 0; i < dependentPropertyCount; i++) {
        nsCOMPtr<sbIProperty> property;
        rv = dependentProperties->GetPropertyAt(i, getter_AddRefs(property));
        NS_ASSERTION(NS_SUCCEEDED(rv),
            "Property cache failed to update dependent properties!");
        if (NS_SUCCEEDED(rv)) {
          nsString propertyID;
          rv = property->GetId(propertyID);
          NS_ASSERTION(NS_SUCCEEDED(rv),
            "Property cache failed to update dependent properties!");
          if (NS_SUCCEEDED(rv)) {
            PRUint32 depPropDBID = mCache->GetPropertyDBIDInternal(propertyID);
            mDirty.PutEntry(depPropDBID);
            mDirtyForInvalidation.insert(depPropDBID);
          }
        }
      }
    }
  }
  // If this bag just became dirty, then let the property cache know.
  // Only notify once in order to avoid unnecessarily locking the
  // property cache
  if (previousDirtyCount == 0) {
    rv = mCache->AddDirty(mGuid, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If this property is user editable we need to
  // set the updated timestamp.  We only
  // track updates to user editable properties
  // since that's all the user cares about.
  PRBool userEditable = PR_FALSE;
  rv = propertyInfo->GetUserEditable(&userEditable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (userEditable) {
#ifdef DEBUG
    // #updated is NOT user editable, so we won't die a firey
    // recursive death here, but lets assert just in case.
    NS_ENSURE_TRUE(!aPropertyID.EqualsLiteral(SB_PROPERTY_UPDATED),
                   NS_ERROR_UNEXPECTED);

#endif
    sbAutoString now((PRUint64)(PR_Now()/PR_MSEC_PER_SEC));
    rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_UPDATED), now);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If this property is one that may be used in the metadata
  // hash identity and it was set, then we need to recalculate
  // the identity for this item.
  PRBool usedInIdentity = PR_FALSE;
  rv = propertyInfo->GetUsedInIdentity(&usedInIdentity);
  NS_ENSURE_SUCCESS(rv, rv);

  if (usedInIdentity) {
    // The propertybag has all the information we need to calculate the
    // identity. Give it to the identity service to get an identity
    nsString identity;
    rv = mIdService->CalculateIdentityForBag(this, identity);
    NS_ENSURE_SUCCESS(rv, rv);

    // save that identity
    rv = mIdService->SaveIdentityToBag(this, identity);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseResourcePropertyBag::Write()
{
  nsresult rv = NS_OK;

  if(mDirty.Count() > 0) {
    rv = mCache->Write();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
sbLocalDatabaseResourcePropertyBag::PutValue(PRUint32 aPropertyID,
                                             const nsAString& aValue)
{
  nsAutoPtr<sbPropertyData> data(new sbPropertyData(aValue,
                                                    EmptyString(),
                                                    EmptyString()));
  nsAutoMonitor mon(mCache->mMonitor);
  PRBool success = mValueMap.Put(aPropertyID, data);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  data.forget();

  return NS_OK;
}

PRBool
sbLocalDatabaseResourcePropertyBag::IsPropertyDirty(PRUint32 aPropertyDBID)
{
  if(mDirty.IsInitialized() && mDirty.GetEntry(aPropertyDBID)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
sbLocalDatabaseResourcePropertyBag::EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc,
                                                   void *aClosure,
                                                   PRUint32 *aDirtyCount)
{
  NS_ENSURE_ARG_POINTER(aClosure);
  NS_ENSURE_ARG_POINTER(aDirtyCount);

  *aDirtyCount = mDirty.EnumerateEntries(aEnumFunc, aClosure);
  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::ClearDirty()
{
  nsAutoMonitor mon(mCache->mMonitor);
  mDirty.Clear();
  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::GetDirtyForInvalidation(std::set<PRUint32> &aDirty)
{
  aDirty.clear();

  if(!mDirtyForInvalidation.empty()) {
    aDirty = mDirtyForInvalidation;
    mDirtyForInvalidation.clear();
  }

  return NS_OK;
}
