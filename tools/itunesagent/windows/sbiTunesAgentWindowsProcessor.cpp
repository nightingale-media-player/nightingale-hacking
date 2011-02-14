/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 et sw=2 ai tw=80: */
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
#include <shlobj.h>
#include <windows.h>
#include <psapi.h>
#include <sstream>

wchar_t const ITUNES_EXECUTABLE_NAME[] = L"itunes.exe";
wchar_t const AGENT_EXPORT_FILENAME_MASK[] = L"songbird_export.task*";
wchar_t const AGENT_ERROR_FILENAME[] = L"itunesexporterrors.txt";
wchar_t const AGENT_LOG_FILENAME[] = L"itunesexport.log";
wchar_t const AGENT_RESULT_FILENAME[] = L"itunesexportresults.txt";
wchar_t const AGENT_SHUTDOWN_FILENAME[] = L"songbird_export.shutdown";
wchar_t const WINDOWS_RUN_KEY[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
wchar_t const WINDOWS_RUN_KEY_VALUE[] = L"sbitunesagent";

// table of number of bytes given a UTF8 lead char
static const int UTF8_SIZE[] = {
  // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 10
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 30
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 50
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 70
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 90
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A0
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B0
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // C0
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // D0
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // E0
     3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5  // F0
};

std::wstring ConvertUTF8ToUTF16(const std::string& src) {
  std::wstring result;
  wchar_t c;
  std::string::const_iterator it, end = src.end();
  #define GET_BITS(c,n) (((c) & 0xFF) & ((unsigned char)(1 << (n)) - 1))
  for (it = src.begin(); it < end;) {
    int bits = UTF8_SIZE[(*it) & 0xFF];
    c = GET_BITS(*it++, 8 - 1 - bits);
    switch(bits) {
      case 5: c <<= 6; c |= GET_BITS(*it++, 6);
      case 4: c <<= 6; c |= GET_BITS(*it++, 6);
      case 3: c <<= 6; c |= GET_BITS(*it++, 6);
      case 2: c <<= 6; c |= GET_BITS(*it++, 6);
      case 1: c <<= 6; c |= GET_BITS(*it++, 6);
    }
    result.append(1, c);
  }
  return result;
  #undef GET_BITS
}

std::string ConvertUTF16ToUTF8(const std::wstring& src) {
  std::string result;
  unsigned long c;
  unsigned char buffer[6];
  std::wstring::const_iterator it, end = src.end();
  for (it = src.begin(); it < end; ++it) {
    c = (unsigned long)(*it);
    int trailCount;
    if (c >= 0x04000000) {
      trailCount = 5;
    } else if (c >= 0x200000) {
      trailCount = 4;
    } else if (c >= 0x010000) {
      trailCount = 3;
    } else if (c >= 0x0800) {
      trailCount = 2;
    } else if (c >= 0x80) {
      trailCount = 1;
    } else {
      // raw, special case
      result.append(1, (char)(c & 0x7F));
      continue;
    }
    for (int i = trailCount; i > 0; --i) {
      buffer[i] = (unsigned char)(c & 0x3F);
      c >>= 6;
    }
    buffer[0] = ((1 << (trailCount + 1)) - 1) << (8 - trailCount - 1);
    buffer[0] |= c & ((1 << (8 - trailCount - 2)) - 1);
    result.append((std::string::value_type*)buffer, trailCount + 1);
  }
  return result;
}

static std::wstring gSongbirdProfileName;

static std::wstring GetSongbirdPath() {
  WCHAR buffer[MAX_PATH + 2];
  HRESULT result = SHGetSpecialFolderPathW(NULL, buffer, CSIDL_APPDATA, true);
  if (!SUCCEEDED(result)) 
    return std::wstring();
 
  std::wstring path(buffer);
  if (!path.empty()) {
    switch (path[path.length()-1]) {
    case L'/':
    case L'\\':
        // Nothing to do here,
      break;
      default:
      path += L'\\';
      break;
    }
  }

  // Use specified --profile path.
  if (!gSongbirdProfileName.empty()) {
    path += gSongbirdProfileName;
  }
  else {
    path += ConvertUTF8ToUTF16(STRINGIZE(SB_APPNAME) STRINGIZE(SB_PROFILE_VERSION));
  }
  path += L'/';
  return path;
}

sbiTunesAgentWindowsProcessor::sbiTunesAgentWindowsProcessor() :
  mAppExistsMutex(0),
  mAutoCOMInit(COINIT_MULTITHREADED) {
}
  
sbiTunesAgentWindowsProcessor::~sbiTunesAgentWindowsProcessor() {
  if (mAppExistsMutex) {
    CloseHandle(mAppExistsMutex);
  }
  miTunesLibrary.Finalize();
}

sbError
sbiTunesAgentWindowsProcessor::AddTracks(std::string const & aSource,
                                         TrackList const & aPaths) {
  typedef std::deque<std::wstring> StringDeque;
  StringDeque paths;
  StringDeque trackIds;
  TrackList::const_iterator const end = aPaths.end();
  for (TrackList::const_iterator iter = aPaths.begin(); iter != end; ++iter) {
    paths.push_back(ConvertUTF8ToUTF16(iter->path));
  }
  sbError error;
  error = miTunesLibrary.AddTracks(ConvertUTF8ToUTF16(aSource),
                                   paths,
                                   trackIds);
  SB_ENSURE_SUCCESS(error, error);

  if (!OpenResultsFile()) {
    return sbError("Failed to open output file");
  }

  TrackList::const_iterator trackIter = aPaths.begin();
  StringDeque::const_iterator idIter;
  for (idIter = trackIds.begin();
       idIter != trackIds.end();
       ++idIter, ++trackIter) {
    OpenResultsFile() << trackIter->id
                      << "="
                      << miTunesLibrary.GetLibraryId()
                      << ","
                      << ConvertUTF16ToUTF8(*idIter)
                      << std::endl;
  }
  return sbNoError;
}

sbError
sbiTunesAgentWindowsProcessor::UpdateTracks(TrackList const & aPaths) {
  std::deque<sbiTunesLibrary::TrackProperty> tracks;
  TrackList::const_iterator const end = aPaths.end();
  for (TrackList::const_iterator iter = aPaths.begin(); iter != end; ++iter) {
    unsigned long long id;
    int count = 0;
    assert(iter->type == Track::TYPE_ITUNES);
    count = sscanf_s(iter->id.c_str(), "%llx", &id);
    if (count < 1) {
      std::string msg("Unable to parse itunes ID ");
      msg.append(iter->id);
      return sbError(msg);
    }
    tracks.push_back(sbiTunesLibrary::TrackProperty(id,
                                                    ConvertUTF8ToUTF16(iter->path)));
  }
  return miTunesLibrary.UpdateTracks(tracks);
}

bool sbiTunesAgentWindowsProcessor::GetIsAgentRunning() {
  // Create a named mutex to prevent other instances from starting
  HANDLE mAppExistsMutex = CreateMutex(0, TRUE, L"songbirditunesagent");
  // Check for other instances. We'll either get the already exist
  // error or it might fail due to permissions
  return !mAppExistsMutex || GetLastError() == ERROR_ALREADY_EXISTS;
}

sbError
sbiTunesAgentWindowsProcessor::CreatePlaylist(
    std::string const & aPlaylistName) {
  return miTunesLibrary.CreatePlaylist(ConvertUTF8ToUTF16(aPlaylistName));
}

sbError
sbiTunesAgentWindowsProcessor::ClearPlaylist(std::string const & aListName) {
  sbError error;
  error = RemovePlaylist(aListName);
  SB_ENSURE_SUCCESS(error, error);

  error = CreatePlaylist(aListName);
  SB_ENSURE_SUCCESS(error, error);

  return sbNoError;
}

bool sbiTunesAgentWindowsProcessor::ErrorHandler(sbError const & aError) {
  std::wstring path(GetSongbirdPath());
  path += AGENT_ERROR_FILENAME;
  std::ofstream error(path.c_str());
  if (error) {
    error << "ERROR: " << aError.Message() << std::endl;
  }
  return true;
}

bool sbiTunesAgentWindowsProcessor::OpenTaskFile(std::ifstream & aStream) {
  std::wstring path(GetSongbirdPath());
  path += AGENT_EXPORT_FILENAME_MASK;
  // Look for a file that matches the mask
  WIN32_FIND_DATAW findData;
  HANDLE hFind = FindFirstFileW(path.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return false;
  }
  // We found one so return the full path of it
  path = GetSongbirdPath() + findData.cFileName;
  mCurrentTaskFile = path;
  FindClose(hFind);
  aStream.open(path.c_str());
  return true;
}

std::ofstream & sbiTunesAgentWindowsProcessor::OpenResultsFile() {
  if (!!mOutputStream && !mOutputStream.is_open()) {
    std::wstring path(GetSongbirdPath());
    path += AGENT_RESULT_FILENAME;
    mOutputStream.open(path.c_str(), std::ios_base::ate);
  }
  return mOutputStream;
}

sbError sbiTunesAgentWindowsProcessor::Initialize() {
  if (!mAutoCOMInit.Succeeded()) {
    return sbError("COM Failed to initialize");
  }
  return miTunesLibrary.Initialize();
}

void sbiTunesAgentWindowsProcessor::Log(std::string const & aMsg) {
  if (mLogState != DEACTIVATED) {
    if (mLogState != OPENED) {
      std::wstring logPath(GetSongbirdPath());
      logPath += AGENT_LOG_FILENAME;
      mLogStream.open(logPath.c_str());
      // If we can't open then don't bother trying again
      mLogState = mLogStream ? OPENED : DEACTIVATED;
    }
    mLogStream << aMsg << std::endl;
  }
}

void sbiTunesAgentWindowsProcessor::RemoveTaskFile() {
  if (!mCurrentTaskFile.empty()) {
    DeleteFileW(mCurrentTaskFile.c_str());
    mCurrentTaskFile.clear();
  }
}

sbError
sbiTunesAgentWindowsProcessor::RemovePlaylist(std::string const & aPlaylist) {
  return miTunesLibrary.RemovePlaylist(ConvertUTF8ToUTF16(aPlaylist));
}

bool sbiTunesAgentWindowsProcessor::ShouldShutdown() {
  std::wstring path = GetSongbirdPath();
  path += AGENT_SHUTDOWN_FILENAME;
  return DeleteFileW(path.c_str()) != 0;
}

bool sbiTunesAgentWindowsProcessor::ShutdownCallback(bool) {
  return ShouldShutdown();
}

/**
 * Registers the application to startup when the user logs in
 */
sbError 
sbiTunesAgentWindowsProcessor::RegisterForStartOnLogin() {
  HKEY runKey;
  LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, 
                              WINDOWS_RUN_KEY,
                              0, 
                              KEY_READ | KEY_WRITE, 
                              &runKey);
  if (result != ERROR_SUCCESS) {
    return sbError("Unable to set the Windows RUN sbitunesagent value");
  }
  wchar_t exePath[MAX_PATH + 1];
  GetModuleFileNameW(0, exePath, MAX_PATH + 1);
  
  sbError error;
  DWORD const bytes = (wcslen(exePath) + 1) * 
                  sizeof(wchar_t);
  result = RegSetValueExW(runKey, 
                          WINDOWS_RUN_KEY_VALUE, 
                          0, 
                          REG_SZ, 
                          reinterpret_cast<BYTE const *>(exePath),
                          bytes);
  if (result != ERROR_SUCCESS) {
    error = sbError("Unable to get Windows RUN registry key");
  }
  RegCloseKey(runKey);
  return error;
}

