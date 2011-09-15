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
#include <sbIMediaFileManager.h>
#include <sbIPropertyManager.h>
#include <sbIWatchFolderService.h>

// Mozilla includes
#include <nsBaseHashtable.h>
#include <nsHashKeys.h>
#include <nsIFile.h>
#include <nsIPrefBranch.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsNetUtil.h>

#define SB_MEDIAFILEMANAGER_DESCRIPTION              \
  "Songbird Media File Manager Implementation"

#define SB_MEDIAFILEMANAGER_CID                      \
  {                                                        \
   0x7b901232,                                             \
   0x1dd2,                                                 \
   0x11b2,                                                 \
   {0x8d, 0x6a, 0xe1, 0x78, 0x4d, 0xbd, 0x2d, 0x89}        \
  }

// Preference keys
#define PREF_MFM_ROOT         "songbird.media_management.library."
#define PREF_MFM_FILEFORMAT   "format.file"
#define PREF_MFM_DEFPROPERTY  "default.property."
#define PREF_MFM_PADTRACKNUM  "pad_track_num"

// String keys
#define STRING_MFM_UNKNOWNPROP  "mediamanager.nonexistingproperty"
#define STRING_MFM_UNKNOWNPROP_EMPTY  "mediamanager.nonexistingproperty.empty"
#define STRING_MFM_RECORDINGS_FOLDER  "mediamanager.recordings_dir"

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
  typedef nsTArray<nsString>                  NameTemplate;
  typedef nsBaseHashtable<nsStringHashKey,
                          NameTemplate,
                          NameTemplate>       NameTemplateMap;
  typedef nsBaseHashtable<nsStringHashKey,
                          nsCOMPtr<nsIFile>,
                          nsIFile *>          MediaFoldersMap;

  nsresult InitMediaFoldersMap(nsIPropertyBag2 * aProperties);

  nsresult GetMediaFolder(sbIMediaItem * aMediaItem,
                          nsIFile **     aFolder);

  nsresult GetMediaFolder(nsIFile *   aFile,
                          nsIFile **  aFolder);

  nsresult InitFolderNameTemplates(nsIPropertyBag2 * aProperties);

  nsresult GetFolderNameTemplate(sbIMediaItem *   aMediaItem,
                                 NameTemplate &   aNameTemplate);

  nsresult CheckDirectoryForDeletion_Recursive(nsIFile *aDir);
  
  void     RemoveBadCharacters(nsString& aStringToParse);

  nsresult GetUnknownValue(nsString  aPropertyKey,
                           nsString& aUnknownValue);

  nsresult GetFormattedFileFolder(const NameTemplate &  aNameTemplate,
                                  sbIMediaItem*         aMediaItem,
                                  PRBool                aAppendProperty,
                                  PRBool                aTrimAtEnd,
                                  nsString              aFileExtension,
                                  nsString&             aRetVal);

  /**
   * \brief This function checks that the media folder exists.  This
   *   allows us to take removeable drives and network connections
   *   into account.
   * \param aMediaItem - used to select which media folder to check
   */
  nsresult CheckManagementFolder(sbIMediaItem * aMediaItem);

  // Hold on to the services we use very often
  nsCOMPtr<nsIPrefBranch>                   mPrefBranch;
  nsCOMPtr<nsINetUtil>                      mNetUtil;
  nsCOMPtr<sbIPropertyManager>              mPropertyManager;
  nsCOMPtr<sbIWatchFolderService>           mWatchFolderService;

  // Where our media folder is located.
  MediaFoldersMap                           mMediaFolders;

  // Formating properties (filename, folders, separators)
  NameTemplate                              mTrackNameTemplate;
  NameTemplateMap                           mFolderNameTemplates;
  
  PRBool                                    mInitialized;
};

