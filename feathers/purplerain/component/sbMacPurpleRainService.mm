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

#include "sbMacPurpleRainService.h"

#include <nsIObserverService.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIWindowMediator.h>
#include <nsICategoryManager.h>
#include <nsIBaseWindow.h>
#include <nsIWidget.h>
#include <nsIProperties.h>
#include <nsStringAPI.h>
#include <nsIFileProtocolHandler.h>
#include <nsIInputStream.h>
#include <nsIBinaryInputStream.h>
#include <nsIFile.h>
#include <nsIChannel.h>
#include <nsNetUtil.h>
#include <nsIChromeRegistry.h>
#include <sbIFeathersManager.h>


//------------------------------------------------------------------------------
// @brief Special subview to color the purple rain title bar for the mac
//        purple rain feather.
//------------------------------------------------------------------------------

@interface PurpleRainTitleBarView : NSView
{
@protected
  NSImage  *mLeftImage;    // strong
  NSImage  *mCenterImage;  // strong
  NSImage  *mRightImage;   // strong
}

- (id)initWithFrame:(NSRect)aFrame
          leftImage:(NSImage *)aLeftImage
        centerImage:(NSImage *)aCenterImage
         rightImage:(NSImage *)aRightImage;
@end

@interface PurpleRainTitleBarView (Private)

- (void)_drawTitlebar;

@end


@implementation PurpleRainTitleBarView

- (id)initWithFrame:(NSRect)aFrame
          leftImage:(NSImage *)aLeftImage
        centerImage:(NSImage *)aCenterImage
         rightImage:(NSImage *)aRightImage
{
  if ((self = [super initWithFrame:aFrame])) {
    mLeftImage = [aLeftImage retain];
    mCenterImage = [aCenterImage retain];
    mRightImage = [aRightImage retain];
  }

  return self;
}

- (void)dealloc
{
  if (mLeftImage) {
    [mLeftImage release];
  }
  if (mCenterImage) {
    [mCenterImage release];
  }
  if (mRightImage) {
    [mRightImage release];
  }

  [super dealloc];
}

- (void)drawRect:(NSRect)aDirtyRect
{
  // If any of the above images are not found, bail.
  if ([mLeftImage size].width == 0 ||
      [mRightImage size].width == 0 || 
      [mCenterImage size].width == 0)
  {
    return;
  }
    
  float trans = 1.0;
  if (![[self window] isKeyWindow]) {
    trans = 0.8;
  }
  
  NSRect curRect = [self bounds];
  
  // First, paint the left endcap
  [mLeftImage drawInRect:curRect 
                fromRect:[self bounds] 
               operation:NSCompositeSourceOver 
                fraction:trans];
  
  // Next, paint the right endcap.
  curRect.origin.x = ([self bounds].size.width - [mRightImage size].width);
  [mRightImage drawInRect:curRect
                 fromRect:[self bounds] 
                operation:NSCompositeSourceOver 
                 fraction:trans];
  
  // Finally, paint the middle porition.
  curRect.origin.x = [self bounds].origin.x + [mLeftImage size].width;
  curRect.size.width =
    [self bounds].size.width - [mRightImage size].width - [mLeftImage size].width;
  
  NSRect drawRect = NSMakeRect(curRect.origin.x,
                               curRect.origin.y,
                               [mCenterImage size].width,
                               [mCenterImage size].height);
  
  NSRect imageRect = NSMakeRect(0, 0,
                                [mCenterImage size].width,
                                [mCenterImage size].height);
  while (drawRect.origin.x + drawRect.size.width < curRect.origin.x + curRect.size.width) {
    [mCenterImage drawInRect:drawRect
                    fromRect:imageRect
                   operation:NSCompositeSourceOver
                    fraction:trans];
    
    // Update draw rect
    drawRect.origin.x += [mCenterImage size].width;
  }
  
  // If there is some gap between the end of |drawRect| and the start of the
  // right endcap, fill in that gap now.
  if (drawRect.origin.x + imageRect.size.width >= curRect.origin.x + curRect.size.width) {
    drawRect.size.width = curRect.origin.x + curRect.size.width - drawRect.origin.x;
    [mCenterImage drawInRect:drawRect
                    fromRect:imageRect
                   operation:NSCompositeSourceOver
                    fraction:trans];
  }
  
  // Lastly, draw the titlebar on top of the newly painted background.
  [self _drawTitlebar];
}

