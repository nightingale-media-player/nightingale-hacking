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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Media list enumeration vector listener services.
//
//   These services provide a media list enumeration listener that collects
// enumerated media items in a vector.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  MLEVL.cpp
 * \brief Source for the media list enumeration vector listener services.
 */

//------------------------------------------------------------------------------
//
// Media list enumeration vector listener imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "MLEVL.h"


//------------------------------------------------------------------------------
//
// Media list enumeration vector listener nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(MLEVL, sbIMediaListEnumerationListener)


//------------------------------------------------------------------------------
//
//   Media list enumeration vector listener sbIMediaListEnumerationListener
// implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return true to begin enumeration, false to cancel.
 */

NS_IMETHODIMP
MLEVL::OnEnumerationBegin(sbIMediaList* aMediaList,
                          PRUint16*     _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);

  // Return results.
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return true to continue enumeration, false to cancel.
 */

NS_IMETHODIMP
MLEVL::OnEnumeratedItem(sbIMediaList* aMediaList,
                        sbIMediaItem* aMediaItem,
                        PRUint16*     _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  // Add the item to the enumeration vector.
  itemList.push_back(aMediaItem);

  // Return results.
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP
MLEVL::OnEnumerationEnd(sbIMediaList* aMediaList,
                        nsresult      aStatusCode)
{
  return NS_OK;
}


