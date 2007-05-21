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

/** 
* \file  DeviceBase.cpp
* \brief Songbird DeviceBase Component Implementation.
*/

#include "DeviceBase.h"
#include "sbIDeviceBase.h"

#include <nsAppDirectoryServiceDefs.h>
#include <nsAutoLock.h>
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
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>

#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbLocalDatabaseCID.h>

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

// Copied from nsCRT.h
#if defined(XP_WIN) || defined(XP_OS2)
  #define FILE_PATH_SEPARATOR       "\\"
  #define FILE_ILLEGAL_CHARACTERS   "/:*?\"<>|"
#elif defined(XP_UNIX) || defined(XP_BEOS)
  #define FILE_PATH_SEPARATOR       "/"
  #define FILE_ILLEGAL_CHARACTERS   ""
#else
  #error need_to_define_your_file_path_separator_and_illegal_characters
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
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar, index);
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
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar, index);
    if (oldCharsIndex > -1)
      aOldString.Replace(index, 1, aNewChar);
  }
}

//sbDeviceBaseLibraryListener class.
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceBaseLibraryListener, 
                              sbIMediaListListener);

sbDeviceBaseLibraryListener::sbDeviceBaseLibraryListener() 
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

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBaseLibraryListener::OnItemAdded(sbIMediaList *aMediaList, 
                                         sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> items;

  //XXXAus: Before adding to queue, make sure it doesn't come from
  //another device. Ask DeviceManager for the device library
  //containing this item.

  items = do_CreateInstance("@mozilla.org/array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  //XXXAus: Read this from a library pref???

  PRUint32 transferItemCount = 0;
  rv = mDevice->TransferItems(mDeviceIdentifier, 
                              items, 
                              uri,
                              sbIDeviceBase::OP_UPLOAD, 
                              PR_TRUE,
                              nsnull, 
                              &transferItemCount);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnBeforeItemRemoved(sbIMediaList *aMediaList, 
                                                 sbIMediaItem *aMediaItem)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnAfterItemRemoved(sbIMediaList *aMediaList, 
                                                sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> items;

  items = do_CreateInstance("@mozilla.org/array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 deleteItemCount;
  rv = mDevice->DeleteItems(mDeviceIdentifier, items, &deleteItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnItemUpdated(sbIMediaList *aMediaList, 
                                           sbIMediaItem *aMediaItem)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> items;

  items = do_CreateInstance("@mozilla.org/array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = items->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 updateItemCount;
  rv = mDevice->UpdateItems(mDeviceIdentifier, items, &updateItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceBaseLibraryListener::OnListCleared(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  PRUint32 deletedItemCount = 0;
  nsresult rv = mDevice->DeleteAllItems(mDeviceIdentifier, &deletedItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

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

  items = do_CreateInstance("@mozilla.org/array;1", &rv);
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

//sbDeviceBase class.
sbDeviceBase::sbDeviceBase()
{
}

sbDeviceBase::~sbDeviceBase()
{
}

nsresult
sbDeviceBase::Init()
{
  if(!mDeviceLibraries.Init() ||
     !mDeviceQueues.Init() ||
     !mDeviceCallbacks.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return NS_OK;
}

nsresult
sbDeviceBase::AddCallback(sbIDeviceBaseCallback* aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  if(mDeviceCallbacks.Put(aCallback, aCallback)) {
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
EnumDeviceCallback(nsISupports *key, sbIDeviceBaseCallback *data, void *closure)
{
  nsCOMArray<sbIDeviceBaseCallback> *array = NS_STATIC_CAST(nsCOMArray<sbIDeviceBaseCallback> *, closure);
  array->AppendObject(data);
  return PL_DHASH_NEXT;
}

void
sbDeviceBase::DoTransferStartCallback(const nsAString& aSourceURL,
                                      const nsAString& aDestinationURL)
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
        callback->OnTransferStart(aSourceURL, aDestinationURL);
      }
      catch(...) {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnTransferStart threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoTransferCompleteCallback(const nsAString& aSourceURL,
                                         const nsAString& aDestinationURL,
                                         PRInt32 aStatus)
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
        callback->OnTransferComplete(aSourceURL, aDestinationURL, aStatus);
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

nsresult 
sbDeviceBase::CreateDeviceLibrary(const nsAString &aDeviceIdentifier,
                                  nsIURI *aDeviceDatabaseURI,
                                  sbIDeviceBase *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsCOMPtr<sbILibraryFactory> libraryFactory = 
    do_CreateInstance(SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv);
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

    PRBool exists = PR_FALSE;
    rv = libraryFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if(!exists) {
      rv = libraryFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsAutoString filename(aDeviceIdentifier);
    filename.AppendLiteral(".db");

    rv = libraryFile->SetLeafName(filename);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = libraryProps->SetPropertyAsInterface(NS_LITERAL_STRING("databaseFile"), 
                                            libraryFile);
  NS_ENSURE_SUCCESS(rv, rv);

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

  rv = list->AddListener(listener);
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
    do_CreateInstance("@mozilla.org/array;1", &rv);
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

  nsCOMPtr<nsIMutableArray> deviceQueue;
  if(mDeviceQueues.Get(aDeviceIdentifier, getter_AddRefs(deviceQueue))) {
    PRUint32 index;
    nsresult rv = deviceQueue->IndexOf(0, aMediaItem, &index);
    NS_ENSURE_SUCCESS(rv, rv);

    return deviceQueue->RemoveElementAt(index);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
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
                                   PRBool &aEmpty)
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