- (void)_drawTitlebar
{
  NSFont *titleFont = [NSFont titleBarFontOfSize:0];  // 0 returns the default size
  NSMutableParagraphStyle *paraStyle = [[NSMutableParagraphStyle alloc] init];
  [paraStyle setParagraphStyle:[NSParagraphStyle defaultParagraphStyle]];
  [paraStyle setAlignment:NSCenterTextAlignment];
  [paraStyle setLineBreakMode:NSLineBreakByTruncatingTail];
  
  NSColor *textColor = [NSColor colorWithDeviceRed:240 green:240 blue:241 alpha:1.0];
  NSMutableDictionary *titleAttrs =
    [NSMutableDictionary dictionaryWithObjectsAndKeys:
        titleFont, NSFontAttributeName,
        textColor, NSForegroundColorAttributeName,
        [[paraStyle copy] autorelease], NSParagraphStyleAttributeName,
        nil];
  
  NSSize titleSize = [[[self window] title] sizeWithAttributes:titleAttrs];
  NSRect titleRect = NSInsetRect([self bounds],
                                 [self bounds].size.height,
                                 (([self bounds].size.height - titleSize.height) / 2.0));
  
  titleRect.size.height += 1;  // HACK
  [[[self window] title] drawInRect:titleRect withAttributes:titleAttrs];
}

@end


//------------------------------------------------------------------------------
// sbMacPurpleRainService
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS2(sbMacPurpleRainService,
                   nsIObserver,
                   nsIWindowMediatorListener)

sbMacPurpleRainService::sbMacPurpleRainService()
{
  mLeftTitlebarImage = nil;
  mCenterTitlebarImage = nil;
  mRightTitlebarImage = nil;
}

sbMacPurpleRainService::~sbMacPurpleRainService()
{
  if (mLeftTitlebarImage) {
    [mLeftTitlebarImage release];
  }
  if (mCenterTitlebarImage) {
    [mCenterTitlebarImage release];
  }
  if (mRightTitlebarImage) {
    [mRightTitlebarImage release];
  }
}

