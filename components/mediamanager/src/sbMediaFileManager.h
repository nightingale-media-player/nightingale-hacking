/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

// Nightingale includes
#include <sbIMediaFileManager.h>
#include <sbIPropertyManager.h>
#include <sbIWatchFolderService.h>

// Mozilla includes
#include <nsIFile.h>
#include <nsIPrefBranch.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsNetUtil.h>

#define SB_MEDIAFILEMANAGER_DESCRIPTION              \
  "Nightingale Media File Manager Implementation"

#define SB_MEDIAFILEMANAGER_CID                      \
  {                                                        \
   0x7b901232,                                             \
   0x1dd2,                                                 \
   0x11b2,                                                 \
   {0x8d, 0x6a, 0xe1, 0x78, 0x4d, 0xbd, 0x2d, 0x89}        \
  }

// Preference keys
#define PREF_MFM_ROOT         "nightingale.media_management.library."
#define PREF_MFM_LOCATION     "nightingale.media_management.library.folder"
#define PREF_MFM_DIRFORMAT    "format.dir"
#define PREF_MFM_FILEFORMAT   "format.file"
#define PREF_MFM_DEFPROPERTY  "default.property."
#define PREF_MFM_PADTRACKNUM  "pad_track_num"

// String keys
#define STRING_MFM_UNKNOWNPROP  "mediamanager.nonexistingproperty"
#define STRING_MFM_UNKNOWNPROP_EMPTY  "mediamanager.nonexistingproperty.empty"

class sbMediaFileManager : public sbIMediaFileManager {
public:
  sbMediaFileManager();
  NS_METHOD Init();
  
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFILEMANAGER

protected:
  virtual ~sbMediaFileManager();
  
  nsresult GetNewPath(sbIMediaItem *aMediaItem,
                      nsString &aPath, 
                      PRBool *aRetVal);

  nsresult GetNewFilename(sbIMediaItem *aMediaItem, 
                          nsIURI *aItemUri,
                          nsString &aFilename, 
                          PRBool *aRetVal);

  nsresult CopyRename(sbIMediaItem *aMediaItem, 
                      nsIFile *aItemFile,
                      nsIFile *aNewFile,
                      PRBool *aRetVal);
  
  nsresult Delete(nsIFile *aItemFile, 
                  PRBool *aRetVal);
  
  nsresult CheckDirectoryForDeletion(nsIFile *aItemFile);
  
  nsresult NormalizeDir(nsString &aDir);

  nsresult ZeroPadTrackNumber(const nsAString & aTrackNumStr,
                              const nsAString & aTotalTrackCountStr,
                              nsString & aOutString);
  
private:
  
  nsresult CheckDirectoryForDeletion_Recursive(nsIFile *aDir);
  
  void     RemoveBadCharacters(nsString& aStringToParse);

  nsresult GetUnknownValue(nsString  aPropertyKey,
                           nsString& aUnknownValue);

  nsresult GetFormattedFileFolder(nsTArray<nsString>  aFormatSpec,
                                  sbIMediaItem*       aMediaItem,
                                  PRBool              aAppendProperty,
                                  PRBool              aTrimAtEnd,
                                  nsString            aFileExtension,
                                  nsString&           aRetVal);

  /**
   * \brief This function checks if we have stored the media folder location
   *   in mMediaFolder and if not gets it from the parameter or from the
   *   preferences. It also checks to ensure that the media folder exists, this
   *   allows us to take removeable drives and network connections into account.
   * \param aMediaFolder - Optional parameter to use as the media folder.
   * \note mMediaFolder will be set with this folder and checked for
   *   existance on successful return.
   */
  nsresult CheckManagementFolder(nsIFile * aMediaFolder);

  // Hold on to the services we use very often
  nsCOMPtr<nsIPrefBranch>                   mPrefBranch;
  nsCOMPtr<nsINetUtil>                      mNetUtil;
  nsCOMPtr<sbIPropertyManager>              mPropertyManager;
  nsCOMPtr<sbIWatchFolderService>           mWatchFolderService;

  // Where our media folder is located.
  nsCOMPtr<nsIFile>                         mMediaFolder;

  // Formating properties (filename, folders, separators)
  nsTArray<nsString>                        mTrackNameConfig;
  nsTArray<nsString>                        mFolderNameConfig;
  
  PRBool                                    mInitialized;
};

