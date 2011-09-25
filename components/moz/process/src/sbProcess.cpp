/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale process services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbProcess.cpp
 * \brief Nightingale Process Services Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale process imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbProcess.h"

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale process class.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Nightingale process nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(sbProcess)


//------------------------------------------------------------------------------
//
// Public process services.
//
//------------------------------------------------------------------------------

/**
 * Create a new process object and return it in aProcess.
 *
 * \param aProcess              Returned created process object.
 */

/* static */ nsresult
sbProcess::New(sbProcess** aProcess)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aProcess);

  // Create a new process object.
  nsRefPtr<sbProcess> process = new sbProcess();
  NS_ENSURE_TRUE(process, NS_ERROR_OUT_OF_MEMORY);

  // Create the process lock.
  process->mProcessLock = nsAutoLock::NewLock("sbProcess::mProcessLock");
  NS_ENSURE_TRUE(process->mProcessLock, NS_ERROR_OUT_OF_MEMORY);

  // Return results.
  process.forget(aProcess);

  return NS_OK;
}


/**
 * Start running the process.
 */

nsresult
sbProcess::Run()
{
  PRStatus status;
  nsresult rv;

  // Set up to auto-kill process.
  sbAutoKillProcess autoSelf(this);

  // Operate under the process lock.
  {
    NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
    nsAutoLock autoProcessLock(mProcessLock);

    // Get the number of arguments.
    PRUint32 argCount = mArgList.Length();
    NS_ENSURE_TRUE(argCount > 0, NS_ERROR_ILLEGAL_VALUE);

    // Convert the UTF-16 argument list to a UTF-8 argument list.
    nsTArray<nsCString> argListUTF8;
    for (PRUint32 i = 0; i < argCount; i++) {
      NS_ENSURE_TRUE(argListUTF8.AppendElement
                                   (NS_ConvertUTF16toUTF8(mArgList[i])),
                     NS_ERROR_OUT_OF_MEMORY);
    }

    // Allocate a null-terminated char* argument list and set it up for
    // auto-disposal.
    char** argList = reinterpret_cast<char**>
                       (NS_Alloc((argCount + 1) * sizeof(char*)));
    NS_ENSURE_TRUE(argList, NS_ERROR_OUT_OF_MEMORY);
    sbAutoNSTypePtr<char*> autoArgList(argList);

    // Convert the argument list to a null-terminated char* argument list.
    for (PRUint32 i = 0; i < argCount; i++) {
      argList[i] = const_cast<char*>(argListUTF8[i].get());
    }
    argList[argCount] = NULL;

    // Set up the process attributes and set them up for auto-disposal.
    PRProcessAttr* processAttr = PR_NewProcessAttr();
    NS_ENSURE_TRUE(processAttr, NS_ERROR_FAILURE);
    sbAutoPRProcessAttr autoProcessAttr(processAttr);

    // Set up process stdin.
    if (mPipeStdinString) {
      // Create a process stdin pipe and set it up for auto-disposal.
      PRFileDesc* stdinReadFD;
      PRFileDesc* stdinWriteFD;
      status = PR_CreatePipe(&stdinReadFD, &stdinWriteFD);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);
      sbAutoPRFileDesc autoStdinReadFD(stdinReadFD);
      sbAutoPRFileDesc autoStdinWriteFD(stdinWriteFD);

      // Set up stdin pipe file descriptors.
      status = PR_SetFDInheritable(stdinReadFD, PR_TRUE);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);
      status = PR_SetFDInheritable(stdinWriteFD, PR_FALSE);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);

      // Fill pipe.
      nsCAutoString writeData = NS_ConvertUTF16toUTF8(mStdinString);
      PRInt32 bytesWritten;
      bytesWritten = PR_Write(stdinWriteFD,
                              writeData.get(),
                              writeData.Length());
      NS_ENSURE_TRUE(bytesWritten == writeData.Length(), NS_ERROR_FAILURE);

      // Redirect process stdin.
      PR_ProcessAttrSetStdioRedirect(processAttr,
                                     PR_StandardInput,
                                     stdinReadFD);

      // Keep stdin read descriptor open for the process to read.  Close the
      // stdin write descriptor so that the process gets EOF when all of the
      // data is read.
      mStdinReadFD = autoStdinReadFD.forget();
      PR_Close(autoStdinWriteFD.forget());
    }

    // Set up process stdout.
    if (mPipeStdoutString) {
      // Create a process stdout pipe and set it up for auto-disposal.
      PRFileDesc* stdoutReadFD;
      PRFileDesc* stdoutWriteFD;
      status = PR_CreatePipe(&stdoutReadFD, &stdoutWriteFD);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);
      sbAutoPRFileDesc autoStdoutReadFD(stdoutReadFD);
      sbAutoPRFileDesc autoStdoutWriteFD(stdoutWriteFD);

      // Set up stdout pipe file descriptors.
      status = PR_SetFDInheritable(stdoutReadFD, PR_FALSE);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);
      status = PR_SetFDInheritable(stdoutWriteFD, PR_TRUE);
      NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);

      // Redirect process stdout.
      PR_ProcessAttrSetStdioRedirect(processAttr,
                                     PR_StandardOutput,
                                     stdoutWriteFD);

      // Keep descriptors.
      mStdoutReadFD = autoStdoutReadFD.forget();
      mStdoutWriteFD = autoStdoutWriteFD.forget();
    }

    // Create and start running the process.
    mBaseProcess = PR_CreateProcess(argList[0], argList, NULL, processAttr);
    NS_ENSURE_TRUE(mBaseProcess, NS_ERROR_FAILURE);

    // Wait for process done on another thread.
    nsCOMPtr<nsIRunnable> runnable = NS_NEW_RUNNABLE_METHOD(sbProcess,
                                                            this,
                                                            WaitForDone);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);
    rv = NS_NewThread(getter_AddRefs(mWaitForDoneThread), runnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Clear process auto-kill.
  autoSelf.forget();

  return NS_OK;
}


