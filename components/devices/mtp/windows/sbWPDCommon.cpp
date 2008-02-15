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

#include "sbWPDCommon.h"
#include <nsAutoPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <sbIDeviceMarshall.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbIDevice.h>
#include <nsIVariant.h>
#include <sbIDeviceEvent.h>
#include <propvarutil.h>

/**
 * Create the Songbird Device manager and return it
 */
nsresult sbWPDCreateDeviceManager(sbIDeviceManager2 ** deviceManager)
{
  NS_ENSURE_ARG(deviceManager);
  nsresult rv;
  nsCOMPtr<sbIDeviceManager2> dm(do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv));
  dm.forget(deviceManager);
  return rv;
}

/**
 * This creates an event for the device and dispatches it to the manager
 */
nsresult sbWPDCreateAndDispatchEvent(sbIDeviceMarshall * marshall,
                                     sbIDevice * device,
                                     PRUint32 eventID,
                                     PRBool async)
{  
  // Create the device manager
  nsCOMPtr<sbIDeviceManager2> manager;
  nsresult rv = sbWPDCreateDeviceManager(getter_AddRefs(manager));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // QI it to the event target
  nsCOMPtr<sbIDeviceEventTarget> target = do_QueryInterface(manager);
  if (!manager || !target)
    return NS_ERROR_FAILURE;

  // Create a variant to hold the device
  nsCOMPtr<nsIWritableVariant> var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  var->SetAsISupports(device);

  // Create the event
  nsCOMPtr<sbIDeviceEvent> event;
  rv = manager->CreateEvent(eventID, var, marshall, getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool dispatched;
  // Dispatch the event
  return target->DispatchEvent(event, async, &dispatched);
}

nsresult sbWPDStringToPropVariant(nsAString const & str,
                               PROPVARIANT & var)
{
  PRInt32 const length = str.Length();
  var.vt = VT_LPWSTR;
  var.pwszVal = reinterpret_cast<LPWSTR>(CoTaskMemAlloc((length + 1 ) * sizeof(PRUnichar)));
  if (!var.pwszVal)
    return NS_ERROR_OUT_OF_MEMORY;
  
  memcpy(var.pwszVal, nsString(str).get(), length * sizeof(PRUnichar));
  var.pwszVal[length] = 0;
  return NS_OK;
}

nsresult sbWPDObjectIDFromPUID(IPortableDeviceContent * content,
                            nsAString const & PUID,
                            nsAString & objectID)
{
  nsRefPtr<IPortableDevicePropVariantCollection> persistentUniqueIDs;
  HRESULT hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IPortableDevicePropVariantCollection,
                                getter_AddRefs(persistentUniqueIDs));
  PROPVARIANT var;
  if (FAILED(sbWPDStringToPropVariant(PUID,
                                   var)) || 
      FAILED(persistentUniqueIDs->Add(&var)))
    return NS_ERROR_FAILURE;
  PropVariantClear(&var);
  nsRefPtr<IPortableDevicePropVariantCollection> pObjectIDs;
  if (FAILED(content->GetObjectIDsFromPersistentUniqueIDs(persistentUniqueIDs,
                                                          getter_AddRefs(pObjectIDs))))
    return NS_ERROR_FAILURE;
  
  DWORD count = 0;
  if (FAILED(pObjectIDs->GetCount(&count)) || !count ||
      FAILED(pObjectIDs->GetAt(0, &var)) || var.vt != VT_LPWSTR)
    return NS_ERROR_FAILURE;
  
  objectID = var.pwszVal;
  PropVariantClear(&var);
  return NS_OK;
}

