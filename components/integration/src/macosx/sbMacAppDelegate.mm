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

#include "sbMacAppDelegate.h"
#include "../NativeWindowFromNode.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIObserverService.h>
#include <nsIWindowMediator.h>
#include <nsIDOMWindowInternal.h>
#include <nsIDOMWindow.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsStringAPI.h>
#include <nsObjCExceptions.h>

#import <AppKit/AppKit.h>

//-----------------------------------------------------------------------------

@interface NSObject (XRAppDelegate)

- (void)setAppDelegateOverride:(id)aDelegateOverride;

@end

@interface SBMacAppDelegate : NSObject
@end

@interface SBMacAppDelegate (Private)

- (void)_setOverride:(id)aOverrideObj;

@end

@implementation SBMacAppDelegate

- (id)init
{
  if ((self = [super init])) {
    [self _setOverride:self];
  }

  return self;
}

- (void)dealloc
{
  [self _setOverride:nil];
  [super dealloc];
}

- (void)_setOverride:(id)aOverrideObj
{
  id delegate = [[NSApplication sharedApplication] delegate];
  if (delegate && 
      [delegate respondsToSelector:@selector(setAppDelegateOverride:)]) 
  {
    [delegate setAppDelegateOverride:aOverrideObj];
  }
}

// NSApplication delegate methods
- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApp 
                    hasVisibleWindows:(BOOL)flag
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  nsresult rv;
  nsCOMPtr<nsIWindowMediator> winMed =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, NO);

  nsCOMPtr<nsIDOMWindowInternal> domWinInternal;
  rv = winMed->GetMostRecentWindow(NS_LITERAL_STRING("Songbird:Main").get(),
                                   getter_AddRefs(domWinInternal));
  if (NS_SUCCEEDED(rv) && domWinInternal) {
    NSWindow *window = NativeWindowFromNode::get(domWinInternal);
    if (window && [window isMiniaturized]) {
      [window deminiaturize:self];
    }
  }
  
  return NO;  // don't let |NSApplication| do anything else

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(NO);
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  nsresult rv;
  nsCOMPtr<nsIWindowMediator> wm = 
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nil);

  // Get the frontmost window
  nsCOMPtr<nsISimpleEnumerator> orderedWindowList;
  rv = wm->GetZOrderDOMWindowEnumerator(nsnull, PR_TRUE,
                                        getter_AddRefs(orderedWindowList));
  NS_ENSURE_SUCCESS(rv, nil);
  PRBool anyWindows = false;
  rv = orderedWindowList->HasMoreElements(&anyWindows);
  NS_ENSURE_SUCCESS(rv, nil);
  nsCOMPtr<nsISupports> frontWindow;
  rv = orderedWindowList->GetNext(getter_AddRefs(frontWindow));
  NS_ENSURE_SUCCESS(rv, nil);

  // Get our list of windows and prepare to iterate. We use this list, ordered
  // by window creation date, instead of the z-ordered list because that's what
  // native apps do.
  nsCOMPtr<nsISimpleEnumerator> windowList;
  rv = wm->GetEnumerator(nsnull, getter_AddRefs(windowList));
  NS_ENSURE_SUCCESS(rv, nil);

  // Iterate through our list of windows to create our menu
  NSMenu *menu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
  PRBool more;
  while (NS_SUCCEEDED(windowList->HasMoreElements(&more)) && more) {
    // Get our native window
    nsCOMPtr<nsISupports> curWindow;
    rv = windowList->GetNext(getter_AddRefs(curWindow));
    NS_ENSURE_SUCCESS(rv, nil);

    nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(curWindow, &rv);
    if (!domWindow)
      continue;

    // Make sure this isn't the Video Window:
    nsCOMPtr<nsIDOMDocument> document;
    domWindow->GetDocument(getter_AddRefs(document));
    if (!document)
      continue;

    nsCOMPtr<nsIDOMElement> element;
    document->GetDocumentElement(getter_AddRefs(element));
    if (!element)
      continue;

    nsString idValue;
    rv = element->GetAttribute(NS_LITERAL_STRING("id"), idValue);
    if (idValue.Equals(NS_LITERAL_STRING("video_window"))) 
      continue;

    NSWindow *cocoaWindow = NativeWindowFromNode::get(domWindow);
    if (!cocoaWindow) 
      continue;
    
    NSString *windowTitle = [cocoaWindow title];
    if (!windowTitle) 
      continue;

    // Now, create a menu item, and add it to the menu
    NSMenuItem *menuItem = [[NSMenuItem alloc]
                              initWithTitle:windowTitle
                                     action:@selector(dockMenuItemSelected:)
                              keyEquivalent:@""];
    [menuItem setTarget:[[NSApplication sharedApplication] delegate]];
    [menuItem setRepresentedObject:cocoaWindow];

    // If this is the foreground window, put a checkmark next to it
    if (SameCOMIdentity(domWindow, frontWindow))
      [menuItem setState:NSOnState];

    [menu addItem:menuItem];
    [menuItem release];
  }
  return menu;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

@end

//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbMacAppDelegateManager, nsIObserver)

sbMacAppDelegateManager::sbMacAppDelegateManager()
{
}

sbMacAppDelegateManager::~sbMacAppDelegateManager()
{
  if (mDelegate) {
    [mDelegate release];
    mDelegate = nil;
  }

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->RemoveObserver(this, "final-ui-startup");
}

NS_IMETHODIMP
sbMacAppDelegateManager::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "final-ui-startup", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "quit-application-granted", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMacAppDelegateManager::Observe(nsISupports *aSubject,
                                 const char *aTopic,
                                 const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);
  
  if (strcmp(aTopic, "final-ui-startup") == 0) {
    mDelegate = [[SBMacAppDelegate alloc] init];
  }
  else if (strcmp(aTopic, "quit-application-granted") == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "final-ui-startup");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* static */ NS_METHOD
sbMacAppDelegateManager::RegisterSelf(nsIComponentManager *aCompMgr,
                                      nsIFile *aPath,
                                      const char *aLoaderStr,
                                      const char *aType,
                                      const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr = 
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_MACAPPDELEGATEMANAGER_CLASSNAME,
                                "service,"
                                SONGBIRD_MACAPPDELEGATEMANAGER_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);

  return NS_OK;
}
