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

#include "sbCDDevice.h"

#include "sbCDLog.h"

#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIGenericFactory.h>
#include <nsIPropertyBag2.h>
#include <nsIProgrammingLanguage.h>
#include <nsIUUIDGenerator.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsILocalFile.h>

#include <sbAutoRWLock.h>
#include <sbDeviceContent.h>
#include <sbDeviceUtils.h>
#include <sbIDeviceEvent.h>
#include <sbILibraryManager.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbVariantUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbCDDevice:5
 */
#ifdef PR_LOGGING
PRLogModuleInfo* gCDDeviceLog = nsnull;
#endif


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

sbCDDevice::sbCDDevice(const nsID & aControllerId,
                       nsIPropertyBag *aProperties)
  : mConnected(PR_FALSE)
  , mStatus(this)
  , mCreationProperties(aProperties)
  , mControllerID(aControllerId)
  , mIsHandlingRequests(PR_FALSE)
{
  mPropertiesLock = nsAutoMonitor::NewMonitor("sbCDDevice::mPropertiesLock");
  NS_ENSURE_TRUE(mPropertiesLock, );

  mConnectLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE,
                              "sbCDDevice::mConnectLock");
  NS_ENSURE_TRUE(mConnectLock, );

  mReqWaitMonitor = nsAutoMonitor::NewMonitor("sbCDDevice::mReqWaitMonitor");
  NS_ENSURE_TRUE(mReqWaitMonitor, );

#ifdef PR_LOGGING
  if (!gCDDeviceLog) {
    gCDDeviceLog = PR_NewLogModule( "sbCDDevice" );
  }
#endif
}

