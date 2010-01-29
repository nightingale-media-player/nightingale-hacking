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
sbIPDLibrary::GetSyncPlaylistList(nsIArray** _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Ensure the library preferences are initialized.
  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to the base device library.
  rv = sbDeviceLibrary::GetSyncPlaylistList(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Set the list of playlists the user wants to sync from the main
 *        library to the device
 *
 * \param aPlaylistList List of sbIMediaLists that represent the playlists to
 *                      sync.
 */

NS_IMETHODIMP
sbIPDLibrary::SetSyncPlaylistListByType(PRUint32 aContentType,
                                        nsIArray* aPlaylistList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPlaylistList);

  // Function variables.
  nsresult rv;

  // Ensure the library preferences are initialized.
  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to the base device library.
  rv = sbDeviceLibrary::SetSyncPlaylistListByType(aContentType, aPlaylistList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to iPod device object.
  rv = mDevice->SetSyncPlaylistList(aPlaylistList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Add a playlist to the list of playlists the user wants to sync from
 *        the main library to the device
 *
 * \param aPlaylist The playlist to add.
 */

NS_IMETHODIMP
sbIPDLibrary::AddToSyncPlaylistList(sbIMediaList* aPlaylist)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPlaylist);

  // Function variables.
  nsresult rv;

  // Ensure the library preferences are initialized.
  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 contentType;
  rv = aPlaylist->GetListContentType(&contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to the base device library.
  rv = sbDeviceLibrary::AddToSyncPlaylistList(contentType, aPlaylist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to iPod device object.
  rv = mDevice->AddToSyncPlaylistList(aPlaylist);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//
// Getters/setters.
//

/**
 * \brief The currently configured device management type preference
 * for the device library.
 *
 * \sa MGMT_TYPE_* constants
 */

NS_IMETHODIMP
sbIPDLibrary::GetMgmtType(PRUint32* aMgmtType)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMgmtType);

  // Function variables.
  nsresult rv;

  // Ensure the library preferences are initialized.
  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to the base device library.
  rv = sbDeviceLibrary::GetMgmtType(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                    aMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbIPDLibrary::SetMgmtType(PRUint32 aMgmtType)
{
  nsresult rv;

  // Ensure the library preferences are initialized.
  rv = InitializePrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to the base device library.
  rv = sbDeviceLibrary::SetMgmtType(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                    aMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Forward to iPod device object.
  rv = mDevice->SetMgmtType(aMgmtType);
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
  rv = SetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_AUDIO, mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the sync playlist list preference.
  nsCOMPtr<nsIArray> syncPlaylistList;
  rv = mDevice->GetSyncPlaylistList(getter_AddRefs(syncPlaylistList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetSyncPlaylistListPref(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                               syncPlaylistList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Preferences are now initialized.
  mPrefsInitialized = PR_TRUE;

  return NS_OK;
}

