/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef SB_WINDOWS_USB_DEVICE_UTILS_H_
#define SB_WINDOWS_USB_DEVICE_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows USB device utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsUSBDeviceUtils.h
 * \brief Songbird Windows USB Device Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows USB device utilities imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbUSBDeviceUtils.h"

// Songbird imports.
#include <sbMemoryUtils.h>

// Windows imports.
#include <windows.h>

#include <cfgmgr32.h>


//------------------------------------------------------------------------------
//
// Songbird Windows device utilities classes.
//
//------------------------------------------------------------------------------

/**
 * This class is used to reference a USB device.
 */

class sbWinUSBDeviceRef : public sbUSBDeviceRef
{
public:
  //
  // devInst                    USB device instance.
  // hubFile                    USB hub file for device.
  // portIndex                  USB hub port index for device.
  //

  DEVINST                       devInst;
  HANDLE                        hubFile;
  ULONG                         portIndex;

  sbWinUSBDeviceRef() :
    devInst(NULL),
    hubFile(INVALID_HANDLE_VALUE) {}
};


//------------------------------------------------------------------------------
//
// Songbird Windows USB device services.
//
//------------------------------------------------------------------------------

nsresult sbWinUSBDeviceOpenRef(DEVINST            aDevInst,
                               sbWinUSBDeviceRef* aDeviceInfo);

nsresult sbWinUSBDeviceCloseRef(sbWinUSBDeviceRef* aDeviceInfo);

nsresult sbWinUSBDeviceGetHubAndPort(DEVINST aDevInst,
                                     HANDLE* aHubFile,
                                     ULONG*  aPortIndex);

nsresult sbWinUSBHubFindDevicePort(HANDLE  aHubFile,
                                   DEVINST aDevInst,
                                   ULONG*  aPortIndex);

nsresult sbWinUSBHubGetPortDriverKey(HANDLE     aHubFile,
                                     ULONG      aPortIndex,
                                     nsAString& aDriverKey);


//------------------------------------------------------------------------------
//
// Songbird Windows USB MSC device services.
//
//------------------------------------------------------------------------------

nsresult sbWinUSBMSCGetLUN(DEVINST   aDevInst,
                           PRUint32* aLUN);


//------------------------------------------------------------------------------
//
// Songbird Windows USB device utilities classes.
//
//   These classes may depend upon function prototypes.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoWinUSBDeviceRef      Wrapper to auto-close Windows USB device
//                              reference objects.
//

SB_AUTO_NULL_CLASS(sbAutoWinUSBDeviceRef,
                   sbWinUSBDeviceRef*,
                   sbWinUSBDeviceCloseRef(mValue));


#endif // SB_WINDOWS_USB_DEVICE_UTILS_H_

