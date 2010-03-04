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

#include "sbKnownFolderManager.h"

// Mozilla Includes
#include <nsILocalFile.h>
#include <nsStringGlue.h>

// Windows Includes
#if defined(_WIN32_WINNT)
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0600 //Windows Vista or Greater
#endif

#include <ShObjIdl.h>

#define SB_WIN_ENSURE_SUCCESS(aHR, aRV) \
  PR_BEGIN_MACRO                        \
  if(FAILED(aHR)) {                     \
    return aRV;                         \
  }                                     \
  PR_END_MACRO

NS_IMPL_ISUPPORTS1(sbKnownFolderManager, 
                   sbIKnownFolderManager);

sbKnownFolderManager::sbKnownFolderManager()
: mCOMInitialized(E_UNEXPECTED)
{
}

sbKnownFolderManager::~sbKnownFolderManager()
{
  if(SUCCEEDED(mCOMInitialized)) {
    CoUninitialize();
  }
}

nsresult
sbKnownFolderManager::Init()
{
  mCOMInitialized = CoInitialize(NULL);
  
  nsRefPtr<IKnownFolderManager> knownFolderManager;
  HRESULT hr = ::CoCreateInstance(CLSID_KnownFolderManager, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IKnownFolderManager, 
                                  getter_AddRefs(knownFolderManager));
  SB_WIN_ENSURE_SUCCESS(hr, NS_OK);

  knownFolderManager.swap(mKnownFolderManager);
}

nsresult 
sbKnownFolderManager::GetDefaultDisplayName(const nsAString &aFolderPath, 
                                            nsAString &aDisplayName)
{
  nsCOMPtr<nsILocalFile> localFile;
  nsresult rv = NS_NewLocalFile(aFolderPath, 
                                PR_FALSE, 
                                getter_AddRefs(localFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = localFile->GetLeafName(aDisplayName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP 
sbKnownFolderManager::GetDisplayNameFromPath(const nsAString &aFolderPath,
                                             nsAString &aDisplayName)
{
  // Get the default return value in case we error out.
  nsresult rv = GetDefaultDisplayName(aFolderPath, aDisplayName);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // First, we cover the case where we are using a Windows Version 
  // that is lower than Windows Vista.
  //

  if(!mKnownFolderManager) {
    // Just return since we already set aDisplayName to a default value.
    return NS_OK;
  }

  //
  // Second, we cover the case where we are using a Window Version
  // that is at least Windows Vista.
  //

  nsRefPtr<IKnownFolder> knownFolder;
  HRESULT hr = 
    mKnownFolderManager->FindFolderFromPath(aFolderPath.BeginReading(), 
                                            FFFP_EXACTMATCH,
                                            getter_AddRefs(knownFolder));
  SB_WIN_ENSURE_SUCCESS(hr, NS_OK);

  nsRefPtr<IShellItem> shellItem;
  hr = knownFolder->GetShellItem(0, IID_IShellItem, getter_AddRefs(shellItem));
  SB_WIN_ENSURE_SUCCESS(hr, NS_OK);

  LPWSTR displayName = NULL;
  hr = shellItem->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName);
  SB_WIN_ENSURE_SUCCESS(hr, NS_OK);

  nsDependentString tempStr(displayName);
  aDisplayName.Assign(tempStr);

  ::CoTaskMemFree(displayName);

  return NS_OK;
}
