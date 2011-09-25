/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file  sbWindowsFormatter.cpp
 * \brief Nightingale Windows Formatter Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows formatter imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowsFormatter.h"

// Local imports.
#include "sbIWindowsFormatter_i.c"

// Nightingale imports.
#include <sbWindowsStorage.h>
#include <sbWindowsUtils.h>


//------------------------------------------------------------------------------
//
// Nightingale Windows formatter IUnknown implementation.
//
//------------------------------------------------------------------------------

/**
 * Add a reference to this object and return the resulting reference count.
 *
 * \return                      New reference count.
 */

STDMETHODIMP_(ULONG)
sbWindowsFormatter::AddRef()
{
  // Increment the reference count and return the result.
  return ++mRefCount;
}


/**
 * Remove a reference to this object and return the resulting reference count.
 * Delete this object if the reference count reaches 0.
 *
 * \return                      New reference count.
 */

STDMETHODIMP_(ULONG)
sbWindowsFormatter::Release()
{
  // Decrement the reference count.
  ULONG refCount = --mRefCount;

  // Delete self if reference count reaches zero.
  if (mRefCount == 0)
    delete this;

  return refCount;
}


/**
 * Return in aInterface an implementation of this object with the interface
 * specified by aIID.  If this object does not support the specified interface,
 * return E_NOINTERFACE.
 *
 * \param aIID                  Requested interface.
 * \param aInterface            Returned interface object.
 *
 * \return E_NOINTERFACE        Requested interface is not supported.
 */

STDMETHODIMP
sbWindowsFormatter::QueryInterface(REFIID aIID,
                                   void** aInterface)
{
  // Validate arguments.
  if (IsBadWritePtr(aInterface, sizeof(void*)))
    return E_POINTER;

  // Initialize interface to null.
  *aInterface = NULL;

  // Return requested interface.
  if (InlineIsEqualGUID(aIID, IID_IUnknown))
    *aInterface = static_cast<IUnknown*>(this);
  else if (InlineIsEqualGUID(aIID, IID_sbIWindowsFormatter))
    *aInterface = static_cast<sbIWindowsFormatter*>(this);
  else
    return E_NOINTERFACE;

  // Add a reference to the interface.
  (static_cast<IUnknown*>(*aInterface))->AddRef();

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Nightingale Windows formatter sbIWindowsFormat implementation.
//
//------------------------------------------------------------------------------

/**
 *   Format the volume with the volume name specified by aVolumeName.  The
 * volume name is the volume GUID path without the trainling slash.
 *   The formatted file system type is specified by aType.  The volume label
 * is specified by aLabel.  The file system unit allocation size is specified
 * by aUnitAllocationSize; a value of 0 specifies that a default value should
 * be used.
 *   If aForce is true, the file system is formatted even if the volume is in
 * use.  If aQuickFormat is true, a quick format is performed.  If
 * aEnableCompression is true, compression is enabled for NTFS file systems.
 *
 * \param aVolumeName         Name of volume to format.
 * \param aType               Type of file system to use.
 * \param aLabel              Volume label.
 * \param aUnitAllocationSize File system unit allocation size.
 * \param aForce              If true, force format even if volume is in use.
 * \param aQuickFormat        If true, perform a quick format.
 * \param aEnableCompression  If true, enable compression for NTFS file
 *                            systems.
 */

STDMETHODIMP
sbWindowsFormatter::Format(BSTR          aVolumeName,
                           unsigned long aType,
                           BSTR          aLabel,
                           DWORD         aUnitAllocationSize,
                           long          aForce,
                           long          aQuickFormat,
                           long          aEnableCompression)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aVolumeName);

  // Log format request.
  sbWindowsEventLog("Formatting volume: %S\n", aVolumeName);

  // Function variables.
  HRESULT result;

  // Get the VDS volume from the volume name and set it up for auto-disposal.
  IVdsVolumeMF* volume;
  result = GetVolume(aVolumeName, &volume);
  SB_WIN_ENSURE_SUCCESS(result, result);
  SB_WIN_ENSURE_TRUE(result == S_OK, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
  sbAutoIUnknown autoVolume(volume);

  // Format the volume.
  IVdsAsync* async;
  result = volume->Format(static_cast<VDS_FILE_SYSTEM_TYPE>(aType),
                          aLabel,
                          aUnitAllocationSize,
                          aForce,
                          aQuickFormat,
                          aEnableCompression,
                          &async);
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoAsync(async);

  // Wait for the format to complete.
  VDS_ASYNC_OUTPUT asyncOutput;
  HRESULT          formatResult;
  result = async->Wait(&formatResult, &asyncOutput);
  SB_WIN_ENSURE_SUCCESS(result, result);
  SB_WIN_ENSURE_SUCCESS(formatResult, formatResult);

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Public Nightingale Windows formatter services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// New
//

/* static */ HRESULT
sbWindowsFormatter::New(sbWindowsFormatter** aWindowsFormatter)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aWindowsFormatter);

  // Function variables.
  HRESULT result;

  // Create the Windows formatter object and set it up for auto-disposal.
  sbWindowsFormatter* windowsFormatter = new sbWindowsFormatter();
  SB_WIN_ENSURE_TRUE(windowsFormatter, E_OUTOFMEMORY);
  windowsFormatter->AddRef();
  sbAutoIUnknown autoWindowsFormatter(windowsFormatter);

  // Initialize the Windows formatter.
  result = windowsFormatter->Initialize();
  SB_WIN_ENSURE_SUCCESS(result, result);

  // Return results.
  *aWindowsFormatter = windowsFormatter;
  autoWindowsFormatter.forget();

  return S_OK;
}


