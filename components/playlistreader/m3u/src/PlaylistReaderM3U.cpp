/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
 * \file PlaylistReaderM3U.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include <string>
#include <algorithm>
#include <map>

#include "sbIDatabaseQuery.h"
#include "PlaylistReaderM3U.h"
#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#include <xpcom/nscore.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIURI.h>
#include <necko/nsNetUtil.h>
#include <necko/nsIStandardURL.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>
#include <docshell/nsIURIFixup.h>

#include <string/nsString.h>
#include <string/nsReadableUtils.h>
#include <unicharutil/nsUnicharUtils.h>

// CLASSES ====================================================================
//=============================================================================
// class CPlaylistReaderM3U
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_ISUPPORTS1(CPlaylistReaderM3U, sbIPlaylistReader)

//-----------------------------------------------------------------------------
CPlaylistReaderM3U::CPlaylistReaderM3U()
: m_Replace(PR_FALSE)
, m_pDescriptionLock(PR_NewLock())
, m_pNameLock(PR_NewLock())
, m_pOriginalURLLock(PR_NewLock())
{
  NS_ASSERTION(m_pDescriptionLock, "PlaylistReaderM3U.m_pDescriptionLock failed");
  NS_ASSERTION(m_pNameLock, "PlaylistReaderM3U.m_pNameLock failed");
  NS_ASSERTION(m_pOriginalURLLock, "PlaylistReaderM3U.m_pOriginalURLLock failed");

  m_Name.AssignLiteral("Songbird M3U Reader");
  m_Description.AssignLiteral("Loads M3U playlists from remote and local locations.");
} //ctor

//-----------------------------------------------------------------------------
CPlaylistReaderM3U::~CPlaylistReaderM3U()
{
  if (m_pDescriptionLock)
    PR_DestroyLock(m_pDescriptionLock);
  if (m_pNameLock)
    PR_DestroyLock(m_pNameLock);
} //dtor

//-----------------------------------------------------------------------------
/* attribute AString originalURL; */
NS_IMETHODIMP CPlaylistReaderM3U::SetOriginalURL(const nsAString &strURL)
{
  nsAutoLock lock(m_pOriginalURLLock);
  m_OriginalURL = strURL;
  return NS_OK;
}

