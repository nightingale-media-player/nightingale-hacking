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
// HIDRemote Class created by Felix Schwarz on 06.04.07.
//  Copyright 2007-2015 IOSPIRIT GmbH. All rights reserved.
//
//  The latest version of this class is available at
//     http://www.iospirit.com/developers/hidremote/
//
//  ** LICENSE *************************************************************************
//
//  Copyright (c) 2007-2014 IOSPIRIT GmbH (http://www.iospirit.com/)
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//  * Redistributions of source code must retain the above copyright notice, this list
//    of conditions and the following disclaimer.
//  
//  * Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//  
//  * Neither the name of IOSPIRIT GmbH nor the names of its contributors may be used to
//    endorse or promote products derived from this software without specific prior
//    written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//
//  ************************************************************************************

//  ************************************************************************************
//  ********************************** DOCUMENTATION ***********************************
//  ************************************************************************************
//
//  - a reference is available at http://www.iospirit.com/developers/hidremote/reference/
//  - for a guide, please see http://www.iospirit.com/developers/hidremote/guide/
//
//  ************************************************************************************
*/

#include "sbAppleRemoteService.h"

#import "HIDRemote.h"
#import <Cocoa/Cocoa.h>
#include "nsCOMPtr.h"
#include "sbIMediacoreManager.h"
#include "sbIMediacoreStatus.h"
#include "sbIApplicationController.h"


#define MAX_VOLUME  1

@interface AppleRemoteDelegate : NSObject <HIDRemoteDelegate>
{
  HIDRemote* mAppleRemote;
  sbAppleRemoteService *mOwner;  // weak
  HIDRemote			 *hidRemote;
}

- (id)initWithCallback:(sbAppleRemoteService *)aCallback;
- (void)dealloc;
- (BOOL)startRemoteControl;
- (void)stopRemoteControl;
- (void)hidRemote:(HIDRemote *)hidRemote eventWithButton:(HIDRemoteButtonCode)buttonCode isPressed:(BOOL)isPressed fromHardwareWithAttributes:(NSMutableDictionary *)attributes;

@end


@implementation AppleRemoteDelegate

- (id)initWithCallback:(sbAppleRemoteService *)aCallback
{
  if ((self = [super init])) {
    mAppleRemote = [[HIDRemote alloc] init];
    // Our application doesn't need these  button codes. Share this information with everybody else
      [mAppleRemote setUnusedButtonCodes:[NSArray arrayWithObjects:
		                                                  [NSNumber numberWithInt:(int)kHIDRemoteButtonCodeMenuHold],
		                                                  [NSNumber numberWithInt:(int)kHIDRemoteButtonCodeUpHold],
		                                                  [NSNumber numberWithInt:(int)kHIDRemoteButtonCodeDownHold],
		                                                  [NSNumber numberWithInt:(int)kHIDRemoteButtonCodeCenterHold],
		                                                  [NSNumber numberWithInt:(int)kHIDRemoteButtonCodePlayHold],
														 nil]
	  ];
    mOwner = aCallback;
  }
  
  return self;
}

- (void)dealloc
{
  [mAppleRemote stopRemoteControl];
  [mAppleRemote setDelegate:nil];
  [mAppleRemote release];
  [super dealloc];
}

#pragma mark -- Start and stop remote control support --
- (BOOL)startRemoteControl
{
  HIDRemoteMode remoteMode;

  remoteMode = kHIDRemoteModeExclusive;
	
  // Check whether the installation of Candelair is required to reliably operate in this mode
  if ([HIDRemote isCandelairInstallationRequiredForRemoteMode:remoteMode])
  {
    // Reliable usage of the remote in this mode under this operating system version requires the Candelair driver to be installed. Let's inform the user about it.
	//TODO: Support this by implementing an informative pop-up window.
	NSAlert *alert;
		
	if ((alert = [NSAlert alertWithMessageText:NSLocalizedString(@"Candelair driver installation necessary", @"")
		                  defaultButton:NSLocalizedString(@"Download", @"")
			              alternateButton:NSLocalizedString(@"More information", @"")
					      otherButton:NSLocalizedString(@"Cancel", @"")
						  informativeTextWithFormat:NSLocalizedString(@"An additional driver needs to be installed before %@ can reliably access the remote under the OS version installed on your computer.", @""), [[NSBundle mainBundle] objectForInfoDictionaryKey:(id)kCFBundleNameKey]]) != nil)
		{
		  switch ([alert runModal])
		    {
		      case NSAlertDefaultReturn:
	            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.candelair.com/download/"]];
			    break;
			   
		      case NSAlertAlternateReturn:
		        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.candelair.com/"]];
			    break;
		    }
		}
  } 
  else
  { 
    // Candelair is either already installed or not required under this OS release => proceed!
	if (remoteMode == kHIDRemoteModeExclusive)
	  {
	    // When used in exclusive, non-auto mode, enable exclusive lock lending. This isn't required but there are good reasons to do this. 
	    [mAppleRemote setExclusiveLockLendingEnabled:YES];
	  }
		
	  // Start remote control
	  [mAppleRemote setDelegate:self];
	  
	  if ([mAppleRemote startRemoteControl:remoteMode])
		{
		    // Start was successful
			return (YES);
		} 
	  else
		{
		    // Start failed
			//TODO: Support this by implementing an informative pop-up window.
			NSLog(@"Couldn't start HIDRemote");
		}
  }
  return (NO);
}

- (void)stopRemoteControl
{
  [mAppleRemote stopRemoteControl];
}

