/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include "sbLocalDatabaseMediaItem.h"

#include <nsMemory.h>
#include <nsIProgrammingLanguage.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <nsNetUtil.h>

NS_IMPL_ISUPPORTS_INHERITED4(sbLocalDatabaseMediaItem,
                             sbLocalDatabaseResourceProperty,
                             sbIMediaItem,
                             sbILocalDatabaseMediaItem,
                             nsIClassInfo,
                             nsISupportsWeakReference)

NS_IMPL_CI_INTERFACE_GETTER4(sbLocalDatabaseMediaItem,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbILocalDatabaseMediaItem,
                             nsISupportsWeakReference)

sbLocalDatabaseMediaItem::sbLocalDatabaseMediaItem(sbILocalDatabaseLibrary* aLibrary,
                          const nsAString& aGuid)
: sbLocalDatabaseResourceProperty(aLibrary, aGuid),
  mMediaItemId(0),
  mLibrary(aLibrary)
{
}

// sbIMediaItem
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetLibrary(sbILibrary** aLibrary)
{
  nsresult rv;
  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aLibrary = library);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetOriginLibrary(sbILibrary** aOriginLibrary)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetIsMutable(PRBool* aIsMutable)
{
  *aIsMutable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaCreated(PRInt32* aMediaCreated)
{
  NS_ENSURE_ARG_POINTER(aMediaCreated);

  nsAutoString str;
  nsresult rv = sbLocalDatabaseResourceProperty::GetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created"), str );
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aMediaCreated = str.ToInteger(&rv);

  return rv;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaCreated(PRInt32 aMediaCreated)
{
  nsAutoString str;
  str.AppendInt( aMediaCreated );

  nsresult rv = sbLocalDatabaseResourceProperty::SetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str );
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaUpdated(PRInt32* aMediaUpdated)
{
  NS_ENSURE_ARG_POINTER(aMediaUpdated);

  nsAutoString str;
  nsresult rv = sbLocalDatabaseResourceProperty::GetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str );
  NS_ENSURE_SUCCESS(rv, rv);

  *aMediaUpdated = str.ToInteger(&rv);

  return rv;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaUpdated(PRInt32 aMediaUpdated)
{
  nsAutoString str;
  str.AppendInt( aMediaUpdated );

  nsresult rv = sbLocalDatabaseResourceProperty::SetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str );
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentSrc(nsIURI** aContentSrc)
{
  NS_ENSURE_ARG_POINTER(aContentSrc);

  nsAutoString str;
  nsresult rv = sbLocalDatabaseResourceProperty::GetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"), str );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aContentSrc, str);
 
  return rv;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentSrc(nsIURI* aContentSrc)
{
  NS_ENSURE_ARG_POINTER(aContentSrc);

  nsCAutoString cstr;
  nsresult rv = aContentSrc->GetSpec( cstr );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbLocalDatabaseResourceProperty::SetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"), NS_ConvertUTF8toUTF16(cstr) );
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentLength(PRInt32* aContentLength)
{
  NS_ENSURE_ARG_POINTER(aContentLength);

  nsAutoString str;
  nsresult rv = sbLocalDatabaseResourceProperty::GetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength"), str );
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aContentLength = str.ToInteger(&rv);

  return rv;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentLength(PRInt32 aContentLength)
{
  nsAutoString str;
  str.AppendInt( aContentLength );

  nsresult rv = sbLocalDatabaseResourceProperty::SetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength"), str );
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentType(nsAString& aContentType)
{
  nsresult rv = sbLocalDatabaseResourceProperty::GetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType"), aContentType );
  NS_ENSURE_SUCCESS(rv, rv);
  
  return rv;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentType(const nsAString& aContentType)
{
  nsresult rv = sbLocalDatabaseResourceProperty::SetProperty( NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType"), aContentType );
  NS_ENSURE_SUCCESS(rv, rv);
  
  return rv;
}



NS_IMETHODIMP
sbLocalDatabaseMediaItem::TestIsAvailable(nsIObserver* aObserver)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenInputStream(PRUint32 aOffset,
                                          nsIInputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenOutputStream(PRUint32 aOffset,
                                           nsIOutputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::ToString(nsAString& _retval)
{
  nsAutoString buff;

  buff.AppendLiteral("MediaItem {guid: ");
  buff.Append(mGuid);
  buff.AppendLiteral("}");

  _retval = buff;
  return NS_OK;
}

// sbILocalDatabaseMediaItem
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaItemId(PRUint32 *_retval)
{
  if (mMediaItemId == 0) {
    nsresult rv = mLibrary->GetMediaItemIdForGuid(mGuid, &mMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *_retval = mMediaItemId;
  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseMediaItem)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetHelperForLanguage(PRUint32 language,
                                               nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetFlags(PRUint32 *aFlags)
{
// not yet  *aFlags = nsIClassInfo::THREADSAFE;
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}
