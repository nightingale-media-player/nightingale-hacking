/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbMediaListDuplicateFilter.h"

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsStringAPI.h>

// Mozilla interfaces
#include <nsISimpleEnumerator.h>

// Songbird includes
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>

// Songbird interfaces
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

NS_IMPL_THREADSAFE_ISUPPORTS3(sbMediaListDuplicateFilter, 
                              nsISimpleEnumerator,
                              sbIMediaListDuplicateFilter,
                              sbIMediaListEnumerationListener);

static char const * const DUPLICATE_PROPERTIES[] = {
  SB_PROPERTY_GUID,
  SB_PROPERTY_CONTENTURL,
  SB_PROPERTY_ORIGINITEMGUID,
  SB_PROPERTY_ORIGINURL
};

sbMediaListDuplicateFilter::sbMediaListDuplicateFilter() :
  mMonitor("sbMediaListDuplicateFilter::mMonitor"),
  mInitialized(PR_FALSE),
  mSBPropKeysLength(NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES)),
  mSBPropKeys(NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES)),
  mDuplicateItems(0),
  mTotalItems(0),
  mRemoveDuplicates(PR_FALSE)
{
  mKeys.Init();
}

sbMediaListDuplicateFilter::~sbMediaListDuplicateFilter()
{
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::Initialize(nsISimpleEnumerator * aSource,
                                       sbIMediaList * aDest,
                                       PRBool aRemoveDuplicates)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDest);

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  nsCOMPtr<sbIMutablePropertyArray> propArray = 
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Can't use strict array when using it to get properties.
  rv = propArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0;
       index < mSBPropKeysLength;
       ++index) {
    NS_ConvertASCIItoUTF16 prop(DUPLICATE_PROPERTIES[index]);
    mSBPropKeys.AppendElement(prop);
    propArray->AppendProperty(prop, EmptyString());
  }

  mSBPropertyArray = do_QueryInterface(propArray, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mRemoveDuplicates = aRemoveDuplicates;
  mSource = aSource;
  mDest = aDest;

  return NS_OK;
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::GetDuplicateItems(PRUint32 * aDuplicateItems)
{
  NS_ENSURE_ARG_POINTER(aDuplicateItems);
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  *aDuplicateItems = mDuplicateItems;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::GetTotalItems(PRUint32 * aTotalItems)
{
  NS_ENSURE_ARG_POINTER(aTotalItems);
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  *aTotalItems = mTotalItems;
  return NS_OK;
}

// nsISimpleEnumerator implementation

NS_IMETHODIMP
sbMediaListDuplicateFilter::HasMoreElements(PRBool *aMore)
{
  NS_ENSURE_ARG_POINTER(aMore);
  if (!mCurrentItem) {
    Advance();
  }
  *aMore = mCurrentItem != nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::GetNext(nsISupports ** aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  if (!mCurrentItem) {
    Advance();
  }
  if (!mCurrentItem) {
    return NS_ERROR_FAILURE;
  }

  rv = CallQueryInterface(mCurrentItem, aItem);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItem = nsnull;

  return NS_OK;
}

// sbIMediaListEnumerationListener implementation

NS_IMETHODIMP
sbMediaListDuplicateFilter::OnEnumerationBegin(sbIMediaList *aMediaList,
                                               PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbMediaListDuplicateFilter::OnEnumeratedItem(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = SaveItemKeys(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbMediaListDuplicateFilter::OnEnumerationEnd(sbIMediaList *aMediaList,
                                             nsresult aStatusCode)
{
  return NS_OK;
}

// Private implementation

nsresult
sbMediaListDuplicateFilter::SaveItemKeys(sbIMediaItem * aItem)
{
  nsresult rv;
  nsString key;
  
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  rv = aItem->GetProperties(mSBPropertyArray, getter_AddRefs(mItemProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  for (PRUint32 index = 0;
       index < mSBPropKeysLength;
       ++index) {
    rv = mItemProperties->GetPropertyAt(index, getter_AddRefs(property));
    if (NS_SUCCEEDED(rv)) {
      rv = property->GetValue(key);
      if (NS_SUCCEEDED(rv) && !key.IsEmpty()) {
        NS_ENSURE_TRUE(mKeys.PutEntry(key), NS_ERROR_OUT_OF_MEMORY);
      }
    }
  }
  return NS_OK;
}

nsresult
sbMediaListDuplicateFilter::IsDuplicate(sbIMediaItem * aItem,
                                        bool & aIsDuplicate)
{
  aIsDuplicate = false;
  nsresult rv;
  nsString key;

  // No need to lock here because IsDuplicate is always
  // called with the monitor already acquired.

  rv = aItem->GetProperties(mSBPropertyArray, getter_AddRefs(mItemProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  for (PRUint32 index = 0;
       index < mSBPropKeysLength;
       ++index) {
    rv = mItemProperties->GetPropertyAt(index, getter_AddRefs(property));
    if (NS_SUCCEEDED(rv)) {
      property->GetValue(key);
      if (mKeys.GetEntry(key)) {
        aIsDuplicate = true;
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

nsresult
sbMediaListDuplicateFilter::Advance()
{
  nsresult rv;

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  if (!mInitialized) {
    // Only enumerate if we need to check for duplicates.
    if (mRemoveDuplicates) {
      rv = mDest->EnumerateAllItems(this, sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    // Always consider ourselves initialized past this point.
    mInitialized = PR_TRUE;
  }

  PRBool more;
  rv = mSource->HasMoreElements(&more);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItem = nsnull;
  while (more && !mCurrentItem) {
    nsCOMPtr<nsISupports> supports;
    rv = mSource->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    mCurrentItem = do_QueryInterface(supports);

    if (mCurrentItem) {
      if (mRemoveDuplicates) {
        bool isDuplicate = false;
        rv = IsDuplicate(mCurrentItem, isDuplicate);
        NS_ENSURE_SUCCESS(rv, rv);
        if (isDuplicate) {
          ++mDuplicateItems;
          // Skipping duplicates then continue enumerating
          mCurrentItem = nsnull;
        }
      }
      ++mTotalItems;
    }
  }
  return NS_OK;
}