#pragma mark -- Handle remote control events --
- (void)hidRemote:(HIDRemote *)hidRemote eventWithButton:(HIDRemoteButtonCode)buttonCode isPressed:(BOOL)isPressed fromHardwareWithAttributes:(NSMutableDictionary *)attributes
{
  // Process this event right away
  if (isPressed) {
    switch (buttonCode) {
      case kHIDRemoteButtonCodeUp:
        mOwner->OnVolumeUp();
        break;
      case kHIDRemoteButtonCodeDown:
        mOwner->OnVolumeDown();
        break;
      case kHIDRemoteButtonCodeMenu:
        mOwner->OnMenuButton();
        break;
      case kHIDRemoteButtonCodePlay:
        mOwner->OnPlayButton();
        break;
      case kHIDRemoteButtonCodeCenter:
        mOwner->OnPlayButton();
        break;
      case kHIDRemoteButtonCodeRight:
        mOwner->OnNextTrack();
        break;
      case kHIDRemoteButtonCodeLeft:
        mOwner->OnPrevTrack();
        break;

      default:
        break;		
    }
  }
  else {
  }
}

@end


//////////////////////////////////////////////////////////////////////////////


NS_IMPL_ISUPPORTS1(sbAppleRemoteService, sbIAppleRemoteService)

sbAppleRemoteService::sbAppleRemoteService()
{
  mDelegate = [[AppleRemoteDelegate alloc] initWithCallback:this];
}

sbAppleRemoteService::~sbAppleRemoteService()
{
  [mDelegate release];
}

NS_IMETHODIMP
sbAppleRemoteService::StartListening()
{
  [mDelegate startRemoteControl];
  return NS_OK;
}

NS_IMETHODIMP 
sbAppleRemoteService::StopListening()
{
  [mDelegate stopRemoteControl];
  return NS_OK;
}

NS_IMETHODIMP
sbAppleRemoteService::GetIsCandelairRequired(PRBool *aIsCandelairRequired)
{
  NS_ENSURE_ARG_POINTER(aIsCandelairRequired);
  *aIsCandelairRequired = [HIDRemote isCandelairInstallationRequiredForRemoteMode:(HIDRemoteMode)kHIDRemoteModeExclusive];
  return NS_OK;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPlayButton()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager = 
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
  rv = manager->GetPlaybackControl(getter_AddRefs(playbackControl));

  PRBool isPlaying = PR_FALSE;
  GetIsPlaybackPlaying(&isPlaying);

  if (NS_SUCCEEDED(rv) && playbackControl && isPlaying) {
    nsCOMPtr<sbIMediacoreStatus> status;
    rv = manager->GetStatus(getter_AddRefs(status));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 state;
    rv = status->GetState(&state);
    NS_ENSURE_SUCCESS(rv, NO);

    if (state == sbIMediacoreStatus::STATUS_PLAYING ||
        state == sbIMediacoreStatus::STATUS_BUFFERING)
    {
      rv = playbackControl->Pause();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    nsCOMPtr<sbIApplicationController> appController =
     do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = appController->PlayDefault();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnMenuButton()
{
  // XXXkreeger Show the library?
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnVolumeUp()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreVolumeControl> volControl;
  rv = GetVolumeControl(getter_AddRefs(volControl));
  NS_ENSURE_SUCCESS(rv, rv);

  double curVolume;
  rv = volControl->GetVolume(&curVolume);
  NS_ENSURE_SUCCESS(rv, rv);
  
  curVolume += 0.05;
  if (curVolume > MAX_VOLUME)
    curVolume = MAX_VOLUME;

  return volControl->SetVolume(curVolume);
}

NS_IMETHODIMP 
sbAppleRemoteService::OnVolumeDown()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreVolumeControl> volControl;
  rv = GetVolumeControl(getter_AddRefs(volControl));
  NS_ENSURE_SUCCESS(rv, rv);

  double curVolume;
  rv = volControl->GetVolume(&curVolume);
  NS_ENSURE_SUCCESS(rv, rv);

  curVolume -= 0.05;
  if (curVolume < 0.0)
    curVolume = 0;

  return volControl->SetVolume(curVolume);
}

NS_IMETHODIMP 
sbAppleRemoteService::OnNextTrackHold()
{
  // TODO: Support scanning...
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnNextTrack()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  return sequencer->Next(false);
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPrevTrackHold()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPrevTrack()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  return sequencer->Previous(false);
}

NS_IMETHODIMP
sbAppleRemoteService::GetPlaybackControl(sbIMediacorePlaybackControl **aPlayCntrl)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return manager->GetPlaybackControl(aPlayCntrl); 
}

NS_IMETHODIMP
sbAppleRemoteService::GetSequencer(sbIMediacoreSequencer **aSequencer)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return manager->GetSequencer(aSequencer);  
}

NS_IMETHODIMP
sbAppleRemoteService::GetVolumeControl(sbIMediacoreVolumeControl **aVolControl)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return manager->GetVolumeControl(aVolControl);
}

NS_IMETHODIMP
sbAppleRemoteService::GetIsPlaybackPlaying(PRBool *aIsPlaying)
{
  NS_ENSURE_ARG_POINTER(aIsPlaying);

  nsresult rv;
  nsCOMPtr<sbIMediacoreManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreStatus> status;
  rv = manager->GetStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 state;
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsPlaying = (state == sbIMediacoreStatus::STATUS_PLAYING ||
                 state == sbIMediacoreStatus::STATUS_BUFFERING);

  return NS_OK;
}

