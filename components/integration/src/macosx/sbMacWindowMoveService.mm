/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbMacWindowMoveService.h"
#include "../NativeWindowFromNode.h"

#include <nsIDOMWindow.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#import <objc/objc-class.h>


//==============================================================================
// ObjC class wrapper for owning nsISupports derivatives.
//==============================================================================

@interface SBISupportsOwner : NSObject
{
  nsISupports *mISupports;  // AddRef'd
}

- (id)initWithValue:(nsISupports *)aValue;

- (nsISupports *)value;  // return is not add-ref'd

@end


@implementation SBISupportsOwner

- (id)initWithValue:(nsISupports *)aValue;
{
  if ((self = [super init])) {
    mISupports = aValue;
    NS_IF_ADDREF(mISupports);
  }

  return self;
}

- (void)dealloc
{
  NS_IF_RELEASE(mISupports);
  [super dealloc];
}

- (nsISupports *)value
{
  return mISupports;
}

@end


//==============================================================================
// Method swizzle method.
//==============================================================================

void MethodSwizzle(Class aClass, SEL orig_sel, SEL alt_sel)
{
  Method orig_method = nil, alt_method = nil;

  // First, look for methods
  orig_method = class_getInstanceMethod(aClass, orig_sel);
  alt_method = class_getInstanceMethod(aClass, alt_sel);

  // If both are found, swizzle them.
  if ((orig_method != nil) && (alt_method != nil)) {
    char *temp_type;
    IMP temp_imp;

    temp_type = orig_method->method_types;
    orig_method->method_types = alt_method->method_types;
    alt_method->method_types = temp_type;

    temp_imp = orig_method->method_imp;
    orig_method->method_imp = alt_method->method_imp;
    alt_method->method_imp = temp_imp;;
  }
}


//==============================================================================
// NSWindow category entry for swizzled method.
//==============================================================================

static NSString *kSBWindowStoppedMovingNotification = @"SBWindowStoppedMoving";


@interface NSWindow (SBSwizzleSupport)

- (void)swizzledSendEvent:(NSEvent *)aEvent;

@end

@implementation NSWindow (SBSwizzleSupport)

- (void)swizzledSendEvent:(NSEvent *)aEvent
{
  // Swizzle back to the original method.
  MethodSwizzle([self class],
                @selector(swizzledSendEvent:),
                @selector(sendEvent:));

  [self sendEvent:aEvent];

  // Check for the mouse up event.
  BOOL shouldReswizzle = YES;
  if (NSLeftMouseUp == [aEvent type]) {
    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter postNotificationName:kSBWindowStoppedMovingNotification
                                 object:self];
    shouldReswizzle = NO;
  }

  // If this event was the mouse up event, don't reswizzle. The method will be
  // reswizzled when this interface gets notified of a window move start in
  // |onWindowWillMove|.
  if (shouldReswizzle) {
    MethodSwizzle([self class],
                  @selector(sendEvent:),
                  @selector(swizzledSendEvent:));
  }
}

@end


//==============================================================================
// Window Listener Context Utility Class.
//==============================================================================

@interface SBWinMoveListenerContext : NSObject
{
  SBISupportsOwner *mListener;       // strong
  NSView           *mWatchedView;    // strong
}

- (id)initWithListener:(sbIWindowMoveListener *)aListener
                  view:(NSView *)aView;

- (void)onWindowWillMove;
- (void)onWindowDidStopMoving:(NSNotification *)aNotification;
- (void)notifyListenerMoveStoppedTimeout;

@end

@implementation SBWinMoveListenerContext

- (id)initWithListener:(sbIWindowMoveListener *)aListener
                  view:(NSView *)aView
{
  if ((self = [super init])) {
    mListener = [[SBISupportsOwner alloc] initWithValue:aListener];
    mWatchedView = [aView retain];
  }

  return self;
}

- (void)dealloc
{
  [mListener release];
  [mWatchedView release];
  [super dealloc];
}

- (void)onWindowWillMove
{
  // First, notify our listener
  nsresult rv;
  nsCOMPtr<sbIWindowMoveListener> listener =
    do_QueryInterface([mListener value], &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  listener->OnMoveStarted();

  // Next, to avoid hacking on XR for bug XXX simply method swizzle the
  // |mouseUp:| event handler for the window class. This can be avoided by
  // patching XR to post an event when this event happens in the |NSWindow|
  // subclasses in the cocoa widget stuff.
  MethodSwizzle([[mWatchedView window] class],
                @selector(sendEvent:),
                @selector(swizzledSendEvent:));

  NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(onWindowDidStopMoving:)
                        name:kSBWindowStoppedMovingNotification
                      object:nil];
}

