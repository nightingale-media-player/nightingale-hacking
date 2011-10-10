/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMetadataAlbumArtFetcher.cpp
 * \brief Nightingale Metadata Album Art Fetcher Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMetadataAlbumArtFetcher.h"

// Nightingale imports.
#include <sbIAlbumArtListener.h>
#include <sbIMediaItem.h>
#include <sbMemoryUtils.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsArrayUtils.h>
#include <nsIFileURL.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>


/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMetadataAlbumArtFetcher:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gMetadataAlbumArtFetcherLog = nsnull;
#define TRACE(args) PR_LOG(gMetadataAlbumArtFetcherLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataAlbumArtFetcherLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataAlbumArtFetcher, sbIAlbumArtFetcher)


//------------------------------------------------------------------------------
//
// sbIAlbumArtFetcher implementation.
//
//------------------------------------------------------------------------------

/* \brief try to fetch album art for the given list of media items
 * \param aMediaItems the list of media items that we're looking for album art
 * \param aListener the listener to inform of success or failure
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::FetchAlbumArtForAlbum(nsIArray*            aMediaItems,
                                                 sbIAlbumArtListener* aListener)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItems);
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> listEnum;
  rv = aMediaItems->Enumerate(getter_AddRefs(listEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool hasMore;
  rv = listEnum->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  // Get the metadata manager.
  nsCOMPtr<sbIMetadataManager> metadataManager =
    do_GetService("@getnightingale.com/Nightingale/MetadataManager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Stash the album art source list into temporary variable in case
  // it's changed from underneath us.
  // (attempt to fix bug 19617/20161)
  nsCOMPtr<nsIArray> sourceList = mAlbumArtSourceList;

  while (NS_SUCCEEDED(listEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> next;
    if (NS_SUCCEEDED(listEnum->GetNext(getter_AddRefs(next))) && next) {
      nsCOMPtr<sbIMediaItem> mediaItem(do_QueryInterface(next, &rv));
      if (NS_SUCCEEDED(rv) && mediaItem) {
        rv = GetImageForItem(mediaItem, sourceList,
                             metadataManager, aListener);
        if (NS_FAILED(rv) && aListener) {
          aListener->OnTrackResult(nsnull, mediaItem);
        }
      }
    } else {
      // Something bad happened that has more returned yes but we could not
      // get the next item.
      break;
    }
  }

  if (aListener) {
    aListener->OnSearchComplete(aMediaItems);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::FetchAlbumArtForTrack(sbIMediaItem*        aMediaItem,
                                                 sbIAlbumArtListener* aListener)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Throw this item into an array and call the Album version so we don't
  // duplicate code. We can do this since we always go through each item
  // anyways.
  nsCOMPtr<nsIMutableArray> items =
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = items->AppendElement(NS_ISUPPORTS_CAST(sbIMediaItem*, aMediaItem),
                            PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return FetchAlbumArtForAlbum(items, aListener);
}

/* \brief shut down the fetcher
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::Shutdown()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  mAlbumArtService = nsnull;
  return NS_OK;
}


//
// Getters/setters.
//

/**
 * \brief Short name of AlbumArtFetcher.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetShortName(nsAString& aShortName)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  aShortName.AssignLiteral("metadata");
  return NS_OK;
}


/**
 * \brief Name of AlbumArtFetcher to display to the user on things like
 *        menus.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetName(nsAString& aName)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  sbStringBundle stringBundle;
  aName.Assign(stringBundle.Get("nightingale.albumart.metadata.name"));
  return NS_OK;
}


/**
 * \brief Description of the AlbumArtFetcher to display to the user.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetDescription(nsAString& aDescription)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  sbStringBundle stringBundle;
  aDescription.Assign(stringBundle.Get("nightingale.albumart.metadata.description"));
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher fetches from local sources.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetIsLocal(PRBool* aIsLocal)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aIsLocal);
  *aIsLocal = PR_TRUE;
  return NS_OK;
}

/**
 * \brief Flag to indicate if this Fetcher is enabled or not
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetIsEnabled(PRBool* aIsEnabled)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetBoolPref("nightingale.albumart.metadata.enabled",
                                          aIsEnabled);
  if (NS_FAILED(rv)) {
    *aIsEnabled = PR_FALSE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetIsEnabled(PRBool aIsEnabled)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetBoolPref("nightingale.albumart.metadata.enabled",
                                   aIsEnabled);
}

/**
 * \brief Priority of this fetcher
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetPriority(PRInt32* aPriority)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aPriority);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetIntPref("nightingale.albumart.metadata.priority",
                                         aPriority);
  if (NS_FAILED(rv)) {
    *aPriority = 0;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetPriority(PRInt32 aPriority)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetIntPref("nightingale.albumart.metadata.priority",
                                  aPriority);
}


/**
 * \brief List of sources of album art (e.g., sbIMetadataHandler).
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetAlbumArtSourceList(nsIArray** aAlbumArtSourceList)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aAlbumArtSourceList);
  NS_ADDREF(*aAlbumArtSourceList = mAlbumArtSourceList);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetAlbumArtSourceList(nsIArray* aAlbumArtSourceList)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  mAlbumArtSourceList = aAlbumArtSourceList;
  return NS_OK;
}


/**
 * \brief Flag to indicate if this fetcher is currently fetching.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetIsFetching(PRBool* aIsFetching)
{
  NS_ENSURE_ARG_POINTER(aIsFetching);
  // This fetcher operates synchronously, so indicate that it's not fetching.
  *aIsFetching = PR_FALSE;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a metadata album art fetcher instance.
 */

