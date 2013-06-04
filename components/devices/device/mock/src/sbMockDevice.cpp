/* vim: set sw=2 :miv */
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

#include "sbMockDevice.h"

#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>

#include <mozilla/Mutex.h>
#include <nsArrayUtils.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>

#include <sbIDeviceCapabilities.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceLibrary.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceProperties.h>
#include <sbRequestItem.h>

#include <sbDeviceContent.h>
#include <sbVariantUtils.h>

/* for an actual device, you would probably want to actually sort the prefs on
 * the device itself (and not the mozilla prefs system).  And even if you do end
 * up wanting to store things in the prefs system for some odd reason, you would
 * want to have a unique id per instance of the device and not a hard-coded one.
 */
#define DEVICE_PREF_BRANCH \
  "songbird.devices.mock.00000000-0000-0000-c000-000000000046."

NS_IMPL_THREADSAFE_ADDREF(sbMockDevice)
NS_IMPL_THREADSAFE_RELEASE(sbMockDevice)
NS_INTERFACE_MAP_BEGIN(sbMockDevice)
  NS_INTERFACE_MAP_ENTRY(sbIMockDevice)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbIDevice, sbBaseDevice)
  NS_INTERFACE_MAP_ENTRY(sbIDeviceEventTarget)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDeviceEventTarget)
NS_INTERFACE_MAP_END

#if _MSC_VER
// Disable warning about 'this' used in base member initializer list.
#pragma warning(disable: 4355)
#endif

sbMockDevice::sbMockDevice()
 : mIsConnected(PR_FALSE)
{
  Init();
}

sbMockDevice::~sbMockDevice()
{
  /* destructor code */
}

/* readonly attribute AString name; */
NS_IMETHODIMP sbMockDevice::GetName(nsAString & aName)
{
  aName.AssignLiteral("Bob's Mock Device");
  return NS_OK;
}

/* readonly attribute AString productName; */
NS_IMETHODIMP sbMockDevice::GetProductName(nsAString & aProductName)
{
  aProductName.AssignLiteral("Mock Device");
  return NS_OK;
}

