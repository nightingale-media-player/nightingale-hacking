/* vim: set sw=2 :miv */
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

#include "sbLibraryListenerHelpers.h"

#include <pratom.h>
#include <prlog.h>

#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbIOrderableMediaList.h"
#include "sbBaseDevice.h"
#include "sbLibraryUtils.h"

#include <sbDebugUtils.h>

#include <nsIURI.h>
#include <nsNetUtil.h>

//
// To log this module, set the following environment variable:
//   NSPR_LOG_MODULES=sbLibraryListenerHelpers:5
//

nsresult
sbBaseIgnore::SetIgnoreListener(PRBool aIgnoreListener) {
  if (aIgnoreListener) {
    PR_AtomicIncrement(&mIgnoreListenerCounter);
  } else {
    PRInt32 SB_UNUSED_IN_RELEASE(result) = PR_AtomicDecrement(&mIgnoreListenerCounter);
    NS_ASSERTION(result >= 0, "invalid device library ignore listener counter");
  }
  return NS_OK;
}

nsresult sbBaseIgnore::IgnoreMediaItem(sbIMediaItem * aItem) {
  NS_ENSURE_ARG_POINTER(aItem);

  nsString guid;
  nsresult rv = aItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);

  PRInt32 itemCount = 0;
  // We don't care if this fails, itemCount is zero in that case which is fine
  // We have to assume failure is always due to "not found"
  mIgnored.Get(guid, &itemCount);
  if (!mIgnored.Put(guid, ++itemCount))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

/**
 * Returns PR_TRUE if the item is currently being ignored
 */
PRBool sbBaseIgnore::MediaItemIgnored(sbIMediaItem * aItem) {
  NS_ENSURE_ARG_POINTER(aItem);

  nsString guid;
  // If ignoring all or ignoring this specific item return PR_TRUE
  if (mIgnoreListenerCounter > 0)
    return PR_TRUE;
  nsAutoLock lock(mLock);
  nsresult rv = aItem->GetGuid(guid);

  // If the guid was valid and it's in our ignore list then it's ignored
  return (NS_SUCCEEDED(rv) && mIgnored.Get(guid, nsnull)) ? PR_TRUE :
                                                            PR_FALSE;
}

nsresult sbBaseIgnore::UnignoreMediaItem(sbIMediaItem * aItem) {
  nsString guid;
  nsresult rv = aItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);
  PRInt32 itemCount = 0;
  if (!mIgnored.Get(guid, &itemCount)) {
    // We're out of balance at this point
    return NS_ERROR_FAILURE;
  }
  // If the item count is less than zero then remove the guid else just decrement it
  if (--itemCount == 0) {
    mIgnored.Remove(guid);
  }
  else
    mIgnored.Put(guid, itemCount);
  return NS_OK;
}

/**
 * Enumerator class that populates the array it is given
 */
class MediaItemContentSrcArrayCreator : public sbIMediaListEnumerationListener
{
public:
  MediaItemContentSrcArrayCreator(nsIMutableArray* aURIs) :
    mURIs(aURIs)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  nsCOMPtr<nsIMutableArray> mURIs;
};

