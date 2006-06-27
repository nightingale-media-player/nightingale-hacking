/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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

#ifndef __MEDIA_KEYBOARD_H__
#define __MEDIA_KEYBOARD_H__


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
#define SONGBIRD_MEDIAKEYBOARD_CONTRACTID                 \
  "@songbirdnest.com/Songbird/MediaKeyboard;1"
#define SONGBIRD_MEDIAKEYBOARD_CLASSNAME                  \
  "Songbird Media Keyboard Manager"
#define SONGBIRD_MEDIAKEYBOARD_CID                        \
{ /* d55b25ec-4c04-4453-b3fa-641b6e8a5a87 */              \
  0xd55b25ec,                                             \
  0x4c04,                                                 \
  0x4453,                                                 \
  {0xb3, 0xfa, 0x64, 0x1b, 0x6e, 0x8a, 0x5a, 0x87}        \
}
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

#endif // __MEDIA_KEYBOARD_H__

