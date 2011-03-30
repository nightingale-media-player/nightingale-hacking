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

/*** sbLibraryUpdateListener ***/

#include "sbDeviceLibraryHelpers.h"

#include <nsCOMPtr.h>

#include <sbIDevice.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbIOrderableMediaList.h>
#include <sbIPropertyArray.h>

#include "sbDeviceLibrary.h"

#include <sbDeviceUtils.h>
#include <sbLibraryUtils.h>
#include <sbMediaListBatchCallback.h>
#include <sbStandardProperties.h>

// note: this isn't actually threadsafe, but may be used on multiple threads
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLibraryUpdateListener, sbIMediaListListener)

sbLibraryUpdateListener::sbLibraryUpdateListener(
                                              sbILibrary * aTargetLibrary,
                                              bool aIgnorePlaylists,
                                              sbIDevice * aDevice)
  : mTargetLibrary(aTargetLibrary),
    mIgnorePlaylists(aIgnorePlaylists),
    mDevice(aDevice)
{
}

sbLibraryUpdateListener::~sbLibraryUpdateListener()
{
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemAdded(sbIMediaList *aMediaList,
                                     sbIMediaItem *aMediaItem,
                                     PRUint32 aIndex,
                                     PRBool *_retval)
{

  // Nothing to do on adding to the main library
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint32 aIndex,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (!list || !mIgnorePlaylists) {
    rv = sbDeviceUtils::SetOriginIsInMainLibrary(aMediaItem,
                                                 mTargetLibrary,
                                                 PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                            sbIMediaItem *aMediaItem,
                                            PRUint32 aIndex,
                                            PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemUpdated(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       sbIPropertyArray *aProperties,
                                       PRBool *_retval)
{
  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemMoved(sbIMediaList *aMediaList,
                                     PRUint32 aFromIndex,
                                     PRUint32 aToIndex,
                                     PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");

  if (_retval) {
    *_retval = PR_TRUE; /* STOP */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnListCleared(sbIMediaList *aMediaList,
                                       PRBool aExcludeLists,
                                       PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                             PRBool aExcludeLists,
                                             PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  return NS_OK;
}
