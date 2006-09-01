/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
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
#include <unicharutil/nsUnicharUtils.h>
#include <xpcom/nsEscape.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataHandlerMP4, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerMP4::sbMetadataHandlerMP4()
{
  m_Completed = false;
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
NS_IMETHODIMP sbMetadataHandlerMP4::SetChannel(nsIChannel *urlChannel)
{
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerMP4::OnChannelData( nsISupports *channel )
{
  // Uhhhhhh..... this is only a synchronous handler.  Just end.  
  // Someone's probably fucking with you, and you shouldn't put up with it.
  m_Completed = true;
  return NS_OK;
}

NS_IMETHODIMP sbMetadataHandlerMP4::GetCompleted(PRBool *_retval)
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

NS_IMETHODIMP sbMetadataHandlerMP4::Vote(const nsAString &url, PRInt32 *_retval )
{
  nsAutoString strUrl( url );
  ToLowerCase(strUrl);

  if ( ( strUrl.Find( ".mp4", PR_TRUE ) != -1 ) || 
       ( strUrl.Find( ".m4a", PR_TRUE ) != -1 ) || 
       ( strUrl.Find( ".mov", PR_TRUE ) != -1 ) )
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
  PRInt32 type = 0; // everything is a string for now?

  // "covr.data" is album art.  deal with this later.
  if ( atom.Find("covr.data") == -1 )
    value = NS_ConvertUTF8toUTF16(value_string);

  // Mozilla Find can't handle high-bit characters.  Grrr.
  if ( atom.Find("nam.data") != -1 ) {
    key = NS_LITERAL_STRING("title");
  }
  else if ( atom.Find("ART.data") != -1 ) {
    key = NS_LITERAL_STRING("artist");
  }
  else if ( atom.Find("alb.data") != -1 ) {
    key = NS_LITERAL_STRING("album");
  }
  else if ( atom.Find("gen.data") != -1 ) {
    key = NS_LITERAL_STRING("genre");
  }
  else if ( atom.Find("trkn.data") != -1 ) {
    type = 1;
    PRUint32 track_no_int = (PRUint8)value_string[3];
    PRUint32 track_total_int = (PRUint8)value_string[5];

    nsAutoString track_no, track_total;
    track_no.AppendInt( track_no_int );
    track_total.AppendInt( track_total_int );

    m_Values->SetValue( NS_LITERAL_STRING("track_no"), track_no, type );
    m_Values->SetValue( NS_LITERAL_STRING("track_total"), track_total, type );
  }
  else if ( atom.Find("disk.data") != -1 ) {
    type = 1;
    PRUint32 disc_no_int = (PRUint8)value_string[3];
    PRUint32 disc_total_int = (PRUint8)value_string[5];

    nsAutoString disc_no, disc_total;
    disc_no.AppendInt( disc_no_int );
    disc_total.AppendInt( disc_total_int );

    m_Values->SetValue( NS_LITERAL_STRING("disc_no"), disc_no, type );
    m_Values->SetValue( NS_LITERAL_STRING("disc_total"), disc_total, type );
  }
  else if ( atom.Find("day.data") != -1 ) {
    key = NS_LITERAL_STRING("year");
  }
  else if ( atom.Find("cpil.data") != -1 ) {
    key = NS_LITERAL_STRING("compilation");
  }
  else if ( atom.Find("tmpo.data") != -1 ) {
    key = NS_LITERAL_STRING("bpm");
  }
  else if ( atom.Find("too.data") != -1 ) {
    key = NS_LITERAL_STRING("vendor");
  }
  else if ( atom.Find("labl.data") != -1 ) {
    key = NS_LITERAL_STRING("label");
  }
  else if ( atom.Find("burl.data") != -1 ) {
    key = NS_LITERAL_STRING("source_url");
  }
  else if ( atom.Find("covr.data") != -1 ) {
    key = NS_LITERAL_STRING("album_art");
  }
  else if ( atom.Find("wrt.data") != -1 ) {
    key = NS_LITERAL_STRING("composer");
  }
  else if ( atom.Find("gnre.data") != -1 ) {
    key = NS_LITERAL_STRING("genre");
  }
  else if ( atom.Find("orch.data") != -1 ) {
    key = NS_LITERAL_STRING("orchestra");
  }
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
    m_Values->SetValue( key, value, type );
  }
} 
//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerMP4::Read(PRInt32 *_retval)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  // This handler is always synchronous
  m_Completed = true; 

  *_retval = 0; // Zero is failure
  if(!m_Channel)
  {
    return NS_ERROR_FAILURE;
  }

  // Get a new values object.
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1", &rv);
  if(NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIURI> pURI;
  rv = m_Channel->GetURI(getter_AddRefs(pURI));
  if(NS_FAILED(rv)) return rv;

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
    {
      quicktime_dump_info(file, static_callback, this);
      PRInt64 sample_rate = quicktime_audio_sample_rate( file, 0 );
      if ( sample_rate )
      {
        PRInt64 sample_duration = quicktime_audio_sample_duration( file, 0 );
        PRInt64 audio_length = quicktime_audio_length( file, 0 );
        PRInt64 length_ms = audio_length * sample_duration * 1000 / sample_rate;
        nsAutoString key, value;
        key.AppendLiteral("length");
        value.AppendInt(length_ms);
        m_Values->SetValue(key, value, 0);
      }
    }

    // Setup retval with the number of values read
    PRInt32 num_values;
    m_Values->GetNumValues(&num_values);
    *_retval = num_values;
  }
  else
  {
    // ?? Remote file.  That would be nice, someday.  
    // We need better metadata parsing code, however.
  }

  return rv;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerMP4::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValues (); */
NS_IMETHODIMP sbMetadataHandlerMP4::GetValues(sbIMetadataValues **_retval)
{
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValues (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerMP4::SetValues(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}