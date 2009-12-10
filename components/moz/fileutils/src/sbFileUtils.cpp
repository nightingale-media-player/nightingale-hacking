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

NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileUtils, sbIFileUtils)


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
}


/**
 * Destroy a Songbird file utilities object.
 */

sbFileUtils::~sbFileUtils()
{
}

