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

#include <sbIWatchFolderService.h>
#include <sbIFileSystemWatcher.h>
#include <sbIFileSystemListener.h>
#include <sbILibrary.h>
#include <nsITimer.h>
#include <nsIObserver.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIFile.h>
#include <nsTArray.h>
#include <nsIMutableArray.h>
#include <nsISupportsPrimitives.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsIIOService.h>
#include <sbILibraryUtils.h>
#include <sbIMediaListListener.h>
#include <nsIDOMWindow.h>
#include <vector>

typedef std::vector<nsString> sbStringVector;

typedef enum {
  eNone  = 0,
  eRemoval = 1,
  eChanged = 2,
} EProcessType;


class sbWatchFolderService : public sbIWatchFolderService,
                             public sbIFileSystemListener,
                             public sbIMediaListEnumerationListener,
                             public nsITimerCallback,
                             public nsIObserver
{
public:
  sbWatchFolderService();
  virtual ~sbWatchFolderService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWATCHFOLDERSERVICE
  NS_DECL_SBIFILESYSTEMLISTENER
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIOBSERVER

  nsresult Init();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

protected:
  nsresult StartWatching();
  nsresult StopWatching();
  nsresult SetEventPumpTimer();
  nsresult ProcessEventPaths(sbStringVector & aEventPathVector,
                             EProcessType aProcessType);
  nsresult ProcessAddedPaths();
  nsresult EnumerateItemsByPaths(sbStringVector & aPathVector);
  nsresult GetFilePathURI(const nsAString & aFilePath, nsIURI **aURIRetVal);
  nsresult GetSongbirdWindow(nsIDOMWindow **aSongbirdWindow);

private:
  nsCOMPtr<sbIFileSystemWatcher> mFileSystemWatcher;
  nsCOMPtr<sbILibrary>           mMainLibrary;
  nsCOMPtr<sbILibraryUtils>      mLibraryUtils;
  nsCOMPtr<nsITimer>             mEventPumpTimer;
  nsCOMPtr<nsITimer>             mAddDelayTimer;
  nsCOMPtr<nsITimer>             mChangeDelayTimer;
  nsCOMPtr<nsIMutableArray>      mEnumeratedMediaItems;
  sbStringVector                 mChangedPaths;
  sbStringVector                 mDelayedChangedPaths;
  sbStringVector                 mAddedPaths;
  sbStringVector                 mRemovedPaths;
  nsString                       mWatchPath;
  PRBool                         mIsSupported;
  PRBool                         mIsEnabled;
  PRBool                         mIsWatching;
  PRBool                         mEventPumpTimerIsSet;
  PRBool                         mAddDelayTimerIsSet;
  PRBool                         mChangeDelayTimerIsSet;
  PRBool                         mShouldProcessAddedPaths;
  EProcessType                   mCurrentProcessType;
};


#endif  // sbWatchFolderService_h_

