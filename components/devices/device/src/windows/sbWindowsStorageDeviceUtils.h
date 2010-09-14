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

#ifndef SB_WINDOWS_STORAGE_DEVICE_UTILS_H_
#define SB_WINDOWS_STORAGE_DEVICE_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows storage device utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsStorageDeviceUtils.h
 * \brief Songbird Windows Storage Device Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows storage device utilities imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsStringGlue.h>
#include <nsTArray.h>

// Windows imports.
#include <devioctl.h>
#include <windows.h>

#include <cfgmgr32.h>
#include <ntddstor.h>
#include <setupapi.h>


//------------------------------------------------------------------------------
//
// Songbird Windows device storage services.
//
//------------------------------------------------------------------------------

nsresult sbWinFindDevicesByStorageDevNum
           (STORAGE_DEVICE_NUMBER* aStorageDevNum,
            PRBool                 aMatchPartitionNumber,
            const GUID*            aGUID,
            nsTArray<DEVINST>&     aDevInstList);

nsresult sbWinGetStorageDevNum(DEVINST                aDevInst,
                               const GUID*            aGUID,
                               STORAGE_DEVICE_NUMBER* aStorageDevNum);

nsresult sbWinGetStorageDevNum(LPCTSTR                aDevPath,
                               STORAGE_DEVICE_NUMBER* aStorageDevNum);


//------------------------------------------------------------------------------
//
// Songbird Windows volume device services.
//
//------------------------------------------------------------------------------

nsresult sbWinVolumeIsReady(DEVINST aDevInst,
                            PRBool* aIsReady);

nsresult sbWinVolumeGetIsReadOnly(const nsAString& aVolumeMountPath,
                                  PRBool*          aIsReadOnly);

nsresult sbWinGetVolumeGUIDPath(DEVINST    aDevInst,
                                nsAString& aVolumeGUIDPath);

nsresult sbWinGetVolumeGUID(DEVINST    aDevInst,
                            nsAString& aVolumeGUID);

nsresult sbWinGetVolumePathNames(DEVINST             aDevInst,
                                 nsTArray<nsString>& aPathNames);

nsresult sbWinGetVolumePathNames(nsAString&          aVolumeGUIDPath,
                                 nsTArray<nsString>& aPathNames);

nsresult sbWinGetVolumeLabel(const nsAString& aVolumeMountPath,
                             nsACString&      aVolumeLabel);

nsresult sbWinSetVolumeLabel(const nsAString&  aVolumeMountPath,
                             const nsACString& aVolumeLabel);


//------------------------------------------------------------------------------
//
// Songbird Windows SCSI device utilities services.
//
//------------------------------------------------------------------------------

nsresult sbWinGetSCSIProductInfo(DEVINST    aDevInst,
                                 nsAString& aVendorID,
                                 nsAString& aProductID);


#endif // SB_WINDOWS_STORAGE_DEVICE_UTILS_H_

