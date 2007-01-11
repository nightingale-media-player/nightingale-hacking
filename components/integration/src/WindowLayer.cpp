/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
#include "nsRect.h"

#if defined(XP_WIN) && !defined(MOZTRANSPARENCY)

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002

#endif

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
#if defined(XP_WIN) && !defined(MOZTRANSPARENCY)
  hUser32 = GetModuleHandle(L"USER32.DLL");
  m_pSetLayeredWindowAttributes = 
                        (lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32, 
                        "SetLayeredWindowAttributes");
#else
  mLayeredWindows.Init();
#endif
} // ctor

//-----------------------------------------------------------------------------
CWindowLayer::~CWindowLayer()
{
#if !defined(XP_WIN) || defined(MOZTRANSPARENCY)
  if (mLayeredWindows.IsInitialized()) mLayeredWindows.Clear();
#endif
} // dtor

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowLayer::SetOpacity(nsISupports *aWindow, PRUint32 value)
{
  NS_ENSURE_ARG_POINTER(aWindow);

#if defined(XP_WIN) && !defined(MOZTRANSPARENCY)
  NATIVEWINDOW wnd = NativeWindowFromNode::get(aWindow);
  if (!wnd) return NS_ERROR_FAILURE;

  if (m_pSetLayeredWindowAttributes) {
    int xs = GetWindowLong(wnd, GWL_EXSTYLE);
    if (value == 255) {
      if (xs & WS_EX_LAYERED) SetWindowLong(wnd, GWL_EXSTYLE, (xs & (~WS_EX_LAYERED)));
    } else {
      if (!(xs & WS_EX_LAYERED)) SetWindowLong(wnd, GWL_EXSTYLE, (xs | WS_EX_LAYERED));
      m_pSetLayeredWindowAttributes(wnd, 0, value, LWA_ALPHA);
    }
  }
#else
  // native moz, can be very slow depending on hardware, window size and platform

  // this doesnt work.
  // this is obviously not the way to do this, but what is the right way ?

  nsresult rv;
  nsIWidget *widget = NativeWindowFromNode::getWidget(aWindow);
  if (!widget) return NS_ERROR_FAILURE;
  PRBool wt;
  rv = widget->GetWindowTranslucency(wt);
  NS_ENSURE_SUCCESS(rv, rv);
    if (value == 255) {
      if (wt) {
        rv = widget->SetWindowTranslucency(PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    } else {
      if (!wt) {
        rv = widget->SetWindowTranslucency(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      nsRect rect;
      rv = widget->GetBounds(rect);
      NS_ENSURE_SUCCESS(rv, rv);
      printf("rect.width = %d rect.height = %d\n alpha = %d\n", rect.width, rect.height, value);
      int size = rect.width * rect.height;

      nsRect wr;
      wr.SetRect(0, 0, rect.width, rect.height);
      PRUint8 *array = new PRUint8[size];
      memset(array, (value & 0xFF), size);
      rv = widget->UpdateTranslucentWindowAlpha(wr, array);
      delete[] array;
    }
    // todo: lookup in hash table and insert if not present
#endif

  return NS_OK;
} // SetOpacity

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowLayer::GetOpacity(nsISupports *aWindow, PRUint32 *retval)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  BYTE alpha = 255;

#if defined(XP_WIN) && !defined(MOZTRANSPARENCY)
  NATIVEWINDOW wnd = NativeWindowFromNode::get(aWindow);
  if (!wnd) return NS_ERROR_FAILURE;

  if (m_pGetLayeredWindowAttributes) {
    m_pGetLayeredWindowAttributes(wnd, NULL, &alpha, NULL);
  }
#else
  // native moz
  nsIWidget *widget = NativeWindowFromNode::getWidget(aWindow);
  if (!widget) return NS_ERROR_FAILURE;
  sbLayerInfo* layerInfo = nsnull;
  mLayeredWindows.Get(widget, &layerInfo);
  if (layerInfo) {
    alpha = layerInfo->alpha;
  }
#endif
  *retval = alpha;
  return NS_OK;
} // GetOpacity

