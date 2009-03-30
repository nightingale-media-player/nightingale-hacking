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

// Songbird includes
#include "sbMediaFileManager.h"
#include <sbIMediaItem.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>

// Mozilla includes
#include <nsCRT.h>
#include <nsStringAPI.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIPrefService.h>
#include <nsIStringBundle.h>
#include <unicharutil/nsUnicharUtils.h>

NS_IMPL_ISUPPORTS1(sbMediaFileManager, sbIMediaFileManager);

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaFileManager:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaFileManagerLog = nsnull;
#define TRACE(args) PR_LOG(gMediaFileManagerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaFileManagerLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * \brief Constructor of the sbMediaFileManager component.
 */
sbMediaFileManager::sbMediaFileManager()
{
#ifdef PR_LOGGING
  if (!gMediaFileManagerLog) {
    gMediaFileManagerLog = PR_NewLogModule("sbMediaFileManager");
  }
#endif
  TRACE(("sbMediaFileManager[0x%.8x] - ctor", this));
  MOZ_COUNT_CTOR(sbMediaFileManager);
}

/**
 * \brief Destructor of the sbMediaFileManager component.
 */
sbMediaFileManager::~sbMediaFileManager()
{
  TRACE(("sbMediaFileManager[0x%.8x] - dtor", this));
  MOZ_COUNT_DTOR(sbMediaFileManager);
}

/**
 * \brief Initialize the sbMediaFileManager component.
 */
