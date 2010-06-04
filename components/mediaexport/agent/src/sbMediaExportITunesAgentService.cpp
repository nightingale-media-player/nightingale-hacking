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

#include "sbMediaExportITunesAgentService.h"

#include <nsIProcess.h>
#include <nsIFileURL.h>
#include <nsNetUtil.h>

#ifdef XP_MACOSX
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include "nsILocalFileMac.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <signal.h>
#include <vector>
#include <string>
#elif XP_WIN
#include <windows.h>
#endif

#ifdef XP_MACOSX
//------------------------------------------------------------------------------
// BSD System Process Helper methods

typedef std::vector<pid_t>          sbPIDVector;
typedef sbPIDVector::const_iterator sbPIDVectorIter;

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
    // Get the full path of the execYESutable.
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

nsresult
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
    return NS_ERROR_FAILURE;
  }

  // Create a buffer large enough to hold the list of |kinfo_proc| structs.
  struct kinfo_proc *kp = (struct kinfo_proc *)malloc(bufferLength);
  NS_ENSURE_TRUE(kp != NULL, NS_ERROR_OUT_OF_MEMORY);

  // Get the full list of |kinfo_proc| structs using the newly created buffer.
  if (sysctl(mib, 4, kp, &bufferLength, NULL, 0) < 0) {
    free(kp);
    return NS_ERROR_FAILURE;
  }

  PRInt32 entries = bufferLength / sizeof(struct kinfo_proc);
  if (entries == 0) {
    free(kp);
    return NS_ERROR_FAILURE;
  }

  for (PRInt32 i = entries; i >= 0; i--) {
    std::string curProcessName = GetPidName((&kp[i])->kp_proc.p_pid);
    if (curProcessName.compare(aProcessName) == 0) {
      aOutVector.push_back((&kp[i])->kp_proc.p_pid);
    }
  }

  free(kp);
  return NS_OK;
}

#endif

NS_IMPL_ISUPPORTS1(sbMediaExportITunesAgentService,
                   sbIMediaExportAgentService)

sbMediaExportITunesAgentService::sbMediaExportITunesAgentService()
{
}

sbMediaExportITunesAgentService::~sbMediaExportITunesAgentService()
{
}

NS_IMETHODIMP
sbMediaExportITunesAgentService::StartExportAgent()
{
  return RunAgent(PR_FALSE);
}

NS_IMETHODIMP
sbMediaExportITunesAgentService::UnregisterExportAgent()
{
  return RunAgent(PR_TRUE);
}

