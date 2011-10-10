/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef SB_WINDOWS_STORAGE_H_
#define SB_WINDOWS_STORAGE_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale Windows storage device services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsStorage.h
 * \brief Nightingale Windows Storage Device Services Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows storage device imported services.
//
//------------------------------------------------------------------------------

// Windows imports.
#include <devioctl.h>
#include <objbase.h>

#include <ntddstor.h>


//------------------------------------------------------------------------------
//
// Nightingale Windows storage device services.
//
//------------------------------------------------------------------------------

/**
 * Return in aStorageDevNum the storage device number for the device with the
 * file path specified by aDevPath.
 *
 * \param aDevPath              Device file path.
 * \param aStorageDevNum        Returned storage device number.
 */
HRESULT sbWinGetStorageDevNum(LPWSTR                 aDevPath,
                              STORAGE_DEVICE_NUMBER* aStorageDevNum);


#endif // SB_WINDOWS_STORAGE_H_

