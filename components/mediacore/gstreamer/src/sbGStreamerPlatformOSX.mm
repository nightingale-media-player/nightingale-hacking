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

#include "sbGStreamerPlatformOSX.h"

#include <prlog.h>
#include <nsDebug.h>

#include <nsCOMPtr.h>
#include <nsIRunnable.h>
#include <nsThreadUtils.h>

#import <AppKit/AppKit.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerPlatformOSX:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerPlatformOSX =
  PR_NewLogModule("sbGStreamerPlatformOSX");

#define LOG(args)                                         \
  if (gGStreamerPlatformOSX)                             \
    PR_LOG(gGStreamerPlatformOSX, PR_LOG_WARNING, args)

#define TRACE(args)                                      \
  if (gGStreamerPlatformOSX)                            \
    PR_LOG(gGStreamerPlatformOSX, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

OSXPlatformInterface::OSXPlatformInterface(sbGStreamerMediacore *aCore) :
    BasePlatformInterface(aCore),
    mParentView(NULL),
    mVideoView(NULL)
{
}

OSXPlatformInterface::~OSXPlatformInterface()
{
  RemoveView();
}

GstElement *
OSXPlatformInterface::SetVideoSink(GstElement *aVideoSink)
{
  if (mVideoSink) {
    gst_object_unref(mVideoSink);
    mVideoSink = NULL;
  }

  mVideoSink = aVideoSink;

  if (!mVideoSink) {
    mVideoSink = gst_element_factory_make("osxvideosink", "video-sink");
    if (mVideoSink) {
      g_object_set (mVideoSink, "embed", TRUE, NULL);
    }
  }

  if (!mVideoSink) {
    // Then hopefully autovideosink will pick something appropriate...
    mVideoSink = gst_element_factory_make("autovideosink", "video-sink");
  }

  // Keep a reference to it.
  if (mVideoSink) 
      gst_object_ref(mVideoSink);

  return mVideoSink;
}

GstElement *
OSXPlatformInterface::SetAudioSink(GstElement *aAudioSink)
{
  if (mAudioSink) {
    gst_object_unref(mAudioSink);
    mAudioSink = NULL;
  }

  mAudioSink = aAudioSink;

  if (!mAudioSink)
    mAudioSink = gst_element_factory_make("osxaudiosink", "audio-sink");
  if (!mAudioSink) {
    // Then hopefully autoaudiosink will pick something appropriate...
    mAudioSink = gst_element_factory_make("autoaudiosink", "audio-sink");
  }

  // Keep a reference to it.
  if (mAudioSink) 
      gst_object_ref(mAudioSink);

  return mAudioSink;
}

nsresult
OSXPlatformInterface::SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget)
{
  // First let the superclass do its thing.
  nsresult rv = BasePlatformInterface::SetVideoBox (aBoxObject, aWidget);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mParentView = (void *)(aWidget->GetNativeData(NS_NATIVE_WIDGET));
  NS_ENSURE_TRUE(mParentView != NULL, NS_ERROR_FAILURE);

  return NS_OK;
}

void
OSXPlatformInterface::PrepareVideoWindow(GstMessage *aMessage)
{
  // Firstly, if we don't already have a video view set up, request a video
  // window, and set up the appropriate parent view.
  if (!mParentView) {
    nsCOMPtr<nsIThread> mainThread;
    rv = NS_GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<nsIRunnable> runnable = 
        NS_NEW_RUNNABLE_METHOD (sbGStreamerMediacore,
                                mCore,
                                RequestVideoWindow);

    rv = mainThread->Dispatch(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  // Now we can deal with setting this up...

  /* This message has an 'nsview' element containing a pointer to
     the NSView that the video is drawn into. Grab the NSView */
  NSView *view;
  const GValue *value = gst_structure_get_value (
          aMessage->structure, "nsview");

  if (!value || !G_VALUE_HOLDS_POINTER(value))
    return;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  RemoveView();

  NSView *parentView = (NSView *)mParentView;
  mVideoView = g_value_get_pointer (value);
  view = (NSView *)mVideoView;

  // Now, we want to set this view as a subview of the NSView we have
  // as our window-for-displaying-video. Don't do this from a non-main
  // thread, though!
  [parentView performSelectorOnMainThread:@selector(addSubview:) withObject:view waitUntilDone:NO];

  nsCOMPtr<nsIRunnable> runnable = NS_NEW_RUNNABLE_METHOD(OSXPlatformInterface, this, ResizeToWindow);
  NS_DispatchToMainThread(runnable);

  [pool release];
}

void
OSXPlatformInterface::MoveVideoWindow (int x, int y, int width, int height)
{
  NSView *view = (NSView *)mVideoView;

  if (view) {
    NSRect rect;
    // Remap to OSX's coordinate system, which is from the bottom left.
    rect.origin.y = [[view superview] frame].size.height - y - height;

    rect.origin.x = x;
    rect.size.width = width;
    rect.size.height = height;

    [view setFrame:rect];
  }
}

void OSXPlatformInterface::RemoveView()
{
  if (mVideoView) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSView *view = (NSView *)mVideoView;
    mVideoView = NULL;

    // Remove the old view, if there was one.
    [view performSelectorOnMainThread:@selector(removeFromSuperview) withObject:nil waitUntilDone:NO];

    [pool release];
  }
}

