/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
// Songbird service manager.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbServiceManager.cpp
 * \brief Songbird Service Manager Source.
 */

//------------------------------------------------------------------------------
//
// Songbird service manager imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbServiceManager.h"

// Songbird imports.
#include <sbThreadUtilsXPCOM.h>

// Mozilla imports.
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird service manager logging services.
//
//------------------------------------------------------------------------------

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbServiceManager:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gServiceManagerLog = nsnull;
#define TRACE(args) PR_LOG(gServiceManagerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gServiceManagerLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// Songbird service manager nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbServiceManager, sbIServiceManager)


//------------------------------------------------------------------------------
//
// Songbird service manager sbIServiceManager implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// IsServiceReady
//

NS_IMETHODIMP
sbServiceManager::IsServiceReady(const char* aServiceContractID,
                                 PRBool*     retval)
{
  // Validate state and arguments.
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(retval);

  // Service is ready if it's in the ready service table.
  *retval = mReadyServiceTable.Get(NS_ConvertUTF8toUTF16(aServiceContractID),
                                   nsnull);
  TRACE(("%s[%.8x] %s %d", __FUNCTION__, this, aServiceContractID, *retval));

  return NS_OK;
}


//-------------------------------------
//
// SetServiceReady
//

NS_IMETHODIMP
sbServiceManager::SetServiceReady(const char* aServiceContractID,
                                  PRBool      aServiceReady)
{
  TRACE(("%s[%.8x] %s %d",
         __FUNCTION__, this, aServiceContractID, aServiceReady));

  // Validate state.
  NS_ENSURE_STATE(mInitialized);

  // Function variables.
  PRBool   serviceReady = !!aServiceReady;
  PRBool   success;
  nsresult rv;

  // Get the current service ready state.
  PRBool currentServiceReady;
  rv = IsServiceReady(aServiceContractID, &currentServiceReady);
  NS_ENSURE_SUCCESS(rv, rv);

  // Just return if state is not changing.
  if (currentServiceReady == serviceReady)
    return NS_OK;

  // Update the ready service table.
  nsAutoString serviceContractID = NS_ConvertUTF8toUTF16(aServiceContractID);
  if (aServiceReady) {
    // Add the service to the ready service table.
    success = mReadyServiceTable.Put(serviceContractID, PR_TRUE);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    // Send notification that the service is ready.
    rv = mObserverService->NotifyObservers(this,
                                           "service-ready",
                                           serviceContractID.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Send notification that the service is about to be shut down.
    rv = mObserverService->NotifyObservers(this,
                                           "before-service-shutdown",
                                           serviceContractID.get());
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove the service from the ready service table.
    mReadyServiceTable.Remove(serviceContractID);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird service manager public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbServiceManager
//

sbServiceManager::sbServiceManager() :
  mInitialized(PR_FALSE)
{
  // Initialize logging.
#ifdef PR_LOGGING
  if (!gServiceManagerLog) {
    gServiceManagerLog = PR_NewLogModule("sbServiceManager");
  }
#endif

  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


//-------------------------------------
//
// ~sbServiceManager
//

sbServiceManager::~sbServiceManager()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Do nothing if not initialized.
  if (!mInitialized)
    return;

  // Clear table of ready services.
  mReadyServiceTable.Clear();
}


//-------------------------------------
//
// Initialize
//

nsresult
sbServiceManager::Initialize()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  nsresult rv;

  // Do nothing if already initialized.
  if (mInitialized)
    return NS_OK;

  // Initialize the table of ready services.
  PRBool success = mReadyServiceTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Get a proxied observer service.
  mObserverService = do_GetService("@mozilla.org/observer-service;1",
                                          &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // The service manager is now initialized.
  mInitialized = PR_TRUE;

  return NS_OK;
}

