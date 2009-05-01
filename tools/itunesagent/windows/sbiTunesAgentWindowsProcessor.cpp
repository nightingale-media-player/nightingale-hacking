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

#include "sbiTunesAgentWindowsProcessor.h"
#include "sbiTunesAgentAppWatcher.h"

wchar_t const ITUNES_EXECUTABLE_NAME[] = L"itunes.exe";

char const AGENT_EXPORT_FILENAME_MASK[] = "songbird_export.task*";
char const AGENT_ERROR_FILENAME[] = "itunesexporterrors.txt";
char const AGENT_LOG_FILENAME[] = "itunesexport.log";
char const AGENT_SHUTDOWN_FILENAME[] = "songbird_export.shutdown";

static std::string GetSongbirdPath() {
  char buffer[4096];
  ::GetEnvironmentVariableA("AppData", buffer, sizeof(buffer));
  std::string path(buffer);
  if (!path.empty()) {
    switch (path[path.length()-1]) {
      case '/':
      case '\\':
        // Nothing to do here,
      break;
      default:
        path += '\\';
      break;
    }
  }
  path += "\\songbird2\\";
  
  return path;
}

 sbiTunesAgentWindowsProcessor::sbiTunesAgentWindowsProcessor() {
   HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
}
  
sbiTunesAgentWindowsProcessor::~sbiTunesAgentWindowsProcessor() {
  miTunesLibrary.Finalize();
  CoUninitialize();
}

sbError
sbiTunesAgentWindowsProcessor::AddTracks(std::wstring const & aSource,
                                         Tracks const & aPaths) {
  
  return miTunesLibrary.AddTracks(aSource, aPaths); 
}

sbError
sbiTunesAgentWindowsProcessor::CreatePlaylist(
    std::wstring const & aPlaylistName) {
  return miTunesLibrary.CreatePlaylist(aPlaylistName);
}

bool sbiTunesAgentWindowsProcessor::ErrorHandler(sbError const & aError) {
  std::string path(GetSongbirdPath());
  path += AGENT_ERROR_FILENAME;
  std::wofstream error(path.c_str());
  if (error) {
    error << L"ERROR: " << aError.Message() << std::endl;
  }
  return true;
}

std::string sbiTunesAgentWindowsProcessor::GetTaskFilePath() {
  std::string path(GetSongbirdPath());
  path += AGENT_EXPORT_FILENAME_MASK;
  // Look for a file that matches the mask
  WIN32_FIND_DATAA findData;
  HANDLE hFind = FindFirstFileA(path.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return std::string();
  }
  // We found one so return the full path of it
  path = GetSongbirdPath() + findData.cFileName;
  FindClose(hFind);
  return path;
}

sbError sbiTunesAgentWindowsProcessor::Initialize() {
  return miTunesLibrary.Initialize();  
}

void sbiTunesAgentWindowsProcessor::Log(std::wstring const & aMsg) {
  if (mLogState != DEACTIVATED) {
    if (mLogState != OPENED) {
      std::string logPath(GetSongbirdPath());
      logPath += AGENT_LOG_FILENAME;
      mLog.open(logPath.c_str());
      // If we can't open then don't bother trying again
      mLogState = mLog ? OPENED : DEACTIVATED;
    }
    mLog << aMsg << std::endl;
  }
}

sbError
sbiTunesAgentWindowsProcessor::RemovePlaylist(std::wstring const & aPlaylist) {
  return miTunesLibrary.RemovePlaylist(aPlaylist);
}

bool sbiTunesAgentWindowsProcessor::Shutdown() {
  std::string path = GetSongbirdPath();
  path += AGENT_SHUTDOWN_FILENAME;
  return DeleteFileA(path.c_str()) != 0;
}

bool sbiTunesAgentWindowsProcessor::ShutdownCallback(bool) {
  return Shutdown();
}

sbError 
sbiTunesAgentWindowsProcessor::WaitForiTunes() {
  sbiTunesAgentAppWatcher appWatcher(ITUNES_EXECUTABLE_NAME);
  sbCallback<sbiTunesAgentWindowsProcessor, 
             bool, 
             bool> callback(this, &sbiTunesAgentWindowsProcessor::ShutdownCallback);
  if (appWatcher.WaitForApp(callback)) {
    return sbNoError;
  }
  return sbError(L"Waiting terminated before iTunes started");
}

sbiTunesAgentProcessor * sbCreatesbiTunesAgentProcessor() {
  return new sbiTunesAgentWindowsProcessor();
}
