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

#include "sbiTunesAgentProcessor.h"

#ifndef _WIN32_WINNT    // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif            
#include <windows.h>

#include <sstream>

wchar_t const * const ADDED_MEDIA_ITEMS = L"added-mediaitems";
wchar_t const * const ADDED_PLAYLISTS = L"added-playlists";
wchar_t const * const DELETED_PLAYLISTS = L"deleted-playlists";

sbiTunesAgentProcessor::sbiTunesAgentProcessor() {
}

sbiTunesAgentProcessor::~sbiTunesAgentProcessor() 
{
  assert(mTrackBatch.empty());
}

inline 
std::wstring Strip(std::wstring const & aText) {
  if (!aText.empty()) {
    std::wstring::size_type start = aText.find_first_not_of(L" \t");
    std::wstring::size_type end = aText.find_last_not_of(L" \t");
    return std::wstring(aText, start, end + 1);
  }
  return aText;
}

static inline
void ParseTask(std::wstring const & aTask,
               std::wstring & aAction,
               std::wstring & aLibrary) {
  std::wstring::size_type const colon = aTask.find(L':');
  if (colon != std::wstring::npos) {
    aAction = Strip(aTask.substr(0, colon));
    aLibrary = Strip(aTask.substr(colon + 1, aAction.length() - colon - 1));
  }
  else {
    aAction = Strip(aTask);
    aLibrary = std::wstring();
  }
}
 
sbError sbiTunesAgentProcessor::AddTrack(std::wstring const & aSource,
                                         std::wstring const & aPath) {
  if (mLastSource.empty()) {
    mLastSource = aSource;
  }
  else if (mLastSource != aSource) {
    sbError const & error = AddTracks(mLastSource,
                                      mTrackBatch);
    if (error && !ErrorHandler(error)) {
      return error;
    }
    mLastSource = aSource;
    mTrackBatch.clear();
  }
  mTrackBatch.push_back(aPath);
  return sbNoError;
}

sbError
sbiTunesAgentProcessor::ProcessTaskFile() {
  
  int retry = 60;
  // Keep process all files we find
  for (std::string taskPath = GetTaskFilePath();
       !taskPath.empty();
       taskPath = GetTaskFilePath()) {
    mInputStream.open(taskPath.c_str());
    if (!mInputStream) {
      // Can't open, maybe it's being written to try again
      if (--retry == 0) {
        return sbError(L"Unable to open the export track file");
      }
      Sleep(1000);
      continue;
    }
    sbError const & error = Initialize();
    if (error && !ErrorHandler(error)) {
      return error;
    }
    std::wstring line;
    std::wstring action;
    std::wstring source;
    int lineno = 0;
    while (std::getline(mInputStream, line)) {
      std::wstring::size_type const beginBracket = 
        line.find_first_not_of(L" \t");
      // Look for the right bracket as first non-whitespace
      if (beginBracket != std::wstring::npos && line[beginBracket] == L'[') {
        std::wstring::size_type const endBracket = line.find_last_of(L']');
        if (endBracket != std::wstring::npos) {
          std::wstring task = line.substr(beginBracket + 1, 
                                          endBracket - beginBracket - 1);
          ParseTask(task, action, source);
        }
      }
      else {
        // If there is no action then there's nothing to do yet
        if (action.empty()) {
          continue;
        }
        std::wstring::size_type const equalSign = line.find(L'=');
        if (equalSign != std::wstring::npos) {
          std::wstring const & key = Strip(line.substr(0, equalSign));
          std::wstring const & value = Strip(line.substr(equalSign + 1));
          // We've been told to add the track
          if (action == ADDED_MEDIA_ITEMS) {
            if (key == L"URL") {
              sbError const & error = AddTrack(source,
                                               value);
              if (error && !ErrorHandler(error)) {
                return error;
              }
            }
            else {
              std::wostringstream msg;
              msg << key << L" is an invalid key, ignoring line " 
                  << lineno << L": " << lineno;
              sbError error(msg.str());
              if (!ErrorHandler(error)) {
                return error;
              }
            }
          }
          else if (action == ADDED_PLAYLISTS) {
            if (!mTrackBatch.empty()) {
              sbError const & error = AddTracks(mLastSource, mTrackBatch);
              if (error && !ErrorHandler(error)) {
                return error;
              }
              mLastSource = std::wstring();
              mTrackBatch.clear();
            }
            sbError const & error = CreatePlaylist(value);
            if (error && !ErrorHandler(error)) {
              return error;
            }
          }
          // We've been told to remove the playlist
          else if (action == DELETED_PLAYLISTS) {
            if (key == L"NAME") {
              if (!mTrackBatch.empty()) {
                sbError const & error = AddTracks(mLastSource, mTrackBatch);
                if (error && !ErrorHandler(error)) {
                  return error;
                }
                mLastSource = std::wstring();
                mTrackBatch.clear();
              }
              sbError const & error = RemovePlaylist(value);
              if (error && !ErrorHandler(error)) {
                return error;
              }
            }
            else {
              std::wostringstream msg;
              msg << key << L" is an invalid key, ignoring line " 
                  << lineno << L": " << lineno;
              sbError error(msg.str());
              if (!ErrorHandler(error)) {
                return error;
              }
            }
          }
          else {
            std::wostringstream msg;
            msg << action << L" is an invalid action , ignoring line " 
                << lineno << L": " << lineno;
            sbError error(msg.str());
            if (!ErrorHandler(error)) {
              return error;
            }
            action = std::wstring();
          }
        }
        else {
          // If the line wasn't blank then report an error
          if (!Strip(line).empty()) {
            std::wostringstream msg;
            msg << action << L" is an invalid action , ignoring line " 
                << lineno << L": " << line;
            sbError error(msg.str());
            if (!ErrorHandler(error)) {
              return error;
            }
          }
        }
      }
      ++lineno;
      if (Shutdown()) {
        mInputStream.close();
        ShutdownDone();
      }
    }
    if (!mTrackBatch.empty()) {
      sbError const & error = AddTracks(mLastSource, mTrackBatch);
      if (error) {
        ErrorHandler(error);
      }
      mLastSource = std::wstring();
      mTrackBatch.clear();
    }
    // Close the stream and remove the file
    mInputStream.close();
    remove(taskPath.c_str());
  }
  return sbNoError;
}