NS_IMETHODIMP CPlaylistReaderM3U::GetOriginalURL(nsAString &strURL)
{
  nsAutoLock lock(m_pOriginalURLLock);
  strURL = m_OriginalURL;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool Read (in wstring strURL, in wstring strGUID, in wstring strDestTable, out PRInt32 errorCode); */
NS_IMETHODIMP CPlaylistReaderM3U::Read(const nsAString &strURL, const nsAString &strGUID, const nsAString &strDestTable, PRBool bReplace, PRInt32 *errorCode, PRBool *_retval)
{
  *_retval = PR_FALSE;
  *errorCode = 0;
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIFileInputStream> pFileReader = do_CreateInstance("@mozilla.org/network/file-input-stream;1");

  nsCOMPtr<nsIFile> pFile;
  rv = NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(strURL), getter_AddRefs(pFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> pURI;
  rv = NS_NewFileURI(getter_AddRefs(pURI), pFile);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool bIsFile = PR_FALSE;
  rv = pFile->IsFile(&bIsFile);

  if(NS_FAILED(rv) || !bIsFile) return NS_ERROR_UNEXPECTED;

  rv = pFileReader->Init(pFile.get(), PR_RDONLY, 0, nsIFileInputStream::CLOSE_ON_EOF);
  if(NS_FAILED(rv)) return rv;

  PRInt64 nFileSize = 0;
  rv = pFile->GetFileSize(&nFileSize);
  if(NS_FAILED(rv)) return rv;

  nsCString buffer;
  buffer.SetLength(nFileSize);
  pFileReader->Read(NS_CONST_CAST(char *, buffer.get()), nFileSize, &rv);
  pFileReader->Close();

  if(NS_SUCCEEDED(rv))
  {
    nsString strBuffer;

    if(IsASCII(buffer))
    {
      strBuffer = NS_ConvertASCIItoUTF16(buffer);
    }
    else if(IsUTF8(buffer))
    {
      strBuffer = NS_ConvertUTF8toUTF16(buffer);
    }
    else //IsUTF16
    {
      strBuffer.SetLength(buffer.Length() / sizeof(PRUnichar));
      strBuffer.Assign(NS_REINTERPRET_CAST(const PRUnichar *, buffer.get()), strBuffer.Length());
    }

    m_Replace = bReplace;
    *errorCode = ParseM3UFromBuffer(pURI,
                                    NS_CONST_CAST(PRUnichar *, PromiseFlatString(strBuffer).get()), 
                                    strBuffer.Length(), 
                                    NS_CONST_CAST(PRUnichar *, PromiseFlatString(strGUID).get()), 
                                    NS_CONST_CAST(PRUnichar *, PromiseFlatString(strDestTable).get()));

    if(!*errorCode)
    {
      rv = NS_OK;
      *_retval = PR_TRUE;
    }
  }

  return rv;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Vote (); */
NS_IMETHODIMP CPlaylistReaderM3U::Vote(const nsAString &strUrl, PRInt32 *_retval)
{
  *_retval = 10000;
  return NS_OK;
} //Vote

//-----------------------------------------------------------------------------
/* wstring Name (); */
NS_IMETHODIMP CPlaylistReaderM3U::Name(nsAString &_retval)
{
  nsAutoLock lock(m_pNameLock);
  _retval = m_Name;
  return NS_OK;
} //Name

//-----------------------------------------------------------------------------
/* wstring Description (); */
NS_IMETHODIMP CPlaylistReaderM3U::Description(nsAString &_retval)
{
  nsAutoLock lock(m_pDescriptionLock);
  _retval = m_Description;
  return NS_OK;
} //Description

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out string aMIMETypes); */
NS_IMETHODIMP CPlaylistReaderM3U::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{ 
  NS_NAMED_LITERAL_STRING(m3uMime0, "audio/mpegurl");
  NS_NAMED_LITERAL_STRING(m3uMime1, "audio/x-mpegurl");
  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(2 * sizeof(PRUnichar *));

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  out[0] = ToNewUnicode(m3uMime0);
  if(!out[0])
    return NS_ERROR_OUT_OF_MEMORY;

  out[1] = ToNewUnicode(m3uMime1);
  if(!out[1])
    return NS_ERROR_OUT_OF_MEMORY;

  *nMIMECount = 2;
  *aMIMETypes = out;

  return NS_OK;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out string aExts); */
NS_IMETHODIMP CPlaylistReaderM3U::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  NS_NAMED_LITERAL_STRING(m3uExt, "m3u");
  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(1 * sizeof(PRUnichar *));

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  out[0] = ToNewUnicode(m3uExt);
  if(!out[0])
    return NS_ERROR_OUT_OF_MEMORY;

  *nExtCount = 1;
  *aExts = out;

  return NS_OK;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
PRInt32 CPlaylistReaderM3U::ParseM3UFromBuffer(nsIURI *pBaseURI, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable)
{
  PRInt32 convError = 0;
  nsresult rv;

  NS_NAMED_LITERAL_STRING(carReturn, "\r");
  NS_NAMED_LITERAL_STRING(lineFeed, "\n");
  NS_NAMED_LITERAL_STRING(tabChar, "\t");
  NS_NAMED_LITERAL_STRING(spaceChar, " ");
  NS_NAMED_LITERAL_STRING(zeroChar, "0");
  NS_NAMED_LITERAL_STRING(nineChar, "9");
  NS_NAMED_LITERAL_STRING(dashSymbol, "-");
  NS_NAMED_LITERAL_STRING(timeSeperator, ":");
  NS_NAMED_LITERAL_STRING(extm3uToken, "#EXTM3U");
  NS_NAMED_LITERAL_STRING(extm3uInfoToken, "#EXTINF:");
  NS_NAMED_LITERAL_STRING(asfToken, "ASF ");
  NS_NAMED_LITERAL_STRING(lengthMetadata, "length");
  NS_NAMED_LITERAL_STRING(titleMetadata, "title");

  nsAutoString strURL, strTitle;
  nsAutoString strLength(NS_LITERAL_STRING("0"));
  
  PRInt32 nAvail = 0;

  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  if(!pQuery) return NS_ERROR_UNEXPECTED;

  pQuery->SetAsyncQuery(PR_TRUE);
  pQuery->SetDatabaseGUID(nsAutoString(strGUID));

  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );
  if(!pLibrary) return NS_ERROR_UNEXPECTED;
  pLibrary->SetQueryObject(pQuery.get());

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIPlaylist> pPlaylist;
  if(!pPlaylistManager) return NS_ERROR_UNEXPECTED;

  pPlaylistManager->GetPlaylist(nsAutoString(strDestTable), pQuery.get(), getter_AddRefs(pPlaylist));
  if(!pPlaylist) return NS_ERROR_UNEXPECTED;

  nsDependentString playlistBuffer(pBuffer);
  nsDependentString::const_iterator fileStart, lineStart, lineEnd, fileEnd;
  playlistBuffer.BeginReading(fileStart); playlistBuffer.BeginReading(lineStart);
  playlistBuffer.EndReading(lineEnd); playlistBuffer.EndReading(fileEnd);

  nsAutoString theline;

  nsCOMPtr<nsIURIFixup> fixup = do_GetService("@mozilla.org/docshell/urifixup;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  do
  {
    PRBool bFound = FindInReadable(lineFeed, lineStart, lineEnd);

    if(bFound)
      theline = Substring(fileStart, lineEnd);
    else
      theline = Substring(fileStart, fileEnd);

    while( (lineStart != fileEnd) && 
           (*lineStart == carReturn[0] || 
            *lineStart == lineFeed[0] || 
            *lineStart == tabChar[0] || 
            *lineStart == spaceChar[0]) ) 
      lineStart++;    

    nsAutoString::const_iterator start, end;
    theline.BeginReading(start); 
    theline.EndReading(end);

    if( FindInReadable(extm3uToken, start, end) )
    {
      nAvail = 1;
    }
    else 
    {
      theline.BeginReading(start); 
      theline.EndReading(end);

      if ( nAvail && FindInReadable(extm3uInfoToken, start, end) )
      {
        start = end;
        strLength = EmptyString();

        while( (*start >= zeroChar[0] && *start <= nineChar[0]) || *start == dashSymbol[0] )
          strLength += *start++;

        *start++;

        {
          PRInt32 nLen = strLength.ToInteger(&convError);
          if(nLen > 0)
          {
            PRInt32 nMins = nLen / 60;
            PRInt32 nSecs = nLen % 60;

            strLength = EmptyString();

            strLength.AppendInt(nMins);
            strLength += timeSeperator;

            if(nSecs < 10) strLength.AppendLiteral("0");
            strLength.AppendInt(nSecs);
          }
        }

        //chop pesky \r or \n.
        theline.EndReading(end);
        strTitle = Substring(start, end);
        strTitle.CompressWhitespace();
      }
      else
      {
        theline.EndReading(end);
        if ( FindInReadable(asfToken, start, end) )
          start = end;
        else
          theline.BeginReading(start);

        theline.EndReading(end);
        if ( *start && *start != extm3uToken[0] && *start != carReturn[0] && *start != lineFeed[0])
        {
          strURL = Substring(start, end);
          strURL.CompressWhitespace();

          nsCAutoString cstrURL;
          cstrURL.Assign(NS_ConvertUTF16toUTF8(strURL));

          // Convert the playlist item into a URL.  First, check to see if it
          // is already a URL
          nsCOMPtr<nsIURI> uri(do_CreateInstance("@mozilla.org/network/simple-uri;1", &rv));
          NS_ENSURE_SUCCESS(rv, rv);

          PRBool isURL = PR_FALSE;
          rv = uri->SetSpec(cstrURL);
          if(NS_SUCCEEDED(rv)) {
            // nsSimpleURI is easily fooled by Windows thinking that a drive
            // letter and its colon is actually a scheme.  This is _not_ a URL
            // if we have a one letter scheme
            nsCAutoString scheme;
            rv = uri->GetScheme(scheme);
            if(NS_SUCCEEDED(rv) && scheme.Length() > 1) {
              isURL = PR_TRUE;
            }
          }

          // If this is not a URL, resolve the path against the URL of the m3u
          // file
          if(!isURL) {
            rv = pBaseURI->Resolve(cstrURL, cstrURL);
            NS_ENSURE_SUCCESS(rv, rv);
          }

          // Finally, fixup the URL.  This is useful to turn Window's absolute
          // paths into actual file:// style URLs.
          nsCOMPtr<nsIURI> fixedURI;
          rv = fixup->CreateFixupURI(cstrURL, nsIURIFixup::FIXUP_FLAG_NONE,
                                     getter_AddRefs(fixedURI));
          NS_ENSURE_SUCCESS(rv, rv);
          rv = fixedURI->GetSpec(cstrURL);
          NS_ENSURE_SUCCESS(rv, rv);

          strURL.Assign(NS_ConvertUTF8toUTF16(cstrURL));

          // Try and load the entry as a playlist
          // If it's not a playlist add it as a playstring
          {
            const static PRUnichar *aMetaKeys[] = {lengthMetadata.get(), titleMetadata.get()};
            const static PRUint32 nMetaKeyCount = sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

            PRBool bRet = PR_FALSE;
            if(strTitle.IsEmpty()) strTitle = strURL;

            PRUnichar** aMetaValues = (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
            aMetaValues[0] = ToNewUnicode(strLength);
            aMetaValues[1] = ToNewUnicode(strTitle);

            nsAutoString guid;
            pLibrary->AddMedia(strURL, nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), m_Replace, PR_TRUE, guid);

            if(!guid.IsEmpty() && pPlaylist)
            {
              pPlaylist->AddByGUID(guid, nsAutoString(strGUID), -1, m_Replace, PR_TRUE, &bRet);
            }

            nsMemory::Free(aMetaValues[0]);
            nsMemory::Free(aMetaValues[1]);
            nsMemory::Free(aMetaValues);
          }
          strTitle = EmptyString();
          strURL = EmptyString();
        }
      }
    }

    fileStart = lineStart;
    playlistBuffer.EndReading(lineEnd);
  } 
  while( fileStart != fileEnd );

  PRInt32 qrv = 0;
  pQuery->Execute(&qrv);

  return qrv;
} //ParseM3UFromBuffer
