/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */

/** 
 * \file  CDDevice.h
 * \brief Songbird CDDevice Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbICDDevice.h"
#include "DeviceBase.h"

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_CDDevice_CONTRACTID  "@songbird.org/Songbird/Device/CDDevice;1"
#define SONGBIRD_CDDevice_CLASSNAME   "Songbird CD Device"
#define SONGBIRD_CDDevice_CID { 0xfed4919e, 0x213f, 0x4448, { 0xa6, 0x89, 0xf2, 0x2d, 0xd1, 0xc5, 0x69, 0x94 } }
// {FED4919E-213F-4448-A689-F22DD1C56994}

#define CONTEXT_COMPACT_DISC_DEVICE NS_LITERAL_STRING("compactdiscDB-").get()

#include "WinCDImplementation.h"

// CLASSES ====================================================================

class sbCDDevice :  public sbICDDevice, public sbDeviceBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBICDDEVICE

  sbCDDevice();

  // Transfer related
  virtual nsString GetDeviceDownloadTable(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTable(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadTableDescription(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableDescription(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadTableType(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableType(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadReadable(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableReadable(const PRUnichar* deviceString);

  virtual PRBool TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber);
  virtual PRBool StopCurrentTransfer();
  virtual PRBool SuspendCurrentTransfer();
  virtual PRBool ResumeTransfer();

private:
  virtual void OnThreadBegin();
  virtual void OnThreadEnd();

  virtual PRBool IsEjectSupported();

  virtual PRBool InitializeSync();
  virtual PRBool FinalizeSync();
  virtual PRBool DeviceEventSync(PRBool mediaInserted);

  ~sbCDDevice();

  WinCDObjectManager mCDManagerObject;
};
