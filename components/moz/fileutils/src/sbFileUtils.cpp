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
// Songbird file utilities.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbFileUtils.cpp
 * \brief Songbird File Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird file utilities imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbFileUtils.h"

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsILocalFile.h>
#include <nsMemory.h>
#include <nsStringGlue.h>

// Windows imports.
#ifdef XP_WIN
#include <direct.h>
#include <windows.h>
#endif

// Std C imports.
#ifndef XP_WIN
#include <unistd.h>
#endif


/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbFileUtils:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gFileUtils = nsnull;
#define TRACE(args) PR_LOG(gFileUtils, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gFileUtils, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif


//------------------------------------------------------------------------------
//
// Songbird file utilities defs.
//
//------------------------------------------------------------------------------

//
// MAXPATHLEN                   Maximum file path length.
//

#ifndef MAXPATHLEN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#elif defined(MAX_PATH)
#define MAXPATHLEN MAX_PATH
#elif defined(_MAX_PATH)
#define MAXPATHLEN _MAX_PATH
#elif defined(CCHMAXPATH)
#define MAXPATHLEN CCHMAXPATH
#else
#define MAXPATHLEN 1024
#endif
#endif


//------------------------------------------------------------------------------
//
// Songbird file utilities nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileUtils, sbIFileUtils);


//------------------------------------------------------------------------------
//
// Songbird file utilities sbIFileUtils implementation.
//
//------------------------------------------------------------------------------

//
// Getters/setters.
//

//-------------------------------------
//
// currentDir
//

NS_IMETHODIMP
sbFileUtils::GetCurrentDir(nsIFile** aCurrentDir)
{
  TRACE(("FileUtils[0x%x] - GetCurrentDir", this));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCurrentDir);

  // Function variables.
  nsresult rv;

  // Get the current directory.
  nsCOMPtr<nsILocalFile> currentDir;
#ifdef XP_WIN
  WCHAR currentDirPath[MAX_PATH];
  NS_ENSURE_TRUE(_wgetcwd(currentDirPath, NS_ARRAY_LENGTH(currentDirPath)),
                 NS_ERROR_FAILURE);
  rv = NS_NewLocalFile(nsDependentString(currentDirPath),
                       PR_TRUE,
                       getter_AddRefs(currentDir));
  NS_ENSURE_SUCCESS(rv, rv);
#else
  char currentDirPath[MAXPATHLEN];
  NS_ENSURE_TRUE(getcwd(currentDirPath, NS_ARRAY_LENGTH(currentDirPath)),
                 NS_ERROR_FAILURE);
  rv = NS_NewNativeLocalFile(nsDependentCString(currentDirPath),
                             PR_TRUE,
                             getter_AddRefs(currentDir));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  // Return results.
  rv = CallQueryInterface(currentDir, aCurrentDir);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFileUtils::SetCurrentDir(nsIFile* aCurrentDir)
{
  TRACE(("FileUtils[0x%x] - SetCurrentDir", this));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCurrentDir);

  // Function variables.
  nsresult rv;

  // Get the new current directory path.
  nsAutoString currentDirPath;
  rv = aCurrentDir->GetPath(currentDirPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the new current directory.
#ifdef XP_WIN
  NS_ENSURE_FALSE(_wchdir(currentDirPath.get()), NS_ERROR_FAILURE);
#else
  NS_ENSURE_FALSE(chdir(NS_ConvertUTF16toUTF8(currentDirPath).get()),
                  NS_ERROR_FAILURE);
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbFileUtils::GetExactPath(const nsAString& aFilePath, nsAString& aExactPath)
{
  TRACE(("FileUtils[0x%x] - GetExactPath", this));

  // Default to an empty string
  aExactPath.Truncate();

#ifdef XP_WIN
  long length = 0;
  nsAutoArrayPtr<WCHAR> shortPath = NULL;
  nsAutoArrayPtr<WCHAR> longPath = NULL;
  // First obtain the size needed by passing NULL and 0.
  length = ::GetShortPathNameW(aFilePath.BeginReading(),
                               NULL,
                               0);
  if (length == 0) return NS_OK;

  // Dynamically allocate the correct size
  shortPath = new WCHAR[length];
  length = ::GetShortPathNameW(aFilePath.BeginReading(),
                               shortPath,
                               length);
  if (length == 0) return NS_OK;

  length = 0;
  // First obtain the size needed by passing NULL and 0.
  length = ::GetLongPathNameW(shortPath,
                              NULL,
                              0);
  if (length == 0) return NS_OK;


  // Dynamically allocate the correct size
  longPath = new WCHAR[length];
  length = ::GetLongPathNameW(shortPath,
                              longPath,
                              length);
  if (length == 0) return NS_OK;

  aExactPath.Assign(longPath);
#endif

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Songbird file utilities public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird file utilities object.
 */

sbFileUtils::sbFileUtils()
{
#ifdef PR_LOGGING
  if (!gFileUtils)
    gFileUtils = PR_NewLogModule("sbFileUtils");
#endif

  TRACE(("FileUtils[0x%x] - Created", this));
}


/**
 * Destroy a Songbird file utilities object.
 */

sbFileUtils::~sbFileUtils()
{
  TRACE(("FileUtils[0x%x] - Destroyed", this));
}

