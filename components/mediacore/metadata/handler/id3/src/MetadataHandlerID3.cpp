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
* \file MetadataHandlerID3.cpp
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataHandlerID3.h"

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>

#include <string/nsReadableUtils.h>
#include <xpcom/nsEscape.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataHandlerID3, sbIMetadataHandler)

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::sbMetadataHandlerID3()
{

} //ctor

//-----------------------------------------------------------------------------
sbMetadataHandlerID3::~sbMetadataHandlerID3()
{

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
NS_IMETHODIMP sbMetadataHandlerID3::SetChannel(nsIChannel *urlChannel, PRInt32 *_retval)
{
  *_retval = 0;
  m_Channel = urlChannel;

  return NS_OK;
} //SetChannel

//-----------------------------------------------------------------------------
/* PRInt32 Read (); */
NS_IMETHODIMP sbMetadataHandlerID3::Read(PRInt32 *_retval)
{
  nsresult nRet = NS_ERROR_UNEXPECTED;

  *_retval = 0;
  if(!m_Channel)
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
    
    size_t nTagSize = m_ID3Tag.Link(NS_UnescapeURL(cstrPath).get());
    *_retval = nTagSize;

    ReadTag(m_ID3Tag);

    nRet = NS_OK;
  }
  else
  {

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

//-----------------------------------------------------------------------------
/* PRInt32 GetNumAvailableTags (); */
NS_IMETHODIMP sbMetadataHandlerID3::GetNumAvailableTags(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
} //GetNumAvailableTags

//-----------------------------------------------------------------------------
/* void GetAvailableTags (out PRUint32 tagCount, [array, size_is (tagCount), retval] out wstring tags); */
NS_IMETHODIMP sbMetadataHandlerID3::GetAvailableTags(PRUint32 *tagCount, PRUnichar ***tags)
{
  return NS_OK;
} //GetAvailableTags

//-----------------------------------------------------------------------------
/* wstring GetTag (in wstring tagName); */
NS_IMETHODIMP sbMetadataHandlerID3::GetTag(const PRUnichar *tagName, PRUnichar **_retval)
{

  return NS_OK;
} //GetTag

//-----------------------------------------------------------------------------
/* PRInt32 SetTag (in wstring tagName, in wstring tagValue); */
NS_IMETHODIMP sbMetadataHandlerID3::SetTag(const PRUnichar *tagName, const PRUnichar *tagValue, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTag

//-----------------------------------------------------------------------------
/* void GetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, out PRUint32 valueCount, [array, size_is (valueCount), retval] out wstring values); */
NS_IMETHODIMP sbMetadataHandlerID3::GetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 *valueCount, PRUnichar ***values)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} //GetTags

//-----------------------------------------------------------------------------
/* PRInt32 SetTags (in PRUint32 tagCount, [array, size_is (tagCount)] in wstring tags, in PRUint32 valueCount, [array, size_is (valueCount)] in wstring values); */
NS_IMETHODIMP sbMetadataHandlerID3::SetTags(PRUint32 tagCount, const PRUnichar **tags, PRUint32 valueCount, const PRUnichar **values, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
} //SetTags

//-----------------------------------------------------------------------------
PRInt32 sbMetadataHandlerID3::ReadTag(ID3_Tag &tag)
{
  PRInt32 ret = 0;

  nsString strTagName;
  ID3_Tag::Iterator *itFrame = tag.CreateIterator();
  ID3_Frame *pFrame = nsnull;
  while( (pFrame = itFrame->GetNext()) != NULL)
  {
    switch(pFrame->GetID())
    {
      //No known frame.
      case ID3FID_NOFRAME: strTagName = NS_LITERAL_STRING(""); break;

      //Audio encryption.
      case ID3FID_AUDIOCRYPTO: strTagName = NS_LITERAL_STRING(""); break;

      //Picture
      case ID3FID_PICTURE: strTagName = NS_LITERAL_STRING(""); break;

      //Comments.
      case ID3FID_COMMENT: strTagName = NS_LITERAL_STRING("comment"); break;

      //Commercial frame.
      case ID3FID_COMMERCIAL: strTagName = NS_LITERAL_STRING(""); break;

      //Encryption method registration.
      case ID3FID_CRYPTOREG: strTagName = NS_LITERAL_STRING(""); break;

      //Equalization.
      case ID3FID_EQUALIZATION: strTagName = NS_LITERAL_STRING(""); break;

      //Event timing codes.
      case ID3FID_EVENTTIMING: strTagName = NS_LITERAL_STRING(""); break;
      
      //General encapsulated object.
      case ID3FID_GENERALOBJECT: strTagName = NS_LITERAL_STRING(""); break;

      //Group identification registration.
      case ID3FID_GROUPINGREG: strTagName = NS_LITERAL_STRING(""); break;

      //Involved people list.
      case ID3FID_INVOLVEDPEOPLE: strTagName = NS_LITERAL_STRING(""); break;

      //Linked information.
      case ID3FID_LINKEDINFO: strTagName = NS_LITERAL_STRING(""); break;

      //Music CD identifier.
      case ID3FID_CDID: strTagName = NS_LITERAL_STRING(""); break;

      //MPEG location lookup table.
      case ID3FID_MPEGLOOKUP: strTagName = NS_LITERAL_STRING(""); break;

      //Ownership frame.
      case ID3FID_OWNERSHIP: strTagName = NS_LITERAL_STRING(""); break;

      //Private frame.
      case ID3FID_PRIVATE: strTagName = NS_LITERAL_STRING(""); break;

      //Play counter.
      case ID3FID_PLAYCOUNTER: strTagName = NS_LITERAL_STRING(""); break;

      //Popularimeter.
      case ID3FID_POPULARIMETER: strTagName = NS_LITERAL_STRING(""); break;

      //Position synchronisation frame.
      case ID3FID_POSITIONSYNC: strTagName = NS_LITERAL_STRING(""); break;

      //Recommended buffer size.
      case ID3FID_BUFFERSIZE: strTagName = NS_LITERAL_STRING(""); break;

      //Relative volume adjustment.
      case ID3FID_VOLUMEADJ: strTagName = NS_LITERAL_STRING(""); break;

      //Reverb.
      case ID3FID_REVERB: strTagName = NS_LITERAL_STRING(""); break;

      //Synchronized lyric/text.
      case ID3FID_SYNCEDLYRICS: strTagName = NS_LITERAL_STRING(""); break;

      //Synchronized tempo codes.
      case ID3FID_SYNCEDTEMPO: strTagName = NS_LITERAL_STRING(""); break;

      //Album/Movie/Show title.
      case ID3FID_ALBUM: strTagName = NS_LITERAL_STRING("album"); break;

      //BPM (beats per minute).
      case ID3FID_BPM: strTagName = NS_LITERAL_STRING("bpm"); break;

      //Composer.
      case ID3FID_COMPOSER: strTagName = NS_LITERAL_STRING("composer"); break;

      //Content type.
      case ID3FID_CONTENTTYPE: strTagName = NS_LITERAL_STRING(""); break;

      //Copyright message.
      case ID3FID_COPYRIGHT: strTagName = NS_LITERAL_STRING("copyright_message"); break;

      //Date.
      case ID3FID_DATE: strTagName = NS_LITERAL_STRING(""); break;

      //Playlist delay.
      case ID3FID_PLAYLISTDELAY: strTagName = NS_LITERAL_STRING(""); break;

      //Encoded by.
      case ID3FID_ENCODEDBY: strTagName = NS_LITERAL_STRING("encoded_by"); break;

      //Lyricist/Text writer.
      case ID3FID_LYRICIST: strTagName = NS_LITERAL_STRING(""); break;

      //File type.
      case ID3FID_FILETYPE: strTagName = NS_LITERAL_STRING(""); break;

      //Time.
      case ID3FID_TIME: strTagName = NS_LITERAL_STRING(""); break;

      //Content group description.
      case ID3FID_CONTENTGROUP: strTagName = NS_LITERAL_STRING(""); break;

      //Title/songname/content description.
      case ID3FID_TITLE: strTagName = NS_LITERAL_STRING("title"); break;

      //Subtitle/Description refinement.
      case ID3FID_SUBTITLE: strTagName = NS_LITERAL_STRING("subtitle"); break;

      //Initial key.
      case ID3FID_INITIALKEY: strTagName = NS_LITERAL_STRING("key"); break;

      //Language(s).
      case ID3FID_LANGUAGE: strTagName = NS_LITERAL_STRING("language"); break;

      //Length.
      case ID3FID_SONGLEN: strTagName = NS_LITERAL_STRING("length"); break;

      //Media type.
      case ID3FID_MEDIATYPE: strTagName = NS_LITERAL_STRING(""); break;

      //Original album/movie/show title.
      case ID3FID_ORIGALBUM: strTagName = NS_LITERAL_STRING("original_album"); break;

      //Original filename.
      case ID3FID_ORIGFILENAME: strTagName = NS_LITERAL_STRING(""); break;

      //Original lyricist(s)/text writer(s).
      case ID3FID_ORIGLYRICIST: strTagName = NS_LITERAL_STRING(""); break;

      //Original artist(s)/performer(s).
      case ID3FID_ORIGARTIST: strTagName = NS_LITERAL_STRING("original_artist"); break;

      //Original release year.
      case ID3FID_ORIGYEAR: strTagName = NS_LITERAL_STRING("original_year"); break;

      //File owner/licensee.
      case ID3FID_FILEOWNER: strTagName = NS_LITERAL_STRING(""); break;

      //Lead performer(s)/Soloist(s).
      case ID3FID_LEADARTIST: strTagName = NS_LITERAL_STRING("lead_performer"); break;

      //Band/orchestra/accompaniment.
      case ID3FID_BAND: strTagName = NS_LITERAL_STRING("accompaniment"); break;

      //Conductor/performer refinement.
      case ID3FID_CONDUCTOR: strTagName = NS_LITERAL_STRING("conductor"); break;

      //Interpreted, remixed, or otherwise modified by.
      case ID3FID_MIXARTIST: strTagName = NS_LITERAL_STRING("interpreter_remixer"); break;

      //Part of a set.
      case ID3FID_PARTINSET: strTagName = NS_LITERAL_STRING("set_collection"); break;

      //Publisher.
      case ID3FID_PUBLISHER: strTagName = NS_LITERAL_STRING("publisher"); break;

      //Track number/Position in set.
      case ID3FID_TRACKNUM: strTagName = NS_LITERAL_STRING("track_no"); break;

      //Recording dates.
      case ID3FID_RECORDINGDATES: strTagName = NS_LITERAL_STRING(""); break;

      //Internet radio station name.
      case ID3FID_NETRADIOSTATION: strTagName = NS_LITERAL_STRING(""); break;

      //Internet radio station owner.
      case ID3FID_NETRADIOOWNER: strTagName = NS_LITERAL_STRING(""); break;

      //Size.
      case ID3FID_SIZE: strTagName = NS_LITERAL_STRING(""); break;

      //ISRC (international standard recording code).
      case ID3FID_ISRC: strTagName = NS_LITERAL_STRING(""); break;

      //Software/Hardware and settings used for encoding.
      case ID3FID_ENCODERSETTINGS: strTagName = NS_LITERAL_STRING(""); break;

      //User defined text information.
      case ID3FID_USERTEXT: strTagName = NS_LITERAL_STRING(""); break;

      //Year.
      case ID3FID_YEAR: strTagName = NS_LITERAL_STRING("year"); break;

      //Unique file identifier.
      case ID3FID_UNIQUEFILEID: strTagName = NS_LITERAL_STRING("uuid"); break;

      //Terms of use.
      case ID3FID_TERMSOFUSE: strTagName = NS_LITERAL_STRING("terms_of_use"); break;

      //Unsynchronized lyric/text transcription.
      case ID3FID_UNSYNCEDLYRICS: strTagName = NS_LITERAL_STRING("lyrics"); break;

      //Commercial information.
      case ID3FID_WWWCOMMERCIALINFO: strTagName = NS_LITERAL_STRING("commercialinfo_url"); break;

      //Copyright/Legal infromation.
      case ID3FID_WWWCOPYRIGHT: strTagName = NS_LITERAL_STRING("copyright_url"); break;

      //Official audio file webpage.
      case ID3FID_WWWAUDIOFILE: strTagName = NS_LITERAL_STRING(""); break;

      //Official artist/performer webpage.
      case ID3FID_WWWARTIST: strTagName = NS_LITERAL_STRING("artist_url"); break;

      //Official audio source webpage.
      case ID3FID_WWWAUDIOSOURCE: strTagName = NS_LITERAL_STRING("source_url"); break;

      //Official internet radio station homepage.
      case ID3FID_WWWRADIOPAGE: strTagName = NS_LITERAL_STRING("netradio_url"); break;

      //Payment.
      case ID3FID_WWWPAYMENT: strTagName = NS_LITERAL_STRING("payment_url"); break;

      //Official publisher webpage.
      case ID3FID_WWWPUBLISHER: strTagName = NS_LITERAL_STRING("publisher_url"); break;

      //User defined URL link.
      case ID3FID_WWWUSER: strTagName = NS_LITERAL_STRING("user_url"); break;

      //Encrypted meta frame (id3v2.2.x).
      case ID3FID_METACRYPTO: strTagName = NS_LITERAL_STRING(""); break;

      //Compressed meta frame (id3v2.2.1).
      case ID3FID_METACOMPRESSION: strTagName = NS_LITERAL_STRING(""); break;

      //Last field placeholder.
      case ID3FID_LASTFRAMEID: strTagName = NS_LITERAL_STRING(""); break;
    }
  }

  delete itFrame;

  return ret;
} //ReadTag

//-----------------------------------------------------------------------------
PRInt32 sbMetadataHandlerID3::ReadFrame(ID3_Frame *frame)
{
  PRInt32 ret = 0;

  ID3_Frame::Iterator *itField = frame->CreateIterator();
  ID3_Field* field = NULL;

  while( (field = itField->GetNext()) != NULL )
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
