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
    char processPathStr[dataLength];
    if (sysctl(mib, 3, &processPathStr[0], &dataLength, NULL, 0) >= 0) {
      // Find the last part of the path to get the executable name.
      std::string processPath(processPathStr);
      size_t lastSlashIndex = processPath.rfind('/');
      if (lastSlashIndex != std::string::npos) {
        processName = processPath.substr(lastSlashIndex + 1);
      }
    }
  }

  return processName;
}

BOOL
GetActiveProcessesByName(const std::string & aProcessName,
                         sbPIDVector & aOutVector)
{
  static int mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_UID,
    geteuid()
  };

  // Get the size of the kinfo_proc array buffer.
  size_t bufferLength = 0;
  if (sysctl(mib, 4, NULL, &bufferLength, NULL, 0) < 0) {
    return NO;
  }

  // Create a buffer large enough to hold the list of |kinfo_proc| structs.
  struct kinfo_proc *kp = (struct kinfo_proc *)malloc(bufferLength);
  if (kp == NULL) {
    return NO;
  }

  // Get the full list of |kinfo_proc| structs using the newly created buffer.
  if (sysctl(mib, 4, kp, &bufferLength, NULL, 0) < 0) {
    free(kp);
    return NO;
  }

  int entries = bufferLength / sizeof(struct kinfo_proc);
  if (entries == 0) {
    free(kp);
    return NO;
  }

  for (int i = entries; i >= 0; i--) {
    // Don't bother appending the current process ID.
    if ((&kp[i])->kp_proc.p_pid != getpid()) {
      std::string curProcessName = GetPidName((&kp[i])->kp_proc.p_pid);
      if (curProcessName.compare(aProcessName) == 0) {
        aOutVector.push_back((&kp[i])->kp_proc.p_pid);
      }
    }
  }
 
  free(kp);
  return YES;
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
  sbPIDVector processes;
  return GetActiveProcessesByName([aProcessName UTF8String], processes) && 
         processes.size() > 0;
}

+ (void)killAllRunningProcesses:(NSString *)aProcessName
{
  sbPIDVector processes;
  if (GetActiveProcessesByName([aProcessName UTF8String], processes)) {
    std::for_each(processes.begin(), processes.end(), SigKill);
  }
}

@end

