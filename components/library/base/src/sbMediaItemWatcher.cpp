/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Media item watcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaItemWatcher.cpp
 * \brief Songbird Media Item Watcher Source.
 */

//------------------------------------------------------------------------------
//
// Media item watcher imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMediaItemWatcher.h"

// Songbird imports.
#include <sbILibrary.h>
#include <sbIPropertyArray.h>

#include <sbStandardProperties.h>

#include <sbDebugUtils.h>


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS2(sbMediaItemWatcher,
                   sbIMediaItemWatcher,
                   sbIMediaListListener)


//------------------------------------------------------------------------------
//
// sbIMediaItemWatcher implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Watch for changes to the media item specified by aMediaItem and call
 *        the listener specified by aListener when any changes occur.  If
 *        aPropertyIDs is specified, only watch for changes to the specified
 *        properties.
 *
 * \param aMediaItem          Media item to watch.
 * \param aListener           Listener to call when changes occur.
 * \param aPropertyIDs        List of properties for which to listen for
 *                            changes.
 */

NS_IMETHODIMP
sbMediaItemWatcher::Watch(sbIMediaItem*         aMediaItem,
                          sbIMediaItemListener* aListener,
                          sbIPropertyArray*     aPropertyIDs)
{
  TRACE_FUNCTION("");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aListener);

  // Function variables.
  nsresult rv;

  // Set the watched media item and its listener.
  mWatchedMediaItem = aMediaItem;
  mListener = aListener;
  mWatchedPropertyIDs = aPropertyIDs;

  // Get the watched media item library as an sbIMediaList.
  nsCOMPtr<sbILibrary> library;
  rv = mWatchedMediaItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  mWatchedLibraryML = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Listen to media item related events in its library.
  rv = mWatchedLibraryML->AddListener
                            (this,
                             PR_FALSE,
                             sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                             sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                             sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                             sbIMediaList::LISTENER_FLAGS_BATCHEND,
                             mWatchedPropertyIDs);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the watched media item properties.
  rv = GetWatchedMediaItemProperties(mWatchedMediaItemProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Cancel watcher.
 */

NS_IMETHODIMP
sbMediaItemWatcher::Cancel()
{
  TRACE_FUNCTION("mBatchLevel=%i", mBatchLevel);
  NS_WARN_IF_FALSE(mBatchLevel > 0,
                   "sbMediaItemWatcher::Cancel called during a batch");
  // Stop listening to library.
  if (mWatchedLibraryML)
    mWatchedLibraryML->RemoveListener(this);

  // Clear object references.
  mWatchedMediaItem = nsnull;
  mListener = nsnull;
  mWatchedPropertyIDs = nsnull;
  mWatchedLibraryML = nsnull;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// sbIMediaListListener implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Called when a media item is added to the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The new media item.
 * \param aIndex The index in the list where the new item was added
 * \return True if you do not want any further onItemAdded notifications for
 *         the current batch.  If there is no current batch, the return value
 *         is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnItemAdded(sbIMediaList* aMediaList,
                                sbIMediaItem* aMediaItem,
                                PRUint32      aIndex,
                                PRBool*       _retval)
{
  TRACE_FUNCTION("");
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}


/**
 * \brief Called before a media item is removed from the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The media item to be removed
 * \param aIndex The index of the item to be removed
 * \return True if you do not want any further onBeforeItemRemoved
 *         notifications for the current batch.  If there is no current batch,
 *         the return value is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                        sbIMediaItem* aMediaItem,
                                        PRUint32      aIndex,
                                        PRBool*       _retval)
{
  TRACE_FUNCTION("");
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}


/**
 * \brief Called after a media item is removed from the list.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The removed media item.
 * \param aIndex Index from where the item was removed
 * \return True if you do not want any further onAfterItemRemoved for the
 *         current batch.  If there is no current batch, the return value is
 *         ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                       sbIMediaItem* aMediaItem,
                                       PRUint32      aIndex,
                                       PRBool*       _retval)
{
  TRACE_FUNCTION("");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  // Do nothing if in a batch.
  if (mBatchLevel > 0) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  // Handle item removed events.
  if (aMediaItem == mWatchedMediaItem)
    mListener->OnItemRemoved(mWatchedMediaItem);

  // Keep calling.
  *_retval = PR_FALSE;

  return NS_OK;
}


/**
 * \brief Called when a media item is changed.
 * \param aMediaList The list that has changed.
 * \param aMediaItem The item that has changed.
 * \param aProperties Array of properties that were updated.  Each property's
 *        value is the previous value of the property
 * \return True if you do not want any further onItemUpdated notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnItemUpdated(sbIMediaList*     aMediaList,
                                  sbIMediaItem*     aMediaItem,
                                  sbIPropertyArray* aProperties,
                                  PRBool*           _retval)
{
  TRACE_FUNCTION("");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  #if PR_LOGGING
  {
    nsString props, src;
    rv = aProperties->ToString(props);
    if (NS_FAILED(rv)) props.AssignLiteral("<ERROR>");
    rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                 src);
    if (NS_FAILED(rv)) src.AssignLiteral("<ERROR>");
    TRACE("item %s updated: %s",
          NS_ConvertUTF16toUTF8(src).get(),
          NS_ConvertUTF16toUTF8(props).get());
  }
  #endif /* PR_LOGGING */

  // Do nothing if in a batch.
  if (mBatchLevel > 0) {
    TRACE("In a batch, skipping");
    *_retval = PR_TRUE;
    return NS_OK;
  }

  // Handle item updated events.
  if (aMediaItem == mWatchedMediaItem) {
    rv = DoItemUpdated();
    TRACE("called: %08x", rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Keep calling.
  *_retval = PR_FALSE;

  return NS_OK;
}


/**
 * \brief Called when a media item is moved.
 * \param sbIMediaList aMediaList The list that contains the item that moved.
 * \param aFromIndex Index of the item that was moved
 * \param aToIndex New index of the moved item
 * \return True if you do not want any further onItemMoved notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnItemMoved(sbIMediaList* aMediaList,
                                PRUint32      aFromIndex,
                                PRUint32      aToIndex,
                                PRBool*       _retval)
{
  TRACE_FUNCTION("");
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}


/**
 * \Brief Called before a media list is cleared.
 * \param sbIMediaList aMediaList The list that is about to be cleared.
 * \param aExcludeLists If true, only media items, not media lists, are being
 *                      cleared.
 * \return True if you do not want any further onBeforeListCleared notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnBeforeListCleared(sbIMediaList* aMediaList,
                                        PRBool        aExcludeLists,
                                        PRBool*       _retval)
{
  TRACE_FUNCTION("");
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}


/**
 * \Brief Called when a media list is cleared.
 * \param sbIMediaList aMediaList The list that was cleared.
 * \param aExcludeLists If true, only media items, not media lists, were
 *                      cleared.
 * \return True if you do not want any further onListCleared notifications
 *         for the current batch.  If there is no current batch, the return
 *         value is ignored.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnListCleared(sbIMediaList* aMediaList,
                                  PRBool        aExcludeLists,
                                  PRBool*       _retval)
{
  TRACE_FUNCTION("");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Do nothing if in a batch.
  if (mBatchLevel > 0) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  // Handle item removed events.
  if (mWatchedMediaItem) {
    mListener->OnItemRemoved(mWatchedMediaItem);
  }

  // Keep calling.
  *_retval = PR_FALSE;

  return NS_OK;
}


/**
 * \brief Called when the library is about to perform multiple operations at
 *        once.
 *
 * This notification can be used to optimize behavior. The consumer may
 * choose to ignore further onItemAdded or onItemRemoved notifications until
 * the onBatchEnd notification is received.
 *
 * \param aMediaList The list that has changed.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnBatchBegin(sbIMediaList* aMediaList)
{
  TRACE_FUNCTION("");
  // Increment the batch level.
  mBatchLevel++;

  return NS_OK;
}


/**
 * \brief Called when the library has finished performing multiple operations
 *        at once.
 *
 * This notification can be used to optimize behavior. The consumer may
 * choose to stop ignoring onItemAdded or onItemRemoved notifications after
 * receiving this notification.
 *
 * \param aMediaList The list that has changed.
 */

NS_IMETHODIMP
sbMediaItemWatcher::OnBatchEnd(sbIMediaList* aMediaList)
{
  TRACE_FUNCTION("");
  nsresult rv;

  // Decrement the batch level.  Do nothing more if still in a batch.
  if (mBatchLevel > 0)
    mBatchLevel--;
  if (mBatchLevel > 0)
    return NS_OK;

  if (!mWatchedMediaItem) {
    NS_WARNING("sbMediaItemWatcher::OnBatchEnd");
    return NS_OK;
  }
  // Get current watched media item properties.
  nsAutoString properties;
  rv = GetWatchedMediaItemProperties(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if watched media item was updated.
  if (!properties.Equals(mWatchedMediaItemProperties)) {
    rv = DoItemUpdated(properties);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Handle item removed events.
  PRBool contains;
  rv = mWatchedLibraryML->Contains(mWatchedMediaItem, &contains);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!contains)
    mListener->OnItemRemoved(mWatchedMediaItem);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a media item watcher instance.
 */

sbMediaItemWatcher::sbMediaItemWatcher() :
  mBatchLevel(0)
{
  SB_PRLOG_SETUP(sbMediaItemWatcher);
  TRACE_FUNCTION("");
}


/**
 * Destroy a media item watcher instance.
 */

sbMediaItemWatcher::~sbMediaItemWatcher()
{
  TRACE_FUNCTION("");
  // Ensure the watcher is cancelled.
  Cancel();
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Handle an item updated event.
 */

nsresult
sbMediaItemWatcher::DoItemUpdated()
{
  nsresult rv;

  // Get current watched media item properties.
  nsAutoString properties;
  rv = GetWatchedMediaItemProperties(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  // Handle the item updated event.
  rv = DoItemUpdated(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Handle and item updated event with the new properties specified by
 * aItemProperties.
 *
 * \param aItemProperties       New item properties.
 */

nsresult
sbMediaItemWatcher::DoItemUpdated(nsAString& aItemProperties)
{
  // Update the watched media item properties.
  mWatchedMediaItemProperties.Assign(aItemProperties);

  // Notify the listener.
  if (mWatchedMediaItem) {
    mListener->OnItemUpdated(mWatchedMediaItem);
  }

  return NS_OK;
}


/**
 * Get the set of watched media item properties and return them in aProperties.
 *
 * \param aProperties           Watched media item properties.
 */

nsresult
sbMediaItemWatcher::GetWatchedMediaItemProperties(nsAString& aProperties)
{
  NS_ENSURE_TRUE(mWatchedMediaItem, NS_ERROR_NOT_AVAILABLE);

  nsresult rv;

  // Get the watched media item properties.
  nsCOMPtr<sbIPropertyArray> propertyArray;
  rv = mWatchedMediaItem->GetProperties(mWatchedPropertyIDs,
                                        getter_AddRefs(propertyArray));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = propertyArray->ToString(aProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

