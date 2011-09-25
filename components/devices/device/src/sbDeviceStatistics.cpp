/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \file  sbDeviceStatistics.cpp
 * \brief Nightingale Device Statistics Source.
 */

//------------------------------------------------------------------------------
//
// Device statistics imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDeviceStatistics.h"

// Local imports.
#include "sbBaseDevice.h"

// Nightingale imports.
#include <sbIDeviceCapabilities.h>
#include <sbStandardProperties.h>

// Mozilla imports.
#include <prprf.h>


//------------------------------------------------------------------------------
//
// Device statistics nsISupports services.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceStatistics,
                              sbIMediaListEnumerationListener)


//------------------------------------------------------------------------------
//
// Device statistics sbIMediaListEnumerationListener services.
//
//------------------------------------------------------------------------------

/**
 * \brief Called when enumeration is about to begin.
 *
 * \param aMediaList - The media list that is being enumerated.
 *
 * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
 *         JavaScript callers may omit the return statement entirely to
 *         continue the enumeration.
 */

NS_IMETHODIMP
sbDeviceStatistics::OnEnumerationBegin(sbIMediaList* aMediaList,
                                       PRUint16*     _retval)
{
  // Continue enumerating.
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}


/**
 * \brief Called once for each item in the enumeration.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aMediaItem - The media item.
 *
 * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
 *         JavaScript callers may omit the return statement entirely to
 *         continue the enumeration.
 */

NS_IMETHODIMP
sbDeviceStatistics::OnEnumeratedItem(sbIMediaList* aMediaList,
                                     sbIMediaItem* aMediaItem,
                                     PRUint16*     _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  // Add the media item to the device statistics.
  AddItem(aMediaItem);

  // Continue enumerating.
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}


/**
 * \brief Called when enumeration has completed.
 *
 * \param aMediaList - The media list that is being enumerated.
 * \param aStatusCode - A code to determine if the enumeration was successful.
 */

NS_IMETHODIMP
sbDeviceStatistics::OnEnumerationEnd(sbIMediaList* aMediaList,
                                     nsresult      aStatusCode)
{
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public device statistics services.
//
//------------------------------------------------------------------------------

/**
 * Create and return in aDeviceStatistics a new device statistics instance for
 * the device specified by aDevice.
 *
 * \param aDevice               Device for which to create a device statistics
 *                              instance.
 * \param aDeviceStatistics     Returned created device statistics instance.
 */

/* static */ nsresult
sbDeviceStatistics::New(class sbBaseDevice*  aDevice,
                        sbDeviceStatistics** aDeviceStatistics)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aDeviceStatistics);

  // Function variables.
  nsresult rv;

  // Create and initialize a new device statistics instance.
  nsRefPtr<sbDeviceStatistics>
    deviceStatistics = new sbDeviceStatistics();
  NS_ENSURE_TRUE(deviceStatistics, NS_ERROR_OUT_OF_MEMORY);
  rv = deviceStatistics->Initialize(aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  deviceStatistics.forget(aDeviceStatistics);

  return NS_OK;
}


/**
 * Add the device library specified by aLibrary to the device statistics.
 *
 * \param aLibrary              Device library to add.
 */

