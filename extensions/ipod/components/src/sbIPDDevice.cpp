/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

/**
 * \file  sbIPDDevice.cpp
 * \brief Songbird iPod Device Source.
 */

//------------------------------------------------------------------------------
//
// iPod device imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbIPDDevice.h"

// Local imports.
#include "sbIPDFairPlayEvent.h"
#include "sbIPDLog.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbAutoRWLock.h>
#include <sbDeviceContent.h>
#include <sbDeviceUtils.h>
#include <sbIDeviceProperties.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediaListView.h>
#include <sbLibraryListenerHelpers.h>
#include <sbStandardProperties.h>
#include <sbStandardDeviceProperties.h>
#include <sbProxyUtils.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIPropertyBag2.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsIPromptService.h>
#include <prprf.h>
#include <nsThreadUtils.h>


//------------------------------------------------------------------------------
//
// iPod device configuration.
//
//------------------------------------------------------------------------------

//
// sbIPDSupportedMediaList      List of supported media file extensions derived
//                 from "http://docs.info.apple.com/article.html?artnum=304784".
// sbIPDSupportedMediaListLength Length of supported media file extension list.
// sbIPDSupportedAudioMediaList List of supported audio media file extensions
//                              derived from
//                      "http://docs.info.apple.com/article.html?artnum=304784".
// sbIPDSupportedAudioMediaListLength
//                              Length of supported audio media file extension
//                              list.
//

const char *sbIPDSupportedMediaList[] =
{
    "mp3",
    "m4a",
    "m4p",
    "jpg",
    "gif",
    "tif",
    "m4v",
    "mov",
    "mp4",
    "aif",
    "wav",
    "aa",
    "aac"
};
PRUint32 sbIPDSupportedMediaListLength =
           NS_ARRAY_LENGTH(sbIPDSupportedMediaList);
const char *sbIPDSupportedAudioMediaList[] =
{
    "mp3",
    "m4a",
    "m4p",
    "mp4",
    "aif",
    "wav",
    "aa",
    "aac"
};
PRUint32 sbIPDSupportedAudioMediaListLength =
           NS_ARRAY_LENGTH(sbIPDSupportedAudioMediaList);


//------------------------------------------------------------------------------
//
// iPod device nsISupports and nsIClassInfo implementation.
//
//------------------------------------------------------------------------------

// nsISupports implementation.
// NS_IMPL_THREADSAFE_ISUPPORTS2_CI(sbIPDDevice,
//                                  sbIDevice,
//                                  sbIDeviceEventTarget)
NS_IMPL_THREADSAFE_ADDREF(sbIPDDevice)
NS_IMPL_THREADSAFE_RELEASE(sbIPDDevice)
NS_IMPL_QUERY_INTERFACE2_CI(sbIPDDevice, sbIDevice, sbIDeviceEventTarget)
NS_IMPL_CI_INTERFACE_GETTER2(sbIPDDevice, sbIDevice, sbIDeviceEventTarget)

// nsIClassInfo implementation.
NS_DECL_CLASSINFO(sbIPDDevice)
NS_IMPL_THREADSAFE_CI(sbIPDDevice)


//------------------------------------------------------------------------------
//
// iPod device sbIDevice implementation.
//
//------------------------------------------------------------------------------

/**
 * Called when the device should initialize.
 */

NS_IMETHODIMP
sbIPDDevice::Connect()
{
  nsresult rv;

  // Connect.  Disconnect on error.
  rv = ConnectInternal();
  if (NS_FAILED(rv))
    Disconnect();

  return rv;
}

