/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

#ifndef __SB_STRINGBUNDLESERVICE_H__
#define __SB_STRINGBUNDLESERVICE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringBundleService.h
 * \brief Songbird String Bundle Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird string bundle service imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIStringBundleService.h>

// Mozilla imports.
#include <nsCOMPtr.h>


//------------------------------------------------------------------------------
//
// Songbird string bundle service defs.
//
//------------------------------------------------------------------------------

//
// Songbird string bundle service component defs.
//

#define SB_STRINGBUNDLESERVICE_CLASSNAME "sbStringBundleService"
#define SB_STRINGBUNDLESERVICE_CID                                             \
  /* {04134658-9CF9-4FEA-90C0-BC159C016037} */                                 \
  {                                                                            \
    0x04134658,                                                                \
    0x9CF9,                                                                    \
    0x4FEA,                                                                    \
    { 0x90, 0xC0, 0xBC, 0x15, 0x9C, 0x01, 0x60, 0x37 }                         \
  }


//------------------------------------------------------------------------------
//
// Songbird string bundle service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the Songbird string bundle service component.
 */

class sbStringBundleService : public sbIStringBundleService
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
  NS_DECL_SBISTRINGBUNDLESERVICE


  //
  // Public services.
  //

  sbStringBundleService();

  virtual ~sbStringBundleService();

  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mBundle                    Main Songbird string bundle.
  // mBrandBundle               Main Songbird brand string bundle.
  //

  nsCOMPtr<nsIStringBundle>     mBundle;
  nsCOMPtr<nsIStringBundle>     mBrandBundle;
};


#endif // __SB_STRINGBUNDLESERVICE_H__

