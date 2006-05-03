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
 * \file PlaylistReaderPLS.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include <string>
#include <algorithm>
#include <map>

#include "IDatabaseQuery.h"
#include "IMediaLibrary.h"
#include "IPlaylist.h"
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

#include <webbrowserpersist/nsIWebBrowserPersist.h>

#include <string/nsStringAPI.h>
#include <string/nsString.h>

#include <prmem.h>

// CLASSES ====================================================================
//=============================================================================
// class CPlaylistReaderPLS
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_ISUPPORTS1(CPlaylistReaderPLS, sbIPlaylistReader)

//-----------------------------------------------------------------------------
CPlaylistReaderPLS::CPlaylistReaderPLS()
: m_Name(NS_LITERAL_STRING("Songbird PLS Reader").get())
, m_Description(NS_LITERAL_STRING("Loads PLS playlist files from remote and local locations.").get())
, m_Replace(PR_FALSE)
, m_pDescriptionLock(PR_NewLock())
, m_pNameLock(PR_NewLock())
{
  NS_ASSERTION(m_pDescriptionLock, "PlaylistReaderPLS.m_pDescriptionLock failed");
  NS_ASSERTION(m_pNameLock, "PlaylistReaderPLS.m_pNameLock failed");
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
NS_IMETHODIMP CPlaylistReaderPLS::SetOriginalURL(const PRUnichar *strURL)
{
  return NS_OK;
}
NS_IMETHODIMP CPlaylistReaderPLS::GetOriginalURL(PRUnichar **strURL)
{
  *strURL = nsnull;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool Read (in wstring strURL, in wstring strGUID, in wstring strDestTable, out PRInt32 errorCode); */
NS_IMETHODIMP CPlaylistReaderPLS::Read(const PRUnichar *strURL, const PRUnichar *strGUID, const PRUnichar *strDestTable, PRBool bReplace, PRInt32 *errorCode, PRBool *_retval)
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

  NS_UTF16ToCString(strTheURL, NS_CSTRING_ENCODING_UTF8, cstrURL);
  pURI->SetSpec(cstrURL);

  nsCString cstrScheme;
  pURI->GetScheme(cstrScheme);

  nsCString cstrPathToPLS;
  if(!cstrScheme.Equals(nsCString("file")))
  {
    return NS_OK;
  }
  else
  {
    pURI->GetPath(cstrPathToPLS);
    cstrPathToPLS.Cut(0, 3); //remove '://'

    NS_CStringToUTF16(cstrPathToPLS, NS_CSTRING_ENCODING_UTF8, strTheURL);
    ret = pFile->InitWithPath(strTheURL);
  }

  if(NS_FAILED(ret)) return ret;

  PRBool bIsFile = PR_FALSE;
  pFile->IsFile(&bIsFile);

  if(!bIsFile) return NS_ERROR_NOT_IMPLEMENTED;

  ret = pFileReader->Init(pFile.get(), PR_RDONLY, 0, 0 /*nsIFileInputStream::CLOSE_ON_EOF*/);
  if(NS_FAILED(ret)) return ret;

  PRInt64 nFileSize = 0;
  pFile->GetFileSize(&nFileSize);

  char *pBuffer = (char *)PR_Calloc(nFileSize, 1);

  pFileReader->Read(pBuffer, nFileSize, &ret);
  pFileReader->Close();

  //pBuffer[nFileSize - 1] = 0;

  if(NS_SUCCEEDED(ret))
  {
    nsCString cstrBuffer(pBuffer);
    nsString strPathToPLS;
    nsString strBuffer;

    NS_CStringToUTF16(cstrPathToPLS, NS_CSTRING_ENCODING_UTF8, strPathToPLS);
    NS_CStringToUTF16(cstrBuffer, NS_CSTRING_ENCODING_UTF8, strBuffer);

    m_Replace = bReplace;
    *errorCode = ParsePLSFromBuffer(const_cast<PRUnichar *>(strPathToPLS.get()), const_cast<PRUnichar *>(strBuffer.get()), strBuffer.Length(), const_cast<PRUnichar *>(strGUID), const_cast<PRUnichar *>(strDestTable));

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
NS_IMETHODIMP CPlaylistReaderPLS::Vote(const PRUnichar *strUrl, PRInt32 *_retval)
{
  *_retval = 10000;
  return NS_OK;
} //Vote

//-----------------------------------------------------------------------------
/* wstring Name (); */
NS_IMETHODIMP CPlaylistReaderPLS::Name(PRUnichar **_retval)
{
  size_t nLen = m_Name.length() + 1;

  {
    nsAutoLock lock(m_pNameLock);
    *_retval = (PRUnichar *) nsMemory::Clone(m_Name.c_str(), nLen * sizeof(PRUnichar));
  }

  return NS_OK;
} //Name

//-----------------------------------------------------------------------------
/* wstring Description (); */
NS_IMETHODIMP CPlaylistReaderPLS::Description(PRUnichar **_retval)
{
  size_t nLen = m_Description.length() + 1;

  {
    nsAutoLock lock(m_pDescriptionLock);
    *_retval = (PRUnichar *) nsMemory::Clone(m_Description.c_str(), nLen * sizeof(PRUnichar));
  }

  return NS_OK;
} //Description

//-----------------------------------------------------------------------------
/* void SupportedMIMETypes (out PRUint32 nMIMECount, [array, size_is (nMIMECount), retval] out string aMIMETypes); */
NS_IMETHODIMP CPlaylistReaderPLS::SupportedMIMETypes(PRUint32 *nMIMECount, PRUnichar ***aMIMETypes)
{
  const static PRUnichar *MIMETypes[] = {NS_LITERAL_STRING("audio/x-scpls").get()};
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
NS_IMETHODIMP CPlaylistReaderPLS::SupportedFileExtensions(PRUint32 *nExtCount, PRUnichar ***aExts)
{
  const static PRUnichar *Exts[] = {NS_LITERAL_STRING("pls").get()};
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
PRInt32 CPlaylistReaderPLS::ParsePLSFromBuffer(PRUnichar *pPathToFile, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable)
{
  PRInt32 ret = 0;
  PRBool bSuccess = PR_FALSE;
  PRBool bInPlaylist = PR_FALSE;
  std::map<std::prustring,std::prustring> list;
  wchar_t *nextline;

  do
  {
    nextline = wcsstr( (wchar_t *)pBuffer, NS_L("\r")); // sue me.  this is a valid cast.

    if(!nextline)
      nextline = wcsstr( (wchar_t *)pBuffer, NS_L("\n"));

    if ( nextline )
    {
      *nextline++ = L'\0';
      while(*nextline != nsnull && (*nextline == '\r' || *nextline == '\n')) nextline++;
    }


    if ( wcsstr( (wchar_t *)pBuffer, NS_L("[playlist]") ) )
    {
      bInPlaylist = PR_TRUE;
    }
    else if ( wcsstr( (wchar_t *)pBuffer, NS_L("[") ) && wcsstr( (wchar_t *)pBuffer, NS_L("]") ) && !wcsstr((wchar_t *)pBuffer, NS_L("=")))
    {
      bInPlaylist = PR_FALSE;
    }

    if ( bInPlaylist )
    {
      wchar_t *val = wcsstr( (wchar_t *)pBuffer, NS_L("=") );
      if ( val )
      {
        *val++ = L'\0';
        std::prustring key = pBuffer;
        std::transform(key.begin(),key.end(), key.begin(), tolower);
        list[key] = val;
      }
    }

  } while( (pBuffer = nextline) != nsnull );

  int nURLs = _wtoi( list[NS_LITERAL_STRING("numberofentries").get()].c_str() );
  PRBool bExtraInfo = _wtoi( list[NS_LITERAL_STRING("version").get()].c_str() )>1;
  if ( nURLs > 0 )
  {
    nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbird.org/Songbird/DatabaseQuery;1" );
    pQuery->SetAsyncQuery(PR_FALSE);
    pQuery->SetDatabaseGUID(strGUID);

    nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbird.org/Songbird/MediaLibrary;1" );
    pLibrary->SetQueryObject(pQuery.get());

    nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbird.org/Songbird/PlaylistManager;1" );
    nsCOMPtr<sbIPlaylist> pPlaylist;
    
    pPlaylistManager->GetPlaylist(strDestTable, pQuery.get(), getter_AddRefs(pPlaylist));

    for( PRInt64 i = 1; i <= nURLs; i++ )
    {
      std::prustring strURL, strTitle, strLength;
      nsString key = NS_LITERAL_STRING( "file" );
      key.AppendInt( i );
      strURL = list[key.get()];

#if defined(XP_WIN)
      if(strURL.length() && (*strURL.begin()) == L'\\')
      {
        std::prustring strPath(pPathToFile);
        //Drive letter is missing, append it.
        strURL = strPath.substr(0, 2) + strURL;
      }
#endif

      if ( bExtraInfo )
      {
        key = NS_LITERAL_STRING("title").get();
        key.AppendInt( i );
        strTitle = list[key.get()];
        
        key = NS_LITERAL_STRING("length").get();
        key.AppendInt( i );
        strLength = list[key.get()];

        int nLen = _wtoi(strLength.c_str());
        if(nLen > 0)
        {
          int nMins = nLen / 60;
          int nSecs = nLen % 60;
          wchar_t wszBuf[64] = {0};

          _itow(nMins, wszBuf, 10);

          strLength = wszBuf;
          strLength += NS_LITERAL_STRING(":").get();
          
          _itow(nSecs, wszBuf, 10);

          if(nSecs < 10)
            strLength += NS_LITERAL_STRING("0").get();

          strLength += wszBuf;
        }
      }
      else
      {
        strTitle = strLength = NS_LITERAL_STRING("").get();
      }

      // Try and load the entry as a playlist
      // If it's not a playlist add it as a playstring
      {
        const static PRUnichar *aMetaKeys[] = {NS_LITERAL_STRING("length").get(), NS_LITERAL_STRING("title").get()};
        const static PRUint32 nMetaKeyCount = sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

        PRBool bRet = PR_FALSE;
        PRUnichar *pGUID = nsnull;

        if(strTitle.empty()) strTitle = strURL;

        PRUnichar** aMetaValues = (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
        aMetaValues[0] = (PRUnichar *) nsMemory::Clone(strLength.c_str(), (wcslen(strLength.c_str()) + 1) * sizeof(PRUnichar));
        aMetaValues[1] = (PRUnichar *) nsMemory::Clone(strTitle.c_str(), (wcslen(strTitle.c_str()) + 1) * sizeof(PRUnichar));

        pLibrary->AddMedia(strURL.c_str(), nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), m_Replace, PR_FALSE, &pGUID);

        if(pGUID && pPlaylist.get())
        {
          pPlaylist->AddByGUID(pGUID, strGUID, -1, m_Replace, PR_FALSE, &bRet);
          nsMemory::Free(pGUID);
        }

        nsMemory::Free(aMetaValues[0]);
        nsMemory::Free(aMetaValues[1]);
        nsMemory::Free(aMetaValues);
      }
    }

    pQuery->Execute(&ret);

    if(!ret)
      bSuccess = PR_TRUE;
  }

  return ret;
} //ParsePLSFromBuffer