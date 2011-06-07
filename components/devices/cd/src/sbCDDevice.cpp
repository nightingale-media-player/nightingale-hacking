/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbCDDevice.h"

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsICryptoHash.h>
#include <nsIFileURL.h>
#include <nsIGenericFactory.h>
#include <nsIPropertyBag2.h>
#include <nsIWritablePropertyBag2.h>
#include <nsIProgrammingLanguage.h>
#include <nsIUUIDGenerator.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsILocalFile.h>
#include <nsISound.h>

#include <sbIDeviceEvent.h>
#include <sbILibraryManager.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIDeviceErrorMonitor.h>

#include <sbArrayUtils.h>
#include <sbAutoRWLock.h>
#include <sbDeviceContent.h>
#include <sbDeviceUtils.h>
#include <sbDeviceStatusHelper.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStandardDeviceProperties.h>
#include <sbVariantUtils.h>
#include <sbStringUtils.h>
#include <sbDebugUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbCDDevice:5
 */

NS_IMPL_THREADSAFE_ADDREF(sbCDDevice)
NS_IMPL_THREADSAFE_RELEASE(sbCDDevice)
NS_IMPL_QUERY_INTERFACE3_CI(sbCDDevice, sbIDevice,
                            sbIJobProgressListener,
                            sbIDeviceEventTarget)
NS_IMPL_CI_INTERFACE_GETTER3(sbCDDevice, sbIDevice,
                             sbIJobProgressListener,
                             sbIDeviceEventTarget)

// nsIClassInfo implementation.
NS_DECL_CLASSINFO(sbCDDevice)
NS_IMPL_THREADSAFE_CI(sbCDDevice)

#if _MSC_VER
// Disable warning about 'this' used in base member initializer list.
#pragma warning(disable: 4355)
#endif

sbCDDevice::sbCDDevice(const nsID & aControllerId,
                       nsIPropertyBag *aProperties)
  : mConnectLock(nsnull)
  , mControllerID(aControllerId)
  , mCreationProperties(aProperties)
  , mPrefAutoEject(PR_FALSE)
  , mPrefNotifySound(PR_FALSE)
{
  mPropertiesLock = nsAutoMonitor::NewMonitor("sbCDDevice::mPropertiesLock");
  NS_ENSURE_TRUE(mPropertiesLock, );

  SB_PRLOG_SETUP(sbCDDevice);

  InitRequestHandler();
}

