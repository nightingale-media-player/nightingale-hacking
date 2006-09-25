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
 * \file PlaylistReaderPLS.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include <string>
#include <algorithm>
#include <map>

#include "sbIDatabaseQuery.h"
#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"
#include "PlaylistReaderPLS.h"

#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsAutoLock.h>
#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsNetUtil.h>
#include <docshell/nsIURIFixup.h>

#include <webbrowserpersist/nsIWebBrowserPersist.h>

#include <unicharutil/nsUnicharUtils.h>

#include <prmem.h>

// CLASSES ====================================================================
//=============================================================================
// class CPlaylistReaderPLS
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_ISUPPORTS1(CPlaylistReaderPLS, sbIPlaylistReader)

//-----------------------------------------------------------------------------
CPlaylistReaderPLS::CPlaylistReaderPLS()
: m_Replace(PR_FALSE)
, m_pDescriptionLock(PR_NewLock())
, m_pNameLock(PR_NewLock())
, m_pOriginalURLLock(PR_NewLock())
{
  NS_ASSERTION(m_pDescriptionLock, "PlaylistReaderPLS.m_pDescriptionLock failed");
  NS_ASSERTION(m_pNameLock, "PlaylistReaderPLS.m_pNameLock failed");
  NS_ASSERTION(m_pOriginalURLLock, "PlaylistReaderPLS.m_pOriginalURLLock failed");

  m_Name.AssignLiteral("Songbird PLS Reader");
  m_Description.AssignLiteral("Loads PLS playlist files from remote and local locations.");
} //ctor

//-----------------------------------------------------------------------------
CPlaylistReaderPLS::~CPlaylistReaderPLS()
{
  if (m_pDescriptionLock)
    PR_DestroyLock(m_pDescriptionLock);
  if (m_pNameLock)
    PR_DestroyLock(m_pNameLock);
} //dtor