//-------------------------------------
//
// ~sbWindowsFormatter
//

sbWindowsFormatter::~sbWindowsFormatter()
{
  // Release the VDS service object.
  if (mVdsService)
    mVdsService->Release();

  // Uninitialize COM.
  CoUninitialize();
}


//------------------------------------------------------------------------------
//
// Private Nightingale Windows formatter services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWindowsFormatter
//

sbWindowsFormatter::sbWindowsFormatter() :
  mRefCount(0),
  mVdsService(NULL)
{
}


//-------------------------------------
//
// GetVolume
//

HRESULT
sbWindowsFormatter::Initialize()
{
  HRESULT result;

  // Initialize COM.
  result = CoInitialize(NULL);
  SB_WIN_ENSURE_SUCCESS(result, result);

  // Get the VDS service loader object and set it up for auto-disposal.
  IVdsServiceLoader* vdsServiceLoader;
  result = CoCreateInstance(CLSID_VdsLoader,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            IID_IVdsServiceLoader,
                            reinterpret_cast<void**>(&vdsServiceLoader));
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoVdsServiceLoader(vdsServiceLoader);

  // Load the VDS service.
  result = vdsServiceLoader->LoadService(NULL, &mVdsService);
  SB_WIN_ENSURE_SUCCESS(result, result);
  result = mVdsService->WaitForServiceReady();
  SB_WIN_ENSURE_SUCCESS(result, result);

  return S_OK;
}


//-------------------------------------
//
// GetVolume
//

HRESULT
sbWindowsFormatter::GetVolume(BSTR           aVolumeName,
                              IVdsVolumeMF** aVolume)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aVolumeName);
  SB_WIN_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  HRESULT result;

  // Return a null volume by default.
  *aVolume = NULL;

  // Get the list of software providers and set it up for auto-disposal.
  IEnumVdsObject* swProviderEnum;
  result = mVdsService->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS,
                                       &swProviderEnum);
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoSwProviderEnum(swProviderEnum);

  // Search each software provider for the volume.  Enumerate all software
  // providers until no more remain or until the volume is found.
  while (1) {
    // Get the next software provider and set it up for auto-disposal.  The
    // returned count will be zero when the enumeration is complete.
    IUnknown* swProviderIUnknown;
    ULONG     count;
    result = swProviderEnum->Next(1, &swProviderIUnknown, &count);
    SB_WIN_ENSURE_SUCCESS(result, result);
    if (!count)
      break;
    sbAutoIUnknown autoSwProviderIUnknown(swProviderIUnknown);

    // QI the software provider and set it up for auto-disposal.
    IVdsSwProvider* swProvider;
    result = swProviderIUnknown->QueryInterface
                                   (IID_IVdsSwProvider,
                                    reinterpret_cast<void**>(&swProvider));
    SB_WIN_ENSURE_SUCCESS(result, result);
    sbAutoIUnknown autoSwProvider(swProvider);

    // Try getting the volume from the software provider.  Return volume if
    // found.
    result = GetVolume(swProvider, aVolumeName, aVolume);
    SB_WIN_ENSURE_SUCCESS(result, result);
    if (result == S_OK)
      return S_OK;
  }

  return S_FALSE;
}


//-------------------------------------
//
// GetVolume
//

