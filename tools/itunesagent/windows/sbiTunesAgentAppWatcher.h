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
#ifndef SBITUNESAGENTAPPWATCHER_H_
#define SBITUNESAGENTAPPWATCHER_H_

#include <windows.h>
#include <tchar.h>
#include <set>
#include <string>

#include "sbCallback.h"

/**
 * Class that waits for an app to be started
 */
class sbiTunesAgentAppWatcher
{
public:
  /**
   * Initializes the process ID count
   */
  sbiTunesAgentAppWatcher(std::wstring const & aAppName);
  enum SearchResult {
    FOUND,
    
  };
  typedef sbCallbackBase<bool, bool> Callback;
  /**
   * Waits for the App process to be started. Returns true
   * if the process started, or false if returning for a different reason
   */
  bool WaitForApp(Callback const & aCallback);
private:
  /**
   * Collection of process ID's type.
   * XXX TODO set might be overkill, vector with a linear search might win
   * but an exercise for a later date
   */
  typedef std::set<DWORD> ProcessIDs;
  
  /**
   * Length of buffer used to receive process ID's from win32 API
   */
  static DWORD const PROCESS_ID_BUFFER_SIZE = 4096;
  /**
   * Buffer used to receive process ID's from win32 API
   */
  DWORD mProcessIDsBuffer[PROCESS_ID_BUFFER_SIZE];
  /**
   * The number of process ID's in the mProcessIDs buffer
   */
  DWORD mProcessIDCount;
  
  /**
   * Length of buffer used to retrieve process name from win32 API
   */
  static DWORD const PROCESS_NAME_BUFFER_SIZE = 4096;
  /**
   * Buffer used to retrieve process name from win32 API
   */
  TCHAR mProcessNameBuffer[PROCESS_NAME_BUFFER_SIZE];
  /**
   * The collection of process ID's previously found
   */
  ProcessIDs mProcessIDs;
  /**
   * The name of the app to watch for
   */
  std::wstring mAppName;
  /**
   * Function that returns true if aProcessID is the App process
   */
  bool IsAppProcess(DWORD aProcessID);
  /**
   * Searches for the App process and returns true if found
   */
  bool SearchForApp();
};

#endif /* SBITUNESAGENTAPPWATCHER_H_ */