//-----------------------------------------------------------------------------
/* attribute wstring originalURL; */
NS_IMETHODIMP CPlaylistReaderPLS::SetOriginalURL(const nsAString &strURL)
{
  nsAutoLock lock(m_pOriginalURLLock);
  m_OriginalURL = strURL;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CPlaylistReaderPLS::GetOriginalURL(nsAString &strURL)
{
  nsAutoLock lock(m_pOriginalURLLock);
  strURL = m_OriginalURL;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool Read (in wstring strURL, in wstring strGUID, in wstring strDestTable, out PRInt32 errorCode); */
NS_IMETHODIMP CPlaylistReaderPLS::Read(const nsAString &strURL, const nsAString &strGUID, const nsAString &strDestTable, PRBool bReplace, PRInt32 *errorCode, PRBool *_retval)
{
  *_retval = PR_FALSE;
  *errorCode = 0;
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCAutoString  cstrURL;

  nsCOMPtr<nsIFileInputStream> pFileReader = do_GetService("@mozilla.org/network/file-input-stream;1");

  nsCOMPtr<nsIFile> pFile;
  rv = NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(strURL), getter_AddRefs(pFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> pURI;
  rv = NS_NewFileURI(getter_AddRefs(pURI), pFile);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool bIsFile = PR_FALSE;
  rv = pFile->IsFile(&bIsFile);

  if(NS_FAILED(rv) || !bIsFile) return NS_ERROR_UNEXPECTED;

  rv = pFileReader->Init(pFile, PR_RDONLY, 0, 0 /*nsIFileInputStream::CLOSE_ON_EOF*/);
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
    *errorCode = ParsePLSFromBuffer(pURI, 
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
NS_IMETHODIMP CPlaylistReaderPLS::Vote(const nsAString &strUrl, PRInt32 *_retval)
{
  *_retval = 10000;
  return NS_OK;
} //Vote

//-----------------------------------------------------------------------------
/* wstring Name (); */
NS_IMETHODIMP CPlaylistReaderPLS::Name(nsAString &_retval)
{
  nsAutoLock lock(m_pNameLock);
  _retval = m_Name;
  return NS_OK;
} //Name

//-----------------------------------------------------------------------------
/* wstring Description (); */
NS_IMETHODIMP CPlaylistReaderPLS::Description(nsAString &_retval)
{
  nsAutoLock lock(m_pDescriptionLock);
  _retval = m_Description;
  return NS_OK;
} //Description

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out string aMIMETypes); */
NS_IMETHODIMP CPlaylistReaderPLS::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  NS_NAMED_LITERAL_STRING(plsMime, "audio/x-scpls");
  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(1 * sizeof(PRUnichar *));

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  out[0] = ToNewUnicode(plsMime);
  if(!out[0])
    return NS_ERROR_OUT_OF_MEMORY;

  *nMIMECount = 1;
  *aMIMETypes = out;

  return NS_OK;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out string aExts); */
NS_IMETHODIMP CPlaylistReaderPLS::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  NS_NAMED_LITERAL_STRING(plsExt, "pls");
  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(1 * sizeof(PRUnichar *));
  *nExtCount = 0;

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  out[0] = ToNewUnicode(plsExt);
  if(!out[0])
    return NS_ERROR_OUT_OF_MEMORY;

  *nExtCount = 1;
  *aExts = out;

  return NS_OK;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
PRInt32 CPlaylistReaderPLS::ParsePLSFromBuffer(nsIURI *pBaseURI, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable)
{
  nsresult rv;
  PRBool bSuccess = PR_FALSE;
  PRBool bInPlaylist = PR_FALSE;

  std::map<nsString, nsString> list;
  nsDependentString playlistBuffer(pBuffer);
  nsDependentString::const_iterator fileStart, lineStart, lineEnd, fileEnd;
  
  playlistBuffer.BeginReading(fileStart); 
  playlistBuffer.BeginReading(lineStart);
  playlistBuffer.EndReading(lineEnd); 
  playlistBuffer.EndReading(fileEnd);

  NS_NAMED_LITERAL_STRING(carReturn, "\r");
  NS_NAMED_LITERAL_STRING(lineFeed, "\n");
  NS_NAMED_LITERAL_STRING(openBracket, "[");
  NS_NAMED_LITERAL_STRING(closeBracket, "]");
  NS_NAMED_LITERAL_STRING(equalSign, "=");
  NS_NAMED_LITERAL_STRING(timeSeperator, ":");
  NS_NAMED_LITERAL_STRING(playlistToken, "[playlist]");
  NS_NAMED_LITERAL_STRING(lengthMetadata, "length");
  NS_NAMED_LITERAL_STRING(titleMetadata, "title");
  
  nsAutoString theline;

  nsCOMPtr<nsIURIFixup> fixup = do_GetService("@mozilla.org/docshell/urifixup;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  do
  {
    PRBool bFound = FindInReadable(carReturn, lineStart, lineEnd);

    if(!bFound)
    {
      lineStart = fileStart;
      playlistBuffer.EndReading(lineEnd);

      bFound = FindInReadable(lineFeed, lineStart, lineEnd);
    }

    if( bFound )
    {
      theline = Substring(fileStart, lineEnd);
      while( (lineStart != fileEnd) && (*lineStart == carReturn[0] || *lineStart == lineFeed[0] ) ) lineStart++;
    }

    if( FindInReadable(playlistToken, theline) )
    {
      bInPlaylist = PR_TRUE;
    }
    else if( FindInReadable(openBracket, theline) && FindInReadable(closeBracket, theline) && !FindInReadable(equalSign, theline) )
    {
      bInPlaylist = PR_FALSE;
    }

    if ( bInPlaylist )
    {
      nsAutoString::const_iterator thelineStart, start, end;
      
      theline.BeginReading(thelineStart); 
      theline.BeginReading(start); 
      theline.EndReading(end);

      if( FindInReadable(equalSign, start, end) )
      {
        nsAutoString key, val;
        key = Substring(thelineStart, --end);
        start = ++end;
        theline.EndReading(end);
        val = Substring(start, end);
        
        key.StripWhitespace();
        val.CompressWhitespace();
        ToLowerCase(key);

        list[key] = val;
      }
    }

    fileStart = lineStart;
    playlistBuffer.EndReading(lineEnd);

  } while( fileStart != fileEnd );

  PRInt32 convError = 0;
  PRInt32 nURLs = list[NS_LITERAL_STRING("numberofentries")].ToInteger(&convError);
  PRBool bExtraInfo = list[NS_LITERAL_STRING("version")].ToInteger(&convError) > 1;
  if ( nURLs > 0 )
  {
    nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
    if(!pQuery) return NS_ERROR_UNEXPECTED;
    pQuery->SetAsyncQuery(PR_FALSE);
    pQuery->SetDatabaseGUID(nsAutoString(strGUID));

    nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );
    if(!pLibrary) return NS_ERROR_UNEXPECTED;
    pLibrary->SetQueryObject(pQuery);

    nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
    if(!pPlaylistManager) return NS_ERROR_UNEXPECTED;

    nsCOMPtr<sbIPlaylist> pPlaylist;
    pPlaylistManager->GetPlaylist(nsAutoString(strDestTable), pQuery.get(), getter_AddRefs(pPlaylist));
    if(!pPlaylist) return NS_ERROR_UNEXPECTED;

    for( PRInt64 i = 1; i <= nURLs; i++ )
    {
      nsAutoString strURL, strTitle, strLength;
      nsAutoString key;
      key.AssignLiteral("file");
      key.AppendInt( i );

      strURL = list[key];

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

      if ( bExtraInfo )
      {
        key.AssignLiteral("title");
        key.AppendInt( i );
        strTitle = list[key];
        
        key.AssignLiteral("length");
        key.AppendInt( i );
        strLength = list[key];
        
        PRInt32 nLen = strLength.ToInteger(&convError);
        if(NS_SUCCEEDED(convError) && nLen > 0)
        {
          PRInt32 nMins = nLen / 60;
          PRInt32 nSecs = nLen % 60;

          strLength = EmptyString();
          
          strLength.AppendInt(nMins);
          strLength += timeSeperator;
          
          if(nSecs < 10) strLength.Append(NS_LITERAL_STRING("0"));
          strLength.AppendInt(nSecs);
        }
      }
      else
      {
        strTitle = strLength = EmptyString();
      }

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
        pLibrary->AddMedia(strURL, nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), m_Replace, PR_FALSE, guid);

        if(!guid.IsEmpty() && pPlaylist)
        {
          pPlaylist->AddByGUID(guid, nsAutoString(strGUID), -1, m_Replace, PR_FALSE, &bRet);
        }

        nsMemory::Free(aMetaValues[0]);
        nsMemory::Free(aMetaValues[1]);
        nsMemory::Free(aMetaValues);
      }
    }

    PRInt32 qrv = 0;
    pQuery->Execute(&qrv);

    if(!qrv)
      bSuccess = PR_TRUE;
  }

  return rv;
} //ParsePLSFromBuffer