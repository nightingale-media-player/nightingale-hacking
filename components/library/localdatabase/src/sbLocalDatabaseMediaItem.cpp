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

#include <nsIProgrammingLanguage.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabasePropertyCache.h>

#include <nsAutoLock.h>
#include <nsNetUtil.h>
#include <nsXPCOM.h>
#include <prprf.h>

struct sbStaticProperty {
  const char* mName;
  const char* mColumn;
  PRUint32    mID;
};

enum {
  sbPropCreated = 0,
  sbPropUpdated,
  sbPropContentUrl,
  sbPropMimeType,
  sbPropContentLength,
};

static sbStaticProperty kStaticProperties[] = {
  {
    "http://songbirdnest.com/data/1.0#created",
      "created",
      PR_UINT32_MAX,
  },
  {
    "http://songbirdnest.com/data/1.0#updated",
      "updated",
      PR_UINT32_MAX - 1,
  },
  {
    "http://songbirdnest.com/data/1.0#contentUrl",
      "content_url",
      PR_UINT32_MAX - 2,
  },
  {
    "http://songbirdnest.com/data/1.0#contentMimeType",
      "content_mime_type",
      PR_UINT32_MAX - 3,
  },
  {
    "http://songbirdnest.com/data/1.0#contentLength",
      "content_length",
      PR_UINT32_MAX - 4,
  }
};

NS_IMPL_ADDREF(sbLocalDatabaseMediaItem)
NS_IMPL_RELEASE(sbLocalDatabaseMediaItem)

NS_INTERFACE_MAP_BEGIN(sbLocalDatabaseMediaItem)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(sbILocalDatabaseResourceProperty)
  NS_INTERFACE_MAP_ENTRY(sbILocalDatabaseMediaItem)
  NS_INTERFACE_MAP_ENTRY(sbIMediaItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbILibraryResource, sbIMediaItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIMediaItem)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER6(sbLocalDatabaseMediaItem, nsIClassInfo,
                                                       nsISupportsWeakReference,
                                                       sbILibraryResource,
                                                       sbILocalDatabaseResourceProperty,
                                                       sbILocalDatabaseMediaItem,
                                                       sbIMediaItem)

sbLocalDatabaseMediaItem::sbLocalDatabaseMediaItem()
: mMediaItemId(0),
  mLibrary(nsnull),
  mPropertyCacheLock(nsnull),
  mPropertyBagLock(nsnull),
  mGuidLock(nsnull),
  mWriteThrough(PR_FALSE),
  mWritePending(PR_FALSE)
{
}

sbLocalDatabaseMediaItem::~sbLocalDatabaseMediaItem()
{
  if(mGuidLock) {
    nsAutoLock::DestroyLock(mGuidLock);
  }

  if(mPropertyCacheLock) {
    nsAutoLock::DestroyLock(mPropertyCacheLock);
  }

  if(mPropertyBagLock) {
    nsAutoLock::DestroyLock(mPropertyBagLock);
  }
}


/**
 * \brief Initializes the media item.
 *
 * \param aLibrary - The library that owns this media item.
 * \param aGuid    - The GUID of the media item.
 */
nsresult
sbLocalDatabaseMediaItem::Init(sbILocalDatabaseLibrary* aLibrary,
                               const nsAString& aGuid)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG(!aGuid.IsEmpty());

  mLibrary = aLibrary;
  mGuid.Assign(aGuid);

  mGuidLock =
    nsAutoLock::NewLock("sbLocalDatabaseMediaItem::mGuidLock");
  NS_ENSURE_TRUE(mGuidLock, NS_ERROR_OUT_OF_MEMORY);

  mPropertyCacheLock =
    nsAutoLock::NewLock("sbLocalDatabaseMediaItem::mPropertyCacheLock");
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_OUT_OF_MEMORY);

  mPropertyBagLock =
    nsAutoLock::NewLock("sbLocalDatabaseMediaItem::mPropertyBagLock");
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mLibrary->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Makes sure that the property bag is available.
 */
