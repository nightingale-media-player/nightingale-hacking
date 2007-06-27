/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/**
* \file MetadataHandlerWMA.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include "MetadataHandlerWMA.h"

#include <nsIChannel.h>
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

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbMetadataHandlerWMA");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif

// DEFINES ====================================================================

#define COM_ENSURE_SUCCESS(_val)                           \
  NS_ENSURE_TRUE(SUCCEEDED(_val), NS_ERROR_FAILURE)

#define countof(_array)                                    \
  sizeof(_array) / sizeof(_array[0])

#define SB_ARTIST       "artist"
#define SB_TITLE        "title"
#define SB_ALBUM        "album"
#define SB_GENRE        "genre"
#define SB_TRACKNO      "track_no"
#define SB_VENDOR       "vendor"
#define SB_COPYRIGHT    "Copyright"
#define SB_COMPOSER     "composer"
#define SB_CONDUCTOR    "conductor"
#define SB_DIRECTOR     "director"
#define SB_BPM          "bpm"
#define SB_LYRICS       "lyrics"
#define SB_YEAR         "year"
#define SB_BITRATE      "bitrate"
#define SB_RATING       "rating"
#define SB_DESCRIPTION  "description"
#define SB_COMMENT      "comment"
#define SB_LENGTH       "length"

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

#define KEY_MAP_HEAD                /* nothing */
#define KEY_MAP_FIRST_ENTRY(_entry) SB_##_entry, WMP_##_entry
#define KEY_MAP_ENTRY(_entry)       , SB_##_entry, WMP_##_entry
#define KEY_MAP_LAST_ENTRY(_entry)  KEY_MAP_ENTRY(_entry)
#define KEY_MAP_TAIL                /* nothing */

// These are the keys we're going to read from the WM interface 
// and push to the SB interface.
static const char* kMetadataKeys[] = {
  KEY_MAP_HEAD
    KEY_MAP_FIRST_ENTRY(ARTIST)
      KEY_MAP_ENTRY(TITLE)        KEY_MAP_ENTRY(ALBUM)
      KEY_MAP_ENTRY(GENRE)        KEY_MAP_ENTRY(TRACKNO)
      KEY_MAP_ENTRY(VENDOR)       KEY_MAP_ENTRY(COPYRIGHT)
      KEY_MAP_ENTRY(COMPOSER)     KEY_MAP_ENTRY(CONDUCTOR)
      KEY_MAP_ENTRY(DIRECTOR)     KEY_MAP_ENTRY(BPM)
      KEY_MAP_ENTRY(LYRICS)       KEY_MAP_ENTRY(YEAR)
      KEY_MAP_ENTRY(BITRATE)      KEY_MAP_ENTRY(RATING)
      KEY_MAP_ENTRY(DESCRIPTION)  KEY_MAP_ENTRY(COMMENT)
    KEY_MAP_LAST_ENTRY(LENGTH)
  KEY_MAP_TAIL
};

#define kMetadataArrayCount countof(kMetadataKeys)

static PRBool sCOMInitialized = PR_FALSE;
// FUNCTIONS ==================================================================

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataHandlerWMA, sbIMetadataHandler)

sbMetadataHandlerWMA::sbMetadataHandlerWMA() :
  m_Completed(PR_FALSE),
  m_COMInitialized(PR_FALSE)
{
  HRESULT hr = CoInitialize(0);
  NS_ASSERTION(SUCCEEDED(hr), "CoInitialize failed!");
  if (SUCCEEDED(hr))
    m_COMInitialized = PR_TRUE;

  // Get a new values object.
  nsresult rv;
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1", &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create MetadataValues");
}

