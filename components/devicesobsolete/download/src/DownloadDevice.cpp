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

/* *****************************************************************************
 *******************************************************************************
 *
 * Download device.
 *
 *******************************************************************************
 ******************************************************************************/

/** 
* \file  sbDownloadDevice.cpp
* \brief Songbird DownloadDevice Component Implementation.
*/

/* *****************************************************************************
 *
 * Download device imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include "DownloadDevice.h"

/* Mozilla imports. */
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsIDOMWindow.h>
#include <nsILocalFile.h>
#include <nsIPrefService.h>
#include <nsIProperties.h>
#include <nsIStandardURL.h>
#include <nsIFileURL.h>
#include <nsISupportsPrimitives.h>
#include <nsITreeView.h>
#include <nsISupportsPrimitives.h>
#include <nsIURL.h>
#include <nsIWindowWatcher.h>
#include <nsServiceManagerUtils.h>
#include <nsNetUtil.h>

/* Songbird imports. */
#include <sbILibraryManager.h>
#include <sbIMetadataJob.h>
#include <sbIPropertyManager.h>
#include <sbStandardProperties.h>


/* *****************************************************************************
 *
 * Download device configuration.
 *
 ******************************************************************************/

/*
 * SB_DOWNLOAD_DEVICE_CATEGORY  Download device category name.
 * SB_DOWNLOAD_DEVICE_ID        Download device identifier.
 * SB_PROPERTY_DESTINATION      Destination property ID.
 * SB_PROPERTY_DESTINATION_NAME Destination property display name.
 * SB_DOWNLOAD_DIR_DR           Default download directory data remote.
 * SB_TMP_DIR                   Songbird temporary directory name.
 * SB_DOWNLOAD_TMP_DIR          Download device temporary directory name.
 * SB_DOWNLOAD_LIB_NAME         Download device library name.
 * SB_STRING_BUNDLE_CHROME_URL  URL for Songbird string bundle.
 * SB_DOWNLOAD_COL_SPEC         Default download device playlist column spec.
 * SB_PREF_DOWNLOAD_LIBRARY     Download library preference name.
 * SB_PREF_WEB_LIBRARY          Web library GUID preference name.
 */

#define SB_DOWNLOAD_DEVICE_CATEGORY                                            \
                            NS_LITERAL_STRING("Songbird Download Device").get()
#define SB_DOWNLOAD_DEVICE_ID   "download"
#define SB_PROPERTY_DESTINATION "http://songbirdnest.com/data/1.0#destination"
#define SB_PROPERTY_DESTINATION_NAME "property.destination"
#define SB_DOWNLOAD_DIR_DR "download.folder"
#define SB_TMP_DIR "Songbird"
#define SB_DOWNLOAD_TMP_DIR "DownloadDevice"
#define SB_DOWNLOAD_LIB_NAME                                                   \
                "&chrome://songbird/locale/songbird.properties#device.download"
#define SB_STRING_BUNDLE_CHROME_URL                                            \
                                "chrome://songbird/locale/songbird.properties"
#define SB_DOWNLOAD_COL_SPEC SB_PROPERTY_TRACKNAME " 395 "                     \
                             SB_PROPERTY_ARTISTNAME " 222 "                    \
                             SB_PROPERTY_ALBUMNAME " 222 "                     \
                             SB_PROPERTY_PROGRESSVALUE
#define SB_PREF_DOWNLOAD_MEDIALIST "songbird.library.download"
#define SB_PREF_WEB_LIBRARY     "songbird.library.web"


/* *****************************************************************************
 *
 * Download device logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbDownloadDevice");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


/* *****************************************************************************
 *
 * Download device nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS2(sbDownloadDevice, sbIDeviceBase, sbIDownloadDevice)


/* *****************************************************************************
 *
 * Download device services.
 *
 ******************************************************************************/

/*
 * sbDownloadDevice
 *
 *   This method is the constructor for the download device class.
 */

sbDownloadDevice::sbDownloadDevice()
:
    sbDeviceBase(),
    mpDeviceMonitor(nsnull),
    mState(STATE_IDLE)
{
}


/*
 * ~sbDownloadDevice
 *
 *   This method is the destructor for the download device class.
 */

sbDownloadDevice::~sbDownloadDevice()
{
}


/* *****************************************************************************
 *
 * Download device sbIDeviceBase implementation.
 *
 ******************************************************************************/

/**
 * \brief Initialize the device category handler.
 */

