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
#include <string/nsString.h>
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"

#include "sbIDatabaseResult.h"
#include "sbIDatabaseQuery.h"
#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#ifdef WIN32
#include "win32/WMDImplementation.h"
#endif

/* Implementation file */

#define NAME_WINDOWS_MEDIA_DEVICE_LEN     NS_LITERAL_STRING("Songbird Windows Media Device").Length()
#define NAME_WINDOWS_MEDIA_DEVICE         NS_LITERAL_STRING("Songbird WM Device").get()

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS2(sbWMDevice, sbIDeviceBase, sbIWMDevice)

//-----------------------------------------------------------------------------
sbWMDevice::sbWMDevice()
: sbDeviceBase(PR_TRUE)
, mpMonitor(nsnull)
{
  mpMonitor = PR_NewMonitor();
  NS_ASSERTION(mpMonitor, "sbWMDevice::mpMonitor failed to be created.");  
  mDeviceManager = new WMDManager(this);
  NS_ASSERTION(mDeviceManager, "sbWMDevice::mDeviceManager failed to be created.");
} //ctor

//-----------------------------------------------------------------------------
sbWMDevice::~sbWMDevice() 
{
  //Specifically because the WM device must maintain a lock
  //on resources shared between the main thread and it's 
  //worker thread. Doing this for *your* device is ill advised!
  sbDeviceBase::RequestThreadShutdown();

  PR_EnterMonitor(mpMonitor);

  delete mDeviceManager;
  mDeviceManager = nsnull;

  PR_ExitMonitor(mpMonitor);
  PR_DestroyMonitor(mpMonitor);
  mpMonitor = nsnull;
} //dtor

NS_IMETHODIMP
sbWMDevice::Initialize(PRBool *_retval)
{
  InitializeAsync();
  return NS_OK;
}

PRBool sbWMDevice::InitializeSync()
{
  CleanupWMDEntries();

  NS_ASSERTION(mDeviceManager, 
    "Warning! sbWMDevice::InitializeSync called with \
    null sbWMDevice::mDeviceManager!");

  if(mDeviceManager)
    mDeviceManager->Initialize();

  return PR_TRUE;
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
  mDeviceManager->GetDeviceStringByIndex(aIndex, _retval);

  return NS_OK;
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
  NS_ASSERTION(mDeviceManager, "sbWMDevice::mDeviceManager is null! WTF!?!?!");
  if(mDeviceManager) mDeviceManager->GetContext(aDeviceString, _retval);

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
  if(mDeviceManager)
  {
    *_retval = mDeviceManager->GetTrackTable(aDeviceString, aDBContext, aTableName);
  }

  return NS_OK;
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
  PR_EnterMonitor(mpMonitor);
  CoInitializeEx(0, COINIT_MULTITHREADED);
}

void
sbWMDevice::OnThreadEnd()
{
  CoUninitialize();
  PR_ExitMonitor(mpMonitor);
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
  if(mDeviceManager)
  {
    *aDeviceCount = mDeviceManager->GetNumDevices();
  }

  return S_OK;
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

void sbWMDevice::CleanupWMDEntries()
{
  // Setup the list of playlists as a persistent query
  nsCOMPtr<sbIDatabaseQuery>  playlistQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  playlistQuery->SetAsyncQuery( PR_FALSE );
  playlistQuery->SetDatabaseGUID( NS_LITERAL_STRING("songbird") );

  nsCOMPtr< sbIPlaylistManager > pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
  if(pPlaylistManager.get())
    pPlaylistManager->GetAllPlaylistList( playlistQuery );

  nsCOMPtr<sbIDatabaseResult> resultset;
  playlistQuery->GetResultObject( getter_AddRefs(resultset) );
  if (resultset.get())
  {
    PRInt32 rowcount = 0;
    resultset->GetRowCount( &rowcount );
    nsString databaseName;
    const nsString wmdDBPrefix(CONTEXT_WINDOWS_MEDIA_DEVICE);
    for (PRInt32 rowNumber = 0; rowNumber < rowcount; rowNumber ++)
    {
      resultset->GetRowCellByColumn( rowNumber, NS_LITERAL_STRING("service_uuid"), databaseName);

      if (databaseName.Length() > wmdDBPrefix.Length())
      {
        nsString dbprefix;
        databaseName.Left(dbprefix, wmdDBPrefix.Length());
        if (dbprefix == wmdDBPrefix)
        {
          // Found one, clear entries
          ClearLibraryData(databaseName);
        }
      }
    }
  }
}

void sbWMDevice::ClearLibraryData(nsAString& dbContext)
{
  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  pQuery->SetAsyncQuery(PR_FALSE);
  pQuery->SetDatabaseGUID(dbContext);

  PRBool retVal;

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );

  if(pPlaylistManager.get())
  {
    pPlaylistManager->DeletePlaylist(nsString(TRACK_TABLE), pQuery, &retVal);

    if(pLibrary.get())
    {
      pLibrary->SetQueryObject(pQuery);
      pLibrary->PurgeDefaultLibrary(PR_FALSE, &retVal);
    }
  }
}