- (void)onWindowDidStopMoving:(NSNotification *)aNotification
{
  // Hack, notify after a short timeout to ensure the listener will actually
  // be able to look at the window frame.
  [self performSelector:@selector(notifyListenerMoveStoppedTimeout)
             withObject:nil
             afterDelay:0.1];

  // No longer need to listen to events for now.
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)notifyListenerMoveStoppedTimeout
{
  // Notify our listener of the move stop
  nsresult rv;
  nsCOMPtr<sbIWindowMoveListener> listener =
    do_QueryInterface([mListener value], &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  listener->OnMoveStopped();
}

@end


//==============================================================================
// SBMacCocoaWindowListener
//==============================================================================

@interface SBMacCocoaWindowListener (Private)

- (void)startListening;
- (void)stopListening;
- (void)windowWillMove:(NSNotification *)aNotification;

@end


@implementation SBMacCocoaWindowListener

- (id)initWithService:(sbMacWindowMoveService *)aService
{
  if ((self = [super init])) {
    mListenerWinDict = [[NSMutableDictionary alloc] init];
    mService = aService;
    mIsObserving = NO;
  }

  return self;
}

- (void)dealloc
{
  [mListenerWinDict release];
  mService = nsnull;
  [super dealloc];
}

- (void)beginObservingWindow:(NSWindow *)aWindow
                 forListener:(sbIWindowMoveListener *)aListener
{
  if (!mIsObserving) {
    [self startListening];
  }

  // The NSWindow goes away when we go fullscreen.  Retain the contentView
  // instead since that persists.
  NSView *contentView = [aWindow contentView];

  SBWinMoveListenerContext *listenerContext =
    [[SBWinMoveListenerContext alloc] initWithListener:aListener
                                                  view:contentView];

  [mListenerWinDict setObject:listenerContext
                       forKey:[NSNumber numberWithInt:[contentView hash]]];
}

- (void)stopObservingWindow:(NSWindow *)aWindow
                forListener:(sbIWindowMoveListener *)aListener
{
  NSView *contentView = [aWindow contentView];

  // If this was the last window in the watch list, stop listening.
  NSNumber *viewHash = [NSNumber numberWithInt:[contentView hash]];
  [mListenerWinDict removeObjectForKey:viewHash];

  // If this was the last listener in the dictionary, stop listening to window
  // events for the app.
  if ([mListenerWinDict count] == 0) {
    [self stopListening];
  }
}

- (void)startListening
{
  if (mIsObserving) {
    return;
  }

  // Listen to window move events.
  NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(windowWillMove:)
                        name:NSWindowWillMoveNotification
                      object:nil];

  mIsObserving = YES;
}

- (void)stopListening
{
  if (!mIsObserving) {
    return;
  }

  [[NSNotificationCenter defaultCenter] removeObserver:self];
  mIsObserving = NO;
}

- (void)windowWillMove:(NSNotification *)aNotification
{
  NSWindow *eventWindow = (NSWindow *)[aNotification object];

  NSView *contentView = [eventWindow contentView];

  SBWinMoveListenerContext *listenerContext =
    (SBWinMoveListenerContext *)[mListenerWinDict objectForKey:
      [NSNumber numberWithInt:[contentView hash]]];
  if (listenerContext) {
    [listenerContext onWindowWillMove];
  }
}

@end


//==============================================================================
// sbMacWindowMoveResizeService
//==============================================================================

NS_IMPL_ISUPPORTS1(sbMacWindowMoveService, sbIWindowMoveService)

sbMacWindowMoveService::sbMacWindowMoveService()
{
}

sbMacWindowMoveService::~sbMacWindowMoveService()
{
  [mWinListener release];
}

nsresult
sbMacWindowMoveService::Init()
{
  mWinListener = [[SBMacCocoaWindowListener alloc] initWithService:this];
  NS_ENSURE_TRUE(mWinListener, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIWindowMoveService

NS_IMETHODIMP
sbMacWindowMoveService::StartWatchingWindow(
    nsISupports *aWindow,
    sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NSWindow *window = NativeWindowFromNode::get(supports);
  NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

  [mWinListener beginObservingWindow:window forListener:aListener];
  return NS_OK;
}

NS_IMETHODIMP
sbMacWindowMoveService::StopWatchingWindow(
    nsISupports *aWindow,
    sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NSWindow *window = NativeWindowFromNode::get(supports);
  NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

  [mWinListener stopObservingWindow:window forListener:aListener];
  return NS_OK;
}