nsresult 
sbMediaExportITunesAgentService::RunAgent(PRBool aShouldUnregister)
{
  nsresult rv;
  nsCOMPtr<nsIURI> agentParentFolderURI;
  rv = NS_NewURI(getter_AddRefs(agentParentFolderURI),
                 NS_LITERAL_STRING("resource://app"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> parentFolderFileURL = 
    do_QueryInterface(agentParentFolderURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> agentFile;
  rv = parentFolderFileURL->GetFile(getter_AddRefs(agentFile));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef XP_MACOSX
  // OS X is a little trickier since the agent needs to be started using
  // LaunchServices and not using the path to the binary.
  rv = agentFile->Append(NS_LITERAL_STRING("songbirditunesagent.app"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFileMac> agentMacFile = 
    do_QueryInterface(agentFile, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  FSRef agentFSRef;
  rv = agentMacFile->GetFSRef(&agentFSRef);

  CFArrayRef argv;
  if (aShouldUnregister) {
    // Sadly, argv is a |CFArrayRef| so push the "--unregister" arg into
    // a CoreFoundation array.
    CFStringRef arg[1];
    arg[0] = CFStringCreateWithCString(kCFAllocatorDefault,
                                       "--unregister",
                                       kCFStringEncodingUTF8);
    argv = CFArrayCreate(kCFAllocatorDefault,
                         (const void **)arg,
                         1,
                         NULL);  // callback
  }

  LSApplicationParameters appParams = {
    0,                  // version
    kLSLaunchDefaults,  // launch flags
    &agentFSRef,        // app FSRef
    nsnull,             // asyncLaunchRefCon
    nsnull,             // enviroment variables
    (aShouldUnregister ? argv : nsnull),
    nsnull,             // initial apple event
  };

  ProcessSerialNumber outPSN;
  OSStatus err = LSOpenApplication(&appParams, &outPSN);
  NS_ENSURE_TRUE(err == noErr, NS_ERROR_FAILURE);

  if (aShouldUnregister) {
    CFRelease(argv);
  }

#elif XP_WIN
  // Windows is simple, simply append the name of the agent + '.exe'
  rv = agentFile->Append(NS_LITERAL_STRING("songbirditunesagent.exe"));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIProcess> agentProcess = 
    do_CreateInstance(NS_PROCESS_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = agentProcess->Init(agentFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString args;
  if (aShouldUnregister) {
    args.AppendLiteral("--unregister");
  }
  
  const char *argStr = args.get();
  rv = agentProcess->Run(aShouldUnregister,  // only block for '--unregister' 
                         &argStr, 
                         (aShouldUnregister ? 1 : 0));
  NS_ENSURE_SUCCESS(rv, rv);

#else
  LOG(("%s: ERROR: Tried to start the export agent on a non-supported OS",
        __FUNCTION__));
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportITunesAgentService::GetIsAgentRunning(PRBool *aIsRunning)
{
  NS_ENSURE_ARG_POINTER(aIsRunning);
  *aIsRunning = PR_FALSE;

#ifdef XP_MACOSX
  sbPIDVector processes;
  nsresult rv = GetActiveProcessesByName("songbirditunesagent", processes);
  *aIsRunning = NS_SUCCEEDED(rv) && processes.size() > 0;
#elif XP_WIN
  // The windows agent uses a mutex handle to prevent duplicate agents 
  // from running. Simply check for the mutex to find out if the agent
  // is currently running.
  HANDLE hMutex = OpenMutex(SYNCHRONIZE, PR_TRUE, L"songbirditunesagent");
  *aIsRunning = hMutex != nsnull;
  if (hMutex) {
    CloseHandle(hMutex);
  }
#endif
  
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportITunesAgentService::KillActiveAgents()
{
  // The itunes agent has a "--kill" (or "--roundhouse") argument that 
  // will kill all the active agents for us. Simply get the file spec to the
  // platforms binary (we don't need to go through LS on macosx for this) and
  // spawn a nsIProcess with the "--kill" argument.

  nsresult rv;
  nsCOMPtr<nsIURI> agentParentFolderURI;
  rv = NS_NewURI(getter_AddRefs(agentParentFolderURI),
                 NS_LITERAL_STRING("resource://app"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> parentFolderFileURL = 
    do_QueryInterface(agentParentFolderURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> agentFile;
  rv = parentFolderFileURL->GetFile(getter_AddRefs(agentFile));
  NS_ENSURE_SUCCESS(rv, rv);

#if XP_MACOSX
  // Mac is slightly more tricky because we have to go inside the .app to
  // find the physical binary.
  nsString agentPath;
  rv = agentFile->GetPath(agentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  agentPath.AppendLiteral("/songbirditunesagent.app/Contents/MacOS/songbirditunesagent");

  nsCOMPtr<nsILocalFileMac> macAgentFileSpec =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = macAgentFileSpec->InitWithPath(agentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  agentFile = do_QueryInterface(macAgentFileSpec, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
#elif XP_WIN
  // Windows is simple, simply append the name of the agent + '.exe'
  rv = agentFile->Append(NS_LITERAL_STRING("songbirditunesagent.exe"));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  // Now that the platform specic chunk is done, spawn the process and wait
  // for it to finish.
  nsCOMPtr<nsIProcess> killProcess = 
    do_CreateInstance(NS_PROCESS_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = killProcess->Init(agentFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString args("--kill");
  const char *argsStr = args.get();
  
  // Run the agent with blocking turned on
  rv = killProcess->Run(PR_TRUE, &argsStr, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

