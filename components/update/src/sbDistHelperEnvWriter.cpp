/* vim: set sw=2 */
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

/* class definition includes */
#include "sbDistHelperEnvWriter.h"

/* Mozilla interface includes */
#include <nsICategoryManager.h>
#include <nsIFile.h>
#include <nsIObserverService.h>
#include <nsIProperties.h>

/* Songbird interface includes */

/* Mozilla header includes */
#include <nsCOMPtr.h>
#include <nsDirectoryServiceDefs.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>
#include <nsXPCOMCID.h>
#include <prio.h>

/* Songbird header includes */

/* Other includes */
#if defined(XP_WIN)
  #include <windows.h>
#elif defined(XP_MACOSX)
  #include <crt_externs.h>
#else
  extern char **environ;
#endif

/* Misc preprocessor macros */
#define NS_APPSTARTUP_CATEGORY "app-startup"
#define XRE_UPDATE_ROOT_DIR  "UpdRootD"
#define TOPIC_UPDATE_STATUS "update-service-pre-update-status"

#ifdef __GNUC__
#  define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDistHelperEnvWriter:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDistHelperEnvWriterLog = nsnull;
#define TRACE(args) PR_LOG(gDistHelperEnvWriterLog , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDistHelperEnvWriterLog , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_ISUPPORTS1(sbDistHelperEnvWriter, nsIObserver)

sbDistHelperEnvWriter::sbDistHelperEnvWriter()
{
  #ifdef PR_LOGGING
    if (!gDistHelperEnvWriterLog)
      gDistHelperEnvWriterLog = PR_NewLogModule("sbDistHelperEnvWriter");
  #endif
}

sbDistHelperEnvWriter::~sbDistHelperEnvWriter()
{
}

/* static */ NS_METHOD
sbDistHelperEnvWriter::RegisterSelf(nsIComponentManager*         aCompMgr,
                                    nsIFile*                     aPath,
                                    const char*                  aLoaderStr,
                                    const char*                  aType,
                                    const nsModuleComponentInfo* aInfo)
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  char* previousEntry;
  rv = categoryManager->AddCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                         "sbDistHelperEnvWriter",
                                         SB_DISTHELPER_ENV_WRITER_CONTRACTID,
                                         PR_TRUE, PR_TRUE, &previousEntry);
  NS_ENSURE_SUCCESS(rv, rv);
  if (previousEntry) {
    NS_Free(previousEntry);
  }

  return NS_OK;
}

/* static */ NS_METHOD
sbDistHelperEnvWriter::UnregisterSelf(nsIComponentManager *aCompMgr,
                                      nsIFile *aPath,
                                      const char *aLoaderStr,
                                      const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete self from the app-startup category.
  rv = categoryManager->DeleteCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                            "sbDistHelperEnvWriter",
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDistHelperEnvWriter::OnUpdatePending(nsIFile *aUpdateDir)
{
  nsresult rv;
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aUpdateDir);

  nsCOMPtr<nsIFile> outFile;
  rv = aUpdateDir->Clone(getter_AddRefs(outFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = outFile->Append(NS_LITERAL_STRING("disthelper.env"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> outStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream),
                                   outFile);
  NS_ENSURE_SUCCESS(rv, rv);
  #if XP_WIN
    // Windows needs to go through the raw Win32 calls, otherwise we get nasty
    // linker errors because we're linking against the CRT DLL.
    const WCHAR ENV_PREFIX[] = L"DISTHELPER_";
    LPWSTR env = GetEnvironmentStringsW();
    while (*env) {
      if (!wcsncmp(ENV_PREFIX, env, NS_ARRAY_LENGTH(ENV_PREFIX) - 1)) {
        nsCAutoString envString;
        CopyUTF16toUTF8(nsDependentString(env), envString);
        TRACE(("env var: %s", envString.get()));
        envString.Append('\n');
        PRUint32 bytesWritten;
        rv = outStream->Write(envString.get(),
                              envString.Length(),
                              &bytesWritten);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      env += wcslen(env) + 1;
    }
  #else
    const char ENV_PREFIX[] = "DISTHELPER_";
    char** env;
    #if defined(XP_MACOSX)
      // Darwin/OSX is similar to Windows in needing to use a call to get environ
      env = *_NSGetEnviron();
    #else
      env = environ;
    #endif
    for (; *env; ++env) {
      if (!strncmp(ENV_PREFIX, *env, NS_ARRAY_LENGTH(ENV_PREFIX) - 1)) {
        nsCAutoString envString(*env);
        TRACE(("env var: %s", envString.get()));
        envString.Append('\n');
        PRUint32 bytesWritten;
        rv = outStream->Write(envString.get(),
                              envString.Length(),
                              &bytesWritten);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  #endif
  rv = outStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/***** nsIObserver *****/
/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP
sbDistHelperEnvWriter::Observe(nsISupports *aSubject,
                               const char *aTopic,
                               const PRUnichar *aData)
{
  nsresult rv;
  TRACE(("%s: %s", __FUNCTION__, aTopic));

  if (!strcmp(aTopic, NS_APPSTARTUP_CATEGORY)) {
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->AddObserver(this, TOPIC_UPDATE_STATUS, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    rv = obsSvc->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->RemoveObserver(this, TOPIC_UPDATE_STATUS);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp(aTopic, TOPIC_UPDATE_STATUS)) {
    if (NS_LITERAL_STRING("pending").Equals(aData)) {
      nsCOMPtr<nsIFile> updateDir = do_QueryInterface(aSubject);
      NS_ENSURE_TRUE(updateDir, NS_ERROR_FAILURE);
      rv = OnUpdatePending(updateDir);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}
