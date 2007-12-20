/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
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

#include "sbMetadataUtils.h"

#include <nsIURI.h>
#include <nsIFile.h>
#include <nsIFileURL.h>

#include <nsNetUtil.h>
#include <nsThreadUtils.h>

NS_IMPL_ISUPPORTS1(sbURIMetadataHelper, sbIURIMetadataHelper)

NS_IMETHODIMP
sbURIMetadataHelper::GetFileSize(const nsAString& aURISpec, PRInt64* aFileSize)
{
  NS_ENSURE_ARG_POINTER(aFileSize);
  NS_ASSERTION(NS_IsMainThread(), "sbURIMetadataHelper is main thread only!");

  nsresult rv;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is not a file url, just return the error
  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri, &rv);
  if (rv == NS_ERROR_NO_INTERFACE) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->GetFileSize(aFileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

