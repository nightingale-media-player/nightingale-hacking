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
 * \file CWindowMinMaxSubclass.cpp
 * \brief Window subclasser object for WindowMinMax service - Implementation.
 */

#include "WindowMinMaxSubclass.h"
#include "WindowMinMax.h"

#ifdef XP_WIN
#include <dbt.h>
#include <cguid.h>
#include <objbase.h>
#endif

#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsString.h>

#include "sbIDeviceManager.h"
#include "sbICDDevice.h"
#include "sbIUSBMassStorageDevice.h"


// CLASSES ====================================================================
//=============================================================================
// CWindowMinMaxSubclass Class
//=============================================================================

#ifdef XP_WIN
//-----------------------------------------------------------------------------
static LRESULT CALLBACK WindowMinMaxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CWindowMinMaxSubclass *_this = reinterpret_cast<CWindowMinMaxSubclass *>(GetWindowLong(hWnd, GWL_USERDATA));
  return _this->WndProc(hWnd, uMsg, wParam, lParam);
} // WindowMinMaxSubclassProc
#endif

//-----------------------------------------------------------------------------
CWindowMinMaxSubclass::CWindowMinMaxSubclass(nsISupports *window, sbIWindowMinMaxCallback *cb) : m_prevWndProc(NULL)
{
  m_dx = m_ix = m_dy = m_iy = 0;
  m_window = window;
  m_hwnd = NativeWindowFromNode::get(window);
  NS_ADDREF(cb);
  m_callback = cb;
  SubclassWindow();
} // ctor

//-----------------------------------------------------------------------------
CWindowMinMaxSubclass::~CWindowMinMaxSubclass()
{
#ifdef XP_WIN
  UnsubclassWindow();
  NS_RELEASE(m_callback);
#endif
} // dtor

