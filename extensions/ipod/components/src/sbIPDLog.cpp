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

/** 
 * \file  sbIPDLog.cpp
 * \brief Songbird iPod Device Logging Source.
 */

//------------------------------------------------------------------------------
//
// iPod device logging imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDLog.h"

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIPrefService.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// iPod device logging services.
//
//------------------------------------------------------------------------------

//
// Static field initializers.
//

PRBool sbIPDLog::mInitialized = PR_FALSE;
#ifdef PR_LOGGING
PRLogModuleInfo* sbIPDLog::mLogModuleInfo = nsnull;
#endif
PRBool sbIPDLog::mFieldLoggingEnabled = PR_FALSE;


/**
 * Initialize the logging services with the log module name specified by
 * aLogModule.  This function may be called more than once, but it will only
 * initialize once.
 *
 * \param aLogModule            Log module name.
 */

void
sbIPDLog::Initialize()
{
  nsresult rv;

  // Do nothing if already initialized.
  if (mInitialized)
    return;

  // Initialize the NSPR logging.
#ifdef PR_LOGGING
  mLogModuleInfo = PR_NewLogModule(IPD_LOG_MODULE_NAME);
#endif

  // Read the field logging enabled preference.
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID,
                                                     &rv);
  if (NS_SUCCEEDED(rv)) {
    prefBranch->GetBoolPref(IPD_LOG_FIELD_LOG_ENABLE_PREF,
                            &mFieldLoggingEnabled);
  } else {
    NS_WARNING("Failed to get field logging enabled preference.");
  }

  // Mark the logging services as initialized.
  mInitialized = PR_TRUE;
}


