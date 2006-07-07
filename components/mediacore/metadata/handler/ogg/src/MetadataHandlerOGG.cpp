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
* \file MetadataHandlerOGG.cpp
* \brief 
*/

#pragma once

#ifdef XP_WIN
#include <windows.h>
#endif
// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataHandlerOGG.h"

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class MetadataHandlerOGGException
{
private:
  MetadataHandlerOGGException() {}
public:
  MetadataHandlerOGGException( PRUint64 seek, PRUint64 buf, PRUint64 size ) : m_Seek( seek ), m_Buf( buf ), m_Size( size ) {}
  PRUint64 m_Seek, m_Buf, m_Size;
};


NS_IMPL_ISUPPORTS1(sbMetadataHandlerOGG, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerOGG::sbMetadataHandlerOGG()
{
  m_Completed = false;
} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerOGG::~sbMetadataHandlerOGG()
{

} //dtor

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out wstring aMIMETypes); */
NS_IMETHODIMP sbMetadataHandlerOGG::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out wstring aExts); */
NS_IMETHODIMP sbMetadataHandlerOGG::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
/* nsIChannel GetChannel (); */
NS_IMETHODIMP sbMetadataHandlerOGG::GetChannel(nsIChannel **_retval)
{
  *_retval = m_Channel.get();
  (*_retval)->AddRef();

  return NS_OK;
} //GetChannel

