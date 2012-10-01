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

// Songbird imports.
#include <sbBaseDeviceMarshall.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceRegistrar.h>
#include "sbLibHal.h"

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>

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
 * This class communicates iPod device arrival and removal events to the
 * sbIDeviceManager service.
 */

class sbIPDMarshall : public sbBaseDeviceMarshall,
                      public nsIClassInfo
{
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
  //   mDeviceMediaPartitionTable
  //                            Table mapping media partitions to devices.
  //   mSBLibHalCtx             HAL library API context.
  //

  PRMonitor                     *mMonitor;
  nsCOMPtr<sbIDeviceManager2>   mDeviceManager;
  nsCOMPtr<sbIDeviceRegistrar>  mDeviceRegistrar;
  nsInterfaceHashtable<nsCStringHashKey, nsISupports>
                                mDeviceList;
  nsClassHashtable<nsCStringHashKey, nsCString>
                                mDeviceMediaPartitionTable;
  sbLibHalCtx*                  mSBLibHalCtx;


  //
  // Event services.
  //

  nsresult EventInitialize();

  void EventFinalize();

  static void HandleLibHalDevicePropertyModified(LibHalContext* aLibHalCtx,
                                                 const char*    aDeviceUDI,
                                                 const char*    aKey,
                                                 dbus_bool_t    aIsRemoved,
                                                 dbus_bool_t    aIsAdded);

  void HandleLibHalDevicePropertyModified(nsACString& aDeviceUDI,
                                          const char* aKey,
                                          dbus_bool_t aIsRemoved,
                                          dbus_bool_t aIsAdded);

  static void HandleLibHalDeviceRemoved(LibHalContext* aLibHalCtx,
                                        const char*    aDeviceUDI);

  void HandleLibHalDeviceRemoved(nsACString& aDeviceUDI);

  void HandleAddedEvent(const nsACString& aDeviceUDI,
                        const nsACString& aMediaPartitionUDI);

  void HandleRemovedEvent(const nsACString& aDeviceUDI,
                          const nsACString& aMediaPartitionUDI);


  //
  // Internal services.
  //

  nsresult Initialize();

  void Finalize();

  nsresult ScanForConnectedDevices();

  nsresult ProbeDev(nsACString &aDeviceUDI);

  PRBool IsIPod(nsACString& aDeviceUDI);

  PRBool IsMediaPartition(nsACString& aDeviceUDI);
};


#endif /* __SB_IPD_MARSHALL_H__ */

