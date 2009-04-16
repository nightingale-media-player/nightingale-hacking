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

#include <nsIFileStreams.h>
#include <nsILocalFile.h>
#include <nsNetCID.h>
#include <nsStringAPI.h>

/**
 * Helper function to open a stream given a file path
 */
inline
nsresult sbOpenInputStream(nsAString const & aPath, nsIInputStream ** aStream) {
  nsresult rv;
  nsCOMPtr<nsILocalFile> file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = file->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIFileInputStream> stream = do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->Init(file, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aStream = stream.forget().get();
  
  return NS_OK;
}
#endif /* SBFILEUTILS_H_ */
