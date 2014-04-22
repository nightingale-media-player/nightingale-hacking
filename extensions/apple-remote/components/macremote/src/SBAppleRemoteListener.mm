/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale Media Player.
//
// Copyright(c) 2014
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

#import "SBAppleRemoteListener.h"
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <AppKit/AppKit.h>
#import <mach/mach.h>
#import <mach/mach_error.h>

// Enable IO logging:
#define LOG_IO_ERRORS   0

// AppKit Version for 10.4
#define NSAppKitVersionNumber10_4 824

// Don't expose the hold-button events via the enum
#define RIGHT_BUTTON_HOLD   6
#define LEFT_BUTTON_HOLD    7
#define MENU_BUTTON_HOLD    8
#define PLAY_BUTTON_HOLD    9

// Globals:
static const char   *sAppleRemoteDeviceName = "AppleIRController";

/////////////////////////////////////////////////////////////////////////////


@interface SBAppleRemoteListener (Private)

- (void)_onQueueCallback:(IOReturn)aResult;
+ (io_object_t)_findDevice;
- (IOHIDDeviceInterface **)_createInterface:(io_object_t)aDevice;
- (IOCFPlugInInterface **)_getPluginInterface:(io_object_t)aDevice;
- (NSArray *)_readDeviceCookies:(IOHIDDeviceInterface **)aDeviceInterface;
- (EAppleRemoteButton)_buttonForCookie:(NSString *)aCookieStr 
                           isHoldEvent:(BOOL *)aIsHoldEvent;
- (NSDictionary *)_getCookieButtonsDict;
- (void)_handleEventWithCookieString:(NSString *)aCookieStr 
                         sumOfValues:(SInt32)aSumOfValues;

@end


/////////////////////////////////////////////////////////////////////////////


static void QueueCallbackFunction(void *target,  
                                  IOReturn result, 
                                  void *refcon, 
                                  void *sender) 
{
  if (target < 0) {
    return; 
  }
  
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  SBAppleRemoteListener *appleRemote = (SBAppleRemoteListener *)target;
  if (appleRemote) {
    [appleRemote _onQueueCallback:result];
  }
  
  [pool release];
}


/////////////////////////////////////////////////////////////////////////////


@implementation SBAppleRemoteListener

+ (BOOL)isRemoteAvailable
{
  BOOL exists = NO;
  io_object_t device = [SBAppleRemoteListener _findDevice];
  if (device != 0) {
    IOObjectRelease(device);
    exists = YES;
  }
  
  return exists;
}


- (id)init
{
  if ((self = [super init])) {
    mPendingHoldButton = eUnknownButton;
  }
  
  return self;
}


- (id)initWithDelegate:(id)aDelegate
{
  if ((self = [self init])) {
    mDelegate = aDelegate;
  }
  
  return self;
}


- (void)dealloc
{
  if (mIsListening)
    [self stopListening];
  
  [super dealloc];
}