/**
 * Kill the process.
 */

nsresult
sbProcess::Kill()
{
  // Do nothing if process already killed.
  {
    NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
    nsAutoLock autoProcessLock(mProcessLock);
    if (mHasBeenKilled)
      return NS_OK;
    mHasBeenKilled = PR_TRUE;
  }

  // Kill the base process.
  if (mBaseProcess)
    PR_KillProcess(mBaseProcess);

  // Shutdown the wait for done thread.
  if (mWaitForDoneThread)
    mWaitForDoneThread->Shutdown();

  return NS_OK;
}


/**
 * Destroy a process object.
 */

sbProcess::~sbProcess()
{
  // Kill the process.
  Kill();

  // Detach the base process.
  //XXXeps detaching the process causes crashes.
#if 0
  if (mBaseProcess)
    PR_DetachProcess(mBaseProcess);
#endif
  mBaseProcess = nsnull;

  // Close the process pipes.
  if (mStdinReadFD)
    PR_Close(mStdinReadFD);
  if (mStdoutReadFD)
    PR_Close(mStdoutReadFD);
  if (mStdoutWriteFD)
    PR_Close(mStdoutWriteFD);

  // Dispose of the process lock.
  if (mProcessLock)
    nsAutoLock::DestroyLock(mProcessLock);
  mProcessLock = nsnull;
}


//
// Getters/setters.
//

/**
 * The list of process command line arguments.  The first argument in the list
 * is the name of the process executable.
 */

nsresult
sbProcess::GetArgList(nsTArray<nsString>& aArgList)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the argument list.
  PRUint32 argCount = mArgList.Length();
  aArgList.Clear();
  for (PRUint32 i = 0; i < argCount; i++) {
    NS_ENSURE_TRUE(aArgList.AppendElement(mArgList[i]), NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbProcess::SetArgList(const nsTArray<nsString>& aArgList)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Set the argument list.
  PRUint32 argCount = aArgList.Length();
  mArgList.Clear();
  for (PRUint32 i = 0; i < argCount; i++) {
    NS_ENSURE_TRUE(mArgList.AppendElement(aArgList[i]), NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}


/**
 * The string that will be piped to the process stdin.
 */

nsresult
sbProcess::GetStdinString(nsAString& aStdinString)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the stdin string.
  if (mPipeStdinString)
    aStdinString.Assign(mStdinString);
  else
    aStdinString.SetIsVoid(PR_TRUE);

  return NS_OK;
}

nsresult
sbProcess::SetStdinString(const nsAString& aStdinString)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Set the stdin string.
  mStdinString.Assign(aStdinString);
  mPipeStdinString = !aStdinString.IsVoid();

  return NS_OK;
}


/**
 * The string that was piped from the process stdout.
 */

nsresult
sbProcess::GetStdoutString(nsAString& aStdoutString)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the stdout string.
  if (mPipeStdoutString)
    aStdoutString.Assign(mStdoutString);
  else
    aStdoutString.SetIsVoid(PR_TRUE);

  return NS_OK;
}


