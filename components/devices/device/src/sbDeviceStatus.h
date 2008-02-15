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
#ifndef SBDEVICESTATUS_H_
#define SBDEVICESTATUS_H_

#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

class sbIDataRemote;
class nsIProxyObjectManager;

/**
 * This class is used to publish status information for the device.
 */
class sbDeviceStatus : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  /**
   * Initializses the status object with the device ID
   */
  static already_AddRefed<sbDeviceStatus> New(nsAString const & deviceID);
public:
  /**
   * Sets the state message
   */
  nsresult StateMessage(nsString const & msg);
  /**
   * Sets the current operation
   */
  nsresult CurrentOperation(nsString const & operationMessage);
  /**
   * Sets the current progress as a percent. Done is 100.0.
   */
  nsresult Progress(double percent);
  /**
   * Sets the current work item # for example processing multiple files
   */
  nsresult WorkItemProgress(PRUint32 current);
  /**
   * Sets the end count for processing multiple items
   */
  nsresult WorkItemProgressEndCount(PRUint32 count);
  /**
   * Sets the item currently being acted on
   */
  void SetItem(sbIMediaItem * item)
  {
    mItem = item;
  }
  /**
   * Sets the list (Context) of the action
   */
  void SetList(sbIMediaList * list)
  {
    mList = list;
  }
private:
  /**
   * Initializses the status object with the device ID
   */
  sbDeviceStatus(nsAString const & deviceID);
  /**
   * cleanup
   */
  virtual ~sbDeviceStatus();

  nsString const mDeviceID;
  nsCOMPtr<sbIDataRemote> mStatusRemote;
  nsCOMPtr<sbIDataRemote> mOperationRemote;
  nsCOMPtr<sbIDataRemote> mProgressRemote;
  nsCOMPtr<sbIDataRemote> mWorkCurrentCountRemote;
  nsCOMPtr<sbIDataRemote> mWorkTotalCountRemote;
  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIMediaList> mList;
  
  nsresult GetDataRemote(nsIProxyObjectManager* aProxyObjectManager,
                         const nsAString& aDataRemoteName,
                         const nsAString& aDataRemotePrefix,
                         void** appDataRemote);
  
  void Init();
};

#endif /*SBDEVICESTATUS_H_*/
