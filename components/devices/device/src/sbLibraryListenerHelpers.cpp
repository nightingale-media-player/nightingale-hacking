/* vim: set sw=2 :miv */
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

#include "sbLibraryListenerHelpers.h"

#include "sbIMediaItem.h"
#include "sbIMediaList.h"
#include "sbBaseDevice.h"

//sbBaseDeviceLibraryListener class.
NS_IMPL_THREADSAFE_ISUPPORTS2(sbBaseDeviceLibraryListener, 
                              sbIDeviceLibraryListener,
                              nsISupportsWeakReference);

sbBaseDeviceLibraryListener::sbBaseDeviceLibraryListener() 
: mDevice(nsnull),
  mIgnoreListener(PR_FALSE)
{
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

nsresult 
sbBaseDeviceLibraryListener::SetIgnoreListener(PRBool aIgnoreListener)
{
  mIgnoreListener = aIgnoreListener;
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceLibraryListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceLibraryListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  return NS_OK;
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

  // Always listen to all added lists.
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    rv = mDevice->ListenToList(list);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(mIgnoreListener) {
    return NS_OK;
  }

  //XXXAus: Before adding to queue, make sure it doesn't come from
  //another device. Ask DeviceManager for the device library
  //containing this item.
  if (list) {
    // new playlist
    rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_NEW_PLAYLIST,
                              aMediaItem, aMediaList, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Hide the item. It is the responsibility of the device to make the item
    // visible when the transfer is successful.
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN), 
                                 NS_LITERAL_STRING("1"));

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

  if(mIgnoreListener) {
    return NS_OK;
  }
  
  nsresult rv;
  
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_DELETE,
                            aMediaItem, aMediaList, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceLibraryListener::OnListCleared(sbIMediaList *aMediaList,
                                           PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  *aNoMoreForBatch = PR_FALSE;
  
  /* yay, we're going to wipe the device! */

  if(mIgnoreListener) {
    return NS_OK;
  }
  
  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_WIPE,
                            nsnull, aMediaList);
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

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_UPDATE,
                            aMediaItem, aMediaList);
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
  
  if(mIgnoreListener) {
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


//sbILocalDatabaseMediaListCopyListener
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceBaseLibraryCopyListener, 
                              sbILocalDatabaseMediaListCopyListener);

sbDeviceBaseLibraryCopyListener::sbDeviceBaseLibraryCopyListener()
: mDevice(nsnull),
  mIgnoreListener(PR_FALSE)
{

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

nsresult 
sbDeviceBaseLibraryCopyListener::SetIgnoreListener(PRBool aIgnoreListener)
{
  mIgnoreListener = aIgnoreListener;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryCopyListener::OnItemCopied(sbIMediaItem *aSourceItem, 
                                              sbIMediaItem *aDestItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestItem);

  nsresult rv;
  rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_READ,
                            aSourceItem, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseDeviceMediaListListener,
                              sbIMediaListListener)

sbBaseDeviceMediaListListener::sbBaseDeviceMediaListListener()
: mDevice(nsnull),
  mIgnoreListener(PR_FALSE)
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

nsresult 
sbBaseDeviceMediaListListener::SetIgnoreListener(PRBool aIgnoreListener)
{
  mIgnoreListener = aIgnoreListener;
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

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbILibrary> lib = do_QueryInterface(aMediaList);
  if (lib) {
    // umm, why are we listening to a library adding an item?
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

  if(mIgnoreListener) {
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

  if(mIgnoreListener) {
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
sbBaseDeviceMediaListListener::OnListCleared(sbIMediaList *aMediaList,
                                             PRBool *_retval)
{
  NS_NOTREACHED("Playlist clearing not yet implemented");
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbBaseDeviceMediaListListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  return NS_OK;
}
