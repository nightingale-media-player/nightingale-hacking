/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file  sbWindowsStorage.cpp
 * \brief Nightingale Windows Storage Device Services Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows storage device imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsStorage.h"

// Local imports.
#include "sbWindowsUtils.h"


//------------------------------------------------------------------------------
//
// Nightingale Windows storage device services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWinGetStorageDevNum
//

HRESULT
sbWinGetStorageDevNum(LPWSTR                 aDevPath,
                      STORAGE_DEVICE_NUMBER* aStorageDevNum)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aDevPath);
  SB_WIN_ENSURE_ARG_POINTER(aStorageDevNum);

  // Function variables.
  BOOL success;

  // Create a device file and set it up for auto-disposal.
  HANDLE devFile = CreateFileW(aDevPath,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
  SB_WIN_ENSURE_TRUE(devFile != INVALID_HANDLE_VALUE,
                     HRESULT_FROM_WIN32(GetLastError()));
  sbAutoHANDLE autoDevFile(devFile);

  // Get the device number.
  DWORD byteCount;
  success = DeviceIoControl(devFile,
                            IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            NULL,
                            0,
                            aStorageDevNum,
                            sizeof(STORAGE_DEVICE_NUMBER),
                            &byteCount,
                            NULL);
  SB_WIN_ENSURE_TRUE(success, HRESULT_FROM_WIN32(GetLastError()));
  SB_WIN_ENSURE_TRUE(aStorageDevNum->DeviceNumber != 0xFFFFFFFF,
                     E_FAIL);

  return S_OK;
}


