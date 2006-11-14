/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
 * \file WindowLayer.cpp
 * \brief Service for setting the opacity of a window - Implementation.
 */
 
  // Yet another shameful hack from lone
#include "WindowLayer.h"

#ifdef XP_WIN

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002
#endif // ndef WS_EX_LAYERED

typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(HWND hWnd, 
                                  COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

lpfnSetLayeredWindowAttributes m_pSetLayeredWindowAttributes;

typedef BOOL (WINAPI *lpfnGetLayeredWindowAttributes)(HWND hWnd, 
                                  COLORREF *crKey, BYTE *bAlpha, DWORD *dwFlags);

lpfnGetLayeredWindowAttributes m_pGetLayeredWindowAttributes;

#endif

// CLASSES ====================================================================
//=============================================================================
// CWindowLayer Class
//=============================================================================

// This implements a callback into js so that script code may dynamically limit 
// the position and size of a window

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowLayer, sbIWindowLayer)

//-----------------------------------------------------------------------------
CWindowLayer::CWindowLayer()
{
#ifdef XP_WIN
  hUser32 = GetModuleHandle(L"USER32.DLL");
  m_pSetLayeredWindowAttributes = 
                        (lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32, 
                        "SetLayeredWindowAttributes");
#endif
} // ctor

//-----------------------------------------------------------------------------
CWindowLayer::~CWindowLayer()
{
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowLayer::SetOpacity(nsISupports *aWindow, PRUint32 value)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  NATIVEWINDOW wnd = NativeWindowFromNode::get(aWindow);

#ifdef XP_WIN
  if (m_pSetLayeredWindowAttributes) {
    int xs = GetWindowLong(wnd, GWL_EXSTYLE);
    if (value == 255) {
      if (xs & WS_EX_LAYERED) SetWindowLong(wnd, GWL_EXSTYLE, (xs & (~WS_EX_LAYERED)));
    } else {
      if (!(xs & WS_EX_LAYERED)) SetWindowLong(wnd, GWL_EXSTYLE, (xs | WS_EX_LAYERED));
      m_pSetLayeredWindowAttributes(wnd, 0, value, LWA_ALPHA);
    }
  }
#endif

  return NS_OK;
} // SetCallback

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowLayer::GetOpacity(nsISupports *aWindow, PRUint32 *retval)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  NATIVEWINDOW wnd = NativeWindowFromNode::get(aWindow);
  BYTE alpha = 255;

#ifdef XP_WIN
  if (m_pGetLayeredWindowAttributes) {
    m_pGetLayeredWindowAttributes(wnd, NULL, &alpha, NULL);
  }
#endif
  *retval = alpha;
  return NS_OK;
} // ResetCallback

