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

#include <sbStandardDeviceProperties.h>

class sbIDeviceManager2;
class sbIDeviceMarshall;
class sbIDevice;
class sbIDeviceEventTarget;
class sbIDeviceEvent;
class nsIVariant;
struct IPortableDeviceContent;

/**
 * Map entry to convert a standard property to PROPERTYKEY.
 * This is used by sbWPPDStandardDevicePropertyToPropertyKey.
 */
typedef struct  
{
  char *        mStandardProperty;
  PROPERTYKEY   mPropertyKey;
} wpdPropertyKeymapEntry_t;

/**
 * Map entry to convert a WPD Object Format GUID to a
 * content type string (ie, GUID to "application/ogg").
 */
typedef struct  
{
  GUID    mObjectFormat;
  char *  mContentType;
  PRBool  mIsContainerFormat;
  PRBool  mIsPlaylistFormat;
} wpdObjectFormatKeymapEntry_t;

/**
 * Retreives the WPD device manager
 * \param deviceManager 
 */
inline
HRESULT sbWPDGetPortableDeviceManager(IPortableDeviceManager ** deviceManager)
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
 * \param deviceManager the newly created device manager
 */
nsresult sbWPDCreateDeviceManager(sbIDeviceManager2 ** deviceManager);

/**
 * This creates an event for the device and dispatches it to the manager
 * \param marshall The marshaller for this device of this event
 * \param device the Songbird device this event is related to
 * \param eventID the ID of the event
 */
nsresult sbWPDCreateAndDispatchEvent(sbIDeviceMarshall * marshall,
                                     sbIDevice * device,
                                     PRUint32 eventID,
                                     PRBool async = PR_FALSE);

/**
 * Returns a PROVARIANT built from the incoming string.
 * It's the callers responsibility to ensure proper cleanup of the PROPVARIANT's
 * resources
 * \param str String to convert to PROPVARIANT.
 * \param var PROPVARIANT that will recieve the converted string.
 */
nsresult sbWPDStringToPropVariant(nsAString const & str,
                                  PROPVARIANT & var);

/**
 * Returns the object ID from a PUID
 */
nsresult sbWPDObjectIDFromPUID(IPortableDeviceContent * content,
                               nsAString const & PUID,
                               nsAString & objectID);

/**
 * Returns the PROPERTYKEY associated with the Standard Device Property
 */
PRBool 
sbWPPDStandardDevicePropertyToPropertyKey(const char* aStandardProp,
                                          PROPERTYKEY &aPropertyKey);

/**
 * This function creates a property key collection from a single key
 */
nsresult sbWPDCreatePropertyKeyCollection(PROPERTYKEY const & key,
                                          IPortableDeviceKeyCollection ** propertyKeys);

/**
 * Converts an nsIVariant to a PROPVARIANT. Call is responsible for releasing
 * resources allocated to the PROPVARIANT via PropVariantClear
 */
nsresult sbWPDnsIVariantToPROPVARIANT(nsIVariant * aValue,
                                      PROPVARIANT & prop);

/**
 * Convert a WPD Object Format GUID to a sbIContentType encoding, 
 * decoding or container format value.
 * \param aObjectFormat The WPD Object Format GUID to convert.
 * \param aContenType This will hold the content type string.
 * \param aIsContainerFormat Indicates whether the returned content type string
 *                           is a container format.
 * \return The content type string and container format flag as well as NS_OK.
 * \retval NS_OK The Object Format has a content type string available.
 * \retval NS_ERROR_NOT_AVAILABLE The Object Format has _no_ content type string available.
 */
nsresult sbWPDObjectFormatToContentTypeString(const GUID &aObjectFormat,
                                              nsACString &aContentType,
                                              PRBool &aIsContainerFormat,
                                              PRBool &aIsPlaylistFormat);

#endif /*SBWPDCOMMON_H_*/
