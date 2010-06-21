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
  : mhDWMAPI(NULL),
    mDwmIsCompositionEnabled(NULL)
{
  NS_ASSERTION(!gSubclassId,
               "sbWindowChromeService constructed twice!");
  if (!gSubclassId) {
    gSubclassId = (UINT_PTR)this;
    mhDWMAPI = ::LoadLibrary(TEXT("dwmapi"));
    if (mhDWMAPI) {
      mDwmIsCompositionEnabled =
        (t_DwmIsCompositionEnabled)::GetProcAddress(mhDWMAPI,
                                                    "DwmIsCompositionEnabled");
    }
  }
}

sbWindowChromeService::~sbWindowChromeService()
{
  if (gSubclassId == (UINT_PTR)this) {
    gSubclassId = NULL;
  }
  if (mhDWMAPI) {
    ::FreeLibrary(mhDWMAPI);
  }
}

/* static */ bool
sbWindowChromeService::IsCompositionEnabled(const sbWindowChromeService* self)
{
  if (!self || !self->mDwmIsCompositionEnabled) {
    // DWM API isn't available
    return false;
  }
  BOOL isDWMCompositionEnabled = FALSE;
  HRESULT hr = self->mDwmIsCompositionEnabled(&isDWMCompositionEnabled);
  if (FAILED(hr)) {
    return false;
  }

  return isDWMCompositionEnabled != FALSE;
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

/* readonly attribute boolean isCompositionEnabled; */
NS_IMETHODIMP
sbWindowChromeService::GetIsCompositionEnabled(PRBool *aIsCompositionEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsCompositionEnabled);
  *aIsCompositionEnabled = IsCompositionEnabled(this);
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
    NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
    bool willMinimize = (params->rgrc[0].left == -32000) &&
                        (params->rgrc[0].top  == -32000);
    if (willMinimize) {
      // we are going to minimize; don't touch anything
      break;
    }
    if (IsZoomed(hWnd)) {
      // On Windows 7, Windows will adjust the size of the maximized windows
      // so that they're larger than the screen to account for the window
      // border (even with Aero disabled).  On XP, however, it does _not_ do
      // that.  So, figure out how big the window wants to be, and how big the
      // screen is, and go for the closest solution.
      RECT* rectWindow = &params->rgrc[0]; // shorthand
      RECT rectMon = {0};
      int xSize = ::GetSystemMetrics(SM_CXSIZEFRAME);
      int ySize = ::GetSystemMetrics(SM_CYSIZEFRAME);
      // Bug 21344 - HMONITOR's are not handles
      HMONITOR hMon = ::MonitorFromRect(rectWindow, MONITOR_DEFAULTTONULL);
      if (hMon) {
        MONITORINFO monInfo = {0};
        monInfo.cbSize = sizeof(monInfo);
        BOOL success = ::GetMonitorInfo(hMon, &monInfo);
        if (success) {
          ::CopyRect(&rectMon, &monInfo.rcWork);
        }
        else {
          ::CopyRect(&rectMon, rectWindow);
        }
      }
      else {
        // no monitor!?
        ::CopyRect(&rectMon, rectWindow);
      }

      const LONG TOLERANCE = 2;
      if (rectMon.top > rectWindow->top &&
          rectMon.top - rectWindow->top < ySize + TOLERANCE)
      {
        rectWindow->top += ySize;
      }
      if (rectWindow->bottom > rectMon.bottom &&
          rectWindow->bottom - rectMon.bottom < ySize + TOLERANCE)
      {
        rectWindow->bottom -= ySize;
      }
      if (rectMon.left > rectWindow->left &&
          rectMon.left - rectWindow->left < xSize + TOLERANCE)
      {
        rectWindow->left += xSize;
      }
      if (rectWindow->right > rectMon.right &&
          rectWindow->right - rectMon.right < xSize + TOLERANCE)
      {
        rectWindow->right -= xSize;
      }

      // Chop off one pixel on any edge with auto-hidden taskbars (deskbars,
      // really).  This is required to make sure those taskbars will remain
      // functional (i.e. pop up) when the mouse goes near them - otherwise
      // Windows assumes we really meant to be a full screen window instead.
      APPBARDATA appbarData = {sizeof(APPBARDATA)};
      appbarData.hWnd = hWnd;
      HWND wnd;
      // Bug 21344 - HMONITOR's are not handles
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
  case WM_NCACTIVATE:
  {
    if (IsCompositionEnabled((sbWindowChromeService*)uIdSubclass)) {
      // When DWM composition is enabled, call the default handler so that
      // it draws the window shadow.  In this case the window is also double
      // buffered (by the DWM), so we don't have to worry about flickering.
      
      // We skip doing this if we're maximized (or look like we are, since that
      // is how Mozilla implemented full screen) to avoid problems with the
      // video window showing the (win9x) non-client area.  We don't need to
      // have shadows in that case anyway.
      RECT rectMon = {0}, rectWindow;
      BOOL success = ::GetWindowRect(hWnd, &rectWindow);
      if (!success) {
        ::SetRect(&rectWindow, -1, -1, -1, -1);
      }
      // Bug 21344 - HMONITOR's are not handles
      HMONITOR hMon = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
      if (hMon) {
        MONITORINFO monInfo = {0};
        monInfo.cbSize = sizeof(monInfo);
        success = ::GetMonitorInfo(hMon, &monInfo);
        if (success) {
          ::CopyRect(&rectMon, &monInfo.rcMonitor);
        }
      }
      if (!::EqualRect(&rectMon, &rectWindow)) {
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
      }
    }
    // No DWM, don't do anything to avoid extra paints of the non-client area
    // which causes bad flickering.
    return TRUE;
  }
  case WM_NCPAINT:
  {
    // We need to call the default implementation in order to get window shadow.
    // Since that only works when DWM is enabled anyway, check if it is - if it
    // is not, do not call the default implementation since that causes flicker
    // on systems with theming but not DWM (e.g. XP, or Vista with 16bpp).
    if (!IsCompositionEnabled((sbWindowChromeService*)uIdSubclass)) {
      // no need to do anything; however, invalidate the window anyway in case
      // this paint was caused by invalidation.
      InvalidateRgn(hWnd, NULL, FALSE);
      return 0;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
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
