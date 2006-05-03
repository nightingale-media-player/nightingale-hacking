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
* \file  MediaKeyboard.h
* \brief Songbird Media Keyboard Manager.
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
};
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include "IMediaKeyboard.h"
#include <list>

// DEFINES ====================================================================
#define SONGBIRD_MEDIAKEYBOARD_CONTRACTID  "@songbird.org/Songbird/MediaKeyboard;1"
#define SONGBIRD_MEDIAKEYBOARD_CLASSNAME   "Songbird Media Keyboard Manager"

// {7007FD60-E3A8-46a0-9FF7-2B025E44D725}
#define SONGBIRD_MEDIAKEYBOARD_CID { 0x7007fd60, 0xe3a8, 0x46a0, { 0x9f, 0xf7, 0x2b, 0x2, 0x5e, 0x44, 0xd7, 0x25 } }

// CLASSES ====================================================================
class CMediaKeyboard : public sbIMediaKeyboard
{
public:
  CMediaKeyboard();
  virtual ~CMediaKeyboard();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAKEYBOARD

#ifdef WIN32
  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
  
protected:
#ifdef WIN32
  HWND m_window;
#endif  
  
  void OnMute();
  void OnVolumeUp();
  void OnVolumeDown();
  void OnNextTrack();
  void OnPrevTrack();
  void OnStop();
  void OnPlayPause();
  
  static std::list<sbIMediaKeyboardCallback*> m_callbacks;
};

