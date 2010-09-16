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
#include <nsIStringBundle.h>
#include <nsObjCExceptions.h>
#include <sbIApplicationController.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreVolumeControl.h>
#include <sbIMediacoreStatus.h>
#include <sbStandardProperties.h>
#include <sbIMediaItem.h>

#import <AppKit/AppKit.h>

//-----------------------------------------------------------------------------

@interface NSObject (XRAppDelegate)

- (void)setAppDelegateOverride:(id)aDelegateOverride;

@end

@interface SBMacAppDelegate : NSObject
{
}

// Menu item event handlers:
- (void)onPlayPauseSelected:(id)aSender;
- (void)onNextSelected:(id)aSender;
- (void)onPreviousSelected:(id)aSender;
- (void)onMuteSelected:(id)aSender;

@end

@interface SBMacAppDelegate (Private)

- (void)_setOverride:(id)aOverrideObj;
- (void)_addPlayerControlMenuItems:(NSMenu *)aMenu;
- (void)_appendMenuItem:(NSString *)aTitle
                 action:(SEL)aSelector
                   menu:(NSMenu *)aParentMenu;
- (BOOL)_isPlaybackMuted;
- (BOOL)_isPlaybackPlaying;
- (NSString *)_stringForLocalizedKey:(const PRUnichar *)aBuffer;

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
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  id delegate = [[NSApplication sharedApplication] delegate];
  if (delegate && 
      [delegate respondsToSelector:@selector(setAppDelegateOverride:)]) 
  {
    [delegate setAppDelegateOverride:aOverrideObj];
  }

  NS_OBJC_END_TRY_ABORT_BLOCK;
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
    if (idValue.Equals(NS_LITERAL_STRING("video_osd_controls_win"))) 
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

  [self _addNowPlayingControls:menu];
  [self _addPlayerControlMenuItems:menu];

  return menu;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (void)_addNowPlayingControls:(NSMenu *)aMenu
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = manager->GetSequencer(getter_AddRefs(sequencer));
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediaItem> curMediaItem;
  rv = sequencer->GetCurrentItem(getter_AddRefs(curMediaItem));
  if (NS_FAILED(rv) || !curMediaItem) {
    return;
  }

  nsString trackProp(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME));
  nsString artistProp(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME));

  nsString trackName;
  rv = curMediaItem->GetProperty(trackProp, trackName);
  if (NS_FAILED(rv)) {
    return;
  }

  nsString artistName;
  rv = curMediaItem->GetProperty(artistProp, artistName);
  if (NS_FAILED(rv)) {
    return;
  }

  // Apppend three spaces before the track and artist name
  trackName.Insert(NS_LITERAL_STRING("   "), 0);
  artistName.Insert(NS_LITERAL_STRING("   "), 0);

  NSString *trackStr = 
    [NSString stringWithCharacters:(unichar *)trackName.get()
                            length:trackName.Length()];

  // Now add the menu items
  [aMenu addItem:[NSMenuItem separatorItem]];

  // Now playing item
  nsString nowPlaying(NS_LITERAL_STRING("albumart.displaypane.title.playing"));
  [self _appendMenuItem:[self _stringForLocalizedKey:nowPlaying.get()]
                 action:nil
                   menu:aMenu];

  // Track name
  NSMenuItem *trackNameItem = 
    [[NSMenuItem alloc] initWithTitle:trackStr
                               action:nil
                        keyEquivalent:@""];
  [self _appendMenuItem:trackStr action:nil menu:aMenu];
  
  // Artist name
  NSString *artistStr =
    [NSString stringWithCharacters:(unichar *)artistName.get()
                            length:artistName.Length()];
  [self _appendMenuItem:artistStr action:nil menu:aMenu];

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)_addPlayerControlMenuItems:(NSMenu *)aMenu
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK; 

  [aMenu addItem:[NSMenuItem separatorItem]];

  // Play menu item
  nsString playPauseCommand;
  if ([self _isPlaybackPlaying]) {
    playPauseCommand.AssignLiteral("playback.label.pause");
  }
  else {
    playPauseCommand.AssignLiteral("playback.label.play");
  }
  [self _appendMenuItem:[self _stringForLocalizedKey:playPauseCommand.get()]
                 action:@selector(onPlayPauseSelected:)
                   menu:aMenu];

  // Next menu item
  nsString nextLabel(NS_LITERAL_STRING("playback.label.next"));
  [self _appendMenuItem:[self _stringForLocalizedKey:nextLabel.get()]
                 action:@selector(onNextSelected:)
                   menu:aMenu];

  // Previous menu item
  nsString previousLabel(NS_LITERAL_STRING("playback.label.previous"));
  [self _appendMenuItem:[self _stringForLocalizedKey:previousLabel.get()]
                 action:@selector(onPreviousSelected:)
                   menu:aMenu];

  // Mute menu item
  nsString muteLabel(NS_LITERAL_STRING("playback.label.mute"));
  NSMenuItem *muteMenuItem = 
    [[NSMenuItem alloc] initWithTitle:[self _stringForLocalizedKey:muteLabel.get()]
                               action:@selector(onMuteSelected:)
                        keyEquivalent:@""];

  int muteState = ([self _isPlaybackMuted] ? NSOnState : NSOffState);
  [muteMenuItem setState:muteState];
  [muteMenuItem setTarget:self];
  [aMenu addItem:muteMenuItem];

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)_appendMenuItem:(NSString *)aTitle
                 action:(SEL)aSelector
                   menu:(NSMenu *)aParentMenu
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  NSMenuItem *menuItem = 
    [[NSMenuItem alloc] initWithTitle:aTitle
                               action:aSelector
                        keyEquivalent:@""];
  [menuItem setTarget:self];
  [aParentMenu addItem:menuItem];
  [menuItem release];

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)onPlayPauseSelected:(id)aSender
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
  rv = manager->GetPlaybackControl(getter_AddRefs(playbackControl));
  if (NS_SUCCEEDED(rv) && playbackControl && [self _isPlaybackPlaying]) {
    playbackControl->Pause();
  }
  else {
    nsCOMPtr<sbIApplicationController> app =
      do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
    NS_ENSURE_SUCCESS(rv,);
    rv = app->PlayDefault();
    NS_ENSURE_SUCCESS(rv,);
  }

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)onNextSelected:(id)aSender
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = manager->GetSequencer(getter_AddRefs(sequencer));
  if (NS_FAILED(rv)) {
    return;
  }

  sequencer->Next(PR_FALSE);

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)onPreviousSelected:(id)aSender
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = manager->GetSequencer(getter_AddRefs(sequencer));
  if (NS_FAILED(rv)) {
    return;
  }

  sequencer->Previous(PR_FALSE);

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)onMuteSelected:(id)aSender
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  NSMenuItem *senderItem = (NSMenuItem *)aSender;
  if (!senderItem) {
    return;
  }

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacoreVolumeControl> volControl;
  rv = manager->GetVolumeControl(getter_AddRefs(volControl));
  if (NS_FAILED(rv)) {
    return;
  }

  PRBool isMuted = PR_FALSE;
  rv = volControl->GetMute(&isMuted);
  if (NS_FAILED(rv)) {
    return;
  }

  if ([self _isPlaybackMuted]) {
    volControl->SetMute(PR_FALSE);
    [senderItem setState:NSOffState];
  }
  else {
    volControl->SetMute(PR_TRUE);
    [senderItem setState:NSOnState];
  }

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (BOOL)_isPlaybackMuted
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  // Until bug 13295 is fixed, use the |volume| property to determine muted state.
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, NO);

  nsCOMPtr<sbIMediacoreVolumeControl> volControl;
  rv = manager->GetVolumeControl(getter_AddRefs(volControl));
  NS_ENSURE_SUCCESS(rv, NO);

  PRBool isMuted = PR_FALSE;
  rv = volControl->GetMute(&isMuted);
  NS_ENSURE_SUCCESS(rv, NO);

  return isMuted;

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(NO);
}

