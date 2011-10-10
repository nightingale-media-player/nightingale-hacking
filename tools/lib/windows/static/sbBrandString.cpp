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

/**
 * \file  sbBrandString.cpp
 * \brief Nightingale Brand String Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale brand string imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbBrandString.h"

// Local imports.
#include "sbWinINIString.h"
#include "stringconvert.h"


//------------------------------------------------------------------------------
//
// Nightingale brand string public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbBrandString
//

sbBrandString::sbBrandString(LPCSTR aKey,
                             LPCSTR aDefault)
{
  // Initialize.
  Initialize(aKey, (aDefault ? aDefault : aKey));
}


//-------------------------------------
//
// sbBrandString
//

sbBrandString::sbBrandString(LPCWSTR aKey,
                             LPCWSTR aDefault)
{
  // Initialize.
  Initialize(aKey, (aDefault ? aDefault : aKey));
}


//------------------------------------------------------------------------------
//
// Nightingale brand string private services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Initialize
//

void
sbBrandString::Initialize(const tstring& aKey,
                          const tstring& aDefault)
{
  HRESULT hr;

  // Initialize the class.
  hr = ClassInitialize();
  if (FAILED(hr)) {
    assign(aDefault);
    return;
  }

  // Get the brand string from the branding INI file.
  assign(sbWinINIString(_T("Branding"),
                        aKey.c_str(),
                        sBrandingINIFilePath->c_str(),
                        aDefault.c_str()));
}


//------------------------------------------------------------------------------
//
// Nightingale brand string private class services.
//
//------------------------------------------------------------------------------

//
// Class fields.
//

BOOL     sbBrandString::sClassInitialized = FALSE;
tstring* sbBrandString::sBrandingINIFilePath = NULL;


//-------------------------------------
//
// ClassInitialize
//

/* static */ HRESULT
sbBrandString::ClassInitialize()
{
  HRESULT hr;

  // Do nothing if already initialized.
  if (sClassInitialized)
    return S_OK;

  // Set up the finalize on exit.
  atexit(ClassFinalize);

  // Allocate the branding INI file path string.
  sBrandingINIFilePath = new tstring();
  SB_WIN_ENSURE_TRUE(sBrandingINIFilePath, E_OUTOFMEMORY);

  // Read the branding INI file from the environment.  If it's not specified,
  // use the default branding file in the running module's directory.
  sbEnvString brandingINIFileEnv(SB_BRAND_INI_FILE_ENV_VAR_NAME);
  if (!brandingINIFileEnv.empty()) {
    // Use the branding INI file from the environment.
    sBrandingINIFilePath->assign(brandingINIFileEnv);
  }
  else {
    // Get the running module paths.
    tstring moduleDirPath;
    tstring moduleFilePath;
    hr = sbWinGetModuleFilePath(NULL, moduleFilePath, moduleDirPath);
    SB_WIN_ENSURE_SUCCESS(hr, hr);

    // Get the branding INI file path.
    sBrandingINIFilePath->assign(moduleDirPath + _T(SB_BRAND_DEFAULT_INI_FILE));
  }

  // Indicate that the class is initialized.
  sClassInitialized = TRUE;

  return S_OK;
}


//-------------------------------------
//
// ClassFinalize
//

/* static */ void
sbBrandString::ClassFinalize()
{
  // Deallocate the branding INI file path string.
  if (sBrandingINIFilePath)
    delete sBrandingINIFilePath;
  sBrandingINIFilePath = NULL;
}

