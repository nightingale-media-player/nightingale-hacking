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
#include "sbGStreamerMediacore.h"

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


/**
 * ObjC NSView category to add some safer subview insertion.
 */
@interface NSView (GStreamerPlatformOSX)

- (void)setFrameAsString:(NSString *)aString;

@end

@implementation NSView (GStreamerPlatformOSX)

- (void)setFrameAsString:(NSString *)aString
{
  NSRect newFrame = NSRectFromString(aString);
  [self setFrame:newFrame];
}

@end


@interface NSView (GstGlView)

- (void)setDelegate:(id)aDelegate;

@end


/**
 * ObjC class to listen to |NSView| events from the GST video view.
 */
@interface SBGstGLViewDelgate : NSObject
{
  OSXPlatformInterface *mOwner;  // weak, it owns us.
}

- (id)initWithPlatformInterface:(OSXPlatformInterface *)aOwner;
- (void)startListeningToResizeEvents;
- (void)stopListeningToResizeEvents;
- (void)mouseMoved:(NSEvent *)theEvent;
- (void)windowResized:(NSNotification *)aNotice;

@end

@implementation SBGstGLViewDelgate

- (id)initWithPlatformInterface:(OSXPlatformInterface *)aOwner
{
  if ((self = [super init])) {
    mOwner = aOwner;
  }

  return self;
}

- (void)dealloc
{
  if (mOwner) {
    mOwner = nsnull;
  }

  [[NSNotificationCenter defaultCenter] removeObserver:self];

  [super dealloc];
}

- (void)startListeningToResizeEvents
{
  if (!mOwner) {
    return;
  }

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowResized:)
                                               name:NSWindowDidResizeNotification
                                             object:nil];
}

- (void)stopListeningToResizeEvents
{
  if (!mOwner) {
    return;
  }

  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
  if (!mOwner) {
    return;
  }

  mOwner->OnMouseMoved(theEvent);
}

- (void)windowResized:(NSNotification *)aNotice
{
  if (!mOwner) {
    return;
  }

  if ([[aNotice object] isEqual:[(NSView *)mOwner->GetVideoView() window]]) {
    mOwner->OnWindowResized();
  }
}

@end


OSXPlatformInterface::OSXPlatformInterface(sbGStreamerMediacore *aCore) :
    BasePlatformInterface(aCore),
    mParentView(NULL),
    mVideoView(NULL),
    mGstGLViewDelegate(NULL)
{
  mGstGLViewDelegate =
    (void *)[[SBGstGLViewDelgate alloc] initWithPlatformInterface:this];
}

OSXPlatformInterface::~OSXPlatformInterface()
{
  RemoveView();

  // Clean up the view delegate.
  SBGstGLViewDelgate *delegate = (SBGstGLViewDelgate *)mGstGLViewDelegate;
  [delegate release];
  mGstGLViewDelegate = NULL;
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

  if (aWidget) {
    mParentView = (void *)(aWidget->GetNativeData(NS_NATIVE_WIDGET));
    NS_ENSURE_TRUE(mParentView != NULL, NS_ERROR_FAILURE);
  }
  else {
    RemoveView();
    mParentView = NULL;
  }

  return NS_OK;
}

