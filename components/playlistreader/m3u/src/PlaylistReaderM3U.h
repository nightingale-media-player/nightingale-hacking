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
 * \file PlaylistReaderM3U.h
 * \brief 
 */

#pragma once

// INCLUDES ===================================================================

// XXX Remove Me !!!
#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
}
#endif

#include "IPlaylistReader.h"

#include <xpcom/nsIFile.h>
#include <nspr/prlock.h>

#include <string>

// DEFINES ====================================================================
#define SONGBIRD_PLREADERM3U_CONTRACTID  "@songbird.org/Songbird/Playlist/Reader/M3U;1"
#define SONGBIRD_PLREADERM3U_CLASSNAME   "Songbird Playlist Reader for M3U Playlists"
#define SONGBIRD_PLREADERM3U_CID { 0x7295bde2, 0xcb17, 0x4bf3, { 0x88, 0x33, 0x68, 0xf5, 0x17, 0x20, 0x0, 0xa7 } }

// CLASSES ====================================================================
class CPlaylistReaderM3U : public sbIPlaylistReader
{
public:
  CPlaylistReaderM3U();
  virtual ~CPlaylistReaderM3U();

  NS_DECL_ISUPPORTS;
  NS_DECL_SBIPLAYLISTREADER;

  PRInt32 ParseM3UFromBuffer(PRUnichar *pPathToFile, PRUnichar *pBuffer, PRInt32 nBufferLen, PRUnichar *strGUID, PRUnichar *strDestTable);

protected:
  PRLock* m_pNameLock;
  std::prustring m_Name;

  PRLock* m_pDescriptionLock;
  std::prustring m_Description;

  PRBool m_Replace;
};