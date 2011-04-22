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
#include <sbILibraryUtils.h>
#include <sbIMediaItem.h>
#include <sbIPropertyInfo.h>
#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbURIUtils.h>

// Mozilla includes
#include <nsCRT.h>
#include <nsDirectoryServiceDefs.h>
#include <nsStringAPI.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIPrefService.h>
#include <nsIPropertyBag2.h>
#include <nsIStringBundle.h>
#include <nsUnicharUtils.h>

#define PERMISSIONS_FILE  0644

#ifndef MFM_MUSIC_FOLDER_KEY
# if defined(XP_MACOSX)
#   define MFM_MUSIC_FOLDER_KEY NS_OSX_MUSIC_DOCUMENTS_DIR
# elif defined (XP_UNIX)
#   define MFM_MUSIC_FOLDER_KEY NS_UNIX_XDG_MUSIC_DIR
# else
#   define MFM_MUSIC_FOLDER_KEY "Music"
# endif
#endif // #ifndef MFM_MUSIC_FOLDER_KEY

#ifndef MFM_VIDEO_FOLDER_KEY
# if defined(XP_MACOSX)
#   define MFM_VIDEO_FOLDER_KEY NS_OSX_MOVIE_DOCUMENTS_DIR
# elif defined (XP_UNIX)
#   define MFM_VIDEO_FOLDER_KEY NS_UNIX_XDG_VIDEOS_DIR
# else
#   define MFM_VIDEO_FOLDER_KEY "Video"
# endif
#endif // #ifndef MFM_VIDEO_FOLDER_KEY

#define _MFM_SEPARATOR ":"


NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediaFileManager, sbIMediaFileManager);

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
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * \brief Constructor of the sbMediaFileManager component.
 */
sbMediaFileManager::sbMediaFileManager()
  : mInitialized(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gMediaFileManagerLog) {
    gMediaFileManagerLog = PR_NewLogModule("sbMediaFileManager");
  }
#endif
  TRACE(("%s", __FUNCTION__));
}

/**
 * \brief Destructor of the sbMediaFileManager component.
 */
sbMediaFileManager::~sbMediaFileManager()
{
  TRACE(("%s", __FUNCTION__));
}

/**
 * \brief Initialize the sbMediaFileManager component.
 */
