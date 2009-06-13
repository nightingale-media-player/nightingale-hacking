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

#include <nsStringGlue.h>

class nsIFile;
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


#endif /* SBFILEUTILS_H_ */
