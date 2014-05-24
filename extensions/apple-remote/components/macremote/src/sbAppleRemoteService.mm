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

#include "sbAppleRemoteService.h"

#import "SBAppleRemoteListener.h"
#include "nsCOMPtr.h"
#include "sbIMediacoreManager.h"
#include "sbIMediacoreStatus.h"
#include "sbIApplicationController.h"


#define MAX_VOLUME  1

@interface AppleRemoteDelegate : NSObject
{
  SBAppleRemoteListener* mAppleRemote;
  sbAppleRemoteService *mOwner;  // weak
}

- (id)initWithCallback:(sbAppleRemoteService *)aCallback;

- (void)startListening;
- (void)stopListening;

@end


@implementation AppleRemoteDelegate

- (id)initWithCallback:(sbAppleRemoteService *)aCallback
{
  if ((self = [super init])) {
    mAppleRemote = [[SBAppleRemoteListener alloc] initWithDelegate: self];
    mOwner = aCallback;
  }
  
  return self;
}

- (void)dealloc
{
  [mAppleRemote release];
  [super dealloc];
}

- (void)startListening
{
  [mAppleRemote startListening];
}

- (void)stopListening
{
  [mAppleRemote stopListening];
}

- (void)onButtonPressed:(EAppleRemoteButton)aButton isHold:(BOOL)aIsHold
{
  // Process this event right away
  if (!aIsHold) {
    switch (aButton) {
      case ePlusButton:
        mOwner->OnVolumeUpPressed();
        break;
      case eMinusButton:
        mOwner->OnVolumeDownPressed();
        break;
      case eMenuButton:
        mOwner->OnMenuButtonPressed();
        break;
      case ePlayButton:
        mOwner->OnPlayButtonPressed();
        break;
      case eRightButton:
        mOwner->OnNextTrackReleased();
        break;
      case eLeftButton:
        mOwner->OnPrevTrackReleased();
        break;      
    }
  }
  else {
  }
}

- (void)onButtonReleased:(EAppleRemoteButton)aButton
{
  // TODO: support this...
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
  [mDelegate startListening];
  return NS_OK;
}

NS_IMETHODIMP 
sbAppleRemoteService::StopListening()
{
  [mDelegate stopListening];
  return NS_OK;
}

NS_IMETHODIMP
sbAppleRemoteService::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  *aIsSupported = [SBAppleRemoteListener isRemoteAvailable];
  return NS_OK;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPlayButtonPressed()
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
sbAppleRemoteService::OnMenuButtonPressed()
{
  // XXXkreeger Show the library?
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnVolumeUpPressed()
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
sbAppleRemoteService::OnVolumeDownPressed()
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
sbAppleRemoteService::OnNextTrackPressed()
{
  // TODO: Support scanning...
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnNextTrackReleased()
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  return sequencer->Next(false);
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPrevTrackPressed()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbAppleRemoteService::OnPrevTrackReleased()
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