sbMetadataHandlerWMA::~sbMetadataHandlerWMA()
{
  if (m_COMInitialized)
    CoUninitialize();
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SupportedMIMETypes(PRUint32* aMIMECount,
                                         PRUnichar*** aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SupportedFileExtensions(PRUint32* eExtCount,
                                              PRUnichar*** aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
  m_Channel = aURLChannel;

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

  m_Values = nsnull;
  m_Channel = nsnull;
  m_ChannelHandler = nsnull;

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
        ( strUrl.Find( ".asf", PR_TRUE ) != -1 ) ||
        ( strUrl.Find( ".asx", PR_TRUE ) != -1 ) ||
        ( strUrl.Find( ".avi", PR_TRUE ) != -1 )
     )
    *_retval = 1;
  else
    *_retval = -1;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Read(PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  // We're never asynchronous.
  m_Completed = PR_TRUE;
  *_retval = 0;

  NS_ENSURE_TRUE(m_Channel, NS_ERROR_FAILURE);

  nsresult rv;
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
  nsAutoString filePathUTF16;
  rv = file->GetPath(filePathUTF16);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we have something, try it.
  if ( filePathUTF16.Length() ) {
    // Try to use the WMFSDK for this data because it's probably faster than
    // initializing the Windows Media Player ActiveX control.
    rv = ReadMetadataWMFSDK(filePathUTF16, _retval);
    if (NS_SUCCEEDED(rv))
      return rv;
    
    // The WMF SDK failed on the file (probably because it is protected), so let
    // the ActiveX control take a crack at it.
    rv = ReadMetadataWMP(filePathUTF16, _retval);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_INVALID_ARG; // This will cause a useful warning in the metadata job.
}

NS_IMETHODIMP
sbMetadataHandlerWMA::Write(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;
  return NS_OK;
} //Write

NS_IMETHODIMP
sbMetadataHandlerWMA::GetValues(sbIMetadataValues **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_IF_ADDREF(*_retval = m_Values);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerWMA::SetValues(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
sbMetadataHandlerWMA::ReadMetadataWMFSDK(const nsAString& aFilePath,
                                         PRInt32* _retval)
{
  CComPtr<IWMSyncReader> reader;
  HRESULT hr = WMCreateSyncReader(nsnull, 0, &reader);
  COM_ENSURE_SUCCESS(hr);

  nsAutoString filePath(aFilePath);

  // This will fail for all protected files, so silence the warning
  hr = reader->Open(filePath.get());
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  CComPtr<IWMHeaderInfo3> headerInfo;
  hr = reader->QueryInterface(&headerInfo);
  COM_ENSURE_SUCCESS(hr);

  PRUint32 count = kMetadataArrayCount;
  for (PRUint32 index = 0; index < count; index += 2) {

    nsAutoString wmpKey;
    wmpKey.AssignLiteral(kMetadataKeys[index + 1]);

    // Get the number of indices
    WORD count = 0;
    hr = headerInfo->GetAttributeIndices(0xFFFF, wmpKey.get(), nsnull, nsnull, &count);
    if (FAILED(hr) || !count) {
      continue;
    }

    // Alloc the space for the indices
    WORD* indices = (WORD*)nsMemory::Alloc(sizeof(WORD) * count);
    if (!indices) {
      NS_ERROR("nsMemory::Alloc failed!");
      continue;
    }

    // Ask for the indices
    hr = headerInfo->GetAttributeIndices(0xFFFF, wmpKey.get(), nsnull, indices,
                                         &count);
    if (FAILED(hr)) {
      nsMemory::Free(indices);
      continue;
    }

    // For now, get the first one?
    WMT_ATTR_DATATYPE type;
    WORD lang;
    DWORD size;
    BYTE *data;
    WORD namesize;

    // Get the type and size
    hr = headerInfo->GetAttributeByIndexEx(0xFFFF, *indices, nsnull, &namesize,
                                           &type, &lang, nsnull, &size);
    if (FAILED(hr)) {
      NS_WARNING("GetAttributeByIndexEx failed");
      nsMemory::Free(indices);
      continue;
    }

    // Alloc
    data = (BYTE*)nsMemory::Alloc(size);
    if (!data) {
      NS_ERROR("nsMemory::Alloc failed!");
      nsMemory::Free(indices);
      continue;
    }

    // Get the data
    hr = headerInfo->GetAttributeByIndexEx(0xFFFF, *indices, NULL, &namesize,
                                           &type, &lang, data, &size);
    if (FAILED(hr)) {
      NS_WARNING("GetAttributeByIndexEx failed");
      nsMemory::Free(data);
      nsMemory::Free(indices);
      continue;
    }

    // Calculate the value
    PRInt32 datatype = 0;
    nsAutoString value;
    switch(type) {
      case WMT_TYPE_STRING:
        value.Assign((PRUnichar*)data);
        break;

      case WMT_TYPE_BINARY:
        break;

      case WMT_TYPE_QWORD: {
        PRInt64 intVal = *((QWORD*)data);
        // "Duration" comes in 100-nanosecond chunks. Wow.
        if (wmpKey.EqualsLiteral("Duration")) {
          intVal /= 10000;
        }

        // Convert the long to a string.
        char buf[30];
        PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", intVal);

        value.Append(NS_ConvertASCIItoUTF16(buf, len));
        datatype = 1;
      } break;

      case WMT_TYPE_DWORD:
        value.AppendInt( (PRUint32)*(DWORD*)data ); // Whee!
        datatype = 1;
        break;

      case WMT_TYPE_WORD:
        value.AppendInt( (PRInt32)*(WORD*)data );
        datatype = 1;
        break;

      case WMT_TYPE_BOOL:
        value.AppendInt( (PRInt32)*(BOOL*)data );
        datatype = 1;
        break;

      case WMT_TYPE_GUID:
        break;

      default:
        NS_WARNING("Value given in an unsupported type");
        break;
    }

    // If there is a value, add it to the Songbird values.
    if (!value.IsEmpty()) {
      nsAutoString sbKey;
      sbKey.AssignLiteral(kMetadataKeys[index]);
      nsresult rv = m_Values->SetValue(sbKey, value, datatype);
      if (NS_SUCCEEDED(rv))
        *_retval += 1;
      else
        NS_WARNING("SetValue failed!");
    }

    // Free the space for the indices
    nsMemory::Free(data);
    nsMemory::Free(indices);
}

  hr = reader->Close();
  COM_ENSURE_SUCCESS(hr);

  return NS_OK;
}

NS_METHOD
sbMetadataHandlerWMA::ReadMetadataWMP(const nsAString& aFilePath,
                                      PRInt32* _retval)
{
  CComPtr<IWMPPlayer3> player;
  HRESULT hr = player.CoCreateInstance(__uuidof(WindowsMediaPlayer),
                                       nsnull, CLSCTX_INPROC_SERVER);
  COM_ENSURE_SUCCESS(hr);

  // Set basic settings
  CComPtr<IWMPSettings> settings;
  hr = player->get_settings(&settings);
  COM_ENSURE_SUCCESS(hr);

  hr = settings->put_autoStart(PR_FALSE);
  COM_ENSURE_SUCCESS(hr);

  VARIANT_BOOL mute = PR_TRUE;
  hr = settings->put_mute(mute);
  COM_ENSURE_SUCCESS(hr);

  CComBSTR uiMode(_T("invisible"));
  hr = player->put_uiMode(uiMode);
  COM_ENSURE_SUCCESS(hr);

  // Make our custom playlist
  CComPtr<IWMPCore3> core3;
  hr = player->QueryInterface(&core3);
  COM_ENSURE_SUCCESS(hr);

  CComPtr<IWMPPlaylist> playlist;
  CComBSTR playlistName(_T("SongbirdMetadataPlaylist"));
  hr = core3->newPlaylist(playlistName, nsnull, &playlist);
  COM_ENSURE_SUCCESS(hr);

  hr = player->put_currentPlaylist(playlist);
  COM_ENSURE_SUCCESS(hr);

  // Make a new media item from our file
  CComPtr<IWMPPlayer4> wmp4;
  hr = player->QueryInterface(&wmp4);
  COM_ENSURE_SUCCESS(hr);

  nsAutoString strPath(aFilePath);
  CComBSTR uriString(strPath.get());

  CComPtr<IWMPMedia> newMedia;
  hr = wmp4->newMedia(uriString, &newMedia);
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

  PRUint32 count = kMetadataArrayCount;
  for (PRUint32 index = 0; index < count; index += 2) {

    CComBSTR key(kMetadataKeys[index + 1]);
    nsAutoString metadataValue;
    PRUint32 metadataValueType = 0;

    // Special case for length... and others?
    if (key == WMP_LENGTH) {
      double duration;
      hr = newMedia->get_duration(&duration);
      if (FAILED(hr)) {
        NS_WARNING("get_duration failed!");
        continue;
      }
      PRUint64 result;
      LL_MUL(result, duration, 1000);

      metadataValue.AppendInt((PRInt64)result);
      metadataValueType = 1;
    }
    else {
      CComBSTR value;
      hr = newMedia->getItemInfo(key, &value);
      if (FAILED(hr)) {
        NS_WARNING("getItemInfo failed!");
        continue;
      }
      metadataValue.Assign(value.m_str);
    }

    if (!metadataValue.IsEmpty()) {
      nsAutoString sbKey;
      sbKey.AssignLiteral(kMetadataKeys[index]);
      nsresult rv =
        m_Values->SetValue(sbKey, metadataValue, metadataValueType);
      if (NS_SUCCEEDED(rv))
        *_retval += 1;
      else
        NS_WARNING("SetValue failed!");
    }
  }

  return NS_OK;
}