NS_IMETHODIMP sbMediaFileManager::Init()
{
  TRACE(("sbMediaFileManager[0x%.8x] - Init", this));

  nsresult rv;
  
  // Get the IOService
  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the NetUtils
  mNetUtil = do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the property manager for the property names
  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the pref branch
  nsCOMPtr<nsIPrefService> prefRoot = 
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  // Get the branch we are interested in
  rv = prefRoot->GetBranch(PREF_MFM_ROOT, getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab our Media Managed Folder
  // Note: prefRoot->GetComplexValue does not work here even passing "folder"
  mMediaFolder = nsnull;
  nsCOMPtr<nsIPrefBranch> rootPrefBranch =
    do_QueryInterface(prefRoot, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootPrefBranch->GetComplexValue(PREF_MFM_LOCATION,
                                       NS_GET_IID(nsILocalFile),
                                       getter_AddRefs(mMediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Track File Format
  nsCString fileFormatString;
  rv = mPrefBranch->GetCharPref(PREF_MFM_FILEFORMAT,
                                getter_Copies(fileFormatString));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Split the string on , character (odd entries are properties, even ones are
  // separators) and save for later.
  nsString_Split(NS_ConvertUTF8toUTF16(fileFormatString),
                 NS_LITERAL_STRING(","),
                 mTrackNameConfig);

  // Get the Folder Format
  nsCString dirFormatString;
  rv = mPrefBranch->GetCharPref(PREF_MFM_DIRFORMAT,
                                getter_Copies(dirFormatString));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Split the string on , character (odd entries are properties, even ones are
  // separators) and save for later.
  nsString_Split(NS_ConvertUTF8toUTF16(dirFormatString),
                 NS_LITERAL_STRING(","),
                 mFolderNameConfig);

  return NS_OK;
}

/**
 * \brief Organize an item in the media library folder.
 * \param aMediaItem  - The media item we are organizing.
 * \param aManageType - How to manage this items file.
 * \param aRetVal     - True if successful.
 */
NS_IMETHODIMP
sbMediaFileManager::OrganizeItem(sbIMediaItem *aMediaItem, 
                                 unsigned short aManageType,
                                 PRBool *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%.8x] - OrganizeItem", this));
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
    rv = Delete(itemUri, aRetVal);
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
  
  if (aManageType & sbIMediaFileManager::MANAGE_MOVE) {
      // Since we are organizing the folder path get the new one
      rv = GetNewPath(aMediaItem, itemUri, path, aRetVal);
      NS_ENSURE_SUCCESS(rv, rv);
  } else if (aManageType & sbIMediaFileManager::MANAGE_COPY) {
      // Since we are only copying and not organizing the folder path just get
      // the root of the managed folder.
      rv = mMediaFolder->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (!path.IsEmpty()) {
        *aRetVal = PR_TRUE;
      }
  }
  // If there was an error trying to get the new path, abort now, but don't
  // cause an exception
  if (!*aRetVal) {
    return NS_OK;
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
      (aManageType & sbIMediaFileManager::MANAGE_COPY) ||
      (aManageType & sbIMediaFileManager::MANAGE_MOVE)) {
    
    // Check if this file is already managed (the old content src is equal to
    // the newly created content src)
    rv = IsOrganized(itemUri, filename, path, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!*aRetVal) {
      // Not managed so perform copy and/or rename and/or move
      rv = CopyRename(aMediaItem, itemUri, filename, path, aRetVal);
      NS_ENSURE_SUCCESS(rv, rv);
  
      if ((aRetVal) &&
          (aManageType & sbIMediaFileManager::MANAGE_MOVE))
      {
        // Delete the original file if it was in the managed folder
        rv = Delete(itemUri, aRetVal);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  
  return NS_OK;
}

/**
 * \brief Construct the new filename based on user settings.
 * \param aMediaItem  - The media item we are organizing.
 * \param aItemUri    - Original file path
 * \param aFilename   - Newly generated filename from the items metadata.
 * \param aRetVal     - True if successful.
 */
nsresult 
sbMediaFileManager::GetNewFilename(sbIMediaItem *aMediaItem, 
                                   nsIURI *aItemUri,
                                   nsString &aFilename, 
                                   PRBool *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%8.x] - GetNewFilename", this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;
  
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  // Start clean
  aFilename.Truncate();

  // Format the file name
  rv = GetFormatedFileFolder(mTrackNameConfig, aMediaItem, PR_FALSE, aFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the resulting filename is empty, we have nothing to rename to, skip the
  // file and report that renaming failed (though the function call succeeded).
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

/**
 * \brief Construct the new path based on user settings.
 * \param aMediaItem  - The media item we are organizing.
 * \param aItemUri    - Original file path
 * \param aPath       - Newly generated path from the items metadata.
 * \param aRetVal     - True if successful.
 */
nsresult 
sbMediaFileManager::GetNewPath(sbIMediaItem *aMediaItem, 
                               nsIURI *aItemUri,
                               nsString &aPath, 
                               PRBool *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%8.x] - GetNewPath", this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;
  
  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;
  
  // Get the root of managed directory
  rv = mMediaFolder->GetPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NormalizeDir(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Format the Folder
  rv = GetFormatedFileFolder(mFolderNameConfig, aMediaItem, PR_TRUE, aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRetVal = PR_TRUE;

  return NS_OK;
}

/**
 * \brief Format a string for a File Name or Folder structure based on the
 *        format spec passed in.
 * \param aFormatSpec - Comma separated values of property,separator,property
 * \param aAppendProperty - When we find a property with no value use "Unknown"
 *        and if this is TRUE append the name of the property so we get values
 *        like "Unkown Album" or Unknown Artist"
 * \param aRetVal - Format string representing the new File Name or Folder.
 */
nsresult
sbMediaFileManager::GetFormatedFileFolder(nsTArray<nsString>  aFormatSpec,
                                          sbIMediaItem*       aMediaItem,
                                          PRBool              aAppendProperty,
                                          nsString&           aRetVal)
{
  TRACE(("sbMediaFileManager[0x%.8x] - GetFormatedFileFolder", this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
 
  nsresult rv;

  for (PRUint32 i=0; i< aFormatSpec.Length(); i++) {
    nsString const & configValue = aFormatSpec[i];
    if (i % 2 != 0) { // Use ! since we start at 0 not 1
      // Evens are the separators
      // We need to decode these before appending...
      nsCAutoString unescapedConfigValue;
      rv = mNetUtil->UnescapeString(NS_ConvertUTF16toUTF8(configValue),
                                    nsINetUtil::ESCAPE_ALL,
                                    unescapedConfigValue);
      NS_ENSURE_SUCCESS(rv, rv);
      aRetVal.AppendLiteral(unescapedConfigValue.get());
    } else {
      // Odds are the properties
      // Get the value of the property for the item
      nsString propertyValue;
      rv = aMediaItem->GetProperty(configValue, propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);
      
      // If the property had no associated value, set it to "unknown"
      if (propertyValue.IsEmpty()) {
        // Use the locale to create the default string for non-existant properties
        // then store it as a pref and always reuse it (so as to avoid it changing
        // on us when the user switches to a different locale)
        nsCString defaultPrefKey;
        defaultPrefKey.AssignLiteral(PREF_MFM_DEFPROPERTY);
        defaultPrefKey.Append(NS_ConvertUTF16toUTF8(configValue));
        
        PRBool prefExists;
        rv = mPrefBranch->PrefHasUserValue(defaultPrefKey.get(), &prefExists);
        NS_ENSURE_SUCCESS(rv, rv);
        
        // If the pref exists, reuse it
        if (prefExists) {
          nsCString value;
          rv = mPrefBranch->GetCharPref(defaultPrefKey.get(),
                                        getter_Copies(value));
          NS_ENSURE_SUCCESS(rv, rv);
          propertyValue = NS_ConvertUTF8toUTF16(value);
        } else {
          // If not, create it
          nsCOMPtr<sbIPropertyInfo> info;
          rv = mPropertyManager->GetPropertyInfo(configValue,
                                                 getter_AddRefs(info));
          NS_ENSURE_SUCCESS(rv, rv);
  
          nsString propertyDisplayName;
          rv = info->GetDisplayName(propertyDisplayName);
          NS_ENSURE_SUCCESS(rv, rv);
  
          sbStringBundle stringBundle;
          nsTArray<nsString> params;
          params.AppendElement(propertyDisplayName);
          propertyValue = stringBundle.Format(STRING_MFM_UNKNOWNPROP,
                                              params,
                                              "Unknown %S");
  
          rv = mPrefBranch->SetCharPref(defaultPrefKey.get(), 
                                   NS_ConvertUTF16toUTF8(propertyValue).get());
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      // Sanitize the property value so that it only contains characters that
      // are valid for a filename
      propertyValue.StripChars(FILE_ILLEGAL_CHARACTERS);
      aRetVal.Append(propertyValue);
    }
  }

  return NS_OK;
}

/**
 * \brief Makes sure that a directory ends with the path separator if it can.
 * \param aDir - The string version of the directory to ensure has a trailing
 *  directory separator.
 */
nsresult sbMediaFileManager::NormalizeDir(nsString &aDir)
{
  TRACE(("sbMediaFileManager[0x%8.x] - NormalizeDir", this));
  // If the directory string is not terminated by a separator, add one,
  // unless the string is empty (empty relative dir)
  nsString separator = NS_LITERAL_STRING(FILE_PATH_SEPARATOR);
  if (!aDir.IsEmpty() && 
      aDir.CharAt(aDir.Length()-1) != separator.CharAt(0)) {
    aDir.Append(separator);
  }
  return NS_OK;
}

/**
 * \brief Copys and/or renames a file based on the items metadata.
 * \param aMediaItem  - The media item we are organizing.
 * \param aItemUri    - Original file path
 * \param aFilename   - Newly generated filename from the items metadata.
 * \param aPath       - Newly generated path from the items metadata.
 * \param aRetVal     - True if successful.
 */
nsresult 
sbMediaFileManager::CopyRename(sbIMediaItem *aMediaItem, 
                               nsIURI *aItemUri,
                               const nsString &aFilename,
                               const nsString &aPath,
                               PRBool *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%.8x] - CopyRename [%s/%s]",
         this,
         NS_ConvertUTF16toUTF8(aPath).get(),
         NS_ConvertUTF16toUTF8(aFilename).get()));
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
    nsCOMPtr<nsILocalFile> newParentDir =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = newParentDir->InitWithPath(aPath);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Point at the new file
    nsCOMPtr<nsILocalFile> newLocalFile;
    newLocalFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newLocalFile->InitWithPath(aPath);
    NS_ENSURE_SUCCESS(rv, rv);
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
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newLocalFile->InitWithPath(aPath);
    NS_ENSURE_SUCCESS(rv, rv);
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
  rv = mIOService->NewFileURI(newFile, getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // And send that URI back as the item contentsrc
  rv = aMediaItem->SetContentSrc(newURI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Renaming/Copying has successfully completed
  *aRetVal = PR_TRUE;

  return NS_OK;
}

/**
 * \brief Checks whether a directory and its parent directories need to be
 *  deleted.
 * \param aItemUri - The Uri of the file we check the directories for.
 */
nsresult sbMediaFileManager::CheckDirectoryForDeletion(nsIURI *aItemUri)
{
  TRACE(("sbMediaFileManager[0x%8.x] - CheckDirectoryDorDeletion", this));
  NS_ENSURE_ARG_POINTER(aItemUri);
  nsresult rv;
  
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
  rv = CheckDirectoryForDeletion_Recursive(parent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Checks a directory to see if it should be removed, it must have no
 *  files and are located in the Managed Folder.
 * \param aDirectory - The folder to check for deletion.
 */
nsresult 
sbMediaFileManager::CheckDirectoryForDeletion_Recursive(nsIFile *aDirectory)
{
  TRACE(("sbMediaFileManager[0x%8.x] - CheckDirectoryForDeletion_Recursive",
         this));
  NS_ENSURE_ARG_POINTER(aDirectory);
  nsresult rv;

  // Make sure this folder is under the managed folder
  PRBool isManaged;
  rv = mMediaFolder->Contains(aDirectory, PR_FALSE, &isManaged);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isManaged) {
    TRACE(("sbMediaFileManager::Check - Not Managed"));
    return NS_OK;
  }

  // Check whether the directory is empty
  nsCOMPtr<nsISimpleEnumerator> dirEntries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(dirEntries));

  PRBool hasMoreElements;
  rv = dirEntries->HasMoreElements(&hasMoreElements);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (hasMoreElements) {
    // Directory is not empty, exit recursion now.
    TRACE(("sbMediaFileManager::Check - Not Empty"));
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
  return CheckDirectoryForDeletion_Recursive(parent);
}


/**
 * \brief Deletes a file associated with an item if it is located in the
 *  managed folder.
 * \param aItemUri  - Original file path
 * \param aRetVal     - True if successful.
 */
nsresult
sbMediaFileManager::Delete(nsIURI *aItemUri, 
                           PRBool *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%8.x] - Delete", this));
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

  // Make sure this file is under the managed folder
  PRBool isManaged;
  rv = mMediaFolder->Contains(file, PR_FALSE, &isManaged);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isManaged) {
    return NS_OK;
  }
  
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

/**
 * \brief IsOrganized determines if this item has already been organized by
 *  comparing the original file path with the newly created file path.
 * \param aItemUri  - Original file path
 * \param aFilename - Newly generated filename from the items metadata.
 * \param aPath     - Newly generated path from the items metadata.
 * \param aRetVal   - True if this item is already organized.
 */
nsresult
sbMediaFileManager::IsOrganized(nsIURI    *aItemUri,
                                nsString  &aFilename,
                                nsString  &aPath,
                                PRBool    *aRetVal)
{
  TRACE(("sbMediaFileManager[0x%8.x] - IsOrganized", this));
  nsresult rv;
  
  // Get an nsIFileURL object from the nsIURI
  nsCOMPtr<nsIFileURL> originalFileUrl(do_QueryInterface(aItemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> originalFile;
  rv = originalFileUrl->GetFile(getter_AddRefs(originalFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> newLocalFile;
  newLocalFile = do_CreateInstance("@mozilla.org/file/local;1");
  rv = newLocalFile->InitWithPath(aPath);
  newLocalFile->Append(aFilename);

  // Check if the old and new content src are the same
  rv = originalFile->Equals(newLocalFile, aRetVal);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}
