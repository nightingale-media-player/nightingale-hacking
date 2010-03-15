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

#ifndef SBDEVICEIMAGES_H_
#define SBDEVICEIMAGES_H_

// Standard includes
#include <set>

// Mozilla includes
#include <nsIArray.h>

// Songbird local includes
#include "sbBaseDevice.h"
#include "sbIDeviceImage.h"

struct sbBaseDevice::TransferRequest;
class sbIFileScanQuery;

class sbDeviceImages
{
public:
  friend class sbBaseDevice;

  // Compute the difference between the images present locally and
  // those provided in the device image array. You must provide a list
  // of supported extensions. Upon return, the copy and delete arrays are
  // filled with sbIDeviceImage items which the device implementation should
  // act upon.
  nsresult ComputeImageSyncArrays(sbIDeviceLibrary *aLibrary,
                                  nsIArray *aDeviceImageArray,
                                  const nsTArray<nsString> &aFileExtensionList,
                                  nsIArray **retCopyArray,
                                  nsIArray **retDeleteArray);

  // This may be called by devices whose underlying storage lies on the
  // filesystem to build an array of sbIDeviceImage items. aScanPath is the
  // directory that is to be scanned. aBasePath is the base of the image
  // directory on that filesystem. You must also provide a list of supported
  // extensions.
  nsresult ScanImages(nsIFile *aScanDir,
                      nsIFile *aBaseDir,
                      const nsTArray<nsString> &aFileExtensionList,
                      PRBool recursive,
                      nsIArray **retImageArray);

  // Create and return in aMediaItem a temporary media item for the local file
  // represented by aImage.
  nsresult CreateTemporaryLocalMediaItem(sbIDeviceImage* aImage,
                                         sbIMediaItem** aMediaItem);

  // Create an nsIFile for an sbIDeviceImage item given the base image sync
  // directory. The function can automatically create the needed subdirectories
  // in which the file will reside, or to create a file object pointing at the
  // file's parent directory rather than at the file itself.
  nsresult MakeFile(sbIDeviceImage* aImage,
                    nsIFile*        aBaseDir,
                    PRBool          aWithFilename,
                    PRBool          aCreateDirectories,
                    nsIFile**       retFile);

private:
  // ctor
  sbDeviceImages(sbBaseDevice * aBaseDevice);

  // add local images to an existing array, this is used to sequentially
  // build the list of all local files that should be synced.
  nsresult
    AddLocalImages(nsIFile *baseDir,
                   nsIFile *subDir,
                   const nsTArray<nsString> aFileExtensionList,
                   PRBool recursive,
                   nsIMutableArray *localImageArray);

  // search for each item that is in searchItem in the searchableImageArray.
  // if an item does not exist, it is inserted in the diffResultsArray
  nsresult DiffImages(nsIMutableArray *diffResultsArray,
                      nsTArray< sbIDeviceImage* > &searchableImageArray,
                      nsIArray *searchItems);

  // perform the actual image scanning of a directory using a filescan object
  nsresult ScanForImageFiles(nsIURI *aImageFilesPath,
                             const nsTArray<nsString> &aFileExtensionList,
                             PRBool recursive,
                             sbIFileScanQuery** aFileScanQuery);

  // A pointer to our parent base device
  sbBaseDevice        *mBaseDevice;
};

// sbIDeviceImage implementation
class sbDeviceImage : public sbIDeviceImage {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEIMAGE

  sbDeviceImage();
  virtual ~sbDeviceImage() {}

private:
  PRUint64   mSize;
  nsString   mFilename;
  nsString   mSubdirectory;
};

// a comparator class for sbIDeviceImages so that we can efficiently search
// through a sorted list.
class sbDeviceImageComparator {
public:
  // Defined for the Sort function on nsTArray
  PRBool LessThan(const sbIDeviceImage *a, const sbIDeviceImage *b) const;
  // This has to be defined for Sort as well, but is used for Searching.
  PRBool Equals(const sbIDeviceImage *a, const sbIDeviceImage *b) const;
};

#endif