PRBool 
sbWPPDStandardDevicePropertyToPropertyKey(const char* aStandardProp,
                                          PROPERTYKEY &aPropertyKey)
{
  static wpdPropertyKeymapEntry_t map[] = {
    { SB_DEVICE_PROPERTY_BATTERY_LEVEL,     WPD_DEVICE_POWER_LEVEL },
    { SB_DEVICE_PROPERTY_CAPACITY,          WPD_STORAGE_CAPACITY },
    { SB_DEVICE_PROPERTY_FIRMWARE_VERSION,  WPD_DEVICE_FIRMWARE_VERSION},
    { SB_DEVICE_PROPERTY_MANUFACTURER,      WPD_DEVICE_MANUFACTURER },
    { SB_DEVICE_PROPERTY_MODEL,             WPD_DEVICE_MODEL },
    { SB_DEVICE_PROPERTY_NAME,              WPD_DEVICE_FRIENDLY_NAME },
    { SB_DEVICE_PROPERTY_SERIAL_NUMBER,     WPD_DEVICE_SERIAL_NUMBER },
    { SB_DEVICE_PROPERTY_POWER_SOURCE,      WPD_DEVICE_POWER_SOURCE },
  };

  static PRUint32 mapSize = sizeof(map) / sizeof(wpdPropertyKeymapEntry_t);

  if(aStandardProp == nsnull) {
    return PR_FALSE;
  }

  for(PRUint32 current = 0; current < mapSize; ++current) {
    if(!strcmp(map[current].mStandardProperty, aStandardProp)) {
      aPropertyKey = map[current].mPropertyKey;
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

nsresult sbWPDnsIVariantToPROPVARIANT(nsIVariant * aValue,
                                 PROPVARIANT & prop)
{
  PRUint16 dataType;
  nsresult rv = aValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (dataType) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_UINT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_UINT32:
    case nsIDataType::VTYPE_BOOL: {
      PRInt32 valueInt;
      rv = aValue->GetAsInt32(&valueInt);
      if (NS_SUCCEEDED(rv)) {  // fall through PRInt64 case otherwise
        NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromInt32(valueInt, &prop)),
                       NS_ERROR_FAILURE);
        return NS_OK;
      }
    }
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT64: {
      PRInt64 valueLong;
      rv = aValue->GetAsInt64(&valueLong);
      if (NS_SUCCEEDED(rv)) {  // fall through double case otherwise
        NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromInt64(valueLong, &prop)),
                       NS_ERROR_FAILURE);
        return NS_OK;
      }
    }
    case nsIDataType::VTYPE_FLOAT:
    case nsIDataType::VTYPE_DOUBLE: {
      double valueDouble;
      rv = aValue->GetAsDouble(&valueDouble);
      NS_ENSURE_SUCCESS(rv, rv);
      
      NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromDouble(valueDouble, &prop)),
                     NS_ERROR_FAILURE);
      return NS_OK;
    }
    case nsIDataType::VTYPE_CHAR:
    case nsIDataType::VTYPE_WCHAR:
    case nsIDataType::VTYPE_DOMSTRING:
    case nsIDataType::VTYPE_CHAR_STR:
    case nsIDataType::VTYPE_WCHAR_STR:
    case nsIDataType::VTYPE_STRING_SIZE_IS:
    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    case nsIDataType::VTYPE_UTF8STRING:
    case nsIDataType::VTYPE_CSTRING:
    case nsIDataType::VTYPE_ASTRING: {
      nsAutoString stringValue;
      rv = aValue->GetAsAString(stringValue);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(SUCCEEDED(sbWPDStringToPropVariant(stringValue, prop)),
                     NS_ERROR_FAILURE);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult sbWPDCreatePropertyKeyCollection(PROPERTYKEY const & key,
                                          IPortableDeviceKeyCollection ** propertyKeys)
{
  NS_ENSURE_ARG(propertyKeys);
  nsRefPtr<IPortableDeviceKeyCollection> propKeys;
  
  NS_ENSURE_TRUE(SUCCEEDED(CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                            NULL,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IPortableDeviceKeyCollection,
                                            (VOID**) &propKeys)),
                 NS_ERROR_FAILURE);
  propKeys.forget(propertyKeys);
  return (*propertyKeys)->Add(key);

}

