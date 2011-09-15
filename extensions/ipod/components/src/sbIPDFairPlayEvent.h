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

#ifndef __SB_IPD_FAIRPLAYEVENT_H__
#define __SB_IPD_FAIRPLAYEVENT_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device FairPlay event defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDFairPlayEvent.h
 * \brief Songbird iPod Device FairPlay Event Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device FairPlay event imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIIPDDeviceEvent.h"

// Songbird imports.
#include <sbDeviceEvent.h>
#include <sbIDevice.h>
#include <sbIMediaItem.h>

// Mozilla imports.
#include <nsStringAPI.h>


//------------------------------------------------------------------------------
//
// iPod device FairPlay event classes.
//
//------------------------------------------------------------------------------

/**
 * This class represents an iPod device FairPlay event.
 */

class sbIPDFairPlayEvent : public sbIIPDFairPlayEvent,
                           public sbDeviceEvent
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // Interface declarations.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIIPDFAIRPLAYEVENT
  NS_DECL_SBIIPDDEVICEEVENT
  NS_FORWARD_SAFE_SBIDEVICEEVENT(mDeviceEvent)
  NS_FORWARD_SAFE_SBDEVICEEVENT(mSBDeviceEvent)


  //
  // Public services.
  //

  static nsresult CreateEvent(sbIPDFairPlayEvent** aFairPlayEvent,
                              sbIDevice*           aDevice,
                              PRUint32             aType,
                              PRUint32             aUserID,
                              nsAString&           aAccountName,
                              nsAString&           aUserName,
                              sbIMediaItem*        aMediaItem);


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  //
  // Protect constructor/destructor.
  //

  sbIPDFairPlayEvent();

  virtual ~sbIPDFairPlayEvent();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mDeviceEvent               Base device event.
  // mSBDeviceEvent             QI'd mDeviceEvent.
  // mUserID                    FairPlay user ID.
  // mAccountName               FairPlay account name.
  // mUserName                  FairPlay user name.
  // mMediaItem                 FairPlay media item.
  //

  nsCOMPtr<sbIDeviceEvent>      mDeviceEvent;
  nsCOMPtr<sbDeviceEvent>       mSBDeviceEvent;
  PRUint32                      mUserID;
  nsString                      mAccountName;
  nsString                      mUserName;
  nsCOMPtr<sbIMediaItem>        mMediaItem;


  //
  // Internal services.
  //

  nsresult InitEvent(sbIDevice*    aDevice,
                     PRUint32      aType,
                     PRUint32      aUserID,
                     nsAString&    aAccountName,
                     nsAString&    aUserName,
                     sbIMediaItem* aMediaItem);
};


#endif // __SB_IPD_FAIRPLAYEVENT_H__

