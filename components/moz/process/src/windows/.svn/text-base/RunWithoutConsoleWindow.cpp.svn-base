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

/**
 * \file  RunWithoutConsoleWindow.cpp
 * \brief Run Without Console Window Main Entry Point Source.
 */

//------------------------------------------------------------------------------
//
// Run without console window imported services.
//
//------------------------------------------------------------------------------

// Windows imports.
#include <fcntl.h>
#include <io.h>
#include <windows.h>

// Std C imports.
#include <stdio.h>


//------------------------------------------------------------------------------
//
// Internal service prototypes.
//
//------------------------------------------------------------------------------

static BOOL IsConsolePresent();

static void InitStdio();


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 *   Run a new process with the command line specified by lpCmdLine without a
 * console window.
 *   This program may be used as a wrapper around a target console application.
 * This program prevents a new console window from being created when the
 * console application is launched.  It passes through stdio and the target
 * exit code to the calling process.
 *   This programs is provided for applications that need to run console
 * applications but don't directly call CreateProcess.  These might be
 * applications that are using some other library for creating processes (e.g.,
 * NSPR).
 *
 * Usage:
 *   RunWithoutConsoleWindow <cmd line>
 *
 * \param hInstance             Current instance of application.
 * \param hPrevInstance         Previous instance of application.
 * \param lpCmdLine             Application command line excluding application
 *                              name.
 * \param nCmdShow              Specifies how the window is to be shown.
 *
 * \return                      Target application exit code.
 */

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPWSTR    lpCmdLine,
                    int       nCmdShow)
{
  BOOL success;

  // Try attaching to parent's console.
  AttachConsole(ATTACH_PARENT_PROCESS);

  // Check if a console is present.
  BOOL isConsolePresent = IsConsolePresent();

  // Initialize stdio for debugging purposes.
  InitStdio();

  // If no console is present, create the target process without a window.
  DWORD creationFlags = 0;
  if (!isConsolePresent)
    creationFlags |= CREATE_NO_WINDOW;

  // Configure the target process to use our stdio handles.
  STARTUPINFO startupInfo;
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.dwFlags = STARTF_USESTDHANDLES;
  startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

  // Create the target process.
  PROCESS_INFORMATION processInfo;
  success = CreateProcessW(NULL,              // lpApplicationName.
                           lpCmdLine,         // lpCommandLine.
                           NULL,              // lpProcessAttributes.
                           NULL,              // lpThreadAttributes.
                           TRUE,              // bInheritHandles.
                           creationFlags,     // dwCreationFlags.
                           NULL,              // lpEnvironment.
                           NULL,              // lpCurrentDirectory.
                           &startupInfo,      // lpStartupInfo.
                           &processInfo);     // lpProcessInformation.
  if (!success) {
    fprintf(stderr, "Internal error %d creating process.\n", GetLastError());
    return -1;
  }

  // Wait for the target process to complete.
  WaitForSingleObject(processInfo.hProcess, INFINITE);

  // Get the target process exit code.
  DWORD exitCode;
  success = GetExitCodeProcess(processInfo.hProcess, &exitCode);
  if (!success) {
    fprintf(stderr, "Internal error %d getting exit code.\n", GetLastError());
    return -1;
  }

  return exitCode;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Return true if a console is present.
 *
 * \return                      True if console is present.
 */

static BOOL IsConsolePresent()
{
  // Try opening the console output.  If this succeeds, a console is present.
  HANDLE conoutHandle = CreateFileW(L"CONOUT$",
                                    GENERIC_WRITE,
                                    FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
  if (conoutHandle == INVALID_HANDLE_VALUE)
    return FALSE;

  // Close the console output handle.
  CloseHandle(conoutHandle);

  return TRUE;
}


/**
 * Initialize stdio.
 */

static void InitStdio()
{
  int   osfHandle;
  FILE* file;

  // Initialize stdin.
  file = NULL;
  osfHandle = _open_osfhandle
                (reinterpret_cast<intptr_t>(GetStdHandle(STD_INPUT_HANDLE)),
                 _O_TEXT);
  if (osfHandle != -1)
    file = _fdopen(osfHandle, "r");
  if (file)
    *stdin = *file;

  // Initialize stdout.
  file = NULL;
  osfHandle = _open_osfhandle
                (reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)),
                 _O_TEXT);
  if (osfHandle != -1)
    file = _fdopen(osfHandle, "w");
  if (file) {
    *stdout = *file;
    setvbuf(stdout, NULL, _IONBF, 0);
  }

  // Initialize stderr.
  file = NULL;
  osfHandle = _open_osfhandle
                (reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)),
                 _O_TEXT);
  if (osfHandle != -1)
    file = _fdopen(osfHandle, "w");
  if (file) {
    *stderr = *file;
    setvbuf(stderr, NULL, _IONBF, 0);
  }
}

