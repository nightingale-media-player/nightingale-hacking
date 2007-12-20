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

/**
* \file MetadataHandlerOGG.cpp
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include "MetadataHandlerOGG.h"
#include <unicharutil/nsUnicharUtils.h>
#include <nsMemory.h>

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


NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataHandlerOGG, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerOGG::sbMetadataHandlerOGG()
{
  m_Completed = PR_FALSE;
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
NS_IMETHODIMP sbMetadataHandlerOGG::SetChannel(nsIChannel *urlChannel)
{
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
        m_Completed = PR_TRUE;
      }
    }
    catch ( const MetadataHandlerOGGException err )
    {
      PRBool completed = PR_FALSE;
      mc->GetCompleted( &completed );
      // If it's a tiny file, it's probably a 404 error
      if ( completed || ( err.m_Seek > ( err.m_Size - 1024 ) ) )
      {
        // If it's a big file, this means it's an ID3v1 and it needs to seek to the end of the track?  Ooops.
        m_Completed = PR_TRUE;
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

NS_IMETHODIMP sbMetadataHandlerOGG::Vote( const nsAString &url, PRInt32 *_retval )
{
  nsAutoString strUrl( url );
  ToLowerCase(strUrl);

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
  nsresult rv = NS_ERROR_UNEXPECTED;

  // If we fail, we're complete.  If we're async, we'll set it PR_FALSE down below.
  m_Completed = PR_TRUE; 

  *_retval = 0;  // Zero is failure
  if(!m_Channel)
    return NS_ERROR_FAILURE;

  // Get a new values object.
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1", &rv);
  if(NS_FAILED(rv)) return rv;

  // Get a new channel handler.
  m_ChannelHandler = do_CreateInstance("@songbirdnest.com/Songbird/MetadataChannel;1", &rv);
  if(NS_FAILED(rv)) return rv;

  // Start reading the data.
  rv = m_ChannelHandler->Open( m_Channel, this );
  if(NS_FAILED(rv)) return rv;

  // This read is always asynchronous
  *_retval = -1;
  m_Completed = PR_FALSE; 

  rv = NS_OK;

  return rv;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerOGG::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValues (); */
NS_IMETHODIMP sbMetadataHandlerOGG::GetValues(sbIMetadataValues **_retval)
{
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValues (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerOGG::SetValues(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

void sbMetadataHandlerOGG::ParseChannel()
{
  // We don't care about the first header?
  sbOGGHeader first_header = ParseHeader();
  if ( first_header.m_Size && first_header.m_Size < 0x00010000 ) // Ogg says this can never be stupid big.
  {
    m_ChannelHandler->Skip( 7 ); // junq - 7 bytes of "*vorbis"

    PRInt32 version, sample_rate, bitrate_max, bitrate_min, bitrate_nom; // 20 bytes
    char channels, blocksize; // 2 bytes
    m_ChannelHandler->ReadInt32( &version );
    m_ChannelHandler->ReadChar ( &channels );
    m_ChannelHandler->ReadInt32( &sample_rate );
    m_ChannelHandler->ReadInt32( &bitrate_max );
    m_ChannelHandler->ReadInt32( &bitrate_nom );
    m_ChannelHandler->ReadInt32( &bitrate_min );
    m_ChannelHandler->ReadChar ( &blocksize );

    PRUint64 bitrate = bitrate_nom ? bitrate_nom : ( bitrate_min + bitrate_max ) / 2;
    if ( bitrate )
    {
      PRUint64 size;
      m_ChannelHandler->GetSize(&size);
      PRInt64 length_ms = size * 8000 / bitrate;
      nsAutoString key, value;
      key.AppendLiteral("length");
      value.AppendInt(length_ms);
      m_Values->SetValue(key, value, 0);
    }

    m_ChannelHandler->Skip( first_header.m_Size - 7 - 20 - 2 ); // skip the rest of whatever was in the header.
  }
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
    m_ChannelHandler->Skip( 7 ); // junq - 7 bytes of "*vorbis"

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
        // This converts the utf8 data to u16
        nsString comment_string = ReadIntString();
        if ( comment_string.Length() )
        {
          // Their syntax is KEY=value;
          PRInt32 split = comment_string.Find( "=", PR_FALSE );
          if ( split != -1 )
          {
            // Split out key and value
            nsAutoString key(StringHead(comment_string, split));
            nsAutoString value(StringTail(comment_string, comment_string.Length() - split - 1));
            PRInt32 type = 0;
            ToLowerCase( key );

            // Transform their keynames to our keynames
            if (key == NS_LITERAL_STRING("tracknumber"))
            {
              key = NS_LITERAL_STRING("track_no");
            }
            if (key == NS_LITERAL_STRING("date"))
            {
              key = NS_LITERAL_STRING("year");
            }

            // Do special stuff if the value is "1 of 12" or "1/12"
            if ( key == NS_LITERAL_STRING("track_no") || key == NS_LITERAL_STRING("disc_no") )
            {
              type = 1; // Int
              PRInt32 mark = key.Find("_");
              nsAutoString totalKey(StringHead(key, mark));
              totalKey.AppendLiteral("_total");

              if ( ( mark = value.Find( "of", PR_TRUE ) ) != -1 )
              {
                nsAutoString _no(StringHead(value, mark - 1));
                nsAutoString _total(StringTail(value, value.Length() - mark - 3));
                m_Values->SetValue( key, _no, type );
                m_Values->SetValue( totalKey, _total, type );
              }
              else if ( ( mark = value.Find( "/", PR_TRUE ) ) != -1 )
              {
                nsAutoString _no(StringHead(value, mark));
                nsAutoString _total(StringTail(value, value.Length() - mark - 1));
                m_Values->SetValue( key, _no, type );
                m_Values->SetValue( totalKey, _total, type );
              }
              else
                m_Values->SetValue( key, value, type );
            }
            else  
              m_Values->SetValue( key, value, type );
          }
          else break; // Crap
        }
        else break; // Double-Crap
      }
    }
  }
}

sbMetadataHandlerOGG::sbOGGHeader sbMetadataHandlerOGG::ParseHeader()
{
  sbOGGHeader retval;

  PRBool failed = PR_FALSE;
  char test[] = { 'O', 'g', 'g', 'S' };
  char test_char;
  for ( int i = 0; i < sizeof(test); i++ )
  {
    m_ChannelHandler->ReadChar( &test_char );
    if ( test_char != test[ i ] )
    {
      failed = PR_TRUE;
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