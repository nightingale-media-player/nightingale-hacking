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

/**
 * \file sbPlayQueueLibraryListener.cpp
 * \brief Implementation of helper class for property synchronization
 */

#include "sbPlayQueueExternalLibraryListener.h"

#include <nsIArray.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>

#include <sbILibraryManager.h>
#include <sbStandardProperties.h>

#include <prlog.h>

#include <algorithm>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbPlayQueueExternalLibraryListener:5
 */
#ifdef PR_LOGGING
  static PRLogModuleInfo* gPlayQueueExternalLibraryListenerLog = nsnull;
#define TRACE(args) PR_LOG(gPlayQueueExternalLibraryListenerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gPlayQueueExternalLibraryListenerLog, PR_LOG_WARN, args)
# ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

//------------------------------------------------------------------------------
//
// Implementation of sbPropertyUpdate
//
//------------------------------------------------------------------------------

bool
sbPropertyUpdate::operator==(sbPropertyUpdate rhs)
{
  if (mItem != rhs.mItem) {
    return false;
  }

  nsresult rv;
  PRUint32 lhLength;
  rv = mUpdate->GetLength(&lhLength);
  NS_ENSURE_SUCCESS(rv, false);
  PRUint32 rhLength;
  rv = rhs.mUpdate->GetLength(&rhLength);
  NS_ENSURE_SUCCESS(rv, false);
  if (lhLength != rhLength) {
    return false;
  }

  for (int i = 0; i < lhLength; i++) {
    nsCOMPtr<sbIProperty> lhProp;
    rv = mUpdate->GetPropertyAt(i, getter_AddRefs(lhProp));
    NS_ENSURE_SUCCESS(rv, false);
    nsAutoString id;
    lhProp->GetId(id);
    nsAutoString lhValue;
    lhProp->GetValue(lhValue);
    nsAutoString rhValue;
    rv = rhs.mUpdate->GetPropertyValue(id, rhValue);
    NS_ENSURE_SUCCESS(rv, false);
    if (!lhValue.Equals(rhValue)) {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
//
// Implementation of sbPlayQueueExternalLibraryListener
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlayQueueExternalLibraryListener,
                              sbIMediaListListener);

sbPlayQueueExternalLibraryListener::sbPlayQueueExternalLibraryListener()
  : mUpdateLock(nsnull)
{
  #if PR_LOGGING
    if (!gPlayQueueExternalLibraryListenerLog) {
      gPlayQueueExternalLibraryListenerLog =
        PR_NewLogModule("sbPlayQueueExternalLibraryListener");
    }
  #endif /* PR_LOGGING */

  TRACE(("%s[%p]", __FUNCTION__, this));

  mUpdateLock = PR_NewLock();
  NS_ASSERTION(mUpdateLock, "failed to create lock!");
}

sbPlayQueueExternalLibraryListener::~sbPlayQueueExternalLibraryListener()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mUpdateLock) {
    PR_DestroyLock(mUpdateLock);
  }
}

