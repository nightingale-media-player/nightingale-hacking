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
// Songbird album art service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtService.cpp
 * \brief Songbird Album Art Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird album art service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbAlbumArtService.h"

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIBinaryOutputStream.h>
#include <nsICryptoHash.h>
#include <nsIFileURL.h>
#include <nsIProperties.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird album art service configuration.
//
//------------------------------------------------------------------------------

//
// sbAlbumArtServiceValidExtensionList  List of valid album art file extensions.
//

static const char* sbAlbumArtServiceValidExtensionList[] =
{
  "jpg",
  "jpeg",
  "gif",
  "png"
};


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbAlbumArtService, sbIAlbumArtService)


//------------------------------------------------------------------------------
//
// sbIAlbumArtFetcher implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Write the album art image specified by aData and aDataLen of type
 *        specified by aMimeType to a cache file and return the cache file
 *        URL.
 *
 * \param aMimeType           MIME type of image data.
 * \param aData               Album art image data.
 * \param aDataLen            Length in bytes of image data.
 *
 * \return                    Album art image cache file URL.
 */

#define NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT -1

NS_IMETHODIMP
sbAlbumArtService::CacheImage(const nsACString& aMimeType,
                              const PRUint8*    aData,
                              PRUint32          aDataLen,
                              nsIFileURL**      _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the image cache file base name.
  nsAutoString fileBaseName;
  rv = GetCacheFileBaseName(aData, aDataLen, fileBaseName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the image cache file extension.
  nsAutoString fileExtension;
  rv = GetCacheFileExtension(aMimeType, fileExtension);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file object within the album art cache directory.
  nsAutoString fileName;
  fileName.Assign(fileBaseName);
  fileName.Append(NS_LITERAL_STRING("."));
  fileName.Append(fileExtension);
  nsCOMPtr<nsIFile> cacheFile;
  rv = mAlbumArtCacheDir->Clone(getter_AddRefs(cacheFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cacheFile->Append(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file nsIFileURL object.
  nsCOMPtr<nsIURI> cacheFileURI;
  rv = mIOService->NewFileURI(cacheFile, getter_AddRefs(cacheFileURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFileURL> cacheFileURL = do_QueryInterface(cacheFileURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If cache file already exists, return it.
  PRBool exists;
  rv = cacheFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (exists) {
    cacheFileURL.forget(_retval);
    return NS_OK;
  }

  // Open a file output stream to the cache file and set to auto-close.
  nsCOMPtr<nsIFileOutputStream> fileOutputStream =
    do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileOutputStream->Init(cacheFile,
                              NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                              NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                              0);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoFileOutputStream autoFileOutputStream(fileOutputStream);

  // Write the cache file.
  nsCOMPtr<nsIBinaryOutputStream> binaryOutputStream =
    do_CreateInstance("@mozilla.org/binaryoutputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = binaryOutputStream->SetOutputStream(fileOutputStream);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = binaryOutputStream->WriteByteArray((PRUint8*) aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  cacheFileURL.forget(_retval);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an album art service instance.
 */

sbAlbumArtService::sbAlbumArtService()
{
}


/**
 * Destroy an album art service instance.
 */

sbAlbumArtService::~sbAlbumArtService()
{
}


/**
 * Initialize the album art service.
 */

nsresult
sbAlbumArtService::Initialize()
{
  nsresult rv;

  // Get the I/O service.
  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the MIME service.
  mMIMEService = do_GetService("@mozilla.org/mime;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the album art cache directory.
  nsCOMPtr<nsIProperties>
    directoryService = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = directoryService->Get("ProfLD",
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(mAlbumArtCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAlbumArtCacheDir->Append(NS_LITERAL_STRING("artwork"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the valid extension list.
  for (PRUint32 i = 0;
       i < NS_ARRAY_LENGTH(sbAlbumArtServiceValidExtensionList);
       i++) {
    mValidExtensionList.AppendElement(sbAlbumArtServiceValidExtensionList[i]);
  }

  // Create the album art cache directory if it doesn't exist.
  PRBool exists;
  rv = mAlbumArtCacheDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    nsCOMPtr<nsIFile> parent;
    rv = mAlbumArtCacheDir->GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 permissions;
    rv = parent->GetPermissions(&permissions);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mAlbumArtCacheDir->Create(nsIFile::DIRECTORY_TYPE, permissions);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Get the album art cache file base name for the image data specified by aData
 * and aDataLen and return it in aFileBaseName.
 *
 * \param aData               Album art image data.
 * \param aDataLen            Length in bytes of image data.
 * \param aFileBaseName       Returned image cache file base name.
 */

nsresult
sbAlbumArtService::GetCacheFileBaseName(const PRUint8* aData,
                                        PRUint32       aDataLen,
                                        nsAString&     aFileBaseName)
{
  // Validate arguments.
  NS_ASSERTION(aData, "aData is null");

  // Function variables.
  nsresult rv;

  // Clear file base name.
  aFileBaseName.Truncate();

  // Generate a hash of the image data.
  nsCAutoString hashValue;
  nsCOMPtr<nsICryptoHash>
    cryptoHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Update(aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Finish(PR_FALSE, hashValue);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file base name.
  PRUint32 hashLength = hashValue.Length();
  PRUint8* hashData = (PRUint8*) hashValue.get();
  for (PRUint32 i = 0; i < hashLength; i++) {
    char hexValue[3];
    PR_snprintf(hexValue, 3, "%02x", hashData[i]);
    aFileBaseName.AppendLiteral(hexValue);
  }

  return NS_OK;
}


/**
 * Get the album art cache file extension for the image with the MIME type
 * specified by aMimeType.
 *
 * \param aMimeType           MIME type of image data.
 * \param aFileExtension      Returned image cache file extension.
 */

nsresult
sbAlbumArtService::GetCacheFileExtension(const nsACString& aMimeType,
                                         nsAString&        aFileExtension)
{
  nsCAutoString fileExtension;
  nsresult      rv;

  // Look up the file extension from the MIME type.
  rv = mMIMEService->GetPrimaryExtension(aMimeType,
                                         NS_LITERAL_CSTRING(""),
                                         fileExtension);
  if (NS_FAILED(rv))
    fileExtension.Truncate();

  // Extract the file extension from the MIME type.
  if (fileExtension.IsEmpty()) {
    PRInt32 mimeSubTypeIndex = aMimeType.RFind("/");
    if (mimeSubTypeIndex >= 0) {
      fileExtension.Assign(nsDependentCSubstring(aMimeType,
                                                 mimeSubTypeIndex + 1));
    } else {
      fileExtension.Assign(aMimeType);
    }
  }

  // Convert file extension to lower-case.
  ToLowerCase(fileExtension);

  // Validate the extension.
  NS_ENSURE_TRUE(mValidExtensionList.Contains(fileExtension),
                 NS_ERROR_FAILURE);

  // Return results.
  aFileExtension.AssignLiteral(fileExtension.get());

  return NS_OK;
}

