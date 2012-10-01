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

/** 
* \file  DeviceBase.cpp
* \brief Songbird DeviceBase Component Implementation.
*/

#include "DeviceBase.h"
#include "sbIDeviceBase.h"

#include <nsAppDirectoryServiceDefs.h>
#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCRTGlue.h>
#include <nsComponentManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsILocalFile.h>
#include <nsIURI.h>
#include <nsIURIFixup.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <nsProxyRelease.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>

#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbIPropertyArray.h>
#include <sbLocalDatabaseCID.h>
#include <sbStandardProperties.h>

#define MSG_DEVICE_BASE             (0x2000) // Base message ID

#define MSG_DEVICE_TRANSFER         (MSG_DEVICE_BASE + 0)
#define MSG_DEVICE_UPLOAD           (MSG_DEVICE_BASE + 1)
#define MSG_DEVICE_DELETE           (MSG_DEVICE_BASE + 2)
#define MSG_DEVICE_UPDATE_METADATA  (MSG_DEVICE_BASE + 3)
#define MSG_DEVICE_EVENT            (MSG_DEVICE_BASE + 4)
#define MSG_DEVICE_INITIALIZE       (MSG_DEVICE_BASE + 5)
#define MSG_DEVICE_FINALIZE         (MSG_DEVICE_BASE + 6)
#define MSG_DEVICE_EJECT            (MSG_DEVICE_BASE + 7)

#define TRANSFER_TABLE_NAME         NS_LITERAL_STRING("Transfer")
#define URL_COLUMN_NAME             NS_LITERAL_STRING("url")
#define SOURCE_COLUMN_NAME          NS_LITERAL_STRING("source")
#define DESTINATION_COLUMN_NAME     NS_LITERAL_STRING("destination")
#define INDEX_COLUMN_NAME           NS_LITERAL_STRING("id")

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceBase:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo *gUSBMassStorageDeviceLog = nsnull;
#define TRACE(args) if (gUSBMassStorageDeviceLog) PR_LOG(gUSBMassStorageDeviceLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (gUSBMassStorageDeviceLog) PR_LOG(gUSBMassStorageDeviceLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

//Utility functions.
void 
ReplaceChars(nsAString& aOldString,
             const nsAString& aOldChars,
             const PRUnichar aNewChar)
{
  PRUint32 length = aOldString.Length();
  for (PRUint32 index = 0; index < length; index++) {
    PRUnichar currentChar = aOldString.CharAt(index);
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar);
    if (oldCharsIndex > -1)
      aOldString.Replace(index, 1, aNewChar);
  }
}

void 
ReplaceChars(nsACString& aOldString,
             const nsACString& aOldChars,
             const char aNewChar)
{
  PRUint32 length = aOldString.Length();
  for (PRUint32 index = 0; index < length; index++) {
    char currentChar = aOldString.CharAt(index);
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar);
    if (oldCharsIndex > -1)
      aOldString.Replace(index, 1, aNewChar);
  }
}

//sbDeviceBaseLibraryListener class.
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceBaseLibraryListener, 
                              sbIMediaListListener);

/* static */ PLDHashOperator PR_CALLBACK
sbDeviceBaseLibraryListener::ShiftIndexesCallback(nsISupportsHashKey::KeyType aKey,
                                                  PRUint32& aEntry,
                                                  void* aUserData)
{
  NS_ASSERTION(aUserData, "Null userData!");

  PRUint32* index = static_cast<PRUint32*>(aUserData);
  NS_ENSURE_TRUE(index, PL_DHASH_STOP);

  if (aEntry > *index) {
    aEntry--;
  }

  return PL_DHASH_NEXT;
}

sbDeviceBaseLibraryListener::sbDeviceBaseLibraryListener() 
: mDevice(nsnull),
  mIgnoreListener(PR_FALSE),
  mManagePlaylists(PR_FALSE)
{
}

sbDeviceBaseLibraryListener::~sbDeviceBaseLibraryListener()
{
}