NS_IMETHODIMP sbDownloadDevice::Initialize()
{
    nsCOMPtr<sbIPropertyManager>
                                pPropertyManager;
    nsCOMPtr<sbIURIPropertyInfo>
                                pURIPropertyInfo;
    nsCOMPtr<sbILibraryManager> pLibraryManager;
    nsCOMPtr<sbILibrary>        pMainLibrary;
    nsCOMPtr<nsIPrefBranch>     pPrefBranch;
    nsresult                    result = NS_OK;

    /* Initialize the base services. */
    result = Init();

    /* Create the download device lock. */
    if (NS_SUCCEEDED(result))
    {
        mpDeviceMonitor = nsAutoMonitor::NewMonitor
                                        ("sbDownloadDevice::mpDeviceMonitor");
        if (!mpDeviceMonitor)
            result = NS_ERROR_OUT_OF_MEMORY;
    }

    /* Get the IO service. */
    if (NS_SUCCEEDED(result))
    {
        mpIOService = do_GetService("@mozilla.org/network/io-service;1",
                                    &result);
    }

    /* Get the pref service. */
    if (NS_SUCCEEDED(result))
        pPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &result);

    /* Get the library manager. */
    if (NS_SUCCEEDED(result))
    {
        pLibraryManager =
            do_GetService("@songbirdnest.com/Songbird/library/Manager;1",
                          &result);
    }

    /* Get the string bundle. */
    if (NS_SUCCEEDED(result))
    {
        nsCOMPtr<nsIStringBundleService>
                                    pStringBundleService;

        /* Get the download device string bundle. */
        pStringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID,
                                             &result);
        if (NS_SUCCEEDED(result))
        {
            result = pStringBundleService->CreateBundle
                                            (SB_STRING_BUNDLE_CHROME_URL,
                                             getter_AddRefs(mpStringBundle));
        }
    }

    /* Get the main library. */
    if (NS_SUCCEEDED(result))
        result = pLibraryManager->GetMainLibrary(getter_AddRefs(pMainLibrary));

    /* Get the web library. */
    if (NS_SUCCEEDED(result))
    {
        nsCOMPtr<nsISupportsString> pSupportsString;
        nsAutoString                webLibraryGUID;

        /* Read the web library GUID from the preferences. */
        if (NS_SUCCEEDED(result))
        {
            result = pPrefBranch->GetComplexValue
                                  (SB_PREF_WEB_LIBRARY,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(pSupportsString));
        }
        if (NS_SUCCEEDED(result))
            result = pSupportsString->GetData(webLibraryGUID);

        /* Get the web library. */
        if (NS_SUCCEEDED(result))
        {
            result = pLibraryManager->GetLibrary(webLibraryGUID,
                                                 getter_AddRefs(mpWebLibrary));
        }

    }

    /* See if we need to make a new media list */
    /* for ourselves in the main library.      */
    if (NS_SUCCEEDED(result)) 
    {
        nsCOMPtr<nsISupportsString> pSupportsString;
        nsAutoString downloadMediaListGUID;

         /* Read the media list GUID from the preferences. */
        if (NS_SUCCEEDED(result))
        {
            result = pPrefBranch->GetComplexValue
                                  (SB_PREF_DOWNLOAD_MEDIALIST,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(pSupportsString));
        }

        if (NS_SUCCEEDED(result))
        {
            /* Get the GUID. */
            result = pSupportsString->GetData(downloadMediaListGUID);

            if (NS_SUCCEEDED(result))
            {
                nsCOMPtr<sbIMediaItem> pMediaItem;
                result = pMainLibrary->GetMediaItem(downloadMediaListGUID,
                                                    getter_AddRefs(pMediaItem));

                if (NS_SUCCEEDED(result))
                {
                    mpDownloadMediaList = do_QueryInterface(pMediaItem, &result);
                    if (NS_FAILED(result))
                        mpDownloadMediaList = nsnull;
                }
            }
        }
        result = NS_OK;
    }

    /* Make a new media list if we still don't have one. */
    if (NS_SUCCEEDED(result) && !mpDownloadMediaList)
    {
        result = pMainLibrary->CreateMediaList
                                        (NS_LITERAL_STRING("simple"),
                                         getter_AddRefs(mpDownloadMediaList));
    }

    /* Make a listener for the media list. */
    if (NS_SUCCEEDED(result))
    {
        NS_NAMED_LITERAL_STRING(deviceID, SB_DOWNLOAD_DEVICE_ID);

        nsRefPtr<sbDeviceBaseLibraryListener> listener;
        NS_NEWXPCOM(listener, sbDeviceBaseLibraryListener);

        if (!listener)
            result = NS_ERROR_OUT_OF_MEMORY;

        if (NS_SUCCEEDED(result))
        {
            result = listener->Init(deviceID, this);
        }
        

        if (NS_SUCCEEDED(result))
        {
            result = mpDownloadMediaList->AddListener(listener, PR_FALSE);
        }

        if (NS_SUCCEEDED(result))
        {
            result = SetListenerForDeviceLibrary(deviceID, listener);
        }
    }

    /* Set our media list guid into the prefs so other folks can find us. */
    if (NS_SUCCEEDED(result))
    {
        nsCOMPtr<nsISupportsString> pSupportsString;
        nsAutoString downloadMediaListGUID;

        pSupportsString =
            do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &result);

        if (NS_SUCCEEDED(result))
        {
            result = mpDownloadMediaList->GetGuid(downloadMediaListGUID);
        }

        if (NS_SUCCEEDED(result))
        {
            result = pSupportsString->SetData(downloadMediaListGUID);
        }

        if (NS_SUCCEEDED(result))
        {
            result = pPrefBranch->SetComplexValue(SB_PREF_DOWNLOAD_MEDIALIST,
                                                  NS_GET_IID(nsISupportsString),
                                                  pSupportsString);
        }
    }

    /* Get the device library listener. */
    if (NS_SUCCEEDED(result))
    {
        nsCOMPtr<sbIMediaListListener>  pMediaListListener;

        result = GetListenerForDeviceLibrary
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID),
                                     getter_AddRefs(pMediaListListener));
        if (NS_SUCCEEDED(result))
	{
            mpDeviceLibraryListener =
                                NS_STATIC_CAST(sbDeviceBaseLibraryListener *,
                                               pMediaListListener.get());
        }
    }

    /* Set the download device media list name.  */
    if (NS_SUCCEEDED(result))
    {
        result = mpDownloadMediaList->SetName
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_LIB_NAME));
    }

    /* Set the download device playlist default column spec. */
    if (NS_SUCCEEDED(result))
    {
        nsString                    downloadColSpec;

        downloadColSpec.AppendLiteral(SB_DOWNLOAD_COL_SPEC);
        result = mpDownloadMediaList->SetProperty
                              (NS_LITERAL_STRING(SB_PROPERTY_DEFAULTCOLUMNSPEC),
                              downloadColSpec);
    }

    /* Write the download device library. */
    if (NS_SUCCEEDED(result))
        result = mpDownloadMediaList->Write();

    /* Create the device transfer queue. */
    if (NS_SUCCEEDED(result))
        result = CreateTransferQueue(NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID));

    /* Create the download destination property. */
    if (NS_SUCCEEDED(result))
    {
        nsString                    displayName;

        pPropertyManager =
                do_GetService
                    ("@songbirdnest.com/Songbird/Properties/PropertyManager;1",
                     &result);
        if (NS_SUCCEEDED(result))
        {
            pURIPropertyInfo =
                        do_CreateInstance
                            ("@songbirdnest.com/Songbird/Properties/Info/URI;1",
                             &result);
        }
        if (NS_SUCCEEDED(result))
        {
            result = pURIPropertyInfo->SetName
                                (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION));
        }
        if (NS_SUCCEEDED(result))
        {
            result = pPropertyManager->GetStringFromName
                            (mpStringBundle,
                             NS_LITERAL_STRING(SB_PROPERTY_DESTINATION_NAME),
                             displayName);
        }
        if (NS_SUCCEEDED(result))
            result = pURIPropertyInfo->SetDisplayName(displayName);
        if (NS_SUCCEEDED(result))
            result = pPropertyManager->AddPropertyInfo(pURIPropertyInfo);
    }

    /* Get the default download directory data remote. */
    if (NS_SUCCEEDED(result))
    {
        mpDownloadDirDR = do_CreateInstance
                                    ("@songbirdnest.com/Songbird/DataRemote;1",
                                     &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = mpDownloadDirDR->Init(NS_LITERAL_STRING(SB_DOWNLOAD_DIR_DR),
                                       EmptyString());
    }

    /* Create temporary download directory. */
    if (NS_SUCCEEDED(result))
    {
        nsCOMPtr<nsIProperties>     pProperties;
        PRUint32                    permissions;
        PRBool                      exists;

        /* Get the system temporary directory. */
        pProperties = do_GetService("@mozilla.org/file/directory_service;1",
                                    &result);
        if (NS_SUCCEEDED(result))
        {
            result = pProperties->Get("TmpD",
                                      NS_GET_IID(nsIFile),
                                      getter_AddRefs(mpTmpDownloadDir));
        }
        if (NS_SUCCEEDED(result))
            result = mpTmpDownloadDir->GetPermissions(&permissions);

        /* Get the Songbird temporary directory and make sure it exists. */
        if (NS_SUCCEEDED(result))
            result = mpTmpDownloadDir->Append(NS_LITERAL_STRING(SB_TMP_DIR));
        if (NS_SUCCEEDED(result))
            result = mpTmpDownloadDir->Exists(&exists);
        if (NS_SUCCEEDED(result) && !exists)
        {
            result = mpTmpDownloadDir->Create(nsIFile::DIRECTORY_TYPE,
                                              permissions);
        }

        /* Get the download temporary directory and make sure it's empty. */
        if (NS_SUCCEEDED(result))
        {
            result = mpTmpDownloadDir->Append
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_TMP_DIR));
        }
        if (NS_SUCCEEDED(result))
            result = mpTmpDownloadDir->Exists(&exists);
        if (NS_SUCCEEDED(result) && exists)
            result = mpTmpDownloadDir->Remove(PR_TRUE);
        if (NS_SUCCEEDED(result))
        {
            result = mpTmpDownloadDir->Create(nsIFile::DIRECTORY_TYPE,
                                              permissions);
        }
    }

    /* Resume incomplete transfers. */
    if (NS_SUCCEEDED(result))
        ResumeTransfers();

    return (result);
}


/**
 * \brief Finalize usage of the device category handler.
 *
 * This effectively prepares the device category handler for
 * application shutdown.
 */

NS_IMETHODIMP sbDownloadDevice::Finalize()
{
    /* Lock the download device for finalization. */
    if (mpDeviceMonitor)
    {
        /* Lock the download device. */
        nsAutoMonitor mon(mpDeviceMonitor);

        /* Dispose of any outstanding download sessions. */
        if (mpDownloadSession)
        {
            mpDownloadSession->Shutdown();
            mpDownloadSession = nsnull;
        }

        /* Remove the device transfer queue. */
        RemoveTransferQueue(NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID));

        /* Remove our listener from the library */
        if (mpDownloadMediaList && mpDeviceLibraryListener)
        {
          mpDownloadMediaList->RemoveListener(mpDeviceLibraryListener);
        }
    }

    /* Dispose of the device lock. */
    if (mpDeviceMonitor)
        nsAutoMonitor::DestroyMonitor(mpDeviceMonitor);

    return (NS_OK);
}


/**
 * \brief Add a device category handler callback.
 *
 * Enables you to get callbacks when devices are connected and when
 * transfers are initiated.
 *
 * \param aCallback The device category handler callback you wish to add.
 */

NS_IMETHODIMP sbDownloadDevice::AddCallback(
    sbIDeviceBaseCallback       *aCallback)
{
    return (sbDeviceBase::AddCallback(aCallback));
}


/**
 * \brief Remove a device category handler callback.
 * 
 * \param aCallback The device category handler callback you wish to remove.
 */

NS_IMETHODIMP sbDownloadDevice::RemoveCallback(
    sbIDeviceBaseCallback       *aCallback)
{
    return (sbDeviceBase::RemoveCallback(aCallback));
}


/**
 * \brief Get the device library representing the content
 * available on the device.
 * 
 * \param aDeviceIdentifier The unique device identifier.
 * \return The library for the specified device.
 */

NS_IMETHODIMP sbDownloadDevice::GetLibrary(
    const nsAString             &aDeviceIdentifier,
    sbILibrary                  **aLibrary)
{
    return (GetLibraryForDevice(aDeviceIdentifier, aLibrary));
}


/**
 * \brief Get the device's current state.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The current state of the device.
 */

NS_IMETHODIMP sbDownloadDevice::GetDeviceState(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aState)
{
    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* Return results. */
    *aState = mState;

    return (NS_OK);
}


/**
 * \brief Get preferred transfer location for item
 *
 * Get a transfer location for the specified media item. This enables
 * the device to determine where best to put this media item based on 
 * it's own set of criteria.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItem The media item that is about to be transferred.
 * \return The transfer location for the item.
 */