NS_IMETHODIMP sbMediaFileManager::Init(nsIPropertyBag2 *aProperties)
{
  TRACE(("%s", __FUNCTION__));

  NS_NAMED_LITERAL_STRING(KEY_FILE_FORMAT, "file-format");

  nsresult rv;

  PRBool hasKey;
  nsCOMPtr<nsIPropertyBag2> properties = aProperties;
  if (!properties) {
    properties = do_CreateInstance("@mozilla.org/hash-property-bag;1");
    NS_ENSURE_TRUE(properties, NS_ERROR_OUT_OF_MEMORY);
  }

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
  nsCOMPtr<nsIThread> mainThread;
  rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(PREF_MFM_ROOT, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = do_GetProxyForObject(mainThread,
                            prefBranch.get(),
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Track File Format
  nsCString fileFormatString;
  rv = properties->HasKey(KEY_FILE_FORMAT, &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    rv = properties->GetPropertyAsACString(KEY_FILE_FORMAT, fileFormatString);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = mPrefBranch->GetCharPref(PREF_MFM_FILEFORMAT,
                                  getter_Copies(fileFormatString));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Split the string on , character (odd entries are properties, even ones are
  // separators) and save for later.
  nsString_Split(NS_ConvertUTF8toUTF16(fileFormatString),
                 NS_LITERAL_STRING(","),
                 mTrackNameTemplate);

  rv = InitFolderNameTemplates(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitMediaFoldersMap(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

/**
 * \brief Organize an item in the media library folder.
 * \param aMediaItem  - The media item we are organizing.
 * \param aManageType - How to manage this items file.
 * \param aForceTargetFile - [optional] a precalculated target
 * \param aRetVal     - True if successful.
 */
NS_IMETHODIMP
sbMediaFileManager::OrganizeItem(sbIMediaItem   *aMediaItem,
                                 unsigned short aManageType,
                                 nsIFile        *aForceTargetFile,
                                 PRBool         *aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  if (aManageType == 0) {
    // No action was requested, fail altogether
    TRACE(("%s - no action requested (%04x)", __FUNCTION__, aManageType));
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;

  // First thing is to check if the Managment folder exists, we need this because
  // all operations require that folder, we only delete from that folder and we
  // only copy to or move in that folder. We also need to make sure we have the
  // management folder, if not get it from the preferences.

  // First see if we have a management folder for this media item
  rv = CheckManagementFolder(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now make sure that we're operating on a file

  // Get to the old file's path
  nsCOMPtr<nsIURI> itemUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(itemUri));
  NS_ENSURE_SUCCESS (rv, rv);

  // Get an nsIFileURL object from the nsIURI
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(itemUri, &rv));
  if (NS_FAILED(rv) || !fileUrl) {
    #if PR_LOGGING
      nsCString spec;
      rv = itemUri->GetSpec(spec);
      if (NS_SUCCEEDED(rv)) {
        TRACE(("%s: item %s is not a local file", __FUNCTION__, spec.get()));
      }
    #endif /* PR_LOGGING */
    return NS_ERROR_INVALID_ARG;
  }

  // Get the file object and check that it exists.
  nsCOMPtr<nsIFile> itemFile;
  rv = fileUrl->GetFile(getter_AddRefs(itemFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure the original file exists before doing any organizing.
  PRBool exists;
  rv = itemFile->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    return NS_ERROR_FILE_NOT_FOUND;
  }

  if (aManageType & sbIMediaFileManager::MANAGE_DELETE) {
    rv = Delete(itemFile, aRetVal);
    NS_ENSURE_SUCCESS(rv, rv);

    // MANAGE_DELETE does not make sense to use in conjunction with other flags,
    // so if anymore action was requested, return false in aRetVal, but don't
    // cause an exception.

    if (aManageType != sbIMediaFileManager::MANAGE_DELETE) {
      LOG(("%s: Recieved a MANAGE_DELETE with other flags (%04x).",
            __FUNCTION__,
            aManageType));
      *aRetVal = PR_FALSE;
    }

    return NS_OK;
  }

  if (!((aManageType & sbIMediaFileManager::MANAGE_MOVE) ||
        (aManageType & sbIMediaFileManager::MANAGE_COPY) ||
        (aManageType & sbIMediaFileManager::MANAGE_RENAME)))
  {
    LOG(("%s: nothing to manage", __FUNCTION__));
    return NS_OK;
  }

  // Get the target file
  nsCOMPtr<nsIFile> newFile;

  if (aForceTargetFile) {
    #if PR_LOGGING
      nsString path;
      rv = aForceTargetFile->GetPath(path);
      if (NS_SUCCEEDED(rv)) {
        TRACE(("%s: has forced file [%s]",
               __FUNCTION__,
               NS_ConvertUTF16toUTF8(path).get()));
      } else {
        TRACE(("%s: has forced file, unknown path (error %08x)", __FUNCTION__, rv));
      }
    #endif /* PR_LOGGING */
    rv = aForceTargetFile->Clone(getter_AddRefs(newFile));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = GetManagedPath(aMediaItem, aManageType, getter_AddRefs(newFile));
    NS_ENSURE_SUCCESS(rv, rv);
    if (rv == NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA) {
      // non-fatal failure, go to next item
      LOG(("%s: failed to get managed path", __FUNCTION__));
      return NS_OK;
    }
    #if PR_LOGGING
      nsString path;
      rv = newFile->GetPath(path);
      if (NS_SUCCEEDED(rv)) {
        TRACE(("%s: has a managed file [%s]",
               __FUNCTION__,
               NS_ConvertUTF16toUTF8(path).get()));
      } else {
        TRACE(("%s: has managed file, unknown path (error %08x)",
               __FUNCTION__,
               rv));
      }
    #endif /* PR_LOGGING */
  }

  // Check if we are already organized;
  PRBool isOrganized = PR_FALSE;
  rv = newFile->Equals(itemFile, &isOrganized);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOrganized) {
    // Already managed so just return ok
    TRACE(("%s: Already organized", __FUNCTION__));
    *aRetVal = PR_TRUE;
    return NS_OK;
  }

  // Not managed so perform copy, move and/or rename
  // NOTE: this will check that our destination is in the managed folder.
  rv = CopyRename(aMediaItem, itemFile, newFile, aRetVal);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Get the path the item would be organized to if we were to do it
 * \param aMediaItem  - the media item we will organize
 * \param aManageType - the actions to take (see flags above)
 * \param _retval     - the resulting path
 */
/* nsIFile getManagedPath (in sbIMediaItem aItem, in unsigned short aManageType); */
NS_IMETHODIMP
sbMediaFileManager::GetManagedPath(sbIMediaItem *aItem,
                                   PRUint16     aManageType,
                                   nsIFile      **_retval)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  if (aManageType == 0) {
    // No action was requested, fail altogether
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;

  // First see if we have a management folder for this media item
  rv = CheckManagementFolder(aItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Next make sure that we're operating on a file

  // Get to the old file's path
  nsCOMPtr<nsIURI> itemUri;
  rv = aItem->GetContentSrc(getter_AddRefs(itemUri));
  NS_ENSURE_SUCCESS (rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> oldItemFile;
  // Get the file from the URI and delete if needed
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(itemUri, &rv));
  if (NS_SUCCEEDED(rv)) {

    rv = fileUrl->GetFile(getter_AddRefs(oldItemFile));
    NS_ENSURE_SUCCESS(rv, rv);

    if (aManageType & sbIMediaFileManager::MANAGE_DELETE) {
      // file will be deleted; don't make a new path
      oldItemFile.forget(_retval);
      return NS_OK;
    }
  }
  // If we're told to delete and this is not a file, that's bad
  else if (aManageType & sbIMediaFileManager::MANAGE_DELETE) {
    return NS_ERROR_INVALID_ARG;
  }

  // Find the target file
  nsCOMPtr<nsIFile> newItemFile;

  nsString filename;
  nsString path;
  PRBool success;

  // figure out the containing folder for the final path
  if (aManageType & sbIMediaFileManager::MANAGE_MOVE) {
    // Since we are organizing the folder path get the new one
    rv = GetNewPath(aItem, path, &success);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(success, NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA);
    nsCOMPtr<nsILocalFile> localFile =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = localFile->InitWithPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    newItemFile = do_QueryInterface(localFile, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aManageType & sbIMediaFileManager::MANAGE_COPY) {
    // Since we are only copying and not organizing the folder path just get
    // the root of the managed folder.
    nsCOMPtr<nsIFile> mediaFolder;
    rv = GetMediaFolder(aItem, getter_AddRefs(mediaFolder));
    NS_ENSURE_SUCCESS(rv, NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA);
    rv = mediaFolder->Clone(getter_AddRefs(newItemFile));
    NS_ENSURE_SUCCESS(rv, NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA);
  } else {
    // neither move nor copy is set, we need to abort
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }

  if (aManageType & sbIMediaFileManager::MANAGE_RENAME) {
    rv = GetNewFilename(aItem, itemUri, filename, &success);
    NS_ENSURE_SUCCESS(rv, rv);
    // If there was an error trying to construct the new filename, abort now,
    // but don't cause an exception
    NS_ENSURE_TRUE(success, NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA);
  } else if (oldItemFile) {
    // not renaming, use the old name
    nsCOMPtr<sbILibraryUtils> libUtils =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> canonicalOldFile;
    rv = libUtils->GetCanonicalPath(oldItemFile,
                                    getter_AddRefs(canonicalOldFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = canonicalOldFile->GetLeafName(filename);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Use the item URL file name.
    nsCAutoString cFileName;
    nsCOMPtr<nsIURL> itemURL = do_MainThreadQueryInterface(itemUri, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = itemURL->GetFileName(cFileName);
    NS_ENSURE_SUCCESS(rv, rv);
    filename = NS_ConvertUTF8toUTF16(cFileName);
  }
  rv = newItemFile->Append(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  newItemFile.forget(_retval);

  return NS_OK;
}

nsresult
sbMediaFileManager::InitMediaFoldersMap(nsIPropertyBag2 * aProperties)
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;

  // There will be at most three media folders, one for audio, one
  // for video, and one to rule them all:
  mMediaFolders.Init(3);

  PRBool ok = PR_FALSE;

  // Check the property bag for a custom media folder.  A custom
  // folder, if provided, will override the default folders:
  nsCOMPtr<nsIFile> customFolder;
  if (aProperties) {
    NS_NAMED_LITERAL_STRING(KEY_CUSTOM_MEDIA_FOLDER, "media-folder");
    PRBool hasKey = PR_FALSE;
    rv = aProperties->HasKey(KEY_CUSTOM_MEDIA_FOLDER, &hasKey);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasKey) {
      rv = aProperties->GetPropertyAsInterface(KEY_CUSTOM_MEDIA_FOLDER,
                                               NS_GET_IID(nsIFile),
                                               getter_AddRefs(customFolder));
      NS_ENSURE_SUCCESS(rv, rv);

      // Got a custom media folder.  Set it as the default and only folder
      // for all content types and return.  The empty key designates the
      // default folder to use if no other folder is defined for the desired
      // content type:
      ok = mMediaFolders.Put(EmptyString(), customFolder);
      NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  nsCOMPtr<nsIProperties> dirService =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use a custom music folder, if provided, or any generic custom folder
  // that may have been provided above.  Otherwise, use the designated
  // music folder for the current platform:
  nsCOMPtr<nsIFile> musicDir = customFolder;
  if (aProperties) {
    NS_NAMED_LITERAL_STRING(KEY_CUSTOM_MUSIC_FOLDER, "media-folder:audio");
    PRBool hasKey = PR_FALSE;
    rv = aProperties->HasKey(KEY_CUSTOM_MUSIC_FOLDER, &hasKey);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasKey) {
      aProperties->GetPropertyAsInterface(KEY_CUSTOM_MUSIC_FOLDER,
                                          NS_GET_IID(nsIFile),
                                          getter_AddRefs(musicDir));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Resort to the platform music folder only if no custom folder
  // was supplied:
  if (!musicDir) {
    dirService->Get(MFM_MUSIC_FOLDER_KEY,
                    NS_GET_IID(nsIFile),
                    getter_AddRefs(musicDir));
  }

  if (musicDir) {
    // Got a music folder.  Set it as the media folder for audio content:
    ok = mMediaFolders.Put(NS_LITERAL_STRING("audio"), musicDir);
    NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_WARNING("Could not get music folder");
  }

  // Use a custom video folder, if provided, or any generic custom folder
  // that may have been provided above.  Otherwise, use the designated
  // video folder for the current platform:
  nsCOMPtr<nsIFile> videoDir = customFolder;
  if (aProperties) {
    NS_NAMED_LITERAL_STRING(KEY_CUSTOM_VIDEO_FOLDER, "media-folder:video");
    PRBool hasKey = PR_FALSE;
    rv = aProperties->HasKey(KEY_CUSTOM_VIDEO_FOLDER, &hasKey);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasKey) {
      aProperties->GetPropertyAsInterface(KEY_CUSTOM_VIDEO_FOLDER,
                                          NS_GET_IID(nsIFile),
                                          getter_AddRefs(videoDir));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Resort to the platform video folder only if no custom folder
  // was supplied:
  if (!videoDir) {
    dirService->Get(MFM_VIDEO_FOLDER_KEY,
                    NS_GET_IID(nsIFile),
                    getter_AddRefs(videoDir));
  }

  if (videoDir) {
    // Got a video folder.  Set it as the media folder for video content:
    ok = mMediaFolders.Put(NS_LITERAL_STRING("video"), videoDir);
    NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_WARNING("Could not get video folder");
  }

  return NS_OK;
}

nsresult
sbMediaFileManager::GetMediaFolder(sbIMediaItem * aMediaItem,
                                   nsIFile **     aFolder)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aFolder);

  nsresult rv;

  // Try to get a media folder appropriate for the content
  // type of the media item:
  nsAutoString contentType;
  rv = aMediaItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool defined = mMediaFolders.Get(contentType, aFolder);
  if (!defined) {
    // No content type-specific media folder.  Try the default
    // folder for all content types:
    defined = mMediaFolders.Get(EmptyString(), aFolder);
    NS_ENSURE_TRUE(defined, NS_ERROR_NOT_AVAILABLE);
  }

  // Should have a folder now.  Return it:
  NS_ENSURE_TRUE(*aFolder, NS_ERROR_NOT_AVAILABLE);
  (*aFolder)->AddRef();

  return NS_OK;
}

namespace
{
  // Context for locating which media folder, if any, a file is in
  struct OnEachMediaFolder_t
  {
    nsIFile * mFile;
    nsIFile ** mFolder;
  };

  // Check whether the folder designated by aData contains the file
  // given in the userArg context:
  PLDHashOperator
  OnEachMediaFolder(nsStringHashKey::KeyType  aKey,
                    nsIFile *                 aData,
                    void*                     userArg)
  {
    TRACE(("%s", __FUNCTION__));

    // Stop iterating if the context is invalid.  If any other
    // argument is invalid, skip to the next iteration:
    NS_ENSURE_TRUE(aData, PL_DHASH_NEXT);
    NS_ENSURE_TRUE(userArg, PL_DHASH_STOP);

    nsresult rv;

    const OnEachMediaFolder_t * context =
      static_cast<const OnEachMediaFolder_t *>(userArg);
    NS_ENSURE_TRUE(context->mFile, PL_DHASH_STOP);
    NS_ENSURE_TRUE(context->mFolder, PL_DHASH_STOP);
    
#if DEBUG
    // For viewing in the debugger:
    nsAutoString contentType;
    contentType = aKey;
    nsAutoString mediaFolder;
    aData->GetPath(mediaFolder);
    nsAutoString itemPath;
    context->mFile->GetPath(itemPath);
#endif

    PRBool found = PR_FALSE;
    rv = aData->Contains(context->mFile, PR_TRUE, &found);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    // Keep iterating until a media folder containing the file
    // is found:
    if (!found) {
      return PL_DHASH_NEXT;
    }

    // Found the containing folder.  Return it:
    NS_ADDREF(*context->mFolder = aData);
    return PL_DHASH_STOP;
  }
}

nsresult
sbMediaFileManager::GetMediaFolder(nsIFile *   aFile,
                                   nsIFile **  aFolder)
{
  TRACE(("%s", __FUNCTION__));

  // Search the media folders for one containing the given file:
  OnEachMediaFolder_t context;
  context.mFile = aFile;
  context.mFolder = aFolder;
  mMediaFolders.EnumerateRead(OnEachMediaFolder, &context);

  return NS_OK;
}

nsresult
sbMediaFileManager::InitFolderNameTemplates(nsIPropertyBag2 * aProperties)
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;

  NameTemplate nameTemplate;
  nsAutoString key;
  PRBool ok = PR_FALSE;
  
  mFolderNameTemplates.Init();

  // Check the property bag for a custom media subfolder path template.
  // A custom template, if provided, will override the default templates:
  if (aProperties) {
    NS_NAMED_LITERAL_STRING(KEY_CUSTOM_DIR_TEMPLATE, "dir-format");
    PRBool hasKey = PR_FALSE;
    rv = aProperties->HasKey(KEY_CUSTOM_DIR_TEMPLATE, &hasKey);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasKey) {
      nsCString templateAsString;
      rv = aProperties->GetPropertyAsACString(KEY_CUSTOM_DIR_TEMPLATE,
                                              templateAsString);
      NS_ENSURE_SUCCESS(rv, rv);

      // Got a custom template.  Split the string on the comma delimiter,
      // set the result as the default and only template for all media
      // files, and return:
      nsString_Split(NS_ConvertUTF8toUTF16(templateAsString),
                     NS_LITERAL_STRING(","),
                     nameTemplate);
      ok = mFolderNameTemplates.Put(EmptyString(), nameTemplate);
      NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
      return NS_OK;
    }
  }

  // No custom media subfolder path template was provided.  Set
  // mFolderNameTemplates to built-in defaults.  mFolderNameTemplates
  // maps specific values of specific properties to the template that
  // should be used when that value describes a file.  For
  // example, the key
  //
  //    http://songbirdnest.com/data/1.0#genre:Zydeco
  //
  // could specify the template to use for items with a genre of "Zydeco".
  // Not all properties are considered, though, when searching for an
  // appropriate subfolder template for a media item.  Indeed, only the
  // properties actually used below,
  //
  //    SB_PROPERTY_IMPORTTYPE, and
  //    SB_PROPERTY_CONTENTTYPE
  //
  // are considered, in that order (see GetFolderNameTemplate()).
  //
  // The empty key designates the default template to use if no other
  // template is found.

  // Default to an empty template, which will situate a media file
  // at the root of its media folder:
  nameTemplate.Clear();

  // We add an empty string here. An empty string is considered equivalent to
  // no entry at all - so this makes this equivalent to an actually empty
  // template. However, due to a bug in how we link things on OSX (see the patch
  // we have to nsTArray.cpp in our tree), destroying an empty nsTArray does
  // not work correctly (malloc assertions, etc.). This hack lets us get around
  // that with minimal additional pain. It's still pretty stupid; it'd be much
  // better to fix the linking - unfortunately my knowledge of Mach-O etc. is
  // a little too limited for that.
  nameTemplate.AppendElement(NS_LITERAL_STRING(""));

  ok = mFolderNameTemplates.Put(EmptyString(), nameTemplate);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  // Audio files default to Artist/Album/
  nameTemplate.Clear();
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME)),
    NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(NS_LITERAL_STRING(FILE_PATH_SEPARATOR)),
    NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME)),
    NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(NS_LITERAL_STRING(FILE_PATH_SEPARATOR)),
    NS_ERROR_OUT_OF_MEMORY);

  key.AssignLiteral(SB_PROPERTY_CONTENTTYPE _MFM_SEPARATOR "audio");
  ok = mFolderNameTemplates.Put(key, nameTemplate);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  // Recordings get their own subfolder:
  sbStringBundle strings;
  nsAutoString recordingsFolder(strings.Get(STRING_MFM_RECORDINGS_FOLDER,
                                            "Recordings"));
  recordingsFolder.AppendLiteral(FILE_PATH_SEPARATOR);

  nameTemplate.Clear();
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(EmptyString()),
    NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(
    nameTemplate.AppendElement(recordingsFolder),
    NS_ERROR_OUT_OF_MEMORY);
  
  key.AssignLiteral(SB_PROPERTY_IMPORTTYPE
                    _MFM_SEPARATOR
                    SB_VALUE_IMPORTTYPE_FM_RECORDING);
  ok = mFolderNameTemplates.Put(key, nameTemplate);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  key.AssignLiteral(SB_PROPERTY_IMPORTTYPE
                    _MFM_SEPARATOR
                    SB_VALUE_IMPORTTYPE_VOICE_RECORDING);
  ok = mFolderNameTemplates.Put(key, nameTemplate);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbMediaFileManager::GetFolderNameTemplate(sbIMediaItem *  aMediaItem,
                                          NameTemplate &  aNameTemplate)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  // Consider these properties in this order when searching for
  // a folder name template for the given media item:
  static const char * const ORGANIZING_PROPERTIES[] = {
                              SB_PROPERTY_IMPORTTYPE,
                              SB_PROPERTY_CONTENTTYPE,
                              };
  for (size_t i = 0; i < NS_ARRAY_LENGTH(ORGANIZING_PROPERTIES); i++) {
    // Construct a key string from the property name and the media item's
    // value for that property.  For example:
    //
    //    http://songbirdnest.com/data/1.0#contentType:audio

    nsAutoString key;
    nsAutoString suffix;
    key.AssignLiteral(ORGANIZING_PROPERTIES[i]);
    rv = aMediaItem->GetProperty(key, suffix);
    NS_ENSURE_SUCCESS(rv, rv);

    key.AppendLiteral(_MFM_SEPARATOR);
    key.Append(suffix);

    // Check for a folder template with this key:
    if (mFolderNameTemplates.Get(key, &aNameTemplate)) {
      // Got a specific template for this property value. Return:
      return NS_OK;
    }
  }

  // No folder name template specific to the media item's properties
  // was found.  Check for a default template:
  if (mFolderNameTemplates.Get(EmptyString(), &aNameTemplate)) {
    return NS_OK;
  }

  // No default template either.  Return an empty template, which
  // will situate the media file at the root of its media folder:
  aNameTemplate.Clear();
  return NS_OK;
}

/**
 * \brief Construct the new filename based on user settings.
 * \param aMediaItem  - The media item we are organizing.
 * \param aItemUri    - Original file path
 * \param aFilename   - Newly generated filename from the items metadata.
 *        This function will replace aFilename with new contents.
 * \param aRetVal     - True if successful.
 */
nsresult
sbMediaFileManager::GetNewFilename(sbIMediaItem *aMediaItem,
                                   nsIURI       *aItemUri,
                                   nsString     &aFilename,
                                   PRBool       *aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aItemUri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  // Start clean
  aFilename.Truncate();

  nsCString extension;

  // Get the previous filename extension
  nsCOMPtr<nsIURL> url(do_QueryInterface(aItemUri, &rv));
  if (NS_FAILED(rv)) {
    rv = sbGetFileExtensionFromURI(aItemUri, extension);
    // Likely to fail on malformed URI's
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  else {
    rv = url->GetFileExtension(extension);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsString fullExtension;
  if (!extension.IsEmpty()) {
    fullExtension.Insert(PRUnichar('.'), 0);
    fullExtension.Append(NS_ConvertUTF8toUTF16(extension));
  }
  // Format the file name
  rv = GetFormattedFileFolder(mTrackNameTemplate,
                              aMediaItem,
                              PR_FALSE,
                              PR_FALSE,           // Trim the full filename
                              fullExtension,
                              aFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the resulting filename is empty, we have nothing to rename to, skip the
  // file and report that renaming failed (though the function call succeeded).
  if (aFilename.IsEmpty()) {
    // aRetVal stays FALSE
    return NS_OK;
  }

  // Append the extension to the new filename
  if (!fullExtension.IsEmpty()) {
    aFilename.Append(fullExtension);
  }

  // We successfully constructed the new filename
  *aRetVal = PR_TRUE;

  return NS_OK;
}

/**
 * \brief Construct the new path based on user settings.
 * \param aMediaItem  - The media item we are organizing.
 * \param aPath       - Newly generated path from the items metadata.
 * \param aRetVal     - True if successful.
 */
nsresult
sbMediaFileManager::GetNewPath(sbIMediaItem *aMediaItem,
                               nsString     &aPath,
                               PRBool       *aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  // First see if we have a management folder for this media item
  rv = CheckManagementFolder(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the root of managed directory
  nsCOMPtr<nsIFile> mediaFolder;
  rv = GetMediaFolder(aMediaItem, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mediaFolder->GetPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NormalizeDir(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Format the Folder
  NameTemplate folderNameTemplate;
  rv = GetFolderNameTemplate(aMediaItem, folderNameTemplate);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GetFormattedFileFolder(folderNameTemplate,
                              aMediaItem,
                              PR_TRUE,
                              PR_TRUE,          // Trim each folder
                              EmptyString(),
                              aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRetVal = PR_TRUE;

  return NS_OK;
}

void
sbMediaFileManager::RemoveBadCharacters(nsString& aStringToParse)
{
  TRACE(("%s", __FUNCTION__));

  // Sanitize the string so that it only contains characters that are valid for
  // a filename
  aStringToParse.StripChars(FILE_ILLEGAL_CHARACTERS);

  // We also need to strip path separators since filenames and folder names
  // should not have them.
  aStringToParse.StripChars(FILE_PATH_SEPARATOR);

  // Windows does not like Spaces at the begining or end of a file/folder name
  // Windows also does not like dots at the begining or end of the file/folder
  // name and dots are bad as well on some other operating systems as they
  // represent a hidden file.
  aStringToParse.Trim(" .", PR_TRUE, PR_TRUE);
}

nsresult
sbMediaFileManager::GetUnknownValue(nsString  aPropertyKey,
                                    nsString& aUnknownValue)
{
  TRACE(("%s (%s)", __FUNCTION__, NS_ConvertUTF16toUTF8(aPropertyKey).BeginReading()));
  nsresult rv;

  // Make sure the result is empty for return in case we can not find
  // what we should put as Unknown
  aUnknownValue.Truncate();

  // Use the locale to create the default string for non-existant properties
  // then store it as a pref and always reuse it (so as to avoid it changing
  // on us when the user switches to a different locale)
  nsCString defaultPrefKey;
  defaultPrefKey.AssignLiteral(PREF_MFM_DEFPROPERTY);
  defaultPrefKey.Append(NS_ConvertUTF16toUTF8(aPropertyKey));

  PRBool prefExists;
  rv = mPrefBranch->PrefHasUserValue(defaultPrefKey.get(), &prefExists);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the pref exists, reuse it
  if (prefExists) {
    nsCString value;
    rv = mPrefBranch->GetCharPref(defaultPrefKey.get(),
                                  getter_Copies(value));
    NS_ENSURE_SUCCESS(rv, rv);
    aUnknownValue.Assign(NS_ConvertUTF8toUTF16(value));
  } else {
    // If not, create it
    nsCOMPtr<sbIPropertyInfo> info;
    rv = mPropertyManager->GetPropertyInfo(aPropertyKey,
                                           getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);

    // try to get a custom localized string first
    sbStringBundle stringBundle;

    nsString unknownKey;
    rv = info->GetLocalizationKey(unknownKey);
    NS_ENSURE_SUCCESS(rv, rv);
    unknownKey.Insert(NS_LITERAL_STRING("."), 0);
    unknownKey.Insert(NS_LITERAL_STRING(STRING_MFM_UNKNOWNPROP), 0);
    aUnknownValue.Assign(stringBundle.Get(unknownKey, aPropertyKey));

    if (aUnknownValue.Equals(aPropertyKey) ||
        aUnknownValue.EqualsLiteral("%S"))
    {
      // no custom default value, use the fallback

      nsString propertyDisplayName;
      rv = info->GetDisplayName(propertyDisplayName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (propertyDisplayName.IsEmpty()) {
        aUnknownValue.Assign(stringBundle.Get(STRING_MFM_UNKNOWNPROP_EMPTY,
                                              "Unknown"));
      } else {
        nsTArray<nsString> params;
        params.AppendElement(propertyDisplayName);
        aUnknownValue.Assign(stringBundle.Format(STRING_MFM_UNKNOWNPROP,
                                                 params,
                                                 "Unknown %S"));
      }
    }

    rv = mPrefBranch->SetCharPref(defaultPrefKey.get(),
                             NS_ConvertUTF16toUTF8(aUnknownValue).get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/**
 * \brief Format a string for a File Name or Folder structure based on the
 *        format spec passed in.
 * \param aNameTemplate - Array of strings alternating between property and
 *        separator. [property, separator, property...]
 * \param aAppendProperty - When we find a property with no value use "Unknown"
 *        and if this is TRUE append the name of the property so we get values
 *        like "Unkown Album" or Unknown Artist"
 * \param aTrimEachProperty - If FALSE then we will wait until all of the properties
 *        have been put together to trim spaces from the front and end of the
 *        result, if TRUE then we will trim each property value.
 * \param aFileExtension - This is the file extension that is going to be
 *        added on to the string, we use this to remove any from the property
 *        value so we don't get duplicates.
 * \param aRetVal - Format string representing the new File Name or Folder.
 *        This function will append to the aRetVal not replace.
 */
nsresult
sbMediaFileManager::GetFormattedFileFolder(
                                      const NameTemplate &  aNameTemplate,
                                      sbIMediaItem*         aMediaItem,
                                      PRBool                aAppendProperty,
                                      PRBool                aTrimEachProperty,
                                      nsString              aFileExtension,
                                      nsString&             aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  for (PRUint32 i=0; i< aNameTemplate.Length(); i++) {
    nsString const & configValue = aNameTemplate[i];
    // Any empty template term evaluates to an empty string
    if (configValue.IsEmpty()) {
      continue;
    }

    if (i % 2 != 0) { // Use ! since we start at 0 not 1
      // Evens are the separators
      // We need to decode these before appending...
      nsCAutoString unescapedValue;
      rv = mNetUtil->UnescapeString(NS_ConvertUTF16toUTF8(configValue),
                                    nsINetUtil::ESCAPE_ALL,
                                    unescapedValue);
      NS_ENSURE_SUCCESS(rv, rv);
      aRetVal.AppendLiteral(unescapedValue.get());
    } else {
      // Odds are the properties
      // Get the value of the property for the item
      nsString propertyValue;
      rv = aMediaItem->GetProperty(configValue, propertyValue);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!propertyValue.IsEmpty()) {
        if (aTrimEachProperty) {
          RemoveBadCharacters(propertyValue);
        }

        // Handle track number padding if the user has turned on the pref.
        // NOTE: Don't bother checking result value.
        PRBool shouldPadTrackNum = PR_FALSE;
        mPrefBranch->GetBoolPref(PREF_MFM_PADTRACKNUM, &shouldPadTrackNum);
        if (shouldPadTrackNum &&
            configValue.EqualsLiteral(SB_PROPERTY_TRACKNUMBER))
        {
          // Get the number of tracks for this mediaitem.
          // NOTE: Don't worry about checking the return value.
          nsString numTracksStr;
          aMediaItem->GetProperty(
              NS_LITERAL_STRING(SB_PROPERTY_TOTALTRACKS),
              numTracksStr);

          nsString formattedProperty;
          rv = ZeroPadTrackNumber(propertyValue,
                                  numTracksStr,
                                  formattedProperty);
          if (NS_SUCCEEDED(rv)) {
            propertyValue = formattedProperty;
          }
        }

        // We need to check the Track Name property for an extension at the end
        // and remove it if it exists.
        if (!aFileExtension.IsEmpty() &&
            configValue.EqualsLiteral(SB_PROPERTY_TRACKNAME)) {
          if (StringEndsWith(propertyValue, aFileExtension)) {
            propertyValue.SetLength(propertyValue.Length() - aFileExtension.Length());
          }
        }
      }

      // If the property had no associated value, set it to "unknown" if we can
      if (propertyValue.IsEmpty()) {
        TRACE(("%s: getting fallback for %s",
               __FUNCTION__,
               NS_ConvertUTF16toUTF8(configValue).get()));
        rv = GetUnknownValue(configValue, propertyValue);
        if (NS_FAILED(rv) || propertyValue.IsEmpty()) {
          // there was no data, _and_ the fallback value was empty
          // skip this property and the next separator
          i++;
          continue;
        }
        if (aTrimEachProperty) {
          RemoveBadCharacters(propertyValue);
        }
      }

      // We finally have something to add
      aRetVal.Append(propertyValue);
    }
  }

  if (!aTrimEachProperty && !aRetVal.IsEmpty()) {
    RemoveBadCharacters(aRetVal);
  }

  return NS_OK;
}

/**
 * \brief Check the managed folder to see if it is available.
 * \param aMediaItem - used to select which media folder to check
 */
nsresult
sbMediaFileManager::CheckManagementFolder(sbIMediaItem * aMediaItem)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  nsCOMPtr<nsIFile> mediaFolder;
  rv = GetMediaFolder(aMediaItem, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS (rv, rv);

  if (!mediaFolder) {
    return NS_ERROR_FAILURE;
  }

  // Always check that the media folder exists since it could have been
  // disconnected or removed from the system.
  PRBool exists = PR_FALSE;
  rv = mediaFolder->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(exists, NS_ERROR_FILE_TARGET_DOES_NOT_EXIST);

  return NS_OK;
}

/**
 * \brief Makes sure that a directory ends with the path separator if it can.
 * \param aDir - The string version of the directory to ensure has a trailing
 *  directory separator.
 */
nsresult sbMediaFileManager::NormalizeDir(nsString &aDir)
{
  TRACE(("%s", __FUNCTION__));
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
 * \brief Format a track number to pad for zeros against the total track count.
 * \param aTrackNumStr The track number property as a string.
 * \param aTotalTrackCountStr The total tracks property as a string
 * \param aOutString The out-param for the formatted string.
 *
 * Example: 2 of 10 tracks will return "02"
 *          2 of 100 tracks will return "002"
 */
nsresult
sbMediaFileManager::ZeroPadTrackNumber(const nsAString & aTrackNumStr,
                                       const nsAString & aTotalTrackCountStr,
                                       nsString & aOutString)
{
  nsString format(aTrackNumStr);
  nsString totalStr(aTotalTrackCountStr);

  PRInt32 padCount = 0;

  // Determine if padding is needed and how many zeros to pad with.
  if (format.Length() < totalStr.Length()) {
    padCount = totalStr.Length() - format.Length();
  }
  else if (totalStr.Length() == 0) {
    // Fallback and use at least a default of two (2) zeros if there isn't
    // any data for the total track count.
    padCount = 2 - format.Length();
  }

  if (padCount > 0) {
    for (PRInt32 i = 0; i < padCount; i++) {
      format.Insert(NS_LITERAL_STRING("0").get(), 0, 1);
    }
  }

  aOutString = format;
  return NS_OK;
}

/**
 * \brief Copys a file from the old location to the new location, ensures that
 * the destination is in the managed folder and that unique filenames are used
 * if the destination file already exists.
 * \param aMediaItem  - The media item we are organizing.
 * \param aSrcFile    - Original file for item.
 * \param aDestFile   - New file for item.
 * \param aRetVal     - True if successful.
 */
nsresult
sbMediaFileManager::CopyRename(sbIMediaItem *aMediaItem,
                               nsIFile      *aSrcFile,
                               nsIFile      *aDestFile,
                               PRBool       *aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aSrcFile);
  NS_ENSURE_ARG_POINTER(aDestFile);
  NS_ENSURE_ARG_POINTER(aRetVal);

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  nsresult rv;

  // First see if we have a management folder for this media item
  rv = CheckManagementFolder(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure the Src and Dest files are not equal
  PRBool isSrcDestSame = PR_FALSE;
  rv = aSrcFile->Equals(aDestFile, &isSrcDestSame);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isSrcDestSame) {
    return NS_ERROR_INVALID_ARG;
  }

  // Make sure the new file is under the managed folder
  nsCOMPtr<nsIFile> mediaFolder;
  rv = GetMediaFolder(aDestFile, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mediaFolder) {
    // aRetVal is false
    return NS_ERROR_INVALID_ARG;
  }

  // Create a Unique filename if the one we want already exists
  #if PR_LOGGING
  {
    nsString path;
    rv = aDestFile->GetPath(path);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get dest path");
    TRACE(("%s: creating file %s",
           __FUNCTION__,
           NS_ConvertUTF16toUTF8(path).get()));
  }
  #endif
  rv = aDestFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, PERMISSIONS_FILE);
  NS_ENSURE_SUCCESS(rv, rv);

  // We have to delete the newly created file since we are copying to it.
  rv = aDestFile->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  nsString newDestPath;
  rv = aDestFile->GetPath(newDestPath);
  if (NS_FAILED(rv)) {
    TRACE(("%s: Unable to get path we are copying to?", __FUNCTION__));
  } else {
    TRACE(("%s: Copying to %s", __FUNCTION__,
          NS_ConvertUTF16toUTF8(newDestPath).get()));
  }
#endif

  // Get the old path
  nsString oldPath;
  rv = aSrcFile->GetPath(oldPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the new path
  nsString newPath;
  rv = aDestFile->GetPath(newPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mWatchFolderService) {
    mWatchFolderService =
      do_GetService("@songbirdnest.com/watch-folder-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = mWatchFolderService->AddIgnoreCount(oldPath, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mWatchFolderService->AddIgnoreCount(newPath, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the new filename
  nsString newFilename;
  rv = aDestFile->GetLeafName(newFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  // Copy the old file to the new location with the new file name
  nsCOMPtr<nsIFile> newParentDir;
  rv = aDestFile->GetParent(getter_AddRefs(newParentDir));
  NS_ENSURE_SUCCESS(rv, rv);

  // If the original was in the managed folder do a move instead of a copy/delete
  mediaFolder.forget();
  rv = GetMediaFolder(aSrcFile, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mediaFolder) {
    // Copy since the original is not in the managed folder
    rv = aSrcFile->CopyTo(newParentDir, newFilename);
    NS_ENSURE_SUCCESS(rv, rv);
    // TODO: Do some checks to make sure we successfully copied the file.
  } else {
    // Move since the original is in the managed folder
    // First we need to create a copy of the original file object
    nsCOMPtr<nsIFile> holdOldFile;
    rv = aSrcFile->Clone(getter_AddRefs(holdOldFile));
    NS_ENSURE_SUCCESS(rv, rv);
    // Now move the file (aSrcFile will be changed to the new location)
    rv = aSrcFile->MoveTo(newParentDir, newFilename);
    NS_ENSURE_SUCCESS(rv, rv);
    // TODO: Do some checks to make sure we successfully moved the file.
    // Now check if we should remove the folder this item was in
    rv = CheckDirectoryForDeletion(holdOldFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Make a new nsIURI object pointing at the new file
  nsCOMPtr<nsIURI> newURI;
  rv = sbLibraryUtils::GetFileContentURI(aDestFile, getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // And send that URI back as the item contentsrc
  rv = aMediaItem->SetContentSrc(newURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Copying has successfully completed
  *aRetVal = PR_TRUE;

  return NS_OK;
}

/**
 * \brief Checks whether a directory and its parent directories need to be
 *  deleted.
 * \param aItemFile - The file we check the directories for.
 */
nsresult sbMediaFileManager::CheckDirectoryForDeletion(nsIFile *aItemFile)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aItemFile);
  nsresult rv;

  // Get the parent file object
  nsCOMPtr<nsIFile> parent;
  rv = aItemFile->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  // And check whether we need to remove it and its parent dirs (recursive call)
  rv = CheckDirectoryForDeletion_Recursive(parent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Checks a directory to see if it should be removed, it must have no
 *  files and be located in the Managed Folder.
 * \param aDirectory - The folder to check for deletion.
 */
nsresult
sbMediaFileManager::CheckDirectoryForDeletion_Recursive(nsIFile *aDirectory)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDirectory);
  nsresult rv;

  // Make sure this folder is under the managed folder
  nsCOMPtr<nsIFile> mediaFolder;
  rv = GetMediaFolder(aDirectory, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mediaFolder) {
    TRACE(("%s: Folder Not Managed", __FUNCTION__));
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
    TRACE(("%s: Folder Not Empty", __FUNCTION__));
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
 * \param aItemFile  - Items file to delete.
 * \param aRetVal    - True if successful.
 */
nsresult
sbMediaFileManager::Delete(nsIFile *aItemFile,
                           PRBool  *aRetVal)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aItemFile);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

  // Assume things won't go well so that we have the right value in aRetVal
  // if we except, regardless of any previous success
  *aRetVal = PR_FALSE;

  // Make sure this file is under the managed folder
  nsCOMPtr<nsIFile> mediaFolder;
  rv = GetMediaFolder(aItemFile, getter_AddRefs(mediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mediaFolder) {
    // aRetVal is false
    return NS_OK;
  }

  // Delete the file
  rv = aItemFile->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check whether we need to also delete the directory (and parent directories)
  rv = CheckDirectoryForDeletion(aItemFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Deletion happened successfully
  *aRetVal = PR_TRUE;

  return NS_OK;
}