//-----------------------------------------------------------------------------
/* PRInt32 SetChannel (in nsIChannel urlChannel); */
NS_IMETHODIMP sbMetadataHandlerOGG::SetChannel(nsIChannel *urlChannel, PRInt32 *_retval)
{
  *_retval = 0;
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerOGG::OnChannelData( nsISupports *channel )
{
  nsCOMPtr<sbIMetadataChannel> mc( do_QueryInterface(channel) ) ;

  if ( mc.get() )
  {
    try
    {
      if ( !m_Completed )
      {
        ParseChannel();
        m_Completed = true;
      }
    }
    catch ( const MetadataHandlerOGGException err )
    {
      PRBool completed = false;
      mc->GetCompleted( &completed );
      // If it's a tiny file, it's probably a 404 error
      if ( completed || ( err.m_Seek > ( err.m_Size - 1024 ) ) )
      {
        // If it's a big file, this means it's an ID3v1 and it needs to seek to the end of the track?  Ooops.
        m_Completed = true;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP sbMetadataHandlerOGG::GetCompleted(PRBool *_retval)
{
  *_retval = m_Completed;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerOGG::Close()
{
  if ( m_ChannelHandler.get() ) 
    m_ChannelHandler->Close();

  m_Values = nsnull;
  m_Channel = nsnull;
  m_ChannelHandler = nsnull;

  return NS_OK;
} //Close

NS_IMETHODIMP sbMetadataHandlerOGG::Vote( const PRUnichar *url, PRInt32 *_retval )
{
  nsString strUrl( url );

  if ( strUrl.Find( ".ogg", PR_TRUE ) != -1 )
    *_retval = 1;
  else
    *_retval = -1;

  return NS_OK;
} //Close

//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerOGG::Read(PRInt32 *_retval)
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

  // Get a new channel handler.
  m_ChannelHandler = do_CreateInstance("@songbirdnest.com/Songbird/MetadataChannel;1");
  if(!m_ChannelHandler.get())
  {
    *_retval = -1;
    return NS_ERROR_FAILURE;
  }
  // Start reading the data.
  m_ChannelHandler->Open( m_Channel, this );

  nRet = NS_OK;

  return nRet;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerOGG::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValuesMap (); */
NS_IMETHODIMP sbMetadataHandlerOGG::GetValuesMap(sbIMetadataValues **_retval)
{
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValuesMap (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerOGG::SetValuesMap(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

void sbMetadataHandlerOGG::ParseChannel()
{
  // Lots of data buffers, here
  PRUint32 count;
  void *buf;

  // We don't care about the first header?
  sbOGGHeader first_header = ParseHeader();
  if ( first_header.m_Size && first_header.m_Size < 0x00010000 ) // Ogg says this can never be stupid big.
    m_ChannelHandler->Skip( first_header.m_Size );
  else
    return; // Dead monkey.  Empty values.


  // They say we want the second header.
  sbOGGHeader comment_header = ParseHeader();
  while ( 
    ( comment_header.stream_serial_number == first_header.stream_serial_number ) &&
    ( comment_header.page_sequence_no == first_header.page_sequence_no ) &&
    ( comment_header.absolute_granule_position == first_header.absolute_granule_position ) &&
    ( comment_header.page_checksum == first_header.page_checksum ) &&
    ( comment_header.m_Size == first_header.m_Size ) )
  {
    // Unfortunately, sometimes idiot files have 2 identical opening headers?
    m_ChannelHandler->Skip( comment_header.m_Size );
    comment_header = ParseHeader();
  }
  // So, let's go...
  if ( comment_header.m_Size && comment_header.m_Size < 0x00010000 ) // Ogg says this can never be stupid big.
  {
    // Head block.
    char head[7];
    m_ChannelHandler->Read( (char *)head, 7, &count ); // junq

    // Vendor block.
    nsString vendor_string = ReadIntString();
    if ( vendor_string.Length() )
    {
      m_Values->SetValue( NS_LITERAL_STRING("vendor"), vendor_string, 0 );
    }

    // Comment block.
    PRInt32 comment_count;
    m_ChannelHandler->ReadInt32( &comment_count );
    if ( (PRUint32)comment_count < 100 ) // ???
    {
      for ( int i = 0; i < comment_count; i++ )
      {
        nsString comment_string = ReadIntString();
        if ( comment_string.Length() )
        {
          PRInt32 split = comment_string.Find( "=", FALSE );
          if ( split != -1 )
          {
            nsString key, value;
            comment_string.Left( key, split );
            comment_string.Right( value, comment_string.Length() - split - 1 );
            m_Values->SetValue( key, value, 0 ); // Lots of bulletproofing before we get here.
          }
          else break; // Crap
        }
        else break; // Crap
      }
    }
  }
}

sbMetadataHandlerOGG::sbOGGHeader sbMetadataHandlerOGG::ParseHeader()
{
  sbOGGHeader retval;

  PRBool failed = false;
  char test[] = { 'O', 'g', 'g', 'S' };
  char test_char;
  for ( int i = 0; i < sizeof(test); i++ )
  {
    m_ChannelHandler->ReadChar( &test_char );
    if ( test_char != test[ i ] )
    {
      failed = true;
      break;
    }
  }

  if ( ! failed )
  {
    m_ChannelHandler->ReadChar( (char *)&retval.stream_structure_version );
    m_ChannelHandler->ReadChar( (char *)&retval.header_type_flag );
    m_ChannelHandler->ReadInt64( &retval.absolute_granule_position );
    m_ChannelHandler->ReadInt32( &retval.stream_serial_number );
    m_ChannelHandler->ReadInt32( &retval.page_sequence_no );
    m_ChannelHandler->ReadInt32( &retval.page_checksum );

    PRUint8 segments, segment;
    m_ChannelHandler->ReadChar( (char *)&segments );
    for ( int i = 0; i < segments; i++ )
    {
      m_ChannelHandler->ReadChar( (char *)&segment );
      retval.m_Size += segment;
    }
  }

  return retval;
}

nsString sbMetadataHandlerOGG::ReadIntString()
{
  nsString retval;
  PRInt32 length;
  PRUint32 count;
  void *buf;
  m_ChannelHandler->ReadInt32( &length );
  if ( length > 0 )
  {
    buf = nsMemory::Alloc( length + 1 );
    if ( buf )
    {
      m_ChannelHandler->Read( (char *)buf, length, &count );
      if ( count == length )
      {
        ((char *)buf)[ count ] = 0; // Null terminate the string
        retval = NS_ConvertUTF8toUTF16((char *)buf);
      }
      nsMemory::Free( buf );
    }
  }
  return retval;
}