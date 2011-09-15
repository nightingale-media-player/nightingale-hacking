/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMacWindowTitlebarService.cpp
 * \brief Songbird Mac Window Titlebar Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMacWindowTitlebarService.h"

// Local imports.
#include "../NativeWindowFromNode.h"

// Songbird imports.
#include <sbFileUtils.h>

// Mozilla imports.
#include <nsColor.h>
#include <nsICategoryManager.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMWindow.h>
#include <nsIWindowWatcher.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>

// Mac OS/X imports.
#include <objc/objc-runtime.h>


//------------------------------------------------------------------------------
//
// Internal Songbird Mac window titlebar services prototypes.
//
//------------------------------------------------------------------------------

static nsresult GetDOMWindowFromNativeWindow(NATIVEWINDOW   aNativeWindow,
                                             nsIDOMWindow** aDOMWindow);

static nsresult GetSBTitlebarColor(NSWindow* aWindow,
                                   NSColor** aColor);

static nsresult GetSBTitlebarImage(NSWindow* aWindow,
                                   NSImage** aImage);

static nsresult GetSBTitlebarTextColor(NSWindow* aWindow,
                                       NSColor** aColor);

static nsresult GetWindowAttribute(NSWindow*        aWindow,
                                   const nsAString& aAttributeName,
                                   nsAString&       aAttributeValue);

static nsresult GetWindowAttribute(NSWindow*        aWindow,
                                   const nsAString& aAttributeName,
                                   NSColor**        aColor);

static nsresult GetWindowAttribute(NSWindow*        aWindow,
                                   const nsAString& aAttributeName,
                                   NSImage**        aImage);

static NSImage* GetImageFromURL(const nsAString& aImageURL);

static PRBool SBParseColor(const nsAString& aColorString,
                           nscolor*         aColor);


//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(sbMacWindowTitlebarService)


//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbMacWindowTitlebarService
//

sbMacWindowTitlebarService::sbMacWindowTitlebarService() :
  mInitialized(PR_FALSE)
{
}


//-------------------------------------
//
// ~sbMacWindowTitlebarService
//

sbMacWindowTitlebarService::~sbMacWindowTitlebarService()
{
  // Finalize the Songbird Mac window titlebar service.
  Finalize();
}


//-------------------------------------
//
// RegisterSelf
//

