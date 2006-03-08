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
 * \file  DownloadDevice.h
 * \brief Songbird DownloadDevice Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbIDownloadDevice.h"
#include "DeviceBase.h"

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_DownloadDevice_CONTRACTID  "@songbird.org/Songbird/Device/DownloadDevice;1"
#define SONGBIRD_DownloadDevice_CLASSNAME   "Songbird Download Device"
#define SONGBIRD_DownloadDevice_CID { 0x5918440D, 0xAF8B, 0x40e9, { 0x80, 0xC4, 0xF1, 0x13, 0x2B, 0xD8, 0x93, 0xA9 } }

// CLASSES ====================================================================

class sbDownloadListener;

class sbDownloadDevice :  public sbIDownloadDevice, public sbDeviceBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBIDOWNLOADDEVICE

  sbDownloadDevice();

private:
  virtual void OnThreadBegin();
  virtual void OnThreadEnd();

  virtual PRBool TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber);
  
  virtual PRBool StopCurrentTransfer();
  virtual PRBool SuspendCurrentTransfer();
  virtual PRBool ResumeTransfer();

  // Transfer related
  virtual nsString GetDeviceDownloadTable(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTable(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadTableDescription(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableDescription(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadTableType(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableType(const PRUnichar* deviceString);
  virtual nsString GetDeviceDownloadReadable(const PRUnichar* deviceString);
  virtual nsString GetDeviceUploadTableReadable(const PRUnichar* deviceString);
  
  friend class sbDownloadListener;
private:
  PRInt32 mCurrentDownloadRowNumber;
  PRBool mDownloading;
  ~sbDownloadDevice();
  class sbDownloadListener *mListener;

  void ReleaseListener();
};
