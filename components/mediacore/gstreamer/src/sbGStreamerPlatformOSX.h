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

#ifndef _SB_GSTREAMER_PLATFORM_OSX_H_
#define _SB_GSTREAMER_PLATFORM_OSX_H_

#include <gst/gst.h>

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

  virtual void PrepareVideoWindow(GstMessage *aMessage);

  // I'm a bad person
  void AddRef() {}
  void Release() {}

protected:
  // Implement virtual methods in BasePlatformInterface
  void MoveVideoWindow (int x, int y, int width, int height);
  void SetXOverlayWindowID (GstXOverlay *aXOverlay) {}
  void FullScreen() {}
  void UnFullScreen() {}

  nsresult SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget);

  void RemoveView();

  void *mParentView;
  void *mVideoView;
};

#endif // _SB_GSTREAMER_PLATFORM_OSX_H_
