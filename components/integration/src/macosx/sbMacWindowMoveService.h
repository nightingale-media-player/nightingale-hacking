/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef sbMacWindowMoveService_h_
#define sbMacWindowMoveService_h_

#include <sbIWindowMoveService.h>

#import <AppKit/AppKit.h>

class sbMacWindowMoveService;


//==============================================================================
//
// @brief Native ObjC class used as a delegate for window move/resize event
//        fowarding to the XPCOM service.
//
//==============================================================================

@interface SBMacCocoaWindowListener : NSObject
{
@private
  NSMutableDictionary          *mListenerWinDict;  // strong
  sbMacWindowMoveService       *mService;          // weak, it owns us.
  BOOL                         mIsObserving;
}

//
// @brief Init a cocoa window listener instace with a callback to the parent
//        window resize/move service.
// @param aService The parent service to callback to when window events occur.
//
- (id)initWithService:(sbMacWindowMoveService *)aService;

//
// @brief Start watching a window. Any move or resize events that occur on
//        |aWindow| will result in callbacks to the parent service.
// @param aWindow The window to start watching.
// @param aListener The listener to inform of window move/resize events.
//
- (void)beginObservingWindow:(NSWindow *)aWindow
                 forListener:(sbIWindowMoveListener *)aListener;

//
// @brief Stop watching a window.
// @param aWindow The window to stop watching for move and resize events.
// @param aListener The listener that was watching the window.
//
- (void)stopObservingWindow:(NSWindow *)aWindow
                forListener:(sbIWindowMoveListener *)aListener;

@end


//==============================================================================
//
// @interface sbMacWindowMoveService
// @brief XPCOM service to provide window move notifications to a
//        listener for a given (gecko) window.
//
//==============================================================================

class sbMacWindowMoveService : public sbIWindowMoveService
{
public:
  sbMacWindowMoveService();
  virtual ~sbMacWindowMoveService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWMOVESERVICE

  nsresult Init();

private:
  SBMacCocoaWindowListener *mWinListener;  // strong
};

#endif  // sbMacWindowMoveService_h_

