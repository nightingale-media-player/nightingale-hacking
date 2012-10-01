/*
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

#include "sbFileObjectStreams.h"
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>


sbFileObjectStream::sbFileObjectStream()
: mFileStreamIsActive(PR_FALSE)
, mObjectStreamIsActive(PR_FALSE)
{
}

/*virtual*/ 
sbFileObjectStream::~sbFileObjectStream()
{
}

NS_IMPL_ISUPPORTS1(sbFileObjectOutputStream, nsISupports)

sbFileObjectOutputStream::sbFileObjectOutputStream()
{
  mFileStreamIsActive = PR_FALSE;
  mObjectStreamIsActive = PR_FALSE;
}

sbFileObjectOutputStream::~sbFileObjectOutputStream()
{
  if (mFileStreamIsActive || mObjectStreamIsActive) {
    if (NS_FAILED(Close())) {
      NS_WARNING("Error cold not close the output streams!");
    }
  }
}

NS_IMETHODIMP 
sbFileObjectOutputStream::InitWithFile(nsIFile *aStreamedFile)
{
  NS_ENSURE_ARG_POINTER(aStreamedFile);

  nsresult rv;
  mFileOutputStream = 
    do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Default setup values are fine for the file stream
  rv = mFileOutputStream->Init(aStreamedFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mFileStreamIsActive = PR_TRUE;

  mObjectOutputStream = 
    do_CreateInstance("@mozilla.org/binaryoutputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mObjectOutputStream->SetOutputStream(mFileOutputStream);
  NS_ENSURE_SUCCESS(rv, rv);

  mObjectStreamIsActive = PR_TRUE;
  
  return NS_OK;
}

nsresult
sbFileObjectOutputStream::WriteObject(nsISupports *aSupports, 
                                      bool aIsStrongRef)
{
  NS_ENSURE_ARG_POINTER(aSupports);

  if (!mFileStreamIsActive || !mObjectStreamIsActive) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mObjectOutputStream->WriteObject(aSupports, aIsStrongRef);
}

nsresult
sbFileObjectOutputStream::WriteCString(const nsACString & aString)
{
  if (!mFileStreamIsActive || !mObjectStreamIsActive) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCString str(aString);
  return mObjectOutputStream->WriteStringZ(str.get());
}

nsresult
sbFileObjectOutputStream::WriteString(const nsAString & aString)
{
  if (!mFileStreamIsActive || !mObjectOutputStream) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsString str(aString);
  return mObjectOutputStream->WriteWStringZ(str.get());
}

nsresult
sbFileObjectOutputStream::WriteUint32(PRUint32 aOutInt)
{
  if (!mFileStreamIsActive || !mObjectStreamIsActive) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mObjectOutputStream->Write32(aOutInt);
}

nsresult
sbFileObjectOutputStream::Writebool(bool aBoolean)
{
  if (!mFileStreamIsActive || !mObjectStreamIsActive) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return mObjectOutputStream->WriteBoolean(aBoolean);
}

nsresult
sbFileObjectOutputStream::WriteBytes(const char *aData, PRUint32 aLength)
{
  NS_ENSURE_ARG_POINTER(aData);

  if (!mFileStreamIsActive || !mObjectStreamIsActive) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mObjectOutputStream->WriteBytes(aData, aLength);
}

NS_IMETHODIMP
sbFileObjectOutputStream::Close()
{
  nsresult rv;
  if (mFileStreamIsActive) {
    rv = mFileOutputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    mFileStreamIsActive = PR_FALSE;
  }
  if (mObjectStreamIsActive) {
    rv = mObjectOutputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    mObjectStreamIsActive = PR_FALSE;
  }

  return NS_OK;
}

//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbFileObjectInputStream, nsISupports)

sbFileObjectInputStream::sbFileObjectInputStream()
{
  mFileStreamIsActive = PR_FALSE;
  mBufferedStreamIsActive = PR_FALSE;
  mObjectStreamIsActive = PR_FALSE;
}

sbFileObjectInputStream::~sbFileObjectInputStream()
{
  if (mFileStreamIsActive || 
      mBufferedStreamIsActive || 
      mObjectStreamIsActive) 
  {
    if (NS_FAILED(Close())) {
      NS_WARNING("Error cold not close the output streams!");
    }
  }
}

NS_IMETHODIMP
sbFileObjectInputStream::InitWithFile(nsIFile *aStreamedFile)
{
  NS_ENSURE_ARG_POINTER(aStreamedFile);

  nsresult rv;
  mFileInputStream = 
    do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Default setup values are fine for the file stream
  rv = mFileInputStream->Init(aStreamedFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mFileStreamIsActive = PR_TRUE;

  mBufferedInputStream = 
    do_GetService("@mozilla.org/network/buffered-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBufferedInputStream->Init(mFileInputStream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  mBufferedStreamIsActive = PR_TRUE;

  mObjectInputStream =
    do_CreateInstance("@mozilla.org/binaryinputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mObjectInputStream->SetInputStream(mBufferedInputStream);
  NS_ENSURE_SUCCESS(rv, rv);

  mObjectStreamIsActive = PR_TRUE;

  return NS_OK;
}

nsresult
sbFileObjectInputStream::ReadObject(bool aIsStrongRef, 
                                    nsISupports **aOutObject)
{
  NS_ENSURE_ARG_POINTER(aOutObject);

  if (!mFileStreamIsActive || 
      !mBufferedStreamIsActive || 
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mObjectInputStream->ReadObject(aIsStrongRef, aOutObject);
}

nsresult
sbFileObjectInputStream::ReadCString(nsACString & aReadString)
{
  if (!mFileStreamIsActive ||
      !mBufferedStreamIsActive ||
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  return mObjectInputStream->ReadCString(aReadString);
}

nsresult
sbFileObjectInputStream::ReadString(nsAString & aReadString)
{
  if (!mFileStreamIsActive || 
      !mBufferedStreamIsActive ||
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mObjectInputStream->ReadString(aReadString);
}

nsresult
sbFileObjectInputStream::ReadUint32(PRUint32 *aReadInt)
{
  NS_ENSURE_ARG_POINTER(aReadInt);

  if (!mFileStreamIsActive || 
      !mBufferedStreamIsActive ||
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_FAILURE;
  }

  return mObjectInputStream->Read32(aReadInt);
}

nsresult
sbFileObjectInputStream::Readbool(bool *aReadBoolean)
{
  NS_ENSURE_ARG_POINTER(aReadBoolean);

  if (!mFileStreamIsActive || 
      !mBufferedStreamIsActive ||
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_FAILURE;
  }

  return mObjectInputStream->ReadBoolean(aReadBoolean);
}

nsresult
sbFileObjectInputStream::ReadBytes(PRUint32 aLength, char **aString)
{
  NS_ENSURE_ARG_POINTER(aString);
  
  if (!mFileStreamIsActive || 
      !mBufferedStreamIsActive ||
      !mObjectStreamIsActive) 
  {
    return NS_ERROR_FAILURE;
  }

  return mObjectInputStream->ReadBytes(aLength, aString);
}

NS_IMETHODIMP
sbFileObjectInputStream::Close()
{
  nsresult rv;
  if (mFileStreamIsActive) {
    rv = mFileInputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    mFileStreamIsActive = PR_FALSE;
  }
  if (mBufferedStreamIsActive) {
    rv = mBufferedInputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    mBufferedStreamIsActive = PR_FALSE;
  }
  if (mObjectStreamIsActive) {
    rv = mObjectInputStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    mObjectStreamIsActive = PR_FALSE;
  }

  return NS_OK;
}

