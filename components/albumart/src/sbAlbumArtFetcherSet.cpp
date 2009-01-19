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
#include <nsIFile.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbAlbumArtFetcherSet:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gAlbumArtFetcherSetLog = nsnull;
#define TRACE(args) PR_LOG(gAlbumArtFetcherSetLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gAlbumArtFetcherSetLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(sbAlbumArtFetcherSet,
                              sbIAlbumArtFetcherSet,
                              sbIAlbumArtFetcher,
                              sbIAlbumArtListener)


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
  TRACE(("sbAlbumArtFetcherSet::GetLocalOnly - IsLocalOnly = %s",
          (mLocalOnly ? "TRUE" : "FALSE")));
  NS_ENSURE_ARG_POINTER(aLocalOnly);
  *aLocalOnly = mLocalOnly;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetLocalOnly(PRBool aLocalOnly)
{
  TRACE(("sbAlbumArtFetcherSet::SetLocalOnly - IsLocalOnly = %s",
          (aLocalOnly ? "TRUE" : "FALSE")));
  
  nsresult rv;
  
  // If this has changed we need to reload the fetcher list
  if (aLocalOnly != mLocalOnly) {
    mLocalOnly = aLocalOnly;
    TRACE(("sbAlbumArtFetcherSet::SetLocalOnly - Reloading fetcher list."));
    rv = mAlbumArtService->GetFetcherList(mLocalOnly,
                                          getter_AddRefs(mFetcherList));
    NS_ENSURE_SUCCESS(rv, rv);
  }

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
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::FetchAlbumArtForMediaItem
                         (sbIMediaItem*        aMediaItem,
                          sbIAlbumArtListener* aListener)
{
  TRACE(("sbAlbumArtFetcherSet::FetchAlbumArtForMediaItem"));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtFetcherSet::FetchAlbumArtForMediaItem is main thread only!");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  PRUint32 fetcherListCount;
  rv = mFetcherList->GetLength(&fetcherListCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (fetcherListCount <= 0) {
    // No fetchers so abort
    aListener->OnResult(nsnull, aMediaItem);
    return NS_OK;
  }

  // Save the listener
  mListener = aListener;

  // Start with the first fetcher
  mFetcherIndex = 0;
  mShutdown = PR_FALSE;
  rv = NextFetcher(aMediaItem);
  if (NS_FAILED(rv)) {
    FinishFetch(nsnull, aMediaItem);
  }

  return NS_OK;
}


/* \brief shut down the fetcher
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::Shutdown()
{
  TRACE(("sbAlbumArtFetcherSet::Shutdown"));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtFetcherSet::Shutdown is main thread only!");
  if (mFetcher) {
    // Shutdown the current fetcher
    mFetcher->Shutdown(); 
    mFetcher = nsnull;
  }
  mShutdown = PR_TRUE;
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
  TRACE(("sbAlbumArtFetcherSet::GetShortName"));
  aShortName.AssignLiteral("set");
  return NS_OK;
}


/**
 * \brief Name of AlbumArtFetcher to display to the user on things like
 *        menus.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetName(nsAString& aName)
{
  TRACE(("sbAlbumArtFetcherSet::GetName"));
  aName.AssignLiteral("set");
  return NS_OK;
}


/**
 * \brief Description of the AlbumArtFetcher to display to the user.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetDescription(nsAString& aDescription)
{
  TRACE(("sbAlbumArtFetcherSet::GetDescription"));
  aDescription.AssignLiteral("set");
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
  TRACE(("sbAlbumArtFetcherSet::GetIsLocal"));
  NS_ENSURE_ARG_POINTER(aIsLocal);
  *aIsLocal = mLocalOnly;
  return NS_OK;
}

/**
 * \brief Flag to indicate if this Fetcher is enabled or not.
 *        The fetcher set is always enabled.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetIsEnabled(PRBool* aIsEnabled)
{
  TRACE(("sbAlbumArtFetcherSet::GetIsEnabled"));
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetIsEnabled(PRBool aIsEnabled)
{
  TRACE(("sbAlbumArtFetcherSet::SetIsEnabled"));
  return NS_OK;
}

/**
 * \brief Priority of this fetcher, this priority doesn't really matter since
 *        we are not an actual fetcher.
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetPriority(PRInt32* aPriority)
{
  TRACE(("sbAlbumArtFetcherSet::GetPriority"));
  NS_ENSURE_ARG_POINTER(aPriority);
  *aPriority = -1;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetPriority(PRInt32 aPriority)
{
  TRACE(("sbAlbumArtFetcherSet::SetPriority"));
  return NS_OK;
}

/**
 * \brief List of sources of album art (e.g., sbIMetadataHandler).
 */

NS_IMETHODIMP
sbAlbumArtFetcherSet::GetAlbumArtSourceList(nsIArray** aAlbumArtSourceList)
{
  TRACE(("sbAlbumArtFetcherSet::GetFoundAlbumArt"));
  NS_ENSURE_ARG_POINTER(aAlbumArtSourceList);
  NS_ADDREF(*aAlbumArtSourceList = mAlbumArtSourceList);
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtFetcherSet::SetAlbumArtSourceList(nsIArray* aAlbumArtSourceList)
{
  TRACE(("sbAlbumArtFetcherSet::SetAlbumArtSourceList"));
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
  mLocalOnly(PR_FALSE),
  mShutdown(PR_FALSE),
  mListener(nsnull),
  mFetcherList(nsnull),
  mFetcherIndex(-1),
  mFetcher(nsnull)
{
#ifdef PR_LOGGING
  if (!gAlbumArtFetcherSetLog) {
    gAlbumArtFetcherSetLog = PR_NewLogModule("sbAlbumArtFetcherSet");
  }
#endif
  TRACE(("sbAlbumArtFetcherSet[0x%.8x] - ctor", this));
  MOZ_COUNT_CTOR(sbAlbumArtFetcherSet);
}


/**
 * Destroy an album art fetcher set instance.
 */

sbAlbumArtFetcherSet::~sbAlbumArtFetcherSet()
{
  TRACE(("sbAlbumArtFetcherSet[0x%.8x] - dtor", this));
  MOZ_COUNT_DTOR(sbAlbumArtFetcherSet);

  // Make sure to shutdown
  Shutdown();
}


/**
 * Initialize the album art fetcher set.
 */

nsresult
sbAlbumArtFetcherSet::Initialize()
{
  TRACE(("sbAlbumArtFetcherSet::Intialize"));
  nsresult rv;

  // Get the album art service.
  mAlbumArtService = do_GetService(SB_ALBUMARTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of fetchers.
  rv = mAlbumArtService->GetFetcherList(mLocalOnly,
                                        getter_AddRefs(mFetcherList));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Get the console service for warning messages
  mConsoleService = do_GetService("@mozilla.org/consoleservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIAlbumArtListner Implementation.
//
//------------------------------------------------------------------------------

/* onChangeFetcher(in sbIAlbumArtFetcher aFetcher); */
NS_IMETHODIMP sbAlbumArtFetcherSet::OnChangeFetcher(sbIAlbumArtFetcher* aFetcher)
{
  TRACE(("sbAlbumArtFetcherSet::OnChangeFetcher"));
  return NS_OK;
}

/* onResult(in nsIURI aImageLocation, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP sbAlbumArtFetcherSet::OnResult(nsIURI* aImageLocation,
                                             sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtFetcherSet::OnResult"));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  if (aImageLocation) {
    // Success so we can now notify the caller and stop processing
    TRACE(("sbAlbumArtFetcherSet::OnResult - Found Image"));
    FinishFetch(aImageLocation, aMediaItem); 
  } else {
    // No image found so try the next fetcher
    mFetcherIndex++;
    rv = NextFetcher(aMediaItem);
    if (NS_FAILED(rv)) {
      FinishFetch(nsnull, aMediaItem);
    }
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

nsresult
sbAlbumArtFetcherSet::FinishFetch(nsIURI* aImageLocation,
                                  sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtFetcherSet::FinishFetch"));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Reset our counter
  mFetcherIndex = -1;

  // Make sure we shutdown the current fetcher
  if (mFetcher) {
    rv = mFetcher->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
    mFetcher = nsnull;
  }

  // Check if this is a local file and warn if not
  if (aImageLocation) {
    nsCOMPtr<nsIFile> fileLocation;
    nsIID nsIFileIID = NS_IFILE_IID;
    rv = aImageLocation->QueryInterface(nsIFileIID, getter_AddRefs(fileLocation));
    if (NS_FAILED(rv) || !fileLocation) {
      // Not a file so this must be online
      mConsoleService->LogStringMessage(
        NS_LITERAL_STRING("Fetcher returned non-local file for image.").get());
    }
  }

  // Notify the listener
  mListener->OnResult(aImageLocation, aMediaItem);

  return NS_OK;
}

nsresult
sbAlbumArtFetcherSet::NextFetcher(sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtFetcherSet::NextFetcher"));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtFetcherSet::NextFetcher is main thread only!");
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Shutdown the existing fetcher
  if (mFetcher) {
    mFetcher->Shutdown();
    mFetcher = nsnull;
  }
  
  // Check if we have been shutdown
  if (mShutdown) {
    return NS_OK;
  }
  
  // Try the next fetcher
  PRUint32 fetcherListCount;
  rv = mFetcherList->GetLength(&fetcherListCount);
  if (NS_FAILED(rv)) {
    TRACE(("sbAlbumArtFetcherSet::NextFetcher - Unable to get count of fetchers"));
    FinishFetch(nsnull, aMediaItem);
    return NS_OK;
  }

  if (mFetcherIndex >= fetcherListCount) {
    TRACE(("sbAlbumArtFetcherSet::NextFetcher - No more fetchers"));
    FinishFetch(nsnull, aMediaItem);
    return NS_OK;
  }

  TRACE(("sbAlbumArtFetcherSet::NextFetcher - Querying fetcher at index %d",
          mFetcherIndex));

  // Get the next fetcher
  nsCAutoString fetcherContractID;
  nsCOMPtr<nsIVariant>
    fetcherContractIDVariant = do_QueryElementAt(mFetcherList,
                                                 mFetcherIndex,
                                                 &rv);
  if (NS_FAILED(rv)) {
    OnResult(nsnull, aMediaItem);
    return NS_OK;
  }

  rv = fetcherContractIDVariant->GetAsACString(fetcherContractID);
  if (NS_FAILED(rv)) {
    OnResult(nsnull, aMediaItem);
    return NS_OK;
  }

  TRACE(("sbAlbumArtFetcherSet::NextFetcher - Trying fetcher %s",
         fetcherContractID.get()));

  // Notify listeners that we have moved on to the next fetcher
  mFetcher = do_CreateInstance(fetcherContractID.get(), &rv);
  if (NS_FAILED(rv)) {
    OnResult(nsnull, aMediaItem);
    return NS_OK;
  }
  mListener->OnChangeFetcher(mFetcher);

  // Set the source list to what we have.
  rv = mFetcher->SetAlbumArtSourceList(mAlbumArtSourceList);
  if (NS_FAILED(rv)) {
    OnResult(nsnull, aMediaItem);
    return NS_OK;
  }

  // Try fetching album art using the fetcher.
  rv = mFetcher->FetchAlbumArtForMediaItem(aMediaItem, this);
  if (NS_FAILED(rv)) {
    OnResult(nsnull, aMediaItem);
    return NS_OK;
  }

  return NS_OK;
}

