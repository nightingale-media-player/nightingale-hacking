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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Temporary media item.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryMediaItem.cpp
 * \brief Songbird Temporary Media Item Source.
 */

//------------------------------------------------------------------------------
//
// Temporary media item imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbTemporaryMediaItem.h"

// Mozilla imports.
#include <nsEnumeratorUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(sbTemporaryMediaItem,
                              sbILibraryResource,
                              sbIMediaItem)


//------------------------------------------------------------------------------
//
// sbILibraryResource implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// GetProperty
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetProperty(const nsAString& aID,
                                  nsAString&       _retval)
{
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}


//-------------------------------------
//
// SetProperty
//

NS_IMETHODIMP
sbTemporaryMediaItem::SetProperty(const nsAString& aID,
                                  const nsAString& aValue)
{
  return NS_OK;
}


//-------------------------------------
//
// GetProperties
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetProperties(sbIPropertyArray*  aPropertyIDs,
                                    sbIPropertyArray** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//-------------------------------------
//
// SetProperties
//

NS_IMETHODIMP
sbTemporaryMediaItem::SetProperties(sbIPropertyArray* aProperties)
{
  return NS_OK;
}


//-------------------------------------
//
// Equals
//

NS_IMETHODIMP
sbTemporaryMediaItem::Equals(sbILibraryResource* aOtherLibraryResource,
                             PRBool*             _retval)
{
  NS_ENSURE_ARG_POINTER(aOtherLibraryResource);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = aOtherLibraryResource == this;
  return NS_OK;
}


//
// Getters/setters.
//

//-------------------------------------
//
// guid
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetGuid(nsAString& aGuid)
{
  aGuid.Truncate();
  return NS_OK;
}


//-------------------------------------
//
// created
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetCreated(PRInt64* aCreated)
{
  NS_ENSURE_ARG_POINTER(aCreated);
  *aCreated = 0;
  return NS_OK;
}


//-------------------------------------
//
// updated
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetUpdated(PRInt64* aUpdated)
{
  NS_ENSURE_ARG_POINTER(aUpdated);
  *aUpdated = 0;
  return NS_OK;
}


//-------------------------------------
//
// propertyIDs
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetPropertyIDs(nsIStringEnumerator** aPropertyIDs)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPropertyIDs);

  // Function variables.
  nsresult rv;

  // Create an empty property ID enumerator.
  nsCOMPtr<nsISimpleEnumerator> propertyIDs;
  rv = NS_NewEmptyEnumerator(getter_AddRefs(propertyIDs));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = CallQueryInterface(propertyIDs, aPropertyIDs);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// userEditableContent
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetUserEditable(PRBool* aUserEditable)
{
  NS_ENSURE_ARG_POINTER(aUserEditable);
  *aUserEditable = PR_FALSE;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// sbIMediaItem implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// TestIsURIAvailable
//

NS_IMETHODIMP
sbTemporaryMediaItem::TestIsURIAvailable(nsIObserver* aObserver)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//-------------------------------------
//
// OpenInputStreamAsync
//

NS_IMETHODIMP
sbTemporaryMediaItem::OpenInputStreamAsync(nsIStreamListener* aListener,
                                           nsISupports*       aContext,
                                           nsIChannel**       _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//-------------------------------------
//
// OpenInputStream
//

NS_IMETHODIMP
sbTemporaryMediaItem::OpenInputStream(nsIInputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//-------------------------------------
//
// OpenOutputStream
//

NS_IMETHODIMP
sbTemporaryMediaItem::OpenOutputStream(nsIOutputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//-------------------------------------
//
// ToString
//

NS_IMETHODIMP
sbTemporaryMediaItem::ToString(nsAString& _retval)
{
  _retval.Assign(NS_LITERAL_STRING("sbTemporaryMediaItem"));
  return NS_OK;
}


//
// Getters/setters.
//

//-------------------------------------
//
// library
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetLibrary(sbILibrary** aLibrary)
{
  return NS_ERROR_NOT_AVAILABLE;
}


//-------------------------------------
//
// isMutable
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetIsMutable(PRBool* aIsMutable)
{
  NS_ENSURE_ARG_POINTER(aIsMutable);
  *aIsMutable = PR_FALSE;
  return NS_OK;
}

//-------------------------------------
//
// isItemDisabled
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetItemController(sbIMediaItemController **aMediaItemController)
{
  NS_ENSURE_ARG_POINTER(aMediaItemController);
  *aMediaItemController = nsnull;
  return NS_OK;
}


//-------------------------------------
//
// mediaCreated
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetMediaCreated(PRInt64* aMediaCreated)
{
  NS_ENSURE_ARG_POINTER(aMediaCreated);
  *aMediaCreated = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryMediaItem::SetMediaCreated(PRInt64 aMediaCreated)
{
  return NS_OK;
}


//-------------------------------------
//
// mediaUpdated
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetMediaUpdated(PRInt64* aMediaUpdated)
{
  NS_ENSURE_ARG_POINTER(aMediaUpdated);
  *aMediaUpdated = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryMediaItem::SetMediaUpdated(PRInt64 aMediaUpdated)
{
  return NS_OK;
}


//-------------------------------------
//
// contentSrc
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetContentSrc(nsIURI** aContentSrc)
{
  NS_ENSURE_ARG_POINTER(aContentSrc);
  NS_IF_ADDREF(*aContentSrc = mContentSrc);
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryMediaItem::SetContentSrc(nsIURI* aContentSrc)
{
  mContentSrc = aContentSrc;
  return NS_OK;
}


//-------------------------------------
//
// contentLength
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetContentLength(PRInt64* aContentLength)
{
  NS_ENSURE_ARG_POINTER(aContentLength);
  *aContentLength = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryMediaItem::SetContentLength(PRInt64 aContentLength)
{
  return NS_OK;
}


//-------------------------------------
//
// contentType
//

NS_IMETHODIMP
sbTemporaryMediaItem::GetContentType(nsAString& aContentType)
{
  aContentType.Assign(mContentType);
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryMediaItem::SetContentType(const nsAString& aContentType)
{
  mContentType.Assign(aContentType);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbTemporaryMediaItem
//

sbTemporaryMediaItem::sbTemporaryMediaItem()
{
}


//-------------------------------------
//
// ~sbTemporaryMediaItem
//

sbTemporaryMediaItem::~sbTemporaryMediaItem()
{
}


