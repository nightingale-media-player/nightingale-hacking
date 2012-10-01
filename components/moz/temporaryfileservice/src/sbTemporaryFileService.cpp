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
// Songbird temporary file service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileService.cpp
 * \brief Songbird Temporary File Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird temporary file service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbTemporaryFileService.h"

// Songbird imports.
#include <sbFileUtils.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsDirectoryServiceDefs.h>


//------------------------------------------------------------------------------
//
// Songbird temporary file service nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(sbTemporaryFileService,
                              sbITemporaryFileService,
                              nsIObserver)


//------------------------------------------------------------------------------
//
// Songbird temporary file service sbITemporaryFileService implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// CreateFile
//

NS_IMETHODIMP
sbTemporaryFileService::CreateFile(PRUint32         aType,
                                   const nsAString& aBaseName,
                                   const nsAString& aExtension,
                                   nsIFile**        _retval)
{
  NS_ENSURE_STATE(mInitialized);
  return mRootTemporaryFileFactory->CreateFile(aType,
                                               aBaseName,
                                               aExtension,
                                               _retval);
}


//
// Getters/setters.
//

//-------------------------------------
//
// rootTemporaryDirectory
//

NS_IMETHODIMP
sbTemporaryFileService::GetRootTemporaryDirectory
                          (nsIFile** aRootTemporaryDirectory)
{
  NS_ENSURE_STATE(mInitialized);
  return mRootTemporaryFileFactory->GetRootTemporaryDirectory
                                      (aRootTemporaryDirectory);
}


//------------------------------------------------------------------------------
//
// Songbird temporary file service nsIObserver implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Observer
//

NS_IMETHODIMP
sbTemporaryFileService::Observe(nsISupports*     aSubject,
                                const char*      aTopic,
                                const PRUnichar* aData)
{
  nsresult rv;

  // Dispatch processing of event.
  if (!strcmp(aTopic, "app-startup")) {
    // Initialize the services.
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp(aTopic, "profile-after-change")) {
    // User profile is now available.
    mProfileAvailable = PR_TRUE;

    // Initialize the services.
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp(aTopic, "quit-application")) {
    // Finalize the services.
    Finalize();
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird temporary file service public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbTemporaryFileService
//

sbTemporaryFileService::sbTemporaryFileService() :
  mInitialized(PR_FALSE),
  mProfileAvailable(PR_FALSE)
{
}


//-------------------------------------
//
// ~sbTemporaryFileService
//

sbTemporaryFileService::~sbTemporaryFileService()
{
  // Finalize the Songbird temporary file service.
  Finalize();
}


//-------------------------------------
//
// Initialize
//

nsresult
sbTemporaryFileService::Initialize()
{
  nsresult rv;

  // Add observers.
  if (!mObserverService) {
    mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mObserverService->AddObserver(this, "profile-after-change", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mObserverService->AddObserver(this, "quit-application", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Wait for the user profile to be available.
  if (!mProfileAvailable)
    return NS_OK;

  // Set up the root temporary directory residing in the OS temporary directory.
  nsCOMPtr<nsIFile> rootTemporaryDirectory;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                              getter_AddRefs(rootTemporaryDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  // Ensure the root temporary directory exists.
  bool exists;
  rv = rootTemporaryDirectory->Append
         (NS_LITERAL_STRING(SB_TEMPORARY_FILE_SERVICE_ROOT_DIR_NAME));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootTemporaryDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = rootTemporaryDirectory->Create(nsIFile::DIRECTORY_TYPE,
                                        SB_DEFAULT_DIRECTORY_PERMISSIONS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Create the root temporary file factory.
  mRootTemporaryFileFactory =
    do_CreateInstance("@songbirdnest.com/Songbird/TemporaryFileFactory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mRootTemporaryFileFactory->SetRootTemporaryDirectory
                                    (rootTemporaryDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  // Services are now initialized.
  mInitialized = PR_TRUE;

  return NS_OK;
}


//-------------------------------------
//
// Finalize
//

void
sbTemporaryFileService::Finalize()
{
  // No longer initialized.
  mInitialized = PR_FALSE;

  // Remove observers.
  if (mObserverService) {
    mObserverService->RemoveObserver(this, "profile-after-change");
    mObserverService->RemoveObserver(this, "quit-application");
  }
  mObserverService = nsnull;

  // Remove the temporary file factory.
  mRootTemporaryFileFactory = nsnull;
}

