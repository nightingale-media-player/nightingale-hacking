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
#include "sbPropertyVariant.h"

#include <nsAutoPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <sbIDeviceMarshall.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbIDevice.h>
#include <nsIVariant.h>
#include <sbIDeviceEvent.h>
#include <propvarutil.h>

#include <sbILibrary.h>
#include <sbIMediaItem.h>
#include <sbIMediaListListener.h>

#include <sbIPropertyArray.h>
#include <sbPropertiesCID.h>

#include <nsID.h>
#include <nsIURI.h>
#include <nsNetUtil.h>

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
sbWPDStandardDevicePropertyToPropertyKey(const char* aStandardProp,
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

// Some weird undocumented GUID for playlists, as found on some Sansa devices
// {BA050000-AE6C-4804-98BA-C57B46965FE7}
static const GUID WPD_OBJECT_FORMAT_PLAPLAYLIST = 
{ 0xBA050000, 0xAE6C, 0x4804, { 0x98, 0xBA, 0xC5, 0x7B, 0x46, 0x96, 0x5F, 0xE7 } };

static wpdFileExtensionKeymapEntry_t MAP_FILE_EXTENSION_KEYMAP[] = {
  { "mp3", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_MP3 },
  { "wma", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_WMA },
  { "aac", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_AAC },
  
  /* playlists */
  { "asx", WPD_CONTENT_TYPE_PLAYLIST, WPD_OBJECT_FORMAT_ASXPLAYLIST },
  { "m3u", WPD_CONTENT_TYPE_PLAYLIST, WPD_OBJECT_FORMAT_M3UPLAYLIST },
  { "pla", WPD_CONTENT_TYPE_PLAYLIST, WPD_OBJECT_FORMAT_PLAPLAYLIST },
  { "pls", WPD_CONTENT_TYPE_PLAYLIST, WPD_OBJECT_FORMAT_PLSPLAYLIST },
  { "wpl", WPD_CONTENT_TYPE_PLAYLIST, WPD_OBJECT_FORMAT_WPLPLAYLIST },
};

nsresult 
sbWPDFileExtensionToGUIDs(const nsACString &aFileExt,
                          GUID &aContentType,
                          GUID &aObjectFormat)
{

  const PRUint32 mapSize = NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_KEYMAP);

  nsCString str(aFileExt);
  for(PRUint32 current = 0; current < mapSize; ++current) {
    if(!strcmp(str.get(), MAP_FILE_EXTENSION_KEYMAP[current].mExtension)) {
      aContentType = MAP_FILE_EXTENSION_KEYMAP[current].mContentType;
      aObjectFormat = MAP_FILE_EXTENSION_KEYMAP[current].mObjectFormat;

      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult sbWPDGUIDtoFileExtension(GUID &aObjectFormat,
                                  /* out */ nsACString& aFileExt)
{
  const PRUint32 mapSize = NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_KEYMAP);
  
  for(PRUint32 current = 0; current < mapSize; ++current) {
    if (aObjectFormat == MAP_FILE_EXTENSION_KEYMAP[current].mObjectFormat) {
      aFileExt.Assign(MAP_FILE_EXTENSION_KEYMAP[current].mExtension);
      return NS_OK;
    }
  }
  aFileExt.Truncate();
  return NS_ERROR_NOT_AVAILABLE;
}


nsString
sbWPDGetFolderForContentType(const GUID &aContentType,
                             const nsAString &aParentID,
                             IPortableDeviceContent *aContent)
{
  struct FolderMapping_t {
    const GUID guid;
    const char ** name;
  };
  
  const char* NAMES_AUDIO[]    = {"MUSIC", "My Music", NULL};
  const char* NAMES_PLAYLIST[] = {"PLAYLISTS", "My Playlists", NULL};
  const char* NAMES_IMAGE[]    = {"PICTURES", "My Pictures", NULL};
  const char* NAMES_VIDEO[]    = {"VIDEO", "My Videos", NULL};
  
  const FolderMapping_t MAP[] = {
    { WPD_CONTENT_TYPE_AUDIO,    NAMES_AUDIO },
    { WPD_CONTENT_TYPE_PLAYLIST, NAMES_PLAYLIST },
    { WPD_CONTENT_TYPE_IMAGE,    NAMES_IMAGE },
    { WPD_CONTENT_TYPE_VIDEO,    NAMES_VIDEO }
  };
  
  nsString resultObjId(aParentID);
  for (int guidIdx = 0; guidIdx < NS_ARRAY_LENGTH(MAP); ++guidIdx) {
    if (MAP[guidIdx].guid == aContentType) {
      for (const char ** name = MAP[guidIdx].name; name; ++name) {
        resultObjId = sbWPDFindChildNamed(aParentID,
                                          NS_ConvertASCIItoUTF16(*name),
                                          aContent);
        if (!resultObjId.IsEmpty()) {
          break;
        }
      }
      break;
    }
  }
  return resultObjId;
}


static const wpdPropertyKeymapEntry_t MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP[] = {
  { SB_PROPERTY_CONTENTLENGTH,        WPD_OBJECT_SIZE },
  { SB_PROPERTY_TRACKNAME,            WPD_MEDIA_TITLE },
  { SB_PROPERTY_ALBUMNAME,            WPD_MUSIC_ALBUM },
  { SB_PROPERTY_ARTISTNAME,           WPD_MEDIA_ARTIST },
  { SB_PROPERTY_DURATION,             WPD_MEDIA_DURATION },
  { SB_PROPERTY_GENRE,                WPD_MEDIA_GENRE },
  { SB_PROPERTY_TRACKNUMBER,          WPD_MUSIC_TRACK },
  // { SB_PROPERTY_YEAR,                 ??? },
  // { SB_PROPERTY_DISCNUMBER,           ??? },
  // { SB_PROPERTY_TOTALDISCS,           ??? },
  // { SB_PROPERTY_TOTALTRACKS,          ??? },
  // { SB_PROPERTY_ISPARTOFCOMPILATION,  ??? },
  // { SB_PROPERTY_PRODUCERNAME,         ??? },
  { SB_PROPERTY_COMPOSERNAME,         WPD_MEDIA_COMPOSER },
  // { SB_PROPERTY_LYRICISTNAME,         ??? },
  { SB_PROPERTY_LYRICS,               WPD_MUSIC_LYRICS },
  // { SB_PROPERTY_RECORDLABELNAME,      ??? },
  // { SB_PROPERTY_PRIMARYIMAGEURL,      ??? },
  { SB_PROPERTY_LASTPLAYTIME,         WPD_MEDIA_LAST_ACCESSED_TIME },
  { SB_PROPERTY_PLAYCOUNT,            WPD_MEDIA_USE_COUNT },
  // { SB_PROPERTY_LASTSKIPTIME,         ??? },
  { SB_PROPERTY_SKIPCOUNT,            WPD_MEDIA_SKIP_COUNT },
  { SB_PROPERTY_RATING,               WPD_MEDIA_STAR_RATING },
  // { SB_PROPERTY_ISLIST,               ??? },
  // { SB_PROPERTY_MEDIALISTNAME,        ??? },
  { "http://songbirdnest.com/data/1.0#WPDPUID", WPD_OBJECT_PERSISTENT_UNIQUE_ID },
};

nsresult 
sbWPDStandardItemPropertyToPropertyKey(const char *aProp,
                                       PROPERTYKEY &aPropertyKey)
{
  static const PRUint32 mapSize = NS_ARRAY_LENGTH(MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP);

  NS_ENSURE_ARG_POINTER(aProp);

  for(PRUint32 current = 0; current < mapSize; ++current) {
    if(!strcmp(MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP[current].mStandardProperty, aProp)) {
      aPropertyKey = MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP[current].mPropertyKey;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult sbWPDPropertyKeyToStandardItemProperty(const PROPERTYKEY &aPropertyKey,
                                                nsACString &aProp)
{
  static const PRUint32 mapSize = NS_ARRAY_LENGTH(MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP);

  for(PRUint32 current = 0; current < mapSize; ++current) {
    if(MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP[current].mPropertyKey.pid == aPropertyKey.pid) {
      aProp.Assign(MAP_PROPERTY_TO_PROPERTYKEY_KEYMAP[current].mStandardProperty);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult sbWPDSetMediaItemPropertiesFromDeviceValues(sbIMediaItem *aItem, 
                                                     const nsTArray<PROPERTYKEY> &aKeys,
                                                     IPortableDeviceValues *aValues)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aValues);

  nsCOMPtr<sbIPropertyArray> props;
  nsresult rv = sbWPDCreatePropertyArrayFromDeviceValues(aKeys, aValues, getter_AddRefs(props));

  return aItem->SetProperties(props);
}


class sbWPDGetMediaItemByPUIDEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnEnumerationBegin(sbIMediaList *aMediaList, PRUint16 *_retval) {
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = sbIMediaListEnumerationListener::CONTINUE;
    return NS_OK;
  }

  NS_IMETHODIMP OnEnumeratedItem(sbIMediaList *aMediaList, sbIMediaItem *aItem, PRUint16 *_retval) {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_ARG_POINTER(_retval);

    NS_WARN_IF_FALSE(!mFoundItem, "Found multiple items with the same WPD PUID!");
    mFoundItem = aItem;

    *_retval = sbIMediaListEnumerationListener::CANCEL;

    return NS_OK;
  }

  NS_IMETHODIMP OnEnumerationEnd(sbIMediaList *aMediaList, nsresult aStatusCode) {
    return NS_OK;
  }

  nsresult GetItem(sbIMediaItem **aItem) {
    NS_ENSURE_ARG_POINTER(aItem);
    NS_ENSURE_TRUE(mFoundItem, NS_ERROR_NOT_AVAILABLE);
    NS_ADDREF(*aItem = mFoundItem);
    return NS_OK;
  }

protected:
  virtual ~sbWPDGetMediaItemByPUIDEnumerationListener() {};
  nsCOMPtr<sbIMediaItem> mFoundItem;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbWPDGetMediaItemByPUIDEnumerationListener,
                              sbIMediaListEnumerationListener);

nsresult sbWPDGetMediaItemByPUID(sbILibrary *aLibrary, 
                                 const nsAString &aPUID,
                                 sbIMediaItem **aItem)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aItem);

  *aItem = nsnull;
  
  nsRefPtr<sbWPDGetMediaItemByPUIDEnumerationListener> listener;
  NS_NEWXPCOM(listener, sbWPDGetMediaItemByPUIDEnumerationListener);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    
  nsresult rv = aLibrary->EnumerateItemsByProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#WPDPUID"),
                                                   aPUID, 
                                                   listener, 
                                                   sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return listener->GetItem(aItem);
}

nsresult sbWPDCreateMediaItemFromDeviceValues(sbILibrary *aLibrary,
                                              const nsTArray<PROPERTYKEY> &aKeys,
                                              IPortableDeviceValues *aValues,
                                              nsID *aDeviceID,
                                              sbIMediaItem **aItem)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_ARG_POINTER(aItem);

  nsCOMPtr<sbIPropertyArray> props;
  nsresult rv = sbWPDCreatePropertyArrayFromDeviceValues(aKeys, aValues, getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);

  char strDeviceID[NSID_LENGTH] = {0};
  aDeviceID->ToProvidedString(strDeviceID);

  nsCString itemSpec(NS_LITERAL_CSTRING("x-mtp://"));
  itemSpec.Append(strDeviceID);
  itemSpec.AppendLiteral("/");
  
  nsString itemPUID;
  rv = props->GetPropertyValue(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#WPDPUID"),
                               itemPUID);
  NS_ENSURE_SUCCESS(rv, rv);

  itemSpec.Append(NS_ConvertUTF16toUTF8(itemPUID));
  itemSpec.AppendLiteral("/");

  LPWSTR itemOriginalFilename = NULL;
  HRESULT hr = aValues->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, 
                                       &itemOriginalFilename);

#if defined(DEBUG)
  if(FAILED(hr)) {
    NS_WARNING("Couldn't get original filename, item URI will seem wrong but still function!");
  }
#endif
  
  if(itemOriginalFilename) {
    itemSpec.Append(NS_ConvertUTF16toUTF8(nsDependentString(itemOriginalFilename)));
    ::CoTaskMemFree(itemOriginalFilename);
  }

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), itemSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  //XXXAus: We allow duplicates currently. 
  nsCOMPtr<sbIMediaItem> item;
  rv = aLibrary->CreateMediaItem(uri, 
                                 nsnull, 
                                 PR_TRUE, 
                                 getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = item->SetProperties(props);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbWPDCreatePropertyArrayFromDeviceValues(const nsTArray<PROPERTYKEY> &aKeys,
                                                  IPortableDeviceValues *aValues,
                                                  sbIPropertyArray **aProps)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ENSURE_ARG_POINTER(aProps);

  DWORD count = aKeys.Length();

  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> props = 
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = props->SetStrict(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  for(DWORD current = 0; current < count; ++current) {
    LPWSTR value = NULL;
    
    nsString propValue;
    nsCString propName;

    HRESULT hr = aValues->GetStringValue(aKeys[current], &value);
    if(FAILED(hr))
      continue;

    propValue.Assign(value);
    ::CoTaskMemFree(value);

    if(NS_SUCCEEDED(sbWPDPropertyKeyToStandardItemProperty(aKeys[current], propName))) {
      rv = props->AppendProperty(NS_ConvertUTF8toUTF16(propName), propValue);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Value of property did not validate. It will not get set.");
    }
  }

  return CallQueryInterface(props, aProps);
}

static PRInt32 LossyCaseInsensitiveCompare(const nsAString::char_type *a,
                                           const nsAString::char_type *b,
                                           PRUint32 length)
{
  return CaseInsensitiveCompare(NS_LossyConvertUTF16toASCII(a, length).get(),
                                NS_LossyConvertUTF16toASCII(b, length).get(),
                                length);
}

nsString sbWPDFindChildNamed(const nsAString& aParent,
                             const nsAString& aName,
                             IPortableDeviceContent* aContent)
{
  HRESULT hr;
  nsresult rv;
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = aContent->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), EmptyString());
  
  nsRefPtr<IEnumPortableDeviceObjectIDs> folderIdList;
  hr = aContent->EnumObjects(0, /* flags */
                             aParent.BeginReading(),
                             NULL, /* no filter */
                             getter_AddRefs(folderIdList));
  if (FAILED(hr)) {
    return EmptyString();
  }
  
  nsRefPtr<IPortableDeviceKeyCollection> nameKey;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_ORIGINAL_FILE_NAME,
                                        getter_AddRefs(nameKey));
  if (FAILED(rv)) {
    return EmptyString();
  }
  
  for(;;) {
    LPWSTR pFolderObjId;
    hr = folderIdList->Next(1, &pFolderObjId, NULL);
    if (FAILED(hr) || (hr == S_FALSE)) {
      return EmptyString();
    }
      
    nsString folderObjId(pFolderObjId); // copies
    ::CoTaskMemFree(pFolderObjId);
    
    nsRefPtr<IPortableDeviceValues> nameValues;
    hr = properties->GetValues(folderObjId.BeginReading(),
                               nameKey,
                               getter_AddRefs(nameValues));
    if (FAILED(hr)) {
      continue;
    }
    
    LPWSTR nameStr;
    hr = nameValues->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &nameStr);
    if (FAILED(hr)) {
      continue;
    }
    
    if (aName.Equals(nameStr, LossyCaseInsensitiveCompare)) {
      ::CoTaskMemFree(nameStr);
      return folderObjId;
    }

    ::CoTaskMemFree(nameStr);
  }
  
  NS_NOTREACHED("returned from infinite loop");
  return EmptyString();
}
