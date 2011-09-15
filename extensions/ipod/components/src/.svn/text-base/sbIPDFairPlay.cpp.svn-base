/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
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

//------------------------------------------------------------------------------
//
// iPod device FairPlay services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * \file  sbIPDFairPlay.cpp
 * \brief Songbird iPod Device FairPlay Source.
 */

//------------------------------------------------------------------------------
//
// iPod device FairPlay imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "QTAtomReader.h"
#include "sbIPDDevice.h"
#include "sbIIPDDeviceEvent.h"
#include "sbIPDLog.h"

// Songbird imports.
#include <sbDeviceUtils.h>
#include <sbStringUtils.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsILocalFile.h>


//------------------------------------------------------------------------------
//
// iPod device FairPlay services.
//
//------------------------------------------------------------------------------

/**
 * Connect the FairPlay services.
 *
 * Since this function is called as a part of the device connection process, it
 * does not need to operate under the connect lock.  Also, the caller does not
 * need to hold the request lock.
 */

nsresult
sbIPDDevice::FPConnect()
{
  nsresult rv;

  // Operate under the request lock.
  nsAutoMonitor autoRequestLock(mDBLock);

  // Get the FairPlay key information.
  rv = FPGetKeyInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Disconnect the FairPlay services.
 *
 * Since this function is called as a part of the device connection process, it
 * does not need to operate under the connect lock.  Also, the caller does not
 * need to hold the request lock.
 */

void
sbIPDDevice::FPDisconnect()
{
  // Operate under the request lock.
  nsAutoMonitor autoRequestLock(mDBLock);

  // Get the list of authorized FairPlay user IDs.
  mFPUserIDList.Clear();
}


/**
 * Check if the iPod is authorized by FairPlay to play the track specified by
 * pTrack.  If it is not, add the authorization information to the list of
 * unauthorized FairPlay accounts.
 *
 * \param aTrack                Track for which to check authorization.
 */

nsresult
sbIPDDevice::FPCheckTrackAuth(Itdb_Track* aTrack)
{
  // Validate parameters.
  NS_ASSERTION(aTrack, "aTrack is null");

  // Function variables.
  nsresult rv;

  // Do nothing if not a FairPlay track.
  if (!aTrack->unk148)
    return NS_OK;

  // Get the FairPlay user ID.
  PRUint32 userID = aTrack->drm_userid;

  // Check if the device is authorized for the user ID.
  PRBool authorized = PR_FALSE;
  for (PRUint32 i = 0; i < mFPUserIDList.Length(); i++) {
    if (mFPUserIDList[i] == userID) {
      authorized = PR_TRUE;
      break;
    }
  }

  // If not authorized, send an event.
  if (!authorized) {
    // Get the authorization info.
    PRUint32     userID;
    nsAutoString accountName;
    nsAutoString userName;
    rv = FPGetTrackAuthInfo(aTrack, &userID, accountName, userName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the Songbird main library media item for the track.  Ignore errors.
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = sbDeviceUtils::GetOriginMediaItemByDevicePersistentId
                          (mDeviceLibrary,
                           sbAutoString(aTrack->dbid),
                           getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Log event.
    FIELD_LOG(("Track not authorized 0x%08x %s %s\n",
               userID,
               NS_ConvertUTF16toUTF8(accountName).get(),
               NS_ConvertUTF16toUTF8(userName).get()));

    // Create and dispatch an event.
    CreateAndDispatchFairPlayEvent
      (sbIIPDDeviceEvent::EVENT_IPOD_FAIRPLAY_NOT_AUTHORIZED,
       userID,
       accountName,
       userName,
       mediaItem);
  }

  return NS_OK;
}


/**
 * Set up the iPod track specified by aTrack with any FairPlay information
 * within the track file specified by aTrackFile.  Check if the track file is
 * protected with FairPlay and do nothing if not.
 *
 * \param aTrackFile            Track file.
 * \param aTrack                iPod track to set up.
 */

nsresult
sbIPDDevice::FPSetupTrackInfo(nsIFile*    aTrackFile,
                              Itdb_Track* aTrack)
{
  // Validate parameters.
  NS_ASSERTION(aTrackFile, "aTrackFile is null");
  NS_ASSERTION(aTrack, "aTrack is null");

  // Function variables.
  nsresult rv;

  // Get the track file name extension.
  nsAutoString fileName;
  nsAutoString fileExt;
  rv = aTrackFile->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  fileExt.Assign(StringTail(fileName, 3));

  // Check for protected extensions.  Do nothing if not protected.
  if (!fileExt.Equals(NS_LITERAL_STRING("m4p")))
    return NS_OK;

  // Set up an atom reader and set up for auto-disposal.
  QTAtomReader* atomReader = new QTAtomReader();
  NS_ENSURE_TRUE(atomReader, NS_ERROR_OUT_OF_MEMORY);
  sbAutoQTAtomReader autoAtomReader(atomReader);
  rv = atomReader->Open(aTrackFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the FairPlay user ID.
  PRUint32 userID;
  rv = atomReader->GetFairPlayUserID(&userID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the track FairPlay fields.
  aTrack->drm_userid = userID;
  aTrack->unk148 = 0x01010100;
  aTrack->filetype = g_strdup("Protected AAC audio file");

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal iPod device FairPlay services.
//
//------------------------------------------------------------------------------

/**
 * Read the device FairPlay key info and save it in the authorized FairPlay user
 * ID list.
 *
 * Since this function is called as a part of the device connection process, it
 * does not need to operate under the connect lock.
 *XXXeps the iEKInfo file does not appear to be present for 3G Nanos.  Need to
 *XXXeps investigate further.
 */

nsresult
sbIPDDevice::FPGetKeyInfo()
{
  nsresult rv;

  // Get the iPod key info file.
  nsCOMPtr<nsILocalFile>
    iEKInfoFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = iEKInfoFile->InitWithPath(mMountPath);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = iEKInfoFile->Append(NS_LITERAL_STRING("iPod_Control"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = iEKInfoFile->Append(NS_LITERAL_STRING("iTunes"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = iEKInfoFile->Append(NS_LITERAL_STRING("iEKInfo"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for existence of the iPod key info file.  Do nothing more if it
  // doesn't exist.
  PRBool exists;
  rv = iEKInfoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    return NS_OK;

  // Set up an atom reader and set up for auto-disposal.
  QTAtomReader* atomReader = new QTAtomReader();
  NS_ENSURE_TRUE(atomReader, NS_ERROR_OUT_OF_MEMORY);
  sbAutoQTAtomReader autoAtomReader(atomReader);
  rv = atomReader->Open(iEKInfoFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of authorized FairPlay user IDs.
  rv = atomReader->GetIEKInfoUserIDs(mFPUserIDList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get and return in aUserID, aAccountName, and aUserName the FairPlay
 * authorization information for the track specified by aTrack
 *
 * \param aTrack                Track for which to get authorization info.
 * \param aUserID               FairPlay user ID.
 * \param aAccountName          FairPlay account name.
 * \param aUserName             FairPlay user name.
 */

nsresult
sbIPDDevice::FPGetTrackAuthInfo(Itdb_Track*      aTrack,
                                PRUint32*        aUserID,
                                nsAString&       aAccountName,
                                nsAString&       aUserName)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aUserID, "aUserID is null");

  // Function variables.
  nsresult rv;

  // Get the track file path and set up for auto-disposal.
  gchar* trackFilePath = itdb_filename_on_ipod(aTrack);
  NS_ENSURE_TRUE(trackFilePath, NS_ERROR_OUT_OF_MEMORY);
  sbAutoGMemPtr autoTrackFilePath(trackFilePath);

  // Get the track file.
  nsCOMPtr<nsILocalFile>
    trackFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = trackFile->InitWithPath(NS_ConvertUTF8toUTF16(trackFilePath));
  NS_ENSURE_SUCCESS(rv, rv);

  // Open an atom reader and set up for auto-disposal.
  QTAtomReader* atomReader = new QTAtomReader();
  NS_ENSURE_TRUE(atomReader, NS_ERROR_OUT_OF_MEMORY);
  sbAutoQTAtomReader autoAtomReader(atomReader);
  rv = atomReader->Open(trackFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the track authorization info.  Ignore errors getting the account and
  // user names.
  rv = atomReader->GetFairPlayUserID(aUserID);
  NS_ENSURE_SUCCESS(rv, rv);
  atomReader->GetFairPlayAccountName(aAccountName);
  atomReader->GetFairPlayUserName(aUserName);

  return NS_OK;
}

