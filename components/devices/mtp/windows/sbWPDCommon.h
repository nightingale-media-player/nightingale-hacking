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

#include <stdio.h>
#include <tchar.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <atlbase.h>
#include <atlstr.h>

// Dealing with Win32 macro mess
#undef CreateDevice
#undef CreateEvent

#include <nscore.h>
#include <nsStringGlue.h>

class sbIDeviceManager2;
class sbIDeviceMarshall;
class sbIDevice;
class sbIDeviceEventTarget;
class sbIDeviceEvent;
struct IPortableDeviceContent;

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

/**
 * Returns a PROVARIANT built from the incoming string
 * It's the callers responsibility to ensure proper cleanup of the PROPVARIANT's
 * resources
 */
nsresult sbStringToPropVariant(nsAString const & str,
                               PROPVARIANT & var);

/**
 * Returns the object ID from a PUID
 */
nsresult sbObjectIDFromPUID(IPortableDeviceContent * content,
                            nsAString const & PUID,
                            nsAString & objectID);
#endif /*SBWPDCOMMON_H_*/