- (void)startListening
{
  io_object_t hidDevice = [SBAppleRemoteListener _findDevice];
  if (hidDevice == 0) {
#if LOG_IO_ERRORS
    NSLog(@"ERROR: Could not find the device in I/O registry!!!");
#endif
    return;
  }
  
  //
  // 1.) Create interface for device
  // 2.) Init cookies
  // 3.) Open device
  //
  
  //
  // 1.) Create an interface for the device:
  //
  mDeviceInterface = [self _createInterface:hidDevice];
  if (mDeviceInterface == NULL) {
#if LOG_IO_ERRORS
    NSLog(@"ERROR: Could not create an interface for the device!");
#endif
    return;
  }
  
  //
  // 2.) Init device cookies:
  //
  mCookies = [self _readDeviceCookies:mDeviceInterface];
  
  //
  // 3.) Open the device
  //
  IOHIDOptionsType openMode = kIOHIDOptionsTypeSeizeDevice;  
  IOReturn ioResult= (*mDeviceInterface)->open(mDeviceInterface, openMode);
  if (ioResult == KERN_SUCCESS) {
    mQueueInterface = (*mDeviceInterface)->allocQueue(mDeviceInterface);
    if (mQueueInterface) {
      // depth: maximum number of elements in queue before oldest elements 
      //        in queue begin to be lost.
      (*mQueueInterface)->create(mQueueInterface, 0, 12);
      
      // Add the found cookies to the queue.
      NSNumber *curCookie = nil;
      NSEnumerator *cookiesEnum = [mCookies objectEnumerator];
      while ((curCookie = [cookiesEnum nextObject])) {
        (*mQueueInterface)->addElement(mQueueInterface, 
                                       (IOHIDElementCookie)[curCookie intValue], 
                                       0);
      }
      
      // Add callbacks for async events:
      ioResult = (*mQueueInterface)->createAsyncEventSource(mQueueInterface, 
                                                            &mEventSource);
      if (ioResult == KERN_SUCCESS) {
        ioResult = (*mQueueInterface)->setEventCallout(mQueueInterface,
                                                       QueueCallbackFunction,
                                                       self, 
                                                       NULL); 
        if (ioResult == KERN_SUCCESS) {
          CFRunLoopAddSource(CFRunLoopGetCurrent(), 
                             mEventSource, 
                             kCFRunLoopDefaultMode); 
          // Begin queue delivery
          (*mQueueInterface)->start(mQueueInterface);
        }
      }
    }
  }
  else if (ioResult == kIOReturnExclusiveAccess) {
    // XXXkreeger -> Handle waiting until exclusive access is over.
  }
  
  IOObjectRelease(hidDevice);
  mIsListening = YES;
}


- (void)stopListening
{
  if (mEventSource) {
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), 
                          mEventSource, 
                          kCFRunLoopDefaultMode);
    CFRelease(mEventSource);
    mEventSource = NULL;
  }
  
  if (mQueueInterface) {
    (*mQueueInterface)->stop(mQueueInterface);
    (*mQueueInterface)->dispose(mQueueInterface);
    (*mQueueInterface)->Release(mQueueInterface);
    mQueueInterface = NULL;
  }
  
  if (mDeviceInterface) {
    (*mDeviceInterface)->close(mDeviceInterface);
    (*mDeviceInterface)->Release(mDeviceInterface);
    mDeviceInterface = NULL;
  }
  
  if (mCookies) {
    [mCookies release];
  }
  
  mIsListening = NO;
}


- (BOOL)IsListening
{
  return mIsListening;
}


- (void)setDelegate:(id)aDelegate
{
  mDelegate = aDelegate;
}


- (void)_onQueueCallback:(IOReturn)aResult
{
  AbsoluteTime zeroTime = { 0, 0 };
  SInt32 sumOfValues = 0;
  NSMutableString *cookieString = [[NSMutableString alloc] init];
  
  while (aResult == kIOReturnSuccess) {
    IOHIDEventStruct curEvent;
    
    aResult = (*mQueueInterface)->getNextEvent(mQueueInterface, 
                                              &curEvent, 
                                              zeroTime, 
                                              0);
    if (aResult != kIOReturnSuccess)
      break;
    
    // XXXkreeger: Handle remote switch event?
    
    sumOfValues += curEvent.value;
    [cookieString appendString:
      [NSString stringWithFormat:@"%d_", (int)curEvent.elementCookie]];
  }
  
  [self _handleEventWithCookieString:cookieString sumOfValues:sumOfValues];
}


