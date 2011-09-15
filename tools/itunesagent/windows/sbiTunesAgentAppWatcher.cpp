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

#include "sbiTunesAgentAppWatcher.h"

#include <algorithm>

#include <psapi.h>

DWORD const ITUNES_CHECK_INTERVAL_MS = 1000; // 1 second

TCHAR const ITUNES_FILE_NAME[] = L"/itunes.exe";
int const ITUNES_FILE_NAME_LENGTH = sizeof(ITUNES_FILE_NAME) / 
  sizeof(ITUNES_FILE_NAME[0]) - 1;

sbiTunesAgentAppWatcher::sbiTunesAgentAppWatcher(std::wstring const & aApp) : 
  mProcessIDCount(0),
  mAppName(aApp) {
}

bool sbiTunesAgentAppWatcher::IsAppProcess(DWORD aProcessID) {
  bool isApp = false;
  
  // Only check the ID if we've not seen it before
  if (mProcessIDs.find(aProcessID) == mProcessIDs.end()) {

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, aProcessID);
    if (hProcess) {
      DWORD const length = 
        GetProcessImageFileName(hProcess, 
                                mProcessNameBuffer,
                                PROCESS_NAME_BUFFER_SIZE);
      // If a named was returned and it matches the app name
      isApp = length > mAppName.length() && 
                 _wcsnicmp(mAppName.c_str(), 
                           mProcessNameBuffer + length - mAppName.length(), 
                           ITUNES_FILE_NAME_LENGTH) == 0;
      
      CloseHandle(hProcess);
    }
  }
  return isApp;
}

bool 
sbiTunesAgentAppWatcher::SearchForApp() {
  bool found = false;
  /**
   * Search for an App process in the ID array buffer
   */
  for (DWORD index = 0; !found && index < mProcessIDCount; ++index) {
    found = IsAppProcess(mProcessIDsBuffer[index]);
  }
  
  if (!found) {
    // Clear out the old ID's in our set and copy the new ones
    mProcessIDs.clear();
    std::copy(mProcessIDsBuffer, 
              mProcessIDsBuffer + mProcessIDCount, 
              std::inserter(mProcessIDs, mProcessIDs.end()));
  }
  return found;
}

bool 
sbiTunesAgentAppWatcher::WaitForApp(sbiTunesAgentAppWatcher::Callback const & aCallback) 
{
  bool foundApp;
  do {
    if (!EnumProcesses(mProcessIDsBuffer, 
                       sizeof(mProcessIDsBuffer), 
                       &mProcessIDCount)) {
      return false;
    }
    foundApp = SearchForApp();
    if (!foundApp) {
      if (aCallback(true)) {
        return false;
      }
      Sleep(ITUNES_CHECK_INTERVAL_MS);
    }
  } while (!foundApp);
  return foundApp;
}

