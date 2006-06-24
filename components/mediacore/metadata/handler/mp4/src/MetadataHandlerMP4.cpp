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
* \file MetadataHandlerMP4.cpp
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataHandlerMP4.h"

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>
#include <necko/nsNetUtil.h>

#include <string/nsReadableUtils.h>
#include <xpcom/nsEscape.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataHandlerMP4, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerMP4::sbMetadataHandlerMP4()
{
} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerMP4::~sbMetadataHandlerMP4()
{

} //dtor

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out wstring aMIMETypes); */
NS_IMETHODIMP sbMetadataHandlerMP4::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out wstring aExts); */
NS_IMETHODIMP sbMetadataHandlerMP4::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
/* nsIChannel GetChannel (); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetChannel(nsIChannel **_retval)
{
  *_retval = m_Channel.get();
  (*_retval)->AddRef();

  return NS_OK;
} //GetChannel

//-----------------------------------------------------------------------------
/* PRInt32 SetChannel (in nsIChannel urlChannel); */
NS_IMETHODIMP sbMetadataHandlerMP4::SetChannel(nsIChannel *urlChannel, PRInt32 *_retval)
{
  *_retval = 0;
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerMP4::OnChannelData( nsISupports *channel )
{
#if 0
  nsCOMPtr<sbIMetadataChannel> mc( do_QueryInterface(channel) ) ;

  if ( mc.get() )
  {
    try
    {
      if ( !m_Completed )
      {
/*
        // How do I calculate length without bitrate?
        PRUnichar *bitrate;
        m_Values->GetValue( NS_LITERAL_STRING("bitrate").get(), &bitrate );
        if ( bitrate )
        {
        }
*/
      }
    }
    catch ( const MetadataHandlerMP4Exception err )
    {
      PRBool completed = false;
      mc->Completed( &completed );
      // If it's a tiny file, it's probably a 404 error
      if ( completed || ( err.m_Seek > ( err.m_Size - 1024 ) ) )
      {
        // If it's a big file, this means it's an MP4v1 and it needs to seek to the end of the track?  Ooops.
        m_Completed = true;
      }

      // Otherwise, take another spin around and try again.
    }
  }
#else
  m_Completed = true;
#endif
  return NS_OK;
}

NS_IMETHODIMP sbMetadataHandlerMP4::Completed(PRBool *_retval)
{
  *_retval = m_Completed;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerMP4::Close()
{
  if ( m_ChannelHandler.get() ) 
    m_ChannelHandler->Close();

  m_Values = nsnull;
  m_Channel = nsnull;
  m_ChannelHandler = nsnull;

  return NS_OK;
} //Close

NS_IMETHODIMP sbMetadataHandlerMP4::Vote(const PRUnichar *url, PRInt32 *_retval )
{
  nsString strUrl( url );

  if ( ( strUrl.Find( ".mp4", PR_TRUE ) != -1 ) || ( strUrl.Find( ".m4a", PR_TRUE ) != -1 ) )
    *_retval = 1;
  else
    *_retval = -1;

  return NS_OK;
} //Close

extern "C" void static_callback( const char *atom_path, const char *value_string, void *context_data )
{
  if ( context_data )
  {
    sbMetadataHandlerMP4 *that = reinterpret_cast<sbMetadataHandlerMP4 *>( context_data );
    that->callback( atom_path, value_string );
  }
}

void sbMetadataHandlerMP4::callback( const char *atom_path, const char *value_string )
{
  nsCString atom( atom_path );
  nsString key, value;
  
  // "covr.data" is album art.  deal with this later.
  if ( key.Find("covr.data") == -1 )
    value = NS_ConvertUTF8toUTF16(value_string);

  // Mozilla Find can't handle high-bit characters.  Grrr.
  if ( atom.Find("nam.data") != -1 )
    key = NS_LITERAL_STRING("title");
  else if ( atom.Find("ART.data") != -1 )
    key = NS_LITERAL_STRING("artist");
  else if ( atom.Find("alb.data") != -1 )
    key = NS_LITERAL_STRING("album");
  else if ( atom.Find("gen.data") != -1 )
    key = NS_LITERAL_STRING("genre");
  else if ( atom.Find("trkn.data") != -1 )
    key = NS_LITERAL_STRING("track_no");
  else if ( atom.Find("disk.data") != -1 )
    key = NS_LITERAL_STRING("disk_no");
  else if ( atom.Find("day.data") != -1 )
    key = NS_LITERAL_STRING("year");
  else if ( atom.Find("cpil.data") != -1 )
    key = NS_LITERAL_STRING("compilation");
  else if ( atom.Find("tmpo.data") != -1 )
    key = NS_LITERAL_STRING("bpm");
  else if ( atom.Find("too.data") != -1 )
    key = NS_LITERAL_STRING("vendor");
  else if ( atom.Find("labl.data") != -1 )
    key = NS_LITERAL_STRING("label");
  else if ( atom.Find("burl.data") != -1 )
    key = NS_LITERAL_STRING("source_url");
  else if ( atom.Find("covr.data") != -1 )
    key = NS_LITERAL_STRING("album_art");
  else if ( atom.Find("wrt.data") != -1 )
    key = NS_LITERAL_STRING("composer");
  else if ( atom.Find("gnre.data") != -1 )
    key = NS_LITERAL_STRING("genre");
  else if ( atom.Find("orch.data") != -1 )
    key = NS_LITERAL_STRING("orchestra");
  else
  {
    // Inform the console that we've a mystery.
    nsCOMPtr<nsIURI> pURI;
    m_Channel->GetURI(getter_AddRefs(pURI));
    nsCString cstrSpec;
    pURI->GetAsciiSpec(cstrSpec);
    printf( "MetadataHandlerMP4 -- %s\nUnknown atom: %s = \"%s\"\n\n", cstrSpec.get(), atom.get(), value_string );
  }

  if ( key.Length() )
  {
    m_Values->SetValue( key.get(), value.get(), 0 );
  }
} 
//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerMP4::Read(PRInt32 *_retval)
{
  nsresult nRet = NS_ERROR_UNEXPECTED;

  *_retval = 0;
  if(!m_Channel)
  {
    *_retval = -1;
    return NS_ERROR_FAILURE;
  }

  // Get a new values object.
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1");
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
    char *url = const_cast<char *>(NS_UnescapeURL(cstrPath).get());

    quicktime_t *file;
    file = quicktime_open( url, 1, 0, 0 );
    if (file)
      quicktime_dump_info(file, static_callback, this);
    m_Completed = true;
  }
  else
  {
    // ?? Remote file
  }

  return nRet;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerMP4::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValuesMap (); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetValuesMap(sbIMetadataValues **_retval)
{
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValuesMap (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerMP4::SetValuesMap(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/* PRInt32 GetNumAvailableTags (); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetNumAvailableTags(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //GetNumAvailableTags

//-----------------------------------------------------------------------------
/* void GetAvailableTags (out PRUint32 tagCount, [array, size_is (tagCount), retval] out wstring tags); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetAvailableTags(PRUint32 *tagCount, PRUnichar ***tags)
{
  return NS_OK;
} //GetAvailableTags

//-----------------------------------------------------------------------------
/* wstring GetTag (in wstring tagName); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetTag(const PRUnichar *tagName, PRUnichar **_retval)
{

  return NS_OK;
} //GetTag

//-----------------------------------------------------------------------------
/* PRInt32 SetTag (in wstring tagName, in wstring tagValue); */
NS_IMETHODIMP sbMetadataHandlerMP4::SetTag(const PRUnichar *tagName, const PRUnichar *tagValue, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTag

//-----------------------------------------------------------------------------
/* void GetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, out PRUint32 valueCount, [array, size_is (valueCount), retval] out wstring values); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 *valueCount, PRUnichar ***values)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //GetTags

//-----------------------------------------------------------------------------
/* PRInt32 SetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, in PRUint32 valueCount, [array, size_is (valueCount)] in wstring values); */
NS_IMETHODIMP sbMetadataHandlerMP4::SetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 valueCount, const PRUnichar **values, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTags
