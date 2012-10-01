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

#ifndef __SB_IPD_MARSHALL_H__
#define __SB_IPD_MARSHALL_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device marshall defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDMarshall.h
 * \brief Songbird iPod Device Marshall Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device marshall imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbBaseDeviceMarshall.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceRegistrar.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>

// MacOS X imports.
#include <Carbon/Carbon.h>

//------------------------------------------------------------------------------
//
// iPod device marshall definitions.
//
//------------------------------------------------------------------------------

//
// iPod device marshall XPCOM component definitions.
//XXXeps should move out of platform specific file
//

#define SB_IPDMARSHALL_CONTRACTID "@songbirdnest.com/Songbird/IPDMarshall;1"
#define SB_IPDMARSHALL_CLASSNAME "iPod Device Marshall"
#define SB_IPDMARSHALL_DESCRIPTION "iPod Device Marshall"
#define SB_IPDMARSHALL_CID                                                     \
{                                                                              \
  0xbbca0e8c,                                                                  \
  0x78a3,                                                                      \
  0x4f13,                                                                      \
  { 0xae, 0x2b, 0x0f, 0xed, 0xb9, 0x47, 0xdf, 0x37 }                           \
}


//------------------------------------------------------------------------------
//
// iPod device marshall classes.
//
//------------------------------------------------------------------------------

/**
 *   This class wraps Mac OS/X core foundation references, providing
 * auto-release support.
 */

template<class RefType>
class cfref {
  RefType mRef;
public:
  cfref() : mRef(NULL) {}
  cfref(RefType ref) : mRef(ref) {}
  ~cfref() { if(mRef) ::CFRelease(mRef); }

  RefType operator=(RefType ref)
  {
    if (mRef && mRef != ref)
      ::CFRelease(mRef);
    return (mRef = ref);
  }

  operator RefType() { return mRef; }
  operator int() { return (mRef != NULL); }
};


/**
 * This class communicates iPod device arrival and removal events to the
 * sbIDeviceManager service.
 */

class sbAutoFinalizeVolumeMonitor;

class sbIPDMarshall : public sbBaseDeviceMarshall,
                      public nsIClassInfo
{
  //----------------------------------------------------------------------------
  //
  // Class friends.
  //
  //----------------------------------------------------------------------------

  friend class sbAutoFinalizeVolumeMonitor;


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEMARSHALL
  NS_DECL_NSICLASSINFO


  //
  // Constructors/destructors.
  //

  sbIPDMarshall();

  virtual ~sbIPDMarshall();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Internal services fields.
  //
  //   mMonitor                 Marshall services monitor.
  //   mDeviceManager           Device manager.
  //   mDeviceRegistrar         Device registrar.
  //   mDeviceList              List of devices.
  //   mEventHandler            Volume event handler proc.
  //   mEventHandlerRef         Volume event handler reference.
  //

  PRMonitor                     *mMonitor;
  nsCOMPtr<sbIDeviceManager2>   mDeviceManager;
  nsCOMPtr<sbIDeviceRegistrar>  mDeviceRegistrar;
  nsInterfaceHashtable<nsUint32HashKey, nsISupports>
                                mDeviceList;
  EventHandlerProcPtr           mVolumeEventHandler;
  EventHandlerRef               mVolumeEventHandlerRef;


  //
  // iPod device marshall monitor services.
  //

  nsresult InitializeVolumeMonitor();

  void FinalizeVolumeMonitor();

  static pascal OSStatus VolumeEventHandler(EventHandlerCallRef aNextHandler,
                                            EventRef            aEventRef,
                                            void*               aHandlerCtx);

  void VolumeEventHandler(EventHandlerCallRef aNextHandler,
                          EventRef            aEventRef);

  void HandleAddedEvent(FSVolumeRefNum aVolumeRefNum);

  void HandleRemovedEvent(FSVolumeRefNum aVolumeRefNum);


  //
  // Internal services.
  //

  nsresult ScanForConnectedDevices();

  bool IsIPod(FSVolumeRefNum aVolumeRefNum);

  nsresult GetVolumePath(FSVolumeRefNum aVolumeRefNum,
                         nsAString&     aVolumePath);

  nsresult GetFirewireGUID(FSVolumeRefNum aVolumeRefNum,
                           nsAString&     aFirewireGUID);
};


/**
 * This class may be used to set up to finalize the iPod device marshall monitor
 * services when exiting from a function or code block.
 */

SB_AUTO_CLASS(sbAutoFinalizeVolumeMonitor,
              sbIPDMarshall*,
              mValue,
              mValue->FinalizeVolumeMonitor(),
              mValue = nsnull);


#endif /* __SB_IPD_MARSHALL_H__ */