nsresult
sbDeviceBaseLibraryListener::Init(const nsAString &aDeviceIdentifier, 
                                  sbIDeviceBase* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mDeviceIdentifier = aDeviceIdentifier;
  mDevice = aDevice;
  bool success = mBeforeRemoveIndexes.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult 
sbDeviceBaseLibraryListener::SetIgnoreListener(bool aIgnoreListener)
{
  mIgnoreListener = aIgnoreListener;
  return NS_OK;
}

nsresult 
sbDeviceBaseLibraryListener::SetManagePlaylists(bool aManagePlaylists)
{
  mManagePlaylists = aManagePlaylists;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryListener::OnItemAdded(sbIMediaList *aMediaList,
                                         sbIMediaItem *aMediaItem,
                                         PRUint32 aIndex,
                                         bool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  *aNoMoreForBatch = PR_FALSE;

  nsresult rv;

  //XXXAus: Before adding to queue, make sure it doesn't come from
  //another device. Ask DeviceManager for the device library
  //containing this item.

  nsCOMPtr<sbILibrary> library;
  rv = aMediaList->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  bool destinationIsLibrary;
  rv = aMediaList->Equals(library, &destinationIsLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem, &rv);
  bool addedIsList = NS_SUCCEEDED(rv);

  // If we're managing playlists and a list is being added to the library, we
  // need to attach a listener so we can track changes to the list
  if (mManagePlaylists && destinationIsLibrary && addedIsList) {
    rv = list->AddListener(this,
                           PR_FALSE,
                           sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                           sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED |
                           sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                           sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                           sbIMediaList::LISTENER_FLAGS_ITEMMOVED |
                           sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                           nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<sbRemovedItemIndexes> removedIndexes(new sbRemovedItemIndexes());
    NS_ENSURE_TRUE(removedIndexes, NS_ERROR_OUT_OF_MEMORY);

    bool success = removedIndexes->Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    success = mBeforeRemoveIndexes.Put(list, removedIndexes);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    removedIndexes.forget();
  }

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsCOMPtr<nsIMutableArray> items =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 transferItemCount = 0;
  if (mManagePlaylists) {
    if (destinationIsLibrary) {
      if (addedIsList) {
        rv = mDevice->CreatePlaylists(mDeviceIdentifier,
                                      items,
                                      &transferItemCount);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        nsCOMPtr<nsIURI> uri;
        //XXXAus: Read this from a library pref???
        rv = mDevice->TransferItems(mDeviceIdentifier,
                                    items,
                                    uri,
                                    sbIDeviceBase::OP_UPLOAD,
                                    PR_TRUE,
                                    nsnull,
                                    &transferItemCount);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      if (addedIsList) {
        NS_WARNING("DeviceBase does not support lists of lists!");
      }
      else {
        // An item is getting added to a list
        rv = mDevice->AddToPlaylist(mDeviceIdentifier,
                                    aMediaList,
                                    items,
                                    0,
                                    &transferItemCount);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  else {
    nsCOMPtr<nsIURI> uri;
    rv = mDevice->TransferItems(mDeviceIdentifier,
                                items,
                                uri,
                                sbIDeviceBase::OP_UPLOAD,
                                PR_TRUE,
                                nsnull,
                                &transferItemCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                                 sbIMediaItem *aMediaItem,
                                                 PRUint32 aIndex,
                                                 bool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  nsresult rv;
  *aNoMoreForBatch = PR_FALSE;

  // If we're not managing playlists, nothing to do here
  if (!mManagePlaylists) {
    return NS_OK;
  }

  if(mIgnoreListener) {
    return NS_OK;
  }

  // We need to track the indexes of items that are about to be removed from
  // lists.  Note that this will not work as expected when removing an item
  // from a list that has multiple instances of the same item.
  nsCOMPtr<sbILibrary> library;
  rv = aMediaList->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  bool fromListIsLibrary;
  rv = aMediaList->Equals(library, &fromListIsLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!fromListIsLibrary) {
    PRUint32 index;
    rv = aMediaList->IndexOf(aMediaItem, 0, &index);
    NS_ENSURE_SUCCESS(rv, rv);

    sbRemovedItemIndexes* removedIndexes;
    bool success = mBeforeRemoveIndexes.Get(aMediaList, &removedIndexes);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    success = removedIndexes->Put(aMediaItem, index);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnAfterItemRemoved(sbIMediaList *aMediaList, 
                                                sbIMediaItem *aMediaItem,
                                                PRUint32 aIndex,
                                                bool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  nsresult rv;

  *aNoMoreForBatch = PR_FALSE;

  nsCOMPtr<sbILibrary> library;
  rv = aMediaList->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  bool fromListIsLibrary;
  rv = aMediaList->Equals(library, &fromListIsLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem, &rv);
  bool removedIsList = NS_SUCCEEDED(rv);

  if (mManagePlaylists && fromListIsLibrary && removedIsList) {
    rv = list->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsCOMPtr<nsIMutableArray> items =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 deleteItemCount;
  if (mManagePlaylists) {
    if (fromListIsLibrary) {
      if (removedIsList) {
        rv = mDevice->DeletePlaylists(mDeviceIdentifier, items, &deleteItemCount);
        NS_ENSURE_SUCCESS(rv, rv);

        mBeforeRemoveIndexes.Remove(aMediaList);
      }
      else {
        rv = mDevice->DeleteItems(mDeviceIdentifier, items, &deleteItemCount);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      if (removedIsList) {
        NS_WARNING("Removal of lists that contain lists is not suported");
      }
      else {
        sbRemovedItemIndexes* removedIndexes;
        bool success = mBeforeRemoveIndexes.Get(aMediaList, &removedIndexes);
        NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

        PRUint32 index;
        bool found = removedIndexes->Get(aMediaItem, &index);
        if (found) {
          removedIndexes->Remove(aMediaItem);
          rv = mDevice->RemoveFromPlaylist(mDeviceIdentifier,
                                           aMediaList,
                                           aMediaItem,
                                           index,
                                           &deleteItemCount);
          NS_ENSURE_SUCCESS(rv, rv);

          // Removing an item from the playlist will shift the indexes of the
          // items that come after it, so we shift the indexes in the
          // mBeforeRemoveIndexes list to keep them in sync.
          removedIndexes->Enumerate(ShiftIndexesCallback, &index);
        }
        else {
          NS_WARNING("OnAfterItemRemoved on item not in mBeforeRemoveIndexes");
        }
      }
    }
  }
  else {
    rv = mDevice->DeleteItems(mDeviceIdentifier, items, &deleteItemCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnItemUpdated(sbIMediaList *aMediaList,
                                           sbIMediaItem *aMediaItem,
                                           sbIPropertyArray* aProperties,
                                           bool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  *aNoMoreForBatch = PR_FALSE;

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<nsIMutableArray> items;
  items = do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 updateItemCount;
  rv = mDevice->UpdateItems(mDeviceIdentifier, items, &updateItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryListener::OnItemMoved(sbIMediaList *aMediaList,
                                         PRUint32 aFromIndex,
                                         PRUint32 aToIndex,
                                         bool *aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  *aNoMoreForBatch = PR_FALSE;

  if(mIgnoreListener) {
    return NS_OK;
  }

  if (aFromIndex == aToIndex) {
    return NS_OK;
  }

  PRUint32 updateItemCount;
  nsresult rv;
  rv = mDevice->MovePlaylistItem(mDeviceIdentifier,
                                 aMediaList,
                                 aFromIndex,
                                 aToIndex,
                                 &updateItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;

}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                                 bool aExcludeLists,
                                                 bool* aNoMoreForBatch)
{
  /* Validate parameters. */
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnListCleared(sbIMediaList *aMediaList,
                                           bool aExcludeLists,
                                           bool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  *aNoMoreForBatch = PR_FALSE;

  if(mIgnoreListener) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbILibrary> library;
  rv = aMediaList->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  bool fromListIsLibrary;
  rv = aMediaList->Equals(library, &fromListIsLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 deletedItemCount = 0;
  if (!mManagePlaylists || fromListIsLibrary) {
    rv = mDevice->DeleteAllItems(mDeviceIdentifier, &deletedItemCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mDevice->ClearPlaylist(mDeviceIdentifier,
                                aMediaList,
                                &deletedItemCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnBatchEnd(sbIMediaList *aMediaList)
{
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
sbDeviceBaseLibraryCopyListener::Init(const nsAString &aDeviceIdentifier, 
                                      sbIDeviceBase* aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mDeviceIdentifier = aDeviceIdentifier;
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
  nsCOMPtr<nsIMutableArray> items;

  items = do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aSourceItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  //XXXAus: Get URI from pref for library???

  nsCOMPtr<sbILibrary> library;
  rv = aDestItem->GetLibrary(getter_AddRefs(library));

  PRUint32 transferItemCount = 0;
  rv = mDevice->TransferItems(mDeviceIdentifier, 
                              items, 
                              uri, 
                              sbIDeviceBase::OP_DOWNLOAD, 
                              PR_TRUE, 
                              library, 
                              &transferItemCount);

  return NS_OK;
}

//sbDeviceBaseCallbackProxy class.
NS_IMPL_THREADSAFE_ADDREF(sbDeviceBaseCallbackProxy)
NS_IMPL_THREADSAFE_RELEASE(sbDeviceBaseCallbackProxy)

sbDeviceBaseCallbackProxy::sbDeviceBaseCallbackProxy()
{
}

sbDeviceBaseCallbackProxy::~sbDeviceBaseCallbackProxy()
{
  // Release callback proxy from the owning thread.
  if (mCallbackProxy && mOwningThread)
  {
    sbIDeviceBaseCallback *callbackProxy = nsnull;
    mCallbackProxy.swap(callbackProxy);
    NS_ProxyRelease(mOwningThread, callbackProxy);
  }
}

nsresult sbDeviceBaseCallbackProxy::Init(sbIDeviceBaseCallback* aCallback)
{
  NS_ASSERTION(aCallback, "aCallback is null");

  nsCOMPtr<nsIProxyObjectManager> proxyObjectManager;
  nsresult rv;

  // Get the callback owning thread so proxy can be released on it.
  nsCOMPtr<nsIThread> thread;
  rv = NS_GetCurrentThread(getter_AddRefs(thread));
  NS_ENSURE_SUCCESS(rv, rv);
  mOwningThread = do_QueryInterface(thread, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Proxy callback so callbacks can be made from other threads.
  proxyObjectManager = do_CreateInstance("@mozilla.org/xpcomproxy;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = proxyObjectManager->GetProxyForObject
                                  (NS_PROXY_TO_CURRENT_THREAD,
                                   NS_GET_IID(sbIDeviceBaseCallback),
                                   aCallback,
                                   nsIProxyObjectManager::INVOKE_ASYNC |
                                   nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                   getter_AddRefs(mCallbackProxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return (NS_OK);
}

//sbDeviceBase class.
sbDeviceBase::sbDeviceBase()
{
#ifdef PR_LOGGING
  if (!gUSBMassStorageDeviceLog) {
    gUSBMassStorageDeviceLog = PR_NewLogModule("sbDeviceBase");
  }
#endif
}

sbDeviceBase::~sbDeviceBase()
{
}

nsresult
sbDeviceBase::Init()
{
  NS_ENSURE_TRUE(mDeviceLibraries.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mDeviceQueues.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mDeviceCallbacks.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mDeviceStates.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mDeviceLibraryListeners.Init(), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbDeviceBase::AddCallback(sbIDeviceBaseCallback* aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  nsRefPtr<sbDeviceBaseCallbackProxy> callbackProxy;
  nsresult rv;

  // Proxy callback so callbacks can be made from other threads.
  callbackProxy = new sbDeviceBaseCallbackProxy();
  NS_ENSURE_TRUE(callbackProxy, NS_ERROR_OUT_OF_MEMORY);
  rv = callbackProxy->Init(aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  if(mDeviceCallbacks.Put(aCallback, callbackProxy)) {
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
sbDeviceBase::RemoveCallback(sbIDeviceBaseCallback* aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);
 
  mDeviceCallbacks.Remove(aCallback);

  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDeviceCallback(nsISupports *key, sbDeviceBaseCallbackProxy *data, void *closure)
{
  nsCOMArray<sbIDeviceBaseCallback> *array = static_cast<nsCOMArray<sbIDeviceBaseCallback> *>(closure);
  array->AppendObject(data->mCallbackProxy);
  return PL_DHASH_NEXT;
}

void
sbDeviceBase::DoTransferStartCallback(sbIMediaItem *aMediaItem)
{
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDeviceBaseCallback> callbackSnapshot;
  mDeviceCallbacks.EnumerateRead(EnumDeviceCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount) 
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDeviceBaseCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback) {
      try {
        callback->OnTransferStart(aMediaItem);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnTransferStart threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoTransferCompleteCallback(sbIMediaItem* aMediaItem,
                                         PRInt32 aStatus)
{
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDeviceBaseCallback> callbackSnapshot;
  mDeviceCallbacks.EnumerateRead(EnumDeviceCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount) 
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDeviceBaseCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback) {
      try {
        callback->OnTransferComplete(aMediaItem, aStatus);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnTransferComplete threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoDeviceConnectCallback(const nsAString& aDeviceString)
{
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDeviceBaseCallback> callbackSnapshot;
  mDeviceCallbacks.EnumerateRead(EnumDeviceCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount) 
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDeviceBaseCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback) {
      try {
        callback->OnDeviceConnect(aDeviceString);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnDeviceConnect threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoDeviceDisconnectCallback(const nsAString& aDeviceString)
{
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDeviceBaseCallback> callbackSnapshot;
  mDeviceCallbacks.EnumerateRead(EnumDeviceCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount) 
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDeviceBaseCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback) {
      try {
        callback->OnDeviceDisconnect(aDeviceString);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnDeviceDisconnect threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoStateChangedCallback(const nsAString& aDeviceString,
                                     PRUint32 aState)
{
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDeviceBaseCallback> callbackSnapshot;
  mDeviceCallbacks.EnumerateRead(EnumDeviceCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount) 
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDeviceBaseCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback) {
      try {
        callback->OnStateChanged(aDeviceString, aState);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnStateChanged threw an exception");
      }
    }
  }
}

nsresult 
sbDeviceBase::CreateDeviceLibrary(const nsAString &aDeviceIdentifier,
                                  nsIURI *aDeviceDatabaseURI,
                                  sbIDeviceBase *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsCOMPtr<sbILibraryFactory> libraryFactory = 
    do_GetService(SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> libraryProps = 
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> libraryFile;
  if(aDeviceDatabaseURI) {
    //Use preferred database location.
    nsCOMPtr<nsIFileURL> furl = do_QueryInterface(aDeviceDatabaseURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = furl->GetFile(getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, rv);
  } 
  else {
    //No preferred database location, fetch default location.
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, 
      getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryFile->Append(NS_LITERAL_STRING("db"));
    NS_ENSURE_SUCCESS(rv, rv);

    bool exists = PR_FALSE;
    rv = libraryFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if(!exists) {
      rv = libraryFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsAutoString filename(aDeviceIdentifier);
    filename.AppendLiteral(".db");

    rv = libraryFile->Append(filename);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = libraryProps->SetPropertyAsInterface(NS_LITERAL_STRING("databaseFile"), 
                                            libraryFile);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  {
    nsCAutoString str;
    if(NS_SUCCEEDED(libraryFile->GetNativePath(str))) {
      LOG(("Attempting to create device library with file path %s", str.get()));
    }
  }
#endif

  nsCOMPtr<sbILibrary> library;
  rv = libraryFactory->CreateLibrary(libraryProps, getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceBaseLibraryListener> listener;
  NS_NEWXPCOM(listener, sbDeviceBaseLibraryListener);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  rv = listener->Init(aDeviceIdentifier, aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list;
  list = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = list->AddListener(listener,
                         PR_FALSE,
                         sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                         sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                         sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                         sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetListenerForDeviceLibrary(aDeviceIdentifier, listener);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSimpleMediaList> simpleList;
  simpleList = do_QueryInterface(list, &rv);
  
  if(NS_SUCCEEDED(rv)) {
    nsRefPtr<sbDeviceBaseLibraryCopyListener> copyListener;
    NS_NEWXPCOM(copyListener, sbDeviceBaseLibraryCopyListener);
    NS_ENSURE_TRUE(copyListener, NS_ERROR_OUT_OF_MEMORY);

    rv = copyListener->Init(aDeviceIdentifier, aDevice);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = simpleList->SetCopyListener(copyListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_WARN_IF_FALSE(rv, 
      "Failed to get sbILocalDatabaseSimpleMediaList interface. Copy Listener disabled.");
  }

  if(mDeviceLibraries.Put(nsAutoString(aDeviceIdentifier), library)) {
    return NS_OK;
  }
  
  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult 
sbDeviceBase::RemoveDeviceLibrary(const nsAString &aDeviceIdentifier)
{
  mDeviceLibraries.Remove(aDeviceIdentifier);
  return NS_OK;
}

nsresult 
sbDeviceBase::GetLibraryForDevice(const nsAString &aDeviceIdentifier,
                                  sbILibrary* *aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);

  if(mDeviceLibraries.Get(nsAutoString(aDeviceIdentifier), aDeviceLibrary)) {
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::RegisterDeviceLibrary(sbILibrary* aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);
  
  nsresult rv;
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager = 
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return libraryManager->RegisterLibrary(aDeviceLibrary, PR_FALSE);
}

nsresult 
sbDeviceBase::UnregisterDeviceLibrary(sbILibrary* aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);

  nsresult rv;
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager = 
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return libraryManager->UnregisterLibrary(aDeviceLibrary);
}

nsresult 
sbDeviceBase::CreateTransferQueue(const nsAString &aDeviceIdentifier)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> deviceQueue = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if(mDeviceQueues.Put(nsAutoString(aDeviceIdentifier), deviceQueue)) {
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult 
sbDeviceBase::RemoveTransferQueue(const nsAString &aDeviceIdentifier)
{
  mDeviceQueues.Remove(nsAutoString(aDeviceIdentifier));
  return NS_OK;
}

nsresult 
sbDeviceBase::AddItemToTransferQueue(const nsAString &aDeviceIdentifier, 
                                     sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(aDeviceIdentifier, getter_AddRefs(deviceQueue))) {
    return deviceQueue->AppendElement(aMediaItem, PR_FALSE);
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::RemoveItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                          sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  
  nsresult rv;
  PRUint32 index = 0;

  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(aDeviceIdentifier, getter_AddRefs(deviceQueue))) {
    rv = deviceQueue->IndexOf(0, aMediaItem, &index);
    NS_ENSURE_SUCCESS(rv, rv);

    return deviceQueue->RemoveElementAt(index);
  }

  return NS_OK;
}

nsresult 
sbDeviceBase::GetNextItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                           sbIMediaItem* *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(aDeviceIdentifier, getter_AddRefs(deviceQueue))) {
    return deviceQueue->QueryElementAt(0, 
                                       NS_GET_IID(sbIMediaItem), 
                                       (void **)aMediaItem);
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::GetItemByIndexFromTransferQueue(const nsAString &aDeviceIdentifier, 
                                              PRUint32 aItemIndex,
                                              sbIMediaItem* *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  
  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(aDeviceIdentifier, getter_AddRefs(deviceQueue))) {
    return deviceQueue->QueryElementAt(aItemIndex, 
                                       NS_GET_IID(sbIMediaItem), 
                                       (void **)aMediaItem);
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::GetTransferQueue(const nsAString &aDeviceIdentifier,
                               nsIMutableArray* *aTransferQueue)
{
  NS_ENSURE_ARG_POINTER(aTransferQueue);
  
  *aTransferQueue = nsnull;
  if(mDeviceQueues.Get(aDeviceIdentifier, aTransferQueue)) {
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::ClearTransferQueue(const nsAString &aDeviceIdentifier)
{
  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(nsAutoString(aDeviceIdentifier), getter_AddRefs(deviceQueue))) {
    return deviceQueue->Clear();
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult 
sbDeviceBase::IsTransferQueueEmpty(const nsAString &aDeviceIdentifier, 
                                   bool &aEmpty)
{
  aEmpty = PR_FALSE;

  nsCOMPtr<nsIMutableArray> queue;
  nsresult rv = GetTransferQueue(aDeviceIdentifier, getter_AddRefs(queue));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = queue->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!length) {
    aEmpty = PR_TRUE;
  }

  return NS_OK;
}

nsresult 
sbDeviceBase::GetDeviceState(const nsAString& aDeviceIdentifier, 
                             PRUint32* aDeviceState)
{
  NS_ENSURE_ARG_POINTER(aDeviceState);
  *aDeviceState = sbIDeviceBase::STATE_IDLE;

  if(mDeviceStates.Get(aDeviceIdentifier, aDeviceState)) {
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult
sbDeviceBase::SetDeviceState(const nsAString& aDeviceIdentifier,
                             PRUint32 aDeviceState)
{
  NS_ENSURE_ARG(aDeviceState >= sbIDeviceBase::STATE_IDLE &&
                aDeviceState <= sbIDeviceBase::STATE_DELETING);

  PRUint32 currentState;
  NS_ENSURE_TRUE(mDeviceStates.Get(aDeviceIdentifier, &currentState),
                 NS_ERROR_INVALID_ARG);
  if(mDeviceStates.Put(aDeviceIdentifier, aDeviceState)) {
    if (aDeviceState != currentState)
      DoStateChangedCallback(aDeviceIdentifier, aDeviceState);
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
sbDeviceBase::InitDeviceState(const nsAString& aDeviceIdentifier)
{
  if(mDeviceStates.Put(aDeviceIdentifier, sbIDeviceBase::STATE_IDLE)) {
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
sbDeviceBase::ClearDeviceState(const nsAString& aDeviceIdentifier)
{
  mDeviceStates.Remove(aDeviceIdentifier);

  return NS_OK;
}


nsresult 
sbDeviceBase::SetListenerForDeviceLibrary(const nsAString& aDeviceIdentifier,
                                          sbIMediaListListener *aMediaListListener)
{
  NS_ENSURE_ARG_POINTER(aMediaListListener);

  if(mDeviceLibraryListeners.Put(aDeviceIdentifier, aMediaListListener)) {
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult 
sbDeviceBase::GetListenerForDeviceLibrary(const nsAString& aDeviceIdentifier,
                                          sbIMediaListListener* *aMediaListListener)
{
  NS_ENSURE_ARG_POINTER(aMediaListListener);

  if(mDeviceLibraryListeners.Get(aDeviceIdentifier, aMediaListListener)) {
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult
sbDeviceBase::RemoveListenerForDeviceLibrary(const nsAString& aDeviceIdentifier)
{
  nsCOMPtr<sbILibrary> library;
  nsCOMPtr<sbIMediaListListener> listener;

  if(mDeviceLibraryListeners.Get(aDeviceIdentifier, getter_AddRefs(listener)) &&
     mDeviceLibraries.Get(aDeviceIdentifier, getter_AddRefs(library))) {
    
    nsresult rv;
    nsCOMPtr<sbIMediaList> list;
    
    list = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = list->RemoveListener(listener);
    NS_ENSURE_SUCCESS(rv, rv);

    mDeviceLibraryListeners.Remove(aDeviceIdentifier);

    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}
