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
// Songbird metadata album art fetcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMetadataAlbumArtFetcher.cpp
 * \brief Songbird Metadata Album Art Fetcher Source.
 */

//------------------------------------------------------------------------------
//
// Songbird metadata album art fetcher imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMetadataAlbumArtFetcher.h"

// Songbird imports.
#include <sbIAlbumArtListener.h>
#include <sbIMediaItem.h>
#include <sbMemoryUtils.h>
#include <sbStandardProperties.h>

// Mozilla imports.
#include <nsArrayUtils.h>
#include <nsIFileURL.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>


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

  while (NS_SUCCEEDED(listEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> next;
    if (NS_SUCCEEDED(listEnum->GetNext(getter_AddRefs(next))) && next) {
      nsCOMPtr<sbIMediaItem> mediaItem(do_QueryInterface(next, &rv));
      if (NS_SUCCEEDED(rv) && mediaItem) {
        rv = GetImageForItem(mediaItem, aListener);
        if (NS_FAILED(rv) && aListener) {
          aListener->OnResult(nsnull, mediaItem);
        }
      }
    }
  }

  if (aListener) {
    aListener->OnAlbumComplete(aMediaItems);
  }
  return NS_OK;
}

/* \brief shut down the fetcher
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::Shutdown()
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
sbMetadataAlbumArtFetcher::GetShortName(nsAString& aShortName)
{
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
  aName.AssignLiteral("metadata");
  return NS_OK;
}


/**
 * \brief Description of the AlbumArtFetcher to display to the user.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetDescription(nsAString& aDescription)
{
  aDescription.AssignLiteral("metadata");
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher fetches from local sources.
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetIsLocal(PRBool* aIsLocal)
{
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
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetBoolPref("songbird.albumart.metadata.enabled",
                                          aIsEnabled);
  if (NS_FAILED(rv)) {
    *aIsEnabled = PR_FALSE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetIsEnabled(PRBool aIsEnabled)
{
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetBoolPref("songbird.albumart.metadata.enabled",
                                   aIsEnabled);
}

/**
 * \brief Priority of this fetcher
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetPriority(PRInt32* aPriority)
{
  NS_ENSURE_ARG_POINTER(aPriority);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetIntPref("songbird.albumart.metadata.priority",
                                         aPriority);
  if (NS_FAILED(rv)) {
    *aPriority = 0;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetPriority(PRInt32 aPriority)
{
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetIntPref("songbird.albumart.metadata.priority",
                                  aPriority);
}


/**
 * \brief List of sources of album art (e.g., sbIMetadataHandler).
 */

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::GetAlbumArtSourceList(nsIArray** aAlbumArtSourceList)
{
  NS_ENSURE_ARG_POINTER(aAlbumArtSourceList);
  NS_ADDREF(*aAlbumArtSourceList = mAlbumArtSourceList);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataAlbumArtFetcher::SetAlbumArtSourceList(nsIArray* aAlbumArtSourceList)
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
 * Construct a metadata album art fetcher instance.
 */

sbMetadataAlbumArtFetcher::sbMetadataAlbumArtFetcher()
{
}


/**
 * Destroy a metadata album art fetcher instance.
 */

sbMetadataAlbumArtFetcher::~sbMetadataAlbumArtFetcher()
{
}


/**
 * Initialize the metadata album art fetcher.
 */

nsresult
sbMetadataAlbumArtFetcher::Initialize()
{
  nsresult rv;

  // Get the album art service.
  mAlbumArtService = do_GetService(SB_ALBUMARTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the metadata manager.
  mMetadataManager =
    do_GetService("@songbirdnest.com/Songbird/MetadataManager;1", &rv);
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
                              sbIMetadataHandler** aMetadataHandler)
{
  // Validate arguments.
  NS_ASSERTION(aContentSrcURI, "aContentSrcURI is null");
  NS_ASSERTION(aMetadataHandler, "aMetadataHandler is null");

  // Function variables.
  nsCOMPtr<sbIMetadataHandler> metadataHandler;
  nsresult rv;

  // Try getting a metadata handler from the album art source list.
  if (mAlbumArtSourceList) {
    PRUint32 albumArtSourceListCount;
    rv = mAlbumArtSourceList->GetLength(&albumArtSourceListCount);
    NS_ENSURE_SUCCESS(rv, rv);
    for (PRUint32 i = 0; i < albumArtSourceListCount; i++) {
      metadataHandler = do_QueryElementAt(mAlbumArtSourceList, i, &rv);
      if (NS_SUCCEEDED(rv))
        break;
    }
  }

  // Try getting a metadata handler from the metadata manager.
  if (!metadataHandler) {
    nsCAutoString contentSrcURISpec;
    rv = aContentSrcURI->GetSpec(contentSrcURISpec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMetadataManager->GetHandlerForMediaURL
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
                                           sbIAlbumArtListener* aListener)
{
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
  rv = GetMetadataHandler(contentSrcURI, getter_AddRefs(metadataHandler));
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
  nsCOMPtr<nsIFileURL> cacheFileURL;
  rv = mAlbumArtService->CacheImage(mimeType,
                                    data,
                                    dataLength,
                                    getter_AddRefs(cacheFileURL));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aListener) {
    // Notify caller we found an image for this item
    aListener->OnResult(cacheFileURL, aMediaItem);
  }

  return NS_OK;
}

