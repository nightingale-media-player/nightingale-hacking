/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbFileUtils.h"

#include <nsIFileStreams.h>
#include <nsIFileURL.h>
#include <nsILocalFile.h>
#include <nsIURI.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsNetCID.h>

#include <sbMemoryUtils.h>

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

