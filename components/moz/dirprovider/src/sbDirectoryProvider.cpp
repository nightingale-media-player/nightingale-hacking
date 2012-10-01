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

#include "sbDirectoryProvider.h"

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>
#include <nsILocalFile.h>
#include <nsStringAPI.h>

#if defined(XP_WIN)

#include <windows.h>
#include <shlobj.h>

#include "winGetFolder.h"

#endif // XP_WIN

NS_IMPL_ISUPPORTS1(sbDirectoryProvider, nsIDirectoryServiceProvider)

sbDirectoryProvider::sbDirectoryProvider()
{
  /* member initializers and constructor code */
}

sbDirectoryProvider::~sbDirectoryProvider()
{
  /* destructor code */
}

nsresult
sbDirectoryProvider::Init()
{
  nsresult rv;

  nsCOMPtr<nsIDirectoryService> dirService =
                            do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,
                                          &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dirService->RegisterProvider(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryProvider::GetFile(const char *aProp,
                             bool *aPersistent,
                             nsIFile **_retval)
{
  NS_ENSURE_ARG(aProp);
  NS_ENSURE_ARG_POINTER(aPersistent);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsILocalFile> localFile;
  nsresult rv = NS_ERROR_FAILURE;

  *_retval = nsnull;
  *aPersistent = PR_TRUE;

#if defined (XP_WIN)
  if (strcmp(aProp, NS_WIN_COMMON_DOCUMENTS) == 0)
    rv = GetWindowsFolder(CSIDL_COMMON_DOCUMENTS, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_COMMON_PICTURES) == 0)
    rv = GetWindowsFolder(CSIDL_COMMON_PICTURES, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_COMMON_MUSIC) == 0)
    rv = GetWindowsFolder(CSIDL_COMMON_MUSIC, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_COMMON_VIDEO) == 0)
    rv = GetWindowsFolder(CSIDL_COMMON_VIDEO, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_DOCUMENTS) == 0)
    rv = GetWindowsFolder(CSIDL_MYDOCUMENTS, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_PICTURES) == 0)
    rv = GetWindowsFolder(CSIDL_MYPICTURES, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_MUSIC) == 0)
    rv = GetWindowsFolder(CSIDL_MYMUSIC, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_VIDEO) == 0)
    rv = GetWindowsFolder(CSIDL_MYVIDEO, getter_AddRefs(localFile));
  else if (strcmp(aProp, NS_WIN_DISCBURNING) == 0)
    rv = GetWindowsFolder(CSIDL_CDBURN_AREA, getter_AddRefs(localFile));
#endif // XP_WIN

  if (NS_SUCCEEDED(rv)) {
    if (localFile)
      rv = CallQueryInterface(localFile, _retval);
    else
      rv = NS_ERROR_FAILURE;
  }

  return rv;
}
