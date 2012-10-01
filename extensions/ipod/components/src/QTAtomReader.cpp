/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

/**
* \file  QTAtomReader.cpp
* \brief QuickTime Atom Reader Source.
*/

//------------------------------------------------------------------------------
//
// QuickTime atom reader imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "QTAtomReader.h"

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <prnetdb.h>


//------------------------------------------------------------------------------
//
// QuickTime atom reader services.
//
//------------------------------------------------------------------------------

/**
 * Construct a QuickTime atom reader object.
 */

QTAtomReader::QTAtomReader() :
  mAtomHdrSize(20)
{
}


/**
 * Destroy a QuickTime atom reader object.
 */

QTAtomReader::~QTAtomReader()
{
}


/**
 * Open the file specified by aFile for reading by the QuickTime atom reader.
 *
 * \param aFile                 File to open.
 */

nsresult
QTAtomReader::Open(nsIFile* aFile)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aFile);

  // Function variables.
  nsresult rv;

  // Save the file object.
  mFile = aFile;

  // Get and initialize a file input stream.
  mFileInputStream = do_CreateInstance
                       ("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mFileInputStream->Init(mFile, PR_RDONLY, 0, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get seekable and generic input streams. */
  mSeekableStream = do_QueryInterface(mFileInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mInputStream = do_QueryInterface(mFileInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Close the file being read by the QuickTime atom reader.
 */

void
QTAtomReader::Close(void)
{
  // Close the stream.
  if (mInputStream)
    mInputStream->Close();
  mInputStream = nsnull;
}


/**
 * Search the open QuickTime atom reader file for a FairPlay account name atom
 * and return the value in aAccountName.  If no FairPlay account name is
 * present, return NS_ERROR_FAILURE and do not modify aAccountName.
 *
 * \param aAccountName          FairPlay account name.
 */

nsresult
QTAtomReader::GetFairPlayAccountName(nsAString& aAccountName)
{
  PRUint32 atom[2];
  PRUint64 offset = 0;
  nsresult rv;

  // Get the starting end offset.
  PRInt64  fileSize;
  PRUint64 endOffset;
  rv = mFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  endOffset = fileSize;

  // Set the atom header size.
  mAtomHdrSize = 8;

  // Get the meta-data atom.
  rv = AtomPathGet("/moov/udta/meta", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay account name atom, skipping the sample description table
  // atom header.
  offset += 12;
  rv = AtomPathGet("/ilst/apID/data", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay account name size and offset.
  offset += 16;
  PRUint32 accountNameSize = (PRUint32)(endOffset - offset);

  // Allocate an account name buffer and set up for auto-disposal.
  char* accountNameBuffer = (char*) NS_Alloc(accountNameSize + 1);
  NS_ENSURE_TRUE(accountNameBuffer, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoAccountNameBuffer(accountNameBuffer);

  // Read the account name.
  PRUint32 bytesRead;
  rv = mSeekableStream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mInputStream->Read(accountNameBuffer, accountNameSize, &bytesRead);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(bytesRead >= accountNameSize, NS_ERROR_FAILURE);
  accountNameBuffer[accountNameSize] = '\0';

  // Return results.
  aAccountName.Assign(NS_ConvertUTF8toUTF16(accountNameBuffer));

  return NS_OK;
}


/**
 * Search the open QuickTime atom reader file for a FairPlay user name atom and
 * return the value in aUserName.  If no FairPlay user name is present, return
 * NS_ERROR_FAILURE and do not modify aUserName.
 *
 * \param aUserName             FairPlay user name.
 */

nsresult
QTAtomReader::GetFairPlayUserName(nsAString& aUserName)
{
  PRUint32 atom[2];
  PRUint64 offset = 0;
  nsresult rv;

  // Get the starting end offset.
  PRInt64  fileSize;
  PRUint64 endOffset;
  rv = mFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  endOffset = fileSize;

  // Set the atom header size.
  mAtomHdrSize = 8;

  // Get the sample description table atom.
  rv = AtomPathGet("/moov/trak/mdia/minf/stbl/stsd",
                   &atom,
                   &offset,
                   &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay DRM atom, skipping the sample description table
  // atom header.
  offset += 16;
  rv = AtomPathGet("/drms", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay user name atom, skipping the FairPlay DRM atom header.
  offset += 0x24;
  rv = AtomPathGet("/sinf/schi/name", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay user name size and offset.
  offset += 8;
  PRUint32 userNameSize = (PRUint32)(endOffset - offset);

  // Allocate a user name buffer and set up for auto-disposal.
  char* userNameBuffer = (char*) NS_Alloc(userNameSize + 1);
  NS_ENSURE_TRUE(userNameBuffer, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoUserNameBuffer(userNameBuffer);

  // Read the user name.
  PRUint32 bytesRead;
  rv = mSeekableStream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mInputStream->Read(userNameBuffer, userNameSize, &bytesRead);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(bytesRead >= userNameSize, NS_ERROR_FAILURE);
  userNameBuffer[userNameSize] = '\0';

  // Return results.
  aUserName.Assign(NS_ConvertUTF8toUTF16(userNameBuffer));

  return NS_OK;
}


/**
 * Search the open QuickTime atom reader file for a FairPlay user ID atom and
 * return the value in aUserID.  If no FairPlay user ID is present, return
 * NS_ERROR_FAILURE and do not modify aUserID.
 *
 * \param aUserID               FairPlay user ID.
 */

nsresult
QTAtomReader::GetFairPlayUserID(PRUint32* aUserID)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aUserID);

  // Function variables.
  PRUint32 atom[2];
  PRUint64 offset = 0;
  nsresult rv;

  // Get the starting end offset.
  PRInt64  fileSize;
  PRUint64 endOffset;
  rv = mFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  endOffset = fileSize;

  // Set the atom header size.
  mAtomHdrSize = 8;

  // Get the sample description table atom.
  rv = AtomPathGet("/moov/trak/mdia/minf/stbl/stsd",
                   &atom,
                   &offset,
                   &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay DRM atom, skipping the sample description table
  // atom header.
  offset += 16;
  rv = AtomPathGet("/drms", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay user ID atom, skipping the FairPlay DRM atom header.
  offset += 0x24;
  rv = AtomPathGet("/sinf/schi/user", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the user ID.
  PRUint32 userIDAtom[3];
  PRUint32 bytesRead;
  PRUint32 userID;
  rv = mSeekableStream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mInputStream->Read((char *) userIDAtom, sizeof(userIDAtom), &bytesRead);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(bytesRead >= sizeof(userIDAtom), NS_ERROR_FAILURE);
  userID = PR_ntohl(userIDAtom[2]);

  // Return results.
  *aUserID = userID;

  return NS_OK;
}


/**
 * Search the open iPod iEKInfo file for all FairPlay user ID atoms and return
 * their values in aUserIDList.
 *
 * \param aUserIDList           List of FairPlay user IDs.
 */

nsresult
QTAtomReader::GetIEKInfoUserIDs(nsTArray<PRUint32>& aUserIDList)
{
  PRUint32 atom[2];
  PRUint64 offset;
  nsresult rv;

  // Get the starting offsets.
  PRInt64  fileSize;
  PRUint64 endOffset;
  rv = mFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  offset = 0x4C;
  endOffset = fileSize;

  // Get the root atom.
  rv = AtomPathGet("/sean", &atom, &offset, &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  offset += mAtomHdrSize;

  // Get the FairPlay user IDs.
  while (1) {
    // Get the next FairPlay user ID atom.
    PRUint64 userIDEndOffset = endOffset;
    rv = AtomPathGet("/user", &atom, &offset, &userIDEndOffset);
    if (NS_FAILED(rv))
      break;

    // Read the user ID.
    PRUint32 userIDAtom[3];
    PRUint32 userID;
    PRUint32 bytesRead;
    rv = mSeekableStream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mInputStream->Read((char *) userIDAtom,
                            sizeof(userIDAtom),
                            &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(bytesRead >= sizeof(userIDAtom), NS_ERROR_FAILURE);
    userID = PR_ntohl(userIDAtom[2]);

    // Add the user ID to the list.
    aUserIDList.AppendElement(userID);

    // Advance past the user ID atom.
    offset += atom[0];
  }

  return NS_OK;
}


/**
 * Search the QuickTime atom reader file for an atom with the path specified by
 * aAtomPath.  Start the search at the file offset specified by aStartOffset and
 * continue up to the offset specified by aEndOffset.  If the atom is found,
 * return its header in aAtom and update aStartOffset with the starting offset
 * of the atom header and aEndOffset with the end offset of the atom.  If the
 * atom is not found, return NS_ERROR_FAILURE.
 *
 * \param aAtomPath             Path to requested atom.
 * \param aAtom                 Requested atom.
 * \param aStartOffset          File offset from which to start searching.
 * \param aEndOffset            File offset from which to stop searching.
 */

nsresult
QTAtomReader::AtomPathGet(const char*     aAtomPath,
                          void*     aAtom,
                          PRUint64* aStartOffset,
                          PRUint64* aEndOffset)
{
  // Validate parameters.
  NS_ASSERTION(aAtomPath, "aAtomPath is null");
  NS_ASSERTION(aAtom, "aAtom is null");
  NS_ASSERTION(aStartOffset, "aStartOffset is null");
  NS_ASSERTION(aEndOffset, "aEndOffset is null");

  // Function variables.
  PRUint32 atom[2];
  nsresult rv;

  // Get the offsets.
  PRUint64 offset = *aStartOffset;
  PRUint64 endOffset = *aEndOffset;

  // Search for each atom in the specified path.
  int   numAtoms = strlen(aAtomPath) / 5;
  const char* pAtomType = aAtomPath;
  for (int i = 0; i < numAtoms; i++) {
    // Skip leading "/".
    pAtomType += 1;

    // Skip parent atom header.
    if (i > 0)
      offset += mAtomHdrSize;

    // Get the atom type.
    PRUint32 atomType = (pAtomType[0] << 24) |
                        (pAtomType[1] << 16) |
                        (pAtomType[2] << 8) |
                        pAtomType[3];
    pAtomType += 4;

    // Get the atom offset.
    rv = AtomGet(atomType, &atom, &offset, &endOffset);
    if (NS_FAILED(rv))
      return rv;
  }

  // Return results.
  memcpy(aAtom, atom, sizeof(atom));
  *aStartOffset = offset;
  *aEndOffset = endOffset;

  return NS_OK;
}


/**
 * Search the QuickTime atom reader file for an atom of the type specified by
 * aAtomType.  Start the search at the file offset specified by aStartOffset and
 * continue up to the offset specified by aEndOffset.  If the atom is found,
 * return its header in aAtom and update aStartOffset with the starting offset
 * of the atom header and aEndOffset with the end offset of the atom.  If the
 * atom is not found, return NS_ERROR_FAILURE.
 *
 * \param aAtomType             Type of atom to get.
 * \param aAtom                 Requested atom.
 * \param aStartOffset          File offset from which to start searching.
 * \param aEndOffset            File offset from which to stop searching.
 */

nsresult
QTAtomReader::AtomGet(PRUint32  aAtomType,
                      void*     aAtom,
                      PRUint64* aStartOffset,
                      PRUint64* aEndOffset)
{
  // Validate parameters.
  NS_ASSERTION(aAtom, "aAtom is null");
  NS_ASSERTION(aStartOffset, "aStartOffset is null");
  NS_ASSERTION(aEndOffset, "aEndOffset is null");

  // Function variables.
  PRUint32 atom[2];
  nsresult rv;

  // Get the offsets.
  PRUint64 offset = *aStartOffset;
  PRUint64 endOffset = *aEndOffset;

  // Search for the specified atom.
  PRBool found = PR_FALSE;
  while (offset < endOffset) {
    // Seek to the current offset.
    rv = mSeekableStream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
    NS_ENSURE_SUCCESS(rv, rv);

    // Read the current atom.
    PRUint32 bytesRead;
    rv = mInputStream->Read((char *) atom, sizeof(atom), &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(bytesRead >= sizeof(atom), NS_ERROR_FAILURE);

    // Convert the atom fields to host byte order.
    atom[0] = PR_ntohl(atom[0]);
    atom[1] = PR_ntohl(atom[1]);

    // Get the atom fields.
    PRUint32 curAtomSize = atom[0];
    PRUint32 curAtomType = atom[1];

    // Validate atom size.
    NS_ENSURE_TRUE(curAtomSize >= mAtomHdrSize, NS_ERROR_FAILURE);

    // Check if the current atom is the one to get.
    if (curAtomType == aAtomType) {
      found = PR_TRUE;
      break;
    } else {
      offset += curAtomSize;
    }
  }

  // Check if the atom was found.
  if (!found)
    return NS_ERROR_FAILURE;

  // Return results.
  memcpy(aAtom, atom, sizeof(atom));
  *aStartOffset = offset;
  *aEndOffset = offset + atom[0];

  return NS_OK;
}