sbCDDevice::~sbCDDevice()
{
  nsresult rv;
  nsCOMPtr<sbILibrary> lib;
  if (mDeviceLibrary) {
    // hold on to the underlying local database library
    rv = mDeviceLibrary->GetLibrary(getter_AddRefs(lib));
    if (NS_FAILED(rv)) {
      lib = nsnull;
    }
    rv = mDeviceLibrary->Finalize();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "sbCDDevice failed to finalize the device library");
    mDeviceLibrary = nsnull;
  }

  if (mPropertiesLock) {
    nsAutoMonitor::DestroyMonitor(mPropertiesLock);
    mPropertiesLock = nsnull;
  }

  if (lib && !mDeviceLibraryPath.IsEmpty()) {
    NS_ENSURE_SUCCESS(rv, /* void */);
    nsCOMPtr<sbILocalDatabaseLibrary> localLib = do_QueryInterface(lib, &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<nsIURI> uri;
    rv = localLib->GetDatabaseLocation(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri, &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<nsIFile> libraryFile;
    rv = fileURL->GetFile(getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, /* void */);

    rv = libraryFile->Append(mDeviceLibraryPath);
    NS_ENSURE_SUCCESS(rv, /* void */);

    rv = libraryFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  if (mConnectLock) {
    PR_DestroyRWLock(mConnectLock);
  }
}

nsresult
sbCDDevice::CreateDeviceID(nsID* aDeviceID)
{
  nsresult rv;

  // Initialize the device ID with zeros.
  PRUint32* buffer = reinterpret_cast<PRUint32*>(aDeviceID);
  memset(buffer, 0, sizeof(nsID));

  // Get the identifier from the CD device
  nsCString identifier;
  rv = mCDDevice->GetIdentifier(identifier);
  NS_ENSURE_SUCCESS(rv, rv);

  // Hash the identifier
  *buffer = HashString(identifier);

  return NS_OK;
}

nsresult
sbCDDevice::InitDevice()
{
  nsresult rv;

  // Log progress.
  LOG("Enter sbCDDevice::InitDevice");

  // Create the connect lock.
  NS_ENSURE_FALSE(mConnectLock, NS_ERROR_ALREADY_INITIALIZED);
  mConnectLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE,
                              "sbCDDevice::mConnectLock");
  NS_ENSURE_TRUE(mConnectLock, NS_ERROR_OUT_OF_MEMORY);

  // Create and initialize the device content object.
  mDeviceContent = sbDeviceContent::New();
  NS_ENSURE_TRUE(mDeviceContent, NS_ERROR_OUT_OF_MEMORY);
  rv = mDeviceContent->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the sbICDDevice reference
  nsCOMPtr<nsIVariant> deviceVar;
  rv = mCreationProperties->GetProperty(NS_LITERAL_STRING("sbICDDevice"),
                                        getter_AddRefs(deviceVar));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceVar->GetAsISupports(getter_AddRefs(mCDDevice));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the device ID.  This depends on mCDDevice being initialised first.
  rv = CreateDeviceID(&mDeviceID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the device state.
  SetState(sbIDevice::STATE_IDLE);

  // Log progress.
  LOG("Exit sbCDDevice::InitDevice");

  return NS_OK;
}

nsresult sbCDDevice::InitializeProperties()
{
  nsresult rv;

  mProperties =
    do_CreateInstance("@songbirdnest.com/Songbird/Device/DeviceProperties;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = mCreationProperties->GetProperty(NS_LITERAL_STRING("sbICDDevice"),
                                        getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = var->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDDevice> cdDevice = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString deviceName;
  rv = cdDevice->GetName(deviceName);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ERROR: Could not get the device name!");

  // Use the localized "CD Rip" string for the device friendly name. The CD
  // lookup jobs will update this value once lookup has been completed.
  rv = mProperties->InitFriendlyName(
      SBLocalizedString("cdrip.service.default_node_name"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProperties->InitDone();
  NS_ENSURE_SUCCESS(rv, rv);

  // CD devices are inherently read-only for now
  nsCOMPtr<nsIPropertyBag2> properties;
  rv = mProperties->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIWritablePropertyBag2> writeProperties =
          do_QueryInterface(properties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  writeProperties->SetPropertyAsAString(
        NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY),
        NS_LITERAL_STRING("ro"));
  return NS_OK;
}

/* static */ nsresult
sbCDDevice::New(const nsID & aControllerId,
                nsIPropertyBag *aProperties,
                sbCDDevice **aOutCDDevice)
{
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aOutCDDevice);

  nsRefPtr<sbCDDevice> device = new sbCDDevice(aControllerId, aProperties);
  NS_ENSURE_TRUE(device, NS_ERROR_OUT_OF_MEMORY);

  // Init the device.
  nsresult rv = device->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return the newly created CD device.
  device.forget(aOutCDDevice);

  return NS_OK;
}

// sbIDevice implementation

/**
 * A human-readable name identifying the device. Optional.
 */

NS_IMETHODIMP
sbCDDevice::GetName(nsAString& aName)
{
  // Operate under the properties lock.
  nsAutoMonitor autoPropertiesLock(mPropertiesLock);

  return GetNameBase(aName);
}


/**
 * A human-readable name identifying the device product (e.g., vendor and
 * model number). Optional.
 */

NS_IMETHODIMP
sbCDDevice::GetProductName(nsAString& aProductName)
{
  // Operate under the properties lock.
  nsAutoMonitor autoPropertiesLock(mPropertiesLock);

  return GetProductNameBase("device.cd.default.model.number",
                            aProductName);
}


/* readonly attribute nsIDPtr controllerId; */
NS_IMETHODIMP
sbCDDevice::GetControllerId(nsID * *aControllerId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aControllerId);

  // Allocate an nsID and set it to the controller ID.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);
  *pId = mControllerID;

  // Return the ID.
  *aControllerId = pId;

  return NS_OK;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP
sbCDDevice::GetId(nsID * *aId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aId);

  // Allocate an nsID and set it to the device ID.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);
  *pId = mDeviceID;

  // Return the ID.
  *aId = pId;

  return NS_OK;
}

nsresult
sbCDDevice::CapabilitiesReset()
{
  nsresult rv;

  // Create the device capabilities object.
  mCapabilities = do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID,
                                    &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mCapabilities->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 functionTypes = sbIDeviceCapabilities::FUNCTION_DEVICE;
  rv = mCapabilities->SetFunctionTypes(&functionTypes, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  // Allow registrars to modify the capabilities
  rv = RegisterDeviceCapabilities(mCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);

  // Complete the device capabilities configuration.
  rv = mCapabilities->ConfigureDone();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbCDDevice::Connect()
{
  nsresult rv;

  // Log progress.
  LOG("Enter sbCDDevice::Connect\n");

  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Do nothing if device is already connected.  If the device is not connected,
  // nothing should be accessing the "connect lock" fields.
  {
    sbAutoReadLock autoConnectLock(mConnectLock);
    if (mConnected)
      return NS_OK;
  }

  // Set up to auto-disconnect in case of error.
  SB_CD_DEVICE_AUTO_INVOKE(AutoDisconnect, Disconnect()) autoDisconnect(this);

  // Invoke the super-class.
  rv = sbBaseDevice::Connect();
  NS_ENSURE_SUCCESS(rv, rv);

  mTranscodeManager = do_ProxiedGetService(
                      "@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1",
                      &rv);

  // Connect the device capabilities.
  rv = CapabilitiesReset();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the device as connected.  After this, "connect lock" fields may be
  // used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_TRUE;
  }

  // Create a device volume.
  nsRefPtr<sbBaseDeviceVolume> volume;
  rv = sbBaseDeviceVolume::New(getter_AddRefs(volume), this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the volume GUID.
  char volumeGUID[NSID_LENGTH];
  mDeviceID.ToProvidedString(volumeGUID);
  rv = volume->SetGUID(NS_ConvertUTF8toUTF16(volumeGUID));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the volume.
  rv = AddVolume(volume);
  NS_ENSURE_SUCCESS(rv, rv);

  // Start the request processing.
  rv = ReqProcessingStart();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mount volume.
  Mount(volume);

  // Cancel auto-disconnect.
  autoDisconnect.forget();

  // Log progress.
  LOG("Exit sbCDDevice::Connect\n");

  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::Disconnect()
{
  return sbBaseDevice::Disconnect();
}

nsresult
sbCDDevice::DeviceSpecificDisconnect()
{
  // Log progress.
  LOG("Enter sbCDDevice::DeviceSpecificDisconnect\n");

  nsresult rv;

  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Remove object references.
  mTranscodeManager = nsnull;

  // Dispose of the library
  nsCOMPtr<sbIDeviceLibrary> deviceLib = mDeviceLibrary.forget();
  if (deviceLib) {
    rv = deviceLib->Finalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Indicate that the device is disconnected.
  mStatus->ChangeState(STATE_DISCONNECTED);

  // Unmount and remove the default volume.
  nsRefPtr<sbBaseDeviceVolume> volume;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    volume = mDefaultVolume;
  }
  if (volume) {
    rv = Unmount(volume);
    NS_ENSURE_SUCCESS(rv, rv);
    RemoveVolume(volume);
  }

  // Mark the device as not connected.  After this, "connect lock" fields may
  // not be used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_FALSE;
  }

  // Disconnect the device capabilities.
  mCapabilities = nsnull;

  // Log progress.
  LOG("Exit sbCDDevice::Disconnect\n");

  return NS_OK;
}

/* readonly attribute boolean connected; */
NS_IMETHODIMP
sbCDDevice::GetConnected(PRBool *aConnected)
{
  NS_ENSURE_ARG_POINTER(aConnected);
  sbAutoReadLock autoConnectLock(mConnectLock);
  *aConnected = mConnected;
  return NS_OK;
}

/* readonly attribute boolean threaded; */
NS_IMETHODIMP
sbCDDevice::GetThreaded(PRBool *aThreaded)
{
  NS_ENSURE_ARG_POINTER(aThreaded);
  *aThreaded = PR_TRUE;
  return NS_OK;
}

/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP
sbCDDevice::GetPreference(const nsAString & aPrefName, nsIVariant **_retval)
{
  // Transcoding profile-related prefs are global for CD device; not per device.
  if (StringBeginsWith(aPrefName, NS_LITERAL_STRING("transcode_profile")))
  {
    nsCOMPtr<nsIPrefBranch> prefBranch;
    nsresult rv = GetPrefBranch(PREF_CDDEVICE_RIPBRANCH,
                                getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, rv);
    return GetPreferenceInternal(prefBranch, aPrefName, _retval);
  }

  // Forward call to base class.
  return sbBaseDevice::GetPreference(aPrefName, _retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP
sbCDDevice::SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPrefValue);
  // Transcoding profile-related prefs are global for CD device; not per device.
  if (StringBeginsWith(aPrefName, NS_LITERAL_STRING("transcode_profile")))
  {
    nsCOMPtr<nsIPrefBranch> prefBranch;
    nsresult rv = GetPrefBranch(PREF_CDDEVICE_RIPBRANCH,
                                getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, rv);
    return SetPreferenceInternal(prefBranch, aPrefName, aPrefValue);
  }

  // Forward call to base class.
  return sbBaseDevice::SetPreference(aPrefName, aPrefValue);
}

/* readonly attribute sbIDeviceCapabilities capabilities; */
NS_IMETHODIMP
sbCDDevice::GetCapabilities(sbIDeviceCapabilities * *aCapabilities)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_IF_ADDREF(*aCapabilities = mCapabilities);
  return NS_OK;
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP
sbCDDevice::GetContent(sbIDeviceContent * *aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);
  NS_ENSURE_STATE(mDeviceContent);
  NS_ADDREF(*aContent = mDeviceContent);
  return NS_OK;
}

/* attribute sbIDeviceLibrary defaultLibrary; */
NS_IMETHODIMP
sbCDDevice::GetDefaultLibrary(sbIDeviceLibrary** aDefaultLibrary)
{
  return sbBaseDevice::GetDefaultLibrary(aDefaultLibrary);
}

NS_IMETHODIMP
sbCDDevice::SetDefaultLibrary(sbIDeviceLibrary* aDefaultLibrary)
{
  return sbBaseDevice::SetDefaultLibrary(aDefaultLibrary);
}

/* readonly attribute sbIDeviceLibrary primaryLibrary; */
NS_IMETHODIMP
sbCDDevice::GetPrimaryLibrary(sbIDeviceLibrary** aPrimaryLibrary)
{
  return sbBaseDevice::GetPrimaryLibrary(aPrimaryLibrary);
}

/* readonly attribute nsIPropertyBag2 parameters; */
NS_IMETHODIMP
sbCDDevice::GetParameters(nsIPropertyBag2 * *aParameters)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aParameters);

  // Function variables.
  nsresult rv = CallQueryInterface(mCreationProperties, aParameters);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* readonly attribute sbIDeviceProperties properties; */
NS_IMETHODIMP
sbCDDevice::GetProperties(sbIDeviceProperties * *aProperties)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aProperties);

  // Operate under the properties lock.
  nsAutoMonitor autoPropertiesLock(mPropertiesLock);

  *aProperties = nsnull;

  // Return results.
  if (mProperties) {
    NS_ADDREF(*aProperties = mProperties);
  } else {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

/* readonly attribute boolean isDirectTranscoding; */
NS_IMETHODIMP
sbCDDevice::GetIsDirectTranscoding(PRBool* aIsDirect)
{
  return sbBaseDevice::GetIsDirectTranscoding(aIsDirect);
}

/* readonly attribute boolean isBusy; */
NS_IMETHODIMP
sbCDDevice::GetIsBusy(PRBool *aIsBusy)
{
  return sbBaseDevice::GetIsBusy(aIsBusy);
}

/* readonly attribute boolean canDisconnect; */
NS_IMETHODIMP
sbCDDevice::GetCanDisconnect(PRBool *aCanDisconnect)
{
  return sbBaseDevice::GetCanDisconnect(aCanDisconnect);
}

/* readonly attribute sbIDeviceStatus currentStatus; */
NS_IMETHODIMP
sbCDDevice::GetCurrentStatus(sbIDeviceStatus * *aCurrentStatus)
{
  NS_ENSURE_ARG_POINTER(aCurrentStatus);
  return mStatus->GetCurrentStatus(aCurrentStatus);
}

/* readonly attribute boolean supportsReformat; */
NS_IMETHODIMP
sbCDDevice::GetSupportsReformat(PRBool *aSupportsReformat)
{
  NS_ENSURE_ARG_POINTER(aSupportsReformat);
  *aSupportsReformat = PR_FALSE;
  return NS_OK;
}

/* readonly attribute unsigned long state; */
NS_IMETHODIMP
sbCDDevice::GetState(PRUint32 *aState)
{
  return sbBaseDevice::GetState(aState);
}

NS_IMETHODIMP
sbCDDevice::SetState(PRUint32 aState)
{
  return sbBaseDevice::SetState(aState);
}

NS_IMETHODIMP
sbCDDevice::GetPreviousState(PRUint32 *aPreviousState)
{
  return sbBaseDevice::GetPreviousState(aPreviousState);
}

NS_IMETHODIMP
sbCDDevice::SubmitRequest(PRUint32 aRequest,
                          nsIPropertyBag2 *aRequestParameters)
{
  // Log progress.
  LOG("sbCDDevice::SubmitRequest\n");

  return sbBaseDevice::SubmitRequest(aRequest, aRequestParameters);
}

/* void cancelRequests (); */
NS_IMETHODIMP
sbCDDevice::CancelRequests()
{
  nsresult rv;

  // Check if we are ripping, if we are pop up a dialog to ask the user if
  // they really want to stop ripping.
  nsCOMPtr<sbIDeviceStatus> status;
  rv = GetCurrentStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 currentState;
  rv = status->GetCurrentState(&currentState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (currentState == sbIDevice::STATE_TRANSCODE) {
    PRBool abortRip;
    rv = sbDeviceUtils::QueryUserAbortRip(&abortRip);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!abortRip) {
      return NS_OK;
    }
  }

  // Cancel requests
  rv = sbBaseDevice::CancelRequests();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void syncLibraries (); */
NS_IMETHODIMP
sbCDDevice::SyncLibraries()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbCDDevice::SupportsMediaItem(sbIMediaItem*                  aMediaItem,
                              sbIDeviceSupportsItemCallback* aCallback)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void eject (); */
NS_IMETHODIMP
sbCDDevice::Eject()
{
  NS_ENSURE_TRUE(mCDDevice, NS_ERROR_UNEXPECTED);

  nsresult rv;
  rv = mCDDevice->Eject();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void format (); */
NS_IMETHODIMP
sbCDDevice::Format()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setWarningDialogEnabled (in AString aWarning, in boolean aEnabled); */
NS_IMETHODIMP
sbCDDevice::SetWarningDialogEnabled(const nsAString & aWarning,
                                    PRBool aEnabled)
{
  return sbBaseDevice::SetWarningDialogEnabled(aWarning, aEnabled);
}

/* boolean getWarningDialogEnabled (in AString aWarning); */
NS_IMETHODIMP
sbCDDevice::GetWarningDialogEnabled(const nsAString & aWarning,
                                    PRBool *_retval)
{
  return sbBaseDevice::GetWarningDialogEnabled(aWarning, _retval);
}

/* void resetWarningDialogs (); */
NS_IMETHODIMP
sbCDDevice::ResetWarningDialogs()
{
  return sbBaseDevice::ResetWarningDialogs();
}

/* nsIInputStream openInputStream(in nsIURI aURI); */
NS_IMETHODIMP
sbCDDevice::OpenInputStream(nsIURI*          aURI,
                            nsIInputStream** retval)
{
  return sbBaseDevice::OpenInputStream(aURI, retval);
}

NS_IMETHODIMP
sbCDDevice::ExportToDevice(sbIDeviceLibrary*    aDevLibrary,
                           sbILibraryChangeset* aExportChangeset)
{
  return sbBaseDevice::ExportToDevice(aDevLibrary, aExportChangeset);
}

NS_IMETHODIMP
sbCDDevice::ImportFromDevice(sbILibrary * aImportToLibrary,
                             sbILibraryChangeset * aImportChangeset)
{
  return sbBaseDevice::ImportFromDevice(aImportToLibrary, aImportChangeset);
}

nsresult
sbCDDevice::Mount(sbBaseDeviceVolume* aVolume)
{
  nsresult rv;

  // This function must only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Do nothing if volume has already been mounted.
  PRBool isMounted;
  rv = aVolume->GetIsMounted(&isMounted);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isMounted)
    return NS_OK;

  // Set the primary and default volume.
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    if (!mPrimaryVolume)
      mPrimaryVolume = aVolume;
    if (!mDefaultVolume)
      mDefaultVolume = aVolume;
  }

  // Make a string out of the device ID.
  char deviceID[NSID_LENGTH];
  mDeviceID.ToProvidedString(deviceID);

  // Create a unique device library ID using the device ID and volumeGUID.
  // Strip off the braces from the device ID.
  mDeviceLibraryPath.AssignLiteral("CD");
  mDeviceLibraryPath.Append(NS_ConvertUTF8toUTF16(deviceID + 1, NSID_LENGTH - 3));
  mDeviceLibraryPath.AppendLiteral("@devices.library.songbirdnest.com");

  // Create the device library.
  nsCOMPtr<sbIDeviceLibrary> deviceLibrary;
  rv = CreateDeviceLibrary(mDeviceLibraryPath,
                           nsnull,
                           getter_AddRefs(deviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the volume device library.
  rv = aVolume->SetDeviceLibrary(deviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the main device library.
  mDeviceLibrary = deviceLibrary;

  // Get the current CD disc hash.
  nsAutoString cdDiscHash;
  rv = GetCDDiscHash(mCDDevice, cdDiscHash);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the previous CD disc hash.
  nsAutoString prevCDDiscHash;
  rv = mDeviceLibrary->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CDDISCHASH),
                                   prevCDDiscHash);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    prevCDDiscHash.Truncate();
    rv = NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (!cdDiscHash.Equals(prevCDDiscHash)) {
    // Clear the library out if the CD disc hash is changing.
    rv = mDeviceLibrary->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                     SBVoidString());
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CDDISCHASH),
                                     SBVoidString());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Reset status if same disk was inserted again
    rv = sbDeviceUtils::BulkSetProperty(mDeviceLibrary,
                                        NS_LITERAL_STRING(SB_PROPERTY_CDRIP_STATUS),
                                        SBVoidString());
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed to reset rip status for CD tracks");

    rv = sbDeviceUtils::BulkSetProperty(mDeviceLibrary,
                                        NS_LITERAL_STRING(SB_PROPERTY_SHOULDRIP),
                                        NS_LITERAL_STRING("1"));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed to reset selection for CD tracks");
  }

  // hide the device library.
  rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                   NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the device properties.
  UpdateProperties();

  // Add the device library.
  rv = AddLibrary(deviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Mount the device. It's very important to send the mount request
  // before checking for setup. If the device isn't already mounting it
  // will not actually be sync-able because the sync request will be
  // submitted before we mount.
  rv = PushRequest(TransferRequest::REQUEST_MOUNT, nsnull, deviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Indicate that the volume has been mounted in Songbird.
  rv = aVolume->SetIsMounted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Release connect lock.
  autoConnectLock.unlock();

  return NS_OK;
}

nsresult
sbCDDevice::Unmount(sbBaseDeviceVolume* aVolume)
{
  nsresult rv;

  // This function must only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Get the volume info and mark the volume as no longer mounted.
  nsRefPtr<sbDeviceStatistics> deviceStatistics;
  rv = aVolume->GetStatistics(getter_AddRefs(deviceStatistics));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aVolume->SetIsMounted(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Do nothing if media volume not mounted.
  if (!mDeviceLibrary) {
    return NS_OK;
  }

  // Remove the device library from the device statistics.
  rv = deviceStatistics->RemoveLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove the device library and dispose of it.
  rv = RemoveLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libManager->UnregisterLibrary(mDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
sbCDDevice::IsRequestAborted()
{
  PRBool aborted = sbBaseDevice::IsRequestAborted();

  // Abort requests if disc is not inserted.
  if (!aborted) {
    PRBool isDiscInserted;
    nsresult rv = mCDDevice->GetIsDiscInserted(&isDiscInserted);
    aborted = NS_SUCCEEDED(rv) && !isDiscInserted ? PR_TRUE : PR_FALSE;
  }

  return aborted;
}

// Override base class: we want to return all profiles, not just those supported
// by the device (since that's not meaningful for a cd device!)
nsresult
sbCDDevice::GetSupportedTranscodeProfiles(PRUint32 aType,
                                          nsIArray **aSupportedProfiles)
{
  nsresult rv;
  nsCOMPtr<nsIArray> profiles;
  rv = sbDeviceUtils::GetTranscodeProfiles(aType,
                                           aSupportedProfiles);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// Override base class: CD devices are always read-only, and users won't
// expect otherwise, so popping up a dialog informing them is confusing.
// This overrides the sbBaseDevice::CheckAccess to not throw up the dialog
nsresult
sbCDDevice::CheckAccess(sbIDeviceLibrary* aDevLib)
{
  return NS_OK;
}

nsresult
sbCDDevice::HandleRipEnd()
{
  nsresult rv;

  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThreadManager> threadMgr =
      do_GetService("@mozilla.org/thread-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIThread> mainThread;
    rv = threadMgr->GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbCDDevice, this, ProxyHandleRipEnd);
    NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

    rv = mainThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    ProxyHandleRipEnd();
  }

  return NS_OK;
}

void
sbCDDevice::ProxyHandleRipEnd()
{
  // Dispatch the event to notify listeners that we've finished the rip job.
  CreateAndDispatchEvent(sbICDDeviceEvent::EVENT_CDRIP_COMPLETED,
                         sbNewVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)));

  // Check to see if any errors occurred during the transcode.
  nsresult rv;
  nsCOMPtr<sbIDeviceErrorMonitor> errMonitor =
      do_GetService("@songbirdnest.com/device/error-monitor-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  PRBool hasErrors;
  rv = errMonitor->DeviceHasErrors(this, EmptyString(), 0, &hasErrors);
  NS_ENSURE_SUCCESS(rv, /* void */);

  if (hasErrors) {
    // The rip operation has completed, but there were a few errors during
    // the transcode. Show those errors now.
    rv = sbDeviceUtils::QueryUserViewErrors(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not show user view errors!");

    // Now that we've shown the errors for the user, clear out the errors
    // for this device.
    rv = errMonitor->ClearErrorsForDevice(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not clear the device errors!");
  }
  else {
    // Check the preferences to see if we should eject
    if (mPrefAutoEject) {
      // Since we successfully ripped all selected tracks and the user has
      // the autoEject preference set, we can eject now.
      rv = Eject();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not eject the CD!");
    }

    // if the user wants a sound notification, then beep
    if (mPrefNotifySound) {
      nsCOMPtr<nsISound> soundInterface =
        do_CreateInstance("@mozilla.org/sound;1", &rv);
      NS_ENSURE_SUCCESS(rv, /* void */);

      soundInterface->Beep();
    }
  }
}

nsresult
sbCDDevice::QueryUserViewErrors()
{
  nsresult rv;

  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThreadManager> threadMgr =
      do_GetService("@mozilla.org/thread-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIThread> mainThread;
    rv = threadMgr->GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbCDDevice, this, ProxyQueryUserViewErrors);
    NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

    rv = mainThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    ProxyQueryUserViewErrors();
  }

  return NS_OK;
}

void
sbCDDevice::ProxyQueryUserViewErrors()
{
  nsresult SB_UNUSED_IN_RELEASE(rv) = sbDeviceUtils::QueryUserViewErrors(this);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not show user view errors!");
}

nsresult
sbCDDevice::GetCDDiscHash(sbICDDevice* aCDDevice,
                          nsAString&   aCDDiscHash)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCDDevice);

  // Function variables.
  nsresult rv;

  // Create a hash object.
  nsCOMPtr<nsICryptoHash>
    cryptoHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the disc TOC.
  nsCOMPtr<sbICDTOC> toc;
  rv = mCDDevice->GetDiscTOC(getter_AddRefs(toc));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the disc tracks.
  nsCOMPtr<nsIArray> tracks;
  rv = toc->GetTracks(getter_AddRefs(tracks));
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 trackCount;
  rv = tracks->GetLength(&trackCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add each track offset to the hash.
  for (PRUint32 i = 0; i < trackCount; i++) {
    // Get the next track.
    nsCOMPtr<sbICDTOCEntry> track = do_QueryElementAt(tracks, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the track offset.
    PRInt32 frameOffset;
    rv = track->GetFrameOffset(&frameOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the track offset to the hash.
    rv = cryptoHash->Update(reinterpret_cast<PRUint8 const *>(&frameOffset),
                            sizeof(PRInt32));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Add the lead out track offset to the hash.
  PRInt32 leadOutOffset;
  rv = toc->GetLeadOutTrackOffset(&leadOutOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Update(reinterpret_cast<PRUint8 const *>(&leadOutOffset),
                          sizeof(PRInt32));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  nsCString cdDiscHash;
  rv = cryptoHash->Finish(PR_TRUE, cdDiscHash);
  NS_ENSURE_SUCCESS(rv, rv);
  aCDDiscHash.Assign(NS_ConvertASCIItoUTF16(cdDiscHash));

  return NS_OK;
}
