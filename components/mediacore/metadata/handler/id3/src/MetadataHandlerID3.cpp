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
#include <nsISeekableStream.h>
#include <necko/nsIIOService.h>
#include <necko/nsNetUtil.h>

#include <string/nsReadableUtils.h>
#include <unicharutil/nsUnicharUtils.h>
#include <xpcom/nsEscape.h>

// DEFINES ====================================================================

#include <id3/reader.h>
  
class MetadataHandlerID3Exception
{
public:
  MetadataHandlerID3Exception() {}
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
  virtual ~ID3_ChannelReader() { close(); }
  virtual void close() { if(m_Channel) { /* m_Channel->Close(); Someone else does this */ m_Channel = nsnull; } }

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
    if ( NS_FAILED( m_Channel->Read( buf, len, &count ) ) )
    {
      return PR_UINT32_MAX; // overflow, try again.
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
    nsresult rv = m_Channel->GetPos( &pos );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
    return (pos_type)pos;
  }

  virtual pos_type getBeg()
  {
    return 0;
  }

  virtual pos_type getEnd()
  {
    PRUint64 size = 0;
    nsresult rv = m_Channel->GetSize( &size );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
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
    nsresult rv = m_Channel->SetPos( pos );
    if ( rv == NS_ERROR_SONGBIRD_METADATA_CHANNEL_RESTART )
    {
      // If I can edit the id3lib code, I can do what I want with calling conventions.
      return PR_UINT32_MAX;
    }
    else if ( NS_FAILED( rv ) )
    {
      // Throw the angry exception that forces completion
      throw MetadataHandlerID3Exception();
    }
    return pos;
  }
};

class ID3_FileReader : public ID3_Reader
{
  nsCOMPtr<nsIFile> m_File;
  nsCOMPtr<nsIInputStream> m_Stream;
  nsCOMPtr<nsISeekableStream> m_Seek;
protected:
public:
  explicit ID3_FileReader(nsACString & spec)
  {
    nsresult rv;
    PRBool success = PR_FALSE;

    rv = NS_GetFileFromURLSpec(spec, getter_AddRefs(m_File));
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();

    m_Stream = do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
      
    nsCOMPtr<nsIFileInputStream> fstream = do_QueryInterface(m_Stream, &rv);
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();

    rv = fstream->Init( m_File, PR_RDONLY, 0, nsIFileInputStream::CLOSE_ON_EOF );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();

    m_Seek = do_QueryInterface( m_Stream, &rv );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
  };
  virtual ~ID3_FileReader() { close(); }
  virtual void close() { if(m_Stream) { m_Stream->Close(); m_Stream = nsnull; } }

  virtual int_type peekChar() 
  { 
    if (!this->atEnd() || !this->getCur())
    {
      unsigned char aByte = 0;
      pos_type pos = this->getCur();
      PRUint32 count = readChars( &aByte, 1 ); // 1 byte, please.
      if (count)
        this->setCur( pos ); // Now back up 1 byte.  Streams shouldn't peek!
      return aByte;
    }
    return END_OF_READER;
  }

  virtual size_type readChars(char buf[], size_type len)
  {
    PRUint32 count = 0;
    nsresult rv =  m_Stream->Read( buf, len, &count );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
    return count;
  }

  virtual size_type readChars(char_type buf[], size_type len)
  {
    return this->readChars(reinterpret_cast<char *>(buf), len);
  }

  virtual pos_type getCur() 
  { 
    PRInt64 pos = 0;
    nsresult rv = m_Seek->Tell( &pos );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
    return (pos_type)pos;
  }

  virtual pos_type getBeg()
  {
    return 0;
  }