/**
 * Unregisters the application to startup when the user logs in
 */
sbError 
sbiTunesAgentWindowsProcessor::UnregisterForStartOnLogin() {
  sbError error;
  HKEY runKey;
  LONG result = RegOpenKeyW(HKEY_CURRENT_USER, WINDOWS_RUN_KEY, &runKey);
  if (result != ERROR_SUCCESS) {
    error = sbError("Unable to get Windows RUN registry key");
  }
  result = RegDeleteValueW(runKey, 
                       WINDOWS_RUN_KEY_VALUE);
  if (result != ERROR_SUCCESS) {
    error = sbError("Unable to remove the Windows RUN sbitunesagent value");
  }
  RegCloseKey(runKey);

  return error;
}

void
sbiTunesAgentWindowsProcessor::RegisterProfile(std::string const & aProfileName)
{
  gSongbirdProfileName = ConvertUTF8ToUTF16(aProfileName);
}

bool sbiTunesAgentWindowsProcessor::TaskFileExists() {
  std::wstring path(GetSongbirdPath());
  path += AGENT_EXPORT_FILENAME_MASK;
  // Look for a file that matches the mask
  WIN32_FIND_DATAW findData;
  HANDLE hFind = FindFirstFileW(path.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return false;
  }
  FindClose(hFind);
  return true;
}

