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
/*** sbLibraryItemUpdateListener ***/

#include "sbDeviceLibraryHelpers.h"

#include <nsCOMPtr.h>

#include <sbIPropertyArray.h>

#include <sbLibraryUtils.h>

// note: this isn't actually threadsafe, but may be used on multiple threads
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLibraryItemUpdateListener, sbIMediaListListener)

sbLibraryItemUpdateListener::sbLibraryItemUpdateListener(sbILibrary* aTargetLibrary)
  : mTargetLibrary(aTargetLibrary)
{ }

NS_IMETHODIMP
sbLibraryItemUpdateListener::OnItemAdded(sbIMediaList *aMediaList,
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
sbLibraryItemUpdateListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
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
sbLibraryItemUpdateListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
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
sbLibraryItemUpdateListener::OnItemUpdated(sbIMediaList *aMediaList,
                                           sbIMediaItem *aMediaItem,
                                           sbIPropertyArray *aProperties,
                                           PRBool *_retval)
{
  nsresult rv;
  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }
  
  nsCOMPtr<sbIMediaItem> targetItem;
  rv = sbLibraryUtils::GetItemInLibrary(aMediaItem,
                                        mTargetLibrary,
                                        getter_AddRefs(targetItem));
  NS_ENSURE_SUCCESS(rv, rv);
  if (targetItem) {
    // the property array here are the old values; we need the new ones
    nsCOMPtr<sbIPropertyArray> newProps;
    rv = aMediaItem->GetProperties(aProperties, getter_AddRefs(newProps));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = targetItem->SetProperties(newProps);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryItemUpdateListener::OnItemMoved(sbIMediaList *aMediaList,
                                         PRUint32 aFromIndex,
                                         PRUint32 aToIndex,
                                         PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryItemUpdateListener::OnListCleared(sbIMediaList *aMediaList,
                                           PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryItemUpdateListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryItemUpdateListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  return NS_OK;
}