- (void)_handleEventWithCookieString:(NSString *)aCookieStr 
                         sumOfValues:(SInt32)aSumOfValues
{
  if (!aCookieStr)
    return;
  
  BOOL isHoldEvent = NO;
  EAppleRemoteButton button = [self _buttonForCookie:aCookieStr 
                                         isHoldEvent:&isHoldEvent];
  
  if (mDelegate && button != eUnknownButton) {
    if (isHoldEvent) {
      // Save this flag for informing the delegate about 'release' events.
      if (mPendingHoldButton == eUnknownButton) {
        // The hold event is starting, stash this event
        mPendingHoldButton = button;
      }
      else {
        // The button was released, 
        mPendingHoldButton = eUnknownButton;
        [mDelegate onButtonReleased:button];
        return;
      }
    }
    
    [mDelegate onButtonPressed:button isHold:isHoldEvent];
  }
}
  
  
+ (io_object_t)_findDevice
{
  io_object_t deviceRetVal = 0;
  
  // Search I/O registry for devices:
  io_iterator_t objectIter = 0;
  IOReturn result = kIOReturnSuccess;
  result = IOServiceGetMatchingServices(kIOMasterPortDefault, 
                                        IOServiceMatching(sAppleRemoteDeviceName),
                                        &objectIter);
  if ((result == kIOReturnSuccess) && (objectIter != 0)) {
    deviceRetVal = IOIteratorNext(objectIter);
  }
  
  IOObjectRelease(objectIter);
  
  return deviceRetVal;
}


- (IOHIDDeviceInterface **)_createInterface:(io_object_t)aDevice
{
  IOHIDDeviceInterface **iodevice = NULL;
  
  IOCFPlugInInterface **pluginInterface = [self _getPluginInterface:aDevice];
  if (pluginInterface != NULL) {
    HRESULT pluginResult;
    pluginResult = (*pluginInterface)->QueryInterface(pluginInterface,
                                                      CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID),
                                                      (LPVOID *)&iodevice);
  }
  
  return iodevice;
}


- (IOCFPlugInInterface **)_getPluginInterface:(io_object_t)aDevice
{
  IOReturn result = kIOReturnSuccess;
  SInt32 score = 0;
  IOCFPlugInInterface **pluginInterface = NULL;
  
  result = IOCreatePlugInInterfaceForService(aDevice, 
                                             kIOHIDDeviceUserClientTypeID, 
                                             kIOCFPlugInInterfaceID, 
                                             &pluginInterface, 
                                             &score);
  if (result != kIOReturnSuccess) {
#if LOG_IO_ERRORS
    NSLog(@"Couldn't get a plugin interface!");
#endif
    return NULL;
  }
  
  return pluginInterface;
}


- (NSArray *)_readDeviceCookies:(IOHIDDeviceInterface **)aDeviceInterface
{
  IOHIDDeviceInterface122 **handle = (IOHIDDeviceInterface122 **)aDeviceInterface;
  if (!handle || !(*handle))
    return nil;
  
  NSMutableArray *cookieList = [[NSMutableArray alloc] init];
  
  IOReturn result = kIOReturnSuccess;
  NSArray *elements = nil;
  result = (*handle)->copyMatchingElements(handle, NULL, (CFArrayRef *)&elements);
  if (result == kIOReturnSuccess) {
    NSDictionary *curElementDict = nil;
    NSEnumerator *elementsEnum = [elements objectEnumerator];
    while ((curElementDict = [elementsEnum nextObject])) {
      // Before adding a cookie, make sure the entry has at least 3 things:
      // Cookie:
      id cookie = 
        [curElementDict valueForKey:(NSString *)CFSTR(kIOHIDElementCookieKey)];
      if (!cookie || ![cookie isKindOfClass:[NSNumber class]] || cookie == 0)
        continue;
      
      // Usage:
      id usage = 
        [curElementDict valueForKey:(NSString *)CFSTR(kIOHIDElementUsageKey)];
      if (!usage || ![usage isKindOfClass:[NSNumber class]])
        continue;
      
      // Usage Page:
      id usagePage =
        [curElementDict valueForKey:(NSString *)CFSTR(kIOHIDElementUsagePageKey)];
      if (!usagePage || ![usagePage isKindOfClass:[NSNumber class]])
        continue;
      
      [cookieList addObject:cookie];
    }
  }
  
  return cookieList;
}


