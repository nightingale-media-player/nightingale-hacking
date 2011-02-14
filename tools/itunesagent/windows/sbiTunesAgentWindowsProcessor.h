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

#ifndef SBITUNESAGENTWINDOWSPROCESSOR_H_
#define SBITUNESAGENTWINDOWSPROCESSOR_H_

// Standard library includes
#include <fstream>

// Songbird includes
#include <sbAutoCOMInitializer.h>

// Local includes
#include "sbiTunesAgentProcessor.h"
#include "sbiTunesLibrary.h"

class sbiTunesAgentWindowsProcessor : public sbiTunesAgentProcessor
{
public:
  /**
   * Initialize any state
   */
  sbiTunesAgentWindowsProcessor();
  
  /**
   * Cleanup resources
   */
  virtual ~sbiTunesAgentWindowsProcessor();
  
  /**
   * Removes the task file
   */
  virtual void RemoveTaskFile();
  
  /**
   * Reports the error
   */
  virtual bool ErrorHandler(sbError const & aError);
  
  /**
   * Registers the application to startup when the user logs in
   */
  virtual sbError RegisterForStartOnLogin();
  
  /**
   * Registers the processor's profile
   */
  virtual void RegisterProfile(std::string const & aProfileName);
  
  /**
   * Returns true if there are any tasks file ready to process
   */
  virtual bool TaskFileExists();
  
  /**
   * Unregisters the application to startup when the user logs in
   */
  virtual sbError UnregisterForStartOnLogin();
 
  /**
   * Returns true if the agent is already running as another process.
   */
  virtual bool GetIsAgentRunning();

  /**
   * Waits for the iTunes process to start
   */
  virtual sbError WaitForiTunes();

  /**
   * Finds and sigkills all the agent processes that the user is currently 
   * running.
   */
  virtual sbError KillAllAgents();
protected:
  /**
   * Adds a track to the iTunes database given a path
   */
  virtual sbError AddTracks(std::string const & aSource,
                            TrackList const & aPaths);

  /**
   * Adds a track to the iTunes database given a path
   */
  virtual sbError UpdateTracks(TrackList const & aPaths);
  
  /**
   * Creates a playlist (Recreates it if it already exists)
   */
  virtual sbError CreatePlaylist(std::string const & aPlaylistName);

  /**
   * Clears the contents of a Songbird playlist.
   */
  virtual sbError ClearPlaylist(std::string const & aPlaylistName);

  /**
   * Performs any initialization necessary. Optional to implement
   */
  virtual sbError Initialize();
  
  /**
   * Retrieve the path to the task file
   */
  virtual bool OpenTaskFile(std::ifstream & aStream);
  
  /**
   * Retrieve the results file
   */
  virtual std::ofstream & OpenResultsFile();

  /**
   * Logs the message to the platform specific log device
   */
  virtual void Log(std::string const & aMsg);
  
  /**
   * Removes a playlist from the iTunes database
   */
  virtual sbError RemovePlaylist(std::string const & aPlaylist);
  
  /**
   * Returns true if we should shutdown
   */
  virtual bool ShouldShutdown();
  
  /**
   * Sleep for x milliseconds
   */
  virtual void Sleep(unsigned long aMilliseconds);
private:
  sbiTunesLibrary miTunesLibrary;
  std::wstring mCurrentTaskFile;
  HANDLE mAppExistsMutex;
  std::ofstream mOutputStream;
  sbAutoCOMInitializer mAutoCOMInit;
  /**
   * Callback for the app watcher to know when a shutdown
   * has been issued.
   */
  bool ShutdownCallback(bool);
};

#endif /* SBITUNESAGENTWINDOWSPROCESSOR_H_ */
