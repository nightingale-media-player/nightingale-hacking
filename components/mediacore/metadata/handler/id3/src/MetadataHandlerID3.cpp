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
* \file MetadataHandlerID3.cpp
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#ifdef XP_WIN
#include <windows.h> // For ansi-code-page character translation.
#endif

#include <nscore.h>
#include "MetadataHandlerID3.h"

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>
#include <necko/nsNetUtil.h>

#include <string/nsReadableUtils.h>
#include <xpcom/nsEscape.h>

// DEFINES ====================================================================

#include <id3/reader.h>
  
class MetadataHandlerID3Exception
{
private:
  MetadataHandlerID3Exception() {}
public:
  MetadataHandlerID3Exception( PRUint64 seek, PRUint64 buf, PRUint64 size ) : m_Seek( seek ), m_Buf( buf ), m_Size( size ) {}
  PRUint64 m_Seek, m_Buf, m_Size;
};


class ID3_ChannelReader : public ID3_Reader
{
  nsCOMPtr<sbIMetadataChannel> m_Channel;
protected:
public:
  ID3_ChannelReader()
  {
  }
  ID3_ChannelReader(sbIMetadataChannel* channel)
  {
    setChannel( channel );
  };
  virtual ~ID3_ChannelReader() { ; }
  virtual void close() { ; }

  void setChannel(sbIMetadataChannel* channel)
  {
    m_Channel = channel;
    this->setCur( 0 );  // Open it as a restartable channel
  }

  virtual int_type peekChar() 
  { 
    if (!this->atEnd() || !this->getCur())
    {
      char aByte;
      PRUint32 count = 0;
      m_Channel->Read( &aByte, 1, &count ); // 1 byte, please.
      this->setCur( this->getCur() - 1 ); // Now back up 1 byte.  Streams shouldn't peek!
      return aByte;
    }
    return END_OF_READER;
  }

  virtual size_type readChars(char buf[], size_type len)
  {
    PRUint32 count = 0;
    if ( ! NS_SUCCEEDED( m_Channel->Read( buf, len, &count ) ) )
    {
      PRUint64 pos, buf, size;
      m_Channel->GetPos( &pos );
      m_Channel->GetBuf( &buf );
      m_Channel->GetSize( &size );
      throw MetadataHandlerID3Exception( pos + len, buf, size );
    }
    return count;
  }
  virtual size_type readChars(char_type buf[], size_type len)
  {
    return this->readChars(reinterpret_cast<char *>(buf), len);
  }

  virtual pos_type getCur() 
  { 
    PRUint64 pos = 0;
    m_Channel->GetPos( &pos );
    return (pos_type)pos;
  }

  virtual pos_type getBeg()
  {
    return 0;
  }

  virtual pos_type getEnd()
  {
    PRUint64 size = 0;
    m_Channel->GetSize( &size );
    return (pos_type)size;
  }

  virtual pos_type remainingBytes()
  {
    return getEnd() - getCur();
  }

  virtual pos_type skipChars(pos_type skip)
  {
    return setCur( getCur() + skip );
  }

  virtual pos_type setCur(pos_type pos)
  {
    if ( !NS_SUCCEEDED( m_Channel->SetPos( pos ) ) )
    {
      PRUint64 seek, buf, size;
      m_Channel->GetPos( &seek );
      m_Channel->GetBuf( &buf );
      m_Channel->GetSize( &size );
      throw MetadataHandlerID3Exception( seek, buf, size );
    }
    return pos;
  }
};

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataHandlerID3, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::sbMetadataHandlerID3()
{
  m_Completed = false;
} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::~sbMetadataHandlerID3()
{
  m_Completed = true;
} //dtor

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out wstring aMIMETypes); */
NS_IMETHODIMP sbMetadataHandlerID3::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out wstring aExts); */
NS_IMETHODIMP sbMetadataHandlerID3::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
/* nsIChannel GetChannel (); */
NS_IMETHODIMP sbMetadataHandlerID3::GetChannel(nsIChannel **_retval)
{
  *_retval = m_Channel.get();
  (*_retval)->AddRef();

  return NS_OK;
} //GetChannel