NS_IMETHODIMP sbDownloadDevice::GetTransferLocation(
    const nsAString             &aDeviceIdentifier,
    sbIMediaItem                *aMediaItem,
    nsIURI                      **aTransferLocationURI)
{
    LOG(("1: GetTransferLocation\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Transfer items to a library or destination.
 *
 * If you call this method with a library only, the device
 * will attempt to read the library's preferred transfer 
 * destination and use it.
 *
 * If you call this method with a destination only, the device
 * will attempt to transfer the items to that destination only.
 *
 * If you call this method with a library and destination path, the
 * device will attempt to simply pass a destination if you do not wish to add
 * the item to a library after the transfer is complete.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItems An array of media items to transfer.
 * \param aDestinationPath The desired destination path, path may include full filename.
 * \param aDeviceOperation The desired operation (upload, download, move).
 * \param aBeginTransferNow Begin the transfer operation immediately?
 * \param aDestinationLibrary The desired destination library.
 * \return Number of items actually queued for transfer.
 */

NS_IMETHODIMP sbDownloadDevice::TransferItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    nsIURI                      *aDestinationPath,
    PRUint32                    aDeviceOperation,
    PRBool                      aBeginTransferNow,
    sbILibrary                  *aDestinationLibrary,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount;
    PRUint32                    i;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Do nothing unless operation is upload.  Uploading an item to the   */
    /* download device will download the item to the destination library. */
    if (aDeviceOperation != OP_UPLOAD)
        return (NS_ERROR_NOT_IMPLEMENTED);

    /* Clear completed items. */
    ClearCompletedItems();

    /* Add all the media items to the transfer queue. */
    result = aMediaItems->GetLength(&itemCount);
    for (i = 0; ((i < itemCount) && NS_SUCCEEDED(result)); i++)
    {
        /* Get the next media item. */
        pMediaItem = do_QueryElementAt(aMediaItems, i, &result1);
        if (NS_FAILED(result1))
            pMediaItem = nsnull;

        /* Set its transfer destination. */
        if (NS_SUCCEEDED(result1))
            result1 = SetTransferDestination(pMediaItem);

        /* Initialize the download progress property. */
        if (NS_SUCCEEDED(result1))
        {
            result1 = pMediaItem->SetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                                 NS_LITERAL_STRING("-1"));
        }
        if (NS_SUCCEEDED(result1))
        {
            nsString                    progressModeStr;

            progressModeStr.AppendInt(nsITreeView::PROGRESS_NONE);
            result1 = pMediaItem->SetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_PROGRESSMODE),
                                 progressModeStr);
        }

        /* Write the media item. */
        /* XXXeps won't need this after bug 3134 is fixed. */
        if (NS_SUCCEEDED(result1))
            result1 = pMediaItem->Write();

        /* Add it to the transfer queue. */
        if (NS_SUCCEEDED(result1))
        {
            nsAutoMonitor mon(mpDeviceMonitor);
            result1 = AddItemToTransferQueue
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID),
                                     pMediaItem);
        }

        /* Remove item on error. */
        if (NS_FAILED(result1) && pMediaItem)
        {
            mpDeviceLibraryListener->SetIgnoreListener(PR_TRUE);
            mpDownloadMediaList->Remove(pMediaItem);
            mpDeviceLibraryListener->SetIgnoreListener(PR_FALSE);
        }
    }

    /* Run the transfer queue. */
    if (NS_SUCCEEDED(result))
        result = RunTransferQueue();

    return (result);
}


/**
 * \brief Update items on the device.
 */

NS_IMETHODIMP sbDownloadDevice::UpdateItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aItemCount)
{
  NS_WARNING("UpdateItems not implemented");
  return NS_OK;
}


/**
 * \brief Delete items from the device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 * \param aMediaItems An array of sbIMediaItem objects.
 *
 * \return Number of items actually deleted from the device.
 */

NS_IMETHODIMP sbDownloadDevice::DeleteItems(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    arrayLength;
    PRUint32                    i;
    PRBool                      equals;
    PRBool                      cancelSession = PR_FALSE;
    PRUint32                    itemCount = 0;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aMediaItems);
    NS_ENSURE_ARG_POINTER(aItemCount);

    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* Remove the items. */
    result = aMediaItems->GetLength(&arrayLength);
    for (i = 0; (NS_SUCCEEDED(result) && (i < arrayLength)); i++)
    {
        /* Get the media item to remove. */
        pMediaItem = do_QueryElementAt(aMediaItems, i, &result);

        /* Remove the item from the transfer queue. */
        if (NS_SUCCEEDED(result))
        {
            result1 = RemoveItemFromTransferQueue(aDeviceIdentifier,
                                                  pMediaItem);
            if (NS_SUCCEEDED(result1))
                itemCount++;
        }

        /* Check if current session should be deleted. */
        if (NS_SUCCEEDED(result) && mpDownloadSession && !cancelSession)
        {
            result1 = pMediaItem->Equals(mpDownloadSession->mpMediaItem,
                                         &equals);
            if (NS_SUCCEEDED(result1) && equals)
                cancelSession = PR_TRUE;
        }
    }

    /* Cancel the session if needed.  This counts as another deleted */
    /* item since the item would not be in the transfer queue.       */
    if (cancelSession)
    {
        result1 = CancelSession();
        if (NS_SUCCEEDED(result1))
            itemCount++;
    }

    /* Return results. */
    *aItemCount = itemCount;

    return (result);
}


/**
 * \brief Delete all items from the device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of items actually deleted from the device.
 */

NS_IMETHODIMP sbDownloadDevice::DeleteAllItems(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aItemCount)
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRUint32                    itemCount = 0;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aItemCount);

    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* Remove all the items in the queue. */
    while (GetNextTransferItem(getter_AddRefs(pMediaItem)))
        itemCount++;

    /* Cancel active download session. */
    if (mpDownloadSession)
    {
        result1 = CancelSession();
        if (NS_SUCCEEDED(result1))
            itemCount++;
    }

    /* Return results. */
    *aItemCount = itemCount;

    return (result);
}


/**
 * \brief Begin transfer operations.
 * \return The media item that began transferring.
 */

