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

#ifndef sbMacFileSystemWatcher_h_
#define sbMacFileSystemWatcher_h_

#include <sbBaseFileSystemWatcher.h>
#include <sbIFileSystemWatcher.h>
#include <sbFileSystemTree.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <CoreServices/CoreServices.h>

typedef nsTArray<nsString> sbStringArray;


class sbMacFileSystemWatcher : public sbBaseFileSystemWatcher
{

public:
  sbMacFileSystemWatcher();
  virtual ~sbMacFileSystemWatcher();

  // sbIFileSystemWatcher
  NS_IMETHOD Init(sbIFileSystemListener *aListener, 
                  const nsAString & aRootPath, 
                  PRBool aIsRecursive);
  NS_IMETHOD InitWithSession(const nsACString & aSessionGuid,
                             sbIFileSystemListener *aListener);
  NS_IMETHOD StopWatching(PRBool aShouldSaveSession);

  // sbFileSystemTreeListener
  NS_IMETHOD OnTreeReady(const nsAString & aTreeRootPath,
                         sbStringArray & aDirPathArray);

protected:
  static void FSEventCallback(ConstFSEventStreamRef aStreamRef,
                              void *aClientCallbackInfo,
                              size_t aNumEvents,
                              const char *const aEventPaths[],
                              const FSEventStreamEventFlags aEventFlags[],
                              const FSEventStreamEventId aEventIds[]);
  void OnFileSystemEvents(const sbStringArray &aEventPaths);

private:
  FSEventStreamRef     mStream;   // strong
  FSEventStreamContext *mContext;  // strong
};

#endif  // sbMacFileSystemWatcher_h_

