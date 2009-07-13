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

#ifndef __MLEVL_H__
#define __MLEVL_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Media list enumeration vector listener services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  MLEVL.h
 * \brief Header for the media list enumeration vector listener services.
 */

//------------------------------------------------------------------------------
//
// Media list enumeration vector listener imported services.
//
//------------------------------------------------------------------------------

// Songird imports.
#include <sbIMediaItem.h>
#include <sbIMediaListListener.h>

// Mozilla imports.
#include <nsCOMPtr.h>

// C++ STL imports.
#include <vector>


//------------------------------------------------------------------------------
//
// Media list enumeration vector listener services classes.
//
//------------------------------------------------------------------------------

/**
 * This class is a media list enumeration listener that adds the enumerated
 * items to a vector.
 *
 *   itemList                   List of enumerated items.
 */

class MLEVL : public sbIMediaListEnumerationListener
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
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER


  //
  // Public fields.
  //

  std::vector< nsCOMPtr<sbIMediaItem> >   itemList;
};

#endif // __MLEVL_H__