void
OSXPlatformInterface::PrepareVideoWindow(GstMessage *aMessage)
{
  BasePlatformInterface::PrepareVideoWindow(aMessage);

  // Firstly, if we don't already have a video view set up, request a video
  // window, and set up the appropriate parent view.
  if (!mParentView) {
    nsCOMPtr<nsIThread> mainThread;
    nsresult rv = NS_GetMainThread(getter_AddRefs(mainThread));
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
  const GValue *value = gst_structure_get_value(
                          gst_message_get_structure(aMessage), "nsview");

  if (!value || !G_VALUE_HOLDS_POINTER(value))
    return;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  RemoveView();

  NSView *parentView = (NSView *)mParentView;
  mVideoView = g_value_get_pointer (value);
  view = (NSView *)mVideoView;

  // Listen to live resize events since gecko resize events aren't posted on
  // Mac until the resize has finished. (see bug 20445).
  SBGstGLViewDelgate *delegate = (SBGstGLViewDelgate *)mGstGLViewDelegate;
  [delegate startListeningToResizeEvents];

  // Now, we want to set this view as a subview of the NSView we have
  // as our window-for-displaying-video. Don't do this from a non-main
  // thread, though!
  [parentView performSelectorOnMainThread:@selector(addSubview:)
                               withObject:view
                            waitUntilDone:YES];

  // Fail safe, ensure that the gst |NSView| responds to the delegate method
  // before attempting to set the delegate of the view.
  if ([view respondsToSelector:@selector(setDelegate:)]) {
    [view setDelegate:(id)mGstGLViewDelegate];
  }

  // Resize the window
  ResizeToWindow();

  [pool release];
}

void*
OSXPlatformInterface::GetVideoView()
{
  return mVideoView;
}

void
OSXPlatformInterface::OnMouseMoved(void *aCocoaEvent)
{
  NS_ENSURE_TRUE(aCocoaEvent, /* void */);
  NS_ENSURE_TRUE(mVideoView, /* void */);

  // Convert the cocoa event to a gecko event
  nsresult rv;
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent;
  rv = CreateDOMMouseEvent(getter_AddRefs(mouseEvent));
  NS_ENSURE_SUCCESS(rv, /* void */);

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Get the window and local view coordinates and convert them from cocoa
  // coords (bottom-left) to gecko coords (top-left).
  NSEvent *event = (NSEvent *)aCocoaEvent;
  NSView *eventView = (NSView *)mVideoView;

  float menuBarHeight = 0.0;
  NSArray *allScreens = [NSScreen screens];
  if ([allScreens count]) {
    menuBarHeight = [[allScreens objectAtIndex:0] frame].size.height;
  }
  float windowHeight = [[[eventView window] contentView] frame].size.height;

  // This will need to be flipped....
  NSPoint viewPoint = [event locationInWindow];
  NSPoint screenPoint = [[eventView window] convertBaseToScreen:viewPoint];

  viewPoint.y = windowHeight - viewPoint.y;
  screenPoint.y = menuBarHeight - screenPoint.y;

  rv = mouseEvent->InitMouseEvent(
      NS_LITERAL_STRING("mousemove"),
      PR_TRUE,
      PR_TRUE,
      nsnull,
      0,
      (PRInt32)screenPoint.x,
      (PRInt32)screenPoint.y,
      (PRInt32)viewPoint.x,
      (PRInt32)viewPoint.y,
      (([event modifierFlags] & NSControlKeyMask) != 0),
      (([event modifierFlags] & NSAlternateKeyMask) != 0),
      (([event modifierFlags] & NSShiftKeyMask) != 0),
      (([event modifierFlags] & NSCommandKeyMask) != 0),
      0,
      nsnull);
  [pool release];
  NS_ENSURE_SUCCESS(rv, /* void */);

  nsCOMPtr<nsIDOMEvent> domEvent(do_QueryInterface(mouseEvent));
  rv = DispatchDOMEvent(domEvent);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

void
OSXPlatformInterface::OnWindowResized()
{
  ResizeToWindow();
}

void
OSXPlatformInterface::MoveVideoWindow (int x, int y, int width, int height)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSView *view = (NSView *)mVideoView;
  if (view) {
    NSRect rect;
    // Remap to OSX's coordinate system, which is from the bottom left.
    rect.origin.y = [[view superview] frame].size.height - y - height;

    rect.origin.x = x;
    rect.size.width = width;
    rect.size.height = height;

    // A ObjC object is needed to simply perform a selector on the main thread.
    // To do this, simply convert the calculated rect to a NSString.
    NSString *frameStr = NSStringFromRect(rect);
    [view performSelectorOnMainThread:@selector(setFrameAsString:)
                           withObject:frameStr
                        waitUntilDone:YES];
  }

  [pool release];
}

void OSXPlatformInterface::RemoveView()
{
  if (mVideoView) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSView *view = (NSView *)mVideoView;
    if (view) {
      [view performSelectorOnMainThread:@selector(removeFromSuperviewWithoutNeedingDisplay)
                             withObject:nil
                          waitUntilDone:YES];
    }

    SBGstGLViewDelgate *delegate = (SBGstGLViewDelgate *)mGstGLViewDelegate;
    [delegate stopListeningToResizeEvents];

    mVideoView = nsnull;
    [pool release];
  }
}