nsresult
sbIPDDevice::ConnectInternal()
{
  nsresult rv;

  // Do nothing if device is already connected.  If the device is not connected,
  // nothing should be accessing the "connect lock" fields.
  {
    sbAutoReadLock autoConnectLock(mConnectLock);
    if (mConnected)
      return NS_OK;
  }

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2> properties = do_QueryInterface(mCreationProperties,
                                                           &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device mount path.
  rv = properties->GetPropertyAsAString(NS_LITERAL_STRING("MountPath"),
                                        mMountPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Firewire GUID.
  rv = properties->GetPropertyAsAString(NS_LITERAL_STRING("FirewireGUID"),
                                        mFirewireGUID);
  if (NS_SUCCEEDED(rv)) {
    FIELD_LOG
      (("Connect %s\n", NS_LossyConvertUTF16toASCII(mFirewireGUID).get()));
  } else {
    FIELD_LOG(("Connect could not get iPod Firewire GUID.\n"));
  }

  // Connect the device capabilities.
  rv = CapabilitiesConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connet the iPod request services.
  rv = ReqConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connect the iPod database.
  rv = DBConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connect the iPod preference services.
  rv = PrefConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connect the Songbird device library.
  rv = LibraryConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connect the FairPlay services.
  rv = FPConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the device as connected.  After this, "connect lock" fields may be
  // used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_TRUE;
  }

  // Start the iPod request processing.
  rv = ReqProcessingStart();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mount the device.
  //XXXeps Do in DBConnect
  rv = PushRequest(TransferRequest::REQUEST_MOUNT, nsnull, mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the iPod for the first time if needed
  rv = SetUpIfNeeded();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Called when the device is to finalize.
 */

NS_IMETHODIMP
sbIPDDevice::Disconnect()
{
  // Stop the iPod request processing.
  ReqProcessingStop();

  // Mark the device as not connected.  After this, "connect lock" fields may
  // not be used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_FALSE;
  }

  // Disconnect the FairPlay services.
  FPDisconnect();

  // Disconnect the Songbird device library.
  LibraryDisconnect();

  // Disconnect the iPod preference services.
  PrefDisconnect();

  // Disconnect the iPod database.
  DBDisconnect();

  // Disconnect the iPod request services. */
  ReqDisconnect();

  return NS_OK;
}


/**
 * Get a preference stored on the device.
 */

NS_IMETHODIMP
sbIPDDevice::GetPreference(const nsAString& aPrefName,
                           nsIVariant**     _retval)
{
  // Forward call to base class.
  return sbBaseDevice::GetPreference(aPrefName, _retval);
}


/**
 * Sets a preference stored on the device.
 */

NS_IMETHODIMP
sbIPDDevice::SetPreference(const nsAString& aPrefName,
                           nsIVariant*      aPrefValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPrefValue);

  // Function variables.
  nsresult rv;

  // If a preference needs to be set on the device, enqueue a set preference
  // request.  The internal, locally stored preference is still set here to
  // provide immediate feedback.
  if (aPrefName.Equals(SB_SYNC_PARTNER_PREF)) {
    rv = ReqPushSetNamedValue(REQUEST_SET_PREF, aPrefName, aPrefValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Forward call to base class.
  return sbBaseDevice::SetPreference(aPrefName, aPrefValue);
}


/**
 *
 */

NS_IMETHODIMP
sbIPDDevice::SubmitRequest(PRUint32         aRequest,
                           nsIPropertyBag2* aRequestParameters)
{
  // Log progress.
  LOG(("Enter: sbIPDDevice::SubmitRequest\n"));

  nsRefPtr<TransferRequest> transferRequest;
  nsresult rv = CreateTransferRequest(aRequest, aRequestParameters,
      getter_AddRefs(transferRequest));
  NS_ENSURE_SUCCESS(rv, rv);

  return PushRequest(transferRequest);
}


/**
 * Cancel all current pending requests
 */

NS_IMETHODIMP
sbIPDDevice::CancelRequests()
{
  nsresult rv;

  // Clear requests.
  rv = ClearRequests();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Eject device.
 */

NS_IMETHODIMP
sbIPDDevice::Eject()
{
  nsresult rv;

  // Log progress.
  LOG(("Enter: sbIPDDevice::Eject\n"));

  // work out if we're playing or not...

  // get the playlist playback
  nsCOMPtr<sbIMediacoreManager> mm =
    do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = mm->GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreStatus> status;
  rv = mm->GetStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 state = 0;
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, rv);

  // Not playing, nothing to do.
  if(state == sbIMediacoreStatus::STATUS_STOPPED ||
     state == sbIMediacoreStatus::STATUS_UNKNOWN) {
    return NS_OK;
  }

  // get the index into the currently playing view
  PRUint32 currentIndex;
  rv = sequencer->GetViewPosition(&currentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the currently playing view
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = sequencer->GetView(getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the currently playing media item from the view
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mediaListView->GetItemByIndex(currentIndex, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the library that the media item lives in
  nsCOMPtr<sbILibrary> library;
  rv = mediaItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  // is that library our device library?
  PRBool equal;
  rv = mDeviceLibrary->Equals(library, &equal);
  NS_ENSURE_SUCCESS(rv, rv);

  if (equal) {
    // ask the user if we should eject
    PRBool eject;
    rv = PromptForEjectDuringPlayback(&eject);
    NS_ENSURE_SUCCESS(rv, rv);

    if (eject) {
      // give them what they asked for
      nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
      rv = mm->GetPlaybackControl(getter_AddRefs(playbackControl));
      NS_ENSURE_SUCCESS(rv, rv);

      if (playbackControl) {
        rv = playbackControl->Stop();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      return NS_OK;
    }

    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

/*
 * Call sync() on all libraries attached to this device
 */
NS_IMETHODIMP
sbIPDDevice::SyncLibraries()
{
  return sbBaseDevice::SyncLibraries();
}

/*
 * Determine if a media item can be transferred to the device
 */
NS_IMETHODIMP
sbIPDDevice::SupportsMediaItem(sbIMediaItem* aMediaItem,
                               PRBool        aReportErrors,
                               PRBool*       _retval)
{
  return sbBaseDevice::SupportsMediaItem(aMediaItem, aReportErrors, _retval);
}

//
// Getters/setters.
//

/**
 * A human-readable name identifying the device. Optional.
 */

NS_IMETHODIMP
sbIPDDevice::GetName(nsAString& aName)
{
  nsresult rv;
  // Log progress.
  LOG(("Enter: sbIPDDevice::GetName\n"));
  nsAutoString name;
  rv = mProperties->GetPropertyAsAString(NS_LITERAL_STRING("FriendlyName"),
                                         name);
  NS_ENSURE_SUCCESS(rv, rv);
  aName.Assign(name);

  return NS_OK;
}


/**
 * A human-readable name identifying the device product (e.g., vendor and
 * model number). Optional.
 */

NS_IMETHODIMP
sbIPDDevice::GetProductName(nsAString& aProductName)
{
  nsAutoString productName;
  PRBool       hasKey;
  nsresult     rv;

  // Get the vendor name.
  nsAutoString vendorName;
  rv = mProperties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                           &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    // Get the vendor name.
    rv = mProperties->GetPropertyAsAString
                        (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                         vendorName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get the device model number, using a default if one is not available.
  nsAutoString modelNumber;
  rv = mProperties->HasKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                           &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    // Get the model number.
    rv = mProperties->GetPropertyAsAString
                            (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                             modelNumber);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (modelNumber.IsEmpty()) {
    // Get the default model number.
    modelNumber = SBLocalizedString("device.msc.default.model.number");
  }

  // Produce the product name.
  if (!vendorName.IsEmpty()) {
    nsTArray<nsString> params;
    NS_ENSURE_TRUE(params.AppendElement(vendorName), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(params.AppendElement(modelNumber), NS_ERROR_OUT_OF_MEMORY);
    productName.Assign(SBLocalizedString("device.product.name", params));
  } else {
    productName.Assign(modelNumber);
  }

  // Return results.
  aProductName.Assign(productName);

  return NS_OK;
}


/**
 * The id of the controller that created the device.
 */

NS_IMETHODIMP
sbIPDDevice::GetControllerId(nsID** aControllerId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aControllerId);

  // Allocate an nsID.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);

  // Return the ID.
  *pId = mControllerID;
  *aControllerId = pId;

  return NS_OK;
}


/**
 * The id of the device
 */

NS_IMETHODIMP
sbIPDDevice::GetId(nsID** aId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aId);

  // Allocate an nsID.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);

  // Return the ID.
  *pId = mDeviceID;
  *aId = pId;

  return NS_OK;
}


/**
 * Whether or not the device is currently connected.
 */

NS_IMETHODIMP
sbIPDDevice::GetConnected(PRBool* aConnected)
{
  NS_ENSURE_ARG_POINTER(aConnected);
  *aConnected = mConnected;
  return NS_OK;
}


/**
 * Whether or not the device's events are being processed in additional threads
 * (i.e. off the main UI thread).
 */

NS_IMETHODIMP
sbIPDDevice::GetThreaded(PRBool* aThreaded)
{
  NS_ENSURE_ARG_POINTER(aThreaded);
  *aThreaded = PR_TRUE;
  return NS_OK;
}


/**
 * Get the capabilities of the device.
 */

NS_IMETHODIMP
sbIPDDevice::GetCapabilities(sbIDeviceCapabilities** aCapabilities)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ADDREF(*aCapabilities = mCapabilities);
  return NS_OK;
}


/**
 * All the device's content.
 */

NS_IMETHODIMP
sbIPDDevice::GetContent(sbIDeviceContent** aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);
  NS_ENSURE_STATE(mDeviceContent);
  NS_ADDREF(*aContent = mDeviceContent);
  return NS_OK;
}


/**
 * The parameters with which the device was created
 */

NS_IMETHODIMP
sbIPDDevice::GetParameters(nsIPropertyBag2** aParameters)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aParameters);

  // Function variables.
  nsresult rv;

  // Return results.
  nsCOMPtr<nsIPropertyBag2> parameters = do_QueryInterface(mCreationProperties,
                                                           &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ADDREF(*aParameters = parameters);

  return NS_OK;
}


/**
 * The device's properties.
 */

NS_IMETHODIMP
sbIPDDevice::GetProperties(sbIDeviceProperties** aProperties)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aProperties);

  // Return results.
  NS_ADDREF(*aProperties = mProperties);

  return NS_OK;
}


/**
 * Whether the device is busy, etc.
 * @see sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_*
 * @see sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_*
 * @see STATE_*
 */

NS_IMETHODIMP
sbIPDDevice::GetState(PRUint32* aState)
{
  LOG(("Enter: sbIPDDevice::GetState\n"));
  return sbBaseDevice::GetState(aState);
}

NS_IMETHODIMP
sbIPDDevice::GetPreviousState(PRUint32* aState)
{
  return sbBaseDevice::GetPreviousState(aState);
}


/* pass through to the base class */
NS_IMETHODIMP
sbIPDDevice::GetIsBusy(PRBool* aIsBusy)
{
  return sbBaseDevice::GetIsBusy(aIsBusy);
}

/* pass through to the base class */
NS_IMETHODIMP
sbIPDDevice::GetCanDisconnect(PRBool* aCanDisconnect)
{
  return sbBaseDevice::GetCanDisconnect(aCanDisconnect);
}

NS_IMETHODIMP sbIPDDevice::SetWarningDialogEnabled(const nsAString & aWarning, PRBool aEnabled)
{
  return sbBaseDevice::SetWarningDialogEnabled(aWarning, aEnabled);
}

NS_IMETHODIMP sbIPDDevice::GetWarningDialogEnabled(const nsAString & aWarning, PRBool *_retval)
{
  return sbBaseDevice::GetWarningDialogEnabled(aWarning, _retval);
}

NS_IMETHODIMP sbIPDDevice::ResetWarningDialogs()
{
  return sbBaseDevice::ResetWarningDialogs();
}


//------------------------------------------------------------------------------
//
// iPod device public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device object.
 */

sbIPDDevice::sbIPDDevice(const nsID&     aControllerID,
                         nsIPropertyBag* aProperties) :
  // Request services.
  mReqStopProcessing(0),
  mIsHandlingRequests(PR_FALSE),

  // Preference services.
  mPrefLock(nsnull),
  mPrefConnected(PR_FALSE),
  mIPodPrefs(NULL),
  mIPodPrefsDirty(PR_FALSE),
  mSyncPlaylistListDirty(PR_FALSE),

  // Internal services.
  mConnectLock(nsnull),
  mRequestLock(nsnull),
  mControllerID(aControllerID),
  mCreationProperties(aProperties),
  mITDB(NULL),
  mITDBDirty(PR_FALSE),
  mITDBDevice(NULL),
  mConnected(PR_FALSE),
  mIPDStatus(nsnull)
{
  // Log progress.
  LOG(("Enter: sbIPDDevice::sbIPDDevice\n"));

  // Validate arguments.
  NS_ASSERTION(aProperties, "aProperties is null");
}


/**
 * Destroy an iPod device controller object.
 */

sbIPDDevice::~sbIPDDevice()
{
  // Log progress.
  LOG(("Enter: sbIPDDevice::~sbIPDDevice\n"));
}


//------------------------------------------------------------------------------
//
// iPod device connection services.
//
//   These services are used to connect iPod devices.  Since they are a part of
// the device connection process, they do not need to operate under the connect
// lock.
//
//------------------------------------------------------------------------------

/**
 * Connect the iPod database data records.
 */

nsresult
sbIPDDevice::DBConnect()
{
  // Operate under the request lock.
  nsAutoMonitor autoRequestLock(mRequestLock);

  // Function variables.
  GError   *gError = nsnull;
  nsresult rv;

  // Get the utf8 form of the mount path.
  nsCString mountPath = NS_ConvertUTF16toUTF8(mMountPath);

  // Initialize the iPod database device data record. */
  mITDBDevice = itdb_device_new();
  NS_ENSURE_TRUE(mITDBDevice, NS_ERROR_OUT_OF_MEMORY);
  itdb_device_set_mountpoint(mITDBDevice, mountPath.get());

  // If the device file system is not supported, dispatch an event and return
  // with a failure.
  if (!IsFileSystemSupported()) {
    CreateAndDispatchEvent
      (sbIIPDDeviceEvent::EVENT_IPOD_UNSUPPORTED_FILE_SYSTEM,
       sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)).get());
    NS_ENSURE_SUCCESS(NS_ERROR_FAILURE, NS_ERROR_FAILURE);
  }

  // If we've got an HFSPlus filesystem that's read-only we special case
  // that
  PRBool hfsplusro;
  rv = mCreationProperties2->HasKey(NS_LITERAL_STRING("HFSPlusReadOnly"),
      &hfsplusro);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hfsplusro) {
    CreateAndDispatchEvent
      (sbIIPDDeviceEvent::EVENT_IPOD_HFSPLUS_READ_ONLY,
       sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)).get());
    NS_ENSURE_SUCCESS(NS_ERROR_FAILURE, NS_ERROR_FAILURE);
  }


  // If the iPod database file does not exist, initialize the iPod files.
  gchar* itdbPath = itdb_get_itunesdb_path(mountPath.get());
  sbAutoGMemPtr autoITDBPath(itdbPath);
  if (itdbPath == nsnull) {
    FIELD_LOG(("Missing iTunesDB, creating a new one.\n"));
    if (!itdb_init_ipod(mountPath.get(), nsnull, "iPod", &gError)) {
      if (gError) {
        if (gError->message) {
          FIELD_LOG((gError->message));
        }
        g_error_free(gError);
        gError = nsnull;
        NS_ENSURE_SUCCESS(NS_ERROR_UNEXPECTED, NS_ERROR_UNEXPECTED);
      }
    }
  }

  // Initialize the sysinfo.
  rv = InitSysInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the iPod database.
  mITDBDirty = PR_FALSE;
  mITDB = itdb_parse(mountPath.get(), &gError);
  if (gError) {
    if (gError->message) {
      FIELD_LOG((gError->message));
    }
    g_error_free(gError);
    gError = NULL;
  }
  NS_ENSURE_TRUE(mITDB, NS_ERROR_FAILURE);

  // Get the master playlist.
  mMasterPlaylist = itdb_playlist_mpl(mITDB);
  NS_ENSURE_TRUE(mMasterPlaylist, NS_ERROR_FAILURE);

  // Convert the playlist name from UTF8 to UTF16 as the device name.
  NS_ConvertUTF8toUTF16 friendlyName(mMasterPlaylist->name);

  // Set the device name.
  rv = mProperties->SetPropertyInternal(NS_LITERAL_STRING("FriendlyName"),
                                        sbIPDVariant(friendlyName).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Disconnect the iPod database data records.
 */

void
sbIPDDevice::DBDisconnect()
{
  // Operate under the request lock.
  nsAutoMonitor autoRequestLock(mRequestLock);

  // Dispose of the iPod database and device data records.
  if (mITDB)
    itdb_free(mITDB);
  mITDB = NULL;
  mITDBDirty = PR_FALSE;
  mMasterPlaylist = NULL;
  if (mITDBDevice)
    itdb_device_free(mITDBDevice);
  mITDBDevice = NULL;
}


/**
 * Connect the Songbird device library.
 */

nsresult
sbIPDDevice::LibraryConnect()
{
  nsresult rv;

  // Create the device library.
  nsAutoString libID;
  libID.AssignLiteral("iPod");
  libID.Append(mFirewireGUID);
  libID.AppendLiteral("@devices.library.songbirdnest.com");
  mDeviceLibrary = new sbIPDLibrary(this);
  NS_ENSURE_TRUE(mDeviceLibrary, NS_ERROR_OUT_OF_MEMORY);
  rv = InitializeDeviceLibrary(mDeviceLibrary, libID, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get an sbIMediaList interface for the device library.
  mDeviceLibraryML = do_QueryInterface
                       (NS_ISUPPORTS_CAST(sbIDeviceLibrary*, mDeviceLibrary),
                        &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device library.
  rv = AddLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Show the device library.
  //XXXeps should wait for after mount, but need to send an event to make it
  //work.
  rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                   NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Disconnect the Songbird device library.
 */

void
sbIPDDevice::LibraryDisconnect()
{
  // Remove the device library from the device content and dispose of it.
  if (mDeviceLibrary) {
    RemoveLibrary(mDeviceLibrary);
    FinalizeDeviceLibrary(mDeviceLibrary);
  }
  mDeviceLibrary = nsnull;
  mDeviceLibraryML = nsnull;
}


/**
 * Initialize the iPod device sysinfo.
 *
 * This function must be called under the request lock.
 */

nsresult
sbIPDDevice::InitSysInfo()
{
  GError*  gError = nsnull;
  PRBool   success;

  // Set the Firewire GUID value if not set.
  if (!mFirewireGUID.IsEmpty()) {
    gchar* fwGUID = itdb_device_get_sysinfo(mITDBDevice, "FirewireGuid");
    if (!fwGUID) {
      // Set the Firewire GUID sysinfo.
      itdb_device_set_sysinfo(mITDBDevice,
                              "FirewireGuid",
                              NS_ConvertUTF16toUTF8(mFirewireGUID).get());

      // Write the sysinfo file.
      success = itdb_device_write_sysinfo(mITDBDevice, &gError);
      if (!success) {
        if (gError) {
          if (gError->message)
            FIELD_LOG((gError->message));
          g_error_free(gError);
          gError = NULL;
        }
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_FAILURE);
      }
    } else {
      g_free(fwGUID);
    }
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device statistics services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * Initialize the device statistics services.
 *
 *   This function does not need to be called within the connect or request
 * locks.
 */

nsresult
sbIPDDevice::StatsInitialize()
{
  // Initialize some of the statistics fields.
  mStatsUpdatePeriod = PR_MillisecondsToInterval(IPOD_STATS_UPDATE_PERIOD);
  mLastStatsUpdate = PR_IntervalNow();

  return NS_OK;
}


/**
 * Finalize the device statistics services.
 *
 *   This function does not need to be called within the connect or request
 * locks.
 */

void
sbIPDDevice::StatsFinalize()
{
}


/**
 * Collect and update the iPod device statistics.  Limit the rate of updates
 * unless aForceUpdate is true.
 *
 * \param aForceUpdate          Force an update.
 */

void
sbIPDDevice::StatsUpdate(PRBool aForceUpdate)
{
  // Function variables.
  PRUint64 musicSpace = 0;
  PRUint64 musicTimeMS = 0;
  PRUint32 musicTrackCount = 0;
  guint64  capacity;
  guint64  free;
  guint64  totalUsed;
  PRBool   success;

  // Do nothing if update is not needed.
  PRIntervalTime lastUpdateInterval;
  if (!aForceUpdate) {
    lastUpdateInterval = PR_IntervalNow() - mLastStatsUpdate;
    if (lastUpdateInterval < mStatsUpdatePeriod)
      return;
  }

  // Update update time.
  mLastStatsUpdate = PR_IntervalNow();

  // Collect the track statistics.
  GList* trackList;
  trackList = mITDB->tracks;
  while (trackList) {
    // Get the track.
    Itdb_Track* track = (Itdb_Track *) trackList->data;
    trackList = trackList->next;

    // Update music statistics.
    if ((track->mediatype == 0) || (track->mediatype == 1)) {
      musicSpace += track->size;
      musicTimeMS += track->tracklen;
      musicTrackCount++;
    }
  }

  // Collect device storage statistics.
  success = itdb_device_get_storage_info(mITDB->device, &capacity, &free);
  NS_ENSURE_TRUE(success, /* void */);
  totalUsed = capacity - free;

  // Update the statistics properties.
  mProperties->SetPropertyInternal
    (NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#capacity"),
     sbIPDVariant(capacity).get());
  mProperties->SetPropertyInternal
    (NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#freeSpace"),
     sbIPDVariant(free).get());
  mProperties->SetPropertyInternal
    (NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#totalUsedSpace"),
     sbIPDVariant(totalUsed).get());
  mProperties->SetPropertyInternal
    (NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#musicUsedSpace"),
     sbIPDVariant(musicSpace).get());
}


//------------------------------------------------------------------------------
//
// iPod device event services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * Create and dispatch a FairPlay event of the type specified by aType and event
 * information specified by aUser, aAccountName, aUserName, and aMediaItem.  If
 * aAsync is true, dispatch asynchronously.
 *
 * \param aType                 Event type.
 * \param aUserID               FairPlay userID.
 * \param aAccountName          FairPlay account name.
 * \param aUserName             FairPlay user name.
 * \param aMediaItem            FairPlay media item.
 * \param aAsync                If true, dispatch asynchronously.
 */

nsresult
sbIPDDevice::CreateAndDispatchFairPlayEvent(PRUint32      aType,
                                            PRUint32      aUserID,
                                            nsAString&    aAccountName,
                                            nsAString&    aUserName,
                                            sbIMediaItem* aMediaItem,
                                            PRBool        aAsync)
{
  nsresult rv;

  // Create the event.
  nsRefPtr<sbIPDFairPlayEvent> fairPlayEvent;
  rv = sbIPDFairPlayEvent::CreateEvent(getter_AddRefs(fairPlayEvent),
                                       this,
                                       aType,
                                       aUserID,
                                       aAccountName,
                                       aUserName,
                                       aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch the event.
  nsCOMPtr<sbIDeviceEvent>
    event = do_QueryInterface
              (NS_ISUPPORTS_CAST(sbIIPDDeviceEvent*, fairPlayEvent), &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = DispatchEvent(event, aAsync, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal iPod device services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the iPod device object.
 */

nsresult
sbIPDDevice::Initialize()
{
  nsresult rv;

  // Initialize the base device object.
  rv = sbBaseDevice::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the connect lock.
  mConnectLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, "sbIPDDevice::mConnectLock");
  NS_ENSURE_TRUE(mConnectLock, NS_ERROR_OUT_OF_MEMORY);

  // Create an nsIFileProtocolHandler object.
  mFileProtocolHandler =
    do_CreateInstance("@mozilla.org/network/protocol;1?name=file", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the property manager service.
  mPropertyManager =
    do_GetService("@songbirdnest.com/Songbird/Properties/PropertyManager;1",
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the library manager service.
  mLibraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ensure the iPod device event handler is initialized.
  mIPodEventHandler =
    do_GetService("@songbirdnest.com/Songbird/IPDEventHandler;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the locale string bundle.
  nsCOMPtr<nsIStringBundleService>
    stringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stringBundleService->CreateBundle(IPOD_LOCALE_BUNDLE_PATH,
                                         getter_AddRefs(mLocale));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device creation properties as an nsIPropertyBag2.
  mCreationProperties2 = do_QueryInterface(mCreationProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Songbird main library.
  rv = mLibraryManager->GetMainLibrary(getter_AddRefs(mSBMainLib));
  NS_ENSURE_SUCCESS(rv, rv);
  mSBMainML = do_QueryInterface(mSBMainLib, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the device ID.  This depends upon some of the device properties.
  rv = CreateDeviceID(&mDeviceID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create and initialize the iPod device status object.
  mIPDStatus = new sbIPDStatus(this);
  NS_ENSURE_TRUE(mIPDStatus, NS_ERROR_OUT_OF_MEMORY);
  rv = mIPDStatus->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Create and initialize the device content object.
  mDeviceContent = sbDeviceContent::New();
  NS_ENSURE_TRUE(mDeviceContent, NS_ERROR_OUT_OF_MEMORY);
  rv = mDeviceContent->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the iPod mapping services.
  rv = MapInitialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the preference services.
  rv = PrefInitialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the device statistics services.
  rv = StatsInitialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the iPod request services.
  rv = ReqInitialize();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Finalize the iPod device object.
 */

void
sbIPDDevice::Finalize()
{
  // Finalize the iPod request services. */
  ReqFinalize();

  // Finalize the device statistics services.
  StatsFinalize();

  // Finalize the preference services.
  PrefFinalize();

  // Finalize the iPod mapping services.
  MapFinalize();

  // Finalize the device content.
  if (mDeviceContent)
    mDeviceContent->Finalize();
  mDeviceContent = nsnull;

  // Finalize the device properties.
  if (mProperties)
    mProperties->Finalize();
  mProperties = nsnull;

  // Dispose of the connect lock.
  if (mConnectLock)
    PR_DestroyRWLock(mConnectLock);
  mConnectLock = nsnull;

  // Finalize and destroy the iPod device status object.
  if (mIPDStatus) {
    mIPDStatus->Finalize();
    delete mIPDStatus;
  }
  mIPDStatus = nsnull;

  // Remove object references.
  mCreationProperties = nsnull;
  mFileProtocolHandler = nsnull;
  mPropertyManager = nsnull;
  mLibraryManager = nsnull;
  mSBMainLib = nsnull;
  mSBMainML = nsnull;
  mIPodEventHandler = nsnull;
}


/**
 * Init the device properties.
 */

nsresult
sbIPDDevice::InitializeProperties()
{
  // Create and initialize the device properties.
  nsresult rv;
  mProperties = new sbIPDProperties(this);
  NS_ENSURE_TRUE(mProperties, NS_ERROR_OUT_OF_MEMORY);
  rv = mProperties->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  return sbBaseDevice::InitializeProperties();
}


/**
 * Import the iPod database, including all tracks and playlists, into the
 * Songbird device library.
 * This function must be called under the request and connect locks.
 */

nsresult
sbIPDDevice::ImportDatabase()
{
  nsresult rv;

  // Mark all device library items as unavailable.
  rv = sbDeviceUtils::BulkSetProperty
                        (mDeviceLibrary,
                         NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                         NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Import all iPod database tracks.
  rv = ImportTracks();
  NS_ENSURE_SUCCESS(rv, rv);

  // Import all iPod database playlists.
  rv = ImportPlaylists();
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete all remaining unavailable items.
  sbDeviceUtils::DeleteUnavailableItems(mDeviceLibrary);

  return NS_OK;
}


/**
 * Flush all data to the device database.
 * This function must be called under the connect lock.
 */

nsresult
sbIPDDevice::DBFlush()
{
  GError   *gError = nsnull;

  // Operate under the request lock.
  nsAutoMonitor autoRequestLock(mRequestLock);

  // Do nothing unless database is dirty.
  if (!mITDBDirty)
    return NS_OK;

  // Write the iPod device database file.
  if (!itdb_write(mITDB, &gError)) {
    if (gError) {
      if (gError->message)
        FIELD_LOG((gError->message));
      g_error_free(gError);
      gError = NULL;
    }
    NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_FAILURE);
  }

  // Write the iPod device Shuffle database file.
  //XXXeps should only do for Shuffle.
  if (!itdb_shuffle_write(mITDB, &gError)) {
    if (gError) {
      if (gError->message)
        FIELD_LOG((gError->message));
      g_error_free(gError);
      gError = NULL;
    }
    NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_FAILURE);
  }

  // Database is no longer dirty.
  mITDBDirty = PR_FALSE;

  // Force updating of statistics.
  StatsUpdate(PR_TRUE);

  return NS_OK;
}


/**
 * Connect the device capabilities.
 */

nsresult
sbIPDDevice::CapabilitiesConnect()
{
  nsresult rv;

  // Create the device capabilities object.
  mCapabilities = do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID,
                                    &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mCapabilities->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the device function types.
  PRUint32 functionTypeList[] =
             { sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK };
  rv = mCapabilities->SetFunctionTypes(functionTypeList,
                                       NS_ARRAY_LENGTH(functionTypeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the device content types.
  PRUint32 contentTypeList[] = { sbIDeviceCapabilities::CONTENT_AUDIO,
                                 sbIDeviceCapabilities::CONTENT_PLAYLIST };
  rv = mCapabilities->AddContentTypes
                        (sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK,
                         contentTypeList,
                         NS_ARRAY_LENGTH(contentTypeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the device formats.
  rv = mCapabilities->AddFormats(sbIDeviceCapabilities::CONTENT_AUDIO,
                                 sbIPDSupportedAudioMediaList,
                                 sbIPDSupportedAudioMediaListLength);
  NS_ENSURE_SUCCESS(rv, rv);

  // Complete the device capabilities configuration.
  rv = mCapabilities->ConfigureDone();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aIPodID the iPod item ID for the media item specified by
 * aMediaItem.
 *
 * \param aMediaItem            Media item for which to get iPod item ID.
 * \param aIPodID               iPod item ID.
 */

nsresult
sbIPDDevice::GetIPodID(sbIMediaItem* aMediaItem,
                       guint64*      aIPodID)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aIPodID, "aIPodID is null");

  // Function variables.
  guint64     iPodID;
  nsresult    rv;

  // Get the iPod ID string.
  nsAutoString iPodIDStr;
  rv = aMediaItem->GetProperty
                     (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                      iPodIDStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // Scan the iPod ID from the string.
  int numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(iPodIDStr).get(),
                             "%llu",
                             &iPodID);
  NS_ENSURE_TRUE(numScanned, NS_ERROR_FAILURE);

  // Return results.
  *aIPodID = iPodID;

  return NS_OK;
}


/**
 * Create a unique and permanent device ID and return it in aDeviceID.
 *
 * \param aDeviceID             Device ID.
 */

nsresult
sbIPDDevice::CreateDeviceID(nsID* aDeviceID)
{
  // Validate arguments.
  NS_ASSERTION(aDeviceID, "aDeviceID is null");

  // Function variables.
  nsresult rv;

  // Get the device parameters.
  nsCOMPtr<nsIPropertyBag2> parameters;
  rv = GetParameters(getter_AddRefs(parameters));
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the device ID with zeros.
  PRUint8* buffer = (PRUint8*) aDeviceID;
  memset(buffer, 0, sizeof(nsID));

  // Fill the device ID with a hash of the device manufacturer, model number,
  // and serial number.
  PRInt32 offset = 0;
  rv = AddNSIDHash(buffer,
                   offset,
                   parameters,
                   NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER));
  NS_WARN_IF_FALSE
    (NS_SUCCEEDED(rv),
     "Couldn't add the device manufaturer to the device ID hash.");
  rv = AddNSIDHash(buffer, offset, parameters,
                   NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL));
  NS_WARN_IF_FALSE
    (NS_SUCCEEDED(rv),
      "Couldn't add the device model number to the device ID hash.");
  rv = AddNSIDHash(buffer, offset, parameters,
                   NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER));
  NS_WARN_IF_FALSE
    (NS_SUCCEEDED(rv),
     "Couldn't add the device serial number to the device ID hash.");

  return NS_OK;
}


/**
 * Add the property data specified by aPropBag and aProp to the nsID hash in the
 * buffer specified by aBuffer, starting at the offset specified by aOffset.
 *
 * \param aBuffer               nsID hash buffer.
 * \param aOffset               Starting hash offset.
 * \param aPropBag              Property bag.
 * \param aProp                 Property to add to hash.
 */

nsresult
sbIPDDevice::AddNSIDHash(unsigned char*   aBuffer,
                         PRInt32&         aOffset,
                         nsIPropertyBag2* aPropBag,
                         const nsAString& aProp)
{
  nsCString value;
  nsresult rv = aPropBag->GetPropertyAsACString(aProp, value);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsCString::char_type *p, *end;

  for (value.BeginReading(&p, &end); p < end; ++p) {
    PRUint32 data = (*p) & 0x7F; // only look at 7 bits
    data <<= (aOffset / 7 + 1) % 8;
    PRInt32 index = (aOffset + 6) / 8;
    if (index > 0)
      aBuffer[(index - 1) % sizeof(nsID)] ^= (data & 0xFF00) >> 8;
    aBuffer[index % sizeof(nsID)] ^= (data & 0xFF);
    aOffset += 7;
  }

  return NS_OK;
}


/**
 * Check if the device file system is supported.  If it is, return true;
 * otherwise, return false.
 *
 * \return                      True if device file system is supported.
 */

PRBool
sbIPDDevice::IsFileSystemSupported()
{
  // If the device storage information is available, the file system is
  // supported.
  guint64 capacity;
  guint64 free;
  return itdb_device_get_storage_info(mITDBDevice, &capacity, &free);
}


// Set up the iPod if it isn't already
nsresult
sbIPDDevice::SetUpIfNeeded()
{
  nsresult rv;

  // is the ipod already set up?
  PRBool isSetUp;
  rv = GetIsSetUp(&isSetUp);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isSetUp) {
    // if the device isn't set up yet, then send an event
    CreateAndDispatchEvent(sbIIPDDeviceEvent::EVENT_IPOD_NOT_INITIALIZED,
        sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)).get());
  }
  return NS_OK;
}



NS_IMETHODIMP
sbIPDDevice::GetCurrentStatus(sbIDeviceStatus * *aCurrentStatus)
{
  NS_ENSURE_ARG(aCurrentStatus);
  return mIPDStatus->GetCurrentStatus(aCurrentStatus);
}

NS_IMETHODIMP sbIPDDevice::Format()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbIPDDevice::GetSupportsReformat(PRBool *aCanReformat) {
  NS_ENSURE_ARG_POINTER(aCanReformat);
  *aCanReformat = PR_FALSE;
  return NS_OK;
}

/* attribute boolean cacheSyncRequests; */
NS_IMETHODIMP
sbIPDDevice::GetCacheSyncRequests(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  return sbBaseDevice::GetCacheSyncRequests(_retval);
}

/* attribute boolean cacheSyncRequests; */
NS_IMETHODIMP
sbIPDDevice::SetCacheSyncRequests(PRBool aCacheSyncRequests)
{
  return sbBaseDevice::SetCacheSyncRequests(aCacheSyncRequests);
}
