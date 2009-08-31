/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows event logging services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsEventLog.cpp
 * \brief Songbird Windows Event Logging Services Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows event logging services imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsEventLog.h"

// Windows imports.
#include <windows.h>

// Std C imports.
#include <stdio.h>


//------------------------------------------------------------------------------
//
// Songbird Windows event logging services configuration.
//
//------------------------------------------------------------------------------

//
// SB_WINDOWS_EVENT_LOG_EVENT_SOURCE_NAME   Name of Songbird logging event
//                                          source name.
//

#define SB_WINDOWS_EVENT_LOG_EVENT_SOURCE_NAME "Songbird"


//------------------------------------------------------------------------------
//
// Internal Songbird Windows event logging services prototypes.
//
//------------------------------------------------------------------------------

void sbWindowsEventLogInitialize();


//------------------------------------------------------------------------------
//
// Songbird Windows event logging services globals.
//
//------------------------------------------------------------------------------

//
// gSBWindowsEventLogInitialized    True if the Songbird Windows event logging
//                                  services have been initialized.
// gSBWindowsEventLogEventSource    Event source for Songbird Windows events.
//

static BOOL gSBWindowsEventLogInitialized = FALSE;
static HANDLE gSBWindowsEventLogEventSource = INVALID_HANDLE_VALUE;


//------------------------------------------------------------------------------
//
// Songbird Windows event logging services.
//
//------------------------------------------------------------------------------

/**
 * \brief Log the printf style message specified by aMsg using the format
 *        arguments.
 *
 * \param aMsg                  Log message.
 */

void
sbWindowsEventLog(const char* aMsg, ...)
{
  // Ensure the services are initialized.
  sbWindowsEventLogInitialize();

  // Produce the log message.
  char msg[256];
  va_list argList;
  va_start(argList, aMsg);
  vsnprintf(msg, sizeof(msg), aMsg, argList);
  va_end(argList);

  // Convert the log message to unicode.
  wchar_t wMsg[256];
  MultiByteToWideChar(CP_UTF8, 0, msg, -1, wMsg, sizeof(wMsg));

  // Report the log event.
  LPCWSTR msgList[1] = { wMsg };
  ReportEvent(gSBWindowsEventLogEventSource,
              EVENTLOG_INFORMATION_TYPE,
              0,
              0,
              NULL,
              1,
              0,
              msgList,
              NULL);
}


//------------------------------------------------------------------------------
//
// Internal Songbird Windows event logging services.
//
//------------------------------------------------------------------------------

/**
 * \brief Initialize the Songbird Windows event logging services.
 */

void
sbWindowsEventLogInitialize()
{
  // Do nothing if already initialized.
  if (gSBWindowsEventLogInitialized)
    return;

  // Register the event source.
  gSBWindowsEventLogEventSource = RegisterEventSourceA
                                    (NULL,
                                     SB_WINDOWS_EVENT_LOG_EVENT_SOURCE_NAME);

  // Service is initialized.
  gSBWindowsEventLogInitialized = TRUE;
}


