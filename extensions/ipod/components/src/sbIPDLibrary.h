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

#ifndef __SB_IPD_LIBRARY_H__
#define __SB_IPD_LIBRARY_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device library defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDLibrary.h
 * \brief Songbird iPod Device Library Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device library imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbDeviceLibrary.h>

// Mozilla imports.
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// iPod device library classes.
//
//------------------------------------------------------------------------------

/**
 * This class represents an iPod device library.
 */

class sbIPDDevice;

class sbIPDLibrary : public sbDeviceLibrary
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // sbIDeviceLibrary overrides.
  //

  NS_SCRIPTABLE NS_IMETHOD GetMgmtType(PRUint32 *aMgmtType);
  NS_SCRIPTABLE NS_IMETHOD SetMgmtType(PRUint32 aMgmtType);
  NS_SCRIPTABLE NS_IMETHOD GetSyncPlaylistList(nsIArray **_retval);
  NS_SCRIPTABLE NS_IMETHOD SetSyncPlaylistListByType(PRUint32 aContentType,
                                                     nsIArray *aPlaylistList);
  NS_SCRIPTABLE NS_IMETHOD AddToSyncPlaylistList(sbIMediaList *aPlaylist);


  //
  // Constructors/destructors.
  //

  sbIPDLibrary(sbIPDDevice* aDevice);

  virtual ~sbIPDLibrary();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mDevice                    Device to which this library belongs.
  // mPrefsInitialized          True if the library preferences have been
  //                            initialized.
  //

  nsRefPtr<sbIPDDevice>         mDevice;
  PRBool                        mPrefsInitialized;


  //
  // Internal services.
  //

  nsresult InitializePrefs();
};


#endif // __SB_IPD_LIBRARY_H__

