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

#ifndef SB_WINDOWS_DEVICE_UTILS_H_
#define SB_WINDOWS_DEVICE_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows device utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsDeviceUtils.h
 * \brief Songbird Windows Device Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows device utilities imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbMemoryUtils.h>

// Mozilla imports.
#include <nsStringGlue.h>

// Windows imports.
#include <objbase.h>

#include <cfgmgr32.h>
#include <setupapi.h>


//------------------------------------------------------------------------------
//
// Songbird Windows device file services.
//
//------------------------------------------------------------------------------

nsresult sbWinCreateDeviceFile(HANDLE*               aDevFile,
                               DEVINST               aDevInst,
                               const GUID*           aGUID,
                               DWORD                 aDesiredAccess,
                               DWORD                 aShareMode,
                               LPSECURITY_ATTRIBUTES aSecurityAttributes,
                               DWORD                 aCreationDisposition,
                               DWORD                 aFlagsAndAttributes,
                               HANDLE                aTemplateFile);

nsresult sbWinCreateAncestorDeviceFile
           (HANDLE*               aDevFile,
            DEVINST               aDevInst,
            const GUID*           aGUID,
            DWORD                 aDesiredAccess,
            DWORD                 aShareMode,
            LPSECURITY_ATTRIBUTES aSecurityAttributes,
            DWORD                 aCreationDisposition,
            DWORD                 aFlagsAndAttributes,
            HANDLE                aTemplateFile);

nsresult sbWinDeviceHasInterface(DEVINST     aDevInst,
                                 const GUID* aGUID,
                                 bool*     aHasInterface);

nsresult sbWinGetDevicePath(DEVINST     aDevInst,
                            const GUID* aGUID,
                            nsAString&  aDevicePath);


//------------------------------------------------------------------------------
//
// Songbird Windows device utilities services.
//
//------------------------------------------------------------------------------

nsresult sbWinFindDevicesByInterface
           (nsTArray<DEVINST>& aDevInstList,
            DEVINST            aRootDevInst,
            const GUID*        aGUID,
            bool             aSearchAncestors = PR_FALSE);

nsresult sbWinFindDeviceByClass(DEVINST*         aDevInst,
                                bool*          aFound,
                                DEVINST          aRootDevInst,
                                const nsAString& aClass);

nsresult sbWinGetDevInfoData(DEVINST          aDevInst,
                             HDEVINFO         aDevInfo,
                             PSP_DEVINFO_DATA aDevInfoData);

nsresult sbWinGetDevDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
                           SP_DEVINFO_DATA*                  aDevInfoData,
                           HDEVINFO                          aDevInfo,
                           const GUID*                       aGUID,
                           DWORD                             aDevIndex);

nsresult sbWinGetDevInterfaceDetail
           (PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
            HDEVINFO                          aDevInfo,
            SP_DEVINFO_DATA*                  aDevInfoData,
            const GUID*                       aGUID);

nsresult sbWinGetDeviceInstanceIDFromDeviceInterfaceName
           (nsAString& aDeviceInterfaceName,
            nsAString& aDeviceInstanceID);

nsresult sbWinGetDeviceInstanceID(DEVINST    aDevInst,
                                  nsAString& aDeviceInstanceID);

nsresult sbWinDeviceEject(DEVINST aDevInst);
nsresult sbWinDeviceEject(nsAString const & aMountPath);

nsresult sbWinDeviceIsDescendantOf(DEVINST aDevInst,
                                   DEVINST aDescendantDevInst,
                                   bool* aIsDescendant);

nsresult sbWinRegisterDeviceHandleNotification(HDEVNOTIFY* aDeviceNotification,
                                               HWND        aEventWindow,
                                               DEVINST     aDevInst,
                                               const GUID& aGUID);


//------------------------------------------------------------------------------
//
// Songbird Windows device utilities classes.
//
//   These classes may depend upon function prototypes.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoHDEVINFO             Wrapper to auto-destroy Win32 device info lists.
//

SB_AUTO_CLASS(sbAutoHDEVINFO,
              HDEVINFO,
              mValue != INVALID_HANDLE_VALUE,
              SetupDiDestroyDeviceInfoList(mValue),
              mValue = INVALID_HANDLE_VALUE);

#endif // SB_WINDOWS_DEVICE_UTILS_H_

