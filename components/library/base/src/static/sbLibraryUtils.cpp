/* vim: set sw=2 : */
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

#include "sbLibraryUtils.h"

#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIProxyObjectManager.h>
#include <nsIThread.h>
#include <nsIURI.h>

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsStringAPI.h>
#include <nsThreadUtils.h>

#include <sbILibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <sbIPropertyArray.h>

#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include "sbMediaListEnumSingleItemHelper.h"
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

/* static */
nsresult sbLibraryUtils::GetItemInLibrary(/* in */  sbIMediaItem*  aItem,
                                          /* in */  sbILibrary*    aLibrary,
                                          /* out */ sbIMediaItem** _retval)
{
  /* this is an internal method, just assert/crash, don't be nice */
  NS_ASSERTION(aItem, "no item to look up!");
  NS_ASSERTION(aLibrary, "no library to search in!");
  NS_ASSERTION(_retval, "null return value pointer!");

  nsresult rv;

  nsCOMPtr<sbILibrary> itemLibrary;
  rv = aItem->GetLibrary(getter_AddRefs(itemLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  {
    PRBool isSameLib;
    rv = itemLibrary->Equals(aLibrary, &isSameLib);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isSameLib) {
      /* okay, so we want to find the same item... */
      NS_ADDREF(*_retval = aItem);
      return NS_OK;
    }
  }
  

  // check if aItem originally was from aLibrary
  nsString originLibraryGuid, originItemGuid;
  nsString sourceLibraryGuid, sourceItemGuid;
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                            originLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                            originItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsString targetLibraryGuid;
    rv = aLibrary->GetGuid(targetLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    if (targetLibraryGuid.Equals(originLibraryGuid)) {
      // the media item originally came from the target library
      // find the media item with the matching guid
      rv = aLibrary->GetMediaItem(originItemGuid, _retval);
      if (NS_FAILED(rv)) {
        *_retval = nsnull;
      }
      return NS_OK;
    }
  }


  // check if some item in aLibrary was originally a copy of aItem
  {
    rv = itemLibrary->GetGuid(sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = aItem->GetGuid(sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                                   sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                   sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsRefPtr<sbMediaListEnumSingleItemHelper> helper =
      sbMediaListEnumSingleItemHelper::New();
    NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);
  
    rv = aLibrary->EnumerateItemsByProperties(propArray,
                                              helper,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIMediaItem> item = helper->GetItem();
    if (item) {
      item.forget(_retval);
      return NS_OK;
    }
  }
  
  
  // check if there is some item in aLibrary that was a copy of the same thing
  // that aItem was a copy of
  if (!originLibraryGuid.IsEmpty() && !originItemGuid.IsEmpty()) {
    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                                   sourceLibraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                   sourceItemGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsRefPtr<sbMediaListEnumSingleItemHelper> helper =
      sbMediaListEnumSingleItemHelper::New();
    NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);

    rv = aLibrary->EnumerateItemsByProperties(propArray,
                                              helper,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIMediaItem> item = helper->GetItem();
    if (item) {
      item.forget(_retval);
      return NS_OK;
    }
  }

  // give up
  *_retval = nsnull;
  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetContentLength(/* in */  sbIMediaItem * aItem,
                                          /* out */ PRInt64      * _retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = aItem->GetContentLength(_retval);

  if(NS_FAILED(rv) || !*_retval) {
    // try to get the length from disk
    nsCOMPtr<sbIMediaItem> item(aItem);
    
    if (!NS_IsMainThread()) {
      // Proxy item to get contentURI.
      // Note that we do *not* call do_GetProxyForObject if we're already on
      // the main thread - doing that causes us to process the next pending event
      nsCOMPtr<nsIThread> target;
      rv = NS_GetMainThread(getter_AddRefs(target));
  
      rv = do_GetProxyForObject(target,
                                NS_GET_IID(sbIMediaItem),
                                aItem,
                                NS_PROXY_SYNC,
                                getter_AddRefs(item));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIURI> contentURI;
    rv = item->GetContentSrc(getter_AddRefs(contentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(contentURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // note that this will abort if this is not a local file.  This is the
    // desired behaviour.

    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = file->GetFileSize(_retval);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString strContentLength;
    AppendInt(strContentLength, *_retval);

    rv = aItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                            strContentLength);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetOriginItem(/* in */ sbIMediaItem*   aItem,
                                       /* out */ sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // Get the origin library and item GUIDs.
  nsAutoString originLibraryGUID;
  nsAutoString originItemGUID;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                          originLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                          originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the origin item.
  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibrary> library;
  rv = libraryManager->GetLibrary(originLibraryGUID, getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = library->GetItemByGuid(originItemGUID, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

inline
nsCOMPtr<nsIIOService> GetIOService(nsresult & rv) 
{
  // Get the IO service.
  if (NS_IsMainThread()) { 
    return do_GetIOService(&rv);
  }
  return do_ProxiedGetService(NS_IOSERVICE_CONTRACTID, &rv);
}

/* static */
nsresult sbLibraryUtils::GetContentURI(nsIURI*  aURI,
                                       nsIURI** _retval,
                                       nsIIOService * aIOService)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIURI> uri = aURI;
  nsresult rv;

  // On Windows, convert "file:" URI's to lower-case.
#ifdef XP_WIN
  PRBool isFileScheme;
  rv = uri->SchemeIs("file", &isFileScheme);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isFileScheme) {
    // Get the URI spec.
    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Convert the URI spec to lower case.
    ToLowerCase(spec);

    // Regenerate the URI.
    nsCOMPtr<nsIIOService> ioService = aIOService ? aIOService : 
                                                    GetIOService(rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = ioService->NewURI(spec, nsnull, nsnull, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif // XP_WIN

  // Return results.
  NS_ADDREF(*_retval = uri);

  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetFileContentURI(nsIFile* aFile,
                                           nsIURI** _retval)
{
  NS_ENSURE_ARG_POINTER(aFile);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIURI> uri;
  nsresult rv;

  // Get the IO service.
  nsCOMPtr<nsIIOService> ioService = GetIOService(rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Note that NewFileURI is broken on Linux when dealing with
  // file names not in the filesystem charset; see bug 6227
#if XP_UNIX && !XP_MACOSX
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile, &rv);
  if (NS_SUCCEEDED(rv)) {
    // Use the local file persistent descriptor to form a URI spec.
    nsCAutoString descriptor;
    rv = localFile->GetPersistentDescriptor(descriptor);
    if (NS_SUCCEEDED(rv)) {
      // Escape the descriptor into a spec.
      nsCOMPtr<nsINetUtil> netUtil =
        do_CreateInstance("@mozilla.org/network/util;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCAutoString spec;
      rv = netUtil->EscapeString(descriptor,
                                 nsINetUtil::ESCAPE_URL_PATH,
                                 spec);
      NS_ENSURE_SUCCESS(rv, rv);

      // Add the "file:" scheme.
      spec.Insert("file://", 0);

      // Create the URI.
      rv = ioService->NewURI(spec, nsnull, nsnull, getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
#endif

  // Get a URI directly from the file.
  if (!uri) {
    rv = ioService->NewFileURI(aFile, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Convert URI to a content URI.
  return GetContentURI(uri, _retval, ioService);
}

/**
 * Enumerator class that populates the array it is given
 */
class MediaItemArrayCreator : public sbIMediaListEnumerationListener
{
public:
  MediaItemArrayCreator(nsCOMArray<sbIMediaItem> & aMediaItems) : 
    mMediaItems(aMediaItems)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  nsCOMArray<sbIMediaItem> & mMediaItems;
};

NS_IMPL_ISUPPORTS1(MediaItemArrayCreator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP MediaItemArrayCreator::OnEnumerationBegin(sbIMediaList*,
                                                         PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP MediaItemArrayCreator::OnEnumeratedItem(sbIMediaList*,
                                                      sbIMediaItem* aItem,
                                                      PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  PRBool const added = mMediaItems.AppendObject(aItem);
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP MediaItemArrayCreator::OnEnumerationEnd(sbIMediaList*,
                                                      nsresult)
{
  return NS_OK;
}


nsresult 
sbLibraryUtils::GetItemsByProperty(sbIMediaList * aMediaList,
                                   nsAString const & aPropertyName, 
                                   nsAString const & aValue,
                                   nsCOMArray<sbIMediaItem> & aMediaItems) {
  nsRefPtr<MediaItemArrayCreator> creator = new MediaItemArrayCreator(aMediaItems);
  return aMediaList->EnumerateItemsByProperty(aPropertyName,
                                              aValue,
                                              creator,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
}
