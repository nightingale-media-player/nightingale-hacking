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
#include "sbDeviceStatus.h"

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
   * Delete all items that are marked not available (availability == 0)
   * in a medialist / library.
   * \param aMediaList The list or library to prune.
   */
  static nsresult DeleteUnavailableItems(/* in */ sbIMediaList *aMediaList);

  static nsresult CreateStatusFromRequest(/* in */ const nsAString &aDeviceID,
                                          /* in */ sbBaseDevice::TransferRequest *aRequest, 
                                          /* out */ sbDeviceStatus **aStatus);

  /**
   * Given a device and a media item, find the sbIDeviceLibrary for it
   * this is necessary because the device library is a wrapper
   */
  static nsresult GetDeviceLibraryForItem(/* in */  sbIDevice* aDevice,
                                          /* in */  sbIMediaItem* aItem,
                                          /* out */ sbIDeviceLibrary** _retval);

  /**
   * Stores the enumerated items in an nsIArray
   * \see sbIMediaList::EnumerateAllItems
   * \see sbIMediaList::EnumerateItemsByProperty
   * \see sbIMediaList::EnumerateItemsByProperies
   */
  class SnapshotEnumerationListener : public sbIMediaListEnumerationListener
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
  
    nsresult Init();
    nsresult GetArray(nsIArray **aArray);
  
  protected:
    virtual ~SnapshotEnumerationListener() {};
    nsCOMPtr<nsIMutableArray> mArray;
  };
};



#endif /* __SBDEVICEUTILS__H__ */
