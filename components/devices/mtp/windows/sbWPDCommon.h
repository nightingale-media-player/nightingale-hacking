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

#ifndef SBWPDCOMMON_H_
#define SBWPDCOMMON_H_

// The following is placed below the other includes to avoid win32 macro 
// madness
#ifndef WINVER              // Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600       // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0600 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400    // Change this to the appropriate value to target IE 5.0 or later.
#endif

#include <stdio.h>
#include <tchar.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <atlbase.h>
#include <atlstr.h>
#include <nscore.h>

class sbIDeviceManager2;
class sbIDeviceMarshall;
class sbIDevice;
class sbIDeviceEventTarget;
class sbIDeviceEvent;

/**
 * Retreives the WPD device manager
 */
inline
HRESULT sbGetPortableDeviceManager(IPortableDeviceManager ** deviceManager)
{
  // CoCreate the IPortableDeviceManager interface to enumerate
  // portable devices and to get information about them.
  return CoCreateInstance(CLSID_PortableDeviceManager,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceManager,
                        (VOID**) deviceManager);
}

/**
 * Create the Songbird Device manager and return it
 * @param deviceManager the newly created device manager
 */
nsresult sbWPDCreateDeviceManager(sbIDeviceManager2 ** deviceManager);
/**
 * This creates an event for the device and dispatches it to the manager
 * @param marshall The marshaller for this device of this event
 * @param device the Songbird device this event is related to
 * @param eventID the ID of the event
 */
nsresult sbWPDCreateAndDispatchEvent(sbIDeviceMarshall * marshall,
                           sbIDevice * device,
                           PRUint32 eventID,
                           PRBool async = PR_FALSE);

#endif /*SBWPDCOMMON_H_*/
