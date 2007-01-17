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
 * \file PlaylistReaderM3U.h
 * \brief 
 */

#ifndef __PLAYLIST_READER_M3U_H__
#define __PLAYLIST_READER_M3U_H__

// INCLUDES ===================================================================
#include "sbIPlaylistReader.h"

#include <nsStringGlue.h>
#include <necko/nsIURI.h>
#include <nspr/prlock.h>

// DEFINES ====================================================================
#define SONGBIRD_PLREADERM3U_CONTRACTID                   \
  "@songbirdnest.com/Songbird/Playlist/Reader/M3U;1"
#define SONGBIRD_PLREADERM3U_CLASSNAME                    \
  "Songbird Playlist Reader for M3U Playlists"
#define SONGBIRD_PLREADERM3U_CID                          \
{ /* 87cb8a38-d664-49a6-9bcf-ce2d26deb0fd */              \
  0x87cb8a38,                                             \
  0xd664,                                                 \
  0x49a6,                                                 \
  {0x9b, 0xcf, 0xce, 0x2d, 0x26, 0xde, 0xb0, 0xfd}        \
}

// CLASSES ====================================================================
class CPlaylistReaderM3U : public sbIPlaylistReader
{
public:
  CPlaylistReaderM3U();
  virtual ~CPlaylistReaderM3U();

  NS_DECL_ISUPPORTS;
  NS_DECL_SBIPLAYLISTREADER;

  PRInt32 ParseM3UFromBuffer(nsIURI *pBaseURI, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable);

protected:
  PRLock*   m_pNameLock;
  nsString  m_Name;

  PRLock*   m_pOriginalURLLock;
  nsString  m_OriginalURL;

  PRLock*   m_pDescriptionLock;
  nsString  m_Description;

  PRBool    m_Replace;
};

#endif // __PLAYLIST_READER_M3U_H__

