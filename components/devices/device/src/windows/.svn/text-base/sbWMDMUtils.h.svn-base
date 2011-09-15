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

#ifndef SB_WMDM_UTILS_H_
#define SB_WMDM_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird WMDM utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWMDMUtils.h
 * \brief Songbird WMDM Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird WMDM utilities imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsStringGlue.h>

// Windows imports.
#include <mswmdm.h>


//------------------------------------------------------------------------------
//
// Songbird WMDM utilities services.
//
//------------------------------------------------------------------------------

/**
 * Return in aDevice the WMDM device with the device instance ID specified by
 * aDeviceInstanceID.
 *
 * \param aDeviceInstanceID     Instance ID of WMDM device.
 * \param aDevice               Returned WMDM device.
 *
 * \returns NS_ERROR_NOT_AVAILABLE
 *                              No WMDM device corresponds to device instance
 *                              ID.
 */
nsresult sbWMDMGetDeviceFromDeviceInstanceID(nsAString&    aDeviceInstanceID,
                                             IWMDMDevice** aDevice);

/**
 * Return in aDeviceManager the Windows media device manager.
 *
 * \param aDeviceManager        Windows media device manager.
 */
nsresult sbWMDMGetDeviceManager(IWMDeviceManager** aDeviceManager);


#endif // SB_WMDM_UTILS_H_