//-----------------------------------------------------------------------------
/* PRInt32 SetChannel (in nsIChannel urlChannel); */
NS_IMETHODIMP sbMetadataHandlerID3::SetChannel(nsIChannel *urlChannel)
{
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerID3::OnChannelData( nsISupports *channel )
{
  nsCOMPtr<sbIMetadataChannel> mc( do_QueryInterface(channel) ) ;

  if ( mc.get() )
  {
    try
    {
      if ( !m_Completed )
      {
        ID3_Tag  tag;
        ID3_ChannelReader channel_reader( mc.get() );
        bool ok = tag.Parse( channel_reader );
        const Mp3_Headerinfo *headerinfo = tag.GetMp3HeaderInfo();
        ReadTag(tag);

        if (!headerinfo)
        {
          // (sigh) Do it by hand.
          mc->SetPos( 0 );
          PRUint64 buf = 0;
          mc->GetBuf(&buf);
          PRUint32 read = 0, size = PR_MIN( 8096, (PRUint32)buf );
          if ( size )
          {
            char *buffer = (char *)nsMemory::Alloc(size);
            if (buffer)
            {
              mc->Read(buffer, size, &read);
              PRUint64 file_size = 0;
              mc->GetSize(&file_size);
              CalculateBitrate(buffer, read, file_size);
              nsMemory::Free(buffer);
            }
          }
        }
      }
    }
    catch ( const MetadataHandlerID3Exception err )
    {
      // If it's a tiny file, it's probably a 404 error
      if ( err.m_Seek > ( err.m_Size - 1024 ) )
      {
        // If it's a big file, this means it's an ID3v1 and it needs to seek to the end of the track?  Ooops.
        m_Completed = true;
      }

      // Otherwise, take another spin around and try again.
    }
  }

  // If the channel thinks it's done, then the handler must be done, too.
  PRBool completed = false;
  mc->GetCompleted( &completed );
  if ( completed )
    m_Completed = true;

  return NS_OK;
}

NS_IMETHODIMP sbMetadataHandlerID3::GetCompleted(PRBool *_retval)
{
  *_retval = m_Completed;

  return NS_OK;
} //SetChannel

NS_IMETHODIMP sbMetadataHandlerID3::Close()
{
  if ( m_ChannelHandler.get() ) 
    m_ChannelHandler->Close();

  m_Values = nsnull;
  m_Channel = nsnull;
  m_ChannelHandler = nsnull;

  return NS_OK;
} //Close

NS_IMETHODIMP sbMetadataHandlerID3::Vote(const nsAString &url, PRInt32 *_retval )
{
  nsPromiseFlatString strUrl( url );

  if ( strUrl.Find( ".mp3", PR_TRUE ) != -1 )
    *_retval = 1; // Yes, we want this.
  else if ( 
    ( strUrl.Find( ".mov", PR_TRUE ) != -1 ) ||
    ( strUrl.Find( ".avi", PR_TRUE ) != -1 ) ||
    ( strUrl.Find( ".wma", PR_TRUE ) != -1 ) ||
    ( strUrl.Find( ".wmv", PR_TRUE ) != -1 ) ||
    ( strUrl.Find( ".asf", PR_TRUE ) != -1 ) ||
    ( strUrl.Find( ".wav", PR_TRUE ) != -1 )
  )
    *_retval = -1; // Ones we _know_ we don't want
  else
    *_retval = 0; // Otherwise, go ahead and try to be the default

  return NS_OK;
} //Close

//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerID3::Read(PRInt32 *_retval)
{
  nsresult nRet = NS_ERROR_UNEXPECTED;

  // Zero is failure.
  *_retval = 0;
  if ( m_Completed )
  {
    return NS_ERROR_UNEXPECTED;
  }

  if(!m_Channel)
  {
    return NS_ERROR_FAILURE;
  }

  // Get a new values object.
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1");
  if(!m_Values.get())
  {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> pURI;
  nRet = m_Channel->GetURI(getter_AddRefs(pURI));
  if(NS_FAILED(nRet)) return nRet;

  PRBool async = true;

  nsCString cstrScheme, cstrPath, cstrSpec;
  pURI->GetScheme(cstrScheme);
  pURI->GetPath(cstrPath);
  pURI->GetSpec(cstrSpec);
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

    if ( cstrPath.Find( NS_LITERAL_CSTRING(":\\") ) == 1 )
    {
      async = false;
    }
#endif
  }

  // Attempt to do it locally, without using the channel.
  if ( !async )
  {
    // Right now, only the fast Windows code uses the synchronous reader on local files.
    ID3_Tag  tag;
    size_t nTagSize = tag.Link(NS_UnescapeURL(cstrPath).get(), ID3TT_ID3V2);
    if ( nTagSize == 0 )
    {
      nTagSize = tag.Link(NS_UnescapeURL(cstrPath).get(), ID3TT_ALL);
    }
    *_retval = nTagSize;

    if ( nTagSize > 0 )
    {
      const Mp3_Headerinfo *headerinfo = tag.GetMp3HeaderInfo();
      ReadTag(tag);

      // Handle the bitrate, if they can't do it for us.
      if (!headerinfo)
      {
        // Then alloc the buffer.
        const PRUint32 size = 8096;
        char *buffer = (char *)nsMemory::Alloc(size);

        // Read the first chunk of the file to get bitrate, etc.
        // Then open a file with it.
        nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
        file->InitWithNativePath(NS_UnescapeURL(cstrPath));

        // Then open an input stream with it.
        nsCOMPtr<nsIFileInputStream> stream = do_CreateInstance("@mozilla.org/network/file-input-stream;1");
        stream->Init( file, PR_RDONLY, 0, nsIFileInputStream::CLOSE_ON_EOF );

        // Then read.
        PRUint32 read = 0;
        stream->Read(buffer, size, &read);

        // Then get the file size.
        PRInt64 file_size = 0;
        file->GetFileSize(&file_size);

        // And finally calculate the bitrate.
        CalculateBitrate(buffer, read, file_size);
        nsMemory::Free(buffer);

        // Setup retval with the number of values read
        PRInt32 num_values;
        m_Values->GetNumValues(&num_values);
        *_retval = num_values;
      }
    }
    else
    {
      async = true; // So, a blocking parse failed.  Try again, using the nsIChannel
    }
    nRet = NS_OK;
  }

  if (async)
  {
    // Get a new channel handler for async operation.
    m_ChannelHandler = do_CreateInstance("@songbirdnest.com/Songbird/MetadataChannel;1");
    if(!m_ChannelHandler.get())
    {
      return NS_ERROR_FAILURE;
    }
    m_ChannelHandler->Open( m_Channel, this );

    nRet = NS_OK;

    // -1 means we're asynchronous.
    *_retval = -1;
  }

  return nRet;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Write (); */
NS_IMETHODIMP sbMetadataHandlerID3::Write(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //Write

/* sbIMetadataValues GetValues (); */
NS_IMETHODIMP sbMetadataHandlerID3::GetValues(sbIMetadataValues **_retval)
{
  if ( ! m_Completed )
    return NS_ERROR_UNEXPECTED;
  *_retval = m_Values;
  if ( (*_retval) )
    (*_retval)->AddRef();
  return NS_OK;
}

/* void SetValues (in sbIMetadataValues values); */
NS_IMETHODIMP sbMetadataHandlerID3::SetValues(sbIMetadataValues *values)
{
  m_Values = values;
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
PRInt32 sbMetadataHandlerID3::ReadTag(ID3_Tag &tag)
{
  PRInt32 ret = 0;

  PRInt32 type = 0; // Always string, for now.

  ID3_Tag::Iterator *itFrame = tag.CreateIterator();
  ID3_Frame *pFrame = nsnull;

  while( (pFrame = itFrame->GetNext()) != nsnull)
  {
    nsAutoString strKey;
    nsAutoString strValue;
    switch(pFrame->GetID())
    {
      //No known frame.
      case ID3FID_NOFRAME: strKey.AppendLiteral(""); break;

      //Audio encryption.
      case ID3FID_AUDIOCRYPTO: strKey.AppendLiteral(""); break;

      //Picture
      case ID3FID_PICTURE: strKey.AppendLiteral(""); break;

      //Comments.
      case ID3FID_COMMENT: strKey.AppendLiteral("comment"); break;

      //Commercial frame.
      case ID3FID_COMMERCIAL: strKey.AppendLiteral(""); break;

      //Encryption method registration.
      case ID3FID_CRYPTOREG: strKey.AppendLiteral(""); break;

      //Equalization.
      case ID3FID_EQUALIZATION: strKey.AppendLiteral(""); break;

      //Event timing codes.
      case ID3FID_EVENTTIMING: strKey.AppendLiteral(""); break;
      
      //General encapsulated object.
      case ID3FID_GENERALOBJECT: strKey.AppendLiteral(""); break;

      //Group identification registration.
      case ID3FID_GROUPINGREG: strKey.AppendLiteral(""); break;

      //Involved people list.
      case ID3FID_INVOLVEDPEOPLE: strKey.AppendLiteral(""); break;

      //Linked information.
      case ID3FID_LINKEDINFO: strKey.AppendLiteral(""); break;

      //Music CD identifier.
      case ID3FID_CDID: strKey.AppendLiteral(""); break;

      //MPEG location lookup table.
      case ID3FID_MPEGLOOKUP: strKey.AppendLiteral(""); break;

      //Ownership frame.
      case ID3FID_OWNERSHIP: strKey.AppendLiteral(""); break;

      //Private frame.
      case ID3FID_PRIVATE: strKey.AppendLiteral(""); break;

      //Play counter.
      case ID3FID_PLAYCOUNTER: strKey.AppendLiteral(""); break;

      //Popularimeter.
      case ID3FID_POPULARIMETER: strKey.AppendLiteral(""); break;

      //Position synchronisation frame.
      case ID3FID_POSITIONSYNC: strKey.AppendLiteral(""); break;

      //Recommended buffer size.
      case ID3FID_BUFFERSIZE: strKey.AppendLiteral(""); break;

      //Relative volume adjustment.
      case ID3FID_VOLUMEADJ: strKey.AppendLiteral(""); break;

      //Reverb.
      case ID3FID_REVERB: strKey.AppendLiteral(""); break;

      //Synchronized lyric/text.
      case ID3FID_SYNCEDLYRICS: strKey.AppendLiteral(""); break;

      //Synchronized tempo codes.
      case ID3FID_SYNCEDTEMPO: strKey.AppendLiteral(""); break;

      //Album/Movie/Show title.
      case ID3FID_ALBUM: strKey.AppendLiteral("album"); break;

      //BPM (beats per minute).
      case ID3FID_BPM: strKey.AppendLiteral("bpm"); break;

      //Composer.
      case ID3FID_COMPOSER: strKey.AppendLiteral("composer"); break;

      //Content type.
      case ID3FID_CONTENTTYPE: strKey.AppendLiteral("genre"); break;

      //Copyright message.
      case ID3FID_COPYRIGHT: strKey.AppendLiteral("copyright_message"); break;

      //Date.
      case ID3FID_DATE: strKey.AppendLiteral(""); break;

      //Playlist delay.
      case ID3FID_PLAYLISTDELAY: strKey.AppendLiteral(""); break;

      //Encoded by.
      case ID3FID_ENCODEDBY: strKey.AppendLiteral("vendor"); break;

      //Lyricist/Text writer.
      case ID3FID_LYRICIST: strKey.AppendLiteral(""); break;

      //File type.
      case ID3FID_FILETYPE: strKey.AppendLiteral(""); break;

      //Time.
      case ID3FID_TIME: strKey.AppendLiteral("length"); break;

      //Content group description.
      case ID3FID_CONTENTGROUP: strKey.AppendLiteral(""); break;

      //Title/songname/content description.
      case ID3FID_TITLE: strKey.AppendLiteral("title"); break;

      //Subtitle/Description refinement.
      case ID3FID_SUBTITLE: strKey.AppendLiteral("subtitle"); break;

      //Initial key.
      case ID3FID_INITIALKEY: strKey.AppendLiteral("key"); break;

      //Language(s).
      case ID3FID_LANGUAGE: strKey.AppendLiteral("language"); break;

      //Length.
      case ID3FID_SONGLEN: strKey.AppendLiteral("length"); break;

      //Media type.
      case ID3FID_MEDIATYPE: strKey.AppendLiteral("mediatype"); break;

      //Original album/movie/show title.
      case ID3FID_ORIGALBUM: strKey.AppendLiteral("original_album"); break;

      //Original filename.
      case ID3FID_ORIGFILENAME: strKey.AppendLiteral(""); break;

      //Original lyricist(s)/text writer(s).
      case ID3FID_ORIGLYRICIST: strKey.AppendLiteral(""); break;

      //Original artist(s)/performer(s).
      case ID3FID_ORIGARTIST: strKey.AppendLiteral("original_artist"); break;

      //Original release year.
      case ID3FID_ORIGYEAR: strKey.AppendLiteral("original_year"); break;

      //File owner/licensee.
      case ID3FID_FILEOWNER: strKey.AppendLiteral(""); break;

      //Lead performer(s)/Soloist(s).
      case ID3FID_LEADARTIST: strKey.AppendLiteral("artist"); break;

      //Band/orchestra/accompaniment.
      case ID3FID_BAND: strKey.AppendLiteral("accompaniment"); break;

      //Conductor/performer refinement.
      case ID3FID_CONDUCTOR: strKey.AppendLiteral("conductor"); break;

      //Interpreted, remixed, or otherwise modified by.
      case ID3FID_MIXARTIST: strKey.AppendLiteral("interpreter_remixer"); break;

      //Part of a set.
      case ID3FID_PARTINSET: strKey.AppendLiteral("set_collection"); break;

      //Publisher.
      case ID3FID_PUBLISHER: strKey.AppendLiteral("publisher"); break;

      //Track number/Position in set.
      case ID3FID_TRACKNUM: strKey.AppendLiteral("track_no"); type = 1; break;

      //Recording dates.
      case ID3FID_RECORDINGDATES: strKey.AppendLiteral(""); break;

      //Internet radio station name.
      case ID3FID_NETRADIOSTATION: strKey.AppendLiteral(""); break;

      //Internet radio station owner.
      case ID3FID_NETRADIOOWNER: strKey.AppendLiteral(""); break;

      //Size.
      case ID3FID_SIZE: strKey.AppendLiteral(""); break;

      //ISRC (international standard recording code).
      case ID3FID_ISRC: strKey.AppendLiteral(""); break;

      //Software/Hardware and settings used for encoding.
      case ID3FID_ENCODERSETTINGS: strKey.AppendLiteral(""); break;

      //User defined text information.
      case ID3FID_USERTEXT: strKey.AppendLiteral(""); break;

      //Year.
      case ID3FID_YEAR: strKey.AppendLiteral("year"); break;

      //Unique file identifier.
      case ID3FID_UNIQUEFILEID: strKey.AppendLiteral("metadata_uuid"); break;

      //Terms of use.
      case ID3FID_TERMSOFUSE: strKey.AppendLiteral("terms_of_use"); break;

      //Unsynchronized lyric/text transcription.
      case ID3FID_UNSYNCEDLYRICS: strKey.AppendLiteral("lyrics"); break;

      //Commercial information.
      case ID3FID_WWWCOMMERCIALINFO: strKey.AppendLiteral("commercialinfo_url"); break;

      //Copyright/Legal infromation.
      case ID3FID_WWWCOPYRIGHT: strKey.AppendLiteral("copyright_url"); break;

      //Official audio file webpage.
      case ID3FID_WWWAUDIOFILE: strKey.AppendLiteral(""); break;

      //Official artist/performer webpage.
      case ID3FID_WWWARTIST: strKey.AppendLiteral("artist_url"); break;

      //Official audio source webpage.
      case ID3FID_WWWAUDIOSOURCE: strKey.AppendLiteral("source_url"); break;

      //Official internet radio station homepage.
      case ID3FID_WWWRADIOPAGE: strKey.AppendLiteral("netradio_url"); break;

      //Payment.
      case ID3FID_WWWPAYMENT: strKey.AppendLiteral("payment_url"); break;

      //Official publisher webpage.
      case ID3FID_WWWPUBLISHER: strKey.AppendLiteral("publisher_url"); break;

      //User defined URL link.
      case ID3FID_WWWUSER: strKey.AppendLiteral("user_url"); break;

      //Encrypted meta frame (id3v2.2.x).
      case ID3FID_METACRYPTO: strKey.AppendLiteral(""); break;

      //Compressed meta frame (id3v2.2.1).
      case ID3FID_METACOMPRESSION: strKey.AppendLiteral(""); break;

      //Last field placeholder.
      case ID3FID_LASTFRAMEID: strKey.AppendLiteral(""); break;
    }

    // If we care,
    if ( strKey.Length() )
    {
      // Get the text field.
      ID3_Field* pField = pFrame->GetField(ID3FN_TEXT);
      if (nsnull != pField)
      {
        ID3_TextEnc enc = pField->GetEncoding();
        PRBool swap = true;
        switch( enc )
        {
          case ID3TE_NONE:
          case ID3TE_ISO8859_1:
          {
            nsCString iso_string( pField->GetRawText() );
#ifdef XP_WIN
            // Just for fun, some systems like to save their values in the current windows codepage.
            // This doesn't guarantee we'll translate them properly, but we'll at least have a better chance.
            int size = MultiByteToWideChar( CP_ACP, 0, iso_string.get(), iso_string.Length(), NULL, 0 );
            PRUnichar *uni_string = reinterpret_cast< PRUnichar * >( nsMemory::Alloc( (size + 1) * sizeof( PRUnichar ) ) );
            int read = MultiByteToWideChar( CP_ACP, 0, iso_string.get(), iso_string.Length(), uni_string, size );
            NS_ASSERTION(size == read, "Win32 ISO8859 conversion failed.");
            uni_string[ size ] = 0;
            strValue.Assign( uni_string );
            nsMemory::Free( uni_string );
#else
            strValue = NS_ConvertUTF8toUTF16(iso_string); // This probably won't work at all.
#endif
            break;
          }
          case ID3TE_UTF16BE: // ?? what do we do with big endian?  cry?
            swap = false; // maybe id3lib is just backwards?
          case ID3TE_UTF16:
          {
            size_t size = pField->Size();
            unicode_t *buffer = (unicode_t *)nsMemory::Alloc( (size + 1) * sizeof(unicode_t) );
            size_t read = pField->Get( buffer, size );
            buffer[read] = 0;
            if (swap)
              // Am I sure this is the rules? I'm always supposed to swap?
              for (size_t i = 0; i < read; i++)  
              {
                char *p = (char *)(buffer+i);
                char temp = p[0];
                p[0] = p[1];
                p[1] = temp;
              }
            strValue.Assign(buffer);
            nsMemory::Free(buffer);
            break;
          }
          case ID3TE_UTF8:
          {
            // Relatively easy.
            strValue = NS_ConvertUTF8toUTF16( pField->GetRawText() );
            break;
          }
        }
      }
      // Do special stuff if the value is "1 of 12" or "1/12"
      if ( strKey == NS_LITERAL_STRING("track_no") || strKey == NS_LITERAL_STRING("disc_no") )
      {
        PRInt32 mark = strKey.Find("_");
        nsAutoString totalKey;
        strKey.Left( totalKey, mark );
        totalKey.AppendLiteral("_total");

        if ( ( mark = strValue.Find( "of", PR_TRUE ) ) != -1 )
        {
          nsAutoString _no, _total;
          strValue.Left( _no, mark - 1 );
          strValue.Right( _total, strValue.Length() - mark - 3 );
          m_Values->SetValue( strKey, _no, type );
          m_Values->SetValue( totalKey, _total, type );
        }
        else if ( ( mark = strValue.Find( "/", PR_TRUE ) ) != -1 )
        {
          nsAutoString _no, _total;
          strValue.Left( _no, mark );
          strValue.Right( _total, strValue.Length() - mark - 1 );
          m_Values->SetValue( strKey, _no, type );
          m_Values->SetValue( totalKey, _total, type );
        }
        else
          m_Values->SetValue( strKey, strValue, type );
      }
      else  
        m_Values->SetValue( strKey, strValue, type );
    }
  }

  // For now, leak this?
  // We crash when we delete it?

//  delete itFrame;

  // Parse up the happy header info.
  const Mp3_Headerinfo *headerinfo = tag.GetMp3HeaderInfo();
  if ( headerinfo )
  {
    // Bitrate.
    nsString br;
    br.AppendInt( (PRInt32)headerinfo->bitrate );
    m_Values->SetValue( NS_LITERAL_STRING("bitrate"), br, 0 );

    // Frequency.
    nsString fr;
    fr.AppendInt( (PRInt32)headerinfo->frequency );
    m_Values->SetValue( NS_LITERAL_STRING("frequency"), fr, 0 );

    // Length.
    nsAutoString value;
    m_Values->GetValue(NS_LITERAL_STRING("length"), value);
    if (!value.Length())
    {
      nsString ln;
      ln.AppendInt( (PRInt32)headerinfo->time * 1000 );
      m_Values->SetValue( NS_LITERAL_STRING("length"), ln, 0 );
    }
  }

  // Fixup the genre info because iTunes is stupid.
  nsAutoString genre;
  m_Values->GetValue(NS_LITERAL_STRING("genre"), genre);
  if (genre.Length())
  {
    PRInt32 lparen = genre.Find("(");
    PRInt32 rparen = genre.Find(")");
    if ( lparen != -1 && rparen != -1 )
    {
      nsAutoString gen;
      genre.Mid( gen, lparen + 1, rparen - 1 );
      PRInt32 aErrorCode;
      PRInt32 g = gen.ToInteger(&aErrorCode);
      if ( !aErrorCode && g < ID3_NR_OF_V1_GENRES )
      {
        m_Values->SetValue( NS_LITERAL_STRING("genre"), NS_ConvertUTF8toUTF16(ID3_v1_genre_description[g]), 0 );
      }
    }
  }

  m_Completed = true;  // And, we're done.
  return ret;
} //ReadTag

//-----------------------------------------------------------------------------
PRInt32 sbMetadataHandlerID3::ReadFrame(ID3_Frame *frame)
{
  PRInt32 ret = 0;

  ID3_Frame::Iterator *itField = frame->CreateIterator();
  ID3_Field* field = nsnull;

  while( (field = itField->GetNext()) != nsnull )
  {
    ReadFields(field);
  }

  delete itField;

  return ret;
} //ReadFrame

//-----------------------------------------------------------------------------
PRInt32 sbMetadataHandlerID3::ReadFields(ID3_Field *field)
{
  PRInt32 ret = 0;

  ID3_FieldType fieldType = field->GetType();
  switch(fieldType)
  {
    case ID3FTY_NONE: break;
    case ID3FTY_INTEGER: break;
    case ID3FTY_BINARY: break;
    case ID3FTY_TEXTSTRING: break;
  }
 
  return ret;
} //ReadField

const PRInt32 gBitrates[3][3][16] = // This is stupid.
{
  { // V1
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}, // L1
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0}, // L2
    {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0}  // L3
  },
  { // V2
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0}, // L1
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}, // L2
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}  // L3
  },
  { // V2.5
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0}, // L1
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}, // L2
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}  // L3
  }
};

