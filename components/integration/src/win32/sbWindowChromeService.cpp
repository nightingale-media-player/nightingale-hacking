/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbWindowChromeService.h"

#include <commctrl.h>
#include <shellapi.h>

#include "../NativeWindowFromNode.h"

typedef HRESULT (WINAPI *t_DwmIsCompositionEnabled)(BOOL *);

static UINT_PTR gSubclassId = NULL;

NS_IMPL_ISUPPORTS1(sbWindowChromeService, sbIWindowChromeService);

sbWindowChromeService::sbWindowChromeService()
{
  if (!gSubclassId) {
    gSubclassId = (UINT_PTR)this;
  }
}

sbWindowChromeService::~sbWindowChromeService()
{
  if (gSubclassId == (UINT_PTR)this) {
    gSubclassId = NULL;
  }
}

/* void hideChrome (in nsISupports aWindow, in boolean aHide); */
NS_IMETHODIMP sbWindowChromeService::HideChrome(nsISupports *aWindow,
                                                PRBool aHide)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_STATE(gSubclassId == (UINT_PTR)this);

  HWND hWnd = NativeWindowFromNode::get(aWindow);
  NS_ENSURE_TRUE(hWnd, NS_ERROR_INVALID_ARG);

  if (aHide) {
    LONG style = ::GetWindowLong(hWnd, GWL_STYLE);
    BOOL success = ::SetWindowSubclass(hWnd,
                                       &sbWindowChromeService::WndProc,
                                       gSubclassId,
                                       style);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    // We need to set WS_CAPTION, to make Windows think that we are a real
    // window (which leads to things like auto-hiding taskbars working properly
    // and having a taskbar button).
    style |= WS_CAPTION;
    ::SetWindowLong(hWnd, GWL_STYLE, style);
  }
  else {
    DWORD_PTR oldStyle;
    BOOL wasSubclassed = ::GetWindowSubclass(hWnd,
                                             &sbWindowChromeService::WndProc,
                                             gSubclassId,
                                             &oldStyle);
    if (wasSubclassed) {
      ::SetWindowLong(hWnd, GWL_STYLE, oldStyle);
      BOOL success = ::RemoveWindowSubclass(hWnd,
                                            &sbWindowChromeService::WndProc,
                                            gSubclassId);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    }
  }
  return NS_OK;
}