sbCDDevice::~sbCDDevice()
{
  nsresult rv;
  if (mDeviceLibrary) {
    rv = mDeviceLibrary->Finalize();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "sbCDDevice failed to finalize the device library");
  }

  mDeviceLibrary = nsnull;

  if (!mDeviceLibraryPath.IsEmpty()) {
    nsCOMPtr<nsILocalFile> libraryFile;
    rv = NS_NewLocalFile(mDeviceLibraryPath,
        PR_FALSE,
        getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, /* void */);

    rv = libraryFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

nsresult
sbCDDevice::CreateDeviceID(nsID & aDeviceID)
{
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uuidGen->GenerateUUIDInPlace(&aDeviceID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbCDDevice::InitDevice()
{
  nsresult rv;

  // Log progress.
  LOG(("Enter sbCDDevice::InitDevice"));

  // Create the connect lock.
  mConnectLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE,
                              "sbCDDevice::mConnectLock");
  NS_ENSURE_TRUE(mConnectLock, NS_ERROR_OUT_OF_MEMORY);

  rv = mStatus.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the device state.
  SetState(sbIDevice::STATE_IDLE);

  // Create the device ID.  This depends upon some of the device properties.
  rv = CreateDeviceID(mDeviceID);
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Log progress.
  LOG(("Exit sbCDDevice::InitDevice"));

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
  if (NS_FAILED(rv)) {
    // XXX TODO: Internationalize
    deviceName.AssignLiteral("Unknown");
  }
  rv = mProperties->InitFriendlyName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mProperties->InitDone();
  NS_ENSURE_SUCCESS(rv, rv);

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

  PRUint32 functionTypes = sbIDeviceCapabilities::FUNCTION_DEVICE;
  rv = mCapabilities->SetFunctionTypes(&functionTypes, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  // Allow registrars to modify the capabilities
  rv = RegisterDeviceCapabilities(mCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);

  // Complete the device capabilities initialization.
  rv = mCapabilities->InitDone();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbCDDevice::ReqConnect()
{
  nsresult rv;

  // Clear the abort requests flag.
  PR_AtomicSet(&mAbortRequests, 0);

  mTranscodeManager =
    do_ProxiedGetService("@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1",
                         &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the request wait monitor.
  mReqWaitMonitor =
    nsAutoMonitor::NewMonitor("sbCDDevice::mReqWaitMonitor");
  NS_ENSURE_TRUE(mReqWaitMonitor, NS_ERROR_OUT_OF_MEMORY);

  // Create the request added event object.
  rv = sbDeviceReqAddedEvent::New(this, getter_AddRefs(mReqAddedEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the request processing thread.
  rv = NS_NewThread(getter_AddRefs(mReqThread), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbCDDevice::Connect()
{
  nsresult rv;

  // Log progress.
  LOG(("Enter sbCDDevice::Connect\n"));

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

  // Connect the request services.
  rv = ReqConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Connect the device capabilities.
  rv = CapabilitiesReset();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the device as connected.  After this, "connect lock" fields may be
  // used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_TRUE;
  }

  Mount();
  // Start the request processing.
  rv = ProcessRequest();
  NS_ENSURE_SUCCESS(rv, rv);

  // Cancel auto-disconnect.
  autoDisconnect.forget();

  // Log progress.
  LOG(("Exit sbCDDevice::Connect\n"));

  return NS_OK;
}

nsresult
sbCDDevice::ProcessRequest()
{
  nsresult rv;

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);

  // if we're not connected then do nothing
  if (!mConnected) {
    return NS_OK;
  }

  // Dispatch processing of the request added event.
  rv = mReqThread->Dispatch(mReqAddedEvent, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbCDDevice::ReqProcessingStop()
{
  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Set the abort requests flag.
  PR_AtomicSet(&mAbortRequests, 1);

  // Notify the request thread of the abort.
  {
    nsAutoMonitor monitor(mReqWaitMonitor);
    monitor.Notify();
  }

  // Shutdown the request processing thread.
  mReqThread->Shutdown();

  return NS_OK;
}

nsresult
sbCDDevice::ReqDisconnect()
{
  // Clear all remaining requests.
  char deviceIDStr[NSID_LENGTH];
  mDeviceID.ToProvidedString(deviceIDStr);
  nsresult rv = ClearRequests(NS_ConvertASCIItoUTF16(deviceIDStr));
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove object references.
  mReqThread = nsnull;
  mReqAddedEvent = nsnull;
  mTranscodeManager = nsnull;

  // Dispose of the request wait monitor.
  if (mReqWaitMonitor) {
    nsAutoMonitor::DestroyMonitor(mReqWaitMonitor);
    mReqWaitMonitor = nsnull;
  }
  return NS_OK;
}

/* void disconnect (); */
NS_IMETHODIMP
sbCDDevice::Disconnect()
{
  // Log progress.
  LOG(("Enter sbCDDevice::Disconnect\n"));

  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  nsresult rv = Unmount();
  NS_ENSURE_SUCCESS(rv, rv);

  // Stop the request processing.
  rv = ReqProcessingStop();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the device as not connected.  After this, "connect lock" fields may
  // not be used.
  {
    sbAutoWriteLock autoConnectLock(mConnectLock);
    mConnected = PR_FALSE;
  }

  // Disconnect the device capabilities.
  mCapabilities = nsnull;

  // Disconnect the device services.
  rv = ReqDisconnect();
  NS_ENSURE_SUCCESS(rv, rv);

  // Send device removed notification.
  rv = CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_REMOVED,
                              sbNewVariant(static_cast<sbIDevice*>(this)));
  NS_ENSURE_SUCCESS(rv, rv);

  // Log progress.
  LOG(("Exit sbCDDevice::Disconnect\n"));

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
  // Forward call to base class.
  return sbBaseDevice::GetPreference(aPrefName, _retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP
sbCDDevice::SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPrefValue);

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
  return mStatus.GetCurrentStatus(aCurrentStatus);
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
sbCDDevice::GetPreviousState(PRUint32 *aPreviousState)
{
  return sbBaseDevice::GetPreviousState(aPreviousState);
}

NS_IMETHODIMP
sbCDDevice::SubmitRequest(PRUint32 aRequest,
                          nsIPropertyBag2 *aRequestParameters)
{
  // Log progress.
  LOG(("sbCDDevice::SubmitRequest\n"));

  // Function variables.
  nsresult rv;

  // Create a transfer request.
  nsRefPtr<TransferRequest> transferRequest;
  rv = CreateTransferRequest(aRequest,
                             aRequestParameters,
                             getter_AddRefs(transferRequest));
  NS_ENSURE_SUCCESS(rv, rv);

  // Push the request onto the request queue.
  return PushRequest(transferRequest);
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

  // Convert the device ID to a string.
  char deviceIDString[NSID_LENGTH];
  mDeviceID.ToProvidedString(deviceIDString);

  // Clear requests.
  rv = ClearRequests(NS_ConvertASCIItoUTF16(deviceIDString, NSID_LENGTH-1));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void syncLibraries (); */
NS_IMETHODIMP
sbCDDevice::SyncLibraries()
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

nsresult
sbCDDevice::Mount()
{
  nsresult rv;

  // This function must only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Do nothing if a media volume has already been mounted.
  if (mDeviceLibrary)
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;

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

  // Set the main device library.
  mDeviceLibrary = deviceLibrary;

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

  // Release connect lock.
  autoConnectLock.unlock();

  return NS_OK;
}

nsresult
sbCDDevice::Unmount()
{
  // This function must only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Do nothing if media volume not mounted.
  if (!mDeviceLibrary) {
    return NS_OK;
  }

  // Remove the device library from the device statistics.
  nsresult rv = mDeviceStatistics->RemoveLibrary(mDeviceLibrary);
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
sbCDDevice::ReqAbortActive()
{
  // Atomically get the abort requests flag by atomically adding 0 and reading
  // the result.
  PRBool abortRequests = PR_AtomicAdd(&mAbortRequests, 0);
  if (!abortRequests) {
    abortRequests = IsRequestAbortedOrDeviceDisconnected();
  }

  return (abortRequests != 0);
}

