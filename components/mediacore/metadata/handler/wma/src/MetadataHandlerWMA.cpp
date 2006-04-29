/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataHandlerWMA.h"

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>
#include <necko/nsNetUtil.h>

#include <xpcom/nsEscape.h>

#include <wmsdk.h>

// DEFINES ====================================================================

// These are the keys we're going to read from the WM interface 
// and push to the SB interface.
const nsString wm_keys[][2] =
{
  { NS_LITERAL_STRING("wm_key"),            NS_LITERAL_STRING("sb_key")  },
  { NS_LITERAL_STRING("Author"),            NS_LITERAL_STRING("artist")  },
  { NS_LITERAL_STRING("Title"),             NS_LITERAL_STRING("title")  },
  { NS_LITERAL_STRING("WM/AlbumTitle"),     NS_LITERAL_STRING("album")  },
  { NS_LITERAL_STRING("WM/Genre"),          NS_LITERAL_STRING("genre")  },
  { NS_LITERAL_STRING("WM/TrackNumber"),    NS_LITERAL_STRING("track_no")  },
  { NS_LITERAL_STRING("WM/ToolName"),       NS_LITERAL_STRING("vendor")  },
  { NS_LITERAL_STRING("Copyright"),         NS_LITERAL_STRING("copyright")  },
  { NS_LITERAL_STRING("WM/Composer"),       NS_LITERAL_STRING("composer")  },
  { NS_LITERAL_STRING("WM/Conductor"),      NS_LITERAL_STRING("conductor")  },
  { NS_LITERAL_STRING("WM/Director"),       NS_LITERAL_STRING("director")  },
  { NS_LITERAL_STRING("WM/BeatsPerMinute "),NS_LITERAL_STRING("bpm")  },
  { NS_LITERAL_STRING("WM/Lyrics"),         NS_LITERAL_STRING("lyrics")  },
  { NS_LITERAL_STRING("WM/Year"),           NS_LITERAL_STRING("year")  },
  { NS_LITERAL_STRING("Bitrate"),           NS_LITERAL_STRING("bitrate")  },
  { NS_LITERAL_STRING("Duration"),          NS_LITERAL_STRING("length")  },
  { NS_LITERAL_STRING("Rating"),            NS_LITERAL_STRING("rating")  },
  { NS_LITERAL_STRING("Description"),       NS_LITERAL_STRING("description")  },
  { NS_LITERAL_STRING("Duration"),          NS_LITERAL_STRING("length")  },
  { NS_LITERAL_STRING("Duration"),          NS_LITERAL_STRING("length")  },
  { nsString(), nsString() } // Blank signals the end.
};

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataHandlerWMA, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerWMA::sbMetadataHandlerWMA()
{
} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerWMA::~sbMetadataHandlerWMA()
{

} //dtor

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out wstring aMIMETypes); */
NS_IMETHODIMP sbMetadataHandlerWMA::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out wstring aExts); */
NS_IMETHODIMP sbMetadataHandlerWMA::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
/* nsIChannel GetChannel (); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetChannel(nsIChannel **_retval)
{
  *_retval = m_Channel.get();
  (*_retval)->AddRef();

  return NS_OK;
} //GetChannel

//-----------------------------------------------------------------------------
/* PRInt32 SetChannel (in nsIChannel urlChannel); */
NS_IMETHODIMP sbMetadataHandlerWMA::SetChannel(nsIChannel *urlChannel, PRInt32 *_retval)
{
  *_retval = 0;
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerWMA::OnChannelData( nsISupports *channel )
{
  return NS_OK;
}

NS_IMETHODIMP sbMetadataHandlerWMA::Completed(PRBool *_retval)
{
  *_retval = m_Completed;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerWMA::Close()
{
  if ( m_ChannelHandler.get() ) 
    m_ChannelHandler->Close();

  m_Values = nsnull;
  m_Channel = nsnull;
  m_ChannelHandler = nsnull;

  return NS_OK;
} //Close

NS_IMETHODIMP sbMetadataHandlerWMA::Vote(const PRUnichar *url, PRInt32 *_retval )
{
  nsString strUrl( url );

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
} //Vote

//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerWMA::Read(PRInt32 *_retval)
{
  nsresult nRet = NS_ERROR_UNEXPECTED;

  *_retval = 0;
  if(!m_Channel)
  {
    *_retval = -1;
    return NS_ERROR_FAILURE;
  }

  // Get a new values object.
  m_Values = do_CreateInstance("@songbird.org/Songbird/MetadataValues;1");
  m_Values->Clear();
  if(!m_Values.get())
  {
    *_retval = -1;
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> pURI;
  nRet = m_Channel->GetURI(getter_AddRefs(pURI));
  if(NS_FAILED(nRet)) return nRet;

  nsCString cstrScheme, cstrPath;
  pURI->GetScheme(cstrScheme);
  pURI->GetPath(cstrPath);

  if(cstrScheme.Equals(NS_LITERAL_CSTRING("file")))
  {

#if defined(XP_WIN)
    nsCString::iterator itBegin, itEnd;

    if(StringBeginsWith(cstrPath, NS_LITERAL_CSTRING("/")))
      cstrPath.Cut(0, 1);

    cstrPath.BeginWriting(itBegin);
    cstrPath.EndWriting(itEnd);

    while(itBegin != itEnd)
    {
      if( (*itBegin) == '/') (*itBegin) = '\\';
      itBegin++;
    }
#endif

    // ?? Local file
    char *u8url = const_cast<char *>(NS_UnescapeURL(cstrPath).get());
    NS_ConvertUTF8toUTF16 u16url( cstrPath );

    // Do it all here.
    HRESULT hr = S_OK;

    hr = WMCreateSyncReader( NULL, 0, &m_pReader );
    if ( FAILED( hr ) )
    {
      printf( "WMA METADATA %s: Could not create Reader: (hr=0x%08x)\n", u8url, hr );
      return NS_OK;
    }

    hr = m_pReader->Open( u16url.get() );
    if ( FAILED( hr ) )
    {
      printf( "WMA METADATA %s: Could not open. error: (hr=0x%08x)\n", u8url, hr );
      return NS_OK;
    }

    IWMHeaderInfo3 *hi3 = NULL;
    hr = m_pReader->QueryInterface( IID_IWMHeaderInfo3, (void **)&hi3 );
    if ( FAILED( hr ) )
    {
      printf( "WMA METADATA %s: QI for IWMHeaderInfo3 failed: (hr=0x%08x)\n", u8url, hr );
      return NS_OK;
    }

    // Read the array of attribute keys to scan
    for ( int i = 0; wm_keys[i][0].Length(); i++ )
    {
      ReadMetadata( wm_keys[i][0], wm_keys[i][1], hi3 );
    }

    printf( "WMA METADATA: %s\n", u8url );
  }
  else
  {
    // ?? Remote file
  }

  return nRet;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerWMA::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValuesMap (); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetValuesMap(sbIMetadataValues **_retval)
{
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValuesMap (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerWMA::SetValuesMap(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/* PRInt32 GetNumAvailableTags (); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetNumAvailableTags(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //GetNumAvailableTags

//-----------------------------------------------------------------------------
/* void GetAvailableTags (out PRUint32 tagCount, [array, size_is (tagCount), retval] out wstring tags); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetAvailableTags(PRUint32 *tagCount, PRUnichar ***tags)
{
  return NS_OK;
} //GetAvailableTags

//-----------------------------------------------------------------------------
/* wstring GetTag (in wstring tagName); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetTag(const PRUnichar *tagName, PRUnichar **_retval)
{

  return NS_OK;
} //GetTag

//-----------------------------------------------------------------------------
/* PRInt32 SetTag (in wstring tagName, in wstring tagValue); */
NS_IMETHODIMP sbMetadataHandlerWMA::SetTag(const PRUnichar *tagName, const PRUnichar *tagValue, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTag

//-----------------------------------------------------------------------------
/* void GetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, out PRUint32 valueCount, [array, size_is (valueCount), retval] out wstring values); */
NS_IMETHODIMP sbMetadataHandlerWMA::GetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 *valueCount, PRUnichar ***values)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //GetTags

//-----------------------------------------------------------------------------
/* PRInt32 SetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, in PRUint32 valueCount, [array, size_is (valueCount)] in wstring values); */
NS_IMETHODIMP sbMetadataHandlerWMA::SetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 valueCount, const PRUnichar **values, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTags

void sbMetadataHandlerWMA::ReadMetadata( const nsString &ms_key, const nsString &sb_key, IWMHeaderInfo3 *hi3 )
{
  HRESULT hr = S_OK;

  // Number of indices
  WORD count = 0;

  // Ask how many indices
  hr = hi3->GetAttributeIndices( 0xFFFF, ms_key.get(), NULL, NULL, &count );
  if ( FAILED( hr ) || !count )
  {
    return;
  }

  // Alloc the space for the indices
  WORD *indices = (WORD *)nsMemory::Alloc( sizeof(WORD) * count );

  // Ask for the indices
  hr = hi3->GetAttributeIndices( 0xFFFF, ms_key.get(), NULL, indices, &count );
  if ( FAILED( hr ) )
  {
    nsMemory::Free( indices );
    return;
  }

  // For now, get the first one?
  WMT_ATTR_DATATYPE type;
  WORD lang;
  DWORD size;
  BYTE *data;
  WORD namesize;

  // Get the type and size
  hr = hi3->GetAttributeByIndexEx( 0xFFFF, *indices, NULL, &namesize, &type, &lang, NULL, &size );
  if ( FAILED( hr ) )
  {
    nsMemory::Free( indices );
    return;
  }

  // Alloc
  data = (BYTE *)nsMemory::Alloc( size );

  // Get the data
  hr = hi3->GetAttributeByIndexEx( 0xFFFF, *indices, NULL, &namesize, &type, &lang, data, &size );
  if ( FAILED( hr ) )
  {
    nsMemory::Free( data );
    nsMemory::Free( indices );
    return;
  }

  // Calculate the value
  nsString value;
  switch( type )
  {
    case WMT_TYPE_STRING:
      value = (PRUnichar *)data;
      break;
    case WMT_TYPE_BINARY:
      break;
    case WMT_TYPE_DWORD:
    case WMT_TYPE_BOOL:
      value.AppendInt( (PRUint32)*(DWORD*)data ); // Whee!
      break;
    case WMT_TYPE_QWORD:
      break;
    case WMT_TYPE_WORD:
      break;
    case WMT_TYPE_GUID:
      break;
  }

  // If there is a value, add it to the Songbird values.
  if ( value.Length() )
  {
    m_Values->SetValue( sb_key.get(), value.get(), 0 );
  }

  // Free the space for the indices
  nsMemory::Free( data );
  nsMemory::Free( indices );
}
