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

// Dealing with Win32 macro mess
#undef CreateDevice
#undef CreateEvent

#include <nscore.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbStandardDeviceProperties.h>
#include <sbStandardProperties.h>

class sbIDeviceManager2;
class sbIDeviceMarshall;
class sbIDevice;
class sbIDeviceEventTarget;
class sbIDeviceEvent;
class sbILibrary;
class sbIMediaItem;
class sbIPropertyArray;

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
 * Map entry to convert a file extension to a WPD Object Format
 * GUID and Content Type GUID.
 */
typedef struct  
{
  char * mExtension;
  GUID   mContentType;
  GUID   mObjectFormat;
} wpdFileExtensionKeymapEntry_t;

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
sbWPDStandardDevicePropertyToPropertyKey(const char* aStandardProp,
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

/**
 * Convert a file extension to a WPD Content Type GUID and
 * WPD Object Format GUID.
 * \param aFileExt The file extension to convert.
 * \param aContentType The WPD content type for the input file extension.
 * \param aObjectFormat The WPD object format for the input file extension.
 * \retval NS_ERROR_NOT_AVAILABLE Couldn't find ContentType / ObjectFormat pair for file extension.
 * \retval NS_OK Found ContentType / ObjectFormat pair.
 */
nsresult sbWPDFileExtensionToGUIDs(const nsACString &aFileExt,
                                   GUID &aContentType,
                                   GUID &aObjectFormat);

/**
 * Look up the file extension corresponding to a given WPD Object Format GUID
 * \param aObjectFormat The WPD object format to look for
 * \param aFileExt The file extension that matches the given object format
 * \retval NS_ERROR_NOT_AVAILABLE couldn't find ObjectFormat
 * \retval NS_OK found corresponding extension
 */
nsresult sbWPDGUIDtoFileExtension(GUID &aObjectFormat,
                                  /* out */ nsACString& aFileExt);

/**
 * Find the most appropriate folder for a specific content type.
 * For example, find the folder to put MUSIC into.
 * If own found, returns aParentID.
 * \param aContentType The WPD content type GUID.
 * \param aParentID The WPD object ID of the parent
 * \param aContent the device content to make it more likely to match
 * \retval The suggested folder's object id.
 */
nsString sbWPDGetFolderForContentType(const GUID &aContentType,
                                      const nsAString &aParentID,
                                      IPortableDeviceContent* aContent);


/* find the child object of the given WPD_OBJECT_ORIGINAL_FILE_NAME
 * @param aParent the WPD object ID of the parent
 * @param aName the name to look for
 * @return The object ID of the child, or empty string if not found
 */
nsString sbWPDFindChildNamed(const nsAString& aParent,
                             const nsAString& aName,
                             IPortableDeviceContent* aContent);

/**
 * Convert a standard data model property to a WPD propertykey
 * \param aProp The standard data model property to convert (ie. SB_PROPERTY_TRACKNAME).
 * \param aPropertyKey The WPD property key equivalent.
 * \retval NS_ERROR_NOT_AVAILABLE Could not determine equivalent WPD property key.
 * \retval NS_OK Found equivalent WPD property key.
 */
nsresult sbWPDStandardItemPropertyToPropertyKey(const char *aProp,
                                                PROPERTYKEY &aPropertyKey);

nsresult sbWPDPropertyKeyToStandardItemProperty(const PROPERTYKEY &aPropertyKey,
                                                nsACString &aProp);

nsresult sbWPDSetMediaItemPropertiesFromDeviceValues(sbIMediaItem *aItem, 
                                                     const nsTArray<PROPERTYKEY> &aKeys,
                                                     IPortableDeviceValues *aValues);

nsresult sbWPDGetMediaItemByPUID(sbILibrary *aLibrary, 
                                 const nsAString &aPUID,
                                 sbIMediaItem **aItem);

nsresult sbWPDCreateMediaItemFromDeviceValues(sbILibrary *aLibrary,
                                              const nsTArray<PROPERTYKEY> &aKeys,
                                              IPortableDeviceValues *aValues,
                                              sbIMediaItem **aItem);

nsresult sbWPDCreatePropertyArrayFromDeviceValues(const nsTArray<PROPERTYKEY> &aKeys,
                                                  IPortableDeviceValues *aValues,
                                                  sbIPropertyArray **aProps);

#endif /*SBWPDCOMMON_H_*/
