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
 * \file sbPlayQueueService.cpp
 * \brief Songbird Play Queue Service Component Implementation.
 */

#include "sbPlayQueueService.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIObserverService.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>

#include <sbILibraryManager.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediaListView.h>
#include <sbIOrderableMediaList.h>
#include <sbLibraryManager.h>
#include <sbStandardProperties.h>

#include <prlog.h>

/*
 * The name of the play queue mediaList. Use the same string that is displayed
 * as a label at the top of our play queue displaypane UI.
 */
#define SB_NAMEKEY_PLAYQUEUE_LIST                                              \
  "&chrome://songbird/locale/songbird.properties#playqueue.pane.title"

/*
 * By default, only show the track name
 */
#define SB_PLAYQUEUE_DEFAULTCOLUMNSPEC                                         \
  NS_LL("http://songbirdnest.com/data/1.0#ordinal 20 ")                        \
  NS_LL("http://songbirdnest.com/data/1.0#trackName 180")

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbPlayQueueService:5
 */
#ifdef PR_LOGGING
  static PRLogModuleInfo* gPlayQueueServiceLog = nsnull;
#define TRACE(args) PR_LOG(gPlayQueueServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gPlayQueueServiceLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_ISUPPORTS4(sbPlayQueueService,
                   sbIPlayQueueService,
                   sbIMediaListListener,
                   sbIMediacoreEventListener,
                   nsIObserver)

sbPlayQueueService::sbPlayQueueService()
  : mMediaList(nsnull),
    mLibrary(nsnull),
    mIndex(0),
    mInitialized(PR_FALSE),
    mBatchRemovalIndex(0),
    mBatchBeginAllHistory(PR_FALSE),
    mIgnoreListListener(PR_FALSE),
    mSequencerOnQueue(PR_FALSE),
    mSequencerPlayingOrPaused(PR_FALSE),
    mLibraryListener(nsnull),
    mWeakMediacoreManager(nsnull)
{
  #if PR_LOGGING
    if (!gPlayQueueServiceLog) {
      gPlayQueueServiceLog = PR_NewLogModule("sbPlayQueueService");
    }
  #endif /* PR_LOGGING */
  TRACE(("%s[%p]", __FUNCTION__, this));
}

sbPlayQueueService::~sbPlayQueueService()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  Finalize();
}

nsresult
sbPlayQueueService::Init()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add observers for library manager topics
  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_READY_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbPlayQueueService::Finalize()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  if (mMediaList) {
    mMediaList->RemoveListener(this);
    mMediaList = nsnull;
  }

  if (mLibraryListener && mLibrary) {
    mLibrary->RemoveListener(mLibraryListener);
    mLibraryListener = nsnull;
  }
  mLibrary = nsnull;

  if (mWeakMediacoreManager) {
    nsCOMPtr<sbIMediacoreEventTarget> target =
        do_QueryReferent(mWeakMediacoreManager, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = target->RemoveListener(this);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to remove mediacore listener");
    }
    mWeakMediacoreManager = nsnull;
  }

  mRemovedItemGUIDs.Clear();

  if (mInitialized) {
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);

    if (NS_SUCCEEDED(rv)) {
        observerService->RemoveObserver(this,
                                        SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    }
  }

  mInitialized = PR_FALSE;
}

