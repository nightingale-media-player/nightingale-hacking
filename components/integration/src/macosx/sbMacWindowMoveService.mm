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
// SBMacCocoaWindowListener
//==============================================================================

@interface SBMacCocoaWindowListener (Private)

- (void)startListening;
- (void)stopListening;
- (void)windowWillMove:(NSNotification *)aNotification;
- (void)windowDidMove:(NSNotification *)aNotification;
- (void)getListenerForNotification:(NSNotification *)aNotification
            outListener:(sbIWindowMoveListener **)aOutListener;

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

  SBISupportsOwner *listener =
    [[SBISupportsOwner alloc] initWithValue:aListener];
  NS_ENSURE_TRUE(listener, /* void */);

  [mListenerWinDict setObject:listener
                       forKey:[NSNumber numberWithInt:[aWindow hash]]];
}

- (void)stopObservingWindow:(NSWindow *)aWindow
                forListener:(sbIWindowMoveListener *)aListener
{
  // If this was the last window in the watch list, stop listening.
  NSNumber *winHash = [NSNumber numberWithInt:[aWindow hash]];
  [mListenerWinDict removeObjectForKey:winHash];

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

  // Move
  [defaultCenter addObserver:self
                    selector:@selector(windowWillMove:)
                        name:NSWindowWillMoveNotification
                      object:nil];
  [defaultCenter addObserver:self
                    selector:@selector(windowDidMove:)
                        name:NSWindowDidMoveNotification
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
  nsCOMPtr<sbIWindowMoveListener> listener;
  [self getListenerForNotification:aNotification
                       outListener:getter_AddRefs(listener)];

  if (listener) {
    nsresult rv = listener->OnMoveStarted();
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

- (void)windowDidMove:(NSNotification *)aNotification
{
  nsCOMPtr<sbIWindowMoveListener> listener;
  [self getListenerForNotification:aNotification
                       outListener:getter_AddRefs(listener)];

  if (listener) {
    nsresult rv = listener->OnMoveStopped();
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

- (void)getListenerForNotification:(NSNotification *)aNotification
            outListener:(sbIWindowMoveListener **)aOutListener
{
  if (!aOutListener || !aNotification) {
    return;
  }

  *aOutListener = nsnull;  // null out by default.

  // The window is the object value on this event.
  NSWindow *window = (NSWindow *)[aNotification object];
  NSNumber *winHash = [NSNumber numberWithInt:[window hash]];
  SBISupportsOwner *supports =
    (SBISupportsOwner *)[mListenerWinDict objectForKey:winHash];

  if (supports) {
    nsresult rv;
    nsCOMPtr<sbIWindowMoveListener> listener =
      do_QueryInterface([supports value], &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);

    listener.forget(aOutListener);
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

