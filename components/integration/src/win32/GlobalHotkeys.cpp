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
 * \file GlobalHotkeys.cpp
 * \brief Songbird Global Hotkey Manager Implementation.
 */
 
#include "GlobalHotkeys.h"
#include "nsString.h"

// CLASSES ====================================================================
//=============================================================================
// CGlobalHotkeys Class
//=============================================================================

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CGlobalHotkeys, sbIGlobalHotkeys)

#ifdef WIN32

#define GLOBALHOTKEYS_WNDCLASS NS_L("sbGlobalHotkeys")
PRInt32 CGlobalHotkeys::m_autoinc = 1;

//-----------------------------------------------------------------------------
static LRESULT CALLBACK GlobalHotkeysProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CGlobalHotkeys *_this = reinterpret_cast<CGlobalHotkeys*>(GetWindowLong(hWnd, GWL_USERDATA));
  return _this->WndProc(hWnd, uMsg, wParam, lParam);
} // GlobalHotkeysProc

#endif

//-----------------------------------------------------------------------------
CGlobalHotkeys::CGlobalHotkeys()
{
#ifdef WIN32
  m_window = NULL;

  WNDCLASS wndClass;
  memset(&wndClass, 0, sizeof(wndClass));
  wndClass.hInstance = GetModuleHandle(NULL);
  wndClass.lpfnWndProc = GlobalHotkeysProc;
  wndClass.lpszClassName = GLOBALHOTKEYS_WNDCLASS;
  RegisterClass(&wndClass);

  m_window = CreateWindow(GLOBALHOTKEYS_WNDCLASS, NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);
  SetWindowLong(m_window, GWL_USERDATA, (LPARAM)this);
#endif
} // ctor

//-----------------------------------------------------------------------------
CGlobalHotkeys::~CGlobalHotkeys()
{
#ifdef WIN32
  RemoveAllHotkeys();
  DestroyWindow(m_window);
  m_window = NULL;
  UnregisterClass(GLOBALHOTKEYS_WNDCLASS, GetModuleHandle(NULL));
#endif
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CGlobalHotkeys::AddHotkey(PRInt32 keyCode, PRBool altKey, PRBool ctrlKey, PRBool shiftKey, PRBool metaKey, const PRUnichar *keyid, sbIGlobalHotkeyCallback *callback)
{
  NS_ENSURE_ARG_POINTER(keyid);
  HOTKEY_HANDLE handle = registerHotkey(keyCode, altKey, ctrlKey, shiftKey, metaKey);
  if (handle == NULL) return NS_ERROR_FAILURE;
  NS_ADDREF(callback);
  GlobalHotkeyEntry *entry = new GlobalHotkeyEntry();
  entry->m_callback = callback;
  entry->m_keyid = keyid;
  entry->m_handle = handle;
  m_hotkeys.push_back(entry);
  return NS_OK;
} // AddHotkey

std::list<GlobalHotkeyEntry *> CGlobalHotkeys::m_hotkeys;

//-----------------------------------------------------------------------------
NS_IMETHODIMP CGlobalHotkeys::RemoveHotkey(const PRUnichar *keyid)
{
  NS_ENSURE_ARG_POINTER(keyid);
  GlobalHotkeyEntry *entry = findHotkeyById(keyid);
  if (!entry) return NS_ERROR_FAILURE;
  removeEntry(entry);
  return NS_OK;
} // RemoveHotkey

//-----------------------------------------------------------------------------
void CGlobalHotkeys::removeEntry(GlobalHotkeyEntry *entry) {
  if (!entry) return;
  m_hotkeys.remove(entry);
  NS_RELEASE(entry->m_callback);  
  unregisterHotkey(entry->m_handle);
  delete entry;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CGlobalHotkeys::RemoveAllHotkeys()
{
  std::list<GlobalHotkeyEntry*>::iterator iter;
  while (m_hotkeys.size() > 0) {
    GlobalHotkeyEntry *entry = *m_hotkeys.begin();
    removeEntry(entry);
  }
  return NS_OK;
} // RemoveAllHotkeys


#ifdef WIN32

//-----------------------------------------------------------------------------
LRESULT CGlobalHotkeys::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_HOTKEY)
  {
    int idHotKey = (int) wParam; 
    GlobalHotkeyEntry *entry = findHotkeyByHandle(idHotKey);
    if (entry) {
      entry->m_callback->OnHotkey(entry->m_keyid.get());
    }
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // WndProc

#endif

//-----------------------------------------------------------------------------
HOTKEY_HANDLE CGlobalHotkeys::registerHotkey(PRInt32 keyCode, PRBool altKey, PRBool ctrlKey, PRBool shiftKey, PRBool metaKey)
{
#ifdef WIN32
  if (m_autoinc == 0xBFFF) return NULL;
  RegisterHotKey(m_window, m_autoinc, makeWin32Mask(altKey, ctrlKey, shiftKey, metaKey), keyCode);
  return m_autoinc++;
#endif
  return NULL;
}

//-----------------------------------------------------------------------------
void CGlobalHotkeys::unregisterHotkey(HOTKEY_HANDLE handle)
{
#ifdef WIN32
  UnregisterHotKey(m_window, handle);
#endif
}

//-----------------------------------------------------------------------------
GlobalHotkeyEntry *CGlobalHotkeys::findHotkeyById(const PRUnichar *keyid)
{
  std::list<GlobalHotkeyEntry*>::iterator iter;
  for (iter = m_hotkeys.begin(); iter != m_hotkeys.end(); iter++)
  {
    GlobalHotkeyEntry *entry = *iter;
    if (entry->m_keyid.Equals(keyid)) return entry;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
GlobalHotkeyEntry *CGlobalHotkeys::findHotkeyByHandle(HOTKEY_HANDLE handle)
{
  std::list<GlobalHotkeyEntry*>::iterator iter;
  for (iter = m_hotkeys.begin(); iter != m_hotkeys.end(); iter++)
  {
    GlobalHotkeyEntry *entry = *iter;
    if (entry->m_handle == handle) return entry;
  }
  return NULL;
}

#ifdef WIN32
//-----------------------------------------------------------------------------
UINT CGlobalHotkeys::makeWin32Mask(PRBool altKey, PRBool ctrlKey, PRBool shiftKey, PRBool metaKey)
{
  return (altKey   ? MOD_ALT     : 0) |
         (ctrlKey  ? MOD_CONTROL : 0) |
         (shiftKey ? MOD_SHIFT   : 0) |
         (metaKey  ? MOD_WIN     : 0);
  
}
#endif

