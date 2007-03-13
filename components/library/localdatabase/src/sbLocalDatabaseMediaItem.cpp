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

NS_IMPL_ISUPPORTS2(sbLocalDatabaseMediaItem, sbIMediaItem, nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbLocalDatabaseMediaItem,
                             sbILibraryResource,
                             sbIMediaItem)

sbLocalDatabaseMediaItem::sbLocalDatabaseMediaItem(sbILibrary* aLibrary,
                                                   const nsAString& aGuid) :
 mLibrary(aLibrary),
 mGuid(aGuid)
{
}

sbLocalDatabaseMediaItem::~sbLocalDatabaseMediaItem()
{
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetLibrary(sbILibrary** aLibrary)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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
sbLocalDatabaseMediaItem::GetGuid(nsAString& aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaCreated(PRInt32* aMediaCreated)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaCreated(PRInt32 aMediaCreated)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaUpdated(PRInt32* aMediaUpdated)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaUpdated(PRInt32 aMediaUpdated)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::TestIsAvailable(nsIObserver* aObserver)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentSrc(nsIURI** aContentSrc)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentSrc(nsIURI* aContentSrc)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentLength(PRInt32* aContentLength)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentLength(PRInt32 aContentLength)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentType(nsAString& aContentType)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentType(const nsAString& aContentType)
{
  /*
   * TODO: Forward this to the resource property stuff
   */
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

NS_IMETHODIMP
sbLocalDatabaseMediaItem::Equals(sbIMediaItem* aOtherItem,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aOtherItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString otherGUID;
  nsresult rv = aOtherItem->GetGuid(otherGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mGuid.Equals(otherGUID);
  return NS_OK;
}

// sbILibraryResource
// XXX - Can this be agg'd?
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetUri(nsIURI** aUri)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetCreated(PRInt32* aCreated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetUpdated(PRInt32* aUpdated)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetProperty(const nsAString& aName,
                                      nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetProperty(const nsAString& aName,
                                      const nsAString& aValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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

