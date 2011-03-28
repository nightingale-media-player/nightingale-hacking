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

/**
 * \file  sbIPDLibrary.cpp
 * \brief Songbird iPod Device Library Source.
 */

//------------------------------------------------------------------------------
//
// iPod device library imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbIPDLibrary.h"

// Local imports.
#include "sbIPDDevice.h"

#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncSettings.h>

//------------------------------------------------------------------------------
//
// iPod device library sbIDeviceLibrary implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Return the list of playlists the user wants to sync from the main
 *        library to the device.
 *
 * \return List of sbIMediaLists that represent the playlists to
 *         sync.
 */

NS_IMETHODIMP
sbIPDLibrary::GetSyncSettings(sbIDeviceLibrarySyncSettings ** aSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aSyncSettings);

  nsresult rv;

  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbDeviceLibrary::GetSyncSettings(aSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbIPDLibrary::SetSyncSettings(sbIDeviceLibrarySyncSettings * aSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aSyncSettings);

  nsresult rv;

  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbDeviceLibrary::SetSyncSettings(aSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbIPDLibrary::GetSyncFolderListByType(PRUint32 aContentType,
                                      nsIArray ** aFolderList)
{
  NS_ENSURE_ARG_POINTER(aFolderList);

  nsresult rv;

  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbDeviceLibrary::GetSyncFolderListByType(aContentType,
                                                aFolderList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbIPDLibrary::SetSyncFolderListByType(PRUint32 aContentType,
                                      nsIArray *aFolderList)
{
  NS_ENSURE_ARG_POINTER(aFolderList);

  nsresult rv;

  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbDeviceLibrary::SetSyncFolderListByType(aContentType,
                                                aFolderList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// iPod device library services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device library object.
 */

sbIPDLibrary::sbIPDLibrary(sbIPDDevice* aDevice) :
  sbDeviceLibrary(aDevice),
  mDevice(aDevice),
  mPrefsInitialized(PR_FALSE)
{
}


/**
 * Destroy an iPod device library object.
 */

sbIPDLibrary::~sbIPDLibrary()
{
}


//------------------------------------------------------------------------------
//
// Internal iPod device library services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the library preferences, ensuring they're set according to the
 * settings on the iPod.
 */

nsresult
sbIPDLibrary::InitializePrefs()
{
  nsresult rv;

  // Do nothing if preferences are already initialized.
  if (mPrefsInitialized)
    return NS_OK;

  // Set the device management type preference.
  PRUint32 mgmtType;
  rv = mDevice->GetMgmtType(&mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                      getter_AddRefs(mediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediaSyncSettings->SetMgmtType(mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the sync playlist list preference.
  nsCOMPtr<nsIArray> syncPlaylistList;
  rv = mDevice->GetSyncPlaylistList(getter_AddRefs(syncPlaylistList));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediaSyncSettings->SetSelectedPlaylists(syncPlaylistList);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetSyncSettings(syncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  // Preferences are now initialized.
  mPrefsInitialized = PR_TRUE;

  return NS_OK;
}