nsresult
sbPlayQueueExternalLibraryListener::RemoveListeners()
{
  nsresult rv;
  rv = mMasterLibrary->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = mExternalLibraries.Count();
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbILibrary> library = mExternalLibraries.ObjectAt(i);
    NS_ENSURE_STATE(library);

    rv = library->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbPlayQueueExternalLibraryListener::SetMasterLibrary(sbILibrary* aLibrary)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  mMasterLibrary = aLibrary;
  rv = mMasterLibrary->AddListener(this,
                                     PR_FALSE,
                                     sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                                     nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueExternalLibraryListener::AddExternalLibrary(sbILibrary* aLibrary)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  PRBool added = mExternalLibraries.AppendObject(aLibrary);
  NS_ENSURE_TRUE(added, NS_ERROR_FAILURE);

  rv = aLibrary->AddListener(this,
                             PR_FALSE,
                             sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                             nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueExternalLibraryListener::SetPropertiesNoSync(
        sbIMediaItem* aMediaItem,
        sbIPropertyArray* aProperties)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  // OnItemUpdated will get called with the old values, so push the
  // current values onto mUpdates here.
  nsCOMPtr<sbIPropertyArray> props;
  rv = aMediaItem->GetProperties(aProperties, getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);

  sbPropertyUpdate update(aMediaItem, props);
  {
    nsAutoLock lock(mUpdateLock);
    mUpdates.push_back(update);
  }

  rv = aMediaItem->SetProperties(aProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsAutoLock lock(mUpdateLock);
    mUpdates.remove(update);
  }

  return NS_OK;
}

nsresult
sbPlayQueueExternalLibraryListener::GenerateUpdates(
        sbIMediaItem* aMediaItem,
        sbIPropertyArray* aProperties,
        Updates& updates)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  nsCOMPtr<sbILibrary> lib;
  rv = aMediaItem->GetLibrary(getter_AddRefs(lib));
  NS_ENSURE_SUCCESS(rv, rv);

  // This ensures that we have an item that is either in our master library or 
  // is duplicated in the master library.
  nsCOMPtr<sbIMediaItem> internalMediaItem;
  if (lib == mMasterLibrary) {
    internalMediaItem = aMediaItem;
  } else {
    rv = mMasterLibrary->GetDuplicate(aMediaItem,
                                        getter_AddRefs(internalMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);
    if (internalMediaItem) {
      sbPropertyUpdate internalUpdate(internalMediaItem, aProperties);
      updates.push_back(internalUpdate);
    } else {
      // If we can't find a duplicate in our master library, just return.
      return NS_OK;
    }
  }

  // We found an item in our master library - go find duplicates in our external
  // libraries to update.
  for (int i = 0; i < mExternalLibraries.Count(); i++)
  {
    if (mExternalLibraries[i] != lib) {
      nsCOMPtr<sbIMediaItem> externalMediaItem;
      rv = mExternalLibraries[i]->GetDuplicate(internalMediaItem,
                                               getter_AddRefs(externalMediaItem));
      if (NS_SUCCEEDED(rv) && externalMediaItem) {
        sbPropertyUpdate externalUpdate(externalMediaItem, aProperties);
        updates.push_back(externalUpdate);
      }
    }
  }
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Implementation of sbIMediaListListener
//
//------------------------------------------------------------------------------

// We only care about OnItemUpdated.
NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnBeforeListCleared(
        sbIMediaList* aMediaList,
        PRBool aExcludeLists,
        PRBool *aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnListCleared(sbIMediaList* aMediaList,
                                                  PRBool aExcludeLists,
                                                  PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}


NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnBatchBegin(sbIMediaList* aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnBatchEnd(sbIMediaList* aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnItemAdded(sbIMediaList* aMediaList,
                                                sbIMediaItem* aMediaItem,
                                                PRUint32 aIndex,
                                                PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnBeforeItemRemoved(
        sbIMediaList* aMediaList,
        sbIMediaItem* aMediaItem,
        PRUint32 aIndex,
        PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                       sbIMediaItem* aMediaItem,
                                                       PRUint32 aIndex,
                                                       PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnItemUpdated(sbIMediaList* aMediaList,
                                                  sbIMediaItem* aMediaItem,
                                                  sbIPropertyArray* aProperties,
                                                  PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  Updates updates;
  sbPropertyUpdate update(aMediaItem, aProperties);
  {
    nsAutoLock lock(mUpdateLock);

    UpdateIter it;
    it = std::find(mUpdates.begin(), mUpdates.end(), update);
    if (it != mUpdates.end()) {
      // This update is already being handled.
      return NS_OK;
    }

    rv = GenerateUpdates(aMediaItem, aProperties, updates);
    NS_ENSURE_SUCCESS(rv, rv);
    if (updates.size() == 0) {
      // No duplicates to update.
      return NS_OK;
    }

    for (it = updates.begin(); it != updates.end(); it++) {
      mUpdates.push_back(*it);
    }
  }

  // Get the updated properties from the item and apply them to the items we
  // need to update.
  nsCOMPtr<sbIPropertyArray> props;
  rv = aMediaItem->GetProperties(aProperties, getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);

  for (UpdateIter it = updates.begin(); it != updates.end(); it++) {
    rv = it->mItem->SetProperties(props);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    nsAutoLock lock(mUpdateLock);

    for (UpdateIter it = updates.begin(); it != updates.end(); it++) {
      mUpdates.remove(*it);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueExternalLibraryListener::OnItemMoved(sbIMediaList* aMediaList,
                                                PRUint32 aFromIndex,
                                                PRUint32 aToIndex,
                                                PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}
