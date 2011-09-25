/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef _SB_GSTREAMER_PLATFORM_WIN32_H_
#define _SB_GSTREAMER_PLATFORM_WIN32_H_

#include <windows.h>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "sbIGstPlatformInterface.h"
#include "sbGStreamerPlatformBase.h"

class Win32PlatformInterface : public BasePlatformInterface
{
public:
  Win32PlatformInterface (sbGStreamerMediacore *aCore);
  
  virtual ~Win32PlatformInterface ();

  // Implement the rest of sbIGstPlatformInterface
  GstElement * SetVideoSink(GstElement *aVideoSink);
  GstElement * SetAudioSink(GstElement *aAudioSink);

protected:
  // Implement virtual methods in BasePlatformInterface
  void MoveVideoWindow(int x, int y, int width, int height);
  void SetXOverlayWindowID(GstXOverlay *aXOverlay);
  void FullScreen();
  void UnFullScreen();

  nsresult SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget);

private:
  HWND mWindow;           // The video window we're rendering into
  HWND mFullscreenWindow; // The fullscreen window we're parented to (when 
                          // in fullscreen mode)
  HWND mParentWindow;     // Our parent window in windowed mode.
  
  PRBool mCursorShowing;  // Are we currently showing the cursor? True = Yes
  
  PRInt32 mLastMouseX;
  PRInt32 mLastMouseY;

  static LRESULT APIENTRY VideoWindowProc(HWND hWnd, UINT message, 
          WPARAM wParam, LPARAM lParam);

  HWND SelectParentWindow(HWND hWnd);
  PRBool HasMouseMoved(PRInt32 aX, PRInt32 aY);
};

#endif // _SB_GSTREAMER_PLATFORM_WIN32_H_
