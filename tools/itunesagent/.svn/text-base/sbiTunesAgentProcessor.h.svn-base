/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef SBITUNESAGENTPROCESSOR_H_
#define SBITUNESAGENTPROCESSOR_H_

#include <deque>
#include <fstream>
#include <string>
#include <vector>
#include <assert.h>

#include "sbError.h"

#define EXPORT_SCHEMAVERSION  2

// Logging options
#define AGENT_LOGGING 1

/**
 * Turns preprocessor labels' values into strings
 */
#define STRINGIZE2(arg) #arg
#define STRINGIZE(arg) STRINGIZE2(arg)

/**
 * Error checking helper macro
 */
#define SB_ENSURE_SUCCESS(res, ret)                            \
    if (res) {                                                 \
      return ret;                                              \
    }

#define SB_ENSURE_ERRORHANDLER(error)                          \
  if (error && !ErrorHandler(error)) {                         \
    return error;                                              \
  }

/**
 * This is the base class for the iTunes agent processor.
 * It provides an interface for tasks. And contains platform neutral code such
 * as parsing the agent ini file.
 * ProcessTaskFile is the main entry point for this class. All other methods are
 * called from there.
 */
class sbiTunesAgentProcessor
{
public:
  static int const BATCH_SIZE = 10;

  /**
   * Clean up any resources
   */
  virtual ~sbiTunesAgentProcessor();

  /**
   * Registers the processor's profile
   */
  virtual void RegisterProfile(std::string const & aProfileName) = 0;

  /**
   * Sets the iTunes playlist folder name
   */
  virtual void RegisterFolderName() = 0;

  /**
   * Returns true if there are any tasks file ready to process
   */
  virtual bool TaskFileExists() = 0; 
  
  /**
   * Removes the task file
   */
  virtual void RemoveTaskFile() = 0;
  
  /**
   * Process the task file
   * This basically opens the file and calls ProcessStream
   */
  virtual sbError ProcessTaskFile();
  
  /**
   * Waits for the iTunes process to start
   */
  virtual sbError WaitForiTunes()=0;
  
  /**
   * Adds a track to the iTunes database given a path
   */
  sbError AddTrack(std::string const & aSource,
                   std::string const & aSongbirdId,
                   std::string const & aPath,
                   const char *aMethod);

  /**
   * Updates a track to the iTunes database given an iTunes ID and new path
   */
  sbError UpdateTrack(std::string const & aID,
                      std::string const & aPath);

  /**
   * Reports the error, returns true if app should continue
   */
  virtual bool ErrorHandler(sbError const & aError) = 0;
  
  /**
   * Registers the application to startup when the user logs in
   */
  virtual sbError RegisterForStartOnLogin()=0;

  /**
   * Unregisters the application from automatically starting when the user
   * logs in
   */
  virtual sbError UnregisterForStartOnLogin()=0;

  /**
   * Returns true if the agent is already running as another process.
   */
  virtual bool GetIsAgentRunning() = 0;

  /**
   * Finds and kills all the agent processes that the user is currently 
   * running.
   */
  virtual sbError KillAllAgents() = 0;

  /**
   * Sets the batch size for processing
   */
  void SetBatchSize(unsigned int aBatchSize) {
    mBatchSize = aBatchSize;
  }
  /**
   * A pair of iTunes-or-Songbird persistent ID and track file path
   * iTunes persistent ID will be 0 when unknown (adding files)
   */
  struct Track {
    enum type_t {
      TYPE_SONGBIRD,
      TYPE_ITUNES
    } type;
    std::string id;
    std::string path;
    Track(type_t aType, std::string aId, std::string aPath)
      : type(aType), id(aId), path(aPath) {}
  };
protected:
  /**
   * An iterable list of tracks
   */
  typedef std::deque<Track> TrackList;

  /**
   * Initialize any state
   */
  sbiTunesAgentProcessor();

  /**
   * Adds a track to the iTunes database given a path
   */
  virtual sbError AddTracks(std::string const & aSource,
                            TrackList const & aPaths)=0;

  /**
   * Update tracks
   */
  virtual sbError UpdateTracks(TrackList const & aPaths)=0;

  /**
   * Removes a playlist from the iTunes database
   */
  virtual sbError RemovePlaylist(std::string const & aPlaylist) = 0;

  /**
   * Creates a playlist (Recreates it if it already exists)
   */
  virtual sbError CreatePlaylist(std::string const & aPlaylistName) = 0;

  /**
   * Clear the contents of a Songbird playlist.
   */
  virtual sbError ClearPlaylist(std::string const & aPlaylistName) = 0;

  /**
   * TODO Add a comment here please.
   */
  sbError ProcessPendingTrackBatch();

  /**
   * Retrieve the path to the task file
   */
  virtual bool OpenTaskFile(std::ifstream & aStream) = 0;

  /**
   * Retrieve the results file
   */
  virtual std::ofstream & OpenResultsFile() = 0;

  /**
   * Performs any initialization necessary. Optional to implement
   */
  virtual sbError Initialize() { return sbNoError; }

  /**
   * Logs the message to the platform specific log device
   */
  virtual void Log(std::string const & aMsg) = 0;

  /**
   * Returns true if we should shutdown
   */
  virtual bool ShouldShutdown() = 0;

  /**
   * Sleep for x milliseconds
   */
  virtual void Sleep(unsigned long aMilliseconds) = 0;

  /**
   * This allows the derived class to perform any kind of
   * cleanup needed after shutdown has been processed. Default
   * implementation is to do nothing
   */
  virtual void ShutdownDone() {}

  enum VersionAction {
    OK,     // It's OK, we like this version we know it well
    ABORT,  // Have no clue what to do with it, so just stop
    RETRY   // Retry we've converted the file and there's a new one to process
            // it. The parser will just delete this file
  };

  /**
   * Returns what to do with the file given it's version
   */
  VersionAction VersionCheck(std::string const & aVersion)
  {
    if (atoi(aVersion.c_str()) == EXPORT_SCHEMAVERSION) {
      return OK;
    }
    return ABORT;
  }

protected:
  typedef enum {
    DEACTIVATED = 0,
    ACTIVE      = 1,
    OPENED      = 2
  } sbLogState;

  sbLogState    mLogState;
  std::ofstream mLogStream;
  unsigned int  mBatchSize;
  std::string   mFolderName;
private:
  std::ifstream  mInputStream;
  TrackList      mTrackBatch;
  std::string    mLastSource;
  std::string    mLastMethod;
};

//------------------------------------------------------------------------------

// Utility method to create a itunes agent parser.
typedef std::auto_ptr<sbiTunesAgentProcessor> sbiTunesAgentProcessorPtr;
sbiTunesAgentProcessor * sbCreatesbiTunesAgentProcessor();

#endif /* SBITUNESAGENTPROCESSOR_H_ */