/**
 * If true, the process stdout will be piped to the output string.
 */

nsresult
sbProcess::GetPipeStdoutString(PRBool* aPipeStdoutString)
{
  NS_ENSURE_ARG_POINTER(aPipeStdoutString);

  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the pipe stdout string flag.
  *aPipeStdoutString = mPipeStdoutString;

  return NS_OK;
}

nsresult
sbProcess::SetPipeStdoutString(PRBool aPipeStdoutString)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the pipe stdout string flag.
  mPipeStdoutString = aPipeStdoutString;

  return NS_OK;
}


/**
 * Monitor that will be notified when the process is done.
 */

nsresult
sbProcess::GetDoneMonitor(PRMonitor** aDoneMonitor)
{
  NS_ENSURE_ARG_POINTER(aDoneMonitor);

  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the done monitor.
  *aDoneMonitor = mDoneMonitor;

  return NS_OK;
}

nsresult
sbProcess::SetDoneMonitor(PRMonitor* aDoneMonitor)
{
  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Set the done monitor.
  mDoneMonitor = aDoneMonitor;

  return NS_OK;
}


/**
 *   If true, the Nightingale process is done.  The Nightingale process is done when
 * either the underlying process has exited or after an internal error occurs.
 *   If the Nightingale process finishes due to process exit, the done result
 * will be NS_OK, and the process exit code will be valid.
 *   If an internal error occurs, it will be set in the done result and the
 * process exit code will not be valid.
 */

nsresult
sbProcess::GetIsDone(PRBool* aIsDone)
{
  NS_ENSURE_ARG_POINTER(aIsDone);

  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the done flag.
  *aIsDone = mIsDone;

  return NS_OK;
}


/**
 * Process end result when done.  Only valid when done is true.
 */

nsresult
sbProcess::GetDoneResult(nsresult* aDoneResult)
{
  NS_ENSURE_ARG_POINTER(aDoneResult);

  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the done result.
  *aDoneResult = mDoneResult;

  return NS_OK;
}


/**
 * Process exit code.  Only valid if process is done with a successful result.
 */