  virtual pos_type getEnd()
  {
    PRInt64 size = 0;
    nsresult rv = m_File->GetFileSize( &size );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
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
    PRInt64 pos64 = pos;
    nsresult rv = m_Seek->Seek( nsISeekableStream::NS_SEEK_SET, pos64 );
    if ( NS_FAILED(rv) )
      throw MetadataHandlerID3Exception();
    return pos;
  }
};

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataHandlerID3, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::sbMetadataHandlerID3()
{
  m_Completed = PR_FALSE;
} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::~sbMetadataHandlerID3()
{
  m_Completed = PR_TRUE;
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
      // If the channel thinks it's done, then the handler must be done, too.
      PRBool completed = PR_FALSE;
      mc->GetCompleted( &completed );
      if ( completed )
        m_Completed = PR_TRUE;

      if ( !m_Completed )
      {
        // Get the size before you do anything else?
        PRUint64 file_size = 0;
        mc->GetSize(&file_size);

        size_t nTagSize = 0;
        ID3_Tag  tag;
        ID3_ChannelReader channel_reader( mc );
        nTagSize = tag.Link(channel_reader, ID3TT_ID3V2);
        if ( nTagSize == PR_UINT32_MAX)
          return NS_OK; // Need to wait a bit longer, just loop around till the next onchanneldata call.  (THIS DOESN'T WORK, I ONLY GET A REQUEST STOP)
        if ( nTagSize == 0 )
        {
          ID3_ChannelReader channel_reader( mc );
          nTagSize = tag.Link(channel_reader, ID3TT_ALL);
        }
        if ( nTagSize == PR_UINT32_MAX)
          return NS_OK; // Need to wait a bit longer, just loop around till the next onchanneldata call.
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
              CalculateBitrate(buffer, read, file_size);
              nsMemory::Free(buffer);
            }
          }
        }
      }
    }
    catch ( const MetadataHandlerID3Exception )
    {
      m_Completed = PR_TRUE;
    }
  }

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
  nsAutoString strUrl( url );
  ToLowerCase(strUrl);

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
  nsresult rv = NS_ERROR_UNEXPECTED;

  // Zero is failure.
  *_retval = 0;
  if ( m_Completed )
  {
    return NS_ERROR_UNEXPECTED;
  }

  // If we fail, we're complete.  If we're async, we'll set it PR_FALSE down below.
  m_Completed = PR_TRUE; 

  if(!m_Channel)
  {
    return NS_ERROR_FAILURE;
  }

  // Get a new values object.
  m_Values = do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> pURI;
  rv = m_Channel->GetURI(getter_AddRefs(pURI));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool async = PR_TRUE;

  nsCString cstrScheme, cstrPath, cstrSpec;
  pURI->GetScheme(cstrScheme);
  pURI->GetPath(cstrPath);
  pURI->GetSpec(cstrSpec);
  if(cstrScheme.Equals(NS_LITERAL_CSTRING("file")))
  {
#if 0 // defined(XP_WIN)
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
#endif
    {
      async = PR_FALSE;
    }
  }

  // Attempt to do it locally, without using the channel.
  if ( !async )
  {
    ID3_Tag  tag;
    size_t nTagSize = 0;

    try
    {
      ID3_FileReader file_reader( cstrSpec );
      nTagSize = tag.Link(file_reader, ID3TT_ID3V2);
      if ( nTagSize == 0 )
      {
        nTagSize = tag.Link(file_reader, ID3TT_ALL);
      }
    }
    catch (MetadataHandlerID3Exception)
    {
      // Oops, failed in the file reader.  That's not good.  
      // Assume it's a total failure and don't waste time trying the channel.
      m_Completed = PR_TRUE; 
      return NS_OK;
    }

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
        nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        file->InitWithNativePath(NS_UnescapeURL(cstrPath));

        // Then open an input stream with it.
        nsCOMPtr<nsIFileInputStream> stream = do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
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

        //Close the stream so the file can be freed as well.
        stream->Close();

        // ps: the fact that the stream doesn't close itself when it is released in this scope is kinda lame.
      }
    }
    else
    {
      async = PR_TRUE; // So, a blocking parse failed.  Try again, using the nsIChannel
    }

    // Setup retval with the number of values read
    PRInt32 num_values;
    m_Values->GetNumValues(&num_values);
    *_retval = num_values;

    rv = NS_OK;
  }

  if (async)
  {
    try
    {
      // Get a new channel handler for async operation.
      m_ChannelHandler = do_CreateInstance("@songbirdnest.com/Songbird/MetadataChannel;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = m_ChannelHandler->Open( m_Channel, this );
      if(NS_FAILED(rv)) return NS_OK;
    }
    catch (MetadataHandlerID3Exception)
    {
      // Oops, failed.  That's not good, either.
      return NS_OK;
    }

    // -1 means we're asynchronous.
    *_retval = -1;
    m_Completed = PR_FALSE; // And we're therefore not completed.
    rv = NS_OK;
  }

  return rv;
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

      //Copyright(c) 2005-2007 POTI, Inc.
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

      //Copyright(c) 2005-2007 POTI, Inc.
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
        PRBool swap = PR_TRUE;
#ifdef IS_BIG_ENDIAN
        swap = PR_FALSE;
#endif
        switch( enc )
        {
          case ID3TE_NONE:
          case ID3TE_ISO8859_1:
          {
            nsCString iso_string( pField->GetRawText() );
#ifdef XP_WIN
            // Just for fun, some systems like to save their values in the current windows codepage.
            // This doesn't guarantee we'll translate them properly, but we'll at least have a better chance.
            int size = MultiByteToWideChar( CP_ACP, 0, iso_string.get(), iso_string.Length(), nsnull, 0 );
            PRUnichar *uni_string = reinterpret_cast< PRUnichar * >( nsMemory::Alloc( (size + 1) * sizeof( PRUnichar ) ) );
            int read = MultiByteToWideChar( CP_ACP, 0, iso_string.get(), iso_string.Length(), uni_string, size );
            NS_ASSERTION(size == read, "Win32 ISO8859 conversion failed.");
            uni_string[ size ] = 0;
            strValue.Assign( uni_string );
            nsMemory::Free( uni_string );
#else
            strValue = NS_ConvertASCIItoUTF16(iso_string); // This probably won't work at all.
#endif
            break;
          }
          case ID3TE_UTF16BE: // ?? what do we do with big endian?  cry?
            swap = !swap; // maybe id3lib is just backwards?
          case ID3TE_UTF16:
          {
            size_t size = pField->Size();
            unicode_t *buffer = (unicode_t *)nsMemory::Alloc( (size + 1) * sizeof(unicode_t) );
            size_t read = pField->Get( buffer, size );
            buffer[read] = 0;

            PRBool string_be = PR_TRUE;
            PRInt32 big_votes = 0, little_votes = 0;
            // Preflight it, looking for 0's so we can better guess endian for 7bit values.
            for (size_t i = 0; i < read; i++)  
            {
              char *p = (char *)(buffer+i);
              if (p[0] == 0) 
                big_votes++;
              else if (p[1] == 0) 
                little_votes++;
            }
            // Hmmm, this is still stupid.
            if ( big_votes == read )
            {
#ifdef IS_BIG_ENDIAN
                swap = PR_FALSE;
#else
                swap = PR_TRUE;
#endif
            }
            if ( little_votes == read )
            {
#ifdef IS_BIG_ENDIAN
                swap = PR_TRUE;
#else
                swap = PR_FALSE;
#endif
            }

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

  // We USED to crash here when we deleted this.  Now we don't anymore.  Beats me.
  delete itFrame;

  nsAutoString str;

  // Parse up the happy header info.
  const Mp3_Headerinfo *headerinfo = tag.GetMp3HeaderInfo();
  if ( headerinfo )
  {
    // Bitrate.
    str.AppendInt( (PRInt32)headerinfo->bitrate );
    m_Values->SetValue( NS_LITERAL_STRING("bitrate"), str, 0 );

    // Frequency.
    str = EmptyString();
    str.AppendInt( (PRInt32)headerinfo->frequency );
    m_Values->SetValue( NS_LITERAL_STRING("frequency"), str, 0 );

    // Length.
    m_Values->GetValue(NS_LITERAL_STRING("length"), str);
    if (!str.Length())
    {
      str = EmptyString();
      str.AppendInt( (PRInt32)headerinfo->time * 1000 );
      m_Values->SetValue( NS_LITERAL_STRING("length"), str, 0 );
    }
  }

  // Fixup the genre info because iTunes is stupid.
  m_Values->GetValue(NS_LITERAL_STRING("genre"), str);
  if (str.Length())
  {
    PRInt32 lparen = str.Find("(");
    PRInt32 rparen = str.Find(")");
    if ( lparen == 0 && rparen != -1 )
    {
      nsAutoString gen;
      str.Mid( gen, lparen + 1, rparen - 1 );
      PRInt32 aErrorCode;
      PRInt32 g = gen.ToInteger(&aErrorCode);
      if ( !aErrorCode && g < ID3_NR_OF_V1_GENRES )
      {
        m_Values->SetValue( NS_LITERAL_STRING("genre"), NS_ConvertUTF8toUTF16(ID3_v1_genre_description[g]), 0 );
      }
    }
  }

  m_Completed = PR_TRUE;  // And, we're done.
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
  const char byte_zero = (const char)0xFF;
  const char byte_one = (const char)0xE0;
  // Skip ID3v2 2k block.  Hope there is no album art.
  if ( length > 2048 && buffer[2048] == byte_zero )
  {
    length -= 2048;
    buffer += 2048;
  }
  PRBool found = PR_FALSE;
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
        found = PR_TRUE;
        break;
      }
    }
  }

  if (found)
  {
    nsAutoString str;
    str.AppendInt(bitrate);
    m_Values->SetValue(NS_LITERAL_STRING("bitrate"), str, 0);

    str = EmptyString();
    str.AppendInt(frequency);
    m_Values->SetValue(NS_LITERAL_STRING("frequency"), str, 0);

    m_Values->GetValue(NS_LITERAL_STRING("length"), str);
    if (!str.Length() && bitrate > 0 && file_size > 0 && file_size != (PRUint64)-1) // No, ben, there is no PR_UINT64_MAX
    {
      // Okay, so, the id3 didn't specify length.  Calculate that, too.
      PRUint32 length_in_ms = (PRUint32)( ( ( ( file_size * (PRUint64)8 ) ) / (PRUint64)bitrate ) & 0x00000000FFFFFFFF );
      nsAutoString ln;
      ln.AppendInt(length_in_ms);
      m_Values->SetValue(NS_LITERAL_STRING("length"), ln, 0);
    }
  }
} //CalculateBitrate


