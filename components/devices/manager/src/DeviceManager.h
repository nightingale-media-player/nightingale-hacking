/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/** 
 * \file  DeviceManager.h
 * \brief Songbird WMDevice Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbIDeviceManager.h"
#include "nsString.h"

#include <xpcom/nsCOMPtr.h>

#include <list>

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_DeviceManager_CONTRACTID  "@songbird.org/Songbird/DeviceManager;1"
#define SONGBIRD_DeviceManager_CLASSNAME   "Songbird Device Manager"
#define SONGBIRD_DeviceManager_CID { 0x937075df, 0x1597, 0x45f4, { 0x97, 0xc7, 0xd3, 0x23, 0x97, 0xcd, 0xd, 0x62 } }
// {937075DF-1597-45f4-97C7-D32397CD0D62}

// CLASSES ====================================================================

class sbDeviceManager :  public sbIDeviceManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEMANAGER

  sbDeviceManager();

  static sbDeviceManager* GetSingleton();

private:
  ~sbDeviceManager();


  sbIDeviceBase* GetDeviceMatchingCategory(const PRUnichar *DeviceCategory);
  
  void InializeInternal();
  void FinalizeInternal();

  static sbDeviceManager* mSingleton;

  typedef nsCOMPtr<sbIDeviceBase> _Device;
  std::list<_Device> mSupportedDevices;
  bool mIntialized;
  bool mFinalized;
};
