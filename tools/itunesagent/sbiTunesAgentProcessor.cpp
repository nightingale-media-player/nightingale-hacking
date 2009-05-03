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

#include <sstream>


//------------------------------------------------------------------------------
// Export media task constants

char const * const ADDED_MEDIA_ITEMS = "added-mediaitems";
char const * const ADDED_PLAYLISTS = "added-medialists";
char const * const DELETED_PLAYLISTS = "removed-medialists";
char const * const SCHEMA_VERSION = "schema-version";

//------------------------------------------------------------------------------

sbiTunesAgentProcessor::sbiTunesAgentProcessor()
{
}

sbiTunesAgentProcessor::~sbiTunesAgentProcessor() 
{
  assert(mTrackBatch.empty());
}

inline std::string 
Strip(std::string const & aText) 
{
  if (!aText.empty()) {
    std::string::size_type start = aText.find_first_not_of(" \t");
    std::string::size_type end = aText.find_last_not_of(" \t");
    return std::string(aText, start, end + 1);
  }
  return aText;
}

static inline void
ParseTask(std::string const & aTask,
          std::string & aAction,
          std::string & aLibrary)
{
  std::string::size_type const colon = aTask.find(':');
  if (colon != std::string::npos) {
    aAction = Strip(aTask.substr(0, colon));
    aLibrary = Strip(aTask.substr(colon + 1, aAction.length() - colon - 1));
  }
  else {
    aAction = Strip(aTask);
    aLibrary.clear();
  }
}
 
sbError sbiTunesAgentProcessor::AddTrack(std::string const & aSource,
                                         std::string const & aPath) 
{
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
sbiTunesAgentProcessor::ProcessTaskFile()
{
  int retry = 60;
  
  // Keep process all files we find
  while (OpenTaskFile(mInputStream)) {
    if (!mInputStream) {
      // Can't open, maybe it's being written to try again
      if (--retry == 0) {
        return sbError("Unable to open the export track file");
      }
      Sleep(1000);
      continue;
    }
    sbError const & error = Initialize();
    if (error && !ErrorHandler(error)) {
      return error;
    }
    std::string line;
    std::string action;
    std::string source;
    int lineno = 0;
    while (std::getline(mInputStream, line)) {
      std::string::size_type const beginBracket = 
        line.find_first_not_of(" \t");
      // Look for the right bracket as first non-whitespace
      if (beginBracket != std::string::npos && line[beginBracket] == '[') {
        std::string::size_type const endBracket = line.find_last_of(']');
        if (endBracket != std::string::npos) {
          std::string task = line.substr(beginBracket + 1, 
                                         endBracket - beginBracket - 1);
          ParseTask(task, action, source);
          if (action == SCHEMA_VERSION) {
            VersionAction const versionAction = VersionCheck(source);
            action.clear();
            source.clear();
            if (versionAction == ABORT) {
              return sbError("Incompatible version encountered");
            }
            else if (versionAction == RETRY) {
              mInputStream.close();
              continue; // Will hit the inner while and it will stop
            }
          }
        }
      }
      else {
        // If there is no action then there's nothing to do yet
        if (action.empty()) {
          ++lineno;
          continue;
        }
        std::string::size_type const equalSign = line.find('=');
        if (equalSign != std::string::npos) {
          std::string const & key = Strip(line.substr(0, equalSign));
          std::string const & value = Strip(line.substr(equalSign + 1));
          // We've been told to add the track
          if (action == ADDED_MEDIA_ITEMS) {
            if (key == "URL") {
              sbError const & error = AddTrack(source, value);
              if (error && !ErrorHandler(error)) {
                return error;
              }
            }
            else {
              std::ostringstream msg;
              msg << key 
                  << " is an invalid key, ignoring line " 
                  << lineno;
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
              mLastSource.clear();
              mTrackBatch.clear();
            }
            sbError const & error = CreatePlaylist(value);
            if (error && !ErrorHandler(error)) {
              return error;
            }
          }
          // We've been told to remove the playlist
          else if (action == DELETED_PLAYLISTS) {
            if (!mTrackBatch.empty()) {
              sbError const & error = AddTracks(mLastSource, mTrackBatch);
              if (error && !ErrorHandler(error)) {
                return error;
              }
              mLastSource.clear();
              mTrackBatch.clear();
            }
            sbError const & error = RemovePlaylist(value);
            if (error && !ErrorHandler(error)) {
              return error;
            }
          }
          else {
            std::ostringstream msg;
            msg << action << " is an invalid action , ignoring line " 
                << lineno;
            sbError error(msg.str());
            if (!ErrorHandler(error)) {
              return error;
            }
            action.clear();
          }
        }
        else {
          // If the line wasn't blank then report an error
          if (!Strip(line).empty()) {
            std::ostringstream msg;
            msg << action 
                << " is an invalid action , ignoring line " << lineno;
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
      mLastSource.clear();
      mTrackBatch.clear();
    }
    // Close the stream and remove the file
    mInputStream.close();
    RemoveTaskFile();
  }
  return sbNoError;
}
