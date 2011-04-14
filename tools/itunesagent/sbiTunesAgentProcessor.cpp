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

#include "sbiTunesAgentProcessor.h"

#include <sstream>


//------------------------------------------------------------------------------
// Export media task constants

char const * const ADDED_MEDIA_ITEMS = "added-mediaitems";
char const * const UPDATED_MEDIA_ITEMS = "updated-mediaitems";
char const * const ADDED_PLAYLISTS = "added-medialists";
char const * const DELETED_PLAYLISTS = "removed-medialists";
char const * const UPDATED_SMARTPLAYLIST = "updated-smartplaylist";
char const * const SCHEMA_VERSION = "schema-version";

//------------------------------------------------------------------------------

sbiTunesAgentProcessor::sbiTunesAgentProcessor()
  : mLogState((sbLogState)AGENT_LOGGING),
    mBatchSize(sbiTunesAgentProcessor::BATCH_SIZE),
    mFolderName("Songbird")
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

inline bool
isHex(char c)
{
  if ((c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F'))
    return true;
  return false;
}

inline int
hexValue(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return 0;
}

inline std::string
Unescape(std::string const & aText)
{
  if (!aText.empty()) {
    std::string unescaped;
    size_t len = aText.length();
    for (size_t i = 0; i < len; i++) {
      if (i+2 < len && 
          aText[i] == '%' && 
          isHex(aText[i+1]) && 
          isHex(aText[i+2]))
      {
        unescaped.push_back((char)
                ((hexValue(aText[i+1]) << 4) | hexValue(aText[i+2])));
        i += 2;
      }
      else if (aText[i] == '+')
        unescaped.push_back(' ');
      else
        unescaped.push_back(aText[i]);
    }
    return unescaped;
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
    aLibrary = Unescape(
            Strip(aTask.substr(colon + 1, aAction.length() - colon - 1)));
  }
  else {
    aAction = Strip(aTask);
    aLibrary.clear();
  }
}
 
sbError sbiTunesAgentProcessor::AddTrack(std::string const & aSource,
                                         std::string const & aSongbirdId,
                                         std::string const & aPath,
                                         const char *aMethod)
{
  if (mLastSource.empty()) {
    mLastSource = aSource;

    // If the last method is empty, and the passed in method is the "updated
    // smart playlist method", the existing list needs to be cleared.
    if (mLastMethod.empty() && strcmp(aMethod, UPDATED_SMARTPLAYLIST) == 0) {
      mLastMethod = aMethod;
      // Clear out the smart list
      sbError const & error = ClearPlaylist(aSource);
      SB_ENSURE_ERRORHANDLER(error);
    }
  }
  else if (mLastSource != aSource || mTrackBatch.size() >= mBatchSize) {
    sbError const & error = AddTracks(mLastSource, mTrackBatch);
    SB_ENSURE_ERRORHANDLER(error);

    mLastSource = aSource;
    mTrackBatch.clear();
  }
  mTrackBatch.push_back(Track(Track::TYPE_SONGBIRD, aSongbirdId, aPath));
  return sbNoError;
}

sbError sbiTunesAgentProcessor::UpdateTrack(std::string const & aID,
                                            std::string const & aPath)
{
  if (mTrackBatch.size() >= mBatchSize) {
    sbError const & error = UpdateTracks(mTrackBatch);
    SB_ENSURE_ERRORHANDLER(error);

    mLastSource = "";
    mTrackBatch.clear();
  }
  mTrackBatch.push_back(Track(Track::TYPE_ITUNES, aID, aPath));
  return sbNoError;
}

sbError
sbiTunesAgentProcessor::ProcessTaskFile()
{
  int retry = 60;

  if (!OpenResultsFile()) {
    return sbError("Failed to open results file to check for start!");
  }
  
  if (OpenResultsFile().tellp() == std::ofstream::pos_type(0)) {
    OpenResultsFile() << '[' << SCHEMA_VERSION << ":2]\n"
                      << '[' << ADDED_MEDIA_ITEMS << "]\n";
  }

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

          sbError error = ProcessPendingTrackBatch();
          SB_ENSURE_ERRORHANDLER(error);
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
          std::string const & key =
            Strip(line.substr(0, equalSign));
          std::string const & value =
            Unescape(Strip(line.substr(equalSign + 1)));

          // We've been told to add the track
          if (action == ADDED_MEDIA_ITEMS) {
            sbError const & error = AddTrack(source, key, value, ADDED_MEDIA_ITEMS);
            SB_ENSURE_ERRORHANDLER(error);
          }
          else if (action == UPDATED_MEDIA_ITEMS) {
            sbError const & error = UpdateTrack(key, value);
            SB_ENSURE_ERRORHANDLER(error);
          }
          else if (action == ADDED_PLAYLISTS) {
            sbError error = CreatePlaylist(value);
            SB_ENSURE_ERRORHANDLER(error);
          }
          // We've been told to remove the playlist
          else if (action == DELETED_PLAYLISTS) {
            sbError error = RemovePlaylist(value);
            SB_ENSURE_ERRORHANDLER(error);
          }
          else if (action == UPDATED_SMARTPLAYLIST) {
            sbError const & error = AddTrack(source,
                                             key,
                                             value,
                                             UPDATED_SMARTPLAYLIST);
            SB_ENSURE_ERRORHANDLER(error);
          }
          else {
            std::ostringstream msg;
            msg << action << " is an invalid action , ignoring line " 
                << lineno;
            sbError error(msg.str());
            SB_ENSURE_ERRORHANDLER(error);

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
            SB_ENSURE_ERRORHANDLER(error);
          }
        }
      }
      ++lineno;
      if (ShouldShutdown()) {
        // Notify shutdown done and exit
        ShutdownDone();
        return sbNoError;
      }
    }
    if (!mTrackBatch.empty()) {
      sbError const & error = ProcessPendingTrackBatch();
      SB_ENSURE_ERRORHANDLER(error);
    }
    // Close the stream and remove the file
    mInputStream.close();
    mInputStream.clear();
    RemoveTaskFile();
  }
  return sbNoError;
}

sbError
sbiTunesAgentProcessor::ProcessPendingTrackBatch()
{
  std::string source(mLastSource);
  mLastSource.clear();
  mLastMethod.clear();
  
  if (mTrackBatch.empty()) {
    return sbNoError;
  }

  if (mTrackBatch.front().type == Track::TYPE_ITUNES) {
    // updates
    sbError const & error = UpdateTracks(mTrackBatch);
    SB_ENSURE_ERRORHANDLER(error);
  } else {
    sbError const & error = AddTracks(source, mTrackBatch);
    SB_ENSURE_ERRORHANDLER(error);
  }

  mTrackBatch.clear();
  return sbNoError;
}