NS_IMPL_ISUPPORTS1(MediaItemContentSrcArrayCreator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP
MediaItemContentSrcArrayCreator::OnEnumerationBegin(sbIMediaList*,
                                                    PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
MediaItemContentSrcArrayCreator::OnEnumeratedItem(sbIMediaList*,
                                                  sbIMediaItem* aItem,
                                                  PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // If the item is a media list, get the playlist URL property.  Otherwise,
  // get the media item content source.
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aItem, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsAutoString playlistURL;
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PLAYLISTURL),
                            playlistURL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(playlistURL));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aItem->GetContentSrc(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Add the URI.
  rv = mURIs->AppendElement(uri, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
MediaItemContentSrcArrayCreator::OnEnumerationEnd(sbIMediaList*,
                                                  nsresult)
{
  return NS_OK;
}

//sbBaseDeviceLibraryListener class.
NS_IMPL_THREADSAFE_ISUPPORTS2(sbBaseDeviceLibraryListener,
                              sbIDeviceLibraryListener,
                              nsISupportsWeakReference);

sbBaseDeviceLibraryListener::sbBaseDeviceLibraryListener()
: mDevice(nsnull)
{
  SB_PRLOG_SETUP(sbLibraryListenerHelpers);
}

sbBaseDeviceLibraryListener::~sbBaseDeviceLibraryListener()
{
}

nsresult
sbBaseDeviceLibraryListener::Init(sbBaseDevice* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mDevice = aDevice;

  return NS_OK;
}

void
sbBaseDeviceLibraryListener::Destroy()
{
  mDevice = nsnull;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_BATCH_BEGIN,
                              nsnull, aMediaList);
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  nsRefPtr<nsISupports> grip(NS_ISUPPORTS_CAST(sbIDevice*, mDevice));
  NS_ENSURE_STATE(grip);
  return mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_BATCH_END,
                              nsnull, aMediaList);
}

inline bool
IsItemHidden(sbIMediaItem * aMediaItem)
{
  nsString hidden;
  nsresult rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                        hidden);
  return NS_SUCCEEDED(rv) && hidden.Equals(NS_LITERAL_STRING("1"));
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnItemAdded(sbIMediaList *aMediaList,
                                         sbIMediaItem *aMediaItem,
                                         PRUint32 aIndex,
                                         PRBool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;

  nsresult rv;

  // Skip hidden media items
  if (IsItemHidden(aMediaList)) {
    return NS_OK;
  }

  // Hide the item. It is the responsibility of the device to make the item
  // visible when the transfer is successful.
  // Listen to all added lists unless hidden.
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    rv = mDevice->ListenToList(list);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  //XXXAus: Before adding to queue, make sure it doesn't come from
  //another device. Ask DeviceManager for the device library
  //containing this item.
  if (list && !IsItemHidden(list)) {
    // new playlist
    rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_NEW_PLAYLIST,
                              aMediaItem, aMediaList, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    IgnoreMediaItem(aMediaItem);
    // Hide the item. It is the responsibility of the device to make the item
    // visible when the transfer is successful.
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                 NS_LITERAL_STRING("1"));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed to hide newly added item");

    UnignoreMediaItem(aMediaItem);

    rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_WRITE,
                              aMediaItem, aMediaList, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                                 sbIMediaItem *aMediaItem,
                                                 PRUint32 aIndex,
                                                 PRBool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                                sbIMediaItem *aMediaItem,
                                                PRUint32 aIndex,
                                                PRBool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  // Skip hidden media items
  if (IsItemHidden(aMediaItem) || IsItemHidden(aMediaList)) {
    return NS_OK;
  }

  nsresult rv;

  /* If the item is hidden, then it wasn't transferred to the device (which
     will clear the hidden property once the transfer is complete), so don't
     delete it
   */
  nsString hiddenValue;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                               hiddenValue);
  if (NS_SUCCEEDED(rv) && hiddenValue.Equals(NS_LITERAL_STRING("1")))
    return NS_OK;

  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_DELETE,
                            aMediaItem, aMediaList, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnListCleared(sbIMediaList *aMediaList,
                                           PRBool aExcludeLists,
                                           PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;

  return NS_OK;
}
NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                                 PRBool aExcludeLists,
                                                 PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  /* yay, we're going to wipe the device! */
  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  // Skip hidden media items
  if (IsItemHidden(aMediaList)) {
    return NS_OK;
  }

  // We capture the list of content that's going to be deleted here
  // and hand it over to the device as an nsIArray<nsIURIs> through
  // the TransferRequest's data member.
  nsCOMPtr<nsIMutableArray> uris =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<MediaItemContentSrcArrayCreator> creator =
                                  new MediaItemContentSrcArrayCreator(uris);
  if (aExcludeLists) {
    rv = aMediaList->EnumerateItemsByProperty
                       (NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                        NS_LITERAL_STRING("0"),
                        creator,
                        sbIMediaList::ENUMERATIONTYPE_LOCKING);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aMediaList->EnumerateAllItems(creator,
                                       sbIMediaList::ENUMERATIONTYPE_LOCKING);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbBaseDevice::TransferRequest> req = sbBaseDevice::TransferRequest::New();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  req->type = sbBaseDevice::TransferRequest::REQUEST_WIPE;
  req->item = aMediaList;
  req->list = nsnull;
  req->index = PR_UINT32_MAX;
  req->otherIndex = PR_UINT32_MAX;
  req->priority = sbBaseDevice::TransferRequest::PRIORITY_DEFAULT;

  // the important bit
  req->data = uris;

  rv = mDevice->PushRequest(req);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnItemUpdated(sbIMediaList *aMediaList,
                                           sbIMediaItem *aMediaItem,
                                           sbIPropertyArray* aProperties,
                                           PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;

  if(MediaItemIgnored(aMediaItem)) {
    return NS_OK;
  }

  // Skip hidden media items
  if (IsItemHidden(aMediaItem)) {
    return NS_OK;
  }

  nsresult rv;

  nsRefPtr<sbBaseDevice::TransferRequest>
    req = sbBaseDevice::TransferRequest::New();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);
  req->type = sbBaseDevice::TransferRequest::REQUEST_UPDATE;
  req->item = aMediaItem;
  req->list = aMediaList;
  req->data = aProperties;
  req->index = PR_UINT32_MAX;
  req->otherIndex = PR_UINT32_MAX;
  req->priority = sbBaseDevice::TransferRequest::PRIORITY_DEFAULT;

  rv = mDevice->PushRequest(req);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnItemMoved(sbIMediaList *aMediaList,
                                         PRUint32 aFromIndex,
                                         PRUint32 aToIndex,
                                         PRBool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  // Skip hidden media items
  if (IsItemHidden(aMediaList)) {
    return NS_OK;
  }

  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_MOVE,
                            nsnull, aMediaList, aFromIndex, aToIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnItemCopied(sbIMediaItem *aSourceItem,
                                          sbIMediaItem *aDestItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestItem);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBeforeCreateMediaItem(nsIURI *aContentUri,
                                                     sbIPropertyArray *aProperties,
                                                     PRBool aAllowDuplicates,
                                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aContentUri);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBeforeCreateMediaList(const nsAString & aType,
                                                     sbIPropertyArray *aProperties,
                                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceLibraryListener::OnBeforeAdd(sbIMediaItem *aMediaItem,
                                         PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP sbBaseDeviceLibraryListener::OnBeforeAddAll(sbIMediaList *aMediaList,
                                                          PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP sbBaseDeviceLibraryListener::OnBeforeAddSome(nsISimpleEnumerator *aMediaItems,
                                                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP sbBaseDeviceLibraryListener::OnBeforeClear(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

NS_IMETHODIMP sbBaseDeviceLibraryListener::OnSyncSettings(
                                   PRUint32 aAction,
                                   sbIDeviceLibrarySyncSettings * aSyncSettings)
{
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  return NS_OK;
}

//sbILocalDatabaseMediaListCopyListener
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceBaseLibraryCopyListener,
                              sbILocalDatabaseMediaListCopyListener);

sbDeviceBaseLibraryCopyListener::sbDeviceBaseLibraryCopyListener()
: mDevice(nsnull)
{
  SB_PRLOG_SETUP(sbLibraryListenerHelpers);
}

sbDeviceBaseLibraryCopyListener::~sbDeviceBaseLibraryCopyListener()
{

}

nsresult
sbDeviceBaseLibraryCopyListener::Init(sbBaseDevice* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mDevice = aDevice;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryCopyListener::OnItemCopied(sbIMediaItem *aSourceItem,
                                              sbIMediaItem *aDestItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestItem);

  nsresult rv;

  #if PR_LOGGING
  nsCOMPtr<nsIURI> srcURI, destURI;
  nsCString srcSpec, destSpec;
  nsCOMPtr<sbILibrary> srcLib, destLib;
  nsString srcLibId, destLibId;
  rv = aSourceItem->GetContentSrc(getter_AddRefs(srcURI));
  if (NS_SUCCEEDED(rv)) {
    rv = srcURI->GetSpec(srcSpec);
  }
  rv = aSourceItem->GetLibrary(getter_AddRefs(srcLib));
  if (NS_SUCCEEDED(rv)) {
    rv = srcLib->GetGuid(srcLibId);
  }
  rv = aDestItem->GetContentSrc(getter_AddRefs(destURI));
  if (NS_SUCCEEDED(rv)) {
    rv = destURI->GetSpec(destSpec);
  }
  rv = aDestItem->GetLibrary(getter_AddRefs(destLib));
  if (NS_SUCCEEDED(rv)) {
    rv = destLib->GetGuid(destLibId);
  }
  TRACE("%s: %s::%s -> %s::%s",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(srcLibId).get(), srcSpec.get(),
         NS_ConvertUTF16toUTF8(destLibId).get(), destSpec.get());
  #endif

  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_READ,
                            aSourceItem, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseDeviceMediaListListener,
                              sbIMediaListListener)

sbBaseDeviceMediaListListener::sbBaseDeviceMediaListListener()
: mDevice(nsnull)
{

}

sbBaseDeviceMediaListListener::~sbBaseDeviceMediaListListener()
{

}

nsresult
sbBaseDeviceMediaListListener::Init(sbBaseDevice* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_FALSE(mDevice, NS_ERROR_ALREADY_INITIALIZED);
  mDevice = aDevice;
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnItemAdded(sbIMediaList *aMediaList,
                                           sbIMediaItem *aMediaItem,
                                           PRUint32 aIndex,
                                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbILibrary> lib = do_QueryInterface(aMediaList);
  if (lib) {
    // umm, why are we listening to a library adding an item?
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    // a list being added to a list? we don't care, I think?
  } else {
    rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_WRITE,
                              aMediaItem, aMediaList, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                                   sbIMediaItem *aMediaItem,
                                                   PRUint32 aIndex,
                                                   PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                                  sbIMediaItem *aMediaItem,
                                                  PRUint32 aIndex,
                                                  PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  nsresult rv;

  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_DELETE,
                            aMediaItem, aMediaList, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnItemUpdated(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             sbIPropertyArray *aProperties,
                                             PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnItemMoved(sbIMediaList *aMediaList,
                                           PRUint32 aFromIndex,
                                           PRUint32 aToIndex,
                                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_MOVE,
                            nsnull, aMediaList, aFromIndex, aToIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                                   PRBool aExcludeLists,
                                                   PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnListCleared(sbIMediaList *aMediaList,
                                             PRBool aExcludeLists,
                                             PRBool * /* aNoMoreForBatch */)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  // Check if we're ignoring then do nothing

  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }

  // Send the wipe request
  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_WIPE,
                            aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }
  return mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_BATCH_BEGIN,
                              nsnull, aMediaList);
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  if(MediaItemIgnored(aMediaList)) {
    return NS_OK;
  }
  return mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_BATCH_END,
                              nsnull, aMediaList);
}
