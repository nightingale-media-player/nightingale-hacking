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
// Songbird local file album art fetcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbFileAlbumArtFetcher.cpp
 * \brief Songbird Local File Album Art Fetcher Source.
 */

//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbFileAlbumArtFetcher.h"

// Songbird imports.
#include <sbIAlbumArtListener.h>
#include <sbIMediaItem.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsIMutableArray.h>

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileAlbumArtFetcher, sbIAlbumArtFetcher)


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
sbFileAlbumArtFetcher::FetchAlbumArtForMediaItem
                         (sbIMediaItem*        aMediaItem,
                          sbIAlbumArtListener* aListener)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;
  nsCOMPtr<nsIFile> albumArtFile = nsnull;

  // Figure out what album we're looking for
  nsString artistName;
  nsString albumName;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                               artistName);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                               albumName);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString artistAlbum(artistName);
  artistAlbum.AppendLiteral(" - ");
  artistAlbum.Append(albumName);
  
  // Before doing anything else, check to see if we've  
  // recently cached a value for the given artist/album
  nsString cacheKey = NS_LITERAL_STRING("File:");
  cacheKey.Append(artistAlbum);
  nsCOMPtr<nsISupports> cacheData = nsnull;
  rv = mAlbumArtService->RetrieveTemporaryData(cacheKey, getter_AddRefs(cacheData));
  
  // Try to get the file from the cache data
  if (NS_SUCCEEDED(rv)) {
    albumArtFile = do_QueryInterface(cacheData, &rv);
    
    // If there is an entry in the cache, but it isn't a file, then
    // we can assume there is nothing for this fetcher to find.
    NS_ENSURE_SUCCESS(rv, rv);

  // If we get a cache miss, then look through the parent directory for art.
  } else {

    // Make sure we look for files in the form
    // albumname.ext and artist - albumname.ext
    ToLowerCase(albumName);
    ToLowerCase(artistAlbum);
    mFileBaseNameList.AppendElement(albumName);
    mFileBaseNameList.AppendElement(artistAlbum);

    // Search the media item content source directory entries for an 
    // album art file.
    rv = FindAlbumArtFile(aMediaItem,
                          getter_AddRefs(albumArtFile));
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Now either save the file in the cache, or an empty
    // string to indicate that there is nothing to be found
    if (albumArtFile) {
      rv = mAlbumArtService->CacheTemporaryData(cacheKey, albumArtFile);
    } else {
      nsCOMPtr<nsISupportsString> supportsStr;
      supportsStr = do_CreateInstance("@mozilla.org/supports-string;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mAlbumArtService->CacheTemporaryData(cacheKey, supportsStr);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Indicate if an album art file was found or not.
  if (albumArtFile) {
    // Create an album art file URI.
    nsCOMPtr<nsIURI> albumArtURI;
    rv = mIOService->NewFileURI(albumArtFile, getter_AddRefs(albumArtURI));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (aListener) {
      aListener->OnResult(albumArtURI, aMediaItem);
    }
  } else {
    // Indicate we did not find album art.
    if (aListener) {
      aListener->OnResult(nsnull, aMediaItem);
    }
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


/* \brief shut down the fetcher
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::Shutdown()
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
sbFileAlbumArtFetcher::GetShortName(nsAString& aShortName)
{
  aShortName.AssignLiteral("file");
  return NS_OK;
}


/**
 * \brief Name of AlbumArtFetcher to display to the user on things like
 *        menus.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetName(nsAString& aName)
{
  aName.AssignLiteral("file");
  return NS_OK;
}


/**
 * \brief Description of the AlbumArtFetcher to display to the user.
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetDescription(nsAString& aDescription)
{
  aDescription.AssignLiteral("file");
  return NS_OK;
}


/**
 * \brief Flag to indicate if this Fetcher fetches from local sources.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetIsLocal(PRBool* aIsLocal)
{
  NS_ENSURE_ARG_POINTER(aIsLocal);
  *aIsLocal = PR_TRUE;
  return NS_OK;
}

/**
 * \brief Flag to indicate if this Fetcher is enabled or not
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetIsEnabled(PRBool* aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetBoolPref("songbird.albumart.file.enabled",
                                          aIsEnabled);
  if (NS_FAILED(rv)) {
    *aIsEnabled = PR_FALSE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbFileAlbumArtFetcher::SetIsEnabled(PRBool aIsEnabled)
{
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetBoolPref("songbird.albumart.file.enabled",
                                   aIsEnabled);
}

/**
 * \brief Priority of this fetcher
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetPriority(PRInt32* aPriority)
{
  NS_ENSURE_ARG_POINTER(aPriority);
  NS_ENSURE_STATE(mPrefService);
  
  nsresult rv = mPrefService->GetIntPref("songbird.albumart.file.priority",
                                         aPriority);
  if (NS_FAILED(rv)) {
    // Default to appending
    *aPriority = -1;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbFileAlbumArtFetcher::SetPriority(PRInt32 aPriority)
{
  NS_ENSURE_STATE(mPrefService);
  return mPrefService->SetIntPref("songbird.albumart.file.priority",
                                  aPriority);
}

/**
 * \brief List of sources of album art (e.g., sbIMetadataHandler).
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetAlbumArtSourceList(nsIArray** aAlbumArtSourceList)
{
  NS_ENSURE_ARG_POINTER(aAlbumArtSourceList);
  NS_ADDREF(*aAlbumArtSourceList = mAlbumArtSourceList);
  return NS_OK;
}

NS_IMETHODIMP
sbFileAlbumArtFetcher::SetAlbumArtSourceList(nsIArray* aAlbumArtSourceList)
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
 * Construct a local file album art fetcher instance.
 */

sbFileAlbumArtFetcher::sbFileAlbumArtFetcher()
{
}


/**
 * Destroy a local file album art fetcher instance.
 */

sbFileAlbumArtFetcher::~sbFileAlbumArtFetcher()
{
}


/**
 * Initialize the local file album art fetcher.
 */

nsresult
sbFileAlbumArtFetcher::Initialize()
{
  nsresult rv;

  // Get the I/O service
  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference branch.
  mPrefService = do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the album art file extension list.
  nsCString fileExtensions;
  rv = mPrefService->GetCharPref("songbird.albumart.file.extensions",
                                 getter_Copies(fileExtensions));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString_Split(NS_ConvertUTF8toUTF16(fileExtensions),
                 NS_LITERAL_STRING(","),
                 mFileExtensionList);

  // Read the album art file base name list.
  nsCString fileBaseNames;
  rv = mPrefService->GetCharPref("songbird.albumart.file.base_names",
                                 getter_Copies(fileBaseNames));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString_Split(NS_ConvertUTF8toUTF16(fileBaseNames),
                 NS_LITERAL_STRING(","),
                 mFileBaseNameList);

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
 * Return in contentSrcDirEntries an enumerator of the files in the directory of the
 * content for the specified url.  If the content is not a
 * local file, return false in aIsLocalFile and don't return anything in
 * contentSrcDirEntries.  Otherwise, return true in aIsLocalFile.
 *
 * \param aURL                  Location for which to return directory
 *                              entries.
 * \param aIsLocalFile          True if media item content is a local file.
 * \param contentSrcDirEntries  Enumerator of media item content directory
 *                              entries.
 */

nsresult
sbFileAlbumArtFetcher::GetURLDirEntries
                         (nsIURL*               aURL,
                          PRBool*               aIsLocalFile,
                          nsISimpleEnumerator** contentSrcDirEntries)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aURL);
  NS_ENSURE_ARG_POINTER(aIsLocalFile);
  NS_ENSURE_ARG_POINTER(contentSrcDirEntries);

  // Function variables.
  nsresult rv;

  // Get the media item content source local file URL.  Do nothing more if not a
  // local file.
  nsCOMPtr<nsIFileURL> contentSrcFileURL = do_QueryInterface(aURL,
                                                             &rv);
  if (NS_FAILED(rv)) {
    *aIsLocalFile = PR_FALSE;
    return NS_OK;
  }

  // Get the media item content source directory.
  nsCOMPtr<nsIFile> contentSrcFile;
  rv = contentSrcFileURL->GetFile(getter_AddRefs(contentSrcFile));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> contentSrcDir;
  rv = contentSrcFile->GetParent(getter_AddRefs(contentSrcDir));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the media item content source directory entries.
  rv = contentSrcDir->GetDirectoryEntries(contentSrcDirEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aIsLocalFile = PR_TRUE;

  return NS_OK;
}


/**
 * Search for album art near the location of the given media item
 *
 * \param aMediaItem            Media item we need art for
 * \param aAlbumArtFile         Found album art file or null if not found.
 */

nsresult
sbFileAlbumArtFetcher::FindAlbumArtFile(sbIMediaItem*        aMediaItem,
                                        nsIFile**            aAlbumArtFile)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aAlbumArtFile);
  
  // Function variables.
  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> contentSrcDirEntries;
  nsCOMPtr<nsIMutableArray> entriesToBeCached = nsnull;

  // Set default result.
  *aAlbumArtFile = nsnull;

  // Get the media item content source
  nsCOMPtr<nsIURI> contentSrcURI;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentSrcURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURL> contentSrcURL = do_QueryInterface(contentSrcURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // First check to see if we have a cached list of all the images
  // in this directory (useful in the edge case of 10000 tracks 
  // from different albums, all in the same directory)

  nsCString directory;
  contentSrcURL->GetDirectory(directory);
  nsString cacheKey = NS_LITERAL_STRING("Directory:");
  cacheKey.Append(NS_ConvertUTF8toUTF16(directory));
  nsCOMPtr<nsISupports> cacheData = nsnull;
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAlbumArtService->RetrieveTemporaryData(cacheKey, getter_AddRefs(cacheData));
  
  // Try to get the entries from the cache data
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIArray> cachedEntries = do_QueryInterface(cacheData, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = cachedEntries->Enumerate(getter_AddRefs(contentSrcDirEntries));
    NS_ENSURE_SUCCESS(rv, rv);

  } else {
    
    // Get the media item content source directory entries.
    PRBool isLocalFile = PR_FALSE;
    rv = GetURLDirEntries(contentSrcURL,
                          &isLocalFile,
                          getter_AddRefs(contentSrcDirEntries));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isLocalFile) {
      return NS_OK;
    }
    
    // Create an array to cache the image entries we find
    entriesToBeCached = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  }

  // Search the media item content source directory entries for an album art
  // file.
  while (1) {
    // Get the next directory entry.
    PRBool hasMoreElements;
    rv = contentSrcDirEntries->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements)
      break;
    nsCOMPtr<nsIFile> file;
    rv = contentSrcDirEntries->GetNext(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip file if it's not a regular file (e.g., a directory).
    PRBool isFile;
    rv = file->IsFile(&isFile);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isFile)
      continue;

    // Get the file leaf name in lower case.
    nsString leafName;
    rv = file->GetLeafName(leafName);
    NS_ENSURE_SUCCESS(rv, rv);
    ToLowerCase(leafName);

    // Get the file extension.  Skip file if it has no extension.
    PRInt32 fileExtensionIndex = leafName.RFind(NS_LITERAL_STRING("."));
    if (fileExtensionIndex < 0)
      continue;

    // Check if file extension matches any album art file extension.  Skip file
    // if it doesn't.
    nsDependentSubstring fileExtension(leafName, fileExtensionIndex + 1);
    PRBool fileExtensionMatched = PR_FALSE;
    for (PRUint32 i = 0; i < mFileExtensionList.Length(); i++) {
      if (fileExtension.Equals(mFileExtensionList[i])) {
        fileExtensionMatched = PR_TRUE;
        break;
      }
    }
    if (!fileExtensionMatched)
      continue;

    // This is an image file.  If we are building
    // a new cache list, add it now
    if (entriesToBeCached) {
      entriesToBeCached->AppendElement(file, PR_FALSE);
    }

    // Check if file name matches any album art file name.  File is an
    // album art file if it does.
    nsDependentSubstring fileBaseName(leafName, 0, fileExtensionIndex);
    // Loop backwards, since album specific images should
    // take precedence over "cover.jpg" etc.
    for (PRInt32 i = (mFileBaseNameList.Length() - 1); i >= 0; i--) {
      if (fileBaseName.Equals(mFileBaseNameList[i])) {
        if (!(*aAlbumArtFile)) {
          file.forget(aAlbumArtFile);
        }
        
        // If we're using the cache we can just exit now.  
        // If we're building the cache, then we have to continue through
        // all files to make sure we've found all the images
        if (!entriesToBeCached) {
          return NS_OK;
        }
      }
    }
  }
  
  // If needed, cache the list of images in this folder 
  if (entriesToBeCached) {
    rv = mAlbumArtService->CacheTemporaryData(cacheKey, entriesToBeCached);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
