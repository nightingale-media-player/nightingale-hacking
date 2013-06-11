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

#include <sbIDevice.h>

#include <prmon.h>
#include <nsID.h>
#include <nsHashKeys.h>

#include <mozilla/ReentrantMonitor.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsIMutableArray.h>
#include <nsStringGlue.h>

class sbBaseDeviceController
{
public:
  sbBaseDeviceController();

protected:
  virtual ~sbBaseDeviceController();

protected:
  // copies the given hash table into the given mutable array
  template<class T>
  static NS_HIDDEN_(PLDHashOperator) EnumerateIntoArray(const nsID& aKey,
                                                        T* aData,
                                                        void* aArray);
  template<class T>
  static NS_HIDDEN_(PLDHashOperator) EnumerateConnectAll(const nsID& aKey,
                                                         T* aData,
                                                         void* aArray);

  template<class T>
  static NS_HIDDEN_(PLDHashOperator) EnumerateDisconnectAll(const nsID& aKey,
                                                            T* aData,
                                                            void* aArray);

  /**
   * 
   */
  nsresult GetControllerIdInternal(nsID &aID);
  /**
   * 
   */
  nsresult SetControllerIdInternal(const nsID &aID);

  /**
   * 
   */
  nsresult GetControllerNameInternal(nsAString &aName);
  /**
   * 
   */
  nsresult SetControllerNameInternal(const nsAString &aName);

  /**
   * 
   */
  nsresult GetMarshallIdInternal(nsID &aID);
  /**
   * 
   */
  nsresult SetMarshallIdInternal(const nsID &aID);

  /**
   * 
   */
  nsresult AddDeviceInternal(sbIDevice *aDevice);
  /**
   * 
   */
  nsresult RemoveDeviceInternal(sbIDevice *aDevice);

  /**
   * 
   */
  nsresult GetDeviceInternal(const nsID * aID, sbIDevice* *aDevice);

  /**
   * 
   */
  nsresult GetDevicesInternal(nsIArray* *aDevices);

  /**
   * 
   */
  nsresult ControlsDeviceInternal(sbIDevice *aDevice, PRBool *_retval);

  /**
   * 
   */
  nsresult ConnectDevicesInternal();
  
  /**
   * 
   */
  nsresult DisconnectDevicesInternal();

  /**
   * 
   */
  nsresult ReleaseDeviceInternal(sbIDevice *aDevice);

  /**
   * 
   */
  nsresult ReleaseDevicesInternal();

private:
  /** 
   *
   */
  mozilla::ReentrantMonitor mMonitor;

  /** 
   * 
   */
  nsID mControllerID;

  /** 
   * 
   */
  nsString mControllerName;

  /** 
   * 
   */
  nsID mMarshallID;

  /** 
   * 
   */
  nsInterfaceHashtable<nsIDHashKey, sbIDevice> mDevices;
};
