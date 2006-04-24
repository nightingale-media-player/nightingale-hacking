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
 * \file WindowResizeHook.cpp
 * \brief Service for setting min/max limit callbacks to a window position and size - Implementation.
 */
 
  // Yet another shameful hack from lone
#include "WindowResizeHook.h"

// CLASSES ====================================================================
//=============================================================================
// CWindowResizeHook Class
//=============================================================================

// This implements a callback into js so that script code may dynamically limit 
// the position and size of a window

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowResizeHook, sbIWindowResizeHook)

//-----------------------------------------------------------------------------
CWindowResizeHook::CWindowResizeHook()
{
#ifdef WIN32
  m_hookid = SetWindowsHookEx(WH_CALLWNDPROCRET, ResizeHook, GetModuleHandle(NULL), GetCurrentThreadId());
#endif
} // ctor

//-----------------------------------------------------------------------------
CWindowResizeHook::~CWindowResizeHook()
{
#ifdef WIN32
  UnhookWindowsHookEx(m_hookid);
  m_hookid = 0;
#endif
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowResizeHook::SetCallback(nsISupports *window, sbIWindowResizeHookCallback *cb)
{
  NATIVEWINDOW wnd = NativeWindowFromNode::get(window);
  if (wnd == NULL) return NS_ERROR_FAILURE;
  if (findItemByCallback(cb)) return NS_ERROR_FAILURE;
  NS_ADDREF(cb);
  CWindowResizeHookItem *wrhi = new CWindowResizeHookItem();
  wrhi->m_callback = cb;
  wrhi->m_window = wnd;
  m_items.push_back(wrhi);
  return NS_OK;
} // SetCallback

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowResizeHook::ResetCallback(sbIWindowResizeHookCallback *cb)
{
  CWindowResizeHookItem *wrhi = findItemByCallback(cb);
  if (!wrhi) return NS_ERROR_FAILURE;
  m_items.remove(wrhi);
  delete wrhi;
  NS_RELEASE(cb);
  return NS_OK;
} // ResetCallback

//-----------------------------------------------------------------------------
CWindowResizeHookItem *CWindowResizeHook::findItemByCallback(sbIWindowResizeHookCallback *cb)
{
  std::list<CWindowResizeHookItem*>::iterator iter;
  for (iter = m_items.begin(); iter != m_items.end(); iter++)
  {
    CWindowResizeHookItem *wrhi= *iter;
    if (wrhi->m_callback == cb) return wrhi;
  }
  return NULL;
} // findItemByInterface

//-----------------------------------------------------------------------------
CWindowResizeHookItem *CWindowResizeHook::findItemByWindow(NATIVEWINDOW wnd)
{
  std::list<CWindowResizeHookItem*>::iterator iter;
  for (iter = m_items.begin(); iter != m_items.end(); iter++)
  {
    CWindowResizeHookItem *wrhi= *iter;
    if (wrhi->m_window == wnd) return wrhi;
  }
  return NULL;
} // findItemByInterface

//-----------------------------------------------------------------------------

std::list<CWindowResizeHookItem *> CWindowResizeHook::m_items;
#ifdef WIN32
HHOOK CWindowResizeHook::m_hookid = 0;
#endif

#ifdef WIN32
//-----------------------------------------------------------------------------
LRESULT CALLBACK ResizeHook(int code, WPARAM wParam, LPARAM lParam)
{
  static int inhere = 0;
  if (code >= 0 && inhere == 0)
  {
    inhere = 1;
    CWPRETSTRUCT *rs = (CWPRETSTRUCT *)lParam;
    if (rs->message == WM_WINDOWPOSCHANGED)
    {
      CWindowResizeHookItem *wrhi = CWindowResizeHook::findItemByWindow(rs->hwnd);
      if (wrhi) 
      {
        RECT r;
        GetWindowRect(rs->hwnd, &r);
        wrhi->m_callback->OnResize(r.left, r.top, r.right-r.left, r.bottom-r.top);
      }
    }
    inhere = 0;
  }
  return ::CallNextHookEx(CWindowResizeHook::m_hookid, code, wParam, lParam);
} // ResizeHook
#endif
