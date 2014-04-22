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

#include "sbAppleMediaKeyController.h"

#include <nsIObserverService.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>

#include <sbIApplicationController.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreVolumeControl.h>
#include <sbIMediacoreStatus.h>

#import <AppKit/AppKit.h>
#import <IOKit/hidsystem/ev_keymap.h>
#import </usr/include/objc/objc-class.h>


//------------------------------------------------------------------------------

void MethodSwizzle(Class aClass, SEL orig_sel, SEL alt_sel)
{
  Method orig_method = nil, alt_method = nil;

  // First, look for methods
  orig_method = class_getInstanceMethod(aClass, orig_sel);
  alt_method = class_getInstanceMethod(aClass, alt_sel);

  // If both are found, swizzle them.
  if ((orig_method != nil) && (alt_method != nil)) {
    char *temp1;
    IMP temp2;

    temp1 = orig_method->method_types;
    orig_method->method_types = alt_method->method_types;
    alt_method->method_types = temp1;

    temp2 = orig_method->method_imp;
    orig_method->method_imp = alt_method->method_imp;
    alt_method->method_imp = temp2;
  }
}

//------------------------------------------------------------------------------

PRBool IsPlaybackPlaying()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsCOMPtr<sbIMediacoreStatus> status;
  rv = manager->GetStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRUint32 state;
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  return state == sbIMediacoreStatus::STATUS_PLAYING ||
         state == sbIMediacoreStatus::STATUS_BUFFERING;
}

void OnPlayPausePressed()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
  rv = manager->GetPlaybackControl(getter_AddRefs(playbackControl));
  if (NS_SUCCEEDED(rv) && playbackControl && IsPlaybackPlaying()) {
    playbackControl->Pause();
  }
  else {
    nsCOMPtr<sbIApplicationController> app =
      do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
    NS_ENSURE_SUCCESS(rv,);
    rv = app->PlayDefault();
    NS_ENSURE_SUCCESS(rv,);
  }
}

void OnPreviousPressed()
{
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
}

void OnNextPressed()
{
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
}

//------------------------------------------------------------------------------

// Add a category to |NSApplication| so that we can swizzle-hack the 
// |sendEvent:| method between our category and the actual implementation.
@interface NSApplication (SongbirdSwizzledEvent)

- (void)sendSongbirdEvent:(NSEvent *)event;

@end

@implementation NSApplication (SongbirdSwizzledEvent)

- (void)sendSongbirdEvent:(NSEvent *)event
{
  // These are the magic bits that are media-key events
  if ([event type] == NSSystemDefined && [event subtype] == 8) {
    int keyCode = (([event data1] & 0xFFFF0000) >> 16);
    int keyFlags = ([event data1] & 0x0000FFFF);
    int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA;

    // Sadly, the media key events get posted 4 times for a key-up/key-down
    // combo. So to prevent making the call 4 times, simply filter out the
    // event until we get the last 'up' event (keyState == 0).
    if (keyState == 0) {
      switch (keyCode) {
        case NX_KEYTYPE_PLAY:
          OnPlayPausePressed();
          break;
        case NX_KEYTYPE_FAST:
        case NX_KEYTYPE_NEXT:
          OnNextPressed();
          break;
        case NX_KEYTYPE_REWIND:
        case NX_KEYTYPE_PREVIOUS:
          OnPreviousPressed();
          break;
      }
    }

    // No need to have the real |NSApplication| process this event.
    return;
  }

  // So un-swizzle - to make the call to the real implementation - 
  // then re-swizzle back to our swizzle'd method.
  MethodSwizzle([NSApplication class],
                @selector(sendSongbirdEvent:),
                @selector(sendEvent:));
  
  [self sendEvent:event];

  // Swizzle back..
  MethodSwizzle([NSApplication class],
                @selector(sendEvent:),
                @selector(sendSongbirdEvent:)); 
}

@end

//------------------------------------------------------------------------------


NS_IMPL_ISUPPORTS1(sbAppleMediaKeyController, nsIObserver)

sbAppleMediaKeyController::sbAppleMediaKeyController()
{
}

sbAppleMediaKeyController::~sbAppleMediaKeyController()
{
}

NS_IMETHODIMP
sbAppleMediaKeyController::Init()
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
sbAppleMediaKeyController::Observe(nsISupports *aSubject,
                                   const char *aTopic,
                                   const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  if (strcmp(aTopic, "final-ui-startup") == 0) {
     MethodSwizzle([NSApplication class],
                    @selector(sendEvent:),
                    @selector(sendSongbirdEvent:));
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
sbAppleMediaKeyController::RegisterSelf(nsIComponentManager *aCompMgr,
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
                                SONGBIRD_APPLEMEDIAKEYCONTROLLER_CLASSNAME,
                                "service,"
                                  SONGBIRD_APPLEMEDIAKEYCONTROLLER_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