//------------------------------------------------------------------------------
//
// Implementation of sbIPlayQueueService and associated helper methods
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbPlayQueueService::GetMediaList(sbIMediaList** aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "GetMediaList() must be called from the main thread");
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF(*aMediaList = mMediaList);
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::GetIndex(PRUint32* aIndex)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "GetIndex() must be called from the main thread");
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aIndex);

  *aIndex = mIndex;
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::SetIndex(PRUint32 aIndex)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "SetIndex() must be called from the main thread");
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  PRUint32 length;
  nsresult rv = mMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // The valid range for our index is from 0 to the length of the list.
  if (aIndex > length) {
    mIndex = length;
  } else {
    mIndex = aIndex;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::QueueNext(sbIMediaItem* aMediaItem)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "QueueNext() must be called from the main thread");
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  mIgnoreListListener = PR_TRUE;

  // If the sequencer is on a view of our list and is playing or paused, insert
  // the item immediately after the playing/paused item, whose index should
  // correspond to mIndex. If the sequencer is stopped or on a view of a
  // different list, insert the item immediately before the item at mIndex. The
  // value of mIndex should never change on a queueNext operation, although the
  // mediaItem at mIndex could change such that the new item is at mIndex.

  // The index into mMediaList that we will insert aMediaItem before.
  PRUint32 insertBeforeIndex =
      (mSequencerOnQueue && mSequencerPlayingOrPaused) ? mIndex + 1 : mIndex;

  // We need to use internal QueueLast methods if insertBeforeIndex is not a
  // valid index into the list because it is beyond the last index.
  PRUint32 length;
  rv = mMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool callQueueLast = (insertBeforeIndex >= length);

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    // The item is a mediaList
    if (callQueueLast) {
      rv = QueueLastInternal(itemAsList);
    } else {
      rv = QueueNextInternal(itemAsList, insertBeforeIndex);
    }
  } else {
    // Just a mediaItem
    if (callQueueLast) {
      rv = QueueLastInternal(aMediaItem);
    } else {
      rv = QueueNextInternal(aMediaItem, insertBeforeIndex);
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mIgnoreListListener = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::QueueLast(sbIMediaItem* aMediaItem)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "QueueLast() must be called from the main thread");
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  mIgnoreListListener = PR_TRUE;

  // Determine if we have a list or just a single item and add it to the end of
  // our list. QueueLast should never change mIndex.

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    // The item is a medialist
    rv = QueueLastInternal(itemAsList);
  } else {
    rv = QueueLastInternal(aMediaItem);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mIgnoreListListener = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::QueueSomeNext(nsISimpleEnumerator* aMediaItems)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "QueueSomeNext() must be called from the main thread");
  NS_ENSURE_ARG_POINTER(aMediaItems);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  mIgnoreListListener = PR_TRUE;

  // The index into mMediaList that we will insert aMediaItem before.
  PRUint32 insertBeforeIndex =
      (mSequencerOnQueue && mSequencerPlayingOrPaused) ? mIndex + 1 : mIndex;

  PRUint32 length;
  rv = mMediaList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if (insertBeforeIndex >= length) {
    rv = mMediaList->AddSome(aMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    nsCOMPtr<sbIOrderableMediaList> orderedList =
        do_QueryInterface(mMediaList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = orderedList->InsertSomeBefore(insertBeforeIndex, aMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mIgnoreListListener = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::QueueSomeLast(nsISimpleEnumerator* aMediaItems)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "QueueSomeLast() must be called from the main thread");
  NS_ENSURE_ARG_POINTER(aMediaItems);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  mIgnoreListListener = PR_TRUE;

  rv = mMediaList->AddSome(aMediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  mIgnoreListListener = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::ClearAll()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "Clear() must be called from the main thread");
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  mIgnoreListListener = PR_TRUE;

  // Remove all non-list items from the play queue library.
  nsresult rv = mLibrary->ClearItems();
  NS_ENSURE_SUCCESS(rv, rv);

  mIndex = 0;

  mIgnoreListListener = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::ClearHistory()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(),
      "ClearHistory() must be called from the main thread");
  nsresult rv;

  if (mIndex == 0) {
    // No history items, do nothing.
    return NS_OK;
  }

  // Remove all tracks with an index lower than mIndex. Create an emumerator so
  // that the actual removal of items occurs in a batch. Note that
  // sbLocalDatabaseSimpleMediaList::RemoveSome uses IndexOf to determine which
  // item to actually remove. This is fine because we're always clearing from
  // the beginning of the list in ClearHistory.

  nsCOMPtr<nsIMutableArray> historyItems =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < mIndex; ++i) {
    nsCOMPtr<sbIMediaItem> item;
    rv = mMediaList->GetItemByIndex(i, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = historyItems->AppendElement(item, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsISimpleEnumerator> historyEnumerator;
  rv = historyItems->Enumerate(getter_AddRefs(historyEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove the items, and let our list listener take care of mIndex and cleanup
  // of the play queue library.
  rv = mMediaList->RemoveSome(historyEnumerator);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::QueueNextInternal(sbIMediaItem* aMediaItem,
                                      PRUint32 aInsertBeforeIndex)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Our list should always implement sbIOrderableMediaList
  nsCOMPtr<sbIOrderableMediaList> orderedList =
      do_QueryInterface(mMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = orderedList->InsertBefore(aInsertBeforeIndex, aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::QueueNextInternal(sbIMediaList* aMediaList,
                                      PRUint32 aInsertBeforeIndex)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaList);
  nsresult rv;

  // Our list should always implement sbIOrderableMediaList
  nsCOMPtr<sbIOrderableMediaList> orderedList =
      do_QueryInterface(mMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = orderedList->InsertAllBefore(aInsertBeforeIndex, aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::QueueLastInternal(sbIMediaItem* aMediaItem)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Add the item to the end of our list
  nsresult rv = mMediaList->Add(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::QueueLastInternal(sbIMediaList* aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Add the contents of aMediaList to the end of our list.
  nsresult rv = mMediaList->AddAll(aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::CreateMediaList()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_STATE(mLibrary);

  nsresult rv;
  rv = mLibrary->CreateMediaList(NS_LITERAL_STRING("simple"),
                                 nsnull,
                                 getter_AddRefs(mMediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Save the GUID as a property on the play queue library
  nsAutoString listGUID;
  rv = mMediaList->GetGuid(listGUID);
  NS_ENSURE_SUCCESS(rv, rv);


  rv = mLibrary->SetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_PLAYQUEUE_MEDIALIST_GUID),
          listGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaList->SetName(NS_LITERAL_STRING(SB_NAMEKEY_PLAYQUEUE_LIST));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaList->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                   NS_MULTILINE_LITERAL_STRING(SB_PLAYQUEUE_DEFAULTCOLUMNSPEC));

  rv = mMediaList->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE),
                               NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::InitLibrary()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  // Get the play queue library guid, which is stored as a pref.
  nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> supportsString;
  nsAutoString guid;
  rv = prefBranch->GetComplexValue(SB_PREF_PLAYQUEUE_LIBRARY,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(supportsString));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = supportsString->GetData(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the library
  nsCOMPtr<sbILibraryManager> libraryManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryManager->GetLibrary(guid, getter_AddRefs(mLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::InitMediaList()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_STATE(mLibrary);
  nsresult rv;

  // The list GUID is stored as a property on the playqueue library
  nsAutoString listGUID;
  rv = mLibrary->GetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_PLAYQUEUE_MEDIALIST_GUID),
          listGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!listGUID.IsEmpty()) {
    // Try to get the list
    nsCOMPtr<sbIMediaItem> listAsItem;
    rv = mLibrary->GetMediaItem(listGUID, getter_AddRefs(listAsItem));
    if (NS_SUCCEEDED(rv)) {
      mMediaList = do_QueryInterface(listAsItem, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // We found our list, so we can return.
      return NS_OK;
    }
  }

  // We either didn't get a GUID or we couldn't find a mediaList with the
  // expected GUID, so clear the play queue library and create a new list. The
  // library can be cleared because we don't ever want it to contain more than
  // the play queue list and items that are contained on that list. Our
  // listeners aren't hooked up yet, so don't worry about notifications.
  rv = mLibrary->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateMediaList();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::SetIndexToPlayingTrack()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  if (mSequencerOnQueue) {
    nsCOMPtr<sbIMediacoreManager> manager =
        do_QueryReferent(mWeakMediacoreManager, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediacoreSequencer> sequencer;
    rv = manager->GetSequencer(getter_AddRefs(sequencer));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaListView> view;
    rv = sequencer->GetView(getter_AddRefs(view));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 filteredIndex;
    rv = sequencer->GetViewPosition(&filteredIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 unfilteredIndex;
    rv = view->GetUnfilteredIndex(filteredIndex, &unfilteredIndex);

    if (NS_SUCCEEDED(rv)) {
      mIndex = unfilteredIndex;
    }
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Implementation of sbIMediaListListener
//
//------------------------------------------------------------------------------

// We have to allow access to our mediaList as a readonly attribute so that
// client code can create UI elements with the list. Thus, we have to be able to
// catch changes to the underlying list and update mIndex accordingly. As a
// general rule, we'll ignore these notifications if we are modifying the list
// from within the service.

// During batch additions to our list, OnItemAdded provides indices reflecting
// the state of the mediaList after the entire batch has been added.

// Batch removal notifications can be unreliable. OnAfterItemRemoved can
// actually be sent before the item is removed. OnBeforeItemRemoved provides
// indices that reflect the state of the mediaList prior to any removals, so we
// don't want to update mIndex incrementally during batch removal. Instead, we
// must always compare the index provided by onBeforeItemRemoved to the initial
// mIndex prior to the batch. This ensures that all comparisons during batch
// removal reflect the state of the list prior to the batch, and we use
// mBatchHelperIndex to track index decrementing during the batch.

NS_IMETHODIMP
sbPlayQueueService::OnBatchBegin(sbIMediaList* aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (!mBatchHelper.IsActive()) {
    mBatchRemovalIndex = mIndex;

    // If no batch is active, we shouldn't have any items marked for removal
    // from the play queue library. If there are any, clear them.
    mRemovedItemGUIDs.Clear();

    // Keep track of whether or not all items were 'history' items at the start
    // of the batch.
    PRUint32 length;
    nsresult rv = mMediaList->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    mBatchBeginAllHistory = length == mIndex;
  }

  mBatchHelper.Begin();

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnBatchEnd(sbIMediaList* aMediaList)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  mBatchHelper.End();
  if (!mBatchHelper.IsActive()) {
    // Iterate through the array of items that were removed from the queue list
    // in the batch. It should not have any repeats. If there are no remaining
    // instances of a given item in the queue, remove the item from the library.
    PRUint32 removedCount = mRemovedItemGUIDs.Length();
    LOG(("removed count %u", removedCount));

    if (removedCount == 0) {
      // This must be an add batch, not a removal.
      return NS_OK;
    }

    for (PRUint32 i = 0; i < removedCount; ++i) {

      nsCOMPtr<sbIMediaItem> item;
      rv = mLibrary->GetMediaItem(mRemovedItemGUIDs[i], getter_AddRefs(item));
      if (NS_FAILED(rv) || ! item) {
        continue;
      }

      PRBool contains;
      rv = mMediaList->Contains(item, &contains);
      if (NS_FAILED(rv)) {
        continue;
      }

      if (!contains) {
        rv = mLibrary->Remove(item);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    mRemovedItemGUIDs.Clear();
    LOG(("Changing index from %i to %i", mIndex, mBatchRemovalIndex));
    mIndex = mBatchRemovalIndex;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnItemAdded(sbIMediaList* aMediaList,
                                sbIMediaItem* aMediaItem,
                                PRUint32 aIndex,
                                PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  if (mIgnoreListListener ||
      (mSequencerOnQueue && mSequencerPlayingOrPaused) ||
      mLibraryListener->ShouldIgnore())
  {
    return NS_OK;
  }

  // We need to know if the list was all 'history' items prior to the addition.
  // This should also be true if the list was empty, as logically they are the
  // same case (i.e. mIndex equals the length of the list)
  PRBool wasAllHistory;
  if (mBatchHelper.IsActive()) {
    wasAllHistory = mBatchBeginAllHistory;
  } else {
    PRUint32 length;
    rv = mMediaList->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    // For non-batch additions, the list was all 'history' items before the
    // addition if the length is one greater than the index here.
    wasAllHistory = length == mIndex + 1;
  }

  // The logic for updating the index on added items is the same for both batch
  // and non-batch additions. This ensures that the first item added to an empty
  // list becomes the 'current' item. It also ensures that an item added to the
  // end of a queue that is all 'history' items becomes the current item.
  if (aIndex <= mIndex  && !(wasAllHistory && aIndex == mIndex)) {
    mIndex++;
  }

  LOG(("Added item at index %u, current index is now %i", aIndex, mIndex));
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                        sbIMediaItem* aMediaItem,
                                        PRUint32 aIndex,
                                        PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  if (mIgnoreListListener ||
      mLibraryListener->ShouldIgnore())
  {
    return NS_OK;
  }

  if (mBatchHelper.IsActive()) {
    LOG(("Removing item at index %u", aIndex));

    nsAutoString guid;
    rv = aMediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("Removed item guid:  %s",
         NS_ConvertUTF16toUTF8(guid).BeginReading()));

    if (mRemovedItemGUIDs.IndexOf(guid) == mRemovedItemGUIDs.NoIndex) {
      nsString* success = mRemovedItemGUIDs.AppendElement(guid);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }

    // In batch removal, aIndex reflects the index in the list prior to removal
    // of any items in the batch. We keep mIndex in the pre-batch state as well,
    // so decrement the helper index if necessary.
    if (aIndex < mIndex) {
      mBatchRemovalIndex--;
    }
  }

  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                       sbIMediaItem* aMediaItem,
                                       PRUint32 aIndex,
                                       PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  if (mIgnoreListListener ||
      (mSequencerOnQueue && mSequencerPlayingOrPaused) ||
      mLibraryListener->ShouldIgnore())
  {
    return NS_OK;
  }

  // Handle non-batch item removal here, where we should get the mediaList in
  // its post-removal state
  if (!mBatchHelper.IsActive()) {
    LOG(("Non batch item removed from index %u", aIndex));
    LOG(("aIndex %u, mIndex %u", aIndex, mIndex));
    if (aIndex < mIndex) {
      mIndex--;
    }

    PRBool contains;
    rv = mMediaList->Contains(aMediaItem, &contains);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!contains) {
      LOG(("Removing the item from the library"));
      rv = mLibrary->Remove(aMediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnItemUpdated(sbIMediaList* aMediaList,
                                  sbIMediaItem* aMediaItem,
                                  sbIPropertyArray* aProperties,
                                  PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mIgnoreListListener || mLibraryListener->ShouldIgnore()) {
    return NS_OK;
  }

  // xxx slloyd TODO we need to push some property changes back to the user's
  //                 main library
  //
  //    play count
  //    skip count
  //    last played time
  //    metadata changes?
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnItemMoved(sbIMediaList* aMediaList,
                                PRUint32 aFromIndex,
                                PRUint32 aToIndex,
                                PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mIgnoreListListener ||
      (mSequencerOnQueue && mSequencerPlayingOrPaused) ||
      mLibraryListener->ShouldIgnore())
  {
    return NS_OK;
  }

  // If the item at the current index is moved, keep that item as current.
  // Otherwise, update the current index if an item was moved from after it to
  // before it (or vise versa).

  if (aFromIndex == mIndex) {
    mIndex = aToIndex;
  } else if (aFromIndex < mIndex && aToIndex >= mIndex) {
    mIndex--;
  } else if (aFromIndex > mIndex && aToIndex <= mIndex) {
    mIndex++;
  }

  LOG(("Item was moved from %u to %u, new index: %i", aFromIndex, aToIndex, mIndex));

  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnBeforeListCleared(sbIMediaList* aMediaList,
                                        PRBool aExcludeLists,
                                        PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mIgnoreListListener || mLibraryListener->ShouldIgnore()) {
    return NS_OK;
  }

  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueService::OnListCleared(sbIMediaList* aMediaList,
                                  PRBool aExcludeLists,
                                  PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mIgnoreListListener || mLibraryListener->ShouldIgnore()) {
    return NS_OK;
  }

  LOG(("Clearing all items, but not lists, from queue library"));
  nsresult rv = mLibrary->ClearItems();
  NS_ENSURE_SUCCESS(rv, rv);

  mIndex = 0;

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Implementation of sbIMediacoreEventListener and mediacore event handlers
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbPlayQueueService::OnMediacoreEvent(sbIMediacoreEvent* aEvent)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aEvent);
  nsresult rv;

  PRUint32 eventType;
  rv = aEvent->GetType(&eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Event type %u", eventType));

  // Determine if we are playing or paused.
  nsCOMPtr<sbIMediacoreManager> manager =
      do_QueryReferent(mWeakMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = manager->GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreStatus> status = do_QueryInterface(sequencer, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 state;
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, rv);

  mSequencerPlayingOrPaused = (state == sbIMediacoreStatus::STATUS_PLAYING ||
                               state == sbIMediacoreStatus::STATUS_PAUSED);

  // Detect view change so we can keep track of whether or not the sequencer's
  // view is a view of our mediaList.
  if (eventType == sbIMediacoreEvent::VIEW_CHANGE) {
    rv = OnViewChange(aEvent);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // Ignore other event types when the sequencer is not on a view of our list
  if (!mSequencerOnQueue) {
    return NS_OK;
  }

  switch (eventType) {

    // Handle STREAM_STOP or STREAM_END by bumping mIndex.
    case sbIMediacoreEvent::STREAM_STOP:
    case sbIMediacoreEvent::STREAM_END:
      // Call SetIndex so mIndex is constrained to the length of the list.
      rv = SetIndex(mIndex + 1);
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    // EXPLICIT_TRACK_CHANGE and TRACK_CHANGE are handled the same way
    case sbIMediacoreEvent::EXPLICIT_TRACK_CHANGE:
    case sbIMediacoreEvent::TRACK_CHANGE:
      rv = OnTrackChange(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case sbIMediacoreEvent::TRACK_INDEX_CHANGE:
      rv = OnTrackIndexChange(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    default:
      break;
  }

  return NS_OK;
}

nsresult
sbPlayQueueService::OnViewChange(sbIMediacoreEvent* aEvent)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aEvent);
  nsresult rv;

  // Check to see if the new view is a view of our mediaList.

  nsCOMPtr<nsIVariant> variant;
  rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = variant->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListView> view = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> viewList;
  rv = view->GetMediaList(getter_AddRefs(viewList));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool onQueue;
  rv = viewList->Equals(mMediaList, &onQueue);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("Is the view change a view of our list? %i", onQueue));

  mSequencerOnQueue = onQueue;

  return NS_OK;
}

nsresult
sbPlayQueueService::OnTrackChange(sbIMediacoreEvent* aEvent)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv = SetIndexToPlayingTrack();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlayQueueService::OnTrackIndexChange(sbIMediacoreEvent* aEvent)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aEvent);

  if (mSequencerPlayingOrPaused) {
    nsresult rv = SetIndexToPlayingTrack();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Implementation of nsIObserver
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbPlayQueueService::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const PRUnichar* aData)
{

  NS_ENSURE_ARG_POINTER(aTopic);
  nsresult rv;

  TRACE(("%s[%p]: Observing %s", __FUNCTION__, this, aTopic));

  nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!strcmp(SB_LIBRARY_MANAGER_READY_TOPIC, aTopic)) {
    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get a weak reference to the mediacore manager
    nsCOMPtr<nsISupportsWeakReference> weak =
        do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = weak->GetWeakReference(getter_AddRefs(mWeakMediacoreManager));
    NS_ENSURE_SUCCESS(rv, rv);

    // Add ourself as a mediacore event listener
    nsCOMPtr<sbIMediacoreEventTarget> target =
        do_QueryReferent(mWeakMediacoreManager, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = target->AddListener(this);
    NS_ENSURE_SUCCESS(rv, rv);

    // Make sure we have a library
    nsresult rv = InitLibrary();
    NS_ENSURE_SUCCESS(rv, rv);

    // Make sure we have our underlying medialist
    rv = InitMediaList();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_STATE(mMediaList);

    // Listen to our list
    rv = mMediaList->AddListener(this,
                                 PR_FALSE,
                                 sbIMediaList::LISTENER_FLAGS_ALL,
                                 nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    // Listen to the play queue library
    mLibraryListener = new sbPlayQueueLibraryListener();
    NS_ENSURE_TRUE(mLibraryListener, NS_ERROR_OUT_OF_MEMORY);

    rv = mLibrary->AddListener(mLibraryListener,
                               PR_FALSE,
                               sbIMediaList::LISTENER_FLAGS_LISTCLEARED |
                               sbIMediaList::LISTENER_FLAGS_BEFORELISTCLEARED,
                               nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitialized = PR_TRUE;
  }
  else if (!strcmp(SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, aTopic)) {
    Finalize();
  }

  return NS_OK;
}

// -----------------------------------------------------------------------------
//
// XPCOM Registration
//
// -----------------------------------------------------------------------------

/* static */ NS_METHOD
sbPlayQueueService::RegisterSelf(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *aLoaderStr,
                                 const char *aType,
                                 const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                SB_PLAYQUEUESERVICE_CLASSNAME,
                                "service,"
                                SB_PLAYQUEUESERVICE_CONTRACTID,
                                PR_TRUE,
                                PR_TRUE,
                                nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