/**
 * Sleep for x milliseconds
 */
void sbiTunesAgentWindowsProcessor::Sleep(unsigned long aMilliseconds){
  ::Sleep(aMilliseconds);
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
  return sbError("Waiting terminated before iTunes started");
}

sbError
sbiTunesAgentWindowsProcessor::KillAllAgents() {
  // Loop through the processes and kill all the running agent processes.
  DWORD allProcesses[1024], cbNeeded, cProcesses, curProcessID;

  curProcessID = GetCurrentProcessId();
  
  if (EnumProcesses(allProcesses, sizeof(allProcesses), &cbNeeded)) {
    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++) {
      if (allProcesses[i] == 0 || allProcesses[i] == curProcessID) {
        continue;
      }

      TCHAR curProcessName[MAX_PATH] = TEXT("");
      HANDLE hProcess = 
        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, 
                    FALSE,
                    allProcesses[i]);
      
      if (hProcess != NULL) {
        if (GetModuleBaseName(hProcess,
                              NULL,
                              curProcessName,
                              sizeof(curProcessName) / sizeof(TCHAR))) {
          
          if (_tcscmp(curProcessName, _T("songbirditunesagent.exe")) == 0) {
            if (!TerminateProcess(hProcess, -1)) {
              std::ostringstream error;
              error << "ERROR: Could not kill pid " << allProcesses[i];
              Log(error.str());
            }
          }
        }
      }

      CloseHandle(hProcess);
    }
  }

  return sbNoError;
}

sbiTunesAgentProcessor * sbCreatesbiTunesAgentProcessor() {
  return new sbiTunesAgentWindowsProcessor();
}
