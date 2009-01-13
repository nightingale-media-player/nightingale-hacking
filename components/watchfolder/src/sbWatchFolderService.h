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
#include <sbIMediaListListener.h>
#include <queue>

typedef std::queue<nsString> sbStringQueue;


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
  nsresult SetTimer();
  nsresult ProcessChangedPaths();
  nsresult ProcessAddedPaths();
  nsresult ProcessRemovedPaths();
  void ClearQueue(sbStringQueue & aStringQueue);

private:
  nsCOMPtr<sbIFileSystemWatcher> mFileSystemWatcher;
  nsCOMPtr<sbILibrary>           mMainLibrary;
  nsCOMPtr<nsIIOService>         mIOService;
  nsCOMPtr<nsITimer>             mTimer;
  nsCOMPtr<nsIMutableArray>      mEnumeratedMediaItems;
  sbStringQueue                  mChangedPathsQueue;
  sbStringQueue                  mAddedPathsQueue;
  sbStringQueue                  mRemovedPathsQueue;
  nsString                       mWatchPath;
  PRBool                         mIsEnabled;
  PRBool                         mIsWatching;
  PRBool                         mTimerIsSet;
};


#endif  // sbWatchFolderService_h_

