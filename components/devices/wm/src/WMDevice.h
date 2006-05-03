/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * \file  WMDevice.h
 * \brief Songbird WMDevice Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbIWMDevice.h"
#include "DeviceBase.h"

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_WMDevice_CONTRACTID  "@songbird.org/Songbird/Device/WMDevice;1"
#define SONGBIRD_WMDevice_CLASSNAME   "Songbird Download Device"
#define SONGBIRD_WMDevice_CID { 0x58061a82, 0x8ae7, 0x4d6b, { 0x95, 0xe4, 0x50, 0xc2, 0xa3, 0x1d, 0xef, 0x97 } }
// {58061A82-8AE7-4d6b-95E4-50C2A31DEF97}

// CLASSES ====================================================================

class sbDownloadListener;

class sbWMDevice :  public sbIWMDevice, public sbDeviceBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBIWMDEVICE

  sbWMDevice();

private:
  virtual void OnThreadBegin();
  virtual void OnThreadEnd();

  virtual bool IsEjectSupported();

  friend class sbDownloadListener;
private:
  ~sbWMDevice();

};