sbMetadataAlbumArtFetcher::sbMetadataAlbumArtFetcher()
{
  #ifdef PR_LOGGING
    if (!gMetadataAlbumArtFetcherLog) {
      gMetadataAlbumArtFetcherLog = PR_NewLogModule("sbMetadataAlbumArtFetcher");
    }
  #endif
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


/**
 * Destroy a metadata album art fetcher instance.
 */

sbMetadataAlbumArtFetcher::~sbMetadataAlbumArtFetcher()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


/**
 * Initialize the metadata album art fetcher.
 */

nsresult
sbMetadataAlbumArtFetcher::Initialize()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv;

  // Get the album art service.
  mAlbumArtService = do_GetService(SB_ALBUMARTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference branch.
  mPrefService = do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Return in aMetadataHandler a metadata handler for the content specified by
 * aContentSrcURI.  Try getting a metadata handler from the album art source
 * list and the metadata manager.
 *
 * \param aContentSrcURI        URI of content source.
 * \param aMetadataHandler      Metadata handler for content.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No metadata handler available.
 */

nsresult
sbMetadataAlbumArtFetcher::GetMetadataHandler
                             (nsIURI*              aContentSrcURI,
                              nsIArray*            aSourceList,
                              sbIMetadataManager*  aManager,
                              sbIMetadataHandler** aMetadataHandler)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Validate arguments.
  NS_ASSERTION(aContentSrcURI, "aContentSrcURI is null");
  NS_ASSERTION(aMetadataHandler, "aMetadataHandler is null");

  // Function variables.
  nsCOMPtr<sbIMetadataHandler> metadataHandler;
  nsresult rv;

  // Try getting a metadata handler from the album art source list.
  if (aSourceList) {
    PRUint32 albumArtSourceListCount;
    rv = aSourceList->GetLength(&albumArtSourceListCount);
    NS_ENSURE_SUCCESS(rv, rv);
    for (PRUint32 i = 0; i < albumArtSourceListCount; i++) {
      metadataHandler = do_QueryElementAt(aSourceList, i, &rv);
      if (NS_SUCCEEDED(rv))
        break;
    }
  }

  // Try getting a metadata handler from the metadata manager.
  if (!metadataHandler) {
    nsCAutoString contentSrcURISpec;
    rv = aContentSrcURI->GetSpec(contentSrcURISpec);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(aManager, NS_ERROR_NOT_INITIALIZED);
    rv = aManager->GetHandlerForMediaURL
                             (NS_ConvertUTF8toUTF16(contentSrcURISpec),
                              getter_AddRefs(metadataHandler));
    if (NS_FAILED(rv))
      metadataHandler = nsnull;
  }

  // Check if a metadata handler was found.
  if (!metadataHandler)
    return NS_ERROR_NOT_AVAILABLE;

  // Return results.
  metadataHandler.forget(aMetadataHandler);

  return NS_OK;
}

nsresult
sbMetadataAlbumArtFetcher::GetImageForItem(sbIMediaItem*        aMediaItem,
                                           nsIArray*            aSourceList,
                                           sbIMetadataManager*  aManager,
                                           sbIAlbumArtListener* aListener)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Do nothing if media item content is not a local file.
  nsCOMPtr<nsIURI> contentSrcURI;
  nsCOMPtr<nsIFileURL> contentSrcFileURL;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentSrcURI));
  NS_ENSURE_SUCCESS(rv, rv);
  contentSrcFileURL = do_QueryInterface(contentSrcURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the metadata handler for the media item content.  Do nothing more if
  // none available.
  nsCOMPtr<sbIMetadataHandler> metadataHandler;
  rv = GetMetadataHandler(contentSrcURI, aSourceList, aManager,
                          getter_AddRefs(metadataHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  // Try reading the front cover metadata image.
  nsCAutoString mimeType;
  PRUint32      dataLength = 0;
  PRUint8*      data = nsnull;
  rv = metadataHandler->GetImageData
                          (sbIMetadataHandler::METADATA_IMAGE_TYPE_FRONTCOVER,
                           mimeType,
                           &dataLength,
                           &data);
  if (NS_FAILED(rv))
    dataLength = 0;

  // If album art not found, try reading from other metadata image.
  if (dataLength == 0) {
    rv = metadataHandler->GetImageData
                            (sbIMetadataHandler::METADATA_IMAGE_TYPE_OTHER,
                             mimeType,
                             &dataLength,
                             &data);
    if (NS_FAILED(rv))
      dataLength = 0;
  }

  // If no album art found, do nothing more.
  if (dataLength == 0) {
    return NS_ERROR_FAILURE;
  }

  // Set up album art data for auto-disposal.
  sbAutoNSMemPtr autoData(data);

  // Cache album art image.
  nsCOMPtr<nsIURI> cacheURI;
  NS_ENSURE_TRUE(mAlbumArtService, NS_ERROR_NOT_INITIALIZED);
  rv = mAlbumArtService->CacheImage(mimeType,
                                    data,
                                    dataLength,
                                    getter_AddRefs(cacheURI));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aListener) {
    // Notify caller we found an image for this item
    aListener->OnTrackResult(cacheURI, aMediaItem);
  }

  return NS_OK;
}

