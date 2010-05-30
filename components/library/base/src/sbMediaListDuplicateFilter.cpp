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
#include <nsMemory.h>
#include <nsStringAPI.h>

// Mozilla interfaces
#include <nsISimpleEnumerator.h>

// Songbird includes
#include <sbStandardProperties.h>

// Songbird interfaces
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

NS_IMPL_ISUPPORTS3(sbMediaListDuplicateFilter, nsISimpleEnumerator,
                                               sbIMediaListDuplicateFilter,
                                               sbIMediaListEnumerationListener);

static char const * const DUPLICATE_PROPERTIES[] = {
  SB_PROPERTY_GUID,
  SB_PROPERTY_CONTENTURL,
  SB_PROPERTY_ORIGINITEMGUID,
  SB_PROPERTY_ORIGINURL
};

sbMediaListDuplicateFilter::sbMediaListDuplicateFilter() :
  mSBPropKeys(NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES)),
  mDuplicateItems(0),
  mTotalItems(0),
  mRemoveDuplicates(false)
{
  mKeys.Init();
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES);
       ++index) {
    mSBPropKeys.AppendElement(
                           NS_ConvertASCIItoUTF16(DUPLICATE_PROPERTIES[index]));
  }
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

  mRemoveDuplicates = aRemoveDuplicates;
  mSource = aSource;

  nsresult rv = aDest->EnumerateAllItems(
                                        this,
                                        sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::GetDuplicateItems(PRUint32 * aDuplicateItems)
{
  NS_ENSURE_ARG_POINTER(aDuplicateItems);
  *aDuplicateItems = mDuplicateItems;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListDuplicateFilter::GetTotalItems(PRUint32 * aTotalItems)
{
  NS_ENSURE_ARG_POINTER(aTotalItems);
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
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES);
       ++index) {
    rv = aItem->GetProperty(mSBPropKeys[index], key);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!key.IsEmpty()) {
      NS_ENSURE_TRUE(mKeys.PutEntry(key), NS_ERROR_OUT_OF_MEMORY);
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
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(DUPLICATE_PROPERTIES);
       ++index) {
    rv = aItem->GetProperty(mSBPropKeys[index], key);
    NS_ENSURE_SUCCESS(rv, rv);
    if (mKeys.GetEntry(key)) {
      aIsDuplicate = true;
      return NS_OK;
    }
  }
  return NS_OK;
}

nsresult
sbMediaListDuplicateFilter::Advance()
{
  nsresult rv;

  PRBool more;
  rv = mSource->HasMoreElements(&more);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItem = nsnull;
  while (more && !mCurrentItem) {
    nsCOMPtr<nsISupports> supports;
    rv = mSource->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    mCurrentItem = do_QueryInterface(supports);

    bool isDuplicate = false;
    if (mCurrentItem) {
      rv = IsDuplicate(mCurrentItem, isDuplicate);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isDuplicate) {
        ++mDuplicateItems;
        // If we're skipping duplicates then continue enumerating
        if (mRemoveDuplicates) {
          mCurrentItem = nsnull;
        }
      }
      ++mTotalItems;
    }
  }
  return NS_OK;
}