nsresult
sbLocalDatabaseMediaItem::GetPropertyBag()
{
  nsAutoLock lock(mPropertyBagLock);

  if (mPropertyBag)
    return NS_OK;

  nsresult rv;

  // Sometimes the property bag isn't ready for us to get during construction
  // because the cache isn't done loading. Check here to see if we need
  // to grab it.

  //XXX: If cache is invalidated, refresh now?
  PRUint32 count = 0;
  const PRUnichar** guids = new const PRUnichar*[1];
  sbILocalDatabaseResourcePropertyBag** bags;

  guids[0] = PromiseFlatString(mGuid).get();

  {
    nsAutoLock cacheLock(mPropertyCacheLock);
    rv = mPropertyCache->GetProperties(guids, 1, &count, &bags);
  }

  if (NS_SUCCEEDED(rv)) {
    if (count > 0 && bags[0]) {
      mPropertyBag = bags[0];

    }

    NS_Free(bags);
  }

  delete[] guids;

  return rv;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetInterfaces(PRUint32* count,
                                        nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseMediaItem)(count, array);
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetHelperForLanguage(PRUint32 language,
                                               nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetFlags(PRUint32 *aFlags)
{
// not yet  *aFlags = nsIClassInfo::THREADSAFE;
  *aFlags = 0;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetGuid(nsAString& aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetCreated(PRInt32* aCreated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aCreated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_ConvertUTF8toUTF16(kStaticProperties[sbPropCreated].mName), str);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aCreated = str.ToInteger(&rv);

  return rv;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetUpdated(PRInt32* aUpdated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aUpdated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_ConvertUTF8toUTF16(kStaticProperties[sbPropUpdated].mName), str);
  NS_ENSURE_SUCCESS(rv, rv);

  *aUpdated = str.ToInteger(&rv);

  return rv;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP 
sbLocalDatabaseMediaItem::GetWriteThrough(PRBool* aWriteThrough)
{
  NS_ENSURE_ARG_POINTER(aWriteThrough);
  *aWriteThrough = mWriteThrough;
  return NS_OK;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetWriteThrough(PRBool aWriteThrough)
{
  mWriteThrough = aWriteThrough;
  return NS_OK;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetWritePending(PRBool* aWritePending)
{
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = mWritePending;
  return NS_OK;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::Write()
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetProperty(const nsAString& aName, 
                                      nsAString& _retval)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = GetPropertyBag();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mPropertyBagLock);

  rv = NS_ERROR_NOT_AVAILABLE;
  if(mPropertyBag)
  {
    rv = mPropertyBag->GetProperty(aName, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetProperty(const nsAString& aName, 
                                      const nsAString& aValue)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::Equals(sbILibraryResource* aOtherLibraryResource,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aOtherLibraryResource);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString otherGUID;
  nsresult rv = aOtherLibraryResource->GetGuid(otherGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mGuid.Equals(otherGUID);
  return NS_OK;
}

/**
 * See sbILocalDatabaseResourceProperty
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::InitResourceProperty(sbILocalDatabasePropertyCache* aPropertyCache, 
                                               const nsAString& aGuid)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aPropertyCache);

  nsAutoLock lock(mPropertyCacheLock);
  
  mPropertyCache = aPropertyCache;
  mGuid = aGuid;

  return NS_OK;
}

/**
 * See sbILocalDatabaseMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaItemId(PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (mMediaItemId == 0) {
    nsresult rv = mLibrary->GetMediaItemIdForGuid(mGuid, &mMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *_retval = mMediaItemId;
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetLibrary(sbILibrary** aLibrary)
{
  nsresult rv;
  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aLibrary = library);
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetOriginLibrary(sbILibrary** aOriginLibrary)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetIsMutable(PRBool* aIsMutable)
{
  *aIsMutable = PR_TRUE;
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaCreated(PRInt32* aMediaCreated)
{
  NS_ENSURE_ARG_POINTER(aMediaCreated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created"), str);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aMediaCreated = str.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaCreated(PRInt32 aMediaCreated)
{
  nsAutoString str;
  str.AppendInt(aMediaCreated);

  nsresult rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaUpdated(PRInt32* aMediaUpdated)
{
  NS_ENSURE_ARG_POINTER(aMediaUpdated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  *aMediaUpdated = str.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaUpdated(PRInt32 aMediaUpdated)
{
  nsAutoString str;
  str.AppendInt(aMediaUpdated);

  nsresult rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentSrc(nsIURI** aContentSrc)
{
  NS_ENSURE_ARG_POINTER(aContentSrc);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aContentSrc, str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentSrc(nsIURI* aContentSrc)
{
  NS_ENSURE_ARG_POINTER(aContentSrc);

  nsCAutoString cstr;
  nsresult rv = aContentSrc->GetSpec(cstr);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl"),
                   NS_ConvertUTF8toUTF16(cstr));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentLength(PRInt32* aContentLength)
{
  NS_ENSURE_ARG_POINTER(aContentLength);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength"), str);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aContentLength = str.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentLength(PRInt32 aContentLength)
{
  nsAutoString str;
  str.AppendInt(aContentLength);

  nsresult rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength"), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentType(nsAString& aContentType)
{
  nsresult rv = GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType"),
                            aContentType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentType(const nsAString& aContentType)
{
  nsresult rv = SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType"),
                            aContentType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::TestIsAvailable(nsIObserver* aObserver)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenInputStream(PRUint32 aOffset,
                                          nsIInputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenOutputStream(PRUint32 aOffset,
                                           nsIOutputStream** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbIMediaItem
 */
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
