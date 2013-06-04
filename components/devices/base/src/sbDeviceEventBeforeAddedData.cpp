/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

#include "sbDeviceEventBeforeAddedData.h"

#include <mozilla/Mutex.h>
#include <nsAutoPtr.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceEventBeforeAddedData,
                              sbIDeviceEventBeforeAddedData)

sbDeviceEventBeforeAddedData::sbDeviceEventBeforeAddedData()
: mLock(nsnull)
, mContinueAddingDevice(PR_TRUE)
{
}

sbDeviceEventBeforeAddedData::~sbDeviceEventBeforeAddedData()
{

}

nsresult 
sbDeviceEventBeforeAddedData::Init(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mDevice = aDevice;

  return NS_OK;
}

/*static*/ nsresult
sbDeviceEventBeforeAddedData::CreateEventBeforeAddedData(
                                sbIDevice *aDevice,
                                sbIDeviceEventBeforeAddedData **aBeforeAddedData)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aBeforeAddedData);

  nsRefPtr<sbDeviceEventBeforeAddedData> beforeAddedData;
  beforeAddedData = new sbDeviceEventBeforeAddedData;

  nsresult rv = beforeAddedData->Init(aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceEventBeforeAddedData> retval = 
    do_QueryInterface(beforeAddedData, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  retval.forget(aBeforeAddedData);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceEventBeforeAddedData::GetContinueAddingDevice(PRBool *aContinueAddingDevice)
{
  NS_ENSURE_ARG_POINTER(aContinueAddingDevice);

  mozilla::MutexAutoLock lock(mLock);
  *aContinueAddingDevice = mContinueAddingDevice;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceEventBeforeAddedData::SetContinueAddingDevice(PRBool aContinueAddingDevice)
{
  mozilla::MutexAutoLock lock(mLock);
  mContinueAddingDevice = aContinueAddingDevice;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceEventBeforeAddedData::GetDevice(sbIDevice **aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  mozilla::MutexAutoLock lock(mLock);

  NS_ENSURE_TRUE(mDevice, NS_ERROR_UNEXPECTED);
  NS_ADDREF(*aDevice = mDevice);

  return NS_OK;
}
