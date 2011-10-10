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

#ifndef sbLinuxFileSystemWatcher_h_
#define sbLinuxFileSystemWatcher_h_

#include <sbBaseFileSystemWatcher.h>
#include <sbIFileSystemWatcher.h>
#include <sbFileSystemTree.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <map>
#include <glib.h>

typedef std::map<int, nsString> sbFileDescMap;


class sbLinuxFileSystemWatcher : public sbBaseFileSystemWatcher
{
public:
  sbLinuxFileSystemWatcher();
  virtual ~sbLinuxFileSystemWatcher();

  NS_IMETHOD StopWatching(PRBool aShouldSaveSession);

  nsresult OnInotifyEvent();
  
  // |sbFileSystemTreeListener|
  NS_IMETHOD OnChangeFound(const nsAString & aChangePath,
                           EChangeType aChangeType);
  NS_IMETHOD OnTreeReady(const nsAString & aTreeRootPath,
                         sbStringArray & aDirPathArray);

protected:
  //
  // \brief Close and cleanup all the inotify and file-system data structures.
  //
  nsresult Cleanup();
  
  //
  // \brief Add a inotify resource to watch. This function will also add the
  //        necessary entry to the file descriptor map.
  // \param aDirPath The absolute path of the directory to watch.
  //
  nsresult AddInotifyHook(const nsAString & aDirPath);

private:
  int            mInotifyFileDesc;
  guint          mInotifySource;  // inotify gsource descriptor
  sbFileDescMap  mFileDescMap;
};

#endif  // sbLinuxFileSystemWatcher_h_

