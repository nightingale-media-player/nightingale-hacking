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

#ifndef __SB_REMOTENOTIFICATIONMANAGER_H__
#define __SB_REMOTENOTIFICATIONMANAGER_H__

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <nsITimer.h>

class nsIStringBundle;
class sbILibrary;
class sbIDataRemote;

class sbRemoteNotificationManager : public nsITimerCallback
{
public:
  enum ActionType {
    eNone                 = 0,
    eDownload             = 1,
    eUpdatedWithItems     = 2,
    eUpdatedWithPlaylists = 3,
    eEditedItems          = 4,
    eEditedPlaylist       = 5
  };

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  sbRemoteNotificationManager();
  ~sbRemoteNotificationManager();

  nsresult Init();
  nsresult Cancel();

  nsresult Action(ActionType aType, sbILibrary* aLibrary);

private:
  struct ListItem {
    ListItem() :
      mDisplayUntilTime(0) {}
    nsString mLibraryName;
    PRTime mDisplayUntilTime;
  };

  nsresult UpdateStatus();

  nsCOMPtr<nsIStringBundle> mBundle;
  nsTArray<ListItem> mPriorityList;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<sbIDataRemote> mDataRemote;
  ActionType mCurrentActionType;
  nsCOMPtr<sbILibrary> mMainLibrary;
  PRBool mCancelPending;
};

#endif // __SB_REMOTENOTIFICATIONMANAGER_H__

