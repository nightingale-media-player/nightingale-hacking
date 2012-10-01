/*
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

#ifndef sbFileObjectStreams_h_
#define sbFileObjectStreams_h_

#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsIFileStreams.h>
#include <nsIBufferedStreams.h>
#include <nsIObjectOutputStream.h>
#include <nsIObjectInputStream.h>


//------------------------------------------------------------------------------
// Pure virtual class for file object streams.

class sbFileObjectStream
{
public:
  sbFileObjectStream();
  virtual ~sbFileObjectStream();
  //
  // \brief Init a stream with a given file spec.
  //
  NS_IMETHOD InitWithFile(nsIFile *aStreamedFile) = 0;

  //
  // \brief Close a file object stream.
  //
  NS_IMETHOD Close() = 0;

protected:
  bool mFileStreamIsActive;
  bool mObjectStreamIsActive;
};

#define SB_DECL_SBFILEOBJECTSTREAM \
  NS_IMETHOD InitWithFile(nsIFile *aStreamedFile); \
  NS_IMETHOD Close(); 


//------------------------------------------------------------------------------
// File object output stream implementation.

class sbFileObjectOutputStream : public sbFileObjectStream,
                                 public nsISupports
{
public:
  sbFileObjectOutputStream();
  virtual ~sbFileObjectOutputStream();

  NS_DECL_ISUPPORTS
  SB_DECL_SBFILEOBJECTSTREAM

  nsresult WriteObject(nsISupports *aSupports, bool aIsStrongRef);

  // Warning: Do not pass a NULL string into this method.
  nsresult WriteCString(const nsACString & aString);

  // Warning: Do not pass a NULL string into this method.
  nsresult WriteString(const nsAString & aString);

  nsresult WriteUint32(PRUint32 aOutInt);

  nsresult Writebool(bool aBoolean);

  nsresult WriteBytes(const char *aData, PRUint32 aLength);

private:
  nsCOMPtr<nsIFileOutputStream>   mFileOutputStream;
  nsCOMPtr<nsIObjectOutputStream> mObjectOutputStream;
};


//------------------------------------------------------------------------------
// File object input stream implementation.

class sbFileObjectInputStream : public sbFileObjectStream,
                                public nsISupports
{
public:
  sbFileObjectInputStream();
  virtual ~sbFileObjectInputStream();

  NS_DECL_ISUPPORTS
  SB_DECL_SBFILEOBJECTSTREAM

  nsresult ReadObject(bool aIsStrongRef, nsISupports **aOutObject);

  nsresult ReadCString(nsACString & aReadString);

  nsresult ReadString(nsAString & aReadString);

  nsresult ReadUint32(PRUint32 *aReadInt);

  nsresult Readbool(bool *aReadBoolean);

  nsresult ReadBytes(PRUint32 aLength, char **aString);

private:
  nsCOMPtr<nsIFileInputStream>     mFileInputStream;
  nsCOMPtr<nsIBufferedInputStream> mBufferedInputStream;
  nsCOMPtr<nsIObjectInputStream>   mObjectInputStream;
  bool                           mBufferedStreamIsActive;
};

#endif  // sbFileObjectStreams_h_

