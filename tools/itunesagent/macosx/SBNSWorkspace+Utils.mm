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

#import "SBNSWorkspace+Utils.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <signal.h>

#include <vector>
#include <string>

typedef std::vector<kinfo_proc>     sbKINFOVector;
typedef sbKINFOVector::iterator     sbKINFOVectorIter;
typedef std::vector<pid_t>          sbPIDVector;
typedef sbPIDVector::const_iterator sbPIDVectorIter;


//------------------------------------------------------------------------------
// BSD System Process Helper methods 

std::string
GetPidName(pid_t aPid)
{
  std::string processName;

  int mib[] = {
    CTL_KERN,
    KERN_PROCARGS,
    aPid
  };

  // Get the size of the buffer needed for the process name.
  size_t dataLength = 0;
  if (sysctl(mib, 3, NULL, &dataLength, NULL, 0) >= 0) {
    // Get the full path of the executable.
    std::string processPath;
    processPath.resize(dataLength);

    if (sysctl(mib, 3, &processPath[0], &dataLength, NULL, 0) >= 0) {
      // Find the last part of the path to get the executable name.
      size_t endIndex = processPath.find('\0');
      size_t lastSlashIndex = processPath.rfind('/', endIndex);
      if (lastSlashIndex != std::string::npos) {
        processName = processPath.substr(lastSlashIndex + 1,
                                         endIndex - lastSlashIndex - 1);
      }
    }
  }

  return processName;
}

sbPIDVector
GetActiveProcessesByName(const std::string & aProcessName)
{
  sbPIDVector processVector;

  int mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_UID,
    geteuid()
  };

  // Get the size of the kinfo_proc array buffer.
  size_t bufferLength = 0;
  if (sysctl(mib, 4, NULL, &bufferLength, NULL, 0) < 0) {
    return processVector;
  }

  // Set the size of the vector storage.
  sbKINFOVector kinfoVector;
  kinfoVector.resize(bufferLength / sizeof(struct kinfo_proc));
  
  // Get the lists of processes using the buffer space.
  if (sysctl(mib, 4, &kinfoVector[0], &bufferLength, NULL, 0) < 0) {
    return processVector;
  }

  // Now find any matching process names.
  sbKINFOVectorIter begin = kinfoVector.begin();
  sbKINFOVectorIter end = kinfoVector.end();
  sbKINFOVectorIter next;
  for (next = begin; next != end; ++next) {
    // Don't bother appending the current process ID.
    if ((*next).kp_proc.p_pid != getpid()) {
      std::string curProcessName = GetPidName((*next).kp_proc.p_pid);
      if (curProcessName.compare(aProcessName) == 0) {
        processVector.push_back((*next).kp_proc.p_pid);
      }
    }
  }

  return processVector;
}

inline void
SigKill(pid_t aPid)
{
  kill(aPid, SIGKILL);
}

//------------------------------------------------------------------------------

@implementation NSWorkspace (SongbirdProcessUtils)

+ (BOOL)isProcessAlreadyRunning:(NSString *)aProcessName
{
  sbPIDVector const & processes = 
    GetActiveProcessesByName([aProcessName UTF8String]); 
  return processes.size() > 0;
}

+ (void)killAllRunningProcesses:(NSString *)aProcessName
{
  sbPIDVector const & processes = 
    GetActiveProcessesByName([aProcessName UTF8String]);
  
  std::for_each(processes.begin(), processes.end(), SigKill);
}

@end

