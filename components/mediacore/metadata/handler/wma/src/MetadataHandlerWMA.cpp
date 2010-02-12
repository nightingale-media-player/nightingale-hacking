/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
* \file MetadataHandlerWMA.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include "MetadataHandlerWMA.h"

/* Songbird interfaces */
#include "sbStandardProperties.h"
#include "sbIPropertyArray.h"
#include "sbPropertiesCID.h"
#include "sbMemoryUtils.h"
#include <sbProxiedComponentManager.h>
#include <sbStringUtils.h>
#include <sbFileUtils.h>

#include <nsIChannel.h>
#include <nsIContentSniffer.h>
#include <nsIFileStreams.h>
#include <nsIFileURL.h>
#include <nsIIOService.h>
#include <nsIURI.h>

#include <nsNetUtil.h>
#include <nsUnicharUtils.h>
#include <prlog.h>
#include <prprf.h>

#include <atlbase.h>
#include <wmsdk.h>
#include <wmp.h>
#include <wininet.h>

#ifdef PR_LOGGING
  static PRLogModuleInfo* gLog = PR_NewLogModule("sbMetadataHandlerWMA");
  #define LOG(args)   \
    PR_BEGIN_MACRO \
      if (gLog) PR_LOG(gLog, PR_LOG_WARN, args); \
    PR_END_MACRO
  #define TRACE(args) \
    PR_BEGIN_MACRO \
      if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args); \
    PR_END_MACRO
  #ifdef __GNUC__
    #define __FUNCTION__ __PRETTY_FUNCTION__
  #endif
#else
  #define LOG(args)   /* nothing */
  #define TRACE(args) /* nothing */
#endif

// DEFINES ====================================================================

#define COM_ENSURE_SUCCESS(_val)                           \
  PR_BEGIN_MACRO                                           \
    nsresult __rv = _val;                                  \
    if (FAILED(__rv)) {                                    \
      NS_ENSURE_SUCCESS_BODY(_val, _val);                  \
      return NS_ERROR_FAILURE;                             \
    }                                                      \
  PR_END_MACRO

#define countof(_array)                                    \
  sizeof(_array) / sizeof(_array[0])

#define SB_ARTIST       SB_PROPERTY_ARTISTNAME
#define SB_TITLE        SB_PROPERTY_TRACKNAME
#define SB_ALBUM        SB_PROPERTY_ALBUMNAME
#define SB_GENRE        SB_PROPERTY_GENRE
#define SB_TRACKNO      SB_PROPERTY_TRACKNUMBER
#define SB_VENDOR       SB_PROPERTY_SOFTWAREVENDOR
#define SB_COPYRIGHT    SB_PROPERTY_COPYRIGHT
#define SB_COMPOSER     SB_PROPERTY_COMPOSERNAME
#define SB_CONDUCTOR    SB_PROPERTY_CONDUCTORNAME
#define SB_DIRECTOR     "director"
#define SB_BPM          SB_PROPERTY_BPM
#define SB_LYRICS       SB_PROPERTY_LYRICS
#define SB_YEAR         SB_PROPERTY_YEAR
#define SB_BITRATE      SB_PROPERTY_BITRATE
#define SB_RATING       SB_PROPERTY_RATING
#define SB_DESCRIPTION  "description"
#define SB_COMMENT      SB_PROPERTY_COMMENT
#define SB_LENGTH       SB_PROPERTY_DURATION
#define SB_ALBUMARTIST  SB_PROPERTY_ALBUMARTISTNAME
#define SB_PROTECTED    SB_PROPERTY_ISDRMPROTECTED

#define WMP_ARTIST      "Author"
#define WMP_TITLE       "Title"
#define WMP_ALBUM       "WM/AlbumTitle"
#define WMP_GENRE       "WM/Genre"
#define WMP_TRACKNO     "WM/TrackNumber"
#define WMP_VENDOR      "WM/ToolName"
#define WMP_COPYRIGHT   "Copyright"
#define WMP_COMPOSER    "WM/Composer"
#define WMP_CONDUCTOR   "WM/Conductor"
#define WMP_DIRECTOR    "WM/Director"
#define WMP_BPM         "WM/BeatsPerMinute"
#define WMP_LYRICS      "WM/Lyrics"
#define WMP_YEAR        "WM/Year"
#define WMP_BITRATE     "Bitrate"
#define WMP_RATING      "Rating"
#define WMP_DESCRIPTION "Description"
#define WMP_COMMENT     "Comment"
#define WMP_LENGTH      "Duration"
#define WMP_ALBUMARTIST "WM/AlbumArtist"
#define WMP_PROTECTED   "Is_Protected"

// Property namespace for Gracenote properties
// Note that this must match those used in sbGracenoteDefines.h, so
// be sure to change those if you change these.
#define SB_GN_EXTENDEDDATA  "http://gracenote.com/pos/1.0#extendedData"
#define SB_GN_TAGID         "http://gracenote.com/pos/1.0#tagId"
#define WMP_GN_EXTENDEDDATA "GN/ExtData"
#define WMP_GN_TAGID        "GN/UniqueFileIdentifier"


// These are the keys we're going to read from the WM interface 
// and push to the SB interface.
typedef struct
{
  PRUnichar const * const songbirdName;
  TCHAR const * const wmpName;
  WMT_ATTR_DATATYPE type;
} metadataKeyMapEntry_t;

