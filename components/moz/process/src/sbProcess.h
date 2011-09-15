/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef __SB_PROCESS_H__
#define __SB_PROCESS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird process services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbProcess.h
 * \brief Songbird Process Services Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird process imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbMemoryUtils.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIThread.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <prmon.h>
#include <prproces.h>


//------------------------------------------------------------------------------
//
// Songbird process services classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements a Songbird process.
 *
 * XXXeps TODO: Make this a full-blown component so JS can use it.
 * XXXeps TODO: Add support for providing a done callback as well as a monitor
 *              This callback should be an sbI*Callback declared as a function
 *              interface so JS code can pass a JS function as a callback.
 */

class sbProcess : public nsISupports
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // Interface implementations.
  //

  NS_DECL_ISUPPORTS


  //
  // Public services.
  //

  /**
   * Create a new process object and return it in aProcess.
   *
   * \param aProcess              Returned created process object.
   */
  static nsresult New(sbProcess** aProcess);

  /**
   * Start running the process.
   */
  nsresult Run();

  /**
   * Kill the process.
   */
  nsresult Kill();

  /**
   * Destroy a process object.
   */
  virtual ~sbProcess();

  /**
   * The list of process command line arguments.  The first argument in the list
   * is the name of the process executable.
   */
  nsresult GetArgList(nsTArray<nsString>& aArgList);
  nsresult SetArgList(const nsTArray<nsString>& aArgList);

  /**
   * The string that will be piped to the process stdin.
   */
  nsresult GetStdinString(nsAString& aStdinString);
  nsresult SetStdinString(const nsAString& aStdinString);

  /**
   * The string that was piped from the process stdout.
   */
  nsresult GetStdoutString(nsAString& aStdoutString);

  /**
   * If true, the process stdout will be piped to the output string.
   */
  nsresult GetPipeStdoutString(PRBool* aPipeStdoutString);
  nsresult SetPipeStdoutString(PRBool aPipeStdoutString);

  /**
   * Monitor to apply to the Songbird process done flag.  The monitor will be
   * notified when the done condition changes.
   */
  nsresult GetDoneMonitor(PRMonitor** aDoneMonitor);
  nsresult SetDoneMonitor(PRMonitor* aDoneMonitor);

  /**
   *   If true, the Songbird process is done.  The Songbird process is done when
   * either the underlying process has exited or after an internal error occurs.
   *   If the Songbird process finishes due to process exit, the done result
   * will be NS_OK, and the process exit code will be valid.
   *   If an internal error occurs, it will be set in the done result and the
   * process exit code will not be valid.
   */
  nsresult GetIsDone(PRBool* aIsDone);

  /**
   * Songbird process object end result when done.  Only valid when done is
   * true.
   */
  nsresult GetDoneResult(nsresult* aDoneResult);

  /**
   * Process exit code.  Only valid if process is done with a successful result.
   */
  nsresult GetExitCode(PRInt32* aExitCode);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Private fields.
  //
  //   mProcessLock             Lock used to serialize access to fields.
  //   mArgList                 List of process command line arguments.
  //   mPipeStdinString         If true, pipe string to process stdin.
  //   mStdinString             String to pipe to process stdin.
  //   mPipeStdoutString        If true, pipe process stdout to string.
  //   mStdoutString            String containing content of process stdout.
  //   mDoneResult              Final result of process when it's done.
  //   mExitCode                Process exit code.
  //   mBaseProcess             Base process object.
  //   mStdinReadFD             File descriptor for reading the process stdin.
  //   mStdoutReadFD            File descriptor for reading the process stdout.
  //   mStdoutWriteFD           File descriptor for writing the process stdout.
  //   mWaitForDoneThread       Thread used to wait for the process to be done.
  //   mHasBeenKilled           If true, process has been killed.
  //
  //   mDoneMonitor             Monitor to notify when process is done.
  //   mIsDone                  True if process is done.
  //

  PRLock*                       mProcessLock;
  nsTArray<nsString>            mArgList;
  PRBool                        mPipeStdinString;
  nsString                      mStdinString;
  PRBool                        mPipeStdoutString;
  nsString                      mStdoutString;
  nsresult                      mDoneResult;
  PRInt32                       mExitCode;
  PRProcess*                    mBaseProcess;
  PRFileDesc*                   mStdinReadFD;
  PRFileDesc*                   mStdoutReadFD;
  PRFileDesc*                   mStdoutWriteFD;
  nsCOMPtr<nsIThread>           mWaitForDoneThread;
  PRBool                        mHasBeenKilled;

  PRMonitor*                    mDoneMonitor;
  PRBool                        mIsDone;


  //
  // Private process services.
  //

  /**
   * Construct a process object.
   */
  sbProcess();

  /**
   * Wait until the process is done.  When the process is done, either
   * successfully or not, update process state and notify the process done
   * monitor.  This method is intended to be run on its own thread.
   */
  void WaitForDone();

  /**
   * Handle a process done event.  Must be called under the process lock.
   */
  nsresult HandleDone();
};


//
// Auto-disposal class wrappers.
//
//   sbAutoPRProcessAttr        Wrapper to auto-destroy process attributes.
//   sbAutoPRFileDesc           Wrapper to auto-close an NSPR file descriptor.
//   sbAutoKillProcess          Wrapper to auto-kill a Songbird process.
//

SB_AUTO_NULL_CLASS(sbAutoPRProcessAttr,
                   PRProcessAttr*,
                   PR_DestroyProcessAttr(mValue));
SB_AUTO_NULL_CLASS(sbAutoPRFileDesc,
                   PRFileDesc*,
                   PR_Close(mValue));
SB_AUTO_NULL_CLASS(sbAutoKillProcess,
                   sbProcess*,
                   mValue->Kill());


//------------------------------------------------------------------------------
//
// Songbird process services.
//
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
                       const nsAString*    aStdin = nsnull,
                       PRBool              aPipeStdoutString = PR_FALSE,
                       PRMonitor*          aDoneMonitor = nsnull);


#endif // __SB_PROCESS_H__

