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
 * \brief Implementation of helper class for play queue library listener
 */

#include "sbPlayQueueLibraryListener.h"

#include <prlog.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbPlayQueueLibraryListener:5
 */
#ifdef PR_LOGGING
  static PRLogModuleInfo* gPlayQueueLibraryListenerLog = nsnull;
#define TRACE(args) PR_LOG(gPlayQueueLibraryListenerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gPlayQueueLibraryListenerLog, PR_LOG_WARN, args)
# ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif


NS_IMPL_ISUPPORTS1(sbPlayQueueLibraryListener,
                   sbIMediaListListener);

sbPlayQueueLibraryListener::sbPlayQueueLibraryListener()
 : mShouldIgnore(PR_FALSE)
{
  #if PR_LOGGING
    if (!gPlayQueueLibraryListenerLog) {
      gPlayQueueLibraryListenerLog = PR_NewLogModule("sbPlayQueueLibraryListener");
    }
  #endif /* PR_LOGGING */
  TRACE(("%s[%p]", __FUNCTION__, this));
}

sbPlayQueueLibraryListener::~sbPlayQueueLibraryListener()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
}

PRBool
sbPlayQueueLibraryListener::ShouldIgnore()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  return mShouldIgnore;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnBeforeListCleared(sbIMediaList* aMediaList,
                                                PRBool aExcludeLists,
                                                PRBool *aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // If the entire library is being cleared, the play queue service's mediaList
  // will get a notification that it is being cleared. Ignore that notification
  // to prevent recursing into calls to clear the libary.
  mShouldIgnore = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnListCleared(sbIMediaList* aMediaList,
                                          PRBool aExcludeLists,
                                          PRBool* aNoMoreForBatch)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  mShouldIgnore = PR_FALSE;
  return NS_OK;
}

// For the time being, we don't care about other listener methods on the
// library, so just ignore them and cancel batch notifications where possible.

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnBatchBegin(sbIMediaList* aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnBatchEnd(sbIMediaList* aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnItemAdded(sbIMediaList* aMediaList,
                                        sbIMediaItem* aMediaItem,
                                        PRUint32 aIndex,
                                        PRBool* aNoMoreForBatch)
{
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                sbIMediaItem* aMediaItem,
                                                PRUint32 aIndex,
                                                PRBool* aNoMoreForBatch)
{
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                               sbIMediaItem* aMediaItem,
                                               PRUint32 aIndex,
                                               PRBool* aNoMoreForBatch)
{
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnItemUpdated(sbIMediaList* aMediaList,
                                          sbIMediaItem* aMediaItem,
                                          sbIPropertyArray* aProperties,
                                          PRBool* aNoMoreForBatch)
{
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlayQueueLibraryListener::OnItemMoved(sbIMediaList* aMediaList,
                                        PRUint32 aFromIndex,
                                        PRUint32 aToIndex,
                                        PRBool* aNoMoreForBatch)
{
  if (aNoMoreForBatch) {
    *aNoMoreForBatch = PR_TRUE;
  }
  return NS_OK;
}

