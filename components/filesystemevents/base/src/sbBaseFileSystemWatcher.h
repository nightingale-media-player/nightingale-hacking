/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbBaseFileSystemWatcher_h_
#define sbBaseFileSystemWatcher_h_

#include <sbIFileSystemWatcher.h>
#include <sbIFileSystemListener.h>
#include "sbFileSystemTree.h"
#include "sbFileSystemTreeListener.h"
#include <nsStringAPI.h>
#include <nsCOMPtr.h>


//
// \brief Base class for the file system watcher. Handles the listener,
//        watch path, and watch state.
//
class sbBaseFileSystemWatcher : public sbIFileSystemWatcher,
                                public sbFileSystemTreeListener
{
public:
  sbBaseFileSystemWatcher();
  virtual ~sbBaseFileSystemWatcher();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILESYSTEMWATCHER

  // sbFileSystemTreeListener
  NS_IMETHOD OnChangeFound(const nsAString & aChangePath, 
                           EChangeType aChangeType);
  NS_IMETHOD OnTreeReady(const nsAString & aTreeRootPath,
                         sbStringArray & aDirPathArray);
  NS_IMETHOD OnRootPathMissing();
  NS_IMETHOD OnTreeSessionLoadError();

protected:
  nsRefPtr<sbFileSystemTree>      mTree;
  nsCOMPtr<sbIFileSystemListener> mListener;
  nsString                        mWatchPath;
  nsID                            mSessionID;
  PRBool                          mIsRecursive;
  PRBool                          mIsWatching;
  PRBool                          mIsSupported;
  PRBool                          mShouldLoadSession;
};

#endif  // sbBaseFileSystemWatcher_h_

