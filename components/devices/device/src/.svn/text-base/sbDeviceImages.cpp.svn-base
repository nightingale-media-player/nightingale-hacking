/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#include <nsArrayUtils.h>
#include <sbIFileScan.h>
#include <nsIFileURL.h>
#include <nsNetUtil.h>
#include <nsCRT.h>
#include <nsUnicharUtils.h>

#include "sbBaseDevice.h"
#include "sbDeviceImages.h"
#include "sbDeviceUtils.h"
#include <sbFileUtils.h>
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbStringUtils.h>

nsresult
sbDeviceImages::GetImagesRootFolder(sbIDeviceLibrary * aDevLib, nsIFile ** aFile)
{
  NS_ASSERTION(aDevLib, "aDevLib is null");
  NS_ASSERTION(aFile, "aFile is null");

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  nsresult rv = aDevLib->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_IMAGE,
                                      getter_AddRefs(mediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString syncFromFolder;
  nsCOMPtr<nsIFile> syncFromFile;
  rv = mediaSyncSettings->GetSyncFromFolder(getter_AddRefs(syncFromFile));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!syncFromFile)
    return NS_ERROR_NOT_AVAILABLE;

  rv = syncFromFile->GetPath(syncFromFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  if (syncFromFolder.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsILocalFile> baseDir;
  rv = NS_NewLocalFile(syncFromFolder, PR_TRUE, getter_AddRefs(baseDir));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(baseDir, aFile);
}

sbDeviceImages::sbDeviceImages(sbBaseDevice * aBaseDevice) :
  mBaseDevice(aBaseDevice)
{
}

// Compute the difference between the images present locally and
// those provided in the device image array. You must provide a list
// of supported extensions. Upon return, the copy array is filled with
// sbIDeviceImage items which the device implementation should act upon.
nsresult
sbDeviceImages::ComputeImageSyncArray
                  (sbIDeviceLibrary *aLibrary,
                   nsIArray *aDeviceImageArray,
                   const nsTArray<nsString> &aFileExtensionList,
                   nsIArray **retCopyArray)
{
  NS_ENSURE_ARG_POINTER(retCopyArray);
  nsresult rv;

  nsCOMPtr<nsIFile> baseDir;
  rv = GetImagesRootFolder(aLibrary, getter_AddRefs(baseDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> subDirs;
  rv = aLibrary->GetSyncFolderListByType(sbIDeviceLibrary::MEDIATYPE_IMAGE,
                                         getter_AddRefs(subDirs));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the array of files to copy
  nsCOMPtr<nsIMutableArray> syncCopyArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // turn the device image array into a binary searchable array
  PRUint32 len;
  rv = aDeviceImageArray->GetLength(&len);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray< sbIDeviceImage* > searchableDeviceImageArray;
  for (PRUint32 i=0; i<len; i++) {
    nsCOMPtr<sbIDeviceImage> image =
      do_QueryElementAt(aDeviceImageArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    searchableDeviceImageArray.AppendElement(image);
  }
  sbDeviceImageComparator comp;
  searchableDeviceImageArray.Sort(comp);

  // Create the array of local files
  nsCOMPtr<nsIMutableArray> localImagesArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Scan local files
  PRUint32 numEntries;
  rv = subDirs->GetLength(&numEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i=0; i<numEntries; i++) {
    nsCOMPtr<nsIFile> subDir =
      do_QueryElementAt(subDirs, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddLocalImages(baseDir,
                        subDir,
                        aFileExtensionList,
                        PR_TRUE,
                        localImagesArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // add any local image missing from the device to the copy array
  DiffImages(syncCopyArray, searchableDeviceImageArray, localImagesArray);

  // return sync array
  rv = CallQueryInterface(syncCopyArray, retCopyArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// This may be called by devices whose underlying storage lies on the
// filesystem to build an array of sbIDeviceImage items. aScanPath is the
// directory that is to be scanned. aBasePath is the base of the image
// directory on that filesystem. You must also provide a list of supported
// extensions.
nsresult
sbDeviceImages::ScanImages(nsIFile *aScanDir,
                           nsIFile *aBaseDir,
                           const nsTArray<nsString> &aFileExtensionList,
                           PRBool recursive,
                           nsIArray **retImageArray) {
  nsresult rv;

  // turn the scan files into a uri
  nsCOMPtr<nsIURI> scanDirUri;
  rv = NS_NewFileURI(getter_AddRefs(scanDirUri), aScanDir);
  NS_ENSURE_SUCCESS(rv, rv);

  // Scan for all media files in image directory.
  nsCOMPtr<sbIFileScanQuery> fileScanQuery;
  rv = ScanForImageFiles(scanDirUri,
                         aFileExtensionList,
                         recursive,
                         getter_AddRefs(fileScanQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the number of scanned files.
  PRUint32 fileCount;
  rv = fileScanQuery->GetFileCount(&fileCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the base paths
  nsString basePath;
  rv = aBaseDir->GetPath(basePath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a list of sbIDeviceImage found at the scan location
  nsCOMPtr<nsIMutableArray> imageArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < fileCount; i++) {
    // Check for abort.
    NS_ENSURE_FALSE(mBaseDevice->IsRequestAborted(), NS_ERROR_ABORT);

    // Get the next file URI spec.
    nsAutoString fileURISpec;
    rv = fileScanQuery->GetFilePath(i, fileURISpec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the file content type.
    PRUint32 contentType = sbIDeviceCapabilities::CONTENT_UNKNOWN;
    sbExtensionToContentFormatEntry_t formatType;
    rv = sbDeviceUtils::GetFormatTypeForURL(fileURISpec, formatType);
    if (NS_SUCCEEDED(rv))
      contentType = formatType.ContentType;

    // Check that the file is really an image
    if (contentType == sbIDeviceCapabilities::CONTENT_IMAGE) {
      // Create a file URI.
      nsCOMPtr<nsIURI> fileURI;
      rv = NS_NewURI(getter_AddRefs(fileURI),
                     NS_ConvertUTF16toUTF8(fileURISpec).get());
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // Create an nsIFile
      nsCOMPtr<nsIFile> file;
      rv = fileURL->GetFile(getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the file's parent directory
      nsCOMPtr<nsIFile> parent;
      rv = file->GetParent(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the local path
      nsString parentPath;
      rv = parent->GetPath(parentPath);
      NS_ENSURE_SUCCESS(rv, rv);

      // Subtract the image folder from the file's path
      NS_ENSURE_TRUE(parentPath.Length() >= basePath.Length(),
                     NS_ERROR_UNEXPECTED);
      nsString partialPath;
      partialPath = parentPath.BeginReading() + basePath.Length();

      // Eat the first path separator if there's one
      if (partialPath.First() == *FILE_PATH_SEPARATOR)
        partialPath = partialPath.BeginReading() + 1;

      // Get the file name
      nsString filename;
      rv = file->GetLeafName(filename);
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the file size
      PRInt64 size;
      rv = file->GetFileSize(&size);
      NS_ENSURE_SUCCESS(rv, rv);

      // Make a new sbIDeviceImage entry
      nsCOMPtr<sbIDeviceImage> image = new sbDeviceImage();
      image->SetFilename(filename);
      image->SetSubdirectory(partialPath);
      image->SetSize(size);

      // Add the image to the list.
      rv = imageArray->AppendElement(image, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }

  // Return the array of images that were found
  return CallQueryInterface(imageArray, retImageArray);
}

// Create and return in aMediaItem a temporary media item for the local file
// represented by aImage and device library aLibrary.
nsresult
sbDeviceImages::CreateTemporaryLocalMediaItem(sbIDeviceImage*   aImage,
                                              sbIDeviceLibrary* aLibrary,
                                              sbIMediaItem**    aMediaItem)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Get the local image base directory.
  nsCOMPtr<nsIFile> baseDir;
  rv = GetImagesRootFolder(aLibrary, getter_AddRefs(baseDir));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(baseDir, NS_ERROR_UNEXPECTED);

  // Get the local device image file URI.
  nsCOMPtr<nsIURI>  imageURI;
  nsCOMPtr<nsIFile> imageFile;
  rv = MakeFile(aImage, baseDir, PR_TRUE, PR_FALSE, getter_AddRefs(imageFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NS_NewFileURI(getter_AddRefs(imageURI), imageFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a temporary media item.
  nsCOMPtr<sbIMediaItem>
    item = do_CreateInstance
             ("@songbirdnest.com/Songbird/Library/TemporaryMediaItem;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the temporary media item.
  rv = item->SetContentType(NS_LITERAL_STRING("image"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetContentSrc(imageURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  item.forget(aMediaItem);

  return NS_OK;
}

// Create an nsIFile for an sbIDeviceImage item given the base image sync
// directory. The function can automatically create the needed subdirectories
// in which the file will reside, or to create a file object pointing at the
// file's parent directory rather than at the file itself.
nsresult sbDeviceImages::MakeFile(sbIDeviceImage* aImage,
                                  nsIFile*        aBaseDir,
                                  PRBool          aWithFilename,
                                  PRBool          aCreateDirectories,
                                  nsIFile**       retFile)
{
  NS_ENSURE_ARG_POINTER(retFile);
  NS_ENSURE_ARG_POINTER(aBaseDir);

  nsresult rv;

  // Create a local file object to point at the source file
  nsCOMPtr<nsIFile> file;
  rv = aBaseDir->Clone(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the image subdirectories
  nsString subDirectories;
  rv = aImage->GetSubdirectory(subDirectories);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add subdirectories to the file object, one by one, optionally making sure
  // that they each exist
  if (!subDirectories.IsEmpty()) {
    nsTArray<nsString> subDirectoryList;
    nsString_Split(subDirectories,
                   NS_LITERAL_STRING(FILE_PATH_SEPARATOR),
                   subDirectoryList);
    for (PRUint32 i = 0; i < subDirectoryList.Length(); ++i) {
      const nsString& subDirectory = subDirectoryList[i];
      if (!subDirectory.IsEmpty()) {
        // Append the sub-directory to the file object
        rv = file->Append(subDirectory);
        NS_ENSURE_SUCCESS(rv, rv);
        // If needed, create the directory
        if (aCreateDirectories) {
          PRBool exists;
          rv = file->Exists(&exists);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!exists) {
            rv = file->Create(nsIFile::DIRECTORY_TYPE,
                              SB_DEFAULT_DIRECTORY_PERMISSIONS);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }
    }
  }

  // If needed, add the filename to the file object
  if (aWithFilename) {
    nsString filename;
    rv = aImage->GetFilename(filename);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = file->Append(filename);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  file.forget(retFile);
  return NS_OK;
}

// ----------------------------------------------------------------------------

// add local images to an existing array, this is used to sequentially
// build the list of all local files that should be synced.
nsresult
sbDeviceImages::
  AddLocalImages(nsIFile *baseDir,
                 nsIFile *scanDir,
                 const nsTArray<nsString> aFileExtensionList,
                 PRBool recursive,
                 nsIMutableArray *localImageArray)
{
  nsresult rv;

  // Scan for images at that location
  nsCOMPtr<nsIArray> items;
  rv = ScanImages(scanDir,
                  baseDir,
                  aFileExtensionList,
                  recursive,
                  getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add all elements to the main array
  PRUint32 len;
  rv = items->GetLength(&len);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i=0;i<len;i++) {
    nsCOMPtr<sbIDeviceImage> image = do_QueryElementAt(items, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    localImageArray->AppendElement(image, PR_FALSE);
  }

  return NS_OK;
}

// search for each item that is in searchItem in the searchableImageArray.
// if an item does not exist, it is inserted in the diffResultsArray
nsresult
sbDeviceImages::DiffImages(nsIMutableArray *diffResultsArray,
                           nsTArray< sbIDeviceImage* > &searchableImageArray,
                           nsIArray *searchItems)
{
  PRUint32 len;
  nsresult rv = searchItems->GetLength(&len);
  NS_ENSURE_SUCCESS(rv, rv);

  // a comparator object for binary search
  sbDeviceImageComparator comp;
  for (PRUint32 i=0;i<len;i++) {
    // Get the nth item
    nsCOMPtr<sbIDeviceImage> image = do_QueryElementAt(searchItems, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // Look for that item in the searchable array
    if (searchableImageArray.BinaryIndexOf(image, comp) ==
        searchableImageArray.NoIndex) {
      // If the item was not found, add it to the diff array
      diffResultsArray->AppendElement(image, PR_FALSE);
    }
  }
  return NS_OK;
}


// perform the actual image scanning of a directory using a filescan object
nsresult
sbDeviceImages::ScanForImageFiles(nsIURI *aImageFilesPath,
                                  const nsTArray<nsString> &aFileExtensionList,
                                  PRBool recursive,
                                  sbIFileScanQuery** aFileScanQuery)
{
  NS_ENSURE_ARG_POINTER(aFileScanQuery);

  nsresult rv;

  // Create a file scan query object to search the device.
  nsCOMPtr<sbIFileScanQuery> fileScanQuery =
    do_CreateInstance("@songbirdnest.com/Songbird/FileScanQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> _url = do_QueryInterface(aImageFilesPath, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> _file;
  rv = _url->GetFile(getter_AddRefs(_file));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString path;
  rv = _file->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileScanQuery->SetDirectory(path);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileScanQuery->SetRecurse(recursive);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileScanQuery->SetSearchHidden(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileScanQuery->SetWantLibraryContentURIs(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device capabilities.
  nsCOMPtr<sbIDeviceCapabilities> caps;
  rv = mBaseDevice->GetCapabilities(getter_AddRefs(caps));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the file extensions to the file scan query.
  for (PRUint32 i = 0; i < aFileExtensionList.Length(); ++i) {
    rv = fileScanQuery->AddFileExtension(aFileExtensionList[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Create a file scan engine and submit the query.
  nsCOMPtr<sbIFileScan>
    fileScan = do_CreateInstance("@songbirdnest.com/Songbird/FileScan;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileScan->SubmitQuery(fileScanQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Wait until the file scan query completes.  Poll instead of using callbacks
  // because individual item callbacks are not needed and would reduce
  // efficiency.
  PRBool isScanning = PR_TRUE;
  while (isScanning) {
    // Check for abort.
    NS_ENSURE_FALSE(mBaseDevice->IsRequestAborted(), NS_ERROR_ABORT);

    // Check if the file scan query is still scanning.
    rv = fileScanQuery->IsScanning(&isScanning);
    NS_ENSURE_SUCCESS(rv, rv);

    // If still scanning, sleep a bit and check again.
    if (isScanning)
      PR_Sleep(PR_MillisecondsToInterval(100));
  }

  // Return results.
  fileScanQuery.forget(aFileScanQuery);

  return NS_OK;
}


// ----------------------------------------------------------------------------

// sbDeviceImage, implements sbIDeviceImage, holds information about a device
// image: filename, subdirectory below the base image directory (be it on the
// actual device, or the sync source directory), and file size.

NS_IMPL_ISUPPORTS1(sbDeviceImage,
                   sbIDeviceImage);

sbDeviceImage::sbDeviceImage() :
  mSize(0)
{
}

// Get the file size in bytes
NS_IMETHODIMP sbDeviceImage::GetSize(PRUint64 *aSizePtr)
{
  NS_ENSURE_ARG_POINTER(aSizePtr);
  *aSizePtr = mSize;
  return NS_OK;
}

// Set the file size in bytes
NS_IMETHODIMP sbDeviceImage::SetSize(PRUint64 aSize)
{
  mSize = aSize;
  return NS_OK;
}
// Get the filename
NS_IMETHODIMP sbDeviceImage::GetFilename(nsAString &aFilename)
{
  aFilename = mFilename;
  return NS_OK;
}

// Set the filename
NS_IMETHODIMP sbDeviceImage::SetFilename(const nsAString &aFilename)
{
  mFilename = aFilename;
  return NS_OK;
}

// Get the subdirectory. This directory is relative to either the local
// sync directory selected by the user, or the device image directory
NS_IMETHODIMP sbDeviceImage::GetSubdirectory(nsAString &aSubdirectory)
{
  aSubdirectory = mSubdirectory;
  return NS_OK;
}

// Set the subdirectory
NS_IMETHODIMP sbDeviceImage::SetSubdirectory(const nsAString &aSubdirectory)
{
  mSubdirectory = aSubdirectory;
  return NS_OK;
}

// Sorting operator.
PRBool sbDeviceImageComparator::LessThan(const sbIDeviceImage *a, const sbIDeviceImage *b) const
{
  nsString a_subdir;
  nsString b_subdir;
  const_cast<sbIDeviceImage *>(a)->GetSubdirectory(a_subdir);
  const_cast<sbIDeviceImage *>(b)->GetSubdirectory(b_subdir);
  PRInt32 comp = a_subdir.Compare(b_subdir, CaseInsensitiveCompare);
  if (comp < 0) {
    return PR_TRUE;
  } else if (comp > 0) {
    return PR_FALSE;
  }
  nsString a_filename;
  nsString b_filename;
  const_cast<sbIDeviceImage *>(a)->GetFilename(a_filename);
  const_cast<sbIDeviceImage *>(b)->GetFilename(b_filename);
  return a_filename.Compare(b_filename, CaseInsensitiveCompare) < 0;
}

// Sorting operator.
PRBool sbDeviceImageComparator::Equals(const sbIDeviceImage *a, const sbIDeviceImage *b) const
{
  nsString a_subdir;
  nsString b_subdir;
  ((sbIDeviceImage *)a)->GetSubdirectory(a_subdir);
  ((sbIDeviceImage *)b)->GetSubdirectory(b_subdir);
  if (!a_subdir.Equals(b_subdir, CaseInsensitiveCompare)) {
    return PR_FALSE;
  }
  nsString a_filename;
  nsString b_filename;
  ((sbIDeviceImage *)a)->GetFilename(a_filename);
  ((sbIDeviceImage *)b)->GetFilename(b_filename);
  return a_filename.Equals(b_filename, CaseInsensitiveCompare);
}