#define KEY_MAP_ENTRY(_entry, _type) { NS_L(SB_##_entry), \
                                       NS_L(WMP_##_entry), \
                                       _type }

static const metadataKeyMapEntry_t kMetadataKeys[] = {
  KEY_MAP_ENTRY(ARTIST, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(TITLE, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(ALBUM, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(GENRE, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(TRACKNO, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(VENDOR, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(COPYRIGHT, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(COMPOSER, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(CONDUCTOR, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(COMMENT, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(BPM, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(LYRICS, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(YEAR, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(BITRATE, WMT_TYPE_DWORD),
  KEY_MAP_ENTRY(RATING, WMT_TYPE_STRING),
//  KEY_MAP_ENTRY(DESCRIPTION, WMT_TYPE_STRING),
//  KEY_MAP_ENTRY(DIRECTOR, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(LENGTH, WMT_TYPE_QWORD),
  KEY_MAP_ENTRY(ALBUMARTIST, WMT_TYPE_STRING),
  KEY_MAP_ENTRY(PROTECTED, WMT_TYPE_BOOL),
  KEY_MAP_ENTRY(GN_EXTENDEDDATA, WMT_TYPE_BINARY),
  KEY_MAP_ENTRY(GN_TAGID, WMT_TYPE_BINARY),
};
#undef KEY_MAP_ENTRY

// HELPER CLASSES =============================================================
SB_AUTO_CLASS(sbCoInitializeWrapper,
              HRESULT,
              SUCCEEDED(mValue),
              Invalidate(),
              if (SUCCEEDED(mValue)) {::CoUninitialize();} mValue = E_FAIL);

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMetadataHandlerWMA,
                              sbIMetadataHandler,
                              sbIMetadataHandlerWMA)

sbMetadataHandlerWMA::sbMetadataHandlerWMA() :
  m_Completed(PR_FALSE)
{
}

sbMetadataHandlerWMA::~sbMetadataHandlerWMA()
{
}

NS_IMETHODIMP
sbMetadataHandlerWMA::GetChannel(nsIChannel** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_IF_ADDREF(*_retval = m_Channel);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SetChannel(nsIChannel* aURLChannel)
{
  nsresult rv;
  m_Channel = aURLChannel;
  NS_ENSURE_STATE(m_Channel);

  nsCOMPtr<nsIURI> pURI;
  rv = m_Channel->GetURI(getter_AddRefs(pURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(pURI, &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("WMA METADATA: Remote files not currently supported");
    return NS_OK;
  }

  // Let's have a look at the file...
  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // How about we start with the cooked path?
  rv = file->GetPath(m_FilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::OnChannelData(nsISupports* aChannel)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::GetCompleted(PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_Completed;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Close()
{
  if (m_ChannelHandler) 
    m_ChannelHandler->Close();

  m_Channel = nsnull;
  m_ChannelHandler = nsnull;
  m_PropertyArray = nsnull;
  m_FilePath = EmptyString();

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Vote(const nsAString& aURL,
                           PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsAutoString strUrl(aURL);
  ToLowerCase(strUrl);

  if ( 
        ( strUrl.Find( ".wma", PR_TRUE ) != -1 ) || 
        ( strUrl.Find( ".wmv", PR_TRUE ) != -1 ) ||
        ( strUrl.Find( ".wm",  PR_TRUE ) != -1 ) ||
        ( strUrl.Find( ".asf", PR_TRUE ) != -1 ) ||
        ( strUrl.Find( ".asx", PR_TRUE ) != -1 )
     )
    *_retval = 140;
  else
    *_retval = -1;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Read(PRInt32* _retval)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(_retval);
  sbCoInitializeWrapper coinit(::CoInitialize(0));

  // We're never asynchronous.
  m_Completed = PR_TRUE;
  *_retval = 0;

  NS_ENSURE_TRUE(m_Channel, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(!m_FilePath.IsEmpty(), NS_ERROR_UNEXPECTED);
  nsresult rv = NS_ERROR_UNEXPECTED;

  m_PropertyArray = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Try to use the WMFSDK for this data because it's probably faster than
  // initializing the Windows Media Player ActiveX control.
  rv = ReadMetadataWMFSDK(m_FilePath, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;
    
  // The WMF SDK failed on the file (probably because it is protected), so let
  // the ActiveX control take a crack at it.
  rv = ReadMetadataWMP(m_FilePath, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;

  TRACE(("%s[%p] - failed to read data", __FUNCTION__, this));
  return NS_ERROR_INVALID_ARG; // This will cause a useful warning in the metadata job.
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Write(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(!m_FilePath.IsEmpty(), NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(m_PropertyArray, NS_ERROR_NOT_INITIALIZED);

  sbCoInitializeWrapper coinit(::CoInitialize(0));

  nsresult rv;
  HRESULT hr;

  *_retval = 0;
  nsCOMPtr<sbIMutablePropertyArray> propArray;
  m_PropertyArray.forget(getter_AddRefs(propArray));

  CComPtr<IWMMetadataEditor> editor;
  hr = WMCreateEditor(&editor);
  COM_ENSURE_SUCCESS(hr);

  hr = editor->Open(m_FilePath.get());
  COM_ENSURE_SUCCESS(hr);

  CComPtr<IWMHeaderInfo3> header;
  hr = editor->QueryInterface(&header);
  COM_ENSURE_SUCCESS(hr);
  
  sbAutoNSTypePtr<WORD> indices;
  WORD indicesSize = 0;

  nsString value;

  // special case primary image url
  rv = propArray->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                                   value);
  if (NS_UNLIKELY(NS_SUCCEEDED(rv))) {
    PRBool success;
    rv = SetImageDataInternal(sbIMetadataHandler::METADATA_IMAGE_TYPE_OTHER,
                              value,
                              header,
                              success);
    if (NS_SUCCEEDED(rv)) {
      if (success) {
        ++*_retval;
      }
    } else {
      nsresult __rv = rv;
      NS_ENSURE_SUCCESS_BODY(rv, rv);
    }
  }
  
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(kMetadataKeys); ++i) {
    rv = propArray->GetPropertyValue(nsDependentString(kMetadataKeys[i].songbirdName),
                                     value);
    if (NS_LIKELY(rv == NS_ERROR_NOT_AVAILABLE)) {
      // no data
      continue;
    }
    NS_ENSURE_SUCCESS(rv, rv);
    
    // find the index of the existing attribute, if any
    WORD indicesCount = 0;
    hr = header->GetAttributeIndices(0xFFFF,
                                     kMetadataKeys[i].wmpName,
                                     NULL,
                                     NULL,
                                     &indicesCount);
    COM_ENSURE_SUCCESS(hr);
    if (indicesCount > indicesSize) {
      indices = reinterpret_cast<WORD*>(NS_Alloc(sizeof(WORD) * indicesCount));
      NS_ENSURE_TRUE(indices, NS_ERROR_OUT_OF_MEMORY);
      indicesSize = indicesCount;
    }
    indicesCount = indicesSize;
    hr = header->GetAttributeIndices(0xFFFF,
                                     kMetadataKeys[i].wmpName,
                                     NULL,
                                     indices.get(),
                                     &indicesCount);
    COM_ENSURE_SUCCESS(hr);
    
    const BYTE* data = NULL;
    QWORD dataBuffer;
    DWORD dataLength = 0;
    switch (kMetadataKeys[i].type) {
      case WMT_TYPE_STRING: {
        data = reinterpret_cast<const BYTE*>(value.get());
        dataLength = value.Length() * sizeof(PRUnichar);
        break;
      }
      case WMT_TYPE_DWORD: {
        DWORD* dword = reinterpret_cast<DWORD*>(&dataBuffer);
        PRInt32 success = PR_sscanf(NS_ConvertUTF16toUTF8(value).BeginReading(),
                                    "%lu",
                                    dword);
        if (NS_UNLIKELY(success < 1)) {
          // the value is not a number but we expected one
          continue;
        }
        data = reinterpret_cast<BYTE*>(dword);
        dataLength = sizeof(DWORD);
        break;
      }
      case WMT_TYPE_QWORD: {
        PRInt32 success = PR_sscanf(NS_ConvertUTF16toUTF8(value).BeginReading(),
                                    "%llu",
                                    &dataBuffer);
        if (NS_UNLIKELY(success < 1)) {
          // the value is not a number but we expected one
          continue;
        }
        data = reinterpret_cast<BYTE*>(&dataBuffer);
        dataLength = sizeof(QWORD);
        break;
      }
      default: {
        LOG(("%s: don't know how to handle type %i",
             __FUNCTION__,
             kMetadataKeys[i].type));
        NS_WARNING(__FUNCTION__ ": don't know how to handle type");
        continue;
      }
    }
    if (indicesCount > 0) {
      if (dataLength == 0) {
        TRACE(("%s: Deleting %S\n", __FUNCTION__, kMetadataKeys[i].wmpName));
        hr = header->DeleteAttribute(0xFFFF,
                                     *(indices.get()));
      } else {
        TRACE(("%s: Modifying %S\n", __FUNCTION__, kMetadataKeys[i].wmpName));
        hr = header->ModifyAttribute(0xFFFF,
                                     *(indices.get()),
                                     kMetadataKeys[i].type,
                                     NULL,
                                     data,
                                     dataLength);
      }
      COM_ENSURE_SUCCESS(hr);
    } else {
      TRACE(("%s: Adding %S\n", __FUNCTION__, kMetadataKeys[i].wmpName));
      WORD index;
      hr = header->AddAttribute(0,
                                kMetadataKeys[i].wmpName,
                                &index,
                                kMetadataKeys[i].type,
                                0,
                                data,
                                dataLength);
      COM_ENSURE_SUCCESS(hr);
    }
    ++*_retval;
  }
  
  hr = editor->Flush();
  COM_ENSURE_SUCCESS(hr);

  return NS_OK;
} //Write

NS_IMETHODIMP
sbMetadataHandlerWMA::GetImageData(PRInt32 aType,
                                   nsACString &aMimeType,
                                   PRUint32 *aDataLen,
                                   PRUint8 **aData)
{
  NS_ENSURE_ARG_POINTER(aDataLen);
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_TRUE(!m_FilePath.IsEmpty(), NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  rv = ReadAlbumArtWMFSDK(m_FilePath, aMimeType, aDataLen, aData);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  // can't use WMFSDK, try WMP instead.
  rv = ReadAlbumArtWMP(m_FilePath, aMimeType, aDataLen, aData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SetImageData(PRInt32 aType,
                                   const nsAString &aURL)
{
  NS_ENSURE_TRUE(!m_FilePath.IsEmpty(), NS_ERROR_NOT_INITIALIZED);

  HRESULT hr;
  nsresult rv;

  CComPtr<IWMMetadataEditor> editor;
  hr = WMCreateEditor(&editor);
  COM_ENSURE_SUCCESS(hr);

  hr = editor->Open(m_FilePath.get());
  COM_ENSURE_SUCCESS(hr);

  CComPtr<IWMHeaderInfo3> header;
  hr = editor->QueryInterface(&header);
  COM_ENSURE_SUCCESS(hr);
  
  PRBool success;
  rv = SetImageDataInternal(aType, aURL, header, success);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (success) {
    hr = editor->Flush();
    COM_ENSURE_SUCCESS(hr);
  } else {
    hr = editor->Close();
    COM_ENSURE_SUCCESS(hr);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SetImageDataInternal(PRInt32 aType,
                                           const nsAString &aURL,
                                           IWMHeaderInfo3 *aHeader,
                                           PRBool &aSuccess)
{
  NS_ENSURE_ARG_POINTER(aHeader);

  HRESULT hr;
  nsresult rv;
  sbAutoNSTypePtr<WORD> indices;
  
  // find the index of the existing attribute, if any
  WORD indicesCount = 0;
  hr = aHeader->GetAttributeIndices(0xFFFF,
                                    g_wszWMPicture,
                                    NULL,
                                    NULL,
                                    &indicesCount);
  COM_ENSURE_SUCCESS(hr);
  indices = reinterpret_cast<WORD*>(NS_Alloc(sizeof(WORD) * indicesCount));
  NS_ENSURE_TRUE(indices, NS_ERROR_OUT_OF_MEMORY);
  hr = aHeader->GetAttributeIndices(0xFFFF,
                                    g_wszWMPicture,
                                    NULL,
                                    indices.get(),
                                    &indicesCount);
  COM_ENSURE_SUCCESS(hr);
  
  nsCString data;
  WM_PICTURE *picData = (WM_PICTURE*)data.BeginWriting();
  WORD targetIndex = static_cast<WORD>(-1);
  if (indicesCount > 0) {
    for (WORD offset = 0; offset < indicesCount; ++offset) {
      WORD nameLength = 0;
      DWORD dataLength = data.Length();
      WMT_ATTR_DATATYPE type;
      // get the size of the data
      hr = aHeader->GetAttributeByIndexEx(0xFFFF,
                                          indices.get()[offset],
                                          NULL,
                                          &nameLength,
                                          &type,
                                          NULL,
                                          reinterpret_cast<BYTE*>(data.BeginWriting()),
                                          &dataLength);
      // now we can go resize things
      if (hr == NS_E_SDK_BUFFERTOOSMALL) {
        data.SetLength(dataLength);
        dataLength = data.Length();
        nameLength = 0;
        picData = (WM_PICTURE*)data.BeginWriting();
        hr = aHeader->GetAttributeByIndexEx(0xFFFF,
                                            indices.get()[offset],
                                            NULL,
                                            &nameLength,
                                            &type,
                                            NULL,
                                            reinterpret_cast<BYTE*>(data.BeginWriting()),
                                            &dataLength);
      }
      COM_ENSURE_SUCCESS(hr);

      if (type != WMT_TYPE_BINARY) {
        LOG(("%s: index %hu got unknown type %i", __FUNCTION__, offset, type));
        continue;
      }
      
      // look at the picture to see if it's the right type
      if (picData->bPictureType != aType) {
        // the picture is of a different type, don't override
        // NOTE: this only works because both Songbird and WMP use the type IDs
        // from ID3v1
        continue;
      }
      
      // this is the right picture
      targetIndex = indices.get()[offset];
      break;
    }
  }
  
  if (aURL.IsEmpty()) {
    // trying to remove the album art
    if (targetIndex != static_cast<WORD>(-1)) {
      hr = aHeader->DeleteAttribute(0xFFFF, targetIndex);
      COM_ENSURE_SUCCESS(hr);
      
      aSuccess = PR_TRUE;
    } else {
      // it wasn't found anyway, so end up doing nothing
      aSuccess = PR_FALSE;
    }
    return NS_OK;
  }
  
  // we do want to set a picture.
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIFile> artFile;
  rv = fileURL->GetFile(getter_AddRefs(artFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCString dataString;
  rv = sbReadFile(artFile, dataString);
  NS_ENSURE_SUCCESS(rv, rv);

  // detect the image type
  nsCString contentType;
  nsCOMPtr<nsIContentSniffer> contentSniffer = 
    do_ProxiedGetService("@mozilla.org/image/loader;1", &rv); 
  NS_ENSURE_SUCCESS(rv, rv); 

  rv = contentSniffer->GetMIMETypeFromContent(NULL,
                                              reinterpret_cast<const PRUint8*>(dataString.BeginReading()),
                                              dataString.Length(),
                                              contentType); 
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ConvertASCIItoUTF16 contentTypeUnicode(contentType);

  nsString emptyString;
  WM_PICTURE newPicData;
  newPicData.pwszMIMEType = contentTypeUnicode.BeginWriting();
  /* this only works because both songbird and WMA copy the types from id3v1 */
  newPicData.bPictureType = aType;
  newPicData.pwszDescription = emptyString.BeginWriting();
  newPicData.pbData = reinterpret_cast<BYTE*>(dataString.BeginWriting());
  newPicData.dwDataLen = dataString.Length();
  
  if (targetIndex == static_cast<WORD>(-1)) {
    // no existing picture found
    WORD index = 0;
    hr = aHeader->AddAttribute(0,
                               g_wszWMPicture,
                               &index,
                               WMT_TYPE_BINARY,
                               0,
                               reinterpret_cast<const BYTE*>(&newPicData),
                               sizeof(WM_PICTURE));
    COM_ENSURE_SUCCESS(hr);
  } else {
    // there's an existing picture of the right type
    hr = aHeader->ModifyAttribute(0xFFFF,
                                  targetIndex,
                                  WMT_TYPE_BINARY,
                                  NULL,
                                  reinterpret_cast<const BYTE*>(&newPicData),
                                  sizeof(WM_PICTURE));
    COM_ENSURE_SUCCESS(hr);
  }

  aSuccess = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::GetProps(sbIMutablePropertyArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(m_PropertyArray);
  NS_IF_ADDREF(*_retval = m_PropertyArray);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SetProps(sbIMutablePropertyArray *props)
{
  m_PropertyArray = props;
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::GetRequiresMainThread(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  // This handler does not use the channel implementation, 
  // and is threadsafe.
  *_retval = PR_FALSE;
  return NS_OK;
}

nsString 
sbMetadataHandlerWMA::ReadHeaderValue(IWMHeaderInfo3 *aHeaderInfo, 
                                      const nsAString &aKey)
{
  TRACE(("%s[%p]: reading %s",
         __FUNCTION__, this, NS_ConvertUTF16toUTF8(aKey).get()));
  HRESULT hr;
  nsString value;

  // Get the number of indices
  WORD count = 0;
  hr = aHeaderInfo->GetAttributeIndices(0xFFFF,
                                        aKey.BeginReading(),
                                        NULL,
                                        NULL,
                                        &count);
  if (FAILED(hr) || !count) {
    return SBVoidString();
  }

  // Alloc the space for the indices
  sbAutoNSTypePtr<WORD> indices = (WORD*)NS_Alloc(sizeof(WORD) * count);
  if (!indices) {
    NS_ERROR("nsMemory::Alloc failed!");
    return SBVoidString();
  }

  // Ask for the indices
  hr = aHeaderInfo->GetAttributeIndices(0xFFFF,
                                        aKey.BeginReading(),
                                        NULL,
                                        indices.get(),
                                        &count);
  if (FAILED(hr)) {
    return SBVoidString();
  }

  // For now, get the first one?
  WMT_ATTR_DATATYPE type;
  WORD lang;
  DWORD size;
  sbAutoNSTypePtr<BYTE> data;
  WORD namesize;

  // Get the type and size
  hr = aHeaderInfo->GetAttributeByIndexEx(0xFFFF,
                                          *indices.get(),
                                          NULL,
                                          &namesize,
                                          &type,
                                          &lang,
                                          NULL,
                                          &size);
  if (FAILED(hr)) {
    NS_WARNING("GetAttributeByIndexEx failed");
    return SBVoidString();
  }

  // Alloc (documented as "suitably aligned for any kind of variable")
  // in practice, it forwards to malloc()
  data = (BYTE*)nsMemory::Alloc(size);
  if (!data) {
    NS_ERROR("nsMemory::Alloc failed!");
    return SBVoidString();
  }

  // Get the data
  hr = aHeaderInfo->GetAttributeByIndexEx(0xFFFF,
                                          *indices.get(),
                                          NULL,
                                          &namesize,
                                          &type,
                                          &lang,
                                          data.get(),
                                          &size);
  if (FAILED(hr)) {
    NS_WARNING("GetAttributeByIndexEx failed");
    return SBVoidString();
  }

  // Calculate the value
  PRInt32 datatype = 0;

  switch(type) {
      case WMT_TYPE_STRING:
        value.Assign(reinterpret_cast<PRUnichar*>(data.get()));
        break;

      case WMT_TYPE_BINARY:
        break;

      case WMT_TYPE_QWORD: {
        PRInt64 intVal = *(reinterpret_cast<QWORD*>(data.get()));
        if (aKey.EqualsLiteral(WMP_LENGTH)) {
          // "Duration" comes in 100-nanosecond chunks. Wow.
          // Songbird wants it in microseconds.
          intVal /= 10;
        }

        // Convert the long to a string.
        char buf[30];
        PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", intVal);

        value.Append(NS_ConvertASCIItoUTF16(buf, len));
        datatype = 1;
      } break;

      case WMT_TYPE_DWORD: {
        PRUint32 intVal = *(reinterpret_cast<DWORD*>(data.get()));
        if (aKey.EqualsLiteral(WMP_BITRATE)) {
          // Songbird wants bit rate in kbps
          intVal /= 1000;
        }
        value.AppendInt( intVal );
        datatype = 1;
      } break;

      case WMT_TYPE_WORD:
        value.AppendInt( (PRInt32)*(reinterpret_cast<WORD*>(data.get())) );
        datatype = 1;
        break;

      case WMT_TYPE_BOOL:
        value.AppendInt( (PRInt32)*(reinterpret_cast<BOOL*>(data.get())) );
        datatype = 1;
        break;

      case WMT_TYPE_GUID:
        break;

      default:
        NS_WARNING("Value given in an unsupported type");
        break;
  }

  TRACE(("%s[%p]: %s -> %s",
         __FUNCTION__,
         this,
         NS_ConvertUTF16toUTF8(aKey).get(),
         NS_ConvertUTF16toUTF8(value).get()));
  return value;
}

NS_METHOD
sbMetadataHandlerWMA::CreateWMPMediaItem(const nsAString& aFilePath,
                                         IWMPMedia3** aMedia)
{
  HRESULT hr;
  NS_ENSURE_ARG_POINTER(aMedia);

  CComPtr<IWMPPlayer4> player;
  hr = player.CoCreateInstance(__uuidof(WindowsMediaPlayer),
                               NULL,
                               CLSCTX_INPROC_SERVER);
  COM_ENSURE_SUCCESS(hr);

  // Set basic settings
  CComPtr<IWMPSettings> settings;
  hr = player->get_settings(&settings);
  COM_ENSURE_SUCCESS(hr);

  hr = settings->put_autoStart(PR_FALSE);
  COM_ENSURE_SUCCESS(hr);

  CComBSTR uiMode(_T("invisible"));
  hr = player->put_uiMode(uiMode);
  COM_ENSURE_SUCCESS(hr);

  // Make our custom playlist
  CComPtr<IWMPPlaylist> playlist;
  CComBSTR playlistName(_T("SongbirdMetadataPlaylist"));
  hr = player->newPlaylist(playlistName, NULL, &playlist);
  COM_ENSURE_SUCCESS(hr);

  hr = player->put_currentPlaylist(playlist);
  COM_ENSURE_SUCCESS(hr);

  CComBSTR uriString(aFilePath.BeginReading());

  CComPtr<IWMPMedia> newMedia;
  hr = player->newMedia(uriString, &newMedia);
  COM_ENSURE_SUCCESS(hr);

  CComPtr<IWMPMedia3> newMedia3;
  hr = newMedia->QueryInterface(&newMedia3);
  COM_ENSURE_SUCCESS(hr);

  // Now add our new item.
  hr = playlist->appendItem(newMedia);
  COM_ENSURE_SUCCESS(hr);

  // Set our new item as the current one
  CComPtr<IWMPControls> controls;
  hr = player->get_controls(&controls);
  COM_ENSURE_SUCCESS(hr);

  hr = controls->put_currentItem(newMedia);
  COM_ENSURE_SUCCESS(hr);

  *aMedia = newMedia3.Detach();

  return NS_OK;
}

NS_METHOD
sbMetadataHandlerWMA::ReadMetadataWMFSDK(const nsAString& aFilePath,
                                         PRInt32* _retval)
{
  nsresult rv;

  CComPtr<IWMMetadataEditor> reader;
  HRESULT hr = WMCreateEditor(&reader);
  COM_ENSURE_SUCCESS(hr);

  nsAutoString filePath(aFilePath);

  // This will fail for all protected files, so silence the warning
  hr = reader->Open(filePath.get());
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  CComPtr<IWMHeaderInfo3> headerInfo;
  hr = reader->QueryInterface(&headerInfo);
  COM_ENSURE_SUCCESS(hr);

  nsString value, wmpKey;

  for (PRUint32 index = 0; index < NS_ARRAY_LENGTH(kMetadataKeys); ++index) {
    wmpKey.Assign(nsDependentString(kMetadataKeys[index].wmpName));

    value = ReadHeaderValue(headerInfo, wmpKey);

    // If there is a value, add it to the Songbird values.
    if (!value.IsEmpty()) {
      nsAutoString sbKey;
      sbKey.Assign(nsDependentString(kMetadataKeys[index].songbirdName));
      nsresult rv = m_PropertyArray->AppendProperty(sbKey, value);
      if (NS_SUCCEEDED(rv)) {
        *_retval += 1;
      }
      else {
        NS_WARNING("AppendProperty failed!");
      }
    }
  }

  // Figure out whether it's audio or video
  wmpKey.AssignLiteral("HasVideo");
  nsString hasVideo = ReadHeaderValue(headerInfo, wmpKey);

  wmpKey.AssignLiteral("HasAudio");
  nsString hasAudio = ReadHeaderValue(headerInfo, wmpKey);

  value = EmptyString();

  if(hasVideo.EqualsLiteral("1")) {
    value = NS_LITERAL_STRING("video");
  }
  else if(hasAudio.EqualsLiteral("1")) {
    value = NS_LITERAL_STRING("audio");
  }

  rv = m_PropertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                       value);

  if(NS_SUCCEEDED(rv)) {
    *_retval += 1;
  }
  else {
    NS_WARNING("AppendProperty Failed!");
  }

  hr = reader->Close();
  COM_ENSURE_SUCCESS(hr);

  return NS_OK;
}

NS_METHOD
sbMetadataHandlerWMA::ReadMetadataWMP(const nsAString& aFilePath,
                                      PRInt32* _retval)
{
  nsresult rv;
  HRESULT hr;

  CComPtr<IWMPMedia3> newMedia;
  rv = CreateWMPMediaItem(aFilePath, &newMedia);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < NS_ARRAY_LENGTH(kMetadataKeys); ++index) {

    CComBSTR key(kMetadataKeys[index].wmpName);
    nsAutoString metadataValue;
    PRUint32 metadataValueType = 0;

    // Special case for length... and others?
    if (key == WMP_LENGTH) {
      // Songbird needs length in microseconds
      double duration;
      hr = newMedia->get_duration(&duration);
      if (FAILED(hr)) {
        NS_WARNING("get_duration failed!");
        continue;
      }
      double result = duration * PR_USEC_PER_SEC;

      AppendInt(metadataValue, static_cast<PRUint64>(result));
      metadataValueType = 1;
    }
    else {
      CComBSTR value;
      hr = newMedia->getItemInfo(key, &value);
      if (FAILED(hr)) {
        NS_WARNING("getItemInfo failed!");
        continue;
      }
      if (key == WMP_BITRATE) {
        // WMP returns bitrate in bits/sec, Songbird wants kbps/sec
        metadataValue.Assign(value.m_str, value.Length() - 3);
      } else if (key == WMP_PROTECTED) {
        // Songbird wants (nothing) or "1"
        if (value.Length() > 0) {
          metadataValue.AssignLiteral("1");
        }
      } else {
        metadataValue.Assign(value.m_str);
      }
    }

    if (!metadataValue.IsEmpty()) {
      nsAutoString sbKey;
      sbKey.Assign(nsDependentString(kMetadataKeys[index].songbirdName));
      nsresult rv =
        m_PropertyArray->AppendProperty(sbKey, metadataValue);
      if (NS_SUCCEEDED(rv))
        *_retval += 1;
      else
        NS_WARNING("SetValue failed!");
    }
  }

  // Simply check the file extension to determine if this is a video file.
  nsString value(NS_LITERAL_STRING("audio"));  // assume audio

  nsCOMPtr<nsILocalFile> localFile =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  if (NS_SUCCEEDED(rv) && localFile) {
    rv = localFile->InitWithPath(aFilePath);
    if (NS_SUCCEEDED(rv)) {
      nsString leafName;
      rv = localFile->GetLeafName(leafName);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 index = leafName.RFindChar('.');
      if (index >= 0) {
        nsString extension(StringTail(leafName, leafName.Length() -1 - index));

        if (extension.EqualsLiteral("wmv") ||
            extension.EqualsLiteral("wm"))
        {
          value.AssignLiteral("video");
        }
      }
    }
  }

  rv = m_PropertyArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                       value);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMetadataHandlerWMA::ReadAlbumArtWMFSDK(const nsAString &aFilePath,
                                         nsACString      &aMimeType,
                                         PRUint32        *aDataLen,
                                         PRUint8        **aData)
{
  CComPtr<IWMMetadataEditor> reader;
  HRESULT hr = WMCreateEditor(&reader);
  COM_ENSURE_SUCCESS(hr);
  
  nsAutoString filePath(aFilePath);

  // This will fail for all protected files, so silence the warning
  hr = reader->Open(filePath.get());
  if (FAILED(hr)) {
    TRACE(("%s: failed to open file %s, assuming protected",
           __FUNCTION__,
           NS_ConvertUTF16toUTF8(filePath).get()));
    return NS_ERROR_FAILURE;
  }

  CComPtr<IWMHeaderInfo3> headerInfo;
  hr = reader->QueryInterface(&headerInfo);
  COM_ENSURE_SUCCESS(hr);

  // Get the number of indices
  WORD count = 0;
  hr = headerInfo->GetAttributeIndices(0xFFFF,
                                       g_wszWMPicture,
                                       NULL,
                                       NULL,
                                       &count);
  COM_ENSURE_SUCCESS(hr);
  NS_ENSURE_TRUE(count, NS_ERROR_NO_CONTENT);

  // Alloc the space for the indices
  sbAutoNSTypePtr<WORD> indices = (WORD*)nsMemory::Alloc(sizeof(WORD) * count);
  NS_ENSURE_TRUE(indices, NS_ERROR_OUT_OF_MEMORY);

  // Ask for the indices
  hr = headerInfo->GetAttributeIndices(0xFFFF,
                                       g_wszWMPicture,
                                       NULL,
                                       indices.get(),
                                       &count);
  COM_ENSURE_SUCCESS(hr);

  // For now, get the first one?
  WMT_ATTR_DATATYPE type;
  WORD lang;
  DWORD size;
  WORD namesize;

  // Get the type and size
  hr = headerInfo->GetAttributeByIndexEx(0xFFFF,
                                         *(indices.get()),
                                         NULL,
                                         &namesize,
                                         &type,
                                         &lang,
                                         NULL,
                                         &size);
  COM_ENSURE_SUCCESS(hr);

  // Alloc
  sbAutoNSTypePtr<BYTE> data = (BYTE*)nsMemory::Alloc(size);
  NS_ENSURE_TRUE(data, NS_ERROR_OUT_OF_MEMORY);

  // Get the data
  hr = headerInfo->GetAttributeByIndexEx(0xFFFF,
                                         *(indices.get()),
                                         NULL,
                                         &namesize,
                                         &type,
                                         &lang,
                                         data.get(),
                                         &size);
  COM_ENSURE_SUCCESS(hr);
  NS_ENSURE_TRUE(type == WMT_TYPE_BINARY, NS_ERROR_UNEXPECTED);

  // get the picture data out (remembering to use a copy)
  WM_PICTURE *picData = (WM_PICTURE*)data.get();
  CopyUTF16toUTF8(nsDependentString(picData->pwszMIMEType), aMimeType);
  *aData = static_cast<PRUint8*>(nsMemory::Clone(picData->pbData, picData->dwDataLen));
  NS_ENSURE_TRUE(*aData, NS_ERROR_OUT_OF_MEMORY);
  *aDataLen = picData->dwDataLen;

  hr = reader->Close();
  return NS_OK;
}


NS_METHOD
sbMetadataHandlerWMA::ReadAlbumArtWMP(const nsAString &aFilePath,
                                      nsACString      &aMimeType,
                                      PRUint32        *aDataLen,
                                      PRUint8        **aData)
{
  nsresult rv;
  HRESULT hr;
  
  CComPtr<IWMPMedia3> newMedia;
  rv = CreateWMPMediaItem(aFilePath, &newMedia);
  NS_ENSURE_SUCCESS(rv, rv);

  // read out the metadata
  long count = 0;
  CComBSTR key(g_wszWMPicture);
  hr = newMedia->getAttributeCountByType(key, NULL, &count);
  COM_ENSURE_SUCCESS(hr);

  for (long idx = 0; idx < count; ++idx) {
    CComVariant var;
    hr = newMedia->getItemInfoByType(key, NULL, idx, &var);
    COM_ENSURE_SUCCESS(hr);

    CComPtr<IWMPMetadataPicture> picture;
    switch(V_VT(&var)) {
      case VT_DISPATCH:
        hr = var.pdispVal->QueryInterface(&picture);
        break;
      case VT_DISPATCH | VT_BYREF:
        if (!var.ppdispVal) {
          nsresult __rv = NS_ERROR_INVALID_POINTER;
          NS_ENSURE_SUCCESS_BODY(hr, hr);
          continue;
        }
        hr = (*(var.ppdispVal))->QueryInterface(&picture);
        break;
      default:
        TRACE(("%s: don't know how to deal with WM/Picture variant type %i",
               __FUNCTION__, var.vt));
        continue;
    }

    if (FAILED(hr) || NS_UNLIKELY(!picture)) {
      nsresult __rv = NS_ERROR_NO_INTERFACE;
      NS_ENSURE_SUCCESS_BODY(hr, hr);
      continue;
    }

    /* get the picture data
     * IWMPMetadataPicture will only give us a URL (of the form
     * "vnd.ms.wmhtml://localhost/WMP<guid>.jpg"), and doesn't directly give us
     * any way to access the stream.  Fortunately, they also get dumped into
     * Temporary Internet Files (with the URL intact).
     * So, we look up the URL in the IE cache and hope it's still there.  It can
     * be resolved to a local file (in a hidden + system directory), which we
     * can then read normally.
     */
    CComBSTR url;
    hr = picture->get_URL(&url);
    COM_ENSURE_SUCCESS(hr);

    DWORD entrySize = 0;
    BOOL success = GetUrlCacheEntryInfo(url, NULL, &entrySize);
    NS_ENSURE_TRUE(!success && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                   NS_ERROR_FAILURE);
    NS_ASSERTION(entrySize > 0, "Unexpected entry size");

    sbAutoNSTypePtr<INTERNET_CACHE_ENTRY_INFO> cacheInfo =
      (INTERNET_CACHE_ENTRY_INFO*)NS_Alloc(entrySize);
    NS_ENSURE_TRUE(cacheInfo, NS_ERROR_OUT_OF_MEMORY);

    cacheInfo.get()->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

    success = GetUrlCacheEntryInfo(url, cacheInfo.get(), &entrySize);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    // at this point, cacheInfo.get()->lpszLocalFileName is the local file path
    nsCOMPtr<nsILocalFile> file =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = file->InitWithPath(nsDependentString(cacheInfo.get()->lpszLocalFileName));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 fileSize;
    rv = file->GetFileSize(&fileSize);
    NS_ENSURE_SUCCESS(rv, rv);

    sbAutoNSTypePtr<PRUint8> fileData = (PRUint8*)NS_Alloc(static_cast<PRSize>(fileSize));
    NS_ENSURE_TRUE(fileData, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIFileInputStream> fileStream =
      do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fileStream->Init(file, PR_RDONLY, -1, nsIFileInputStream::CLOSE_ON_EOF);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 bytesRead;
    rv = fileStream->Read((char*)fileData.get(), fileSize, &bytesRead);
    NS_ENSURE_SUCCESS(rv, rv);

    // the stream is actually already closed (CLOSE_ON_EOF), but no harm to be
    // explicit about it
    rv = fileStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    // get the picture type
    CComBSTR mimeType;
    hr = picture->get_mimeType(&mimeType);
    COM_ENSURE_SUCCESS(hr);
    aMimeType.Assign(NS_ConvertUTF16toUTF8(nsDependentString(mimeType)));

    *aDataLen = bytesRead;
    *aData = fileData.forget();

    return NS_OK;
  }

  // if we get here, we ran through the pictures and found nothing useful
  return NS_ERROR_FAILURE;
}

/* PRBool isDRMProtected (in AString aPath); */
NS_IMETHODIMP
sbMetadataHandlerWMA::IsDRMProtected(const nsAString & aPath,
                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  BOOL isProtected; // needed for data type conversion
  HRESULT hr = WMIsContentProtected(aPath.BeginReading(), &isProtected);
  *_retval = isProtected;
  return SUCCEEDED(hr) ? NS_OK : NS_ERROR_FAILURE;
}
