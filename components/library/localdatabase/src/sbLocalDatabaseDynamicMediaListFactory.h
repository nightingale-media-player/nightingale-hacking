/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_FACTORY_H__
#define __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_FACTORY_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale dynamic media list factory defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbLocalDatabaseDynamicMediaListFactory.h
 * \brief Nightingale Local Database Dynamic Media List Factory Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale dynamic media list factory imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIMediaListFactory.h>


//------------------------------------------------------------------------------
//
// Nightingale dynamic media list factory classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements a dynamic media list factory.
 */

class sbLocalDatabaseDynamicMediaListFactory : public sbIMediaListFactory
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // Interface implementations.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTFACTORY
};


#endif // __SB_LOCAL_DATABASE_DYNAMIC_MEDIALIST_FACTORY_H__

