/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

#include "IDatabaseQuery.h"
#include "PlaylistReaderM3U.h"
#include "IMediaLibrary.h"
#include "IPlaylist.h"

#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <xpcom/nsILocalFile.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <string/nsStringAPI.h>
#include <string/nsString.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>

#include <prmem.h>

// CLASSES ====================================================================
//=============================================================================
// class CPlaylistReaderM3U
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_ISUPPORTS1(CPlaylistReaderM3U, sbIPlaylistReader)

//-----------------------------------------------------------------------------
CPlaylistReaderM3U::CPlaylistReaderM3U()
: m_Name(NS_LITERAL_STRING("Songbird M3U Reader").get())
, m_Description(NS_LITERAL_STRING("Loads M3U playlists from remote and local locations.").get())
, m_Replace(PR_FALSE)
, m_pDescriptionLock(PR_NewLock())
, m_pNameLock(PR_NewLock())
{
  NS_ASSERTION(m_pDescriptionLock, "PlaylistReaderPLS.m_pDescriptionLock failed");
  NS_ASSERTION(m_pNameLock, "PlaylistReaderPLS.m_pNameLock failed");
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
/* attribute wstring originalURL; */
NS_IMETHODIMP CPlaylistReaderM3U::SetOriginalURL(const PRUnichar *strURL)
{
  return NS_OK;
}
NS_IMETHODIMP CPlaylistReaderM3U::GetOriginalURL(PRUnichar **strURL)
{
  *strURL = nsnull;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool Read (in wstring strURL, in wstring strGUID, in wstring strDestTable, out PRInt32 errorCode); */
NS_IMETHODIMP CPlaylistReaderM3U::Read(const PRUnichar *strURL, const PRUnichar *strGUID, const PRUnichar *strDestTable, PRBool bReplace, PRInt32 *errorCode, PRBool *_retval)
{
  *_retval = PR_FALSE;
  *errorCode = 0;
  nsresult ret = NS_ERROR_UNEXPECTED;
  
  nsString strTheURL(strURL);
  nsCString cstrURL;
  
  nsCOMPtr<nsIFileInputStream> pFileReader = do_GetService("@mozilla.org/network/file-input-stream;1");
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");
  nsCOMPtr<nsIURI> pURI = do_GetService("@mozilla.org/network/simple-uri;1");

  if(!pFile || !pURI || !pFileReader) return ret;

  NS_UTF16ToCString(strTheURL, NS_CSTRING_ENCODING_ASCII, cstrURL);
  pURI->SetSpec(cstrURL);

  nsCString cstrScheme;
  pURI->GetScheme(cstrScheme);

  nsCString cstrPathToPLS;
  if(!cstrScheme.Equals(nsCString("file")))
  {
    //Yeah... it's supposed to be on disk by now, oops?
    return NS_ERROR_UNEXPECTED;
  }
  else
  {
    pURI->GetPath(cstrPathToPLS);
    cstrPathToPLS.Cut(0, 3); //remove '///'

    ret = pFile->InitWithNativePath(cstrPathToPLS);
  }

  if(NS_FAILED(ret)) return ret;

  PRBool bIsFile = PR_FALSE;
  pFile->IsFile(&bIsFile);

  if(!bIsFile) return NS_ERROR_NOT_IMPLEMENTED;

  ret = pFileReader->Init(pFile.get(), PR_RDONLY, 0, nsIFileInputStream::CLOSE_ON_EOF);
  if(NS_FAILED(ret)) return ret;

  PRInt64 nFileSize = 0;
  pFile->GetFileSize(&nFileSize);

  char *pBuffer = (char *)PR_Calloc(nFileSize, 1);
  pFileReader->Read(pBuffer, nFileSize, &ret);
  pFileReader->Close();
  pBuffer[nFileSize - 1] = 0;

  if(NS_SUCCEEDED(ret))
  {
    nsCString cstrBuffer(pBuffer);
    nsString strPathToPLS;
    nsString strBuffer;

    NS_CStringToUTF16(cstrPathToPLS, NS_CSTRING_ENCODING_ASCII, strPathToPLS);
    NS_CStringToUTF16(cstrBuffer, NS_CSTRING_ENCODING_ASCII, strBuffer);

    m_Replace = bReplace;
    *errorCode = ParseM3UFromBuffer(const_cast<PRUnichar *>(strPathToPLS.get()), const_cast<PRUnichar *>(strBuffer.get()), strBuffer.Length() + 1, const_cast<PRUnichar *>(strGUID), const_cast<PRUnichar *>(strDestTable));

    if(!*errorCode)
    {
      ret = NS_OK;
      *_retval = PR_TRUE;
    }
  }

  PR_Free(pBuffer);

  return ret;
} //Read

//-----------------------------------------------------------------------------
/* PRInt32 Vote (); */
NS_IMETHODIMP CPlaylistReaderM3U::Vote(const PRUnichar *strUrl, PRInt32 *_retval)
{
  *_retval = 10000;
  return NS_OK;
} //Vote

//-----------------------------------------------------------------------------
/* wstring Name (); */
NS_IMETHODIMP CPlaylistReaderM3U::Name(PRUnichar **_retval)
{
  nsAutoLock lock(m_pNameLock);

  size_t nLen = m_Name.length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(m_Name.c_str(), nLen * sizeof(PRUnichar));
  
  if(*_retval == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
} //Name

//-----------------------------------------------------------------------------
/* wstring Description (); */
NS_IMETHODIMP CPlaylistReaderM3U::Description(PRUnichar **_retval)
{
  nsAutoLock lock(m_pDescriptionLock);
  
  size_t nLen = m_Description.length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(m_Description.c_str(), nLen * sizeof(PRUnichar));

  if(*_retval == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
} //Description

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out string aMIMETypes); */
NS_IMETHODIMP CPlaylistReaderM3U::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{ 
  const static PRUnichar *MIMETypes[] = {NS_L("audio/mpegurl"), NS_L("audio/x-mpegurl")};
  const static PRUint32 MIMETypesCount = sizeof(MIMETypes) / sizeof(MIMETypes[0]);

  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(MIMETypesCount * sizeof(PRUnichar *));

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  for(PRUint32 i = 0; i < MIMETypesCount; ++i)
  {
    out[i] = (PRUnichar *) nsMemory::Clone(MIMETypes[i], (wcslen(MIMETypes[i]) + 1) * sizeof(PRUnichar));

    if(!out[i])
      return NS_ERROR_OUT_OF_MEMORY;
  }

  *nMIMECount = MIMETypesCount;
  *aMIMETypes = out;

  return NS_OK;
} //SupportedMIMETypes

//-----------------------------------------------------------------------------
/* void SupportedFileExtensions (out PRUint32 nExtCount, [array, size_is (nExtCount), retval] out string aExts); */
NS_IMETHODIMP CPlaylistReaderM3U::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  const static PRUnichar *Exts[] = {NS_LITERAL_STRING("m3u").get()};
  const static PRUint32 ExtsCount = sizeof(Exts) / sizeof(Exts[0]);

  PRUnichar** out = (PRUnichar **) nsMemory::Alloc(ExtsCount * sizeof(PRUnichar *));

  if(!out)
    return NS_ERROR_OUT_OF_MEMORY;

  for(PRUint32 i = 0; i < ExtsCount; ++i)
  {
    out[i] = (PRUnichar *) nsMemory::Clone(Exts[i], (wcslen(Exts[i]) + 1) * sizeof(PRUnichar));

    if(!out[i])
      return NS_ERROR_OUT_OF_MEMORY;
  }

  *nExtCount = ExtsCount;
  *aExts = out;

  return NS_OK;
} //SupportedFileExtensions

//-----------------------------------------------------------------------------
PRInt32 CPlaylistReaderM3U::ParseM3UFromBuffer(PRUnichar *pPathToFile, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable)
{
  PRInt32 ret = 0;
  wchar_t *strNextLine = nsnull;
  std::prustring strURL = NS_LITERAL_STRING("").get();
  std::prustring strTitle = NS_LITERAL_STRING("").get();
  std::prustring strLength = NS_LITERAL_STRING("-1").get();
  int nAvail = 0;

  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbird.org/Songbird/DatabaseQuery;1" );
  pQuery->SetAsyncQuery(PR_TRUE);
  pQuery->SetDatabaseGUID(strGUID);

  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbird.org/Songbird/MediaLibrary;1" );
  pLibrary->SetQueryObject(pQuery.get());

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbird.org/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIPlaylist> pPlaylist;

  pPlaylistManager->GetPlaylist(strDestTable, pQuery.get(), getter_AddRefs(pPlaylist));

  do
  {
    strNextLine = wcsstr( (wchar_t *)pBuffer, NS_L("\n"));
    if ( strNextLine )
    {
      *strNextLine++ = '\0';
    }
    
    while( *pBuffer && *pBuffer == ' ' || *pBuffer == '\t' || *pBuffer == '\r')
      pBuffer++;

    if ( !wcsncmp( (wchar_t *)pBuffer, NS_L("#EXTM3U"), 7 ) )
    {
      nAvail = 1;
    } 
    else if ( nAvail && ! wcsncmp( (wchar_t *)pBuffer, NS_L("#EXTINF:"), 8 ) )
    {
      wchar_t *title = (wchar_t *)pBuffer + 8;
      strLength = NS_LITERAL_STRING("").get();

      while( (*title >= '0' && *title <= '9') || *title == '-' )
        strLength += *title++;
      title++;

      {
        int nLen = _wtoi((wchar_t *)strLength.c_str());
        if(nLen > 0)
        {
          int nMins = nLen / 60;
          int nSecs = nLen % 60;
          wchar_t wszBuf[64] = {0};

          _itow(nMins, wszBuf, 10);

          strLength = wszBuf;
          strLength += ':';

          _itow(nSecs, wszBuf, 10);

          if(nSecs < 10)
            strLength += '0';

          strLength += wszBuf;
        }
      }

      //chop pesky \r or \n.
      int len = wcslen(title);
      if(title[len - 1] == '\r') title[len - 1] = 0;

      strTitle = title;
    }

    if ( ! wcsncmp( (wchar_t *)pBuffer, NS_L("ASF "), 4 ) )
      pBuffer += 4;

    if ( *pBuffer && *pBuffer != '#' && *pBuffer != '\r' && *pBuffer != '\n')
    {
      strURL = pBuffer;
      size_t nPos = strURL.find('\r');
      if(nPos != strURL.npos) strURL = strURL.substr(0, nPos);

#if defined(_WIN32)
      if(strURL.length() && (*strURL.begin()) == '\\')
      {
        std::prustring strPath(pPathToFile);
        //Drive letter is missing, append it.
        strURL = strPath.substr(0, 2) + strURL;
      }
#else
  #error "PORTME"
#endif

      // Try and load the entry as a playlist
      // If it's not a playlist add it as a playstring
      {
        const static PRUnichar *aMetaKeys[] = {NS_LITERAL_STRING("length").get(), NS_LITERAL_STRING("title").get()};
        const static PRUint32 nMetaKeyCount = sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

        PRBool bRet = PR_FALSE;
        PRUnichar *pGUID = nsnull;

        if(strTitle.empty()) strTitle = strURL;

        PRUnichar** aMetaValues = (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
        aMetaValues[0] = (PRUnichar *) nsMemory::Clone(strLength.c_str(), (wcslen((wchar_t *)strLength.c_str()) + 1) * sizeof(PRUnichar));
        aMetaValues[1] = (PRUnichar *) nsMemory::Clone(strTitle.c_str(), (wcslen((wchar_t *)strTitle.c_str()) + 1) * sizeof(PRUnichar));

        pLibrary->AddMedia(strURL.c_str(), nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), m_Replace, PR_TRUE, &pGUID);

        if(pGUID && pPlaylist.get())
        {
          pPlaylist->AddByGUID(pGUID, strGUID, -1, m_Replace, PR_TRUE, &bRet);
          nsMemory::Free(pGUID);
        }

        nsMemory::Free(aMetaValues[0]);
        nsMemory::Free(aMetaValues[1]);
        nsMemory::Free(aMetaValues);
      }

      strTitle.clear();
      strURL.clear();
    }
  } 
  while( (pBuffer = (PRUnichar *)strNextLine) != nsnull && *pBuffer );

  pQuery->Execute(&ret);

  return ret;
} //ParseM3UFromBuffer
