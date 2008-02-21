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

#include "sbWPDCapabilitiesBuilder.h"
#include "sbWPDCommon.h"
#include <map>
#include <set>
#include <vector>
#include <sbDeviceCapabilities.h>
#include <sbIDeviceEvent.h>

/**
 * Maps the WPD content type to the Songbird function and content type
 */
typedef struct CapMapEntryStruct 
{
  CapMapEntryStruct(GUID const & aGuid,
              PRUint32 content,
              PRUint32 function) :
                capGuid(aGuid),
                sbContent(content),
                sbFunction(function) {}
  GUID const capGuid;
  PRUint32 const sbContent;
  PRUint32 const sbFunction;
} CapMapEntry_t;

/**
 * Returns the Songbird content type and function type for the WPD content type
 */
static PRBool ConvertContentType(GUID const & contentTypeGuid, PRUint32 & content, PRUint32 & function)
{
  static CapMapEntry_t const CapMap[] = 
  {
    CapMapEntry_t(WPD_CONTENT_TYPE_FOLDER, sbIDeviceCapabilities::CONTENT_FOLDER, sbIDeviceCapabilities::FUNCTION_DEVICE),
    CapMapEntry_t(WPD_CONTENT_TYPE_IMAGE, sbIDeviceCapabilities::CONTENT_IMAGE, sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY),
    CapMapEntry_t(WPD_CONTENT_TYPE_DOCUMENT, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_CONTACT, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_CONTACT_GROUP, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_AUDIO, sbIDeviceCapabilities::CONTENT_AUDIO, sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK),
    CapMapEntry_t(WPD_CONTENT_TYPE_VIDEO, sbIDeviceCapabilities::CONTENT_VIDEO, sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK),
    CapMapEntry_t(WPD_CONTENT_TYPE_PLAYLIST, sbIDeviceCapabilities::CONTENT_PLAYLIST, sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK),
    CapMapEntry_t(WPD_CONTENT_TYPE_MIXED_CONTENT_ALBUM, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_AUDIO_ALBUM, sbIDeviceCapabilities::CONTENT_ALBUM, sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK),
    CapMapEntry_t(WPD_CONTENT_TYPE_IMAGE_ALBUM, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_VIDEO_ALBUM, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_MEMO, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_EMAIL, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_APPOINTMENT, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_TASK, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_PROGRAM, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_GENERIC_FILE, sbIDeviceCapabilities::CONTENT_FILE, sbIDeviceCapabilities::FUNCTION_DEVICE), 
    CapMapEntry_t(WPD_CONTENT_TYPE_CALENDAR, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_GENERIC_MESSAGE, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_NETWORK_ASSOCIATION, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_CERTIFICATE, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_WIRELESS_PROFILE, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_MEDIA_CAST, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
    CapMapEntry_t(WPD_CONTENT_TYPE_SECTION, sbIDeviceCapabilities::CONTENT_UNKNOWN, sbIDeviceCapabilities::FUNCTION_UNKNOWN),
  };
  int const CapMapSize = sizeof(CapMap) / sizeof(CapMapEntry_t);
  for (int index = 0; index < CapMapSize; ++index) {
    if (contentTypeGuid == CapMap[index].capGuid) {
      content = CapMap[index].sbContent;
      function = CapMap[index].sbFunction;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}
typedef std::multimap<PRUint32, PRUint32> sbCaps_t;
typedef std::set<PRUint32> sbFunctionTypes_t;
typedef std::set<PRUint32> sbContentTypes_t;

/**
 * Helper that sets the content types for a specific function type
 */
template <class T>
static nsresult SetContenTypesForFunctions(sbIDeviceCapabilities * capabilities,
                                           PRUint32 function,
                                           T const & collection)
{
  PRUint32 * buffer = new PRUint32[collection.size()];
  NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
  std::copy(collection.begin(),
            collection.end(),
            buffer);
  nsresult rv = capabilities->AddContentTypes(function,
                                              buffer,
                                              collection.size());
  delete [] buffer;
  return rv;
}

sbWPDCapabilitiesBuilder::sbWPDCapabilitiesBuilder(IPortableDeviceCapabilities * wpdCapabilities) :
                                                   mWPDCapabilities(wpdCapabilities)
{
}

sbWPDCapabilitiesBuilder::~sbWPDCapabilitiesBuilder()
{
}

nsresult sbWPDCapabilitiesBuilder::Create(sbIDeviceCapabilities ** capabilities)
{
  nsresult rv;
  mSBCapabilities = do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetEventTypes();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetContentTypes();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetContentCaptureTypes();
  NS_ENSURE_SUCCESS(rv, rv);
  mSBCapabilities.forget(capabilities);
  return NS_OK;
}

/**
 * Sets the list of formats for a given content type
 */
nsresult sbWPDCapabilitiesBuilder::SetSupportedFormats(GUID contentType)
{
  nsRefPtr<IPortableDevicePropVariantCollection> formats;
  HRESULT hr = mWPDCapabilities->GetSupportedFormats(contentType, 
                                                     getter_AddRefs(formats));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  DWORD formatCount = 0;
  hr = formats->GetCount(&formatCount);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  // Build an array of the format strings
  char const ** formatStrings = new char const *[formatCount];
  NS_ENSURE_TRUE(formatStrings, NS_ERROR_OUT_OF_MEMORY);
  DWORD index;
  for (index = 0; index < formatCount; ++index) {
    PROPVARIANT format = {0};
    PropVariantInit(&format);
    hr = formats->GetAt(index, &format);
    NS_ENSURE_TRUE(SUCCEEDED(hr) || format.vt != VT_CLSID, NS_ERROR_FAILURE);
    nsCString contentType;
    PRBool isContainerFormat;
    PRBool isPlaylistFormat;
    sbWPDObjectFormatToContentTypeString(*(format.puuid),
                                         contentType,
                                         isContainerFormat,
                                         isPlaylistFormat);
    formatStrings[index] = ToNewCString(contentType);
    NS_ENSURE_TRUE(formatStrings[index], NS_ERROR_OUT_OF_MEMORY);
  }
  PRUint32 sbContentType;
  PRUint32 sbFunctionType;
  hr = ConvertContentType(contentType,
                          sbContentType,
                          sbFunctionType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = mSBCapabilities->AddFormats(sbContentType,
                                   formatStrings,
                                   formatCount);
  // Free our strings and array
  for (index = 0; index < formatCount; ++index) {
    NS_Free(const_cast<char*>(formatStrings[index]));
  }
  delete [] formatStrings;
  return FAILED(hr) ? NS_ERROR_FAILURE : NS_OK;
}

/**
 * Sets the list of content types for rendering function types
 */
nsresult sbWPDCapabilitiesBuilder::SetContentTypes()
{
  // Look at all of the WPD supported content types
  nsRefPtr<IPortableDevicePropVariantCollection>   objects;
  HRESULT hr = mWPDCapabilities->GetSupportedContentTypes(WPD_FUNCTIONAL_CATEGORY_ALL,
                                                          getter_AddRefs(objects));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  DWORD totalObjects = 0;
  hr = objects->GetCount(&totalObjects);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  // Accumulate all the Songbird function and content types as well as the 
  // mapping of function to content. This is only playback, capture will
  // be processed separately in \see sbWPDCapabilitiesBuilder::SetContentCaptureTypes
  sbCaps_t cabilitiesMap;
  typedef std::vector<GUID> sbWPDContentTypes;
  sbWPDContentTypes wpdContentTypes;
  sbFunctionTypes_t functionTypes;
  for (DWORD index = 0; index < totalObjects; ++index) {
    PROPVARIANT pv = {0};
    PropVariantInit(&pv);
    hr = objects->GetAt(index, &pv);
    if (SUCCEEDED(hr) && pv.puuid != NULL &&
        pv.vt == VT_CLSID) {
      PRUint32 content;
      PRUint32 function;
      if (ConvertContentType(*(pv.puuid), content, function) &&
        function != sbIDeviceCapabilities::FUNCTION_UNKNOWN &&
        content != sbIDeviceCapabilities::CONTENT_UNKNOWN) {
        wpdContentTypes.push_back(*(pv.puuid));
        cabilitiesMap.insert(sbCaps_t::value_type(function, content));
        functionTypes.insert(function);
      }
    }
    PropVariantClear(&pv);
  }
  // Now we need to set the list of songbird function types
  PRUint32 * buffer = new PRUint32[functionTypes.size()];
  NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
  std::copy(functionTypes.begin(),
            functionTypes.end(),
            buffer);
  nsresult rv = mSBCapabilities->SetFunctionTypes(buffer, functionTypes.size());
  delete [] buffer;
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Now map the supported content types by function.
  sbCaps_t::const_iterator const end = cabilitiesMap.end();
  if (!cabilitiesMap.empty()) {
    std::vector<PRUint32> contentTypes;
    // Save off the first key for group by logic
    PRUint32 lastFunctionType = cabilitiesMap.begin()->first;
    for (sbCaps_t::const_iterator iter = cabilitiesMap.begin();
         iter != end;
         ++iter) {
      // If we've found the end of the function type set
      // the list of content types for it
      if (lastFunctionType != iter->first) {
        rv = SetContenTypesForFunctions(mSBCapabilities,
                                        lastFunctionType,
                                        contentTypes);
        NS_ENSURE_SUCCESS(rv, rv);
        contentTypes.clear();
        lastFunctionType = iter->first;
      }
      contentTypes.push_back(iter->second);
    }
    // Set the content types for the last function type in the list
    rv = SetContenTypesForFunctions(mSBCapabilities,
                                    lastFunctionType,
                                    contentTypes);
  }
  // Iterate over the entire list of supported content types and add their formats
  sbWPDContentTypes::const_iterator const contentTypesEnd = wpdContentTypes.end();
  for (sbWPDContentTypes::const_iterator iter = wpdContentTypes.begin();
       iter != contentTypesEnd;
       ++iter) {
    SetSupportedFormats(*iter);
  }

  return rv;
}

/**
 * This function sets the content types for the various supported capture functional
 * categories.
 */
nsresult sbWPDCapabilitiesBuilder::SetContentCaptureTypes()
{
  struct WPDCategoryFunctionMap
  {
    WPDCategoryFunctionMap(GUID const & category, PRUint32 function) :
      WPDCategory(category),
      sbFunction(function) {}
    GUID const WPDCategory;
    PRUint32 const sbFunction;
  };  // This is the list of capture functional categories
  static WPDCategoryFunctionMap const sbWPDFunctionalCategories[] =
  {
    WPDCategoryFunctionMap(WPD_FUNCTIONAL_CATEGORY_DEVICE, sbIDeviceCapabilities::FUNCTION_DEVICE),
    WPDCategoryFunctionMap(WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE, sbIDeviceCapabilities::FUNCTION_IMAGE_CAPTURE),
    WPDCategoryFunctionMap(WPD_FUNCTIONAL_CATEGORY_AUDIO_CAPTURE, sbIDeviceCapabilities::FUNCTION_AUDIO_CAPTURE),
    WPDCategoryFunctionMap(WPD_FUNCTIONAL_CATEGORY_VIDEO_CAPTURE, sbIDeviceCapabilities::FUNCTION_VIDEO_CAPTURE)
  };
  int const CATEGORIES = sizeof(sbWPDFunctionalCategories) / sizeof(WPDCategoryFunctionMap);
  // We'll look at each capture WPD functional category. We're assuming nothing will be
  // returned if this category is not supported
  for (int index = 0; index < CATEGORIES; ++index) {
    // Get the WPD content types for the WPD capture functional category
    nsRefPtr<IPortableDevicePropVariantCollection>   objects;
    HRESULT hr = mWPDCapabilities->GetSupportedContentTypes(sbWPDFunctionalCategories[index].WPDCategory,
                                                            getter_AddRefs(objects));
    DWORD totalObjects = 0;
    hr = objects->GetCount(&totalObjects);
    if (FAILED(hr))
      return NS_ERROR_FAILURE;
    // Accumulate all the content types for the functional category
    sbContentTypes_t contentTypes;
    for (DWORD typeIndex = 0; typeIndex < totalObjects; ++typeIndex) {
      PROPVARIANT pv = {0};
      PropVariantInit(&pv);
      hr = objects->GetAt(typeIndex, &pv);
      if (SUCCEEDED(hr) && pv.puuid != NULL &&
        pv.vt == VT_CLSID) {
        PRUint32 content;
        PRUint32 function;
        if (ConvertContentType(*(pv.puuid), content, function) &&
            function != sbIDeviceCapabilities::FUNCTION_UNKNOWN &&
            content != sbIDeviceCapabilities::CONTENT_UNKNOWN) {
          contentTypes.insert(content);
        }
      }
      PropVariantClear(&pv);
    }
    // Take the accumulated content types and map them to the capture conent type
    if (!contentTypes.empty()) {
      nsresult rv = SetContenTypesForFunctions(mSBCapabilities,
                                               sbWPDFunctionalCategories[index].sbFunction,
                                               contentTypes);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  // The format types should have already been set for these content types.
  return NS_OK;
}

PRBool EventGuidIn(IPortableDevicePropVariantCollection * eventGUIDS,
                   GUID const & eventGuid)
{
  DWORD count = 0;
  eventGUIDS->GetCount(&count);
  PRBool match = PR_FALSE;
  for (DWORD index = 0; !match && index < count; ++index) {
    PROPVARIANT var = {0};
    PropVariantInit(&var);
    match = SUCCEEDED(eventGUIDS->GetAt(index, &var)) && 
        var.vt == VT_CLSID && 
        *(var.puuid) == eventGuid;
    PropVariantClear(&var);
 }
  return match;
}
/**
 * Helper function that sets the list of event types we support
 */
nsresult sbWPDCapabilitiesBuilder::SetEventTypes()
{
  // Maps the Songbird event to the WPD event. If the WPD event GUID is null
  // this means there isn't a direct relationship to WPD and so is supported
  // implicitily.
  struct EventMapEntry
  {
    PRUint32 eventType;
    GUID WPDEventGUID;
  };
  static EventMapEntry const eventTypes[] =
  {
    { sbIDeviceEvent::COMMAND_DEVICE_RESET, WPD_EVENT_DEVICE_RESET },
    { sbIDeviceEvent::COMMAND_DEVICE_MEDIA_FORMAT, WPD_EVENT_STORAGE_FORMAT },
    { sbIDeviceEvent::EVENT_DEVICE_ADDED, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_REMOVED, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_INSERTED, WPD_EVENT_OBJECT_ADDED, },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_REMOVED, WPD_EVENT_OBJECT_REMOVED },
    { sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_TRANSFER_PROGRESS, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_START, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_END, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_FAILED, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_START, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_END, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_FAILED, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_UNSUPPORTED_TYPE, GUID_NULL },
    { sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE, GUID_NULL }
  };
  PRUint32 const eventTypesSize = sizeof(eventTypes) / sizeof(EventMapEntry);
  // Get the WPD list of supported events
  nsRefPtr<IPortableDevicePropVariantCollection> supportedWPDEvents;
  if (FAILED(mWPDCapabilities->GetSupportedEvents(getter_AddRefs(supportedWPDEvents))))
    return NS_ERROR_FAILURE;
  // Allocate the maximum sized buffer.
  PRUint32 supportedEventTypes[eventTypesSize];
  PRUint32 supportedEventTypesSize = 0;
  for (PRUint32 index = 0; index < eventTypesSize; ++index) {
    EventMapEntry const & mapEntry = eventTypes[index];
    if (mapEntry.WPDEventGUID == GUID_NULL || 
        EventGuidIn(supportedWPDEvents, mapEntry.WPDEventGUID)) {
      supportedEventTypes[supportedEventTypesSize++] = eventTypes[index].eventType;
    }
  }
  mSBCapabilities->SetEventTypes(supportedEventTypes,
                                 supportedEventTypesSize);
}