#ifdef XP_WIN
//-----------------------------------------------------------------------------
LRESULT CWindowMinMaxSubclass::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_SETCURSOR:
    {
      // a WM_SETCURSOR message with WM_LBUTTONDOWN is issued each time the mouse is clicked on a resizer.
      // it's also done plenty of other times, but it doesn't matter, this is merely used to reset the compensation
      // variables between two resizing sessions (see comments below)
      if (HIWORD(lParam) == WM_LBUTTONDOWN)
      { 
        m_dx = m_ix = m_dy = m_iy = 0;
      }
      break;
    }
      
    case WM_WINDOWPOSCHANGING:
    {
      WINDOWPOS *wp = (WINDOWPOS*)lParam;
      if (m_callback && wp && !(wp->flags & SWP_NOSIZE))
      {
        // previous window position on screen
        RECT r;
        GetWindowRect(hWnd, &r);
        
        // determine which way the window is being dragged, because mozilla does not honor WM_SIZING/WM_EXITSIZEMOVE
        int _r, _t, _l, _b;
        _r = r.right != wp->x + wp->cx;
        _b = r.bottom != wp->y + wp->cy;
        _l = r.left != wp->x;
        _t = r.top != wp->y;
        
        // if all sides are moving, this is not a user-initiated resize, let moz handle it
        if (_r && _b && _l && _t) break;
        
        // ask the script callback for current min/max values         
        int _minwidth, _minheight, _maxwidth, _maxheight;
        m_callback->GetMaxWidth(&_maxwidth);
        m_callback->GetMaxHeight(&_maxheight);
        m_callback->GetMinWidth(&_minwidth);
        m_callback->GetMinHeight(&_minheight);
        
        // fill default resize values from mozilla's resizer object
        LRESULT ret = CallWindowProc(m_prevWndProc, hWnd, uMsg, wParam, lParam);

        // original size to which mozilla would like to resize the window
        int ocx = wp->cx;
        int ocy = wp->cy;
        
        // because the moz resize object handles resizing using relative mouse tracking, we need to compensate for the motion of the mouse outside of the allowed values. 
        // for instance, when going past the minimum size allowed, the mouse will naturally travel further than the side of the window (the mouse will no longer be in front of 
        // the resizer anymore, but will move over the content of the window). this is normal, it's what we want, but because of the use of relative tracking, moving the mouse back will 
        // immediately start to resize the window in the other direction instead of waiting until the mouse has come back on top of the resizer. that's what you get for using relative
        // tracking... well, that and bugs, too.
        //
        // anyway, to fix this (definitely unwanted) behavior, we keep track of four values, which respectively describe how much the mouse has traveled past the horizontal minimum, 
        // horizontal maximum, vertical minimum and vertical maximum on the left and bottom sides (fotunately, the resizer object does not provoke the same problem with top/left tracking 
        // or we would need 8 of these).
        //
        // using the size of the window mozilla would like to set, and the previous size of the window, we determine which way the mouse has been tracked, and use this to update
        // the four compensation variables. we use their values when the mouse moves the other direction and decrease or increase the size of the window so that it looks like
        // it is never resized at all when the mouse is past the min/max values (ie, it looks like an absolute mouse tracking).
        //
        // this of course has to work in conjunction with the normal tailoring of the window position and size that we're here to implement in the first place.
        //
        // fun.
        
        if (_r && _minwidth != -1) 
        {
          // compensate -- increment left motion past minimum width
          int v = _minwidth-ocx;
          if (v > 0) m_dx += v;
        }

        if (_r && _maxwidth != -1) 
        {
          // compensate -- increment right motion past maximum width
          int v = ocx-_maxwidth;
          if (v > 0) m_ix += v;
        }

        if (_b && _minheight != -1) 
        {
          // compensate -- increment top motion past minimum height
          int v = _minheight-ocy;
          if (v > 0) m_dy += v;
        }

        if (_b && _maxheight != -1) 
        {
          // compensate -- increment bottom motion past maximum height
          int v = ocy-_maxheight;
          if (v > 0) m_iy += v;
        }

        if (_r) // the user is dragging the window by the right side (or one of the corners on the right), we only need to modify the width of the window to constrain it
        {
          if (_minwidth != -1 && wp->cx < _minwidth) wp->cx = _minwidth; 
          else 
          { 
            // compensate -- decrement left motion past minimum width, decrease resulting width so it looks like nothing happened
            int v = ocx-(r.right-r.left);
            if (v > 0)
            {
              if (m_dx >= v) { wp->cx -= v; m_dx -= v; } else { wp->cx -= m_dx; m_dx = 0; }
            }
          }
          if (_maxwidth != -1 && wp->cx > _maxwidth) wp->cx = _maxwidth;
          else
          {
            // compensate -- decrement right motion past maximum width, increase resulting width so it looks like nothing happened
            int v = (r.right-r.left)-ocx;
            if (v > 0)
            {
              if (m_ix >= v) { wp->cx += v; m_ix -= v; } else { wp->cx += m_ix; m_ix = 0; }
            }
          }
        }
        if (_l) // the user is dragging the window by the left side (or one of the corners on the left), we need to modify both the width and the right anchor of the window to constrain it
        {
          if (_maxwidth != -1 && wp->cx > _maxwidth)
          { 
            wp->cx = _maxwidth; 
            wp->x = r.right - _maxwidth; 
          }
          if (_minwidth != -1 && wp->cx < _minwidth) 
          { 
            wp->cx = _minwidth; 
            wp->x = r.right - _minwidth; 
          }
        }
        if (_b) // the user is dragging the window by the bottom side (or one of the corners on the bottom), we only need to modify the height of the window to constrain it
        {
          if (_minheight != -1 && wp->cy < _minheight) wp->cy = _minheight;
          else 
          { 
            // compensate -- decrement upward motion past minimum height, decrease resulting height so it looks like nothing happened
            int v = ocy-(r.bottom-r.top);
            if (v > 0)
            {
              if (m_dy >= v) { wp->cy -= v; m_dy -= v; } else { wp->cy -= m_dy; m_dy = 0; }
            }
          }
          if (_maxheight != -1 && wp->cy > _maxheight) wp->cy = _maxheight;
          else
          {
            // compensate -- decrement downward motion past maximum height, increase resulting height so it looks like nothing happened
            int v = (r.bottom-r.top)-ocy;
            if (v > 0)
            {
              if (m_iy >= v) { wp->cy += v; m_iy -= v; } else { wp->cy += m_iy; m_iy = 0; }
            }
          }
        }
        if (_t) // the user is dragging the window by the top side (or one of the corners on the top), we need to modify both the height and the top anchor of the window to constrain it
        {
          if (_maxheight != -1 && wp->cy > _maxheight)
          { 
            wp->cy = _maxheight; 
            wp->y = r.bottom - _maxheight; 
          }
          if (_minheight != -1 && wp->cy < _minheight) 
          { 
            wp->cy = _minheight; 
            wp->y = r.bottom - _minheight; 
          }
        }

        // all done, return value we got from moz
        return ret;
      }
    }
    break;

    // Added WM_DEVICECHANGE handler for CDDevice object, as a Windows specific
    // implementation, for receiving media insert and removal notifications.
    case WM_DEVICECHANGE:
    {
      nsresult rv;
      nsCOMPtr<sbIDeviceManager> deviceManager =
        do_GetService("@songbirdnest.com/Songbird/DeviceManager;1", &rv);
      if (NS_FAILED(rv))
      {
        NS_WARNING("Failed to get the DeviceManager!");
        break;
      }

      // Send an event to the CDDevice, if present.
      PRBool hasCDDevice;
      NS_NAMED_LITERAL_STRING(cdCategory, "Songbird CD Device");

      rv = deviceManager->HasDeviceForCategory(cdCategory, &hasCDDevice);
      if (NS_SUCCEEDED(rv) && hasCDDevice)
      {
        nsCOMPtr<sbIDeviceBase> baseDevice;
        rv = deviceManager->GetDeviceByCategory(cdCategory,
                                                getter_AddRefs(baseDevice));
        if (NS_SUCCEEDED(rv))
        {
          nsCOMPtr<sbICDDevice> cdDevice = do_QueryInterface(baseDevice, &rv);
          if (NS_SUCCEEDED(rv))
          {
            PRBool retVal;
            PRBool mediaInserted = DBT_DEVICEARRIVAL == wParam;
            rv = cdDevice->OnCDDriveEvent(mediaInserted, &retVal);
          }
        }
      }

      // Send an event to the USBDevice, if present.
      PRBool hasUSBDevice;
      NS_NAMED_LITERAL_STRING(usbCategory, "Songbird USBMassStorage Device");

      rv = deviceManager->HasDeviceForCategory(usbCategory, &hasUSBDevice);
      if (NS_SUCCEEDED(rv) && hasUSBDevice)
      {
        // Bail if we don't care about the message
        if (DBT_DEVICEARRIVAL != wParam &&
            DBT_DEVICEREMOVECOMPLETE != wParam)
          break;

        nsAutoString strDeviceName, strDeviceGUID;
        GUID deviceGUID = GUID_NULL;
        PDEV_BROADCAST_HDR pBroadcastHeader = (PDEV_BROADCAST_HDR) lParam;

        if (pBroadcastHeader->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
          PDEV_BROADCAST_DEVICEINTERFACE pDevBroadcastHeader =
            (PDEV_BROADCAST_DEVICEINTERFACE) pBroadcastHeader;

          strDeviceName.Assign((PRUnichar*)pDevBroadcastHeader->dbcc_name);
          if (strDeviceName.IsEmpty())
            break;

          deviceGUID = pDevBroadcastHeader->dbcc_classguid;
          if (deviceGUID == GUID_NULL)
            break;

          wchar_t wszGUID[64] = {0};
          StringFromGUID2(deviceGUID, wszGUID, 63);
          strDeviceGUID.Assign(wszGUID);

          nsCOMPtr<sbIDeviceBase> baseDevice;
          rv = deviceManager->GetDeviceByCategory(usbCategory,
                                                  getter_AddRefs(baseDevice));
          if (NS_FAILED(rv))
            break;

          nsCOMPtr<sbIUSBMassStorageDevice> usbDevice =
            do_QueryInterface(baseDevice, &rv);
          if (NS_FAILED(rv))
            break;
          
          PRBool retVal;
          PRBool mediaInserted = DBT_DEVICEARRIVAL == wParam;
          rv = usbDevice->OnUSBDeviceEvent(mediaInserted, strDeviceName,
                                           strDeviceGUID, &retVal);
        }
      }
      break;
    }
    // Added WM_SYSCOMMAND handler for taskbar buttons' close command
    case WM_SYSCOMMAND:
    {
      if (wParam == SC_CLOSE && m_callback)
      {
        m_callback->OnWindowClose();
        return 0;
      }
    }
    break;
 }
  return CallWindowProc(m_prevWndProc, hWnd, uMsg, wParam, lParam);
} // WndProc
#endif

