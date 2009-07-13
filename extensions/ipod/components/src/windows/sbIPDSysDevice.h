/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
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
//=END SONGBIRD GPL
*/

#ifndef __SB_IPD_SYS_DEVICE_H__
#define __SB_IPD_SYS_DEVICE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod system dependent device services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDSysDevice.h
 * \brief Songbird iPod System Dependent Device Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod system dependent device imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"

/* Win32 imports. */
#include <windows.h>

#include <cfgmgr32.h>
#include <setupapi.h>


//------------------------------------------------------------------------------
//
// iPod system dependent device services classes.
//
//   All fields and methods are in the "not locked" category with respect to
// threading.
//
//------------------------------------------------------------------------------

/**
 * This class provides system dependent services for an iPod device.
 */

class sbIPDSysDevice : public sbIPDDevice
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // sbIDevice services.
  //

  NS_SCRIPTABLE NS_IMETHOD Eject(void);


  //
  // Constructors/destructors.
  //

  sbIPDSysDevice(const nsID&     aControllerID,
                 nsIPropertyBag* aProperties);

  ~sbIPDSysDevice();

  nsresult Initialize();

  void Finalize();


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Internal services fields.
  //
  //   mProperties              Set of device properties.
  //   mDevInst                 Win32 device instance.
  //

  nsCOMPtr<nsIPropertyBag>      mProperties;
  DEVINST                       mDevInst;


  //
  // Internal services.
  //

  nsresult GetFirewireGUID(nsAString& aFirewireGUID);

  nsresult GetDevInst(char     aDriveLetter,
                      DEVINST* aDevInst);

  nsresult GetDevNum(LPCTSTR aDevPath,
                     ULONG*  aDevNum);

  nsresult GetDevDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
                        SP_DEVINFO_DATA*                  aDevInfoData,
                        HDEVINFO                          aDevInfo,
                        GUID*                             aGUID,
                        DWORD                             aDevIndex);
};


//
// Auto-disposal class wrappers.
//
//   sbIPDAutoDevInfo           Wrapper to auto-destroy Win32 device info lists.
//   sbIPDAutoFile              Wrapper to auto-close Win32 files.
//

SB_AUTO_CLASS(sbIPDAutoDevInfo,
              HDEVINFO,
              mValue != INVALID_HANDLE_VALUE,
              SetupDiDestroyDeviceInfoList(mValue),
              mValue = INVALID_HANDLE_VALUE);
SB_AUTO_CLASS(sbIPDAutoFile,
              HANDLE,
              mValue != INVALID_HANDLE_VALUE,
              CloseHandle(mValue),
              mValue = INVALID_HANDLE_VALUE);


#endif // __SB_IPD_SYS_DEVICE_H__

