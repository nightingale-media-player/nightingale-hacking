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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art fetcher set.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtFetcherSet.cpp
 * \brief Songbird Album Art Fetcher Set Source.
 */

//------------------------------------------------------------------------------
//
// Songbird album art fetcher set imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbAlbumArtFetcherSet.h"

// Mozilla imports.
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(sbAlbumArtFetcherSet,
                              sbIAlbumArtFetcherSet,
                              sbIAlbumArtFetcher)


//------------------------------------------------------------------------------
//
// sbIAlbumArtFetcherSet implementation.
//
//------------------------------------------------------------------------------

//
// Getters/setters.
//

/**
 * \brief If true, only attempt to fetch album art from local sources.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetLocalOnly(PRBool* aLocalOnly)
{
  NS_ENSURE_ARG_POINTER(aLocalOnly);
  *aLocalOnly = mLocalOnly;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetLocalOnly(PRBool aLocalOnly)
{
  mLocalOnly = aLocalOnly;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// sbIAlbumArtFetcher implementation.
//
//------------------------------------------------------------------------------

/* \brief try to fetch album art for the given media item
 * \param aMediaItem the media item that we're looking for album art for
 * \param aListener the listener to inform of success or failure
 * \param aWindow the window this was called from, can be null
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::FetchAlbumArtForMediaItem
                         (sbIMediaItem*        aMediaItem,
                          sbIAlbumArtListener* aListener,
                          nsIDOMWindow*        aWindow)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Reset fetcher state.
  mIsComplete = PR_FALSE;
  mFoundAlbumArt = PR_FALSE;

  // Get the list of fetchers.
  nsCOMPtr<nsIArray> fetcherList;
  rv = mAlbumArtService->GetFetcherList(mLocalOnly,
                                        getter_AddRefs(fetcherList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Try each fetcher until one is successful.
  PRUint32 fetcherListCount;
  rv = fetcherList->GetLength(&fetcherListCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < fetcherListCount; i++) {
    // Get the next fetcher contract ID.
    nsCAutoString fetcherContractID;
    nsCOMPtr<nsIVariant>
      fetcherContractIDVariant = do_QueryElementAt(fetcherList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = fetcherContractIDVariant->GetAsACString(fetcherContractID);
    NS_ENSURE_SUCCESS(rv, rv);

    // Try fetching album art using the fetcher.  Continue fetching on error.
    FetchAlbumArtForMediaItem(fetcherContractID.get(),
                              aMediaItem,
                              aWindow,
                              &mFoundAlbumArt);
    if (mFoundAlbumArt)
      break;
  }

  // Update fetcher state.
  mIsComplete = PR_TRUE;

  return NS_OK;
}


/* \brief shut down the fetcher
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::Shutdown()
{
  return NS_OK;
}


//
// Getters/setters.
//

/**
 * \brief Short name of AlbumArtFetcher.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetShortName(nsAString& aShortName)
{
  aShortName.AssignLiteral("set");
  return NS_OK;
}


/**
 * \brief Name of AlbumArtFetcher to display to the user on things like
 *        menus.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetName(nsAString& aName)
{
  aName.AssignLiteral("set");
  return NS_OK;
}


/**
 * \brief Description of the AlbumArtFetcher to display to the user.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetDescription(nsAString& aDescription)
{
  aDescription.AssignLiteral("set");
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher can be used as a fetcher from a
 *        user menu.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetUserFetcher(PRBool* aUserFetcher)
{
  NS_ENSURE_ARG_POINTER(aUserFetcher);
  *aUserFetcher = PR_TRUE;
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher fetches from local sources.
 *XXXeps it can fetch both locally and remotely, but it really doesn't apply to
 *XXXeps this component.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetIsLocal(PRBool* aIsLocal)
{
  NS_ENSURE_ARG_POINTER(aIsLocal);
  *aIsLocal = PR_TRUE;
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher is enabled or not
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetIsEnabled(PRBool* aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetIsEnabled(PRBool aIsEnabled)
{
  return NS_OK;
}


/**
 * \brief Flag to indicate if fetching is complete.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetIsComplete(PRBool* aIsComplete)
{
  NS_ENSURE_ARG_POINTER(aIsComplete);
  *aIsComplete = mIsComplete;
  return NS_OK;
}


/**
 * \brief Flag to indicate whether album art was found.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetFoundAlbumArt(PRBool* aFoundAlbumArt)
{
  NS_ENSURE_ARG_POINTER(aFoundAlbumArt);
  *aFoundAlbumArt = mFoundAlbumArt;
  return NS_OK;
}


/**
 * \brief List of sources of album art (e.g., sbIMetadataHandler).
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetAlbumArtSourceList(nsIArray** aAlbumArtSourceList)
{
  NS_ENSURE_ARG_POINTER(aAlbumArtSourceList);
  NS_ADDREF(*aAlbumArtSourceList = mAlbumArtSourceList);
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetAlbumArtSourceList(nsIArray* aAlbumArtSourceList)
{
  mAlbumArtSourceList = aAlbumArtSourceList;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an album art fetcher set instance.
 */

sbAlbumArtFetcherSet::sbAlbumArtFetcherSet() :
  mIsComplete(PR_FALSE),
  mFoundAlbumArt(PR_FALSE),
  mLocalOnly(PR_FALSE)
{
}


/**
 * Destroy an album art fetcher set instance.
 */

sbAlbumArtFetcherSet::~sbAlbumArtFetcherSet()
{
}


/**
 * Initialize the album art fetcher set.
 */

nsresult
sbAlbumArtFetcherSet::Initialize()
{
  nsresult rv;

  // Get the album art service.
  mAlbumArtService = do_GetService(SB_ALBUMARTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Try fetching album art for the media item specified by aMediaItem using the
 * album art fetcher with the contract ID specified by
 * aAlbumArtFetcherContractID.  If album art was found, return true in
 * aFoundAlbumArt.
 *
 * \param aAlbumArtFetcherContractID
 *                              Contract ID of album art fetcher component.
 * \param aMediaItem            Media item for which to fetcher album art.
 * \param aWindow               The window this was called from, can be null.
 * \param aFoundAlbumArt        Set to true if album art was found.
 */

nsresult
sbAlbumArtFetcherSet::FetchAlbumArtForMediaItem
                        (const char*   aAlbumArtFetcherContractID,
                         sbIMediaItem* aMediaItem,
                         nsIDOMWindow* aWindow,
                         PRBool*       aFoundAlbumArt)
{
  // Validate arguments.
  NS_ASSERTION(aAlbumArtFetcherContractID,
               "aAlbumArtFetcherContractID is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aFoundAlbumArt, "aFoundAlbumArt is null");

  // Function variables.
  nsresult rv;

  // Set up the album art fetcher.
  nsCOMPtr<sbIAlbumArtFetcher>
    fetcher = do_CreateInstance(aAlbumArtFetcherContractID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fetcher->SetAlbumArtSourceList(mAlbumArtSourceList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Try fetching album art using the fetcher.
  rv = fetcher->FetchAlbumArtForMediaItem(aMediaItem, nsnull, aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if album art was found.
  rv = fetcher->GetFoundAlbumArt(aFoundAlbumArt);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


