/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
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

#ifndef _SB_GSTREAMER_PLATFORM_OSX_H_
#define _SB_GSTREAMER_PLATFORM_OSX_H_

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "sbIGstPlatformInterface.h"
#include "sbGStreamerPlatformBase.h"

#include <nsIBoxObject.h>

class OSXPlatformInterface : public BasePlatformInterface
{
public:
  OSXPlatformInterface (sbGStreamerMediacore *aCore);
  virtual ~OSXPlatformInterface();

  // Implementation of the rest of sbIGstPlatformInterface interface
  GstElement * SetVideoSink (GstElement *aVideoSink);
  GstElement * SetAudioSink (GstElement *aAudioSink);

  // Dummy override for ResizeToWindow in base class - NS_NEW_RUNNABLE_METHOD
  // doesn't like being passed methods not defined on current class (bug 18744)
  virtual void ResizeToWindow() {return BasePlatformInterface::ResizeToWindow();}
  virtual void PrepareVideoWindow(GstMessage *aMessage);

  // Get the main view view
  void* GetVideoView();

  // Event callback member functions
  void OnMouseMoved(void *aCocoaEvent);
  void OnWindowResized();

protected:
  // Implement virtual methods in BasePlatformInterface
  void MoveVideoWindow (int x, int y, int width, int height);
  void SetVideoOverlayWindowID(GstVideoOverlay *aVideoOverlay) {}
  void FullScreen() {}
  void UnFullScreen() {}

  nsresult SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget);

  void RemoveView();

  // ObjC member variables stored as void pointers for build simplification:
  void *mParentView;         // weak (NSView *)
  void *mVideoView;          // weak (NSView *)
  void *mGstGLViewDelegate;  // strong (NSObject *)
};

#endif // _SB_GSTREAMER_PLATFORM_OSX_H_
