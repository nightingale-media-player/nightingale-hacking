/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

#ifndef __SB_IPD_LOG_H__
#define __SB_IPD_LOG_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device logging services defs.
//
//   These services provide support for development debugging and end-user field
// logging.
//   Debug logging uses the NSPR logging services.  Use the LOG macro for debug
// logs.  Debug log output can be enabled by setting the environment variable
// NSPR_LOG_MODULES to sbIPDModule:5.
//   Use the FIELD_LOG macro of field logs.  These can be enabled by setting the
// songbird.ipod.logging.enabled boolean preference to true.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDLog.h
 * \brief Songbird iPod Device Logging Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device logging imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nscore.h>
#include <prlog.h>

// Standard C imports.
#include <stdio.h>


//------------------------------------------------------------------------------
//
// iPod device logging services definitions.
//
//------------------------------------------------------------------------------

//
// IPD_LOG_MODULE_NAME              Name of log module.
// IPD_LOG_FIELD_LOG_ENABLE_PREF    Field logging enable preference name.
//

#define IPD_LOG_MODULE_NAME "sbIPDModule"
#define IPD_LOG_FIELD_LOG_ENABLE_PREF "songbird.ipod.logging.enabled"


//------------------------------------------------------------------------------
//
// iPod device logging services classes.
//
//------------------------------------------------------------------------------

/**
 * This class manages device logging.
 */

class sbIPDLog
{
  //
  // Public interface.
  //
public:
  static void Initialize();


  //
  // mInitialized               True if the logging services have been
  //                            initialized.
  // mLogModuleInfo             Log module information.
  // mFieldLoggingEnabled       If true, field logging is enabled.
  //

  static bool                 mInitialized;
#ifdef PR_LOGGING
  static PRLogModuleInfo*       mLogModuleInfo;
#endif
  static bool                 mFieldLoggingEnabled;
};


//------------------------------------------------------------------------------
//
// iPod device logging services macros.
//
//------------------------------------------------------------------------------

/**
 * Output the logging info specified by args if field logging is enabled.
 *
 * \param args                  Logging arguments in printf format.
 */

#define FIELD_LOG(args)                                                        \
{                                                                              \
  if (sbIPDLog::mFieldLoggingEnabled) {                                        \
    printf("sbIPDLog: ");                                                      \
    printf args;                                                               \
  }                                                                            \
}


#endif /* __SB_IPD_LOG_H__ */

