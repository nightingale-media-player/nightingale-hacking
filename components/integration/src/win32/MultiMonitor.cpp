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
 * \file MultiMonitor.cpp
 * \brief Songbird Multiple Montir Support - Implementation.
 */

#ifdef XP_WIN

#include <windows.h>
#include <memory.h>

#include <shlobj.h>
#include <multimon.h>
#if !defined(HMONITOR_DECLARED) && (WINVER < 0x500)
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif

#endif

#include "MultiMonitor.h"

// CLASSES ====================================================================

//=============================================================================
// CMultipleMonitor Class
//=============================================================================

//-----------------------------------------------------------------------------
/* Implementation file */

void CMultiMonitor::GetMonitorFromPoint(RECT *r, POINT *pt, bool excludeTaskbar) {
#ifdef XP_WIN  
  if (pt != NULL) 
  {
    // Load dll dynamically so as to be compatible with older versions of Win32
    HINSTANCE user32 = LoadLibrary(L"USER32.DLL");
    if (user32 != NULL) 
    {
      HMONITOR (WINAPI *_MonitorFromPoint)(POINT pt, DWORD dwFlags) = (HMONITOR (WINAPI *)(POINT, DWORD)) GetProcAddress(user32, "MonitorFromPoint");
      BOOL (WINAPI *_GetMonitorInfoW)(HMONITOR mon, LPMONITORINFO lpmi) = (BOOL (WINAPI *)(HMONITOR, LPMONITORINFO)) GetProcAddress(user32, "GetMonitorInfoW");
      if (_MonitorFromPoint && _GetMonitorInfoW) 
      {
        HMONITOR hmon;
        hmon = _MonitorFromPoint(*pt, MONITOR_DEFAULTTONULL);
        if (hmon != NULL) 
        {
          MONITORINFOEX mie;
          memset(&mie, sizeof(mie), 0);
          mie.cbSize = sizeof(mie);
          if (_GetMonitorInfoW(hmon, &mie))
          {
            if (excludeTaskbar) 
              *r = mie.rcWork;
            else 
              *r = mie.rcMonitor;
            FreeLibrary(user32);
            return;
          }
        }
      }
      FreeLibrary(user32);
    }
  }
  SystemParametersInfo(SPI_GETWORKAREA, 0, r, 0);
#endif
} // GetMonitorFromPoint
