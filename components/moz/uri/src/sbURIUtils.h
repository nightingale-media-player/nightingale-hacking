/*
 * sbURIUtils.h
 *
 *  Created on: Jun 19, 2009
 *      Author: dbradley
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
 * URI specified by aSpec, aCharSet, and aBaseURI.  This function may be called
 * from any thread.
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
  rv = ioService->NewURI(aSpec, aCharSet, aBaseURI, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

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

