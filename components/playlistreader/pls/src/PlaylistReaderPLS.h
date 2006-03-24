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
 * \file PlaylistReaderPLS.h
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
#define SONGBIRD_PLREADERPLS_CONTRACTID  "@songbird.org/Songbird/Playlist/Reader/PLS;1"
#define SONGBIRD_PLREADERPLS_CLASSNAME   "Songbird Playlist Reader for PLS Playlists"

// {89CB756E-299A-4146-8ABA-2A6554D143C0}
#define SONGBIRD_PLREADERPLS_CID { 0x89cb756e, 0x299a, 0x4146, { 0x8a, 0xba, 0x2a, 0x65, 0x54, 0xd1, 0x43, 0xc0 } }

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
  PRLock* m_pNameLock;
  std::prustring m_Name;

  PRLock* m_pDescriptionLock;
  std::prustring m_Description;

  PRBool m_Replace;
};