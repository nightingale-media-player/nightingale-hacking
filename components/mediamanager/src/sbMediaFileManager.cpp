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
#include <nsStringAPI.h>
#include <unicharutil/nsUnicharUtils.h>

NS_IMPL_ISUPPORTS1(sbMediaFileManager, sbIMediaFileManager);

// ctor
sbMediaFileManager::sbMediaFileManager() {
}

// dtor
sbMediaFileManager::~sbMediaFileManager() {
}

// Init
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
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);
  
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  if (aManageType == 0) {
    // No action was requested, fail altogether
    return NS_ERROR_INVALID_ARG;
  }
  
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
    rv = Delete(aMediaItem, itemUri, aRetVal);
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
  
  if ((aManageType & sbIMediaFileManager::MANAGE_RENAME) ||
      (aManageType & sbIMediaFileManager::MANAGE_COPY)) {
    
    // Perform copy and/or rename
    rv = CopyRename(aMediaItem, itemUri, filename, path, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aManageType & sbIMediaFileManager::MANAGE_COPY) {
      // Check whether the old directory (and parent directories) needs
      // to be deleted
      rv = CheckDirectoryForDeletion(itemUri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  return NS_OK;
}

// Construct the new filename based on user settings
nsresult 
sbMediaFileManager::GetNewFilename(sbIMediaItem *aMediaItem, 
                                   nsIURI *aItemUri,
                                   nsString &aFilename, 
                                   PRBool *aRetVal) {
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

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

  // Get the previous filename extension
  nsCOMPtr<nsIURL> url(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString extension;
  rv = url->GetFileExtension(extension);
  NS_ENSURE_SUCCESS(rv, rv);

  // Append the extension to the new filename
  if (!extension.IsEmpty()) {
    aFilename.Append(NS_LITERAL_STRING("."));
    aFilename.Append(NS_ConvertUTF8toUTF16(extension));
  }
  
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
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;
  
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;
  
  // Get the root of managed directory
  rv = GetManagedDirectoryRoot(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: read from preferences instead of static array
  nsString copyConfig[2] = {
    NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
    NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME)
  };

  for (PRInt32 i=0; i< NS_ARRAY_LENGTH(copyConfig); i++) {
    // get the next property used to make up the new filename
    nsString propertyId = copyConfig[i];
    
    // Get the value of the property for the item
    nsString propertyValue;
    rv = aMediaItem->GetProperty(propertyId, propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // If the property had no associated value, skip it
    if (propertyValue.IsEmpty()) {
      // Ideally this would use the language file, but this implies changing the
      // directory structure whenever the locale is changed. Also it would be
      // nice to be able to do "Unknown %S" where %S is the property name, but
      // this implies the same locale issue. Use the static "Unknown" string for
      // now, though eventually we might want to define a whole bunch of
      // property dependent defaults. Or we could extend sbIProperty to expose
      // such a string, but that's a stopgap measure while waiting for the
      // proper locale-complying infrastructure.
      propertyValue = NS_LITERAL_STRING("Unknown");
    }
    
    // Sanitize the property value so that it only contains characters that
    // are valid for a paths
    propertyValue.StripChars(FILE_ILLEGAL_CHARACTERS);
    
    // Concatenate with the current value
    aPath.Append(propertyValue);
    
    // Add a path separator
    aPath.Append(NS_LITERAL_STRING(FILE_PATH_SEPARATOR));
  }
  
  *aRetVal = PR_TRUE;

  return NS_OK;
}

// Returns the root of the managed directory
nsresult sbMediaFileManager::GetManagedDirectoryRoot(nsString &aRootDir) {
  // TODO: get it from the prefs
  aRootDir = NS_LITERAL_STRING("X:\\Test");
  
  nsresult rv = NormalizeDir(aRootDir);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

// Makes sure that a directory ends with the path separator if it can
nsresult sbMediaFileManager::NormalizeDir(nsString &aDir) {
  // If the directory string is not terminated by a separator, add one,
  // unless the string is empty (empty relative dir)
  nsString separator = NS_LITERAL_STRING(FILE_PATH_SEPARATOR);
  if (!aDir.IsEmpty() && 
      aDir.CharAt(aDir.Length()-1) != separator.CharAt(0)) {
    aDir.Append(separator);
  }
  return NS_OK;
}



// Rename the item to the user-defined pattern
// (sbIMediaFileManager::MANAGE_RENAME, sbIMediaFileManager::MANAGE_COPY)
nsresult 
sbMediaFileManager::CopyRename(sbIMediaItem *aMediaItem, 
                               nsIURI *aItemUri,
                               const nsString &aFilename,
                               const nsString &aPath,
                               PRBool *aRetVal) {
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

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

  // Get the old filename
  nsString oldFilename;
  rv = file->GetLeafName(oldFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIFile> newFile;

  // Is this a copy ?
  if (!aPath.IsEmpty()) {
    nsCOMPtr<nsILocalFile> newParentDir = do_CreateInstance("@mozilla.org/file/local;1");
    rv = newParentDir->InitWithPath(aPath);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Point at the new file
    nsCOMPtr<nsILocalFile> newLocalFile;
    newLocalFile = do_CreateInstance("@mozilla.org/file/local;1");
    rv = newLocalFile->InitWithPath(aPath);
    if (aFilename.IsEmpty())
      newLocalFile->Append(oldFilename);
    else
      newLocalFile->Append(aFilename);
    
    // Are we trying to organize a file that's already where it should be ?
    PRBool equals;
    rv = newLocalFile->Equals(file, &equals);
    NS_ENSURE_SUCCESS(rv, rv);
    if (equals) {
      // yes, skip copying but don't report an error
      *aRetVal = PR_TRUE;
      return NS_OK;
    }
    
    // If the file exists, skip copying and report an error
    PRBool exists;
    rv = newLocalFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (exists) {
      *aRetVal = PR_FALSE;
      return NS_OK;
    }
    
    // Copy
    rv = file->CopyTo(newParentDir, aFilename);
    NS_ENSURE_SUCCESS(rv, rv);

    // We'll use this nsIFile to modify the media item contentsrc
    newFile = do_QueryInterface(newLocalFile, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

  } else {

    // If the file already has the right name, skip renaming, but don't report
    // an error
    #ifdef XP_WIN
    PRBool equals = oldFilename.Equals(aFilename, CaseInsensitiveCompare);
    #else
    PRBool equals = oldFilename.Equals(aFilename);
    #endif
    if (equals) {
      *aRetVal = PR_TRUE;
      return NS_OK;
    }

    // Point at the new file
    nsString path;
    rv = file->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsILocalFile> newLocalFile;
    newLocalFile = do_CreateInstance("@mozilla.org/file/local;1");
    rv = newLocalFile->InitWithPath(aPath);
    newLocalFile->Append(aFilename);
    
    // If the file exists, skip renaming and report an error
    PRBool exists;
    rv = newLocalFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (exists) {
      *aRetVal = PR_FALSE;
      return NS_OK;
    }
    
    // Rename
    rv = file->MoveTo(NULL, aFilename);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // MoveTo() has changed the filename and path in the nsIFile object,
    // so we'll just reuse it when we change the media item contentsrc
    newFile = file;
  }
  
  // Make a new nsIURI object pointing at the new file
  nsCOMPtr<nsIURI> newURI;
  rv = mIOService->NewFileURI(file, getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // And send that URI back as the item contentsrc
  rv = aMediaItem->SetContentSrc(newURI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Renaming/Copying has successfully completed
  *aRetVal = PR_TRUE;

  return NS_OK;
}

// Checks whether a directory and its parent directories need to be deleted
nsresult sbMediaFileManager::CheckDirectoryForDeletion(nsIURI *aItemUri) {
  NS_ENSURE_ARG_POINTER(aItemUri);

  // Get the root of the managed directory
  nsString root;
  nsresult rv = GetManagedDirectoryRoot(root);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the fileurl
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the parent file object
  nsCOMPtr<nsIFile> parent;
  rv = file->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  // And check whether we need to remove it and its parent dirs (recursive call)
  rv = CheckDirectoryForDeletion_Recursive(root, parent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// Recursively checks whether a directory and its parent directories need to
// be deleted
nsresult 
sbMediaFileManager::CheckDirectoryForDeletion_Recursive(nsString &aRoot, 
                                                        nsIFile *aDirectory) {
  NS_ENSURE_ARG_POINTER(aDirectory);

  // Get the path to the directory
  nsString dir;
  nsresult rv = aDirectory->GetPath(dir);
  NS_ENSURE_SUCCESS(rv, rv); 
  
  // Make sure we have a trailing dir char if needed
  rv = NormalizeDir(dir);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check whether the item directory starts with the root of the managed
  // directory (insensitive compare on win32), if that's not the case then
  // we shouldn't be deleting the dir regardless of content
  nsDependentString part(dir.BeginReading(), aRoot.Length());
  #ifdef XP_WIN
  PRBool isRootSubdir = part.Equals(aRoot, CaseInsensitiveCompare);
  #else
  PRBool isRootSubdir = part.Equals(aRoot);
  #endif
  
  if (!isRootSubdir)
    return NS_OK;

  // Check whether the directory is empty
  nsCOMPtr<nsISimpleEnumerator> dirEntries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(dirEntries));

  PRBool hasMoreElements;
  rv = dirEntries->HasMoreElements(&hasMoreElements);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (hasMoreElements) {
    // Directory is not empty, exit recursion now.
    return NS_OK;
  }

  // Directory is empty, and is a subdirectory of the root managed directory,
  // so delete it
  rv = aDirectory->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the parent directory
  nsCOMPtr<nsIFile> parent;
  rv = aDirectory->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  // And recurse ...  
  return CheckDirectoryForDeletion_Recursive(aRoot, parent);
}


// Delete the file
// (sbIMediaFileManager::MANAGE_DELETE)
nsresult
sbMediaFileManager::Delete(sbIMediaItem *aMediaItem, 
                           nsIURI *aItemUri, 
                           PRBool *aRetVal) {
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;
  
  // Get the fileurl
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Delete the file
  rv = file->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check whether we need to also delete the directory (and parent directories)
  rv = CheckDirectoryForDeletion(aItemUri);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Deletion happened successfully
  *aRetVal = PR_TRUE;
  
  return NS_OK;
}

