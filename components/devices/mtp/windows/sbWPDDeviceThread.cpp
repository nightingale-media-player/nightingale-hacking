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
#include "sbWPDDeviceThread.h"
#include <nsAutoLock.h>
#include "sbWPDDevice.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbWPDDeviceThread,
                              nsIRunnable)
                              
sbWPDDeviceThread::sbWPDDeviceThread(sbWPDDevice * device,
                                     HANDLE event) :
  mDevice(device),
  mTimeToDie(PR_FALSE),
  mEvent(event)
{
  NS_ASSERTION(device, "Device cannot be null");
}

sbWPDDeviceThread::~sbWPDDeviceThread()
{
  Die();
}

NS_IMETHODIMP sbWPDDeviceThread::Run()
{
  do
  {
    // If PRocessThreadsRequest returns true it means it's it's time to die.
    // This may be due to a catostrophic error or normal shutdown while requests
    // are being processed.
    mTimeToDie = !mDevice->ProcessThreadsRequest();
    // Wait on the event, will get set when there are items on the queue
    ::WaitForSingleObject(mEvent, INFINITE);
  } while (!mTimeToDie);
  return NS_OK;
}
