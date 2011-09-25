/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale temporary file factory.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileFactory.cpp
 * \brief Nightingale Temporary File Factory Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale temporary file factory imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbTemporaryFileFactory.h"

// Nightingale imports.
#include <sbFileUtils.h>
#include <sbITemporaryFileService.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Nightingale temporary file factory nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTemporaryFileFactory, sbITemporaryFileFactory)


//------------------------------------------------------------------------------
//
// Nightingale temporary file factory sbITemporaryFileFactory implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// CreateFile
//

NS_IMETHODIMP
sbTemporaryFileFactory::CreateFile(PRUint32         aType,
                                   const nsAString& aBaseName,
                                   const nsAString& aExtension,
                                   nsIFile**        _retval)
{
  // Validate arguments and state.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Ensure the root temporary directory exists.
  rv = EnsureRootTemporaryDirectory();
  NS_ENSURE_SUCCESS(rv, rv);

  // Start the temporary file object in the root temporary directory.
  nsCOMPtr<nsIFile> file;
  rv = mRootTemporaryDirectory->Clone(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce file name from base file name and extension and append to temporary
  // file object.
  nsAutoString fileName;
  if (!aBaseName.IsEmpty()) {
    fileName.Assign(aBaseName);
  }
  else {
    fileName.Assign(NS_LITERAL_STRING("tmp"));
    AppendInt(fileName, PR_Now());
  }
  if (!aExtension.IsEmpty()) {
    fileName.Append(NS_LITERAL_STRING("."));
    fileName.Append(aExtension);
  }
  rv = file->Append(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Determine the desired temporary file permissions.
  PRUint32 permissions;
  if (aType == nsIFile::DIRECTORY_TYPE)
    permissions = SB_DEFAULT_DIRECTORY_PERMISSIONS;
  else
    permissions = SB_DEFAULT_FILE_PERMISSIONS;

  // Create a unique temporary file.
  rv = file->CreateUnique(aType, permissions);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  file.forget(_retval);

  return NS_OK;
}


NS_IMETHODIMP
sbTemporaryFileFactory::Clear()
{
  // Remove root temporary directory.
  if (mRootTemporaryDirectory)
    mRootTemporaryDirectory->Remove(PR_TRUE);
  mRootTemporaryDirectory = nsnull;

  return NS_OK;
}


//
// Getters/setters.
//

//-------------------------------------
//
// rootTemporaryDirectory
//

NS_IMETHODIMP
sbTemporaryFileFactory::SetRootTemporaryDirectory
                          (nsIFile* aRootTemporaryDirectory)
{
  mRootTemporaryDirectory = aRootTemporaryDirectory;
  return NS_OK;
}

NS_IMETHODIMP
sbTemporaryFileFactory::GetRootTemporaryDirectory
                          (nsIFile** aRootTemporaryDirectory)
{
  NS_ENSURE_ARG_POINTER(aRootTemporaryDirectory);
  nsresult rv = EnsureRootTemporaryDirectory();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ADDREF(*aRootTemporaryDirectory = mRootTemporaryDirectory);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Nightingale temporary file factory public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbTemporaryFileFactory
//

sbTemporaryFileFactory::sbTemporaryFileFactory()
{
}


//-------------------------------------
//
// ~sbTemporaryFileFactory
//

sbTemporaryFileFactory::~sbTemporaryFileFactory()
{
  // Clear the temporary files.
  Clear();
}


//------------------------------------------------------------------------------
//
// Nightingale temporary file factory private services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// EnsureRootTemporaryDirectory
//

nsresult
sbTemporaryFileFactory::EnsureRootTemporaryDirectory()
{
  nsresult rv;

  // Ensure that the root temporary directory exists.
  if (!mRootTemporaryDirectory) {
    // Get the temporary file services.
    nsCOMPtr<sbITemporaryFileService>
      temporaryFileService =
        do_GetService("@getnightingale.com/Nightingale/TemporaryFileService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Create a root temporary directory.
    rv = temporaryFileService->CreateFile
                                 (nsIFile::DIRECTORY_TYPE,
                                  SBVoidString(),
                                  SBVoidString(),
                                  getter_AddRefs(mRootTemporaryDirectory));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


