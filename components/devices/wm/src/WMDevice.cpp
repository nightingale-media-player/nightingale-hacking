/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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
* \file  sbWMDevice.cpp
* \Windows Media device Component Implementation.
*/
#include "WMDevice.h"
#include "nspr.h"
#include "objbase.h"

#include <xpcom/nscore.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <necko/nsIURI.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsMemory.h>
#include <string/nsStringAPI.h>
#include "NSString.h"
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"

#ifdef WIN32
#include "win32/WMDImplementation.h"
#endif

/* Implementation file */

#define NAME_WINDOWS_MEDIA_DEVICE_LEN     NS_LITERAL_STRING("Songbird Windows Media Device").Length()
#define NAME_WINDOWS_MEDIA_DEVICE         NS_LITERAL_STRING("Songbird Windows Media Device").get()

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbWMDevice, sbIDeviceBase, sbIWMDevice)

//-----------------------------------------------------------------------------
sbWMDevice::sbWMDevice():
  sbDeviceBase(PR_TRUE)
{
  mDeviceManager = new WMDManager(this);
} //ctor

//-----------------------------------------------------------------------------
sbWMDevice::~sbWMDevice() 
{
  delete mDeviceManager;
  mDeviceManager = NULL;
} //dtor

NS_IMETHODIMP
sbWMDevice::Initialize(PRBool *_retval)
{
  mDeviceManager->Initialize();
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::Finalize(PRBool *_retval)
{
  mDeviceManager->Finalize();
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::AddCallback(sbIDeviceBaseCallback* aCallback,
                        PRBool* _retval)
{
  return sbDeviceBase::AddCallback(aCallback, _retval);
}

NS_IMETHODIMP
sbWMDevice::RemoveCallback(sbIDeviceBaseCallback* aCallback,
                           PRBool* _retval)
{
  return sbDeviceBase::RemoveCallback(aCallback, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetDeviceStringByIndex(PRUint32 aIndex,
                                   nsAString& _retval)
{
  return sbDeviceBase::GetDeviceStringByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbWMDevice::SetName(const nsAString& aName)
{
  return sbDeviceBase::SetName(aName);
}

NS_IMETHODIMP
sbWMDevice::GetContext(const nsAString& aDeviceString,
                       nsAString& _retval)
{
  if (mDeviceManager)
    mDeviceManager->GetContext(aDeviceString, _retval);
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::IsDownloadSupported(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::GetSupportedFormats(const nsAString& aDeviceString,
                                PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::IsUploadSupported(const nsAString& aDeviceString,
                              PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::IsTransfering(const nsAString& aDeviceString,
                          PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::IsDeleteSupported(const nsAString& aDeviceString,
                              PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetUsedSpace(const nsAString& aDeviceString,
                         PRUint32* _retval)
{
  return sbDeviceBase::GetUsedSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetAvailableSpace(const nsAString& aDeviceString,
                              PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetTrackTable(const nsAString& aDeviceString,
                          nsAString& aDBContext,
                          nsAString& aTableName,
                          PRBool *_retval)
{
  return sbDeviceBase::GetTrackTable(aDeviceString, aDBContext, aTableName,
                                     _retval);
}

NS_IMETHODIMP
sbWMDevice::AutoDownloadTable(const nsAString& aDeviceString,
                              const nsAString& aContextInput,
                              const nsAString& aTableName,
                              const nsAString& aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const nsAString& aSourcePath,
                              const nsAString& aDestPath,
                              nsAString& aTransferTable,
                              PRBool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbWMDevice::AutoUploadTable(const nsAString& aDeviceString,
                            const nsAString& aContextInput,
                            const nsAString& aTableName,
                            const nsAString& aFilterColumn,
                            PRUint32 aFilterCount,
                            const PRUnichar** aFilterValues,
                            const nsAString& aSourcePath,
                            const nsAString& aDestPath,
                            nsAString& aTransferTable,
                            PRBool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbWMDevice::AbortTransfer(const nsAString& aDeviceString, PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::DeleteTable(const nsAString& aDeviceString,
                        const nsAString& aDBContext,
                        const nsAString& aTableName,
                        PRBool *_retval)
{
  return sbDeviceBase::DeleteTable(aDeviceString, aDBContext, aTableName,
                                   _retval);
}

NS_IMETHODIMP
sbWMDevice::UpdateTable(const nsAString& aDeviceString,
                        const nsAString& aTableName,
                        PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbWMDevice::EjectDevice(const nsAString& aDeviceString,
                        PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(aDeviceString, _retval);
}

/* End DeviceBase */

NS_IMETHODIMP
sbWMDevice::GetDeviceCategory(nsAString& aDeviceCategory)
{   
  aDeviceCategory.Assign(NAME_WINDOWS_MEDIA_DEVICE);
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::GetName(nsAString& aName)
{
  aName.Assign(NAME_WINDOWS_MEDIA_DEVICE);
  return NS_OK;
}

PRBool
sbWMDevice::IsEjectSupported() 
{
  return PR_FALSE;
}

void
sbWMDevice::OnThreadBegin()
{
  CoInitializeEx(0, COINIT_MULTITHREADED);
}

void
sbWMDevice::OnThreadEnd()
{
  CoUninitialize();
}

NS_IMETHODIMP
sbWMDevice::IsUpdateSupported(const nsAString& aDeviceString,
                              PRBool *_retval)
{
  return sbDeviceBase::IsUpdateSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::IsEjectSupported(const nsAString& aDeviceString,
                             PRBool *_retval)
{
  return sbDeviceBase::IsEjectSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetDeviceCount(PRUint32* aDeviceCount)
{
  return sbDeviceBase::GetDeviceCount(aDeviceCount);
}

NS_IMETHODIMP
sbWMDevice::GetDestinationCount(const nsAString& aDeviceString,
                                PRUint32* _retval)
{
  return sbDeviceBase::GetDestinationCount(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::MakeTransferTable(const nsAString& aDeviceString,
                              const nsAString& aContextInput,
                              const nsAString& aTableName,
                              const nsAString& aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const nsAString& aSourcePath,
                              const nsAString& aDestPath,
                              PRBool aDownloading,
                              nsAString& aTransferTable,
                              PRBool* _retval)
{
  return sbDeviceBase::MakeTransferTable(aDeviceString, aContextInput,
                                         aTableName, aFilterColumn,
                                         aFilterCount, aFilterValues,
                                         aSourcePath, aDestPath,
                                         aDownloading, aTransferTable,
                                         _retval);
}

NS_IMETHODIMP
sbWMDevice::SuspendTransfer(const nsAString& aDeviceString,
                            PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

PRBool
sbWMDevice::SuspendCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

NS_IMETHODIMP
sbWMDevice::ResumeTransfer(const nsAString& aDeviceString,
                           PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbWMDevice::GetDeviceState(const nsAString& aDeviceString,
                           PRUint32 *_retval)
{
  return sbDeviceBase::GetDeviceState(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::RemoveTranferTracks(const nsAString& aDeviceString,
                                PRUint32 aIndex,
                                PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(aDeviceString, aIndex, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetDownloadTable(const nsAString& aDeviceString,
                             nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbWMDevice::GetUploadTable(const nsAString& aDeviceString,
                           nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbWMDevice::UploadTable(const nsAString& aDeviceString,
                        const nsAString& aTableName,
                        PRBool *_retval)
{
  return sbDeviceBase::UploadTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbWMDevice::DownloadTable(const nsAString& aDeviceString,
                          const nsAString& aTableName,
                          PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbWMDevice::SetDownloadFileType(const nsAString& aDeviceString,
                                PRUint32 aFileType,
                                PRBool *_retval)
{
  return sbDeviceBase::SetDownloadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbWMDevice::SetUploadFileType(const nsAString& aDeviceString,
                              PRUint32 aFileType,
                              PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetDownloadFileType(const nsAString& aDeviceString,
                                PRUint32 *_retval)
{
  return sbDeviceBase::GetDownloadFileType(aDeviceString, _retval);
}

NS_IMETHODIMP
sbWMDevice::GetUploadFileType(const nsAString& aDeviceString,
                              PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(aDeviceString, _retval);
}