- (BOOL)_isPlaybackPlaying
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, NO);

  nsCOMPtr<sbIMediacoreStatus> status;
  rv = manager->GetStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, NO);

  PRUint32 state;
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, NO);
  
  return state == sbIMediacoreStatus::STATUS_PLAYING ||
         state == sbIMediacoreStatus::STATUS_BUFFERING;

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(NO);
}

- (NSString *)_stringForLocalizedKey:(const PRUnichar *)aBuffer
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  nsresult rv;
  nsCOMPtr<nsIStringBundleService> strBundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv);
  NS_ENSURE_SUCCESS(rv, @"");

  nsCOMPtr<nsIStringBundle> songbirdBundle;
  rv = strBundleService->CreateBundle("chrome://songbird/locale/songbird.properties",
                                      getter_AddRefs(songbirdBundle));
  NS_ENSURE_SUCCESS(rv, nil);

  nsString localizedStr;
  rv = songbirdBundle->GetStringFromName(aBuffer, getter_Copies(localizedStr));
  NS_ENSURE_SUCCESS(rv, @"");

  return [NSString stringWithCharacters:(unichar *)localizedStr.get()
                                 length:localizedStr.Length()];

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(@"");
}

@end

//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS1(sbMacAppDelegateManager, nsIObserver)

sbMacAppDelegateManager::sbMacAppDelegateManager()
  : mDelegate(nil)
{
}

sbMacAppDelegateManager::~sbMacAppDelegateManager()
{
  if (mDelegate) {
    [mDelegate release];
    mDelegate = nil;
  }
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

