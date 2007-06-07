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

#include <nsIObserver.h>
#include <nsIURIChecker.h>
#include <nsIFileURL.h>
#include <nsAutoLock.h>
#include <nsNetUtil.h>
#include <nsXPCOM.h>
#include <prprf.h>
#include <sbStandardProperties.h>

static void AppendInt(nsAString &str, PRInt64 val)
{
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%lld", val);
  str.Append(NS_ConvertASCIItoUTF16(buf));
}

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
    SB_PROPERTY_CREATED,
    "created",
    PR_UINT32_MAX,
  },
  {
    SB_PROPERTY_UPDATED,
    "updated",
    PR_UINT32_MAX - 1,
  },
  {
    SB_PROPERTY_CONTENTURL,
    "content_url",
    PR_UINT32_MAX - 2,
  },
  {
    SB_PROPERTY_CONTENTMIMETYPE,
    "content_mime_type",
    PR_UINT32_MAX - 3,
  },
  {
    SB_PROPERTY_CONTENTLENGTH,
    "content_length",
    PR_UINT32_MAX - 4,
  }
};

NS_IMPL_THREADSAFE_ADDREF(sbLocalDatabaseMediaItem)
NS_IMPL_THREADSAFE_RELEASE(sbLocalDatabaseMediaItem)

NS_INTERFACE_MAP_BEGIN(sbLocalDatabaseMediaItem)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(sbILocalDatabaseResourceProperty)
  NS_INTERFACE_MAP_ENTRY(sbILocalDatabaseMediaItem)
  NS_INTERFACE_MAP_ENTRY(sbIMediaItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbILibraryResource, sbIMediaItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIMediaItem)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER5(sbLocalDatabaseMediaItem, nsIClassInfo,
                                                       nsISupportsWeakReference,
                                                       nsIRequestObserver,
                                                       sbILibraryResource,
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
sbLocalDatabaseMediaItem::GetCreated(PRInt64* aCreated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aCreated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_ConvertUTF8toUTF16(kStaticProperties[sbPropCreated].mName), str);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PR_sscanf( NS_ConvertUTF16toUTF8(str).get(), "%lld", aCreated);

  return rv;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetUpdated(PRInt64* aUpdated)
{
  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aUpdated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_ConvertUTF8toUTF16(kStaticProperties[sbPropUpdated].mName), str);
  NS_ENSURE_SUCCESS(rv, rv);

  PR_sscanf( NS_ConvertUTF16toUTF8(str).get(), "%lld", aUpdated);

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

  nsresult rv = NS_OK;

  if(mWritePending) {
    nsAutoLock lock(mPropertyBagLock);

    rv = mPropertyBag->Write();
    NS_ENSURE_SUCCESS(rv, rv);

    mWritePending = PR_FALSE;
  }

  return rv;
}

