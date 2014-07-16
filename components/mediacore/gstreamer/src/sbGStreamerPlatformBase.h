/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2008 POTI, Inc.
 * http://songbirdnest.com
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#ifndef _SB_GSTREAMER_PLATFORM_BASE_H_
#define _SB_GSTREAMER_PLATFORM_BASE_H_

#include <nsCOMPtr.h>
#include <nsIBoxObject.h>
#include <nsIWidget.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "sbIGstPlatformInterface.h"

class sbGStreamerMediacore;

class BasePlatformInterface : public sbIGstPlatformInterface
{
public:
  BasePlatformInterface (sbGStreamerMediacore *aCore);
  virtual ~BasePlatformInterface ();

  // Implementation of (some parts of) the sbIGstPlatformInterface interface
  //
  virtual void ResizeToWindow();
  bool GetFullscreen();
  void SetFullscreen(bool aFullscreen);
  void SetDisplayAspectRatio(int aNumerator, int aDenominator);
  virtual void PrepareVideoWindow(GstMessage *aMessage);

protected:
  virtual nsresult SetVideoBox(nsIBoxObject *aVideoBox, nsIWidget *aWidget);

  // Set the display area available for rendering the video to
  void SetDisplayArea(int x, int y, int width, int height);
  // Resize the video to an appropriate (aspect-ratio preserving) subrectangle
  // of the currently set display area.
  void ResizeVideo();

  // Set the owning DOM document
  virtual nsresult SetDocument(nsIDOMDocument *aDocument);

  // Create a DOM mouse event
  nsresult CreateDOMMouseEvent(nsIDOMMouseEvent **aMouseEvent);
  // Create a DOM key event
  nsresult CreateDOMKeyEvent(nsIDOMKeyEvent **aKeyEvent);
  // Dispatch a DOM event
  nsresult DispatchDOMEvent(nsIDOMEvent *aEvent);

  // Actually render the video window in this precise area, which has been
  // aspect-ratio corrected.
  virtual void MoveVideoWindow(int x, int y, int width, int height) = 0;

  // Set the window ID (as appropriate to the particular platform) on this
  // VideoOverlay object.
  virtual void SetVideoOverlayWindowID(GstVideoOverlay *aVideoOverlay) {}

  // Switch from windowed mode to full screen mode.
  virtual void FullScreen() = 0;

  // Set to windowed mode from full screen mode.
  virtual void UnFullScreen() = 0;

  int mDisplayWidth;  // Width of current display area
  int mDisplayHeight; // Height of current display area
  int mDisplayX;      // X coordinate of current display area
  int mDisplayY;      // Y coordinate of current display area

  int mDARNum;        // Current Display Aspect Ratio as a fraction
  int mDARDenom;      // (numerator and denominator)

  bool mFullscreen;   // Are we currently in fullscreen mode?

  // The box object for the video display area, access to this object
  // is proxied to the main thread.
  nsCOMPtr<nsIBoxObject> mVideoBox; 
  // The widget corresponding to this box object, also main-thread only.
  nsCOMPtr<nsIWidget> mWidget;
  // The owning document of the box object.
  nsCOMPtr<nsIDOMDocument> mDocument;

  GstElement *mVideoSink;        // The GStreamer video sink we created
  GstElement *mAudioSink;        // The GStreamer audio sink we created

  sbGStreamerMediacore *mCore;   // The core associated with this object.
};

#endif // _SB_GSTREAMER_PLATFORM_BASE_H_
