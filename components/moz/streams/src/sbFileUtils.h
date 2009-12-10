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

#ifndef SBFILEUTILS_H_
#define SBFILEUTILS_H_

#include <nsCOMPtr.h>
#include <nsCRT.h>
#include <nsIFile.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include <nsStringGlue.h>
#include <sbMemoryUtils.h>

class nsIInputStream;
class nsIOutputStream;
class nsIURI;

/**
 * Helper function to open a stream given a file path
 */
nsresult sbOpenInputStream(nsAString const & aPath, nsIInputStream ** aStream);

/**
 * Helper function to open a stream given a file URL
 */
nsresult sbOpenInputStream(nsIURI * aURI, nsIInputStream ** aStream);

/**
 * Helper function to open a stream given a file
 */
nsresult sbOpenInputStream(nsIFile * aFile, nsIInputStream ** aStream);

/**
 * Helper function to open an output stream given a file
 */
nsresult sbOpenOutputStream(nsIFile * aFile, nsIOutputStream ** aStream);

/**
 * Helper function to open an output stream given a file path
 */
nsresult sbOpenOutputStream(nsAString const & aPath,
                            nsIOutputStream ** aStream);
/**
 * Read a file into a buffer
 */
nsresult sbReadFile(nsIFile * aFile, nsACString &aBuffer);

/**
 * From xpcom/io/nsStreamUtils.h
 */
nsresult
sbConsumeStream(nsIInputStream *aSource, PRUint32 aMaxCount,
                nsACString &aBuffer);

/**
 * Return in aURI a URI object for the file specified by aFile.  Avoids Songbird
 * bug 6227 in nsIIOService.newFileURI.
 *
 * \param aFile File for which to get URI.
 * \param aURI Returned file URI.
 */
nsresult sbNewFileURI(nsIFile* aFile,
                      nsIURI** aURI);

/**
 * Remove all bad file name characters from the file name specified by
 * aFileName.  If aAllPlatforms is true, remove characters that are bad on any
 * platform; otherwise, only remove characters that are bad on the current
 * platform.
 *
 * \param aFileName File name from which to remove bad characters.
 * \param aAllPlatforms If true, remove characters that are bad on any platform.
 */
void RemoveBadFileNameCharacters(nsAString& aFileName,
                                 PRBool     aAllPlatforms);


//
// Auto-disposal class wrappers.
//
//   sbAutoInputStream          Wrapper to auto-close an input stream.
//   sbAutoOutputStream         Wrapper to auto-close an output stream.
//   sbAutoRemoveFile           Warpper to auto-remove an nsIFile.
//

SB_AUTO_NULL_CLASS(sbAutoInputStream,
                   nsCOMPtr<nsIInputStream>,
                   mValue->Close());
SB_AUTO_NULL_CLASS(sbAutoOutputStream,
                   nsCOMPtr<nsIOutputStream>,
                   mValue->Close());
SB_AUTO_NULL_CLASS(sbAutoRemoveFile,
                   nsCOMPtr<nsIFile>,
                   mValue->Remove(PR_FALSE));


//
// Default file permissions.
//

#define SB_DEFAULT_FILE_PERMISSIONS 0644


//
// Bad file name characters.
//
//   SB_FILE_BAD_CHARACTERS     Bad file name characters, including file path
//                              separators.
//   SB_FILE_BAD_CHARACTERS_ALL_PLATFORMS
//                              Bad file name characters for all platforms,
//                              including file path separators for all
//                              platforms.
//

#define SB_FILE_BAD_CHARACTERS FILE_ILLEGAL_CHARACTERS FILE_PATH_SEPARATOR
#define SB_FILE_BAD_CHARACTERS_ALL_PLATFORMS CONTROL_CHARACTERS "/\\:*?\"<>|"

#endif /* SBFILEUTILS_H_ */