nsresult sbWPDObjectFormatToContentTypeString(const GUID &aObjectFormat,
                                              nsACString &aContentType,
                                              PRBool &aIsContainerFormat,
                                              PRBool &aIsPlaylistFormat)
{
  static wpdObjectFormatKeymapEntry_t map[] = {
    { WPD_OBJECT_FORMAT_3GP,                    "video/3gpp",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_AAC,                    "audio/aac",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_AIFF,                   "audio/x-aiff",                   PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_ASF,                    "video/x-ms-asf",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_ASXPLAYLIST,            "application/asx",                PR_FALSE, PR_TRUE },
    { WPD_OBJECT_FORMAT_AUDIBLE,                "audio/audible ",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_AVI,                    "video/x-msvideo",                PR_TRUE,  PR_FALSE },
    { WPD_OBJECT_FORMAT_BMP,                    "image/bmp",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_EXECUTABLE,             "application/octet-stream",       PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_EXIF,                   "image/x-raw",                    PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_FLAC,                   "audio/x-flac",                   PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_FLASHPIX,               "application/vnd.netfpx",         PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_GIF,                    "image/gif",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_HTML,                   "text/html",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_ICON,                   "image/x-icon",                   PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_JFIF,                   "image/jpeg",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_JP2,                    "image/jp2",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_JPX,                    "image/jpx",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_M3UPLAYLIST,            "audio/x-mpegurl",                PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MHT_COMPILED_HTML,      "message/rfc822",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MICROSOFT_EXCEL,        "application/vnd.ms-excel",       PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MICROSOFT_POWERPOINT,   "application/vnd.ms-powerpoint",  PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MICROSOFT_WORD,         "application/msword",             PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MP2,                    "video/mpeg2",                    PR_FALSE, PR_FALSE},
    { WPD_OBJECT_FORMAT_MP3,                    "audio/mpeg",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MP4,                    "video/mp4",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_MPEG,                   "video/mpeg",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_OGG,                    "application/ogg",                PR_TRUE,  PR_FALSE },
    { WPD_OBJECT_FORMAT_PCD,                    "image/x-photo-cd",               PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_PICT,                   "image/pict",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_PLSPLAYLIST,            "audio/scpls",                    PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_PNG,                    "image/png",                      PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_TEXT,                   "text/plain",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_TIFF,                   "image/tiff",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_TIFFEP,                 "image/tiff",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_TIFFIT,                 "image/tiff",                     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_UNSPECIFIED,            "application/octet-stream",       PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_VCALENDAR1,             "text/calendar",                  PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_VCARD2,                 "text/x-vcard",                   PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_VCARD3,                 "text/x-vcard",                   PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_WAVE,                   "audio/x-wav",                    PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_WINDOWSIMAGEFORMAT,     "image/x-ms-bmp",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_WMA,                    "audio/x-ms-wma",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_WMV,                    "video/x-ms-wmv",                 PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_WPLPLAYLIST,            "application/vnd.ms-wpl",         PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_X509V3CERTIFICATE,      "application/x-x509-ca-cert",     PR_FALSE, PR_FALSE },
    { WPD_OBJECT_FORMAT_XML,                    "application/xml",                PR_FALSE, PR_FALSE }

    /* XXXAus: No idea, this is for CIFF (Canon Camera Image File Format) */
    /*{ WPD_OBJECT_FORMAT_CIFF,                   "",PR_FALSE, PR_FALSE },*/

    /* XXXAus: No idea, this is for the Digital Print Order File Format.*/
    /*{ WPD_OBJECT_FORMAT_DPOF,                   "",PR_FALSE, PR_FALSE },*/

    /* XXXAus: No idea, this is the Windows Connect Now File Format */
    /*{ WPD_OBJECT_FORMAT_MICROSOFT_WFC,          "",PR_FALSE, PR_FALSE },*/
    
    /* XXXAus: No idea, this is a playlist format used with AVCHD files.
    /*{ WPD_OBJECT_FORMAT_MPLPLAYLIST,            "",PR_FALSE, PR_TRUE },*/
    
    /* XXXAus: No idea, this is for Network Association Files.*/
    /* { WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION,    "",PR_FALSE, PR_FALSE },*/

    /* XXXAus: No idea what to do with this one, it's for objects that are made up of properties only.
    /*{ WPD_OBJECT_FORMAT_PROPERTIES_ONLY,      "", PR_FALSE, PR_FALSE } */

    /* XXXAus: No idea what to do with this one, it's for a script file that is specific to the device model in use.
    /*{ WPD_OBJECT_FORMAT_SCRIPT,               "", PR_FALSE, PR_FALSE },*/
  };

  static PRUint32 mapSize = sizeof(map) / sizeof(wpdObjectFormatKeymapEntry_t);

  for(PRUint32 current = 0; current < mapSize; ++current) {
    if( IsEqualGUID(map[current].mObjectFormat, aObjectFormat)) {
      
      aContentType = map[current].mContentType;
      aIsContainerFormat = map[current].mIsContainerFormat;
      aIsPlaylistFormat = map[current].mIsPlaylistFormat;
      
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}