/* readonly attribute nsIDPtr controllerId; */
NS_IMETHODIMP sbMockDevice::GetControllerId(nsID * *aControllerId)
{
  NS_ENSURE_ARG_POINTER(aControllerId);

  *aControllerId = (nsID*)NS_Alloc(sizeof(nsID));
  NS_ENSURE_TRUE(*aControllerId, NS_ERROR_OUT_OF_MEMORY);
  **aControllerId = NS_GET_IID(nsISupports);
  return NS_OK;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP sbMockDevice::GetId(nsID * *aId)
{
  NS_ENSURE_ARG_POINTER(aId);

  nsID mockDeviceID;

  PRBool success =
    mockDeviceID.Parse("{3572E6FC-4954-4458-AFE7-0D0A65BF5F55}");
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  *aId = (nsID*)NS_Alloc(sizeof(nsID));
  NS_ENSURE_TRUE(*aId, NS_ERROR_OUT_OF_MEMORY);

  **aId = mockDeviceID;

  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbMockDevice::Connect()
{
  NS_ENSURE_STATE(!mIsConnected);
  nsresult rv;

  // Invoke the super-class.
  rv = sbBaseDevice::Connect();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceContent> deviceContent;
  rv = GetContent(getter_AddRefs(deviceContent));
  NS_ENSURE_SUCCESS(rv, rv);

  // Start the request processing.
  rv = ReqProcessingStart();
  NS_ENSURE_SUCCESS(rv, rv);

  mIsConnected = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::Disconnect()
{
  mBatch.clear();
  return sbBaseDevice::Disconnect();
}

/* void disconnect (); */
nsresult sbMockDevice::DeviceSpecificDisconnect()
{
  NS_ENSURE_STATE(mIsConnected);

  nsresult rv;
  nsRefPtr<sbBaseDeviceVolume> volume;
  {
    mozilla::MutexAutoLock autoVolumeLock(mVolumeLock);
    volume = mDefaultVolume;
    mDefaultVolume = nsnull;
  }
  if (volume)
    RemoveVolume(volume);

  mIsConnected = PR_FALSE;

  // Finalize the device content and device libraries
  if (mContent) {
    // Get a copy of the list of device libraries
    nsCOMArray<sbIDeviceLibrary> libraryListCopy;
    PRInt32                      libraryListCopyCount;
    nsCOMPtr<nsIArray>           libraryList;
    PRUint32                     libraryCount;
    rv = mContent->GetLibraries(getter_AddRefs(libraryList));
    if (NS_SUCCEEDED(rv))
      rv = libraryList->GetLength(&libraryCount);
    if (NS_SUCCEEDED(rv)) {
      for (PRUint32 i = 0; i < libraryCount; i++) {
        nsCOMPtr<sbIDeviceLibrary>
          library = do_QueryElementAt(libraryList, i, &rv);
        if (NS_FAILED(rv))
          continue;
        libraryListCopy.AppendObject(library);
      }
    }
    libraryListCopyCount = libraryListCopy.Count();

    // Finalize each device library
    for (PRInt32 i = 0; i < libraryListCopyCount; i++) {
      RemoveLibrary(libraryListCopy[i]);
      FinalizeDeviceLibrary(libraryListCopy[i]);
    }

    // Finalize the device content
    mContent->Finalize();
    mContent = nsnull;
  }

  PRUint32 state = sbIDevice::STATE_IDLE;

  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbNewVariant data(static_cast<sbIDevice*>(static_cast<sbBaseDevice*>(this)));
  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(sbIDeviceEvent::EVENT_DEVICE_REMOVED,
                            sbNewVariant(static_cast<sbIDevice*>(static_cast<sbBaseDevice*>(this))),
                            data,
                            state,
                            state,
                            getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dispatched;
  rv = DispatchEvent(deviceEvent, PR_TRUE, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* readonly attribute boolean connected; */
NS_IMETHODIMP sbMockDevice::GetConnected(PRBool *aConnected)
{
  NS_ENSURE_ARG_POINTER(aConnected);
  *aConnected = mIsConnected;
  return NS_OK;
}

/* readonly attribute boolean threaded; */
NS_IMETHODIMP sbMockDevice::GetThreaded(PRBool *aThreaded)
{
  NS_ENSURE_ARG_POINTER(aThreaded);
  *aThreaded = PR_FALSE;
  return NS_OK;
}

/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP sbMockDevice::GetPreference(const nsAString & aPrefName, nsIVariant **_retval)
{
  /* what, you expect a mock device to be actually useful? */
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefRoot =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(DEVICE_PREF_BRANCH, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_LossyConvertUTF16toASCII prefNameC(aPrefName);

  PRInt32 prefType;
  rv = prefBranch->GetPrefType(prefNameC.get(),
                               &prefType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> result =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(prefType) {
    case nsIPrefBranch::PREF_INVALID: {
      rv = result->SetAsVoid();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_STRING: {
      char *value = nsnull;
      rv = prefBranch->GetCharPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsString(value);
      NS_Free(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_INT: {
      PRInt32 value;
      rv = prefBranch->GetIntPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsInt32(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_BOOL: {
      PRBool value;
      rv = prefBranch->GetBoolPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsBool(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    default: {
      /* wtf? */
      return NS_ERROR_UNEXPECTED;
    }
  }

  return CallQueryInterface(result, _retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP sbMockDevice::SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue)
{
  NS_ENSURE_ARG_POINTER(aPrefValue);
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefRoot =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(DEVICE_PREF_BRANCH, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_LossyConvertUTF16toASCII prefNameC(aPrefName);

  PRUint16 prefType;
  rv = aPrefValue->GetDataType(&prefType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 oldPrefType;
  rv = prefBranch->GetPrefType(prefNameC.get(), &oldPrefType);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(prefType) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_UINT16:
    case nsIDataType::VTYPE_UINT32:
    case nsIDataType::VTYPE_UINT64:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_INT) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      PRInt32 value;
      rv = aPrefValue->GetAsInt32(&value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetIntPref(prefNameC.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);

      /* special case for state */
      if (aPrefName.Equals(NS_LITERAL_STRING("state"))) {
        mState = value;
      }

      break;
    }
    case nsIDataType::VTYPE_BOOL:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_BOOL) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      PRBool value;
      rv = aPrefValue->GetAsBool(&value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetBoolPref(prefNameC.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    default:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_STRING) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      nsCString value;
      rv = aPrefValue->GetAsACString(value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetCharPref(prefNameC.get(), value.get());
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
  }

  return NS_OK;
}

/* readonly attribute sbIDeviceCapabilities capabilities; */
NS_IMETHODIMP sbMockDevice::GetCapabilities(sbIDeviceCapabilities * *aCapabilities)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);
  nsresult rv;

  // Create the device capabilities object.
  nsCOMPtr<sbIDeviceCapabilities> caps =
    do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = caps->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 functionTypes[] = {sbIDeviceCapabilities::FUNCTION_DEVICE,
                              sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK};
  rv = caps->SetFunctionTypes(functionTypes, NS_ARRAY_LENGTH(functionTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 contentTypes[] = {sbIDeviceCapabilities::CONTENT_VIDEO};
  rv = caps->AddContentTypes(sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK,
                             contentTypes, NS_ARRAY_LENGTH(contentTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  const char* K_MIMETYPE_STRING = "ogg-theora-vorbis-sample-1";
  rv = caps->AddMimeTypes(sbIDeviceCapabilities::CONTENT_VIDEO,
                          &K_MIMETYPE_STRING, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapVideoStream> videoFormat =
    do_CreateInstance(SB_IDEVCAPVIDEOSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIImageSize> videoSize =
    do_CreateInstance(SB_IMAGESIZE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoSize->Initialize(1280, 720);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> videoSizeArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoSizeArray->AppendElement(videoSize, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> videoBitrateRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoBitrateRange->Initialize(100, 4 * 1024 * 1024, 100);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the video par range:
  nsCOMPtr<sbIDevCapFraction> parFraction =
    do_CreateInstance("@songbirdnest.com/Songbird/Device/sbfraction;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = parFraction->Initialize(1, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> parFractionArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = parFractionArray->AppendElement(parFraction, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the video frame rate range:
  nsCOMPtr<sbIDevCapFraction> frameRateFraction =
    do_CreateInstance("@songbirdnest.com/Songbird/Device/sbfraction;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = frameRateFraction->Initialize(30000, 1001);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> frameRateFractionArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = frameRateFractionArray->AppendElement(frameRateFraction, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = videoFormat->Initialize(NS_LITERAL_CSTRING("video/x-theora"),
                               videoSizeArray,
                               nsnull, // explicit sizes only
                               nsnull, // explicit sizes only
                               parFractionArray,
                               PR_FALSE,
                               frameRateFractionArray,
                               PR_FALSE,
                               videoBitrateRange);

  nsCOMPtr<sbIDevCapAudioStream> audioFormat =
    do_CreateInstance(SB_IDEVCAPAUDIOSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDevCapRange> audioBitrateRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioBitrateRange->Initialize(100, 4 * 1024 * 1024, 100);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDevCapRange> audioSampleRateRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioSampleRateRange->Initialize(22050, 44100, 22050);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDevCapRange> audioChannelsRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioChannelsRange->Initialize(1, 2, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioFormat->Initialize(NS_LITERAL_CSTRING("audio/x-vorbis"),
                               audioBitrateRange,
                               audioSampleRateRange,
                               audioChannelsRange);

  nsCOMPtr<sbIVideoFormatType> formatType =
    do_CreateInstance(SB_IVIDEOFORMATTYPE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = formatType->Initialize(NS_LITERAL_CSTRING("application/ogg"),
                              videoFormat, audioFormat);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = caps->AddFormatType(sbIDeviceCapabilities::CONTENT_VIDEO,
                           NS_ConvertASCIItoUTF16(K_MIMETYPE_STRING),
                           formatType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Complete the device capabilities configuration.
  rv = caps->ConfigureDone();
  NS_ENSURE_SUCCESS(rv, rv);

  caps.forget(aCapabilities);
  return NS_OK;
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP sbMockDevice::GetContent(sbIDeviceContent * *aContent)
{
  nsresult rv;
  if (!mContent) {
    nsRefPtr<sbDeviceContent> deviceContent = sbDeviceContent::New();
    NS_ENSURE_TRUE(deviceContent, NS_ERROR_OUT_OF_MEMORY);
    rv = deviceContent->Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
    mContent = deviceContent;

    // Create a device volume.
    nsRefPtr<sbBaseDeviceVolume> volume;
    rv = sbBaseDeviceVolume::New(getter_AddRefs(volume), this);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the volume GUID.
    char volumeGUID[NSID_LENGTH];
    nsID* deviceID;
    rv = GetId(&deviceID);
    NS_ENSURE_SUCCESS(rv, rv);
    deviceID->ToProvidedString(volumeGUID);
    NS_Free(deviceID);
    rv = volume->SetGUID(NS_ConvertUTF8toUTF16(volumeGUID));
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the volume.
    rv = AddVolume(volume);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the primary and default volume.
    {
      mozilla::MutexAutoLock autoVolumeLock(mVolumeLock);
      mPrimaryVolume = volume;
      mDefaultVolume = volume;
    }

    // make a mock library too
    NS_NAMED_LITERAL_STRING(LIBID, "mock-library.mock-device");
    nsCOMPtr<sbIDeviceLibrary> devLib;
    rv = CreateDeviceLibrary(LIBID, nsnull, getter_AddRefs(devLib));
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the volume device library.
    rv = volume->SetDeviceLibrary(devLib);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddLibrary(devLib);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ADDREF(*aContent = mContent);
  return NS_OK;
}

/* attribute sbIDeviceLibrary defaultLibrary; */
NS_IMETHODIMP
sbMockDevice::GetDefaultLibrary(sbIDeviceLibrary** aDefaultLibrary)
{
  return sbBaseDevice::GetDefaultLibrary(aDefaultLibrary);
}

NS_IMETHODIMP
sbMockDevice::SetDefaultLibrary(sbIDeviceLibrary* aDefaultLibrary)
{
  return sbBaseDevice::SetDefaultLibrary(aDefaultLibrary);
}

/* readonly attribute sbIDeviceLibrary primaryLibrary; */
NS_IMETHODIMP
sbMockDevice::GetPrimaryLibrary(sbIDeviceLibrary** aPrimaryLibrary)
{
  return sbBaseDevice::GetPrimaryLibrary(aPrimaryLibrary);
}

/* readonly attribute nsIPropertyBag2 parameters; */
NS_IMETHODIMP sbMockDevice::GetParameters(nsIPropertyBag2 * *aParameters)
{
  nsresult rv;

  nsCOMPtr<nsIWritablePropertyBag> writeBag =
        do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> deviceType =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Pretend like we're an MTP device.
  rv = deviceType->SetAsAString(NS_LITERAL_STRING("MTP"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = writeBag->SetProperty(NS_LITERAL_STRING("DeviceType"),
                             deviceType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPropertyBag2> propBag = do_QueryInterface(writeBag, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  propBag.forget(aParameters);

  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::GetProperties(sbIDeviceProperties * *theProperties)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  if(!mProperties) {
    nsCOMPtr<sbIDeviceProperties> properties =
      do_CreateInstance("@songbirdnest.com/Songbird/Device/DeviceProperties;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitFriendlyName(NS_LITERAL_STRING("Testing Device"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitVendorName(NS_LITERAL_STRING("ACME Inc."));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWritableVariant> modelNumber =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = modelNumber->SetAsString("ACME 9000");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitModelNumber(modelNumber);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWritableVariant> serialNumber =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = serialNumber->SetAsString("ACME-9000-0001-2000-3000");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitSerialNumber(serialNumber);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitFirmwareVersion(NS_LITERAL_STRING("1.0.0.0"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWritablePropertyBag> writeBag =
        do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWritableVariant> freeSpace =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = freeSpace->SetAsString("17179869184");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = writeBag->SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#freeSpace"),
                               freeSpace);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWritableVariant> totalUsedSpace =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = totalUsedSpace->SetAsString("4294967296");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = writeBag->SetProperty(NS_LITERAL_STRING("http://songbirdnest.com/device/1.0#totalUsedSpace"),
                               totalUsedSpace);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPropertyBag2> propBag = do_QueryInterface(writeBag, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitDeviceProperties(propBag);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->InitDone();
    NS_ENSURE_SUCCESS(rv, rv);

    mProperties = properties;
  }

  NS_ADDREF(*theProperties = mProperties);

  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::SubmitRequest(PRUint32 aRequestType,
                                          nsIPropertyBag2 *aRequestParameters)
{
  return sbBaseDevice::SubmitRequest(aRequestType, aRequestParameters);
}

nsresult sbMockDevice::ProcessBatch(Batch & aBatch)
{
  std::insert_iterator<std::vector<nsRefPtr<sbRequestItem> > >
    insertIter(mBatch, mBatch.end());
  std::copy(aBatch.begin(), aBatch.end(), insertIter);

  /* don't process, let the js deal with it */
  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::CancelRequests()
{
  return mRequestThreadQueue->CancelRequests();
}

NS_IMETHODIMP sbMockDevice::Eject()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMockDevice::GetIsDirectTranscoding(PRBool* aIsDirect)
{
  return sbBaseDevice::GetIsDirectTranscoding(aIsDirect);
}

NS_IMETHODIMP sbMockDevice::GetIsBusy(PRBool *aIsBusy)
{
  nsCOMPtr<nsIVariant> busyVariant;
  nsresult rv =
    GetPreference(NS_LITERAL_STRING("testing.busy"),
                  getter_AddRefs(busyVariant));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 dataType = 0;
  rv = busyVariant->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if(dataType == nsIDataType::VTYPE_BOOL) {
    rv = busyVariant->GetAsBool(aIsBusy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = sbBaseDevice::GetIsBusy(aIsBusy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::GetCanDisconnect(PRBool *aCanDisconnect)
{
  return sbBaseDevice::GetCanDisconnect(aCanDisconnect);
}

NS_IMETHODIMP sbMockDevice::GetState(PRUint32 *aState)
{
  return sbBaseDevice::GetState(aState);
}

NS_IMETHODIMP sbMockDevice::SetState(PRUint32 aState)
{
  return sbBaseDevice::SetState(aState);
}

NS_IMETHODIMP sbMockDevice::GetPreviousState(PRUint32 *aState)
{
  return sbBaseDevice::GetPreviousState(aState);
}

NS_IMETHODIMP sbMockDevice::SyncLibraries()
{
  return sbBaseDevice::SyncLibraries();
}

NS_IMETHODIMP sbMockDevice::SupportsMediaItem(
    sbIMediaItem*                  aMediaItem,
    sbIDeviceSupportsItemCallback* aCallback)
{
  return sbBaseDevice::SupportsMediaItem(aMediaItem, aCallback);
}


/****************************** sbIMockDevice ******************************/

#define SET_PROP(type, name) \
  rv = bag->SetPropertyAs ## type(NS_LITERAL_STRING(#name), \
                                  transferRequest->name); \
  NS_ENSURE_SUCCESS(rv, rv);

/* nsIPropertyBag2 PopRequest (); */
NS_IMETHODIMP sbMockDevice::PopRequest(nsIPropertyBag2 **_retval)
{
  // while it's easier to reuse PeekRequest, that sort of defeats the purpose
  // of testing.
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  if (mBatch.empty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  nsRefPtr<sbRequestItem> requestItem = *mBatch.begin();
  mBatch.erase(mBatch.begin());


  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const PRInt32 batchCount  = mBatch.size();

  rv = bag->SetPropertyAsInt32(NS_LITERAL_STRING("requestType"),
                               requestItem->GetType());
  rv = bag->SetPropertyAsInt32(NS_LITERAL_STRING("batchCount"), batchCount);
  NS_ENSURE_SUCCESS(rv, rv);

  const PRInt32 batchIndex = requestItem->GetBatchIndex();

  rv = bag->SetPropertyAsInt32(NS_LITERAL_STRING("batchIndex"), batchIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  const PRInt32 requestId = requestItem->GetRequestId();

  rv = bag->SetPropertyAsInt32(NS_LITERAL_STRING("itemTransferID"),
                               requestId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (requestItem->GetType() >= sbRequestThreadQueue::USER_REQUEST_TYPES) {
    TransferRequest * transferRequest =
        static_cast<TransferRequest *>(requestItem.get());
    SET_PROP(Interface, item);
    SET_PROP(Interface, list);
    SET_PROP(Interface, data);
    SET_PROP(Uint32, index);
    SET_PROP(Uint32, otherIndex);
  }
  return CallQueryInterface(bag, _retval);
}

NS_IMETHODIMP sbMockDevice::SetWarningDialogEnabled(const nsAString & aWarning, PRBool aEnabled)
{
  return sbBaseDevice::SetWarningDialogEnabled(aWarning, aEnabled);
}

NS_IMETHODIMP sbMockDevice::GetWarningDialogEnabled(const nsAString & aWarning, PRBool *_retval)
{
  return sbBaseDevice::GetWarningDialogEnabled(aWarning, _retval);
}

NS_IMETHODIMP sbMockDevice::ResetWarningDialogs()
{
  return sbBaseDevice::ResetWarningDialogs();
}

NS_IMETHODIMP sbMockDevice::OpenInputStream(nsIURI*          aURI,
                                            nsIInputStream** retval)
{
  return sbBaseDevice::OpenInputStream(aURI, retval);
}

NS_IMETHODIMP sbMockDevice::GetCurrentStatus(sbIDeviceStatus * *aCurrentStatus)
{
  NS_ENSURE_ARG(aCurrentStatus);
  return mStatus->GetCurrentStatus(aCurrentStatus);
}


/* void Format(); */
NS_IMETHODIMP sbMockDevice::Format()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
/* readonly attribute boolean supportsReformat; */
NS_IMETHODIMP sbMockDevice::GetSupportsReformat(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbMockDevice::ExportToDevice(sbIDeviceLibrary*    aDevLibrary,
                             sbILibraryChangeset* aExportChangeset)
{
  return sbBaseDevice::ExportToDevice(aDevLibrary, aExportChangeset);
}

NS_IMETHODIMP
sbMockDevice::ImportFromDevice(sbILibrary * aImportToLibrary,
                               sbILibraryChangeset * aImportChangeset)
{
  return sbBaseDevice::ImportFromDevice(aImportToLibrary, aImportChangeset);
}

