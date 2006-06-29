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
 * \file PlaylistReaderPLS.h
 * \brief 
 */

#ifndef __PLAYLIST_READER_PLS_H__
#define __PLAYLIST_READER_PLS_H__

// INCLUDES ===================================================================

#include "IPlaylistReader.h"
#include <string/nsString.h>
#include <xpcom/nsIFile.h>
#include <nspr/prlock.h>

// DEFINES ====================================================================
#define SONGBIRD_PLREADERPLS_CONTRACTID                   \
  "@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"
#define SONGBIRD_PLREADERPLS_CLASSNAME                    \
  "Songbird Playlist Reader for PLS Playlists"
#define SONGBIRD_PLREADERPLS_CID                          \
{ /* efe1b6b0-fb9a-40a1-8876-42d9a035291f */              \
  0xefe1b6b0,                                             \
  0xfb9a,                                                 \
  0x40a1,                                                 \
  {0x88, 0x76, 0x42, 0xd9, 0xa0, 0x35, 0x29, 0x1f}        \
}
// CLASSES ====================================================================
class CPlaylistReaderPLS : public sbIPlaylistReader
{
public:
  CPlaylistReaderPLS();
  virtual ~CPlaylistReaderPLS();

  NS_DECL_ISUPPORTS;
  NS_DECL_SBIPLAYLISTREADER;

  PRInt32 ParsePLSFromBuffer(PRUnichar *pPathToFile, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable);

protected:
  PRLock*   m_pNameLock;
  nsString  m_Name;

  PRLock*   m_pDescriptionLock;
  nsString  m_Description;

  PRBool m_Replace;
};

#endif // __PLAYLIST_READER_PLS_H__

