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

#ifndef __SBDEVICEUTILS__H__
#define __SBDEVICEUTILS__H__

#include <nscore.h>
#include <nsCOMPtr.h>
#include <nsIMutableArray.h>
#include <nsStringGlue.h>

#include <sbIMediaListListener.h>

#include "sbBaseDevice.h"
#include "sbIDeviceStatus.h"

class nsIFile;
class nsIMutableArray;

/**
 * Utilities to aid in implementing devices
 */
class sbDeviceUtils
{
public:
  /**
   * Give back a nsIFile pointing to where a file should be transferred to
   * using the media item properties to build a directory structure.
   * Note that the resulting file may reside in a directory that does not yet
   * exist.
   * @param aParent the directory to place the file
   * @param aItem the media item to be transferred
   * @return the file path to store the item in
   */
  static nsresult GetOrganizedPath(/* in */ nsIFile *aParent,
                                   /* in */ sbIMediaItem *aItem,
                                   nsIFile **_retval);

  /**
   * Sets a property on all items that match a filter in a list / library.
   * \param aMediaList      [in] The list or library to process.
   * \param aPropertyId     [in] The id of the property to set.
   * \param aPropertyValue  [in] The value of the property to set to.
   * \param aPoprertyFilter [in] A set of properties to filter on.
   *                             Pass in null to set on all items.
   *
   * For example, to set all items in a library as unavailable, call:
   * sbDeviceUtils::BulkSetProperty(library,
   *                                NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
   *                                NS_LITERAL_STRING("0"));
   */
  static nsresult BulkSetProperty(sbIMediaList *aMediaList,
                                  const nsAString& aPropertyId,
                                  const nsAString& aPropertyValue,
                                  sbIPropertyArray* aPropertyFilter = nsnull);

  /**
   * Delete all items that have the property with the given value
   * \param aMediaList the list to remove items from
   * \param aPropertyId the ID of the property we're interested in
   * \param aValue The value of the property that if matched will be deleted
   */
  static nsresult DeleteByProperty(sbIMediaList *aMediaList,
                                   nsAString const & aProperty,
                                   nsAString const & aValue);
  /**
   * Delete all items that are marked not available (availability == 0)
   * in a medialist / library.
   * \param aMediaList The list or library to prune.
   */
  static nsresult DeleteUnavailableItems(/* in */ sbIMediaList *aMediaList);

  /**
   * Given a device and a media item, find the sbIDeviceLibrary for it
   * this is necessary because the device library is a wrapper
   */
  static nsresult GetDeviceLibraryForItem(/* in */  sbIDevice* aDevice,
                                          /* in */  sbIMediaItem* aItem,
                                          /* out */ sbIDeviceLibrary** _retval);

  /**
   * Search the library specified by aLibrary for an item with a device
   * persisent ID property matching aDevicePersistentId.  Return the found item
   * in aItem.
   *
   * \param aLibrary            [in]  Library in which to search for items.
   * \param aDevicePersistentId [in]  Device persistent ID for which to search.
   * \param aItem               [out] Found item.
   */
  static nsresult GetMediaItemByDevicePersistentId
                    (/* in */  sbILibrary*      aLibrary,
                     /* in */  const nsAString& aDevicePersistentId,
                     /* out */ sbIMediaItem**   aItem);

  /**
   * Search the library specified by aLibrary for an item with a device
   * persistent ID property matching aDevicePersistentId.  Return that item's
   * origin media item in aItem.
   *
   * \param aLibrary            [in]  Library in which to search for items.
   * \param aDevicePersistentId [in]  Device persistent ID for which to search.
   * \param aItem               [out] Found origin item.
   */

  static nsresult GetOriginMediaItemByDevicePersistentId
                    (/* in */  sbILibrary*      aLibrary,
                     /* in */  const nsAString& aDevicePersistentId,
                     /* out */ sbIMediaItem**   aItem);

  /**
   * Ask the user what action to take in response to an operation space exceeded
   * event for the device specified by aDevice and device library specified by
   * aLibrary.  The amount of space needed for the operation is specified by
   * aSpaceNeeded and the amount available by aSpaceAvailable.  If the user
   * chooses to abort the operation, true is returned in aAbort.
   *
   * \param aDevice         [in] Target device of operation.
   * \param aLibrary        [in] Target device library of operation.
   * \param aSpaceNeeded    [in] Space needed by operation.
   * \param aSpaceAvailable [in] Space available to operation.
   * \param aAbort          [out] True if user selected to abort operation.
   */

  static nsresult QueryUserSpaceExceeded
                    (/* in */  sbIDevice*        aDevice,
                     /* in */  sbIDeviceLibrary* aLibrary,
                     /* in */  PRInt64           aSpaceNeeded,
                     /* in */  PRInt64           aSpaceAvailable,
                     /* out */ PRBool*           aAbort);

  /**
   * Check if the device specified by aDevice is linked to the local sync
   * partner.  If it is, return true in aIsLinkedLocally; otherwise, return
   * false.  If aRequestPartnerChange is true and the device is not linked
   * locally, make a request to the user to change the device sync partner to
   * the local sync partner.
   *
   * \param aDevice                 Device to check.
   * \param aRequestPartnerChange   Request that the sync partner be changed to
   *                                the local sync partner.
   * \param aIsLinkedLocally        Returned true if the device is linked to the
   *                                local sync partner.
   */
  static nsresult SyncCheckLinkedPartner(sbIDevice* aDevice,
                                         PRBool     aRequestPartnerChange,
                                         PRBool*    aIsLinkedLocally);

  /**
   * Make a request of the user to change the sync partner of the device
   * specified by aDevice to the local sync partner.  If the user grants the
   * request, return true in aPartnerChangeGranted; otherwise, return false.
   *
   * \param aDevice                 Device.
   * \param aPartnerChangeGranted   Returned true if the user granted the
   *                                request.
   */
  static nsresult SyncRequestPartnerChange(sbIDevice* aDevice,
                                           PRBool*    aPartnerChangeGranted);
};


#endif /* __SBDEVICEUTILS__H__ */
