/*
 * sbURIUtils.h
 *
 *  Created on: Jun 19, 2009
 *      Author: dbradley
 */

#ifndef SBURIUTILS_H_
#define SBURIUTILS_H_

#include <nsStringAPI.h>
#include <nsIFileURL.h>
#include <nsIURI.h>
#include <nsNetError.h>

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

#endif /* SBURIUTILS_H_ */