NS_IMETHODIMP sbDownloadDevice::BeginTransfer(
    const nsAString             &aDeviceIdentifier,
    sbIMediaItem                **aMediaItem)
{
    LOG(("1: BeginTransfer\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Cancel a transfer by removing it from the queue.
 *
 * Cancel a transfer by removing it from the queue. If the item
 * has already been transfered this function has no effect.
 *
 * \param aDeviceIdentifier The device unique identifier
 * \param aMediaItems An array of media items.
 * \return The number of transfers actually cancelled.
 */

NS_IMETHODIMP sbDownloadDevice::CancelTransfer(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    *aMediaItems,
    PRUint32                    *aNumItems)
{
    LOG(("1: CancelTransfer\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Suspend all transfers.
 *
 * Suspend all active transfers.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of transfer items actually suspended.
 */

NS_IMETHODIMP sbDownloadDevice::SuspendTransfer(
    const nsAString             &aDeviceIdentifier,
    PRUint32                    *aNumItems)
{
    PRUint32                    numItems = 0;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aNumItems);

    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* If there's a download session, suspend it. */
    if (mpDownloadSession)
    {
        result = mpDownloadSession->Suspend();
        if (NS_SUCCEEDED(result))
        {
            mState = STATE_DOWNLOAD_PAUSED;
            numItems = 1;
        }
    }

    /* Return results. */
    *aNumItems = numItems;

    return (result);
}


/**
 * \brief Resume pending transfers.
 *
 * Resume pending transfers for a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return Number of transfer items actually resumed.
 */

NS_IMETHODIMP sbDownloadDevice::ResumeTransfer(
   const nsAString              &aDeviceIdentifier,
   PRUint32                     *aNumItems)
{
    PRUint32                    numItems = 0;
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(aNumItems);

    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* If there's a download session, resume it. */
    if (mpDownloadSession)
    {
        result = mpDownloadSession->Resume();
        if (NS_SUCCEEDED(result))
        {
            mState = STATE_DOWNLOADING;
            numItems = 1;
        }
    }

    /* Return results. */
    *aNumItems = numItems;

    return (result);
}


/**
 * \brief Get the amount of used space from a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The amount of used space on the device, in bytes.
 */

NS_IMETHODIMP sbDownloadDevice::GetUsedSpace(
    const nsAString             &aDeviceIdentifier,
    PRInt64                     *aUsedSpace)
{
    LOG(("1: GetUsedSpace\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Get the amount of available space from a device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return The amount of available space, in bytes.
 */

NS_IMETHODIMP sbDownloadDevice::GetAvailableSpace(
    const nsAString             &aDeviceIdentifier,
    PRInt64                     *aAvailableSpace)
{
    LOG(("1: GetAvailableSpace\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Returns a list of file extensions representing the 
 * formats supported by a specific device.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return An array of nsISupportsString containing the supported
 * formats in file extension format.
 */

NS_IMETHODIMP sbDownloadDevice::GetSupportedFormats(
    const nsAString             &aDeviceIdentifier,
    nsIArray                    **aSupportedFormats)
{
    LOG(("1: GetSupportedFormats\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Download is to copy a track from the device to the host.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsDownloadSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aDownloadSupported)
{
    LOG(("1: IsDownloadSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is uploading supported on this device?
 *
 * Upload is to copy a track from host to the device
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsUploadSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aUploadSupported)
{
    LOG(("1: IsUploadSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is it possible to delete items from the device?
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsDeleteSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aDeleteSupported)
{
    LOG(("1: IsDeleteSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is it possible to update items directly on the device?
 *
 * This method could be used for updating tracks on a device
 * or applying CDDB match for a CD.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsUpdateSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aUpdateSupported)
{
    LOG(("1: IsUpdateSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Is eject or unmount supported by the device?.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::IsEjectSupported(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aEjectSupported)
{
    LOG(("1: IsEjectSupported\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
 * \brief Eject or unmount the device from the system.
 *
 * \param aDeviceIdentifier The device unique identifier.
 *
 * \return True or false.
 */

NS_IMETHODIMP sbDownloadDevice::EjectDevice(
    const nsAString             &aDeviceIdentifier,
    PRBool                      *aEjected)
{
    LOG(("1: EjectDevice\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/*
 * sbIDeviceBase attribute getters/setters.
 */

NS_IMETHODIMP sbDownloadDevice::GetName(
    nsAString                   &aName)
{
    LOG(("1: GetName\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::SetName(
    const nsAString             &aName)
{
    LOG(("1: SetName\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceCategory(
    nsAString                   &aDeviceCategory)
{
    aDeviceCategory.Assign(SB_DOWNLOAD_DEVICE_CATEGORY);
    return (NS_OK);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceIdentifiers(
    nsIArray                    **aDeviceIdentifiers)
{
    LOG(("1: GetDeviceIdentifiers\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbDownloadDevice::GetDeviceCount(
    PRUint32                    *aDeviceCount)
{
    LOG(("1: GetDeviceCount\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/* *****************************************************************************
 *
 * Download device transfer services.
 *
 ******************************************************************************/

/*
 * RunTransferQueue
 *
 *   This function runs the transfer queue.  It will start transferring the next
 * item in the transfer queue.
 */

nsresult sbDownloadDevice::RunTransferQueue()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    PRBool                      initiated = PR_FALSE;
    nsresult                    result = NS_OK;

    /* Lock the device. */
    nsAutoMonitor mon(mpDeviceMonitor);

    /* Initiate transfers until queue is busy or empty. */
    while (   !mpDownloadSession
           && GetNextTransferItem(getter_AddRefs(pMediaItem)))
    {
        /* Initiate item transfer. */
        mpDownloadSession = new sbDownloadSession(this, pMediaItem);
        if (!mpDownloadSession)
            result = NS_ERROR_OUT_OF_MEMORY;
        if (NS_SUCCEEDED(result))
            result = mpDownloadSession->Initiate();
        if (NS_SUCCEEDED(result))
            initiated = PR_TRUE;
        else
            initiated = PR_FALSE;

        /* Set state to downloading. */
        if (initiated)
            mState = STATE_DOWNLOADING;

        /* Release the download session if not initiated. */
        if (!initiated && mpDownloadSession)
            mpDownloadSession = nsnull;
    }

    return (result);
}


/*
 * GetNextTransferItem
 *
 *   <-- appMediaItem           Next media item to transfer.
 *
 *   <-- True                   The next media item to transfer was returned.
 *       False                  The transfer queue is busy or no media is
 *                              available.
 *
 *   This function returns in appMediaItem the next media item to transfer.  If
 * a media item is available for transfer and the transfer queue is not busy,
 * this function returns true; otherwise, it returns false.
 */

PRBool sbDownloadDevice::GetNextTransferItem(
    sbIMediaItem                **appMediaItem)
{
    nsCOMPtr<sbIMediaItem>          pMediaItem;
    nsresult                        result = NS_OK;

    /* Get the next media item to transfer. */
    result = GetNextItemFromTransferQueue
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID),
                                     getter_AddRefs(pMediaItem));
    if (NS_SUCCEEDED(result))
    {
        result = RemoveItemFromTransferQueue
                                    (NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID),
                                     pMediaItem);
    }

    /* Return results. */
    if (NS_SUCCEEDED(result))
    {
        NS_ADDREF(*appMediaItem = pMediaItem);
        return (PR_TRUE);
    }
    else
    {
        return (PR_FALSE);
    }
}


/*
 * ResumeTransfers
 *
 *   This function resumes all uncompleted transfers in the download device
 * library.
 */

nsresult sbDownloadDevice::ResumeTransfers()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    nsString                    progressStr;
    PRInt32                     progress;
    PRUint32                    itemCount;
    PRUint32                    queuedCount = 0;
    PRUint32                    i;
    nsresult                    itemResult;
    nsresult                    result = NS_OK;

    result = mpDownloadMediaList->GetLength(&itemCount);

    /* Resume each incomplete item in list. */
    for (i = 0; (NS_SUCCEEDED(result) && (i < itemCount)); i++)
    {
        /* Get the next item. */
        itemResult = mpDownloadMediaList->GetItemByIndex
                                          (i, getter_AddRefs(pMediaItem));

        /* Get the download progress. */
        if (NS_SUCCEEDED(itemResult))
        {
            itemResult = pMediaItem->GetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                                 progressStr);
        }
        if (NS_SUCCEEDED(itemResult))
            progress = progressStr.ToInteger(&itemResult);

        /* Add item to transfer queue if not complete. */
        if (NS_SUCCEEDED(itemResult) && (progress < 101))
        {
            nsAutoMonitor mon(mpDeviceMonitor);
            itemResult = AddItemToTransferQueue
                            (NS_LITERAL_STRING(SB_DOWNLOAD_DEVICE_ID),
                             pMediaItem);
            if (NS_SUCCEEDED(itemResult))
                queuedCount++;
        }
    }

    /* Run the transfer queue if any items were queued. */
    if (queuedCount > 0)
        RunTransferQueue();

    return (result);
}


/*
 * ClearCompletedItems
 *
 *   This function clears completed media items from the download device
 * library.
 */

nsresult sbDownloadDevice::ClearCompletedItems()
{
    nsCOMPtr<sbIMediaItem>      pMediaItem;
    nsString                    progressStr;
    PRInt32                     progress;
    PRUint32                    itemCount;
    PRInt32                     i;
    nsresult                    itemResult;
    nsresult                    result = NS_OK;

    result = mpDownloadMediaList->GetLength(&itemCount);

    /* Remove each complete item in list. */
    if (NS_SUCCEEDED(result))
    {
        for (i = itemCount - 1; i >= 0; i--)
        {
            /* Get the next item. */
            itemResult = mpDownloadMediaList->GetItemByIndex(i,
                                                             getter_AddRefs(pMediaItem));

            /* Get the download progress. */
            if (NS_SUCCEEDED(itemResult))
            {
                itemResult = pMediaItem->GetProperty
                                (NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                                 progressStr);
            }
            if (NS_SUCCEEDED(itemResult))
                progress = progressStr.ToInteger(&itemResult);

            /* Remove item from download device library if complete. */
            if (NS_SUCCEEDED(itemResult) && (progress == 101))
                itemResult = mpDownloadMediaList->Remove(pMediaItem);

            /* Warn if item could not be removed. */
            if (NS_FAILED(itemResult))
                NS_WARNING("Failed to remove completed download item.\n");
        }
    }

    return (result);
}


/*
 * SetTransferDestination
 *
 *   --> pMediaItem             Media item for which to set transfer
 *                              destination.
 *
 *   This function sets the transfer destination property for the media item
 * specified by pMediaItem.  If the transfer destination is already set, this
 * function does nothing.
 */

nsresult sbDownloadDevice::SetTransferDestination(
    nsCOMPtr<sbIMediaItem>      pMediaItem)
{
    nsString                    dstProp;
    nsString                    dstDir;
    nsCOMPtr<nsILocalFile>      pDstFile;
    nsCOMPtr<nsIURI>            pDstURI;
    nsCString                   dstSpec;
    nsresult                    propertyResult;
    PRBool                      cancelDownload;
    nsresult                    result = NS_OK;

    /* Do nothing if destination is already set. */
    propertyResult = pMediaItem->GetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     dstProp);
    if (NS_SUCCEEDED(propertyResult))
        return (result);

    /* Get the default destination directory. */
    if (NS_SUCCEEDED(result))
        result = mpDownloadDirDR->GetStringValue(dstDir);

    /* If not default destination directory is set, */
    /* query user.  Treat cancellation as an error. */
    if (NS_SUCCEEDED(result) && (dstDir.Length() == 0))
    {
        result = QueryUserForDestination(&cancelDownload, dstDir);
        if (NS_SUCCEEDED(result) && cancelDownload)
            result = NS_ERROR_UNEXPECTED;
    }

    /* Create a unique local destination file object.    */
    /* note that we only record the directory, we do not */
    /* append the filename until when the file begins    */
    /* to download                                       */
    if (NS_SUCCEEDED(result))
        result = NS_NewLocalFile(dstDir, PR_FALSE, getter_AddRefs(pDstFile));
    /* Get the destination URI spec. */
    if (NS_SUCCEEDED(result))
        result = mpIOService->NewFileURI(pDstFile, getter_AddRefs(pDstURI));
    if (NS_SUCCEEDED(result))
        result = pDstURI->GetSpec(dstSpec);

    /* Set the destination property. */
    if (NS_SUCCEEDED(result))
    {
        result = pMediaItem->SetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     NS_ConvertUTF8toUTF16(dstSpec));
    }

    return (result);
}


/*
 * CancelSession
 *
 *   This function cancels the active download session and runs the transfer
 * queue.
 */

nsresult sbDownloadDevice::CancelSession()
{
    nsresult                    result = NS_OK;

    /* Shutdown the session. */
    if (mpDownloadSession)
    {
        mpDownloadSession->Shutdown();
        mpDownloadSession = nsnull;
    }

    /* Run the transfer queue. */
    RunTransferQueue();

    return (result);
}


/*
 * SessionCompleted
 *
 *   --> pDownloadSession       Completed session.
 *
 *   This function is called when the download session specified by
 * pDownloadSession completes.
 */

void sbDownloadDevice::SessionCompleted(
    sbDownloadSession           *pDownloadSession)
{
    /* Complete session. */
    {
        /* Lock the device. */
        nsAutoMonitor mon(mpDeviceMonitor);

        /* Set state to idle. */
        mState = STATE_IDLE;

        /* Release the download session. */
        mpDownloadSession = nsnull;
    }

    /* Run the transfer queue. */
    RunTransferQueue();
}


/* *****************************************************************************
 *
 * Private download device services.
 *
 ******************************************************************************/

/*
 * GetTmpFile
 *
 *   <-- ppTmpFile              Temporary file.
 *
 *   This function returns in ppTmpFile an nsIFile object for a unique temporary
 * file.  This function does not create the file.
 */

nsresult sbDownloadDevice::GetTmpFile(
    nsIFile                     **ppTmpFile)
{
    nsCOMPtr<nsIFile>           pTmpFile;
    nsString                    tmpFileName;
    PRInt32                     fileNum;
    PRBool                      exists;
    nsresult                    result = NS_OK;

    /* Get a unique temporary download file. */
    fileNum = 1;
    do
    {
        /* Start at the temporary download directory. */
        result = mpTmpDownloadDir->Clone(getter_AddRefs(pTmpFile));

        /* Append the temporary file name. */
        if (NS_SUCCEEDED(result))
        {
            tmpFileName.AssignLiteral("tmp");
            tmpFileName.AppendInt(fileNum++);
            result = pTmpFile->Append(tmpFileName);
        }

        /* Check if the file exists. */
        if (NS_SUCCEEDED(result))
            result = pTmpFile->Exists(&exists);
    }
    while (exists && NS_SUCCEEDED(result));

    /* Return results. */
    if (NS_SUCCEEDED(result))
        NS_ADDREF(*ppTmpFile = pTmpFile);

    return (result);
}


/*
 * MakeFileUnique
 *
 *   --> apFile                 File to make unique.
 *
 *   This function makes the file object specified by apFile refer to a unique,
 * non-existent file.  If the specified file does not exist, this function does
 * nothing.  Otherwise, it tries different file leaf names until a non-existent
 * file is found and sets the specified file object's leaf name accordingly.
 */

nsresult sbDownloadDevice::MakeFileUnique(
    nsIFile                     *apFile)
{
    nsCOMPtr<nsIFile>           pUniqueFile;
    nsAutoString                leafName;
    nsAutoString                uniqueLeafName;
    PRInt32                     extOffset = -1;
    nsAutoString                uniqueStr;
    PRInt32                     uniqueNum = 1;
    PRBool                      exists;
    nsresult                    result = NS_OK;

    /* Do nothing if file does not exist. */
    result = apFile->Exists(&exists);
    if (NS_FAILED(result) || !exists)
        return (result);

    /* Clone the file object. */
    if (NS_SUCCEEDED(result))
        result = apFile->Clone(getter_AddRefs(pUniqueFile));

    /* Get the file leaf name. */
    if (NS_SUCCEEDED(result))
        result = pUniqueFile->GetLeafName(leafName);
    if (NS_SUCCEEDED(result))
        extOffset = leafName.RFindChar('.');

    /* Find a non-existent file name. */
    while (NS_SUCCEEDED(result) && exists)
    {
        /* Try producing a unique string. */
        uniqueStr.AssignLiteral("_");
        uniqueStr.AppendInt(uniqueNum++);
        uniqueStr.AppendLiteral("_");

        /* Add the unique string to the file leaf name. */
        uniqueLeafName.Assign(leafName);
        if (extOffset == -1)
            uniqueLeafName.Append(uniqueStr);
        else
            uniqueLeafName.Insert(uniqueStr, extOffset);

        /* Check if the file exists. */
        result = pUniqueFile->SetLeafName(uniqueLeafName);
        if (NS_SUCCEEDED(result))
            result = pUniqueFile->Exists(&exists);

        /* Limit number of tries. */
        if (exists && (uniqueNum > 1000))
            result = NS_ERROR_FILE_TOO_BIG;
    }

    /* Update the file with a unique leaf name. */
    if (NS_SUCCEEDED(result))
        result = apFile->SetLeafName(uniqueLeafName);

    return (result);
}


/*
 * QueryUserForDestination
 *
 *   <-- apCancelDownload       True if user cancelled the download.
 *   <-- aDstDir                Download destination directory.
 *
 *   This function queries the user for the default download destination
 * directory.  The selected directory is returned in aDstDir.  If the user
 * cancels the download, apCancelDownload is set to true; otherwise, it's set to
 * false.
 */

nsresult sbDownloadDevice::QueryUserForDestination(
    PRBool                      *apCancelDownload,
    nsAString                   &aDstDir)
{
    nsCOMPtr<nsIDialogParamBlock>
                                pDialogPB;
    nsString                    dstDir;
    PRInt32                     okPressed;
    nsresult                    result = NS_OK;

    /* Get a dialog parameter block. */
      pDialogPB = do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1",
                                    &result);

    /* Open the dialog. */
    if (NS_SUCCEEDED(result))
    {
        result = OpenDialog("chrome://songbird/content/xul/download.xul",
                            pDialogPB);
    }

    /* Check if the OK button was pressed. */
    if (NS_SUCCEEDED(result))
        result = pDialogPB->GetInt(0, &okPressed);

    /* Get the destination directory. */
    if (NS_SUCCEEDED(result) && okPressed)
        result = pDialogPB->GetString(0, getter_Copies(dstDir));

    /* Return results. */
    if (NS_SUCCEEDED(result))
    {
        if (okPressed)
	{
            *apCancelDownload = PR_FALSE;
            aDstDir = dstDir;
        }
        else
	{
            *apCancelDownload = PR_TRUE;
        }
    }

    return (result);
}


/*
 * OpenDialog
 *
 *   --> aChromeURL             URL to dialog chrome.
 *   --> apDialogPB             Dialog parameter block.
 *
 *   This function opens a dialog window with the chrome specified by aChromeURL
 * and the parameter block specified by apDialogPB.
 */

nsresult sbDownloadDevice::OpenDialog(
    char                        *aChromeURL,
    nsIDialogParamBlock         *apDialogPB)
{
    nsCOMPtr<nsIWindowWatcher>  pWindowWatcher;
    nsCOMPtr<nsIDOMWindow>      pActiveWindow;
    nsCOMPtr<nsIDOMWindow>      pWindow;
    nsCOMPtr<sbIDataRemote>     pDataRemote;
    nsCAutoString               chromeFeatures;
    PRBool                      accessibilityEnabled;
    nsresult                    result = NS_OK;

    /* Get the window watcher services. */
    pWindowWatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &result);

    /* Get the active window. */
    if (NS_SUCCEEDED(result))
        result = pWindowWatcher->GetActiveWindow(getter_AddRefs(pActiveWindow));

    /* Check if accessibility is enabled. */
    if (NS_SUCCEEDED(result))
    {
        pDataRemote = do_CreateInstance
                                    ("@songbirdnest.com/Songbird/DataRemote;1",
                                     &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = pDataRemote->Init(NS_LITERAL_STRING("accessibility.enabled"),
                                   EmptyString());
    }
    if (NS_SUCCEEDED(result))
        result = pDataRemote->GetBoolValue(&accessibilityEnabled);

    /* Get the chrome feature set. */
    if (NS_SUCCEEDED(result))
    {
        chromeFeatures = NS_LITERAL_CSTRING
                                ("chrome,centerscreen,modal=yes,resizable=no");
        if (accessibilityEnabled)
            chromeFeatures.AppendLiteral(",titlebar=yes");
        else
            chromeFeatures.AppendLiteral(",titlebar=no");
    }

    /* Open the dialog. */
    if (NS_SUCCEEDED(result))
    {
        pWindowWatcher->OpenWindow(pActiveWindow,
                                   aChromeURL,
                                   nsnull,
                                   chromeFeatures.get(),
                                   apDialogPB,
                                   getter_AddRefs(pWindow));
    }

    return (result);
}


/* *****************************************************************************
 *******************************************************************************
 *
 * Download session.
 *
 *******************************************************************************
 ******************************************************************************/

/* *****************************************************************************
 *
 * Download session imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsIChannel.h>


/* *****************************************************************************
 *
 * Public download session services.
 *
 ******************************************************************************/

/*
 * sbDownloadSession
 *
 *   This method is the constructor for the download session class.
 */

sbDownloadSession::sbDownloadSession(
    sbDownloadDevice            *pDownloadDevice,
    sbIMediaItem                *pMediaItem)
:
    mpMediaItem(pMediaItem),
    mpSessionLock(nsnull),
    mpDownloadDevice(pDownloadDevice),
    mCurrentProgress(-1),
    mShutdown(PR_FALSE),
    mSuspended(PR_FALSE)
{
}


/*
 * ~sbDownloadSession
 *
 *   This method is the destructor for the download session class.
 */

sbDownloadSession::~sbDownloadSession()
{
    /* Dispose of the session lock. */
    if (mpSessionLock)
        nsAutoLock::DestroyLock(mpSessionLock);
}


/*
 * Initiate
 *
 *   This function initiates the download session.
 */
nsresult sbDownloadSession::Initiate()
{
    nsCOMPtr<sbILibraryManager> pLibraryManager;
    nsCOMPtr<nsIWebBrowserPersist>
                                mpWebBrowser;
    nsCOMPtr<nsIURI>            pSrcURI;
    nsCOMPtr<nsIURI>            pDstURI;
    nsString                    dstSpec;
    nsCString                   dstCSpec;
    nsCOMPtr<nsILocalFile>      pDstFile;
    nsCString                   fileName;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIStandardURL>    pStandardURL;
    nsCOMPtr<nsIURL>            pURL;
    nsresult                    result = NS_OK;

    /* Get the IO and file protocol services. */
    mpIOService = do_GetService("@mozilla.org/network/io-service;1",
                                &result);
    if (NS_SUCCEEDED(result))
    {
        mpFileProtocolHandler = do_CreateInstance
                                ("@mozilla.org/network/protocol;1?name=file",
                                 &result);
    }

    /* Create the session lock. */
    if (NS_SUCCEEDED(result))
    {
        mpSessionLock = nsAutoLock::NewLock("sbDownloadSession::mpSessionLock");
        if (!mpSessionLock)
            result = NS_ERROR_OUT_OF_MEMORY;
    }

    /* Get a unique temporary download file. */
    if (NS_SUCCEEDED(result))
        result = mpDownloadDevice->GetTmpFile(getter_AddRefs(mpTmpFile));

    /* Get the destination download file and URI. */
    if (NS_SUCCEEDED(result))
    {
        result = mpMediaItem->GetProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                     dstSpec);
    }
    if (NS_SUCCEEDED(result))
    {
      /* Create a unique local destination file object. */
        nsCOMPtr<nsIURI>            pURI;
        nsCOMPtr<nsIFileURL>        pFileURL;
        nsCOMPtr<nsIFile>           pFile;
        result = NS_NewURI(getter_AddRefs(pURI), dstSpec);
        if (NS_SUCCEEDED(result)) 
          pFileURL = do_QueryInterface(pURI, &result);
        if (NS_SUCCEEDED(result)) 
          result = pFileURL->GetFile(getter_AddRefs(pFile));
        if (NS_SUCCEEDED(result)) 
          pDstFile = do_QueryInterface(pFile, &result);
    }
    if (NS_SUCCEEDED(result)) {
      // if the property contains a directory, make a complete filename
      // based on the original source url
      PRBool bFlag;
      pDstFile->IsDirectory(&bFlag);
      if (bFlag) {

        /* Get the source URI. */
        result = mpMediaItem->GetContentSrc(getter_AddRefs(pURI));

        /* Convert the URI to a URL. */
        if (NS_SUCCEEDED(result))
        {
            pStandardURL =
                        do_CreateInstance("@mozilla.org/network/standard-url;1",
                                          &result);
        }
        if (NS_SUCCEEDED(result))
        {
            result = pStandardURL->Init(nsIStandardURL::URLTYPE_STANDARD,
                                        0,
                                        NS_LITERAL_CSTRING(""),
                                        nsnull,
                                        pURI);
        }

        /* Get the file name from the URL. */
        if (NS_SUCCEEDED(result))
            pURL = do_QueryInterface(pStandardURL, &result);
        if (NS_SUCCEEDED(result))
            result = pURL->GetFileName(fileName);
        /* append to the path */
        if (NS_SUCCEEDED(result))
          result = pDstFile->Append(NS_ConvertUTF8toUTF16(fileName));
        /* ensure the filename is unique */
        if (NS_SUCCEEDED(result))
          result = sbDownloadDevice::MakeFileUnique(pDstFile);

        /* Get the destination URI spec. */
        if (NS_SUCCEEDED(result))
            result = mpIOService->NewFileURI(pDstFile, getter_AddRefs(pDstURI));
        if (NS_SUCCEEDED(result))
            result = pDstURI->GetSpec(dstCSpec);

        /* Set the destination property. */
        if (NS_SUCCEEDED(result))
        {
          result = mpMediaItem->SetProperty
                                        (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                        NS_ConvertUTF8toUTF16(dstCSpec));
        }
        if (NS_SUCCEEDED(result))
        {
            result = mpMediaItem->GetProperty
                                        (NS_LITERAL_STRING(SB_PROPERTY_DESTINATION),
                                        dstSpec);
        }

      }
    }

    if (NS_SUCCEEDED(result))
    {
        result = mpFileProtocolHandler->GetFileFromURLSpec
                                                (NS_ConvertUTF16toUTF8(dstSpec),
                                                 getter_AddRefs(mpDstFile));
    }

    if (NS_SUCCEEDED(result))
    {
        result = mpFileProtocolHandler->NewFileURI(mpDstFile,
                                                   getter_AddRefs(mpDstURI));
    }

    /* Get the destination library. */
    if (NS_SUCCEEDED(result))
    {
        pLibraryManager = do_GetService
                                ("@songbirdnest.com/Songbird/library/Manager;1",
                                 &result);
    }
    if (NS_SUCCEEDED(result))
        result = pLibraryManager->GetMainLibrary(getter_AddRefs(mpDstLibrary));

    /* Get a channel for the source. */
    if (NS_SUCCEEDED(result))
        result = mpMediaItem->GetContentSrc(getter_AddRefs(pSrcURI));
    if (NS_SUCCEEDED(result))
    {
        result = mpIOService->NewChannelFromURI(pSrcURI,
                                                getter_AddRefs(mpChannel));
    }

    /* Create a persistent download web browser. */
    if (NS_SUCCEEDED(result))
    {
        mpWebBrowser = do_CreateInstance
                            ("@mozilla.org/embedding/browser/nsWebBrowser;1",
                             &result);
    }

    /* Set up the listeners and callbacks. */
    if (NS_SUCCEEDED(result))
        result = mpWebBrowser->SetProgressListener(this);
    if (NS_SUCCEEDED(result))
        result = mpChannel->SetNotificationCallbacks(this);

    /* Initiate the download. */
    if (NS_SUCCEEDED(result))
        result = mpWebBrowser->SaveChannel(mpChannel, mpTmpFile);

    return (result);
}


/*
 * Suspend
 *
 *   This function suspends the download session.
 */

nsresult sbDownloadSession::Suspend()
{
    nsCOMPtr<nsIRequest>        pRequest;
    nsresult                    result = NS_OK;

    /* Lock the session. */
    nsAutoLock lock(mpSessionLock);

    /* Get the download channel request. */
    pRequest = do_QueryInterface(mpChannel, &result);

    /* Suspend the request. */
    if (NS_SUCCEEDED(result))
        result = pRequest->Suspend();
    if (NS_SUCCEEDED(result))
        mSuspended = PR_TRUE;

    return (result);
}


/*
 * Resume
 *
 *   This function resumes the download session.
 */

nsresult sbDownloadSession::Resume()
{
    nsCOMPtr<nsIRequest>        pRequest;
    nsresult                    result = NS_OK;

    /* Lock the session. */
    nsAutoLock lock(mpSessionLock);

    /* Get the download channel request. */
    pRequest = do_QueryInterface(mpChannel, &result);

    /* Resume the request. */
    if (NS_SUCCEEDED(result))
        result = pRequest->Resume();
    if (NS_SUCCEEDED(result))
        mSuspended = PR_FALSE;

    return (result);
}


/*
 * Shutdown
 *
 *   This function shuts down the download session.
 */

void sbDownloadSession::Shutdown()
{
    nsCOMPtr<nsIRequest>        pRequest;
    nsresult                    result = NS_OK;

    /* Lock the session. */
    nsAutoLock lock(mpSessionLock);

    /* Mark session for shutdown. */
    mShutdown = PR_TRUE;

    /* Get the download channel request. */
    pRequest = do_QueryInterface(mpChannel, &result);

    /* Cancel the request. */
    if (NS_SUCCEEDED(result))
        result = pRequest->Cancel(NS_ERROR_ABORT);
}


/* *****************************************************************************
 *
 * Download session nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS4(sbDownloadSession,
                              nsIWebProgressListener,
                              nsIInterfaceRequestor,
                              nsIProgressEventSink,
                              nsIHttpEventSink)


/* *****************************************************************************
 *
 * Download session nsIWebProgressListener services.
 *
 ******************************************************************************/

/**
 * Notification indicating the state has changed for one of the requests
 * associated with aWebProgress.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification
 * @param aRequest
 *        The nsIRequest that has changed state.
 * @param aStateFlags
 *        Flags indicating the new state.  This value is a combination of one
 *        of the State Transition Flags and one or more of the State Type
 *        Flags defined above.  Any undefined bits are reserved for future
 *        use.
 * @param aStatus
 *        Error status code associated with the state change.  This parameter
 *        should be ignored unless aStateFlags includes the STATE_STOP bit.
 *        The status code indicates success or failure of the request
 *        associated with the state change.  NOTE: aStatus may be a success
 *        code even for server generated errors, such as the HTTP 404 error.
 *        In such cases, the request itself should be queried for extended
 *        error information (e.g., for HTTP requests see nsIHttpChannel).
 */

NS_IMETHODIMP sbDownloadSession::OnStateChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRUint32                    aStateFlags,
    nsresult                    aStatus)
{
    nsString                    currentProgressStr;
    nsString                    progressModeStr;
    nsresult                    status = aStatus;
    nsresult                    result = NS_OK;

    /* Process state change with the session locked. */
    {
        /* Lock the session. */
        nsAutoLock lock(mpSessionLock);

        /* Do nothing if download has not stopped or if shutting down. */
        if (!(aStateFlags & STATE_STOP) || mShutdown)
            return (NS_OK);

        /* Do nothing on abort. */
        /* XXXeps This is a workaround for the fact that shutdown */
        /* isn't called until after channel is aborted.          */
        if (status == NS_ERROR_ABORT)
            return (NS_OK);

        /* Check HTTP response status. */
        if (NS_SUCCEEDED(status))
        {
            nsCOMPtr<nsIHttpChannel>    pHttpChannel;
            PRBool                      requestSucceeded;

            /* Try to get HTTP channel. */
            pHttpChannel = do_QueryInterface(mpChannel, &result);

            /* Check if request succeeded. */
            if (NS_SUCCEEDED(result))
                result = pHttpChannel->GetRequestSucceeded(&requestSucceeded);
            if (NS_SUCCEEDED(result) && !requestSucceeded)
                status = NS_ERROR_UNEXPECTED;

            /* Don't propagate errors from here. */
            result = NS_OK;
        }

        /* Complete the transfer on success. */
        if (NS_SUCCEEDED(result) && NS_SUCCEEDED(status))
            result = CompleteTransfer();

        /* Set the progress to complete. */
        mCurrentProgress = 101;
        currentProgressStr.AppendInt(mCurrentProgress);
        mpMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                                 currentProgressStr);
        progressModeStr.AppendInt(nsITreeView::PROGRESS_NONE);
        mpMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PROGRESSMODE),
                                 progressModeStr);

        /* XXXeps won't need Write after bug 3134 is fixed. */
        mpMediaItem->Write();
    }

    /* Send notification of session completion.  Do this */
    /* outside of session lock to prevent deadlock.      */
    mpDownloadDevice->SessionCompleted(this);

    return (NS_OK);
}


/**
 * Notification that the progress has changed for one of the requests
 * associated with aWebProgress.  Progress totals are reset to zero when all
 * requests in aWebProgress complete (corresponding to onStateChange being
 * called with aStateFlags including the STATE_STOP and STATE_IS_WINDOW
 * flags).
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new progress.
 * @param aCurSelfProgress
 *        The current progress for aRequest.
 * @param aMaxSelfProgress
 *        The maximum progress for aRequest.
 * @param aCurTotalProgress
 *        The current progress for all requests associated with aWebProgress.
 * @param aMaxTotalProgress
 *        The total progress for all requests associated with aWebProgress.
 *
 * NOTE: If any progress value is unknown, or if its value would exceed the
 * maximum value of type long, then its value is replaced with -1.
 *
 * NOTE: If the object also implements nsIWebProgressListener2 and the caller
 * knows about that interface, this function will not be called. Instead,
 * nsIWebProgressListener2::onProgressChange64 will be called.
 */

NS_IMETHODIMP sbDownloadSession::OnProgressChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRInt32                     aCurSelfProgress,
    PRInt32                     aMaxSelfProgress,
    PRInt32                     aCurTotalProgress,
    PRInt32                     aMaxTotalProgress)
{
    /* Update progress. */
    UpdateProgress(aCurSelfProgress, aMaxSelfProgress);

    return (NS_OK);
}


/**
 * Called when the location of the window being watched changes.  This is not
 * when a load is requested, but rather once it is verified that the load is
 * going to occur in the given window.  For instance, a load that starts in a
 * window might send progress and status messages for the new site, but it
 * will not send the onLocationChange until we are sure that we are loading
 * this new page here.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The associated nsIRequest.  This may be null in some cases.
 * @param aLocation
 *        The URI of the location that is being loaded.
 */

NS_IMETHODIMP sbDownloadSession::OnLocationChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    nsIURI                      *aLocation)
{
    return (NS_OK);
}


/**
 * Notification that the status of a request has changed.  The status message
 * is intended to be displayed to the user (e.g., in the status bar of the
 * browser).
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new status.
 * @param aStatus
 *        This value is not an error code.  Instead, it is a numeric value
 *        that indicates the current status of the request.  This interface
 *        does not define the set of possible status codes.  NOTE: Some
 *        status values are defined by nsITransport and nsISocketTransport.
 * @param aMessage
 *        Localized text corresponding to aStatus.
 */

NS_IMETHODIMP sbDownloadSession::OnStatusChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    nsresult                    aStatus,
    const PRUnichar             *aMessage)
{
    return (NS_OK);
}


/**
 * Notification called for security progress.  This method will be called on
 * security transitions (eg HTTP -> HTTPS, HTTPS -> HTTP, FOO -> HTTPS) and
 * after document load completion.  It might also be called if an error
 * occurs during network loading.
 *
 * @param aWebProgress
 *        The nsIWebProgress instance that fired the notification.
 * @param aRequest
 *        The nsIRequest that has new security state.
 * @param aState
 *        A value composed of the Security State Flags and the Security
 *        Strength Flags listed above.  Any undefined bits are reserved for
 *        future use.
 *
 * NOTE: These notifications will only occur if a security package is
 * installed.
 */

NS_IMETHODIMP sbDownloadSession::OnSecurityChange(
    nsIWebProgress              *aWebProgress,
    nsIRequest                  *aRequest,
    PRUint32                    aState)
{
    return (NS_OK);
}


/* *****************************************************************************
 *
 * Download session nsIInterfaceRequestor services.
 *
 ******************************************************************************/

/**
 * Retrieves the specified interface pointer.
 *
 * @param uuid The IID of the interface being requested.
 * @param aResult [out] The interface pointer to be filled in if
 *                the interface is accessible.
 * @return NS_OK - interface was successfully returned.
 *         NS_NOINTERFACE - interface not accessible.
 *         NS_ERROR* - method failure.
 */

NS_IMETHODIMP sbDownloadSession::GetInterface(
    const nsIID                 &uuid,
    void                        **aResult)
{
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(aResult);

    /* Query for the requested interface. */
    result = QueryInterface(uuid, aResult);

    return (result);
}


/* *****************************************************************************
 *
 * Download session nsIProgressEventSink services.
 *
 ******************************************************************************/

/**
 * Called to notify the event sink that progress has occurred for the
 * given request.
 *
 * @param aRequest
 *        the request being observed (may QI to nsIChannel).
 * @param aContext
 *        if aRequest is a channel, then this parameter is the listener
 *        context passed to nsIChannel::asyncOpen.
 * @param aProgress
 *        numeric value in the range 0 to aProgressMax indicating the
 *        number of bytes transfered thus far.
 * @param aProgressMax
 *        numeric value indicating maximum number of bytes that will be
 *        transfered (or 0xFFFFFFFFFFFFFFFF if total is unknown).
 */

NS_IMETHODIMP sbDownloadSession::OnProgress(
    nsIRequest                  *aRequest,
    nsISupports                 *aContext,
    PRUint64                    aProgress,
    PRUint64                    aProgressMax)
{
    /* Update progress. */
    UpdateProgress(aProgress, aProgressMax);

    return (NS_OK);
}


/**
 * Called to notify the event sink with a status message for the given
 * request.
 *
 * @param aRequest
 *        the request being observed (may QI to nsIChannel).
 * @param aContext
 *        if aRequest is a channel, then this parameter is the listener
 *        context passed to nsIChannel::asyncOpen.
 * @param aStatus
 *        status code (not necessarily an error code) indicating the
 *        state of the channel (usually the state of the underlying
 *        transport).  see nsISocketTransport for socket specific status
 *        codes.
 * @param aStatusArg
 *        status code argument to be used with the string bundle service
 *        to convert the status message into localized, human readable
 *        text.  the meaning of this parameter is specific to the value
 *        of the status code.  for socket status codes, this parameter
 *        indicates the host:port associated with the status code.
 */

NS_IMETHODIMP sbDownloadSession::OnStatus(
    nsIRequest                  *aRequest,
    nsISupports                 *aContext,
    nsresult                    aStatus,
    const PRUnichar             *aStatusArg)
{
    return (NS_OK);
}


/* *****************************************************************************
 *
 * Download session nsIHttpEventSink services.
 *
 ******************************************************************************/

/**
 * Called when a redirect occurs due to a HTTP response like 302.  The
 * redirection may be to a non-http channel.
 *
 * @return failure cancels redirect
 */

NS_IMETHODIMP sbDownloadSession::OnRedirect(
    nsIHttpChannel              *httpChannel,
    nsIChannel                  *newChannel)
{
    nsresult                    result = NS_OK;

    /* Lock the session. */
    nsAutoLock lock(mpSessionLock);

    /* Get the new channel. */
    mpChannel = newChannel;

    /* If session is suspended, suspend channel. */
    if (mSuspended)
        mpChannel->Suspend();

    return (result);
}


/* *****************************************************************************
 *
 * Private download session services.
 *
 ******************************************************************************/

/*
 * CompleteTransfer
 *
 *   This function completes transfer of the download file into the destination
 * library.
 */

nsresult sbDownloadSession::CompleteTransfer()
{
    nsCOMPtr<nsIFile>           pFileDir;
    nsString                    fileName;
    nsCOMPtr<sbIMediaList>      pDstMediaList;
    nsresult                    result = NS_OK;

    /* Move the temporary download file to the final location. */
    result = mpDstFile->GetLeafName(fileName);
    if (NS_SUCCEEDED(result))
        result = mpDstFile->GetParent(getter_AddRefs(pFileDir));
    if (NS_SUCCEEDED(result))
        result = mpTmpFile->MoveTo(pFileDir, fileName);

    /* Update the download media item content source property. */
    if (NS_SUCCEEDED(result))
        result = mpMediaItem->SetContentSrc(mpDstURI);

    /* Add the download media item to the destination library. */
    if (NS_SUCCEEDED(result))
        pDstMediaList = do_QueryInterface(mpDstLibrary, &result);
    if (NS_SUCCEEDED(result))
        result = pDstMediaList->Add(mpMediaItem);

    /* Write destination library. */
    if (NS_SUCCEEDED(result))
        result = mpDstLibrary->Write();

    /* Update the destination library metadata. */
    if (NS_SUCCEEDED(result))
        UpdateDstLibraryMetadata();

    /* Update the web library with the local downloaded file URL. */
    if (NS_SUCCEEDED(result))
    {
        nsRefPtr<WebLibraryUpdater> pWebLibraryUpdater;
        nsCOMPtr<sbIMediaList>      pWebMediaList;
        nsCOMPtr<nsIURI>            pSrcURI;
        nsCString                   srcSpec;

        /* Get the web library media list. */
        if (NS_SUCCEEDED(result))
        {
            pWebMediaList = do_QueryInterface(mpDownloadDevice->mpWebLibrary,
                                              &result);
        }

        /* Get the source URI spec. */
        if (NS_SUCCEEDED(result))
            result = mpMediaItem->GetContentSrc(getter_AddRefs(pSrcURI));
        if (NS_SUCCEEDED(result))
            result = pSrcURI->GetSpec(srcSpec);

        /* Update the web library. */
        if (NS_SUCCEEDED(result))
        {
            pWebLibraryUpdater = new WebLibraryUpdater(this);
            if (!pWebLibraryUpdater)
                result = NS_ERROR_OUT_OF_MEMORY;
        }
        if (NS_SUCCEEDED(result))
        {
            result = pWebMediaList->EnumerateItemsByProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     NS_ConvertUTF8toUTF16(srcSpec),
                                     pWebLibraryUpdater,
                                     sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        }

        /* Don't propagate errors from here. */
        result = NS_OK;
    }

    return (result);
}


/*
 * UpdateDstLibraryMetadata
 *
 *   This function updates the download media item metadata in the destination
 * library.  If the metadata is already set in the media item, this function
 * does nothing.
 */

nsresult sbDownloadSession::UpdateDstLibraryMetadata()
{
    nsCOMPtr<sbIMediaList>      pDstMediaList;
    nsRefPtr<LibraryMetadataUpdater>
                                pLibraryMetadataUpdater;
    nsCString                   dstSpec;
    nsString                    durationStr;
    PRInt32                     duration;
    PRBool                      updateDstLibraryMetadata = PR_TRUE;
    nsresult                    result1;
    nsresult                    result = NS_OK;

    /* Check if the download media item has metadata. */
    result1 = mpMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                                       durationStr);
    if (NS_SUCCEEDED(result1))
        duration = durationStr.ToInteger(&result1);
    if (NS_SUCCEEDED(result1) && (duration > 0))
        updateDstLibraryMetadata = PR_FALSE;

    /* Update the destination library metadata if needed. */
    if (updateDstLibraryMetadata)
    {
        /* Get the destination URI spec. */
        result = mpDstURI->GetSpec(dstSpec);

        /* Create a library metadata updater. */
        if (NS_SUCCEEDED(result))
        {
            pLibraryMetadataUpdater = new LibraryMetadataUpdater();
            if (!pLibraryMetadataUpdater)
                result = NS_ERROR_OUT_OF_MEMORY;
        }

        /* Update the download media item metadata in the library. */
        if (NS_SUCCEEDED(result))
            pDstMediaList = do_QueryInterface(mpDstLibrary, &result);
        if (NS_SUCCEEDED(result))
        {
            result = pDstMediaList->EnumerateItemsByProperty
                                    (NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     NS_ConvertUTF8toUTF16(dstSpec),
                                     pLibraryMetadataUpdater,
                                     sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        }
    }

    return (result);
}


/*
 * UpdateProgress
 *
 *   --> aProgress              Current progress value.
 *   --> aProgressMax           Maximum progress value.
 *
 *   This function updates the download session progress status with the
 * progress values specified by aProgress and aProgressMax.
 */

void sbDownloadSession::UpdateProgress(
    PRUint64                    aProgress,
    PRUint64                    aProgressMax)
{
    PRInt64                     progressPct;
    nsString                    currentProgressStr;
    nsString                    progressModeStr;

    /* Compute the download progress.  Save the   */
    /* last percent for download post-processing. */
    progressPct = (100 * aProgress) / aProgressMax;

    /* Update download progress if it has changed. */
    if (progressPct != mCurrentProgress)
    {
        /* Update the download progress. */
        mCurrentProgress = progressPct;
        currentProgressStr.AppendInt(mCurrentProgress);
        mpMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PROGRESSVALUE),
                                 currentProgressStr);
        progressModeStr.AppendInt(nsITreeView::PROGRESS_NORMAL);
        mpMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PROGRESSMODE),
                                 progressModeStr);
        mpMediaItem->Write();
    }
}


/* *****************************************************************************
 *
 * Download device LibraryMetadataUpdater services.
 *
 *   This class updates a library items' metadata by scanning a local,
 * downloaded file.
 *
 ******************************************************************************/

/* LibraryMetadataUpdater nsISupports implementation. */
NS_IMPL_ISUPPORTS1(sbDownloadSession::LibraryMetadataUpdater,
                   sbIMediaListEnumerationListener)


/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return true to begin enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumerationBegin(
    sbIMediaList                *aMediaList,
    PRBool                      *_retval)
{
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(_retval);

    /* Create an array to contain the media items to scan. */
    mpMediaItemArray = do_CreateInstance("@mozilla.org/array;1", &result);

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *_retval = PR_TRUE;
    else
        *_retval = PR_FALSE;

    return (result);
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return true to continue enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumeratedItem(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRBool                      *_retval)
{
    nsresult                    result = NS_OK;

    /* Validate arguments. */
    NS_ENSURE_ARG_POINTER(_retval);

    /* Put the media item into the scanning array. */
    result = mpMediaItemArray->AppendElement(aMediaItem, PR_FALSE);

    /* Return results. */
    *_retval = PR_TRUE;

    return (NS_OK);
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP sbDownloadSession::LibraryMetadataUpdater::OnEnumerationEnd(
    sbIMediaList                *aMediaList,
    nsresult                    aStatusCode)
{
    nsCOMPtr<sbIMetadataJobManager>
                                pMetadataJobManager;
    nsCOMPtr<sbIMetadataJob>    pMetadataJob;
    nsresult                    result = NS_OK;

    /* Start a metadata scanning job. */
    pMetadataJobManager = do_GetService
                            ("@songbirdnest.com/Songbird/MetadataJobManager;1",
                             &result);
    if (NS_SUCCEEDED(result))
    {
        result = pMetadataJobManager->NewJob(mpMediaItemArray,
                                             5,
                                             getter_AddRefs(pMetadataJob));
    }

    return (result);
}


/* *****************************************************************************
 *
 * Download session WebLibraryUpdater services.
 *
 *   This class updates web library items' content source properties with
 * downloaded local URLs.
 *
 ******************************************************************************/

/* WebLibraryUpdater nsISupports implementation. */
NS_IMPL_ISUPPORTS1(sbDownloadSession::WebLibraryUpdater,
                   sbIMediaListEnumerationListener)


/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return true to begin enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumerationBegin(
    sbIMediaList                *aMediaList,
    PRBool                      *_retval)
{
    nsresult                    result = NS_OK;

    /* Return results. */
    *_retval = PR_TRUE;

    return (result);
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return true to continue enumeration, false to cancel.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumeratedItem(
    sbIMediaList                *aMediaList,
    sbIMediaItem                *aMediaItem,
    PRBool                      *_retval)
{
    nsresult                    result = NS_OK;

    /* Update the content source. */
    aMediaItem->SetContentSrc(mpDownloadSession->mpDstURI);

    /* Return results. */
    *_retval = PR_TRUE;

    return (result);
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP sbDownloadSession::WebLibraryUpdater::OnEnumerationEnd(
    sbIMediaList                *aMediaList,
    nsresult                    aStatusCode)
{
    nsresult                    result = NS_OK;

    return (result);
}


