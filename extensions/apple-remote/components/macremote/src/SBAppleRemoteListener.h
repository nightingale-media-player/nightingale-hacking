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

#import <Cocoa/Cocoa.h>
#import <IOKit/hid/IOHIDLib.h>

//
// @brief Apple remote control buttons
//
typedef enum {
  eUnknownButton  = -1,
  ePlusButton     = 0,
  eMinusButton    = 1,
  eMenuButton     = 2,
  ePlayButton     = 3,
  eRightButton    = 4,
  eLeftButton     = 5,
} EAppleRemoteButton;


//
// @brief Informal protocol for getting callback information about Apple
//        remote control events.
//
@interface NSObject (AppleRemoteDelegate)

//
// @brief Callback function when a button is pressed.
//
- (void)onButtonPressed:(EAppleRemoteButton)aButton isHold:(BOOL)aIsHold;

//
// @brief Callback function when a button is released.
//
- (void)onButtonReleased:(EAppleRemoteButton)aButton;

@end


//
// @brief A class for listening to Apple Remote Control Events.
//     
// XXXkreeger --> Better name here please...
//
@interface SBAppleRemoteListener : NSObject
{
  IOHIDDeviceInterface   **mDeviceInterface;
  IOHIDQueueInterface    **mQueueInterface;
  CFRunLoopSourceRef	   mEventSource;
  NSArray                *mCookies;               // strong
  id                     mDelegate;               // weak
  BOOL                   mIsListening;
  EAppleRemoteButton     mPendingHoldButton;
}

//
// @brief Find out if the current machine supports the Apple remote.
//
+ (BOOL)isRemoteAvailable;

//
// @brief Create a remote control listener for the Apple remote with a 
//        callback delegate. This function will not start listening for
//        device events. To engage listening, call |startListening|.
//
- (id)initWithDelegate:(id)aDelegate;

//
// @brief Start listening to the apple remote. This will claim exclusive 
//        access to the device. To release exclusive access, call 
//        |stopListening|.
//
- (void)startListening;

//
// @brief Stop listening to the device. This method will release exclusive
//        access to the device.
//
- (void)stopListening;

//
// @brief Get the listening state of the remote.
//
- (BOOL)IsListening;

//
// @brief Set the remote delegate.
//
- (void)setDelegate:(id)aDelegate;

@end