- (EAppleRemoteButton)_buttonForCookie:(NSString *)aCookieStr 
                           isHoldEvent:(BOOL *)aIsHoldEvent
{
  EAppleRemoteButton buttonType = eUnknownButton;
  if (!aIsHoldEvent)
    return  buttonType;
  
  NSDictionary *cookiesDict = [self _getCookieButtonsDict];
  
  id buttonNum = [cookiesDict valueForKey:aCookieStr];
  if (buttonNum) {
    unsigned int buttonCode = [buttonNum intValue];
    if (buttonCode > eLeftButton) {
      // Hold events are not defined in |EAppleRemoteButton|.
      switch (buttonCode) {
        case RIGHT_BUTTON_HOLD:
          buttonType = eRightButton;
          *aIsHoldEvent = YES;
          break;
        case LEFT_BUTTON_HOLD:
          buttonType = eLeftButton;
          *aIsHoldEvent = YES;
          break;
          
        // There is not any callback (up events) for the "Menu" and "Play"
        // buttons - just send those events with out the hold flag.
        case MENU_BUTTON_HOLD:
          buttonType = eMenuButton;
          *aIsHoldEvent = NO;
          break;
        case PLAY_BUTTON_HOLD:
          buttonType = ePlayButton;
          *aIsHoldEvent = NO;
          break;
      }
    }
    else {
      buttonType = (EAppleRemoteButton) [buttonNum intValue];
    }
  }
  
  return buttonType;
}


- (NSDictionary *)_getCookieButtonsDict
{
  // make this static?
  static NSDictionary *cookiesDict = nil;
  // Button cookies are different on Tiger and Leopard.
  if (!cookiesDict) {
    NSArray *cookies = nil;
    NSArray *buttons = 
      [NSArray arrayWithObjects:[NSNumber numberWithInt:ePlusButton],
                                [NSNumber numberWithInt:eMinusButton],
                                [NSNumber numberWithInt:eMenuButton],
                                [NSNumber numberWithInt:ePlayButton],
                                [NSNumber numberWithInt:eRightButton],
                                [NSNumber numberWithInt:eLeftButton],
                                [NSNumber numberWithInt:RIGHT_BUTTON_HOLD],
                                [NSNumber numberWithInt:LEFT_BUTTON_HOLD],
                                [NSNumber numberWithInt:MENU_BUTTON_HOLD],
                                [NSNumber numberWithInt:PLAY_BUTTON_HOLD],
                                nil];
    
    if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_4) {
      // 10.4.x Tiger
      cookies = 
        [NSArray arrayWithObjects:@"14_12_11_6_", 
                                  @"14_13_11_6_", 
                                  @"14_7_6_14_7_6_",
                                  @"14_8_6_14_8_6_",
                                  @"14_9_6_14_9_6_",
                                  @"14_10_6_14_10_6_",
                                  @"14_6_4_2_",
                                  @"14_6_3_2_",
                                  @"14_6_14_6_",
                                  @"18_14_6_18_14_6_",
                                  nil];
    }
    else {
      // 10.5.x Leopard
      cookies = 
        [NSArray arrayWithObjects:@"31_29_28_19_18_", 
                                  @"31_30_28_19_18_", 
                                  @"31_20_19_18_31_20_19_18_",
                                  @"31_21_19_18_31_21_19_18_",
                                  @"31_22_19_18_31_22_19_18_",
                                  @"31_23_19_18_31_23_19_18_",
                                  @"31_19_18_4_2_",
                                  @"31_19_18_3_2_",
                                  @"31_19_18_31_19_18_",
                                  @"35_31_19_18_35_31_19_18_",
                                  nil];
    }
    
    cookiesDict = [[NSDictionary alloc] initWithObjects:buttons forKeys:cookies];
  }
  
  return cookiesDict;
}


@end