/* static */
LRESULT
sbWindowChromeService::WndProc(HWND hWnd,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam,
                               UINT_PTR uIdSubclass,
                               DWORD_PTR dwRefData)
{
  if (uIdSubclass != gSubclassId) {
    // We're majorly screwed up somehow; do nothing
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
  }
  switch (uMsg) {
  case WM_NCDESTROY:
  {
    // clean up
    RemoveWindowSubclass(hWnd, &sbWindowChromeService::WndProc, uIdSubclass);
    break;
  }
  case WM_NCCALCSIZE:
  {
    if (!(BOOL)wParam) {
      // we do not need to do anything special for this case
      break;
    }
    if (IsZoomed(hWnd)) {
      // For maximized windows, adjust by the size of the borders as well,
      // because Windows actually makes the window bigger than the screen to
      // accoommodate for window borders.
      NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
      int xSize = ::GetSystemMetrics(SM_CXSIZEFRAME);
      int ySize = ::GetSystemMetrics(SM_CYSIZEFRAME);
      params->rgrc[0].top += ySize;
      params->rgrc[0].bottom -= ySize;
      params->rgrc[0].left += xSize;
      params->rgrc[0].right -= xSize;

      // Chop off one pixel on any edge with auto-hidden taskbars (deskbars,
      // really).  This is required to make sure those taskbars will remain
      // functional (i.e. pop up) when the mouse goes near them - otherwise
      // Windows assumes we really meant to be a full screen window instead.
      APPBARDATA appbarData = {sizeof(APPBARDATA)};
      appbarData.hWnd = hWnd;
      HWND wnd;
      HMONITOR selfMon = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),
               targetMon;

      appbarData.uEdge = ABE_TOP;
      wnd = (HWND)::SHAppBarMessage(ABM_GETAUTOHIDEBAR, &appbarData);
      if (wnd) {
        targetMon = ::MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
        if (targetMon == selfMon) {
          // instead of bumping the window down, subtract a pixel from the
          // bottom instead - otherwise we end up exposing a pixel of the
          // top which 1) can lead to clicking on nearly-invisible widgets (e.g.
          // the close button), 2) loses fitt's law gains of having things at top
          params->rgrc[0].bottom -= 1;
        }
      }
      appbarData.uEdge = ABE_RIGHT;
      wnd = (HWND)::SHAppBarMessage(ABM_GETAUTOHIDEBAR, &appbarData);
      if (wnd) {
        targetMon = ::MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
        if (targetMon == selfMon) {
          params->rgrc[0].right -= 1;
        }
      }
      appbarData.uEdge = ABE_BOTTOM;
      wnd = (HWND)::SHAppBarMessage(ABM_GETAUTOHIDEBAR, &appbarData);
      if (wnd) {
        targetMon = ::MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
        if (targetMon == selfMon) {
          params->rgrc[0].bottom -= 1;
        }
      }
      appbarData.uEdge = ABE_LEFT;
      wnd = (HWND)::SHAppBarMessage(ABM_GETAUTOHIDEBAR, &appbarData);
      if (wnd) {
        targetMon = ::MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
        if (targetMon == selfMon) {
          params->rgrc[0].left += 1;
        }
      }
      return 0;
    }
    // we have a window we want to hook, but it's not maximized; leave the
    // rectangles as-is, meaning that there should be no non-client area (i.e.
    // no chrome at all).
    return 0;
  }
  // for these messages, we want to call the default window proc then invalidate
  // the non-client areas
  case WM_NCACTIVATE:
  case WM_SETTEXT:
  case WM_ACTIVATE:
  {
    // unset enough window styles to turn off rendering of the non-client area
    // while we call the default implementations.  The usual solution found on
    // the internet seems to call for unsetting WS_VISIBLE instead, but it
    // seems to cause repainting problems in other windows when we deactivate.
    LONG style = ::GetWindowLong(hWnd, GWL_STYLE);
    ::SetWindowLong(hWnd,
                    GWL_STYLE,
                    (style & ~(WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_THICKFRAME)));
    LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
    // undo the window styles hack
    ::SetWindowLong(hWnd, GWL_STYLE, style);
    return result;
  }
  case WM_NCPAINT:
  {
    // We need to call the default implementation in order to get window shadow.
    // Since that only works when DWM is enabled anyway, check if it is - if it
    // is not, do not call the default implementation since that causes flicker
    // on systems with theming but not DWM (e.g. XP, or Vista with 16bpp).
    HMODULE hDWMAPI = ::LoadLibrary(TEXT("dwmapi"));
    if (!hDWMAPI) {
      // no DWMAPI; no need to do any NCPAINT
      return 0;
    }
    t_DwmIsCompositionEnabled pDwmIsCompositionEnabled =
      (t_DwmIsCompositionEnabled)::GetProcAddress(hDWMAPI,
                                                  "DwmIsCompositionEnabled");
    if (!pDwmIsCompositionEnabled) {
      ::FreeLibrary(hDWMAPI);
      return 0;
    }
    BOOL isDWMCompositionEnabled = FALSE;
    HRESULT hr = pDwmIsCompositionEnabled(&isDWMCompositionEnabled);
    if (FAILED(hr)) {
      isDWMCompositionEnabled = FALSE;
    }
    ::FreeLibrary(hDWMAPI);

    if (!isDWMCompositionEnabled) {
      // DWM composition isn't on; don't do NCPAINT
      return 0;
    }

    // Try to convince it that it doesn't really need to draw the window.
    // This usually ends up not working very well - but that seems to be the
    // best we can do for now.
    HRGN region = (HRGN)wParam;
    RECT rect;
    ::GetWindowRect(hWnd, &rect);
    HRGN rectRgn = CreateRectRgnIndirect(&rect);
    CombineRgn(region, region, rectRgn, RGN_DIFF);
    DeleteObject(rectRgn);
    LRESULT result = DefSubclassProc(hWnd, uMsg, (WPARAM)region, lParam);
    InvalidateRgn(hWnd, NULL, FALSE);
    return result;
  }
  case 0xAE: /* undocumented: WM_NCUAHDRAWCAPTION */
  case 0xAF: /* undocumented: WM_NCUAHDRAWFRAME */
    // these messages, according to chromium.org, are sent by Windows to redraw
    // the caption / frame at unpredictable times.  Intercept these and don't
    // use the default behaviour.
    return 0;
  }
  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
