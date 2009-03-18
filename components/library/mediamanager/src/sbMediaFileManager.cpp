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

#include "sbMediaFileManager.h"
#include <sbStandardProperties.h>
#include <sbIMediaItem.h>
#include <nsIFile.h>
#include <nsNetUtil.h>
#include <nsIFileURL.h>
#include <nsCRT.h>

NS_IMPL_ISUPPORTS1(sbMediaFileManager, sbIMediaFileManager);

sbMediaFileManager::sbMediaFileManager() {
}

sbMediaFileManager::~sbMediaFileManager() {
}

NS_IMETHODIMP sbMediaFileManager::Init() {

  nsresult rv;
  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
  * Organize an item in the media library folder
  */
NS_IMETHODIMP
sbMediaFileManager::OrganizeItem(sbIMediaItem *aMediaItem, 
                                 unsigned short aManageType,
                                 PRBool *aRetVal) {
  if (aManageType == 0) {
    // No action was requested, fail altogether
    return NS_ERROR_INVALID_ARG;
  }
  
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);
  
  nsresult rv;

  // First make sure that we're operating on a file
  
  // Get to the old file's path
  nsCOMPtr<nsIURI> itemUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(itemUri));
  NS_ENSURE_SUCCESS (rv, rv);
  
  PRBool isFile;
  rv = itemUri->SchemeIs("file", &isFile);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!isFile) {
    return NS_ERROR_INVALID_ARG;
  }
  
  if (aManageType & sbIMediaFileManager::MANAGE_DELETE) {
    rv = Delete(aMediaItem, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // MANAGE_DELETE does not make sense to use in conjunction with other flags,
    // so if anymore action was requested, return false in aRetVal, but don't
    // cause an exception.

    if (aManageType != sbIMediaFileManager::MANAGE_DELETE)
      *aRetVal = PR_FALSE;

    return NS_OK;
  }

  nsString filename;
  nsString path;
  
  if (aManageType & sbIMediaFileManager::MANAGE_COPY) {
    rv = GetNewPath(aMediaItem, itemUri, path, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // If there was an error trying to get the new path, abort now, but don't
    // cause an exception
    if (!*aRetVal) {
      return NS_OK;
    }
  } else {
    // leave path empty, we'll send NULL as destination directory to MoveTo.
  }

  if (aManageType & sbIMediaFileManager::MANAGE_RENAME) {
    rv = GetNewFilename(aMediaItem, itemUri, filename, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // If there was an error trying to construct the new filename, abort now,
    // but don't cause an exception
    if (!*aRetVal) {
      return NS_OK;
    }
  }
  
  rv = CopyRename(aMediaItem, itemUri, filename, path, aRetVal);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

// Construct the new filename based on user settings
nsresult 
sbMediaFileManager::GetNewFilename(sbIMediaItem *aMediaItem, 
                                   nsIURI *aItemUri,
                                   nsString &aFilename, 
                                   PRBool *aRetVal) {

  nsresult rv;
  
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;
  
  nsString separator;
  // TODO: get separator from preferences
  separator = NS_LITERAL_STRING("_");

  // Start clean
  aFilename.Truncate();
    
  // TODO: read from preferences instead of static array
  nsString renameConfig[2] = {
    NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
    NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME)
  };
  for (PRInt32 i=0; i< NS_ARRAY_LENGTH(renameConfig); i++) {
    // get the next property used to make up the new filename
    nsString propertyId = renameConfig[i];
    
    // Get the value of the property for the item
    nsString propertyValue;
    rv = aMediaItem->GetProperty(propertyId, propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // If the property had no associated value, skip it
    if (propertyValue.IsEmpty())
      continue;
    
    // Sanitize the property value so that it only contains characters that
    // are valid for a filename
    propertyValue.StripChars(FILE_ILLEGAL_CHARACTERS);
    
    // Concatenate with the value, inserting the separator as needed
    if (!aFilename.IsEmpty()) {
      aFilename.Append(separator);
    }
    aFilename.Append(propertyValue);
  }
  
  // If the resulting filename is empty, we have nothing to rename to, skip the
  // file and report that renaming failed (though the function call succeeded).
  // There may be a better way to handle this (default values for properties
  // with no values ? random filenames ?)
  if (aFilename.IsEmpty()) {
    // aRetVal stays FALSE
    return NS_OK;
  }

  // Get an nsIFileURL object from the nsIURI
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Get the path to the file
  nsString path;
  rv = file->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  // Extract the extension from the file path
  nsString extension;
  rv = GetExtension(path, extension);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Append the extension to the new filename
  aFilename.Append(extension);
  
  // We successfully constructed the new filename
  *aRetVal = PR_TRUE;

  return NS_OK;
}

// Construct the new path based on user settings
nsresult 
sbMediaFileManager::GetNewPath(sbIMediaItem *aMediaItem, 
                               nsIURI *aItemUri,
                               nsString &aPath, 
                               PRBool *aRetVal) {
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  return NS_ERROR_NOT_IMPLEMENTED;
}


// Rename the item to the user-defined pattern
// (sbIMediaFileManager::MANAGE_RENAME)
nsresult 
sbMediaFileManager::CopyRename(sbIMediaItem *aMediaItem, 
                               nsIURI *aItemUri,
                               const nsString &aFilename,
                               const nsString &aPath,
                               PRBool *aRetVal) {
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  nsresult rv;

  NS_ENSURE_STATE(mIOService);

  // Get an nsIFileURL object from the nsIURI
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // Rename to the new filename
  nsCOMPtr<nsILocalFile> newParentDir;
  if (!aPath.IsEmpty()) {
    newParentDir = do_CreateInstance("@mozilla.org/file/local;1");
    rv = newParentDir->InitWithPath(aPath);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // keep newParentDir as NULL, this will cause MoveTo to simply rename
  }
  
  // Do the copy/rename. Note that it is safe to call this with the current
  // path/file (ie, nothing changed, and nothing will happen here)
  rv = file->MoveTo(newParentDir, aFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // MoveTo() has changed the filename and path in the nsIFile object, so make a
  // new URI from it and send it back into the item's contentSrc
  nsCOMPtr<nsIURI> newURI;
  rv = mIOService->NewFileURI(file, getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aMediaItem->SetContentSrc(newURI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Renaming has successfully completed
  *aRetVal = PR_TRUE;

  return NS_OK;
}

// Extracts a file extension from a string
nsresult sbMediaFileManager::GetExtension(const nsString &aURISpec,
                                          nsString &aRetVal) {
  
  aRetVal.Truncate();
  
  PRInt32 fileExtensionIndex = aURISpec.RFind(NS_LITERAL_STRING("."));
  if (fileExtensionIndex < 0)
    return NS_OK;

  aRetVal = nsDependentSubstring(aURISpec, fileExtensionIndex,
                                 aURISpec.Length() - fileExtensionIndex);

  return NS_OK;
}

// Delete the file
nsresult
sbMediaFileManager::Delete(sbIMediaItem *aMediaItem, PRBool *aRetVal) {

  // TODO: Delete the file
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

