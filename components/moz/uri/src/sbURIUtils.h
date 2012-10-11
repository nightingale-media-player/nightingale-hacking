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

#ifndef SBURIUTILS_H_
#define SBURIUTILS_H_

#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsIURI.h>
#include <nsNetCID.h>
#include <nsNetError.h>
#include <nsStringAPI.h>
#include <nsThreadUtils.h>

#include <sbProxiedComponentManager.h>

static inline nsresult
sbGetFileExtensionFromURI(nsIURI* aURI, nsACString& _retval)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCString strExtension;

  nsCString strPath;
  nsresult rv = aURI->GetPath(strPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (strPath.IsEmpty())
    return NS_ERROR_MALFORMED_URI;

  PRInt32 fileExtPosStart = strPath.RFindChar('.');
  PRInt32 fileExtPosEnd = strPath.RFindChar('?');

  if(fileExtPosEnd > fileExtPosStart) {
    strExtension = Substring(strPath, fileExtPosStart, fileExtPosEnd - fileExtPosStart);
  }
  else {
    strExtension = Substring(strPath, fileExtPosStart);
  }

  // Strip '.' from the beginning and end, if it exists
  strExtension.Trim(".");

  _retval.Assign(strExtension);
  return NS_OK;
}


static inline nsresult
sbInvalidateFileURLCache(nsIFileURL* aFileURL)
{
  NS_ENSURE_ARG_POINTER(aFileURL);

  // Function variables.
  nsresult rv;

  // Touch the file URL scheme to invalidate the file URL cache.  Some file URL
  // changes do not invalidate the cache.  For example, changing the file base
  // name and reading the file URL file object will return a cached file object
  // that does not contain the new file base name.  Reading and writing back the
  // URI scheme invalidates the cache.
  nsCAutoString scheme;
  rv = aFileURL->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aFileURL->SetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aIOService the IO service object usable from the current thread.
 * If the current thread is not the main thread, return a main thread proxied
 * IO service object.
 *
 * \param aIOService            Returned IO service.
 */

static inline nsresult
SB_GetIOService(nsIIOService** aIOService)
{
  nsresult rv;

  // Get the IO service.
  nsCOMPtr<nsIIOService> ioService;
  if (NS_IsMainThread()) {
    ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    ioService = do_ProxiedGetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  ioService.forget(aIOService);

  return NS_OK;
}


/**
 * Return in aURI a new URI, created using the URI spec, character set, and base
 * URI specified by aSpec, aCharSet, and aBaseURI.
 * This function may be called from any thread, but the returned URI can only be
 * used by the calling thread.
 *
 * \param aURI                  Returned URI.
 * \param aSpec                 URI spec.
 * \param aCharSet              URI character set.
 * \param aBaseURI              Base URI.
 */

static inline nsresult
SB_NewURI(nsIURI**          aURI,
          const nsACString& aSpec,
          const char*       aCharSet = nsnull,
          nsIURI*           aBaseURI = nsnull)
{
  nsresult rv;

  // Get the IO service.
  nsCOMPtr<nsIIOService> ioService;
  rv = SB_GetIOService(getter_AddRefs(ioService));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the URI.
  nsCOMPtr<nsIURI> uri;
  rv = ioService->NewURI(aSpec, aCharSet, aBaseURI, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a main thread URI.
  nsCOMPtr<nsIURI> mainThreadURI = do_MainThreadQueryInterface(uri, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  mainThreadURI.forget(aURI);

  return NS_OK;
}

static inline nsresult
SB_NewURI(nsIURI**         aURI,
          const nsAString& aSpec,
          const char*      aCharSet = nsnull,
          nsIURI*          aBaseURI = nsnull)
{
  return SB_NewURI(aURI, NS_ConvertUTF16toUTF8(aSpec), aCharSet, aBaseURI);
}

static inline nsresult
SB_NewURI(nsIURI**    aURI,
          const char* aSpec,
          nsIURI*     aBaseURI = nsnull)
{
  return SB_NewURI(aURI, nsDependentCString(aSpec), nsnull, aBaseURI);
}

#endif /* SBURIUTILS_H_ */

