/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
 * \file  sbBaseDeviceVolume.cpp
 * \brief Nightingale Base Device Volume Source.
 */

//------------------------------------------------------------------------------
//
// Base device volume imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbBaseDeviceVolume.h"

// Local imports.
#include "sbBaseDevice.h"


//------------------------------------------------------------------------------
//
// Base device volume nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(sbBaseDeviceVolume)


//------------------------------------------------------------------------------
//
// Public base device volume services.
//
//------------------------------------------------------------------------------

/* static */ nsresult
sbBaseDeviceVolume::New(sbBaseDeviceVolume** aVolume,
                        sbBaseDevice*        aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aVolume);
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  nsresult rv;

  // Create and initialize a new base device volume instance.
  nsRefPtr<sbBaseDeviceVolume> volume = new sbBaseDeviceVolume();
  NS_ENSURE_TRUE(volume, NS_ERROR_OUT_OF_MEMORY);
  rv = volume->Initialize(aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  volume.forget(aVolume);

  return NS_OK;
}

sbBaseDeviceVolume::~sbBaseDeviceVolume()
{
  // Dispose of the volume lock.
  if (mVolumeLock)
    nsAutoLock::DestroyLock(mVolumeLock);
  mVolumeLock = nsnull;
}


//
// Getters/setters.
//

nsresult
sbBaseDeviceVolume::GetGUID(nsAString& aGUID)
{
  nsAutoLock autoVolumeLock(mVolumeLock);
  aGUID.Assign(mGUID);
  return NS_OK;
}

nsresult
sbBaseDeviceVolume::SetGUID(const nsAString& aGUID)
{
  nsAutoLock autoVolumeLock(mVolumeLock);
  mGUID.Assign(aGUID);
  return NS_OK;
}


nsresult
sbBaseDeviceVolume::GetIsMounted(PRBool* aIsMounted)
{
  NS_ENSURE_ARG_POINTER(aIsMounted);
  nsAutoLock autoVolumeLock(mVolumeLock);
  *aIsMounted = mIsMounted;
  return NS_OK;
}

nsresult
sbBaseDeviceVolume::SetIsMounted(PRBool aIsMounted)
{
  nsAutoLock autoVolumeLock(mVolumeLock);
  mIsMounted = aIsMounted;
  return NS_OK;
}


nsresult
sbBaseDeviceVolume::GetRemovable(PRInt32* aRemovable)
{
  NS_ENSURE_ARG_POINTER(aRemovable);
  nsAutoLock autoVolumeLock(mVolumeLock);
  *aRemovable = mRemovable;
  return NS_OK;
}

nsresult
sbBaseDeviceVolume::SetRemovable(PRInt32 aRemovable)
{
  nsAutoLock autoVolumeLock(mVolumeLock);
  mRemovable = aRemovable;
  return NS_OK;
}


nsresult
sbBaseDeviceVolume::GetDeviceLibrary(sbIDeviceLibrary** aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDeviceLibrary);
  nsAutoLock autoVolumeLock(mVolumeLock);
  NS_IF_ADDREF(*aDeviceLibrary = mDeviceLibrary);
  return NS_OK;
}

nsresult
sbBaseDeviceVolume::SetDeviceLibrary(sbIDeviceLibrary* aDeviceLibrary)
{
  nsresult rv;

  // Get the current library and its GUID.
  nsCOMPtr<sbIDeviceLibrary> currentLibrary;
  nsAutoString               currentLibraryGUID;
  {
    nsAutoLock autoVolumeLock(mVolumeLock);
    currentLibrary = mDeviceLibrary;
  }
  if (currentLibrary) {
    rv = currentLibrary->GetGuid(currentLibraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Remove current device library.
  if (currentLibrary) {
    {
      nsAutoLock autoVolumeLock(mDevice->mVolumeLock);
      mDevice->mVolumeLibraryGUIDTable.Remove(currentLibraryGUID);
    }
    {
      nsAutoLock autoVolumeLock(mVolumeLock);
      mDeviceLibrary = nsnull;
    }
  }

  // Set new device library.
  if (aDeviceLibrary) {
    nsAutoString libraryGUID;
    rv = aDeviceLibrary->GetGuid(libraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);
    {
      nsAutoLock autoVolumeLock(mDevice->mVolumeLock);
      NS_ENSURE_TRUE(mDevice->mVolumeLibraryGUIDTable.Put(libraryGUID, this),
                     NS_ERROR_OUT_OF_MEMORY);
    }
    {
      nsAutoLock autoVolumeLock(mVolumeLock);
      mDeviceLibrary = aDeviceLibrary;
    }
  }

  return NS_OK;
}


nsresult
sbBaseDeviceVolume::GetStatistics(sbDeviceStatistics** aStatistics)
{
  NS_ENSURE_ARG_POINTER(aStatistics);
  nsAutoLock autoVolumeLock(mVolumeLock);
  NS_ENSURE_STATE(aStatistics);
  NS_ADDREF(*aStatistics = mStatistics);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Protected base device volume services.
//
//------------------------------------------------------------------------------

nsresult
sbBaseDeviceVolume::Initialize(sbBaseDevice* aDevice)
{
  nsresult rv;

  // Create the volume lock.
  mVolumeLock = nsAutoLock::NewLock("sbBaseDeviceVolume::mVolumeLock");
  NS_ENSURE_TRUE(mVolumeLock, NS_ERROR_OUT_OF_MEMORY);

  // Set the volume device.
  mDevice = aDevice;

  // Get a device statistics instance.
  rv = sbDeviceStatistics::New(aDevice, getter_AddRefs(mStatistics));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


sbBaseDeviceVolume::sbBaseDeviceVolume() :
  mVolumeLock(nsnull),
  mIsMounted(PR_FALSE),
  mRemovable(-1)
{
}

