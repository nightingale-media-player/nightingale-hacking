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

#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>

class sbBaseDeviceController
{
public:
  sbBaseDeviceController();
  virtual ~sbBaseDeviceController();

protected:
  /** 
   * 
   */
  nsresult Init();

  /**
   * 
   */
  nsresult GetControllerIDInternal(nsID &aID);
  /**
   * 
   */
  nsresult SetControllerIDInternal(const nsID &aID);

  /**
   * 
   */
  nsresult GetControllerNameInternal(nsString &aName);
  /**
   * 
   */
  nsresult SetControllerNameInternal(const nsString &aName);

  /**
   * 
   */
  nsresult GetMarshallIDInternal(nsID &aID);
  /**
   * 
   */
  nsresult SetMarshallIDInternal(const nsID &aID);

  /**
   * 
   */
  nsresult AddDeviceInternal(sbIDevice *aDevice);
  /**
   * 
   */
  nsresult RemoveDeviceInternal(sbIDevice *aDevice);

protected:
  /** 
   *
   */
  PRMonitor* mMonitor;

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