nsresult
sbMacPurpleRainService::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "final-ui-startup", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "quit-application-granted", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert the 'chrome://' URL of the resource images to a path that the
  // cocoa NSImage class can use for painting.
  nsCOMPtr<nsIURI> leftImageChromeURI;
  rv = NS_NewURI(getter_AddRefs(leftImageChromeURI),
      NS_LITERAL_STRING("chrome://purplerain/skin/os-specific/mac/title-bar-endcap-l.png"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> rightImageChromeURI;
  rv = NS_NewURI(getter_AddRefs(rightImageChromeURI),
      NS_LITERAL_STRING("chrome://purplerain/skin/os-specific/mac/title-bar-endcap-r.png"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> centerImageChromeURI;
  rv = NS_NewURI(getter_AddRefs(centerImageChromeURI),
      NS_LITERAL_STRING("chrome://purplerain/skin/os-specific/mac/title-bar-backing.png"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChromeRegistry> chromeRegistry =
    do_GetService("@mozilla.org/chrome/chrome-registry;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> leftImageURI;
  rv = chromeRegistry->ConvertChromeURL(leftImageChromeURI,
                                        getter_AddRefs(leftImageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> rightImageURI;
  rv = chromeRegistry->ConvertChromeURL(rightImageChromeURI,
                                        getter_AddRefs(rightImageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> centerImageURI;
  rv = chromeRegistry->ConvertChromeURL(centerImageChromeURI,
                                        getter_AddRefs(centerImageURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert the chrome URL to a cocoa image to use in the custom view. 
  mLeftTitlebarImage = GetCocoaImageFromChromeURI(leftImageURI);
  mRightTitlebarImage = GetCocoaImageFromChromeURI(rightImageURI);
  mCenterTitlebarImage = GetCocoaImageFromChromeURI(centerImageURI);
  
  return NS_OK;
}

NSWindow*
sbMacPurpleRainService::GetCocoaWindow(nsIXULWindow *aXULWindow)
{
  if (!aXULWindow) {
    return nil; 
  }

  nsresult rv;
  nsCOMPtr<nsIBaseWindow> xulWindowBase = do_QueryInterface(aXULWindow, &rv);
  NS_ENSURE_SUCCESS(rv, nil);

  nsCOMPtr<nsIWidget> mainWidget;
  rv = xulWindowBase->GetMainWidget(getter_AddRefs(mainWidget));
  NS_ENSURE_SUCCESS(rv, nil);

  NSView *mainView = 
    reinterpret_cast<NSView *>(mainWidget->GetNativeData(NS_NATIVE_WIDGET));

  return [mainView window];
}

nsresult
sbMacPurpleRainService::InsertSubview(NSWindow *aWindow)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSView *borderView = [aWindow valueForKey:@"borderView"];
  if (!borderView) {
    [pool release];
    return NS_OK;
  }
  
  NSRect borderBounds = [borderView bounds];
  const float height = 22.0f;
  
  NSRect viewFrame = NSMakeRect(NSMinX(borderBounds),
                                NSMaxY(borderBounds) - height,
                                NSWidth(borderBounds), height);
  
  PurpleRainTitleBarView *view =
    [[PurpleRainTitleBarView alloc] initWithFrame:viewFrame
                                        leftImage:mLeftTitlebarImage
                                      centerImage:mCenterTitlebarImage
                                       rightImage:mRightTitlebarImage];

  [view setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  
  [borderView addSubview:view
              positioned:NSWindowBelow
               relativeTo:nil];

  [pool release];
  return NS_OK;
}

NSImage*
sbMacPurpleRainService::GetCocoaImageFromChromeURI(nsIURI *aChromeURI)
{
  if (!aChromeURI) {
    return nil;
  }

  nsresult rv;
  nsCOMPtr<nsIIOService> ioService =
    do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, nil);

  nsCOMPtr<nsIChannel> chromeChannel;
  rv = ioService->NewChannelFromURI(aChromeURI, getter_AddRefs(chromeChannel));
  NS_ENSURE_SUCCESS(rv, nil);

  // For now, do this synchronously - these are small local png files so it
  // shouldn't be a huge issue.
  nsCOMPtr<nsIInputStream> sourceStream;
  rv = chromeChannel->Open(getter_AddRefs(sourceStream));
  NS_ENSURE_SUCCESS(rv, nil);

  nsCOMPtr<nsIBinaryInputStream> binaryInputSream =
    do_CreateInstance("@mozilla.org/binaryinputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, nil);

  rv = binaryInputSream->SetInputStream(sourceStream);
  NS_ENSURE_SUCCESS(rv, nil);

  // Read all the byes available in the stream (this might not work).
  PRUint32 bytes = 0;
  rv = binaryInputSream->Available(&bytes);
  NS_ENSURE_SUCCESS(rv, nil);

  PRUint8 *data;
  rv = binaryInputSream->ReadByteArray(bytes, &data);
  NS_ENSURE_SUCCESS(rv, nil);

  rv = binaryInputSream->Close();
  NS_ENSURE_SUCCESS(rv, nil);

  NSData *dataWrapper = [[NSData alloc] initWithBytesNoCopy:data length:bytes];

  NSImage *image = [[NSImage alloc] initWithData:dataWrapper];
  [dataWrapper release];

  return image;
}

/* static */ NS_METHOD
sbMacPurpleRainService::RegisterSelf(nsIComponentManager *aCompMgr,
                                     nsIFile *aFile,
                                     const char *aLoaderStr,
                                     const char *aType,
                                     const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aFile);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_MACPURPLERAINSERVICE_CLASSNAME,
                                "service,"
                                  SONGBIRD_MACPURPLERAINSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbMacPurpleRainService::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (strcmp(aTopic, "final-ui-startup") == 0) {
    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    // Start listening to XUL window events.
    nsCOMPtr<nsIWindowMediator> winMed =
      do_GetService("@mozilla.org/appshell/window-mediator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = winMed->AddListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (strcmp(aTopic, "quit-application-granted") == 0) {
    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    // Stop listening to XUL window events.
    nsCOMPtr<nsIWindowMediator> winMed =
      do_GetService("@mozilla.org/appshell/window-mediator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = winMed->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIWindowMediatorListener

NS_IMETHODIMP
sbMacPurpleRainService::OnWindowTitleChange(nsIXULWindow *window,
                                            const PRUnichar *newTitle)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMacPurpleRainService::OnOpenWindow(nsIXULWindow *window)
{
  NS_ENSURE_ARG_POINTER(window);

  // First, make sure that the purple rain feather is turned on.
  nsresult rv;
  nsCOMPtr<sbIFeathersManager> feathersMgr =
    do_GetService("@songbirdnest.com/songbird/feathersmanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString skinName;
  rv = feathersMgr->GetCurrentSkinName(skinName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't bother adding our subview if the purplerain feather isn't active.
  if (!skinName.EqualsLiteral("purplerain")) {
    return NS_OK;
  }

  // If the new window is of the normal NSWindow type or the special XULRunner
  // mac toolbar window (ToolbarWindow), insert our special toolbar view.
  NSWindow *cocoaWin = GetCocoaWindow(window);
  if ([[cocoaWin className] isEqualToString:@"ToolbarWindow"] ||
      [[cocoaWin className] isEqualToString:@"NSWindow"])
  {
    InsertSubview(cocoaWin);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMacPurpleRainService::OnCloseWindow(nsIXULWindow *window)
{
  return NS_OK;
}

