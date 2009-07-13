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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// C++ source for the Mozilla XPCOM component loader services.
//
//   Based on Benjamin Smedberg's loader at
// http://developer.mozilla.org/en/docs/
// Using_Dependent_Libraries_In_Extension_Components.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbComponentLoader.cpp
 * \brief Component Loader Source.
 */

//------------------------------------------------------------------------------
//
// Component loader services imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsILocalFile.h>
#include <nsModule.h>
#include <nsStringAPI.h>

// Mac OS/X imports.
#ifdef XP_MACOSX
#include <mach-o/dyld.h>
#endif


//------------------------------------------------------------------------------
//
// Component loader services configuration.
//
//------------------------------------------------------------------------------

//
// CLModuleLib                  Component module library to load.
// CLLibList                    List of dependent libraries to load.
//

#if defined(XP_MACOSX)

#ifdef DEBUG
static const char *CLModuleLib = "sbIPDDevice_d.dylib";
#else
static const char *CLModuleLib = "sbIPDDevice.dylib";
#endif

static const char *CLLibList[] =
{
  "libiconv.dylib",
  "libintl.dylib",
  "libglib-2.0.dylib",
  "libgobject-2.0.dylib",
  "libgpod.dylib"
};

#elif defined(linux)

#ifdef DEBUG
static const char *CLModuleLib = "sbIPDDevice_d.so";
#else
static const char *CLModuleLib = "sbIPDDevice.so";
#endif

static const char *CLLibList[] =
{
  "libgpod.so"
};

#elif defined(XP_WIN)

#ifdef DEBUG
static const char *CLModuleLib = "sbIPDDevice_d.dll";
#else
static const char *CLModuleLib = "sbIPDDevice.dll";
#endif

static const char *CLLibList[] =
{
  "iconv.dll",
  "intl.dll",
  "libglib-2.0-0.dll",
  "libgobject-2.0-0.dll",
  "libgpod.dll"
};

#else

#error "Unsupported OS."

#endif


//------------------------------------------------------------------------------
//
// Component loader services functions.
//
//------------------------------------------------------------------------------

extern "C" {

static nsresult LoadLibrary(nsCOMPtr<nsIFile> aLibDir,
                            nsString          aLibPath);


/**
 * Wrapper for a target component's NSGetModule function.  This function loads
 * the target component and any of its dependent libraries and calls the target
 * component's NSGetModule function with the specified function parameters.
 *
 * \param aComponentManager     Component manager services.
 * \param aComponentFile        Component file.
 * \param aComponentModule      Component module.
 */

NS_EXPORT nsresult
NSGetModule(nsIComponentManager* aComponentManager,
            nsIFile*             aComponentFile,
            nsIModule**          aComponentModule)
{
  nsresult rv;

  // Get the libraries directory.
  nsCOMPtr<nsIFile> pLibDir;
  nsCOMPtr<nsIFile> _pLibDir;
  rv = aComponentFile->GetParent(getter_AddRefs(pLibDir));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  _pLibDir = pLibDir;
  rv = _pLibDir->GetParent(getter_AddRefs(pLibDir));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  rv = pLibDir->Append(NS_LITERAL_STRING("libraries"));

  // Load the dependent libraries.
  PRUint32 libCount = sizeof (CLLibList) / sizeof (CLLibList[0]);
  for (PRUint32 i = 0; i < libCount; i++) {
    rv = LoadLibrary(pLibDir, NS_ConvertUTF8toUTF16(CLLibList[i]));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  }

  // Produce the library file.
  nsCOMPtr<nsILocalFile> pLibFile;
  nsCOMPtr<nsIFile>      _pLibFile;
  rv = pLibDir->Clone(getter_AddRefs(_pLibFile));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  pLibFile = do_QueryInterface(_pLibFile, &rv);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  rv = pLibFile->Append(NS_ConvertUTF8toUTF16(CLModuleLib));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);

  // Load the component library.
  PRLibrary *pLibrary;
  rv = pLibFile->Load(&pLibrary);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);

  // Get the component get module function.
  nsGetModuleProc getModuleProc = (nsGetModuleProc)
                    PR_FindFunctionSymbol(pLibrary,
                                          NS_GET_MODULE_SYMBOL);
  NS_ENSURE_TRUE(getModuleProc, NS_ERROR_NOT_AVAILABLE);

  // Call the component get module function.
  rv = getModuleProc(aComponentManager,
                     aComponentFile,
                     aComponentModule);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);

  return NS_OK;
}


/**
 * Load the library with the relative file path specified by aLibPath within the
 * library directory specified by aLibDir.
 *
 * \param aLibDir               Directory containing libraries.
 * \param aLibPath              Path to library file within libraries directory.
 */

nsresult
LoadLibrary(nsCOMPtr<nsIFile> aLibDir,
            nsString          aLibPath)
{
  nsresult rv;

  // Produce the library file.
  nsCOMPtr<nsILocalFile> pLibFile;
  nsCOMPtr<nsIFile>      _pLibFile;
  rv = aLibDir->Clone(getter_AddRefs(_pLibFile));
  NS_ENSURE_SUCCESS(rv, rv);
  pLibFile = do_QueryInterface(_pLibFile, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pLibFile->Append(aLibPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the library.
#ifndef XP_MACOSX
  PRLibrary *pLibrary;
  rv = pLibFile->Load(&pLibrary);
  NS_ENSURE_SUCCESS(rv, rv);
#else
  nsCString filePath;
  rv = pLibFile->GetNativePath(filePath);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(NSAddImage(filePath.get(),
                            NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME),
                 NS_ERROR_NOT_AVAILABLE);
#endif

  return NS_OK;
}

}; // extern "C"


