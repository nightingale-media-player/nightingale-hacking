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
 * \file WindowCloak.cpp
 * \brief Songbird Window Cloaker Object Implementation.
 */
 
  // ... <sigh>

#include "WindowCloak.h"

// CLASSES ====================================================================
//=============================================================================
// CWindowCloak Class
//=============================================================================

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowCloak, sbIWindowCloak)

//-----------------------------------------------------------------------------
CWindowCloak::CWindowCloak()
{
} // ctor

//-----------------------------------------------------------------------------
CWindowCloak::~CWindowCloak()
{
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowCloak::Cloak( nsISupports *window )
{
  NATIVEWINDOW wnd = NativeWindowFromNode::get(window);
  if (!wnd) return NS_OK;  // Fail silent
  if (findItem(wnd)) return NS_OK; // Fail silently.  Multiple cloaks are ok, no refcount.
  
  NATIVEWINDOW oldparent = NULL;
  
#ifdef XP_WIN
  // Detach window from group
  oldparent = SetParent(wnd, NULL);
  // Hide it
  ShowWindow(wnd, SW_HIDE);
#endif

  // Remember it
  WindowCloakEntry *wce = new WindowCloakEntry();
  wce->m_hwnd = wnd;
  wce->m_oldparent = oldparent;
  m_items.push_back(wce);

  return NS_OK;
} // Cloak

std::list<WindowCloakEntry *> CWindowCloak::m_items;

//-----------------------------------------------------------------------------
WindowCloakEntry *CWindowCloak::findItem(NATIVEWINDOW wnd)
{
  std::list<WindowCloakEntry*>::iterator iter;
  for (iter = m_items.begin(); iter != m_items.end(); iter++)
  {
    WindowCloakEntry *wce = *iter;
    if (wce->m_hwnd == wnd) return wce;
  }
  return NULL;
} // findItem

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowCloak::Uncloak(nsISupports *window )
{
  NATIVEWINDOW wnd = NativeWindowFromNode::get(window);
  if (!wnd) return NS_OK; // Fail silent

  WindowCloakEntry *wce = findItem(wnd);
  if (!wce) return NS_OK; // Fail silent
  
#ifdef XP_WIN
  // restore window
  SetParent(wce->m_hwnd, wce->m_oldparent);
  ShowWindow(wce->m_hwnd, SW_SHOW);
#endif
  
  m_items.remove(wce);
  delete wce;
  return NS_OK;
} // Uncloak
