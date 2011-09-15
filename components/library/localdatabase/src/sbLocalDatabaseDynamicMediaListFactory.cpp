/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

/**
 * \file  sbLocalDatabaseDynamicMediaListFactory.cpp
 * \brief Songbird Local Database Dynamic Media List Factory Source.
 */

//------------------------------------------------------------------------------
//
// Songbird dynamic media list factory imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbLocalDatabaseDynamicMediaListFactory.h"

// Local imports.
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseDynamicMediaList.h"


//------------------------------------------------------------------------------
//
// Songbird dynamic media list factory nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbLocalDatabaseDynamicMediaListFactory, sbIMediaListFactory)


//------------------------------------------------------------------------------
//
// Songbird dynamic media list factory sbIMediaListFactory implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Create a new instance of a media list based on the template
 *        sbIMediaItem provided.
 *
 * \param aInner - An sbIMediaItem created by the library
 *
 * \return A object implementing the sbIMediaList interface.
 */

NS_IMETHODIMP
sbLocalDatabaseDynamicMediaListFactory::CreateMediaList(sbIMediaItem*  aInner,
                                                        sbIMediaList** _retval)
{
  return sbLocalDatabaseDynamicMediaList::New(aInner, _retval);
}


//
// Getters/setters.
//

/**
 * \brief A human-readable string identifying the type of media list that will
 *        be created by this factory.
 */

NS_IMETHODIMP
sbLocalDatabaseDynamicMediaListFactory::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("dynamic"));
  return NS_OK;
}


/**
 * \brief The contract ID through which an instance of this class can be
 *        created.
 */

NS_IMETHODIMP
sbLocalDatabaseDynamicMediaListFactory::GetContractID(nsACString& aContractID)
{
  aContractID.Assign(SB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CONTRACTID);
  return NS_OK;
}


