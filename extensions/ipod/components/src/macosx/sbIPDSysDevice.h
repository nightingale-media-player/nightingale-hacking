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

// MacOS X imports.
#include <Carbon/Carbon.h>


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

  nsresult Eject();


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
  //   mVolumeRefNum            Device volume reference number.
  //

  nsCOMPtr<nsIPropertyBag>      mProperties;
  FSVolumeRefNum                mVolumeRefNum;
};


#endif // __SB_IPD_SYS_DEVICE_H__

