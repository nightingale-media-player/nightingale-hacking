/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

#ifndef __SB_LIBRARYLOADERUTILS__
#define __SB_LIBRARYLOADERUTILS__

#include <nsIFile.h>
#include <nsIProperties.h>
#include <nsStringGlue.h>
#include <nsServiceManagerUtils.h>
#include <nsILineInputStream.h>
#include <nsNetUtil.h>
#include <prerror.h>
#include <prlog.h>
#include <prprf.h>

#ifdef XP_MACOSX
#include <mach-o/dyld.h>
#endif

#define LOG(args) PR_LOG(loadLibrariesLog, PR_LOG_WARN, args)

static nsresult
SB_LoadLibraries(nsIFile* aManifest)
{
  NS_ENSURE_ARG_POINTER(aManifest);

  nsresult rv;

#ifdef PR_LOGGING
  PRLogModuleInfo* loadLibrariesLog = PR_NewLogModule("sbLoadLibraries");
#endif

  nsCOMPtr<nsIProperties> directorySvc =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> appDir;
  rv = directorySvc->Get("resource:app",
                         NS_GET_IID(nsIFile),
                         getter_AddRefs(appDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> fis;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fis), aManifest);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lis = do_QueryInterface(fis, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString line;
  PRBool moreLines;
  do {
    rv = lis->ReadLine(line, &moreLines);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!line.IsEmpty() && line.CharAt(0) != '#') {

      // If the line starts with @, then consider it a relative path from the
      // application directory.  Otherwise, it is relative to the location of
      // the manifest
      nsCOMPtr<nsILocalFile> libLocal;
      if (line.CharAt(0) == '@') {
        nsCOMPtr<nsIFile> clone;
        rv = appDir->Clone(getter_AddRefs(clone));
        NS_ENSURE_SUCCESS(rv, rv);

        libLocal = do_QueryInterface(clone, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsDependentCSubstring path(line.BeginReading() + 1, line.Length() - 1);
        rv = libLocal->AppendRelativeNativePath(path);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        nsCOMPtr<nsIFile> parent;
        rv = aManifest->GetParent(getter_AddRefs(parent));
        NS_ENSURE_SUCCESS(rv, rv);

        while (StringBeginsWith(line, NS_LITERAL_CSTRING(".."))) {
          nsCOMPtr<nsIFile> newParent;

          // remove .. + delimiter from the path to append
          line.Cut(0, 3);

          rv = parent->GetParent(getter_AddRefs(newParent));
          NS_ENSURE_SUCCESS(rv, rv);

          newParent.swap(parent);
        }

        libLocal = do_QueryInterface(parent, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = libLocal->AppendRelativePath(NS_ConvertASCIItoUTF16(line));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      PRBool success;

#ifdef PR_LOGGING
      {
        nsCString _path;
        libLocal->GetNativePath(_path);
        LOG(("SB_LoadLibraries: loading '%s'\n", _path.BeginReading()));
      }
#endif

#if XP_MACOSX
      nsCString filePath;
      libLocal->GetNativePath(filePath);
      const struct mach_header* header = NSAddImage(filePath.get(),
                 NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME |
                 NSADDIMAGE_OPTION_WITH_SEARCHING);

      if (!header) {
        NSLinkEditErrors linkEditError;
        int errorNum;
        const char *errorString;
        const char *fileName;

        NSLinkEditError(&linkEditError, &errorNum, &fileName, &errorString);
        char *message = PR_smprintf(
            "SB_LoadLibraries: ERROR %d:%d for file %s\n%s",
              linkEditError, errorNum, fileName, errorString);
        
        NS_WARNING(message);
        PR_smprintf_free(message);
      }

      success = (header != nsnull);
#else
      PRLibrary* library;
      rv = libLocal->Load(&library);
      // Leaking the library, we don't care
      success = NS_SUCCEEDED(rv);
#endif

      if (!success) {
        nsCString libPath;
        libLocal->GetNativePath(libPath);
        char* message = PR_smprintf("SB_LoadLibraries: Error loading library: %s, OS error was: %i",
                                    libPath.get(), PR_GetOSError());
        NS_WARNING(message);
        PR_smprintf_free(message);
      }
    }
  }
  while (moreLines);

  rv = fis->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#endif /* __SB_LIBRARYLOADERUTILS__ */
