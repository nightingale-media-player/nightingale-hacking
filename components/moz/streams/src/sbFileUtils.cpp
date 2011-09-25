/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbFileUtils.h"

#include <nsIFileStreams.h>
#include <nsIFileURL.h>
#include <nsILocalFile.h>
#include <nsIURI.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsNetCID.h>
#include <nsNetUtil.h>

#include <sbMemoryUtils.h>
#include <sbProxiedComponentManager.h>

/**
 * Helper functions to open a stream given a file path
 */
nsresult sbOpenInputStream(nsAString const & aPath, nsIInputStream ** aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;
  nsCOMPtr<nsILocalFile> file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbOpenInputStream(file, aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbOpenInputStream(nsIURI * aURI, nsIInputStream ** aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv;
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbOpenInputStream(file, aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbOpenInputStream(nsIFile * aFile, nsIInputStream ** aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aFile);

  nsresult rv;
  nsCOMPtr<nsIFileInputStream> fileStream =
    do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Init(aFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  stream.forget(aStream);

  return NS_OK;
}

nsresult sbOpenOutputStream(nsIFile * aFile, nsIOutputStream ** aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aFile);

  nsresult rv;
  nsCOMPtr<nsIFileOutputStream> fileStream =
    do_CreateInstance(NS_LOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Init(aFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> stream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  stream.forget(aStream);

  return NS_OK;
}

nsresult sbOpenOutputStream(nsAString const & aPath, nsIOutputStream ** aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;
  nsCOMPtr<nsILocalFile> file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbOpenOutputStream(file, aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult sbReadFile(nsIFile * aFile, nsACString &aBuffer)
{
  NS_ENSURE_ARG_POINTER(aFile);

  nsresult rv;
  PRInt64 fileSize;
  rv = aFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream;
  rv = sbOpenInputStream(aFile, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbConsumeStream(stream, static_cast<PRUint32>(fileSize), aBuffer);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* from xpcom/io/nsStreamUtils.cpp
 * (it's hidden in libxul, so we need a copy :( )
 */
nsresult
sbConsumeStream(nsIInputStream *stream, PRUint32 maxCount, nsACString &result)
{
    nsresult rv = NS_OK;
    result.Truncate();

    while (maxCount) {
        PRUint32 avail;
        rv = stream->Available(&avail);
        if (NS_FAILED(rv)) {
            if (rv == NS_BASE_STREAM_CLOSED)
                rv = NS_OK;
            break;
        }
        if (avail == 0)
            break;
        if (avail > maxCount)
            avail = maxCount;

        // resize result buffer
        PRUint32 length = result.Length();
        result.SetLength(length + avail);
        if (result.Length() != (length + avail))
            return NS_ERROR_OUT_OF_MEMORY;
        char *buf = result.BeginWriting() + length;

        PRUint32 n;
        rv = stream->Read(buf, avail, &n);
        if (NS_FAILED(rv))
            break;
        if (n != avail)
            result.SetLength(length + n);
        if (n == 0)
            break;
        maxCount -= n;
    }

    return rv;
}

inline
nsCOMPtr<nsIIOService> GetIOService(nsresult & rv)
{
  // Get the IO service.
  if (NS_IsMainThread()) {
    return do_GetIOService(&rv);
  }
  return do_ProxiedGetService(NS_IOSERVICE_CONTRACTID, &rv);
}

nsresult
sbNewFileURI(nsIFile* aFile,
             nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aFile);
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv;

  // Get the IO service.
  nsCOMPtr<nsIIOService> ioService = GetIOService(rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Note that NewFileURI is broken on Linux when dealing with
  // file names not in the filesystem charset; see bug 6227
#if XP_UNIX && !XP_MACOSX
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile, &rv);
  if (NS_SUCCEEDED(rv)) {
    // Use the local file persistent descriptor to form a URI spec.
    nsCAutoString descriptor;
    rv = localFile->GetPersistentDescriptor(descriptor);
    if (NS_SUCCEEDED(rv)) {
      // Escape the descriptor into a spec.
      nsCOMPtr<nsINetUtil> netUtil =
        do_CreateInstance("@mozilla.org/network/util;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCAutoString spec;
      rv = netUtil->EscapeString(descriptor,
                                 nsINetUtil::ESCAPE_URL_PATH,
                                 spec);
      NS_ENSURE_SUCCESS(rv, rv);

      // Add the "file:" scheme.
      spec.Insert("file://", 0);

      // Create the URI.
      rv = ioService->NewURI(spec, nsnull, nsnull, aURI);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }
  }
#endif

  // Get a URI directly from the file.
  rv = ioService->NewFileURI(aFile, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
RemoveBadFileNameCharacters(nsAString& aFileName,
                            PRBool     aAllPlatforms)
{
  // Get the list of bad characters.
  const char* badCharacters;
  if (aAllPlatforms)
    badCharacters = SB_FILE_BAD_CHARACTERS_ALL_PLATFORMS;
  else
    badCharacters = SB_FILE_BAD_CHARACTERS;

  // Remove all bad characters from the file name.
  aFileName.StripChars(badCharacters);

  // Windows does not like spaces at the begining or end of a file/folder name.
  // Windows also does not like dots at the begining or end of the file/folder
  // name and dots are bad as well on some other operating systems as they
  // represent a hidden file.
  aFileName.Trim(" .", PR_TRUE, PR_TRUE);
}

