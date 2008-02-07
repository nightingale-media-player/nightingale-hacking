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
#ifndef SBDEVICETHREAD_H_
#define SBDEVICETHREAD_H_

#include <windows.h>
#include <nsIRunnable.h>
#include <nsCOMPtr.h>

class sbWPDDevice;

/**
 * This is the thread that handles communication with the device and allows
 * the UI thread to continue on.
 */
class sbWPDDeviceThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  
  /**
   * Sets the device and handle
   * TODO: Need to find the proper way to initialize
   */
  sbWPDDeviceThread(sbWPDDevice * device,
                    HANDLE event);
  /**
   * Cleans up the mDevice
   */
	virtual ~sbWPDDeviceThread();
	/**
	 * Wakes up the thread to look at the queue for work
	 */
	void WakeUp()
	{
	  ::SetEvent(mEvent);
	}
	/**
	 * Used to tell the thread it's time to go
	 */
	void Die()
	{
	  mTimeToDie = PR_TRUE;
	  WakeUp();
	}
private:
  HANDLE mEvent;
  sbWPDDevice * mDevice;
  PRBool mTimeToDie;
};

#define SB_WPDDEVICETHREAD_CID \
{0x323c534f, 0xafc0, 0x4a41, {0x90, 0x2a, 0x84, 0x23, 0x69, 0x2e, 0xcf, 0x88}}

#define SB_WPDDEVICETHREAD_CONTRACTID \
  "@songbirdnest.com/Songbird/WPDDeviceThread;1"

#define SB_WPDDEVICETHREAD_CLASSNAME \
  "WPDDeviceThread"

#define SB_WPDDEVICETHREAD_DESCRIPTION \
  "Windows Portable Device Thread"

#endif /*SBDEVICETHREAD_H_*/
