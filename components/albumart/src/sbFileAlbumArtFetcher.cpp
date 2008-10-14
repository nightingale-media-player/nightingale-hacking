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
#include <sbIMediaItem.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsIPrefBranch.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <unicharutil/nsUnicharUtils.h>


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
 * \param aWindow the window this was called from, can be null
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::FetchAlbumArtForMediaItem
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

  // Get the media item content source directory entries.
  nsCOMPtr<nsISimpleEnumerator> contentSrcDirEntries;
  PRBool                        isLocalFile;
  rv = GetMediaItemDirEntries(aMediaItem,
                              &isLocalFile,
                              getter_AddRefs(contentSrcDirEntries));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isLocalFile) {
    mIsComplete = PR_TRUE;
    return NS_OK;
  }

  // Get the media item album name.
  nsString albumName;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                               albumName);
  NS_ENSURE_SUCCESS(rv, rv);
  ToLowerCase(albumName);

  // Search the media item content source directory entries for an album art
  // file.
  nsCOMPtr<nsIFile> albumArtFile;
  rv = FindAlbumArtFile(contentSrcDirEntries,
                        albumName,
                        getter_AddRefs(albumArtFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // If an album art file was found, set it as the media item album art.
  if (albumArtFile) {
    rv = SetMediaItemAlbumArt(aMediaItem, albumArtFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update fetcher state.
  if (albumArtFile)
    mFoundAlbumArt = PR_TRUE;
  mIsComplete = PR_TRUE;

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
 * \brief Flag to indicate if this Fetcher can be used as a fetcher from a
 *        user menu.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetUserFetcher(PRBool* aUserFetcher)
{
  NS_ENSURE_ARG_POINTER(aUserFetcher);
  *aUserFetcher = PR_TRUE;
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
 *XXXeps stub for now.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetIsEnabled(PRBool* aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbFileAlbumArtFetcher::SetIsEnabled(PRBool aIsEnabled)
{
  return NS_OK;
}


/**
 * \brief Flag to indicate if fetching is complete.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetIsComplete(PRBool* aIsComplete)
{
  NS_ENSURE_ARG_POINTER(aIsComplete);
  *aIsComplete = mIsComplete;
  return NS_OK;
}


/**
 * \brief Flag to indicate whether album art was found.
 */

NS_IMETHODIMP
sbFileAlbumArtFetcher::GetFoundAlbumArt(PRBool* aFoundAlbumArt)
{
  NS_ENSURE_ARG_POINTER(aFoundAlbumArt);
  *aFoundAlbumArt = mFoundAlbumArt;
  return NS_OK;
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

sbFileAlbumArtFetcher::sbFileAlbumArtFetcher() :
  mIsComplete(PR_FALSE),
  mFoundAlbumArt(PR_FALSE)
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

  // Get the I/O service, proxied to the main thread.
  mIOService = do_ProxiedGetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference branch.
  nsCOMPtr<nsIPrefBranch>
    prefService = do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the album art file extension list.
  nsCString fileExtensions;
  rv = prefService->GetCharPref("songbird.albumart.file.extensions",
                                getter_Copies(fileExtensions));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString_Split(NS_ConvertUTF8toUTF16(fileExtensions),
                 NS_LITERAL_STRING(","),
                 mFileExtensionList);

  // Read the album art file base name list.
  nsCString fileBaseNames;
  rv = prefService->GetCharPref("songbird.albumart.file.base_names",
                                getter_Copies(fileBaseNames));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString_Split(NS_ConvertUTF8toUTF16(fileBaseNames),
                 NS_LITERAL_STRING(","),
                 mFileBaseNameList);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Return in aDirEntries an enumerator of the files in the directory of the
 * content for the media item specified by aMediaItem.  If the content is not a
 * local file, return false in aIsLocalFile and don't return anything in
 * aDirEntries.  Otherwise, return true in aIsLocalFile.
 *
 * \param aMediaItem            Media item for which to return directory
 *                              entries.
 * \param aIsLocalFile          True if media item content is a local file.
 * \param aDirEntries           Enumerator of media item content directory
 *                              entries.
 */

nsresult
sbFileAlbumArtFetcher::GetMediaItemDirEntries
                         (sbIMediaItem*         aMediaItem,
                          PRBool*               aIsLocalFile,
                          nsISimpleEnumerator** aDirEntries)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aIsLocalFile, "aIsLocalFile is null");
  NS_ASSERTION(aDirEntries, "aDirEntries is null");

  // Function variables.
  nsresult rv;

  // Get the media item content source URI.
  nsCOMPtr<nsIURI> contentSrcURI;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentSrcURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the media item content source local file URL.  Do nothing more if not a
  // local file.
  nsCOMPtr<nsIFileURL> contentSrcFileURL = do_QueryInterface(contentSrcURI,
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
  rv = contentSrcDir->GetDirectoryEntries(aDirEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aIsLocalFile = PR_TRUE;

  return NS_OK;
}


/**
 * Search the files specified by aDirEntries for an album art file for the album
 * specified by aAlbumName.  If a file is found, return it in aAlbumArtFile;
 * otherwise, return null in aAlbumArtFile.
 *
 * \param aDirEntries           Diretory entries in which to search.
 * \param aAlbumName            Name of album for which to search for art.
 * \param aAlbumArtFile         Found album art file or null if not found.
 */

nsresult
sbFileAlbumArtFetcher::FindAlbumArtFile(nsISimpleEnumerator* aDirEntries,
                                        nsAString&           aAlbumName,
                                        nsIFile**            aAlbumArtFile)
{
  // Validate arguments.
  NS_ASSERTION(aDirEntries, "aDirEntries is null");
  NS_ASSERTION(aAlbumArtFile, "aAlbumArtFile is null");

  // Function variables.
  nsresult rv;

  // Set default result.
  *aAlbumArtFile = nsnull;

  // Search the media item content source directory entries for an album art
  // file.
  while (1) {
    // Get the next directory entry.
    PRBool hasMoreElements;
    rv = aDirEntries->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements)
      break;
    nsCOMPtr<nsIFile> file;
    rv = aDirEntries->GetNext(getter_AddRefs(file));
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

    // Check if file base name matches any album art file base name.  File is an
    // album art file if it does.
    nsDependentSubstring fileBaseName(leafName, 0, fileExtensionIndex);
    for (PRUint32 i = 0; i < mFileBaseNameList.Length(); i++) {
      if (fileBaseName.Equals(mFileBaseNameList[i])) {
        file.forget(aAlbumArtFile);
        return NS_OK;
      }
    }

    // Check if file base name matches album name.  File is an album art file if
    // it does.
    if (fileBaseName.Equals(aAlbumName)) {
      file.forget(aAlbumArtFile);
      return NS_OK;
    }
  }

  return NS_OK;
}


/**
 * Set the album art for the media item specified by aMediaItem to the file
 * specified by aAlbumArtFile.
 *
 * \param aMediaItem            Media item for which to set album art.
 * \param aAlbumArtFile         Album art file.
 */

nsresult
sbFileAlbumArtFetcher::SetMediaItemAlbumArt(sbIMediaItem* aMediaItem,
                                            nsIFile*      aAlbumArtFile)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aAlbumArtFile, "aAlbumArtFile is null");

  // Function variables.
  nsresult rv;

  // Create an album art file URI.
  nsCOMPtr<nsIURI> albumArtURI;
  rv = mIOService->NewFileURI(aAlbumArtFile, getter_AddRefs(albumArtURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the album art file URI spec.
  nsCString albumArtURISpec;
  rv = albumArtURI->GetSpec(albumArtURISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the media item primary image URL property.
  rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                               NS_ConvertUTF8toUTF16(albumArtURISpec));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

