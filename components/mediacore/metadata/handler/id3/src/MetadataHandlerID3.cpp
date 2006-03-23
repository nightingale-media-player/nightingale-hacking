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

#include <string/nsStringAPI.h>
#include <string/nsString.h>
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

  ID3_Tag::Iterator *itFrame = tag.CreateIterator();
  ID3_Frame *pFrame = nsnull;
  while( (pFrame = itFrame->GetNext()) != NULL)
  {
    switch(pFrame->GetID())
    {
      //No known frame.
      case ID3FID_NOFRAME:
      {
      }
      break;

      //Audio encryption.
      case ID3FID_AUDIOCRYPTO:
      {
      }
      break;

      //Picture
      case ID3FID_PICTURE:  	
      {
      }
      break;

      //Comments.
      case ID3FID_COMMENT:
      {
      }
      break;

      //Commercial frame.
      case ID3FID_COMMERCIAL:
      {
      }
      break;

      //Encryption method registration.
      case ID3FID_CRYPTOREG:
      {

      }
      break;

      //Equalization.
      case ID3FID_EQUALIZATION:
      {

      }
      break;


      //Event timing codes.
      case ID3FID_EVENTTIMING:
      {

      }
      break;
      
      //General encapsulated object.
      case ID3FID_GENERALOBJECT:
      {

      }
      break;

      //Group identification registration.
      case ID3FID_GROUPINGREG:
      {

      }
      break;

      //Involved people list.
      case ID3FID_INVOLVEDPEOPLE:
      {

      }
      break;

      //Linked information.
      case ID3FID_LINKEDINFO:
      {

      }
      break;

      //Music CD identifier.
      case ID3FID_CDID:
      {

      }
      break;

      //MPEG location lookup table.
      case ID3FID_MPEGLOOKUP:
      {

      }
      break;

      //Ownership frame.
      case ID3FID_OWNERSHIP:
      {

      }
      break;

      //Private frame.
      case ID3FID_PRIVATE:
      {

      }
      break;

      //Play counter.
      case ID3FID_PLAYCOUNTER:
      {

      }
      break;

      //Popularimeter.
      case ID3FID_POPULARIMETER:
      {

      }
      break;

      //Position synchronisation frame.
      case ID3FID_POSITIONSYNC:
      {

      }
      break;

      //Recommended buffer size.
      case ID3FID_BUFFERSIZE:
      {

      }
      break;

      //Relative volume adjustment.
      case ID3FID_VOLUMEADJ:
      {

      }
      break;

      //Reverb.
      case ID3FID_REVERB:
      {

      }
      break;

      //Synchronized lyric/text.
      case ID3FID_SYNCEDLYRICS:
      {

      }
      break;

      //Synchronized tempo codes.
      case ID3FID_SYNCEDTEMPO:
      {

      }
      break;

      //Album/Movie/Show title.
      case ID3FID_ALBUM:
      {

      }
      break;

      //BPM (beats per minute).
      case ID3FID_BPM:
      {

      }
      break;

      //Composer.
      case ID3FID_COMPOSER:
      {

      }
      break;

      //Content type.
      case ID3FID_CONTENTTYPE:
      {

      }
      break;

      //Copyright message.
      case ID3FID_COPYRIGHT:
      {

      }
      break;

      //Date.
      case ID3FID_DATE:
      {

      }
      break;

      //Playlist delay.
      case ID3FID_PLAYLISTDELAY:
      {

      }
      break;

      //Encoded by.
      case ID3FID_ENCODEDBY:
      {

      }
      break;

      //Lyricist/Text writer.
      case ID3FID_LYRICIST:
      {

      }
      break;

      //File type.
      case ID3FID_FILETYPE:
      {

      }
      break;

      //Time.
      case ID3FID_TIME:
      {

      }
      break;

      //Content group description.
      case ID3FID_CONTENTGROUP:
      {

      }
      break;

      //Title/songname/content description.
      case ID3FID_TITLE:
      {

      }
      break;

      //Subtitle/Description refinement.
      case ID3FID_SUBTITLE:
      {

      }
      break;

      //Initial key.
      case ID3FID_INITIALKEY:
      {

      }
      break;

      //Language(s).
      case ID3FID_LANGUAGE:
      {

      }
      break;

      //Length.
      case ID3FID_SONGLEN:
      {

      }
      break;

      //Media type.
      case ID3FID_MEDIATYPE:
      {

      }
      break;

      //Original album/movie/show title.
      case ID3FID_ORIGALBUM:
      {

      }
      break;

      //Original filename.
      case ID3FID_ORIGFILENAME:
      {

      }
      break;

      //Original lyricist(s)/text writer(s).
      case ID3FID_ORIGLYRICIST:
      {

      }
      break;

      //Original artist(s)/performer(s).
      case ID3FID_ORIGARTIST:
      {

      }
      break;

      //Original release year.
      case ID3FID_ORIGYEAR:
      {

      }
      break;

      //File owner/licensee.
      case ID3FID_FILEOWNER:
      {

      }
      break;

      //Lead performer(s)/Soloist(s).
      case ID3FID_LEADARTIST:
      {

      }
      break;

      //Band/orchestra/accompaniment.
      case ID3FID_BAND:
      {

      }
      break;

      //Conductor/performer refinement.
      case ID3FID_CONDUCTOR:
      {

      }
      break;

      //Interpreted, remixed, or otherwise modified by.
      case ID3FID_MIXARTIST:
      {

      }
      break;

      //Part of a set.
      case ID3FID_PARTINSET:
      {

      }
      break;

      //Publisher.
      case ID3FID_PUBLISHER:
      {

      }
      break;

      //Track number/Position in set.
      case ID3FID_TRACKNUM:
      {

      }
      break;

      //Recording dates.
      case ID3FID_RECORDINGDATES:
      {

      }
      break;

      //Internet radio station name.
      case ID3FID_NETRADIOSTATION:
      {

      }
      break;

      //Internet radio station owner.
      case ID3FID_NETRADIOOWNER:
      {

      }
      break;

      //Size.
      case ID3FID_SIZE:
      {

      }
      break;

      //ISRC (international standard recording code).
      case ID3FID_ISRC:
      {

      }
      break;

      //Software/Hardware and settings used for encoding.
      case ID3FID_ENCODERSETTINGS:
      {

      }
      break;

      //User defined text information.
      case ID3FID_USERTEXT:
      {

      }
      break;

      //Year.
      case ID3FID_YEAR:
      {

      }
      break;

      //Unique file identifier.
      case ID3FID_UNIQUEFILEID:
      {

      }
      break;

      //Terms of use.
      case ID3FID_TERMSOFUSE:
      {

      }
      break;

      //Unsynchronized lyric/text transcription.
      case ID3FID_UNSYNCEDLYRICS:
      {

      }
      break;

      //Commercial information.
      case ID3FID_WWWCOMMERCIALINFO:
      {

      }
      break;

      //Copyright/Legal infromation.
      case ID3FID_WWWCOPYRIGHT:
      {

      }
      break;

      //Official audio file webpage.
      case ID3FID_WWWAUDIOFILE:
      {

      }
      break;

      //Official artist/performer webpage.
      case ID3FID_WWWARTIST:
      {

      }
      break;

      //Official audio source webpage.
      case ID3FID_WWWAUDIOSOURCE:
      {

      }
      break;

      //Official internet radio station homepage.
      case ID3FID_WWWRADIOPAGE:
      {

      }
      break;

      //Payment.
      case ID3FID_WWWPAYMENT:
      {

      }
      break;

      //Official publisher webpage.
      case ID3FID_WWWPUBLISHER:
      {

      }
      break;

      //User defined URL link.
      case ID3FID_WWWUSER:
      {

      }
      break;

      //Encrypted meta frame (id3v2.2.x).
      case ID3FID_METACRYPTO:
      {

      }
      break;

      //Compressed meta frame (id3v2.2.1).
      case ID3FID_METACOMPRESSION:
      {

      }
      break;

      //Last field placeholder.
      case ID3FID_LASTFRAMEID:
      {

      }
      break;
    }
  }

  return ret;
} //ReadTag