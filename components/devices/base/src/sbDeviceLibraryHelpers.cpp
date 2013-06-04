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

namespace
{
  /**
   * This is used to reset the origin is in main library property in a library
   * the sbIMediaListBatchCallback is used to perform the batch operation,
   * while the sbIMediaListEnumerationListener is used to set the property for
   * each item in the library
   */
  class EnumerateForOriginIsInMainLibrary :
    public sbIMediaListEnumerationListener,
    public sbIMediaListBatchCallback
  {
  public:
    NS_DECL_ISUPPORTS;

    /**
     * Set the library and whether to ignore playlists
     */
    void Initialize(sbILibrary * aLibrary,
                    bool aIgnorePlaylists)
    {
      mTargetLibrary = aLibrary;
      mIgnorePlaylists = aIgnorePlaylists;
    }

    // sbIMediaListEnumerationListener interface implementation
    /**
     * \brief Called when enumeration is about to begin.
     *
     * \param aMediaList - The media list that is being enumerated.
     *
     * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
     *         JavaScript callers may omit the return statement entirely to
     *         continue the enumeration.
     */
    /* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
    NS_SCRIPTABLE NS_IMETHOD OnEnumerationBegin(sbIMediaList *,
                                                PRUint16 *_retval)
    {
      NS_ENSURE_STATE(mTargetLibrary);
      if (_retval) {
        *_retval = CONTINUE;
      }
      return NS_OK;
    }

    /**
     * \brief Called once for each item in the enumeration.
     *
     * \param aMediaList - The media list that is being enumerated.
     * \param aMediaItem - The media item.
     *
     * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
     *         JavaScript callers may omit the return statement entirely to
     *         continue the enumeration.
     */

    NS_SCRIPTABLE NS_IMETHOD OnEnumeratedItem(sbIMediaList *aMediaList,
                                              sbIMediaItem *aMediaItem,
                                              PRUint16 *_retval)
    {
      nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
      if (!list || !mIgnorePlaylists)
      {
        NS_NAMED_LITERAL_STRING(SB_PROPERTY_FALSE, "0");

        nsresult rv;

        rv = aMediaItem->SetProperty(
                       NS_LITERAL_STRING(SB_PROPERTY_ORIGIN_IS_IN_MAIN_LIBRARY),
                       SB_PROPERTY_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      *_retval = CONTINUE;
      return NS_OK;
    }

    /**
     * \brief Called when enumeration has completed.
     *
     * \param aMediaList - The media list that is being enumerated.
     * \param aStatusCode - A code to determine if the enumeration was successful.
     */
    NS_SCRIPTABLE NS_IMETHOD OnEnumerationEnd(sbIMediaList *,
                                              nsresult aStatusCode)
    {
      return NS_OK;
    }

    // sbIMediaListBatchCallback interface

    /**
     * This updates the origin is in main library property in batch mode using
     * it's own object for the enumeration.
     */
    NS_SCRIPTABLE NS_IMETHOD RunBatched(nsISupports *)
    {
      nsresult rv;

      NS_ENSURE_STATE(mTargetLibrary);

      rv = mTargetLibrary->EnumerateAllItems(
                                         this,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }
  private:
    nsCOMPtr<sbILibrary> mTargetLibrary;
    bool mIgnorePlaylists;
  };

  NS_IMPL_ISUPPORTS1(EnumerateForOriginIsInMainLibrary,
                     sbIMediaListEnumerationListener);

}

NS_IMETHODIMP
sbLibraryUpdateListener::OnListCleared(sbIMediaList *aMediaList,
                                       PRBool aExcludeLists,
                                       PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  // Make sure we're dealing with the library
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

  // We clear the origin is in main library property for all device library
  // items. This is a bit of a short cut which is fine for the current
  // behavior. This will need to be changed, if we ever would import to
  // a non-main library.
  nsCOMPtr<sbILibrary> library = do_QueryInterface(aMediaList);
  if (library) {
    EnumerateForOriginIsInMainLibrary * enumerator;
    enumerator = new EnumerateForOriginIsInMainLibrary;
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
    enumerator->Initialize(mTargetLibrary,
                           mIgnorePlaylists);
    nsresult rv = mTargetLibrary->RunInBatchMode(enumerator, nsnull);
    NS_RELEASE(enumerator);
    NS_ENSURE_SUCCESS(rv, rv);
  }
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
