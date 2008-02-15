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

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsresult rv;

  //XXXAus: Before adding to queue, make sure it doesn't come from
  //another device. Ask DeviceManager for the device library
  //containing this item.
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    // new playlist
    rv = mDevice->PushRequest(sbBaseDevice::TransferRequest::REQUEST_NEW_PLAYLIST,
                              aMediaItem, aMediaList, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
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