/* static */ NS_METHOD
sbMacWindowTitlebarService::
  RegisterSelf(nsIComponentManager*         aCompMgr,
               nsIFile*                     aPath,
               const char*                  aLoaderStr,
               const char*                  aType,
               const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add self to the application startup category.
  rv = categoryManager->AddCategoryEntry
                          ("app-startup",
                           SB_MAC_WINDOW_TITLEBAR_SERVICE_CLASSNAME,
                           "service," SB_MAC_WINDOW_TITLEBAR_SERVICE_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/* static */ NS_METHOD
sbMacWindowTitlebarService::
  UnregisterSelf(nsIComponentManager*         aCompMgr,
                 nsIFile*                     aPath,
                 const char*                  aLoaderStr,
                 const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete self from the application startup category.
  rv = categoryManager->DeleteCategoryEntry
                          ("app-startup",
                           SB_MAC_WINDOW_TITLEBAR_SERVICE_CLASSNAME,
                           PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// Initialize
//

nsresult
sbMacWindowTitlebarService::Initialize()
{
  // Do nothing if already initialized.
  if (mInitialized)
    return NS_OK;

  // Get the original, Songbird, and placeholder versions of the
  // NSGrayFrame drawRect: method.
  Method origMethod = class_getInstanceMethod([NSGrayFrame class],
                                              @selector(drawRect:));
  Method sbMethod = class_getInstanceMethod([NSGrayFrame class],
                                            @selector(_sbDrawRect:));
  Method origPlaceholderMethod =
    class_getInstanceMethod([NSGrayFrame class], @selector(_sbOrigDrawRect:));

  // Replace the original placeholder method with the original method and
  // replace the original method with the Songbird method.
  origPlaceholderMethod->method_imp = origMethod->method_imp;
  origMethod->method_imp = sbMethod->method_imp;

  // Services are now initialized.
  mInitialized = PR_TRUE;

  return NS_OK;
}


//-------------------------------------
//
// Finalize
//

void
sbMacWindowTitlebarService::Finalize()
{
  // Do nothing if not initialized.
  if (!mInitialized)
    return;

  // Get the original and placeholder versions of the NSGrayFrame drawRect:
  // method.
  Method origMethod = class_getInstanceMethod([NSGrayFrame class],
                                              @selector(drawRect:));
  Method origPlaceholderMethod = class_getInstanceMethod
                                   ([NSGrayFrame class],
                                    @selector(_sbOrigDrawRect:));

  // Restore the original method from the original placeholder method.
  origMethod->method_imp = origPlaceholderMethod->method_imp;

  // No longer initialized.
  mInitialized = PR_FALSE;
}


//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar NSGrayFrame services.
//
//------------------------------------------------------------------------------

@implementation NSGrayFrame (SBGrayFrame)

- (void)_sbDrawRect:(NSRect)aDirtyRect
{
  // Let original method draw.
  [self _sbOrigDrawRect: aDirtyRect];

  // Fill titlebar with the Songbird titlebar color.  Only draw where the
  // titlebar is opaque to preserve rounded corners.
  NSColor* titlebarColor = nil;
  GetSBTitlebarColor([self window], &titlebarColor);
  if (titlebarColor) {
    [titlebarColor set];
    NSRectFillUsingOperation([self titlebarRect], NSCompositeSourceIn);
  }

  // Fill the titlebar with the Songbird titlebar image.  Only draw where the
  // titlebar is opaque to preserve rounded corners.  Draw using
  // NSCompositeSourceAtop so that a translucent gradient image may be applied
  // over the titlebar color.
  NSImage* titlebarImage = nil;
  GetSBTitlebarImage([self window], &titlebarImage);
  if (titlebarImage) {
    // Set the image pattern to start at the top of the titlebar.
    NSGraphicsContext* currentContext = [NSGraphicsContext currentContext];
    float titlebarTop = [self titlebarRect].origin.y +
                        [self titlebarRect].size.height;
    [currentContext saveGraphicsState];
    [currentContext setPatternPhase: NSMakePoint(0, titlebarTop)];

    // Fill titlebar with the image.
    [[NSColor colorWithPatternImage: titlebarImage] set];
    NSRectFillUsingOperation([self titlebarRect], NSCompositeSourceAtop);

    // Restore the graphics state.
    [currentContext restoreGraphicsState];
  }

  // Get the titlebar text color.
  NSColor* textColor = nil;
  GetSBTitlebarTextColor([self window], &textColor);

  // Draw the titlebar text in the Songbird color.
  if (textColor) {
    // Get the default sized titlebar font.
    NSFont *titleFont = [NSFont titleBarFontOfSize:0];

    // Get the titlebar paragraph style.
    NSMutableParagraphStyle
      *paraStyle = [[[NSMutableParagraphStyle alloc] init] autorelease];
    [paraStyle setParagraphStyle:[NSParagraphStyle defaultParagraphStyle]];
    [paraStyle setAlignment:NSCenterTextAlignment];
    [paraStyle setLineBreakMode:NSLineBreakByTruncatingTail];

    // Set up the title attributes.
    NSMutableDictionary *titleAttrs =
      [NSMutableDictionary dictionaryWithObjectsAndKeys:
        titleFont, NSFontAttributeName,
        textColor, NSForegroundColorAttributeName,
        [[paraStyle copy] autorelease], NSParagraphStyleAttributeName,
        nil];

    // Draw the title.
    [[[self window] title] drawInRect: [self _titlebarTitleRect]
                       withAttributes: titleAttrs];
  }
}

- (void)_sbOrigDrawRect:(NSRect)aDirtyRect
{
}

@end


//------------------------------------------------------------------------------
//
// Internal Songbird Mac window titlebar services.
//
//------------------------------------------------------------------------------

/**
 * Return in aDOMWindow the DOM window corresponding to the native window
 * specified by aNativeWindow.
 *
 * \param aNativeWindow         Native window for which to find corresponding
 *                              DOM window.
 * \param aDOMWindow            Returned DOM window.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No DOM window corresponds to the native window.
 */

nsresult
GetDOMWindowFromNativeWindow(NATIVEWINDOW   aNativeWindow,
                             nsIDOMWindow** aDOMWindow)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDOMWindow);

  // Function variables.
  nsresult rv;

  // Get the list of DOM windows.
  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  nsCOMPtr<nsIWindowWatcher>
    windowWatcher = do_GetService("@mozilla.org/embedcomp/window-watcher;1",
                                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = windowWatcher->GetWindowEnumerator(getter_AddRefs(windowEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search for the DOM window corresponding to the native window.
  while (1) {
    // Get the next DOM window.  Break from loop if no more windows.
    nsCOMPtr<nsISupports> domWindow;
    PRBool                hasMoreElements;
    rv = windowEnumerator->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements)
      break;
    rv = windowEnumerator->GetNext(getter_AddRefs(domWindow));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the native window corresponding to the DOM window.
    NATIVEWINDOW nativeWindow = NativeWindowFromNode::get(domWindow);

    // Return with result if DOM window corresponds to the specified native
    // window.
    if (nativeWindow == aNativeWindow) {
      rv = CallQueryInterface(domWindow, aDOMWindow);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}


/**
 * Return in aColor the Songbird color that should be used for the titlebar for
 * the window specified by aWindow.  The color depends upon whether or not the
 * window is the key window.
 *
 * \param aWindow               Window for which to get titlebar color.
 * \param aColor                Returned titlebar color.
 */

nsresult
GetSBTitlebarColor(NSWindow* aWindow,
                   NSColor** aColor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aColor);

  // Function variables.
  nsresult rv;

  // Get the titlebar color.
  if ([aWindow isKeyWindow]) {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("activetitlebarcolor"),
                            aColor);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("inactivetitlebarcolor"),
                            aColor);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Return in aImage the Songbird image that should be used for the titlebar for
 * the window specified by aWindow.  The image depends upon whether or not the
 * window is the key window.
 *
 * \param aWindow               Window for which to get titlebar image.
 * \param aImage                Returned titlebar image.
 */

nsresult
GetSBTitlebarImage(NSWindow* aWindow,
                   NSImage** aImage)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aImage);

  // Function variables.
  nsresult rv;

  // Get the titlebar image.
  if ([aWindow isKeyWindow]) {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("activetitlebarimage"),
                            aImage);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("inactivetitlebarimage"),
                            aImage);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Return in aColor the Songbird color that should be used for the titlebar text
 * for the window specified by aWindow.  The color depends upon whether or not
 * the window is the key window.
 *
 * \param aWindow               Window for which to get titlebar text color.
 * \param aColor                Returned titlebar text color.
 */

nsresult
GetSBTitlebarTextColor(NSWindow* aWindow,
                       NSColor** aColor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aColor);

  // Function variables.
  nsresult rv;

  // Get the titlebar color.
  if ([aWindow isKeyWindow]) {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("activetitlebartextcolor"),
                            aColor);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = GetWindowAttribute(aWindow,
                            NS_LITERAL_STRING("inactivetitlebartextcolor"),
                            aColor);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Return in aAttributeValue the value string for the attribute with the name
 * specified by aAttributeName for the window specified by aWindow.
 *
 * \param aWindow               Window for which to get attribute value.
 * \param aAttributeName        Name of attribute to get.
 * \param aAttributeValue       Returned attribute value.
 */

nsresult
GetWindowAttribute(NSWindow*        aWindow,
                   const nsAString& aAttributeName,
                   nsAString&       aAttributeValue)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);

  // Function variables.
  nsresult rv;

  // Default to no attribute value.
  aAttributeValue.Truncate();

  // Get the DOM window from the native window.  Return no value if no DOM
  // window.
  nsCOMPtr<nsIDOMWindow> domWindow;
  rv = GetDOMWindowFromNativeWindow(aWindow, getter_AddRefs(domWindow));
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_OK;
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the DOM window document element.
  nsCOMPtr<nsIDOMElement>  documentElement;
  nsCOMPtr<nsIDOMDocument> document;
  rv = domWindow->GetDocument(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = document->GetDocumentElement(getter_AddRefs(documentElement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the attribute.
  rv = documentElement->GetAttribute(aAttributeName, aAttributeValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aColor the color value for the attribute with the name specified by
 * aAttributeName for the window specified by aWindow.
 *
 * \param aWindow               Window for which to get attribute value.
 * \param aAttributeName        Name of attribute to get.
 * \param aColor                Returned attribute color value.
 */

nsresult
GetWindowAttribute(NSWindow*        aWindow,
                   const nsAString& aAttributeName,
                   NSColor**        aColor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aColor);

  // Function variables.
  nsresult rv;

  // Default to no color.
  *aColor = nil;

  // Get the color string.
  nsAutoString colorString;
  rv = GetWindowAttribute(aWindow, aAttributeName, colorString);
  NS_ENSURE_SUCCESS(rv, rv);
  if (colorString.IsEmpty())
    return NS_OK;

  // Convert the color string to an nscolor.
  nscolor color = 0;
  if (!SBParseColor(colorString, &color))
    return NS_OK;

  // Return results.
  *aColor = [NSColor colorWithDeviceRed:NS_GET_R(color)/255.0
                                  green:NS_GET_G(color)/255.0
                                   blue:NS_GET_B(color)/255.0
                                  alpha:NS_GET_A(color)/255.0];

  return NS_OK;
}


/**
 * Return in aImage the image value for the attribute with the name specified by
 * aAttributeName for the window specified by aWindow.
 *
 * \param aWindow               Window for which to get attribute value.
 * \param aAttributeName        Name of attribute to get.
 * \param aImage                Returned attribute image value.
 */

nsresult
GetWindowAttribute(NSWindow*        aWindow,
                   const nsAString& aAttributeName,
                   NSImage**        aImage)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aImage);

  // Function variables.
  nsresult rv;

  // Default to no image.
  *aImage = nil;

  // Get the image URL.
  nsAutoString imageURL;
  rv = GetWindowAttribute(aWindow, aAttributeName, imageURL);
  NS_ENSURE_SUCCESS(rv, rv);
  if (imageURL.IsEmpty())
    return NS_OK;

  // Return results.
  *aImage = GetImageFromURL(imageURL);

  return NS_OK;
}


/**
 * Return an image object read from the image URL specified by aImageURL.
 *
 * \param aImageURL             URL of image.
 *
 * \return                      Image object.
 */

NSImage*
GetImageFromURL(const nsAString& aImageURL)
{
  nsresult rv;

  // Get the image URI.
  nsCOMPtr<nsIURI> imageURI;
  rv = NS_NewURI(getter_AddRefs(imageURI), aImageURL);
  NS_ENSURE_SUCCESS(rv, nil);

  // Open an image input stream.
  nsCOMPtr<nsIInputStream> sourceStream;
  rv = NS_OpenURI(getter_AddRefs(sourceStream), imageURI);
  NS_ENSURE_SUCCESS(rv, nil);
  sbAutoInputStream autoSourceStream(sourceStream);

  // Read the image data.
  nsCString data;
  rv = sbConsumeStream(sourceStream, 0xFFFFFFFF, data);
  NS_ENSURE_SUCCESS(rv, nil);

  // Create the image object.
  NSData *dataWrapper = [NSData dataWithBytes:data.BeginReading()
                                       length:data.Length()];
  NSImage *image = [[[NSImage alloc] initWithData:dataWrapper] autorelease];

  return image;
}


/**
 * Parse the color string specified by aColorString and return the parsed color
 * in aColor.  See Mozilla's nsAttrValue::ParseColor.
 *
 * \param aColorString          Color string to be parsed.
 * \param aColor                Returned parsed color string.
 *
 * \return PR_TRUE              Whether the string could be parsed to a color.
 */

PRBool
SBParseColor(const nsAString& aColorString,
             nscolor*         aColor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aColor);

  // Function variables.
  nsresult rv;

  // Get the color string and remove the leading "#".
  nsAutoString colorString(aColorString);
  colorString.Trim("#", PR_TRUE, PR_FALSE);

  // Convert color string to an integer.
  PRUint32 color32 = colorString.ToInteger(&rv, 16);
  if (NS_FAILED(rv))
    return PR_FALSE;

  // Return results.
  *aColor = NS_RGBA((color32 >> 16) & 0xFF,
                    (color32 >> 8) & 0xFF,
                    color32 & 0xFF,
                    255);

  return PR_TRUE;
}