/**
 * See sbILibraryResource
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetPropertyNames(nsIStringEnumerator** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  NS_ENSURE_TRUE(mPropertyCacheLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPropertyBagLock, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = GetPropertyBag();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mPropertyBagLock);

  if(mPropertyBag) {
    rv = mPropertyBag->GetNames(_retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
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

  if(mPropertyBag) {
    rv = mPropertyBag->GetProperty(aName, _retval);
    if (NS_SUCCEEDED(rv) || rv == NS_ERROR_ILLEGAL_VALUE) {
      return rv;
    }
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

  nsresult rv = GetPropertyBag();
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsAutoLock lock(mPropertyBagLock);

    rv = NS_ERROR_NOT_AVAILABLE;
    if(mPropertyBag) {
      rv = mPropertyBag->SetProperty(aName, aValue);
      NS_ENSURE_SUCCESS(rv, rv);

      if(mWriteThrough) {
        rv = mPropertyBag->Write();
        mWritePending = PR_FALSE;
      }
      else {
        mWritePending = PR_TRUE;
      }
    }
  }

  mLibrary->NotifyListenersItemUpdated(this);

  return rv;
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
//XXXAus: This method is junk if we don't have a LDBRP base class anymore.
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

/*
 * See sbIMediaItem
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetOriginLibrary(sbILibrary** aOriginLibrary)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
 */

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
sbLocalDatabaseMediaItem::GetMediaCreated(PRInt64* aMediaCreated)
{
  NS_ENSURE_ARG_POINTER(aMediaCreated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CREATED), str);
  NS_ENSURE_SUCCESS(rv, rv);

  PR_sscanf( NS_ConvertUTF16toUTF8(str).get(), "%lld", aMediaCreated);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaCreated(PRInt64 aMediaCreated)
{
  nsAutoString str;
  AppendInt(str, aMediaCreated);

  nsresult rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CREATED), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetMediaUpdated(PRInt64* aMediaUpdated)
{
  NS_ENSURE_ARG_POINTER(aMediaUpdated);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING(SB_PROPERTY_UPDATED), str);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(
    PR_sscanf( NS_ConvertUTF16toUTF8(str).get(), "%lld", aMediaUpdated),
    NS_ERROR_FAILURE
  );

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetMediaUpdated(PRInt64 aMediaUpdated)
{
  nsAutoString str;
  AppendInt(str, aMediaUpdated);

  nsresult rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_UPDATED), str);
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
  nsresult rv = GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), str);
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

  rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                   NS_ConvertUTF8toUTF16(cstr));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentLength(PRInt64* aContentLength)
{
  NS_ENSURE_ARG_POINTER(aContentLength);

  nsAutoString str;
  nsresult rv = GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH), str);
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ENSURE_TRUE(
    PR_sscanf( NS_ConvertUTF16toUTF8(str).get(), "%lld", aContentLength),
    NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::SetContentLength(PRInt64 aContentLength)
{
  nsAutoString str;
  AppendInt(str, aContentLength);

  nsresult rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::GetContentType(nsAString& aContentType)
{
  nsresult rv = GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTMIMETYPE),
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
  nsresult rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTMIMETYPE),
                            aContentType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenInputStreamAsync(nsIStreamListener *aListener, nsISupports *aContext, nsIChannel** _retval)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(_retval);

  // Get our URI
  nsCOMPtr<nsIURI> pURI;
  rv = this->GetContentSrc( getter_AddRefs(pURI) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the IO service
  nsCOMPtr<nsIIOService> pIOService;
  pIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a new channel
  rv = pIOService->NewChannelFromURI(pURI, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set notification callbacks if possible
  nsCOMPtr<nsIInterfaceRequestor> pIIR = do_QueryInterface( aListener );
  if ( pIIR )
    (*_retval)->SetNotificationCallbacks( pIIR );

  // Open the channel to read asynchronously
  return (*_retval)->AsyncOpen( aListener, aContext );
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenInputStream(nsIInputStream** _retval)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(_retval);

  // Get our URI
  nsCOMPtr<nsIURI> pURI;
  rv = this->GetContentSrc( getter_AddRefs(pURI) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the IO service
  nsCOMPtr<nsIIOService> pIOService;
  pIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a new channel
  nsCOMPtr<nsIChannel> pChannel;
  rv = pIOService->NewChannelFromURI(pURI, getter_AddRefs(pChannel));
  NS_ENSURE_SUCCESS(rv, rv);

  return pChannel->Open(_retval);
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OpenOutputStream(nsIOutputStream** _retval)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIURI> pURI;
  rv = this->GetContentSrc( getter_AddRefs(pURI) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Why are these private functions on web browser persist not static inline methods somewhere?

  // From nsWebBrowserPersist.cpp#1160 - nsWebBrowserPersist::GetLocalFileFromURI( 
  nsCOMPtr<nsILocalFile> localFile;
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(pURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv))
      localFile = do_QueryInterface(file, &rv);
  }

  if (localFile) {
    // From nsWebBrowserPersist.cpp#2282 - nsWebBrowserPersist::MakeOutputStreamFromFile( 
    nsCOMPtr<nsIFileOutputStream> fileOutputStream =
        do_CreateInstance(NS_LOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    
    rv = fileOutputStream->Init(localFile, -1, -1, 0);
    NS_ENSURE_SUCCESS(rv, rv);
    
    NS_ENSURE_SUCCESS(CallQueryInterface(fileOutputStream, _retval), NS_ERROR_FAILURE);
    (*_retval)->AddRef();
  }
  else {
#if 0
    // From nsWebBrowserPersist.cpp#2311 - nsWebBrowserPersist::MakeOutputStreamFromURI( 
    PRUint32 segsize = 8192;
    PRUint32 maxsize = PRUint32(-1);
    nsCOMPtr<nsIStorageStream> storStream;
    nsresult rv = NS_NewStorageStream(segsize, maxsize, getter_AddRefs(storStream));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_SUCCESS(CallQueryInterface(storStream, _retval), NS_ERROR_FAILURE);
    (*_retval)->AddRef();
    // ??? Note that this never uses pURI?!?  WTF?
#else
    *_retval = nsnull;
#endif
  }
  
  return rv;
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

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::TestIsAvailable(nsIObserver* aObserver)
{
  // Create a URI Checker interface
  nsresult rv;
  nsCOMPtr<nsIURIChecker> pURIChecker = do_CreateInstance("@mozilla.org/network/urichecker;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set it up with the URI in question
  nsCOMPtr<nsIURI> pURI;
  rv = this->GetContentSrc( getter_AddRefs(pURI) );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pURIChecker->Init( pURI );
  NS_ENSURE_SUCCESS(rv, rv);

  // Do an async check with the given observer as the context
  rv = pURIChecker->AsyncCheck( this, aObserver );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See nsIRequestObserver
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OnStartRequest( nsIRequest *aRequest, nsISupports *aContext )
{
  // We don't care.  It's about to check if available.
  return NS_OK;
}

/**
 * See nsIRequestObserver
 */
NS_IMETHODIMP
sbLocalDatabaseMediaItem::OnStopRequest( nsIRequest *aRequest, nsISupports *aContext, PRUint32 aStatus )
{
  // Get the target observer
  nsresult rv;
  nsCOMPtr<nsIObserver> observer = do_QueryInterface( aContext, &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  // Tell it whether we succeed or fail
  nsAutoString data = ( aStatus == NS_BINDING_SUCCEEDED ) ? 
    NS_LITERAL_STRING( "true" ) : NS_LITERAL_STRING( "false" );
  observer->Observe( aRequest, "available", data.get() );

  return NS_OK;
}

NS_IMPL_ISUPPORTS2(sbLocalDatabaseIndexedMediaItem,
                   nsIClassInfo,
                   sbIIndexedMediaItem)

NS_IMPL_CI_INTERFACE_GETTER2(sbLocalDatabaseIndexedMediaItem,
                             nsIClassInfo,
                             sbIIndexedMediaItem)

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetIndex(PRUint32* aIndex)
{
  *aIndex = mIndex;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetMediaItem(sbIMediaItem** aMediaItem)
{
  NS_ADDREF(*aMediaItem = mMediaItem);
  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseIndexedMediaItem)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetHelperForLanguage(PRUint32 language,
                                                       nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseIndexedMediaItem::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

