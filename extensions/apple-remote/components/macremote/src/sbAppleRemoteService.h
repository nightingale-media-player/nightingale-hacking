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

#ifndef sbAppleRemoteService_h_
#define sbAppleRemoteService_h_

#include <nsCOMArray.h>
#include <nsServiceManagerUtils.h>
#include "sbIMacRemoteControl.h"
#include "sbIMediacorePlaybackControl.h"
#include "sbIMediacoreSequencer.h"
#include "sbIMediacoreVolumeControl.h"

@class AppleRemoteDelegate;

//
// Apple Remote control service implementation:
//
class sbAppleRemoteService : public sbIAppleRemoteService
{
public:
  sbAppleRemoteService();
  virtual ~sbAppleRemoteService();
  
  NS_DECL_ISUPPORTS
  NS_DECL_SBIAPPLEREMOTESERVICE
  
  NS_IMETHOD OnPlayButton();
  NS_IMETHOD OnMenuButton();
  NS_IMETHOD OnVolumeUp();
  NS_IMETHOD OnVolumeDown();
  NS_IMETHOD OnNextTrack();
  NS_IMETHOD OnNextTrackHold();
  NS_IMETHOD OnPrevTrack();
  NS_IMETHOD OnPrevTrackHold();

protected:
  NS_IMETHOD GetPlaybackControl(sbIMediacorePlaybackControl **aPlaybackControl);
  NS_IMETHOD GetSequencer(sbIMediacoreSequencer **aSequencer);
  NS_IMETHOD GetVolumeControl(sbIMediacoreVolumeControl **aVolControl);
  NS_IMETHOD GetIsPlaybackPlaying(PRBool *aIsPlaying);

private:
  AppleRemoteDelegate *mDelegate;  // strong
};

#endif  // sbAppleRemoteService_h_
