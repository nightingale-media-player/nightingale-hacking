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
 * \file MediaKeyboard.cpp
 * \brief Songbird Media Keyboard Manager Implementation.
 */
 
#include "MediaKeyboard.h"
#include "nsString.h"

// CLASSES ====================================================================
//=============================================================================
// CMediaKeyboard Class
//=============================================================================

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CMediaKeyboard, sbIMediaKeyboard)

#define MEDIAKEYBOARD_WNDCLASS NS_L("sbMediaKeyboardWindow")
#define HOTKEY_VOLUME_MUTE         0x100
#define HOTKEY_VOLUME_DOWN         0x101
#define HOTKEY_VOLUME_UP           0x102
#define HOTKEY_MEDIA_NEXT_TRACK    0x103
#define HOTKEY_MEDIA_PREV_TRACK    0x104
#define HOTKEY_MEDIA_STOP          0x105
#define HOTKEY_MEDIA_PLAY_PAUSE    0x106

//-----------------------------------------------------------------------------
static LRESULT CALLBACK MediaKeyboardProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CMediaKeyboard *_this = reinterpret_cast<CMediaKeyboard*>(GetWindowLong(hWnd, GWL_USERDATA));
  return _this->WndProc(hWnd, uMsg, wParam, lParam);
} // MediaKeyboardProc

//-----------------------------------------------------------------------------
CMediaKeyboard::CMediaKeyboard()
{
#ifdef WIN32
  m_window = NULL;

  WNDCLASS wndClass;
  memset(&wndClass, 0, sizeof(wndClass));
  wndClass.hInstance = GetModuleHandle(NULL);
  wndClass.lpfnWndProc = MediaKeyboardProc;
  wndClass.lpszClassName = MEDIAKEYBOARD_WNDCLASS;
  RegisterClass(&wndClass);

  m_window = CreateWindow(MEDIAKEYBOARD_WNDCLASS, NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);
  SetWindowLong(m_window, GWL_USERDATA, (LPARAM)this);

  RegisterHotKey(m_window, HOTKEY_VOLUME_MUTE,      0, VK_VOLUME_MUTE);
  RegisterHotKey(m_window, HOTKEY_VOLUME_DOWN,      0, VK_VOLUME_DOWN);
  RegisterHotKey(m_window, HOTKEY_VOLUME_UP,        0, VK_VOLUME_UP);
  RegisterHotKey(m_window, HOTKEY_MEDIA_NEXT_TRACK, 0, VK_MEDIA_NEXT_TRACK);
  RegisterHotKey(m_window, HOTKEY_MEDIA_PREV_TRACK, 0, VK_MEDIA_PREV_TRACK);
  RegisterHotKey(m_window, HOTKEY_MEDIA_STOP,       0, VK_MEDIA_STOP);
  RegisterHotKey(m_window, HOTKEY_MEDIA_PLAY_PAUSE, 0, VK_MEDIA_PLAY_PAUSE);
#endif
} // ctor

//-----------------------------------------------------------------------------
CMediaKeyboard::~CMediaKeyboard()
{
#ifdef WIN32
  UnregisterHotKey(m_window, HOTKEY_VOLUME_MUTE);
  UnregisterHotKey(m_window, HOTKEY_VOLUME_DOWN);
  UnregisterHotKey(m_window, HOTKEY_VOLUME_UP);
  UnregisterHotKey(m_window, HOTKEY_MEDIA_NEXT_TRACK);
  UnregisterHotKey(m_window, HOTKEY_MEDIA_PREV_TRACK);
  UnregisterHotKey(m_window, HOTKEY_MEDIA_STOP);
  UnregisterHotKey(m_window, HOTKEY_MEDIA_PLAY_PAUSE);
  
  DestroyWindow(m_window);
  m_window = NULL;

  UnregisterClass(MEDIAKEYBOARD_WNDCLASS, GetModuleHandle(NULL));
#endif
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CMediaKeyboard::AddCallback( sbIMediaKeyboardCallback *callback)
{
  NS_ADDREF(callback);
  m_callbacks.push_back(callback);
  return NS_OK;
} // AddCallback

std::list<sbIMediaKeyboardCallback *> CMediaKeyboard::m_callbacks;

//-----------------------------------------------------------------------------
NS_IMETHODIMP CMediaKeyboard::RemoveCallback(sbIMediaKeyboardCallback *callback )
{
  m_callbacks.remove(callback);
  NS_RELEASE(callback);  
  return NS_OK;
} // RemoveCallback

//-----------------------------------------------------------------------------
LRESULT CMediaKeyboard::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_HOTKEY)
  {
    int idHotKey = (int) wParam; 
    switch (idHotKey) 
    {
      case HOTKEY_VOLUME_MUTE:      OnMute();       break;
      case HOTKEY_VOLUME_DOWN:      OnVolumeDown(); break;
      case HOTKEY_VOLUME_UP:        OnVolumeUp();   break;
      case HOTKEY_MEDIA_NEXT_TRACK: OnNextTrack();  break;
      case HOTKEY_MEDIA_PREV_TRACK: OnPrevTrack();  break;
      case HOTKEY_MEDIA_STOP:       OnStop();       break;
      case HOTKEY_MEDIA_PLAY_PAUSE: OnPlayPause();  break;
    }
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // WndProc

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnMute() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnMute();
} // OnMute

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnVolumeUp() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnVolumeUp();
} // OnVolumeUp

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnVolumeDown() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnVolumeDown();
} // OnVolumeDown

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnNextTrack() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnNextTrack();
} // OnNextTrack

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnPrevTrack() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnPreviousTrack();
} // OnPrevTrack

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnStop() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnStop();
} // OnStop

//-----------------------------------------------------------------------------
void CMediaKeyboard::OnPlayPause() 
{
  std::list<sbIMediaKeyboardCallback*>::iterator iter;
  for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) (*iter)->OnPlayPause();
} // OnPlayPause

