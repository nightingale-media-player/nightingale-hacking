/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbWatchFolderService_h_
#define sbWatchFolderService_h_

#include "sbWatchFolderPrefMgr.h"

#include <sbIWatchFolderService.h>
#include <sbIFileSystemWatcher.h>
#include <sbIFileSystemListener.h>
#include <sbILibrary.h>
#include <sbIJobProgress.h>
#include <sbILibraryUtils.h>
#include <sbIMediaListListener.h>

#include <nsITimer.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>
#include <nsTArray.h>
#include <nsIMutableArray.h>
#include <nsISupportsPrimitives.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsIIOService.h>
#include <nsIDOMWindow.h>
#include <nsUnicharUtils.h>

#include <set>
#include <map>


//==============================================================================
//
// @brief Ignore case comparision function for the string maps.
//
//==============================================================================

struct sbStringIgnoreCaseCompare
{
  bool operator() (const nsAString & s1, const nsAString & s2) const
  {
#if defined(XP_WIN)
    return s1.Compare(s2, CaseInsensitiveCompare) < 0;
#else
    return s1.Compare(s2) < 0;
#endif
  }
};

//==============================================================================
//
// @interface sbWatchFolderService
// @brief Songbird watch folder implementation class.
//
//==============================================================================

class sbWatchFolderService : public sbIWatchFolderService,
                             public sbIFileSystemListener,
                             public sbIMediaListEnumerationListener,
                             public nsITimerCallback,
                             public sbIJobProgressListener
{
  friend class sbWatchFolderPrefMgr;

public:
  sbWatchFolderService();
  virtual ~sbWatchFolderService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWATCHFOLDERSERVICE
  NS_DECL_SBIFILESYSTEMLISTENER
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_SBIJOBPROGRESSLISTENER

  nsresult Init();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

protected:
  typedef std::set<nsString, sbStringIgnoreCaseCompare> sbStringSet;
  typedef sbStringSet::iterator sbStringSetIter;
  struct ignorePathData_t {
    PRInt32 depth; // number of calls to addIgnorePath()
    PRInt32 count; // number from addIgnoreCount()
    ignorePathData_t() : depth(0), count(0) {}
    ignorePathData_t(PRInt32 aDepth, PRInt32 aCount)
      : depth(aDepth), count(aCount) {}
  };
  typedef std::map<nsString, ignorePathData_t, sbStringIgnoreCaseCompare> sbStringMap;

  typedef enum {
    eNone  = 0,
    eRemoval = 1,
    eChanged = 2,
    eMoveOrRename = 3,
  } EProcessType;

  typedef enum {
    eNotSupported = 0,  // Service is not supported on current platform
    eDisabled     = 1,  // Service is disabled (by user pref)
    eStarted      = 2,  // Service has initialized internally, not watching yet.
    eWatching     = 3,  // Service is now watching changes
  } EWatchFolderState;

protected:
  //
  // \brief Internal delayed startup handling method.
  //
  nsresult InitInternal();

  //
  // \brief Handles starting up the file system watcher, timers, and other
  //        member resources used to track changes.
  //
  nsresult StartWatchingFolder();

  //
  // \brief Handle shutting down the file system watcher, timer, and other
  //        member resources that are being used to track changes.
  //
  nsresult StopWatchingFolder();

  //
  // \brief Set the startup delay timer. This method is used to postpone
  //        watchfolder bootstrapping.
  //
  nsresult SetStartupDelayTimer();

  //
  // \brief Set the standard event pump timer. This method should be called
  //        everytime a event is received.
  //        NOTE: This method will not init the event pump timer if the
  //              |sbIFileSystemWatcher| has not started watching.
  //
  nsresult SetEventPumpTimer();

  //
  // \brief This method will handle processing all the normal events (i.e. not
  //        delayed events).
  //
  nsresult ProcessEventPaths();

  //
  // \brief Handle a set of changed paths for changed and removed items. This
  //        method will look up media items by contentURL.
  //
  nsresult HandleEventPathList(sbStringSet & aEventPathSet,
                               EProcessType aProcessType);

  //
  // \brief Handle the set of added paths to the library.
  //
  nsresult ProcessAddedPaths();

  //
  // \brief Enumerate media items in the library with the given paths.
  //
  nsresult EnumerateItemsByPaths(sbStringSet & aPathSet);

  //
  // \brief Get an array of media item URIs from a list of string paths
  //
  nsresult GetURIArrayForStringPaths(sbStringSet & aPathsSet, nsIArray **aURIs);

  //
  // \brief Get a |nsIURI| instance for a given absolute path.
  //
  nsresult GetFilePathURI(const nsAString & aFilePath, nsIURI **aURIRetVal);

  //
  // \brief Get the main Songbird window.
  //
  nsresult GetSongbirdWindow(nsIDOMWindow **aSongbirdWindow);

  //
  // \brief Handle the situation where the watcher fails to load a saved
  //        session.
  //
  nsresult HandleSessionLoadError();

  //
  // \brief Handle the situation where the watcher reports that the root watch
  //        folder path is missing.
  //
  nsresult HandleRootPathMissing();

  //
  // \brief Check to see if a given file path is on the ignored paths list.
  //        If found, decrements the times-to-ignore count as appropriate.
  //
  nsresult DecrementIgnoredPathCount(const nsAString & aFilePath,
                                     PRBool *aIsIgnoredPath);

  //----------------------------------------------------------------------------
  // Pref Changes

  //
  // \brief Callback method when the app has started up and the watch folder
  //        service should startup.
  //
  nsresult OnAppStartup();

  //
  // \brief Callback method when the app is about to shutdown and the watch
  //        folder service should shutdown.
  //
  nsresult OnAppShutdown();

  //
  // \brief Callback method when the watchfolder path changes.
  //
  nsresult OnWatchFolderPathChanged(const nsAString & aNewPath);

  //
  // \brief Callback method when the watch folder enable pref has changed.
  //
  nsresult OnEnableWatchFolderChanged(PRBool aShouldEnable);

private:
  nsCOMPtr<sbIFileSystemWatcher> mFileSystemWatcher;
  nsCOMPtr<sbILibrary>           mMainLibrary;
  nsCOMPtr<sbILibraryUtils>      mLibraryUtils;
  nsCOMPtr<nsITimer>             mEventPumpTimer;
  nsCOMPtr<nsITimer>             mChangeDelayTimer;
  nsCOMPtr<nsITimer>             mStartupDelayTimer;
  nsCOMPtr<nsITimer>             mFlushFSWatcherTimer;
  nsCOMPtr<nsIMutableArray>      mEnumeratedMediaItems;
  nsRefPtr<sbWatchFolderPrefMgr> mPrefMgr;
  sbStringSet                    mChangedPaths;
  sbStringSet                    mDelayedChangedPaths;
  sbStringSet                    mAddedPaths;
  sbStringSet                    mRemovedPaths;
  sbStringMap                    mIgnorePaths;
  nsString                       mWatchPath;
  nsCString                      mFileSystemWatcherGUID;
  EWatchFolderState              mServiceState;
  PRBool                         mHasWatcherStarted;
  PRBool                         mShouldReinitWatcher;
  PRBool                         mEventPumpTimerIsSet;
  PRBool                         mShouldProcessEvents;
  PRBool                         mChangeDelayTimerIsSet;
  EProcessType                   mCurrentProcessType;
};

#endif  // sbWatchFolderService_h_