const PRInt32 gFrequencies[3][4] =
{
  {44100, 48000, 32000, 0}, // V1
  {22050, 24000, 16000, 0}, // V2
  {11025, 12000,  8000, 0}  // V2.5
};

//-----------------------------------------------------------------------------
void sbMetadataHandlerID3::CalculateBitrate(const char *buffer, PRUint32 length, PRUint64 file_size)
{
  const char byte_zero = 0xFF;
  const char byte_one = 0xE0;
  // Skip ID3v2 2k block.
  if ( length > 2048 && buffer[2048] == byte_zero )
  {
    length -= 2048;
    buffer += 2048;
  }
  PRBool found = false;
  PRInt32 version = -1; // 0 = V1, 1 = V2, 2 = V2.5
  PRInt32 layer = -1; // 0 = L1, 1 = L2, 2 = L3
  PRInt32 bitrate = -1;
  PRInt32 frequency = -1;
  for (PRUint32 count = 0; count < length; ++count, ++buffer)
  {
    // Scan forward in the buffer, till we find a header match
    if ( buffer[0] == byte_zero ) 
      if ( ( buffer[1] & byte_one ) == byte_one )
      {
        // Check the version bits.
        char v = ( buffer[1] & 0x18 ) >> 3;
        if ( v == 0 )
          version = 2; // MPEG Version 2.5
        else if ( v == 1 )
          continue; // RESERVED -- ERROR, NOT A HEADER
        else if ( v == 2 )
          version = 1; // MPEG Version 2 (ISO/IEC 13818-3)
        else
          version = 0; // MPEG Version 1 (ISO/IEC 11172-3)

        // Check the layer bits.
        char l = ( buffer[1] & 0x06 ) >> 1;
        if ( l == 0 )
          continue; // RESERVED -- ERROR, NOT A HEADER
        else if ( l == 1 )
          layer = 2; // Layer III
        else if ( l == 2 )
          layer = 1; // Layer II
        else
          layer = 0; // Layer I
      }
      else
        version = layer = -1;
    else
      version = layer = -1;

    // NO BAD DEREFERENCES!
    if ( version >= 0 && layer >= 0 && version < 3 && layer < 3 )
    {
      char b = ( buffer[2] & 0xF0 ) >> 4;
      bitrate = gBitrates[version][layer][b];
      char f = ( buffer[2] & 0x0C ) >> 2;
      frequency = gFrequencies[version][f];

      if ( bitrate > 0 && frequency > 0 )
      {
        // Okay, we're done!
        found = true;
        break;
      }
    }
  }

  if (found)
  {
    nsAutoString br;
    br.AppendInt(bitrate);
    m_Values->SetValue(NS_LITERAL_STRING("bitrate"), br, 0);

    nsAutoString fr;
    fr.AppendInt(frequency);
    m_Values->SetValue(NS_LITERAL_STRING("frequency"), fr, 0);

    nsAutoString value;
    m_Values->GetValue(NS_LITERAL_STRING("length"), value);
    if (!value.Length() && bitrate > 0 && file_size > 0)
    {
      // Okay, so, the id3 didn't specify length.  Calculate that, too.
      PRUint32 length_in_ms = (PRUint32)( ( ( ( file_size * (PRUint64)8 ) ) / (PRUint64)bitrate ) & 0x00000000FFFFFFFF );
      nsAutoString ln;
      ln.AppendInt(length_in_ms);
      m_Values->SetValue(NS_LITERAL_STRING("length"), ln, 0);
    }
  }
} //CalculateBitrate