//-----------------------------------------------------------------------------
nsISupports *CWindowMinMaxSubclass::getWindow()
{
  return m_window;
} // getWindow

//-----------------------------------------------------------------------------
sbIWindowMinMaxCallback *CWindowMinMaxSubclass::getCallback()
{
  return m_callback;
} // getCallback

//-----------------------------------------------------------------------------
HWND CWindowMinMaxSubclass::getNativeWindow()
{
  return m_hwnd;
} // getNativeWindow

//-----------------------------------------------------------------------------
void CWindowMinMaxSubclass::SubclassWindow()
{
#ifdef XP_WIN
  if (m_prevWndProc != NULL) UnsubclassWindow();
  m_prevWndProc = (WNDPROC)GetWindowLong(m_hwnd, GWL_WNDPROC);
  SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);
  SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)WindowMinMaxSubclassProc);

  {
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    char szMsg[80];

    ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_NULL;

    HDEVNOTIFY hDevNotify = RegisterDeviceNotification( m_hwnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );

    if(!hDevNotify) 
    {
      ::wsprintfA(szMsg, "RegisterDeviceNotification failed: %d\n", ::GetLastError());
      ::MessageBoxA(m_hwnd, szMsg, "Registration", MB_OK);
    }
    else
    {
      m_hDevNotify = hDevNotify;
    }
  }

#endif
}

//-----------------------------------------------------------------------------
void CWindowMinMaxSubclass::UnsubclassWindow()
{
#ifdef XP_WIN
  if (m_prevWndProc == NULL) return;
  SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)m_prevWndProc);
  SetWindowLong(m_hwnd, GWL_USERDATA, 0);
  m_prevWndProc = NULL;

  if(m_hDevNotify != NULL)
  {
    UnregisterDeviceNotification(m_hDevNotify);
    m_hDevNotify = NULL;
  }
#endif
}

