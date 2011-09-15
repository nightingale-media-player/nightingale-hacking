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

#ifndef __SB_IPD_CONTROLLER_H__
#define __SB_IPD_CONTROLLER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device controller defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDController.h
 * \brief Songbird iPod Device Controller Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device controller imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbBaseDeviceController.h>
#include <sbIDeviceController.h>

// Mozilla imports.
#include <nsIClassInfo.h>


//------------------------------------------------------------------------------
//
// iPod device controller definitions.
//
//------------------------------------------------------------------------------

//
// iPod device controller XPCOM component definitions.
//XXXeps should move out of platform specific file
//

#define SB_IPDCONTROLLER_CONTRACTID "@songbirdnest.com/Songbird/IPDController;1"
#define SB_IPDCONTROLLER_CLASSNAME "iPod Device Controller"
#define SB_IPDCONTROLLER_DESCRIPTION "iPod Device Controller"
#define SB_IPDCONTROLLER_CID                                                   \
{                                                                              \
  0x72c755b2,                                                                  \
  0xd735,                                                                      \
  0x4a08,                                                                      \
  { 0x87, 0x41, 0x9e, 0x62, 0x1b, 0xc4, 0xb5, 0xd5 }                           \
}


//------------------------------------------------------------------------------
//
// iPod device controller classes.
//
//------------------------------------------------------------------------------

/**
 * This class controls all iPod devices.
 */

class sbIPDController : public sbBaseDeviceController,
                        public sbIDeviceController,
                        public nsIClassInfo
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICECONTROLLER
  NS_DECL_NSICLASSINFO


  //
  // Constructors/destructors.
  //

  sbIPDController();

  virtual ~sbIPDController();
};


#endif // __SB_IPD_CONTROLLER_H__

