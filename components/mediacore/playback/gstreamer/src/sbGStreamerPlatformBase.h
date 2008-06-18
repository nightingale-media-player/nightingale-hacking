/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef _SB_GSTREAMER_PLATFORM_BASE_H_
#define _SB_GSTREAMER_PLATFORM_BASE_H_

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "sbIGstPlatformInterface.h"

class BasePlatformInterface : public sbIGstPlatformInterface
{
public:
  BasePlatformInterface ();
  virtual ~BasePlatformInterface ();

  // Implementation of (some parts of) the sbIGstPlatformInterface interface
  //
  void Resize (int x, int y, int width, int height);
  bool GetFullscreen ();
  void SetFullscreen (bool fullscreen);
  void SetDisplayAspectRatio (int numerator, int denominator);
  void PrepareVideoWindow();

protected:
  // Set the display area available for rendering the video to
  void SetDisplayArea (int x, int y, int width, int height);
  // Resize the video to an appropriate (aspect-ratio preserving) subrectangle
  // of the currently set display area.
  void ResizeVideo ();

  // Actually render the video window in this precise area, which has been
  // aspect-ratio corrected.
  virtual void MoveVideoWindow (int x, int y, int width, int height) = 0;

  // Set the window ID (as appropriate to the particular platform) on this
  // XOverlay object.
  virtual void SetXOverlayWindowID (GstXOverlay *xoverlay) = 0;

  // Switch from windowed mode to full screen mode.
  virtual void FullScreen () = 0;

  // Set to windowed mode from full screen mode.
  virtual void UnFullScreen () = 0;

  int mDisplayWidth;  // Width of current display area
  int mDisplayHeight; // Height of current display area
  int mDisplayX;      // X coordinate of current display area
  int mDisplayY;      // Y coordinate of current display area

  int mDARNum;        // Current Display Aspect Ratio as a fraction
  int mDARDenom;      // (numerator and denominator)

  bool mFullscreen;   // Are we currently in fullscreen mode?

  GstElement *mVideoSink;        // The GStreamer video sink we created
  GstElement *mAudioSink;        // The GStreamer audio sink we created
};

#endif // _SB_GSTREAMER_PLATFORM_BASE_H_