HRESULT
sbWindowsFormatter::GetVolume(IVdsSwProvider* aSwProvider,
                              BSTR            aVolumeName,
                              IVdsVolumeMF**  aVolume)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aSwProvider);
  SB_WIN_ENSURE_ARG_POINTER(aVolumeName);
  SB_WIN_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  HRESULT result;

  // Return a null volume by default.
  *aVolume = NULL;

  // Get the list of VDS packs and set it up for auto-disposal.
  IEnumVdsObject* packEnum;
  result = aSwProvider->QueryPacks(&packEnum);
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoPackEnum(packEnum);

  // Search each pack for the volume.  Enumerate all packs until no more remain
  // or until the volume is found.
  while (1) {
    // Get the next pack and set it up for auto-disposal.  The returned count
    // will be zero when the enumeration is complete.
    IUnknown* packIUnknown;
    ULONG     count;
    result = packEnum->Next(1, &packIUnknown, &count);
    SB_WIN_ENSURE_SUCCESS(result, result);
    if (!count)
      break;
    sbAutoIUnknown autoPackIUnknown(packIUnknown);

    // QI the pack and set it up for auto-disposal.
    IVdsPack* pack;
    result = packIUnknown->QueryInterface(IID_IVdsPack,
                                          reinterpret_cast<void**>(&pack));
    SB_WIN_ENSURE_SUCCESS(result, result);
    sbAutoIUnknown autoPack(pack);

    // Try getting the volume from the pack.  Return volume if found.
    result = GetVolume(pack, aVolumeName, aVolume);
    SB_WIN_ENSURE_SUCCESS(result, result);
    if (result == S_OK)
      return S_OK;
  }

  return S_FALSE;
}


//-------------------------------------
//
// GetVolume
//

HRESULT
sbWindowsFormatter::GetVolume(IVdsPack*      aPack,
                              BSTR           aVolumeName,
                              IVdsVolumeMF** aVolume)
{
  // Validate arguments.
  SB_WIN_ENSURE_ARG_POINTER(aPack);
  SB_WIN_ENSURE_ARG_POINTER(aVolumeName);
  SB_WIN_ENSURE_ARG_POINTER(aVolume);

  // Function variables.
  HRESULT result;

  // Return a null volume by default.
  *aVolume = NULL;

  // Get the target volume storage device number.
  STORAGE_DEVICE_NUMBER targetDevNum;
  result = sbWinGetStorageDevNum(aVolumeName, &targetDevNum);
  SB_WIN_ENSURE_SUCCESS(result, result);

  // Get the list of VDS pack volumes and set it up for auto-disposal.
  IEnumVdsObject* volumeEnum;
  result = aPack->QueryVolumes(&volumeEnum);
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoVolumeEnum(volumeEnum);

  // Search for the volume.  Enumerate all volumes until no more remain or until
  // the target volume is found.
  while (1) {
    // Get the next volume and set it up for auto-disposal.  The returned count
    // will be zero when the enumeration is complete.
    IUnknown* volumeIUnknown;
    ULONG     count;
    result = volumeEnum->Next(1, &volumeIUnknown, &count);
    SB_WIN_ENSURE_SUCCESS(result, result);
    if (!count)
      break;
    sbAutoIUnknown autoVolumeIUnknown(volumeIUnknown);

    // QI the volume and set it up for auto-disposal.
    IVdsVolume* volume;
    result = volumeIUnknown->QueryInterface(IID_IVdsVolume,
                                            reinterpret_cast<void**>(&volume));
    SB_WIN_ENSURE_SUCCESS(result, result);
    sbAutoIUnknown autoVolume(volume);

    // Get the volume properties.
    VDS_VOLUME_PROP volumeProp;
    result = volume->GetProperties(&volumeProp);
    SB_WIN_ENSURE_SUCCESS(result, result);

    // Get the volume storage device number.
    STORAGE_DEVICE_NUMBER volumeDevNum;
    result = sbWinGetStorageDevNum(volumeProp.pwszName, &volumeDevNum);
    SB_WIN_ENSURE_SUCCESS(result, result);

    // Return volume if it matches the target.
    if ((volumeDevNum.DeviceType == targetDevNum.DeviceType) &&
        (volumeDevNum.DeviceNumber == targetDevNum.DeviceNumber) &&
        (volumeDevNum.PartitionNumber == targetDevNum.PartitionNumber)) {
      result = volumeIUnknown->QueryInterface
                                 (IID_IVdsVolumeMF,
                                  reinterpret_cast<void**>(aVolume));
      SB_WIN_ENSURE_SUCCESS(result, result);
      return S_OK;
    }
  }

  return S_FALSE;
}