nsresult
sbProcess::GetExitCode(PRInt32* aExitCode)
{
  NS_ENSURE_ARG_POINTER(aExitCode);

  // Operate under the process lock.
  NS_ENSURE_TRUE(mProcessLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock autoProcessLock(mProcessLock);

  // Get the exit code.
  *aExitCode = mExitCode;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Private process services.
//
//------------------------------------------------------------------------------

/**
 * Construct a process object.
 */

sbProcess::sbProcess() :
  mProcessLock(nsnull),
  mPipeStdinString(PR_FALSE),
  mPipeStdoutString(PR_FALSE),
  mStdinReadFD(nsnull),
  mStdoutReadFD(nsnull),
  mStdoutWriteFD(nsnull),
  mBaseProcess(nsnull),
  mDoneResult(NS_OK),
  mExitCode(0),
  mHasBeenKilled(PR_FALSE),
  mDoneMonitor(nsnull),
  mIsDone(PR_FALSE)
{
}


/**
 * Wait until the process is done.  When the process is done, either
 * successfully or not, update process state and notify the process done
 * monitor.  This method is intended to be run on its own thread.
 */

void
sbProcess::WaitForDone()
{
  PRStatus status = PR_SUCCESS;
  nsresult rv = NS_OK;

  // Wait for the process to complete and get the exit code.
  PRInt32 exitCode;
  status = PR_WaitProcess(mBaseProcess, &exitCode);
  if (status != PR_SUCCESS)
    rv = NS_ERROR_FAILURE;

  // Operate under the process lock.
  {
    nsAutoLock autoProcessLock(mProcessLock);

    // Handle the process done event.
    if (NS_SUCCEEDED(rv))
      rv = HandleDone();

    // Check done result of process.
    if (NS_SUCCEEDED(rv))
      mExitCode = exitCode;
    mDoneResult = rv;
  }

  // Change the done condition to true.  If a done monitor was provided, change
  // the condition under the monitor and send notification of the change.
  if (mDoneMonitor) {
    nsAutoMonitor monitor(mDoneMonitor);
    {
      nsAutoLock autoProcessLock(mProcessLock);
      mIsDone = PR_TRUE;
    }
    monitor.Notify();
  } else {
    nsAutoLock autoProcessLock(mProcessLock);
    mIsDone = PR_TRUE;
  }
}


/**
 * Handle a process done event.  Must be called under the process lock.
 */

nsresult
sbProcess::HandleDone()
{
  // Read the process stdout if set to do so.
  if (mPipeStdoutString) {
    // Get the stdout read and write file descriptors.
    PRFileDesc* stdoutReadFD = mStdoutReadFD;
    PRFileDesc* stdoutWriteFD = mStdoutWriteFD;

    // Close the write pipe so that reading the pipe returns EOF when all data
    // is read.  Close outside of process lock to avoid any potential for
    // deadlock.
    {
      nsAutoUnlock autoProcessUnlock(mProcessLock);
      PR_Close(stdoutWriteFD);
    }
    mStdoutWriteFD = nsnull;

    // Read the stdout data into the stdout string.
    char readData[512];
    PRInt32 bytesRead;
    mStdoutString.Truncate();
    do {
      // Read the data.  A return value of zero indicates EOF.  Read outside of
      // the process lock to avoid any potential for deadlock.
      {
        nsAutoUnlock autoProcessUnlock(mProcessLock);
        bytesRead = PR_Read(stdoutReadFD, readData, sizeof(readData));
      }
      NS_ENSURE_TRUE(bytesRead >= 0, NS_ERROR_FAILURE);

      // Add the data to the stdout string.
      if (bytesRead) {
        nsCAutoString dataString;
        dataString.Assign(readData, bytesRead);
        mStdoutString.Append(NS_ConvertUTF8toUTF16(dataString));
      }
    } while (bytesRead > 0);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale process services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 *   Start running a new process with the list of command line arguments
 * specified by aArgList.  The first entry in the argument list specifies the
 * process executable.  The executable argument does not need to be a full path.
 *   If aStdin is not null, pipe the string to the process's stdin.  If
 * aPipeStdoutString is true, pipe the process's stdout to a string that can be
 * read from the created process.  If aDoneMonitor is specified, it is notified
 * when the process is done (due to process exit, an internal error, or a kill
 * request).
 *   The new process is returned in aProcess.
 *
 * \param aProcess              Returned created process.
 * \param aArgList              List of process command line arguments.
 * \param aStdin                Optional string to use for process stdin.
 * \param aPipeStdoutString     If true, pipe process stdout to a string.
 * \param aDoneMonitor          Optional monitor to notify when process is done.
 */

nsresult SB_RunProcess(sbProcess**         aProcess,
                       nsTArray<nsString>& aArgList,
                       const nsAString*    aStdin,
                       PRBool              aPipeStdoutString,
                       PRMonitor*          aDoneMonitor)
{
  nsresult rv;

  // Create a Nightingale process.
  nsRefPtr<sbProcess> process;
  rv = sbProcess::New(getter_AddRefs(process));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the Nightingale process.
  rv = process->SetArgList(aArgList);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aStdin) {
    rv = process->SetStdinString(*aStdin);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = process->SetPipeStdoutString(aPipeStdoutString);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = process->SetDoneMonitor(aDoneMonitor);
  NS_ENSURE_SUCCESS(rv, rv);

  // Start running the process.
  rv = process->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  process.forget(aProcess);

  return NS_OK;
}