nsresult
sbDeviceStatistics::AddLibrary(sbIDeviceLibrary* aLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Clear library statistics.
  rv = ClearLibraryStatistics(aLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Collect the media statistics from the library.
  rv = aLibrary->EnumerateAllItems(this,
                                   sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Remove the device library specified by aLibrary from the device statistics.
 *
 * \param aLibrary              Device library to remove.
 */

nsresult
sbDeviceStatistics::RemoveLibrary(sbIDeviceLibrary* aLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Clear library statistics.
  rv = ClearLibraryStatistics(aLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Add the media item specified by aMediaItem to the device statistics.
 *
 * \param aMediaItem            Media item to add.
 */

nsresult
sbDeviceStatistics::AddItem(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Update the statistics for the added item.
  rv = UpdateForItem(aMediaItem, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Remove the media item specified by aMediaItem to the device statistics.
 *
 * \param aMediaItem            Media item to remove.
 */

nsresult
sbDeviceStatistics::RemoveItem(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Update the statistics for the removed item.
  rv = UpdateForItem(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Remove all media items in the device library specified by aLibrary from the
 * device statistics.
 *
 * \param aLibrary              Library for which to remove item statistics.
 */

nsresult
sbDeviceStatistics::RemoveAllItems(sbIDeviceLibrary* aLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Function variables.
  nsresult rv;

  // Clear library statistics.
  rv = ClearLibraryStatistics(aLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Device statistics setter/getter services.
//
//------------------------------------------------------------------------------

//
// Audio count setter/getters.
//

PRUint32 sbDeviceStatistics::AudioCount()
{
  nsAutoLock autoStatLock(mStatLock);
  return mAudioCount;
}

void sbDeviceStatistics::SetAudioCount(PRUint32 aAudioCount)
{
  nsAutoLock autoStatLock(mStatLock);
  mAudioCount = aAudioCount;
}

void sbDeviceStatistics::AddAudioCount(PRInt32 aAddAudioCount)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt32 audioCount =
            PR_MAX(static_cast<PRInt32>(mAudioCount) + aAddAudioCount, 0);
  mAudioCount = audioCount;
}


//
// Audio used setter/getters.
//

PRUint64 sbDeviceStatistics::AudioUsed()
{
  nsAutoLock autoStatLock(mStatLock);
  return mAudioUsed;
}

void sbDeviceStatistics::SetAudioUsed(PRUint64 aAudioUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  mAudioUsed = aAudioUsed;
}

void sbDeviceStatistics::AddAudioUsed(PRInt64 aAddAudioUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt64 audioUsed =
            PR_MAX(static_cast<PRInt64>(mAudioUsed) + aAddAudioUsed, 0);
  mAudioUsed = audioUsed;
}


//
// Audio play time setter/getters.
//

PRUint64 sbDeviceStatistics::AudioPlayTime()
{
  nsAutoLock autoStatLock(mStatLock);
  return mAudioPlayTime;
}

void sbDeviceStatistics::SetAudioPlayTime(PRUint64 aAudioPlayTime)
{
  nsAutoLock autoStatLock(mStatLock);
  mAudioPlayTime = aAudioPlayTime;
}

void sbDeviceStatistics::AddAudioPlayTime(PRInt64 aAddAudioPlayTime)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt64 audioPlayTime =
            PR_MAX(static_cast<PRInt64>(mAudioPlayTime) + aAddAudioPlayTime, 0);
  mAudioPlayTime = audioPlayTime;
}


//
// Video count setter/getters.
//

PRUint32 sbDeviceStatistics::VideoCount()
{
  nsAutoLock autoStatLock(mStatLock);
  return mVideoCount;
}

void sbDeviceStatistics::SetVideoCount(PRUint32 aVideoCount)
{
  nsAutoLock autoStatLock(mStatLock);
  mVideoCount = aVideoCount;
}

void sbDeviceStatistics::AddVideoCount(PRInt32 aAddVideoCount)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt32 videoCount =
            PR_MAX(static_cast<PRInt32>(mVideoCount) + aAddVideoCount, 0);
  mVideoCount = videoCount;
}


//
// Video used setter/getters.
//

PRUint64 sbDeviceStatistics::VideoUsed()
{
  nsAutoLock autoStatLock(mStatLock);
  return mVideoUsed;
}

void sbDeviceStatistics::SetVideoUsed(PRUint64 aVideoUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  mVideoUsed = aVideoUsed;
}

void sbDeviceStatistics::AddVideoUsed(PRInt64 aAddVideoUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt64 videoUsed =
            PR_MAX(static_cast<PRInt64>(mVideoUsed) + aAddVideoUsed, 0);
  mVideoUsed = videoUsed;
}


//
// Video play time setter/getters.
//

PRUint64 sbDeviceStatistics::VideoPlayTime()
{
  nsAutoLock autoStatLock(mStatLock);
  return mVideoPlayTime;
}

void sbDeviceStatistics::SetVideoPlayTime(PRUint64 aVideoPlayTime)
{
  nsAutoLock autoStatLock(mStatLock);
  mVideoPlayTime = aVideoPlayTime;
}

void sbDeviceStatistics::AddVideoPlayTime(PRInt64 aAddVideoPlayTime)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt64 videoPlayTime =
            PR_MAX(static_cast<PRInt64>(mVideoPlayTime) + aAddVideoPlayTime, 0);
  mVideoPlayTime = videoPlayTime;
}


//
// Image count setter/getters.
//

PRUint32 sbDeviceStatistics::ImageCount()
{
  nsAutoLock autoStatLock(mStatLock);
  return mImageCount;
}

void sbDeviceStatistics::SetImageCount(PRUint32 aImageCount)
{
  nsAutoLock autoStatLock(mStatLock);
  mImageCount = aImageCount;
}

void sbDeviceStatistics::AddImageCount(PRInt32 aAddImageCount)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt32 ImageCount =
            PR_MAX(static_cast<PRInt32>(mImageCount) + aAddImageCount, 0);
  mImageCount = ImageCount;
}


//
// Image used setter/getters.
//

PRUint64 sbDeviceStatistics::ImageUsed()
{
  nsAutoLock autoStatLock(mStatLock);
  return mImageUsed;
}

void sbDeviceStatistics::SetImageUsed(PRUint64 aImageUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  mImageUsed = aImageUsed;
}

void sbDeviceStatistics::AddImageUsed(PRInt64 aAddImageUsed)
{
  nsAutoLock autoStatLock(mStatLock);
  PRInt64 ImageUsed =
            PR_MAX(static_cast<PRInt64>(mImageUsed) + aAddImageUsed, 0);
  mImageUsed = ImageUsed;
}


//------------------------------------------------------------------------------
//
// Private device statistics services.
//
//------------------------------------------------------------------------------

/**
 * Construct a device statistics instance.
 */

sbDeviceStatistics::sbDeviceStatistics() :
  mBaseDevice(nsnull),
  mStatLock(nsnull),
  mAudioCount(0),
  mAudioUsed(0),
  mAudioPlayTime(0),
  mVideoCount(0),
  mVideoUsed(0),
  mVideoPlayTime(0),
  mImageCount(0),
  mImageUsed(0)
{
}


/**
 * Destroy a device statistics instance.
 */

sbDeviceStatistics::~sbDeviceStatistics()
{
  // Dispose of the statistics lock.
  if (mStatLock)
    nsAutoLock::DestroyLock(mStatLock);
  mStatLock = nsnull;
}


/**
 * Initialize the device statistics for the device specified by aDevice.
 *
 * \param aDevice               Device for which to initialize device
 *                              statistics.
 */

nsresult
sbDeviceStatistics::Initialize(class sbBaseDevice* aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);

  // Get the device.
  mBaseDevice = aDevice;

  // Create the statistics lock.
  mStatLock = nsAutoLock::NewLock(__FILE__"::mStatLock");
  NS_ENSURE_TRUE(mStatLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}


/**
 * Clear the statistics for the device library specified by aLibrary.
 *
 * \param aLibrary              Device library for which to clear statistics.
 */

nsresult
sbDeviceStatistics::ClearLibraryStatistics(sbIDeviceLibrary* aLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLibrary);

  // Clear library statistics.
  mAudioCount = 0;
  mAudioUsed = 0;
  mAudioPlayTime = 0;
  mVideoCount = 0;
  mVideoUsed = 0;
  mVideoPlayTime = 0;
  mImageCount = 0;
  mImageUsed = 0;

  return NS_OK;
}


/**
 * Update the device statistics for the item specified by aMediaItem.  If
 * aItemAdded is true, the item has been added; otherwise, the item is being
 * removed.
 *
 * \param aMediaItem            Media item for which to update device
 *                              statistics.
 * \param aItemAdded            If true, item has been added.
 */

nsresult
sbDeviceStatistics::UpdateForItem(sbIMediaItem* aMediaItem,
                                  PRBool        aItemAdded)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Ignore media lists.
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aMediaItem, &rv);
  if (NS_SUCCEEDED(rv))
    return NS_OK;

  // Get the item content type.
  PRUint32 contentType;
  rv = mBaseDevice->GetItemContentType(aMediaItem, &contentType);
  if (NS_FAILED(rv)) {
    contentType = sbIDeviceCapabilities::CONTENT_UNKNOWN;
  }

  // Get the item count update value.
  PRInt32 itemCountUpdate;
  if (aItemAdded)
    itemCountUpdate = 1;
  else
    itemCountUpdate = -1;

  // Get the item used update value.
  PRInt64 itemUsedUpdate;
  rv = aMediaItem->GetContentLength(&itemUsedUpdate);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!aItemAdded)
    itemUsedUpdate = -itemUsedUpdate;

  // Get the item play time update value.
  PRInt64      itemPlayTimeUpdate = 0;
  nsAutoString duration;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                               duration);
  if (NS_SUCCEEDED(rv)) {
    if (PR_sscanf(NS_ConvertUTF16toUTF8(duration).get(),
                  "%lld",
                  &itemPlayTimeUpdate) == 0) {
      itemPlayTimeUpdate = 0;
    }
  }
  if (!aItemAdded)
    itemPlayTimeUpdate = -itemPlayTimeUpdate;

  // Update appropriate statistics depending upon the content type.
  // Statistics for other content types will be collected together elsewhere
  // in the total other statistics.
  if (contentType == sbIDeviceCapabilities::CONTENT_AUDIO) {
    AddAudioCount(itemCountUpdate);
    AddAudioUsed(itemUsedUpdate);
    AddAudioPlayTime(itemPlayTimeUpdate);
  } else if (contentType == sbIDeviceCapabilities::CONTENT_VIDEO) {
    AddVideoCount(itemCountUpdate);
    AddVideoUsed(itemUsedUpdate);
    AddVideoPlayTime(itemPlayTimeUpdate);
  }

  return NS_OK;
}


