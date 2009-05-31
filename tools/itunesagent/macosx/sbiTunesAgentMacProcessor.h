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

#ifndef sbiTunesAgentMacProcessor_h_
#define sbiTunesAgentMacProcessor_h_

#include <fstream>
#include "sbiTunesAgentProcessor.h"
#include "sbiTunesLibrary.h"


class sbiTunesAgentMacProcessor : public sbiTunesAgentProcessor
{
public:
  sbiTunesAgentMacProcessor();
  virtual ~sbiTunesAgentMacProcessor();

  // sbiTunesAgentProcessor
  virtual bool TaskFileExists();
  virtual void RemoveTaskFile();
  virtual sbError WaitForiTunes();
  virtual bool ErrorHandler(sbError const & aError);
  virtual sbError RegisterForStartOnLogin();
  virtual sbError UnregisterForStartOnLogin();
  virtual bool GetIsAgentRunning();
  virtual sbError KillAllAgents();
  sbError ProcessTaskFile();

protected:
  virtual sbError AddTracks(std::string const & aSource,
                            Tracks const & aPaths);
  virtual sbError RemovePlaylist(std::string const & aPlaylistName);
  virtual sbError CreatePlaylist(std::string const & aPlaylistName);
  virtual bool OpenTaskFile(std::ifstream & aStream);
  virtual void Log(std::string const & aMsg);
  virtual bool ShouldShutdown();
  virtual void Sleep(unsigned long aMilliseconds);
  virtual void ShutdownDone();

  // Get the next taskfile by it's absolute path.
  std::string GetNextTaskfilePath();
  bool GetIsItunesRunning();
  
private:
  std::string            mCurrentTaskFile;
  sbiTunesLibraryManager *mLibraryMgr;
};

#endif  // sbiTunesAgentMacProcessor_h_

