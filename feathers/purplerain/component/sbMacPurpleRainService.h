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

#ifndef sbMacPurpleRainService_h_
#define sbMacPurpleRainService_h_

#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsIWindowMediatorListener.h>
#include <nsIXULWindow.h>
#include <nsIURI.h>
#include <nsIFile.h>
#include <nsIStreamListener.h>

#import <AppKit/AppKit.h>


//------------------------------------------------------------------------------
//
// @inteface sbMacPurpleRainService
// @brief Mac specific class to provide some coloring to the Purple Rain
//        Songbird feathers title bar on the native mac title bar.
//
//------------------------------------------------------------------------------

class sbMacPurpleRainService : public nsIObserver,
                               public nsIWindowMediatorListener
{
public:
  sbMacPurpleRainService();
  virtual ~sbMacPurpleRainService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWINDOWMEDIATORLISTENER
  
  nsresult Init();
  
  static NS_METHOD RegisterSelf(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *aLoaderStr,
                                const char *aType,
                                const nsModuleComponentInfo *aInfo);

protected:
  //
  // @brief Get the native cocoa window (NSWindow *) from a given XUL window.
  // @param aXULWindow The XUL window to lookup the native cocoa window.
  //
  NSWindow* GetCocoaWindow(nsIXULWindow *aXULWindow);

  //
  // @brief Insert the special purple rain title bar view into the specified
  //        window's title bar view.
  // @param aWindow The window to insert the custom title bar view into.
  //
  nsresult InsertSubview(NSWindow *aWindow);

  //
  // @brief Get a cocoa image from a given chrome URI.
  // @param aChromeURI The chrome URI of the image to load as a NSImage.
  //
  NSImage* GetCocoaImageFromChromeURI(nsIURI *aChromeURI);

private:
  NSImage *mLeftTitlebarImage;    // strong
  NSImage *mRightTitlebarImage;   // strong
  NSImage *mCenterTitlebarImage;  // strong
};


// Component Registration Stuff:
#define SONGBIRD_MACPURPLERAINSERVICE_CONTRACTID                \
  "@songbirdnest.com/mac-purplerain-service;1"
#define SONGBIRD_MACPURPLERAINSERVICE_CLASSNAME                 \
  "Songbird Mac Purple Rain Service"
#define SONGBIRD_MACPURPLERAINSERVICE_CID                       \
  {0x6e465540, 0x1dd2, 0x11b2, {0xbb, 0x72, 0xfa, 0x6e, 0xf5, 0x96, 0xfd, 0x7a}}

#endif  // sbMacPurpleRainService_h_

