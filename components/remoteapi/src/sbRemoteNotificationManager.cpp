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

#include "sbRemoteNotificationManager.h"

#include <nsIStringBundle.h>
#include <nsAutoPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>

#include <sbIDataRemote.h>
#include <sbILibrary.h>
#include <sbILibraryManager.h>

#define SB_DATAREMOTE_FACEPLATE_STATUS \
  NS_LITERAL_STRING("faceplate.status.override.text")
#define TIMER_RESOLUTION 500
#define MAX_NOTIFICATION_TIME 1 * PR_USEC_PER_SEC

#define BUNDLE_URL "chrome://songbird/locale/songbird.properties"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteNotificationManager:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteNotificationManagerLog = nsnull;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gRemoteNotificationManagerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gRemoteNotificationManagerLog, PR_LOG_WARN, args)

NS_IMPL_ISUPPORTS1(sbRemoteNotificationManager, nsITimerCallback)

sbRemoteNotificationManager::sbRemoteNotificationManager() :
  mCurrentActionType(eNone)
{
#ifdef PR_LOGGING
  if (!gRemoteNotificationManagerLog) {
    gRemoteNotificationManagerLog = PR_NewLogModule("sbRemoteNotificationManager");
  }
#endif
  TRACE(("sbRemoteNotificationManager[0x%.8x] - Constructed", this));
}

sbRemoteNotificationManager::~sbRemoteNotificationManager()
{
  TRACE(("sbRemoteNotificationManager[0x%.8x] - Destructed", this));
}

nsresult
sbRemoteNotificationManager::Init()
{
  TRACE(("sbRemoteNotificationManager[0x%.8x] - Init", this));
  nsresult rv;

  PRBool success = mPriorityList.SetLength(eEditedPlaylist + 1);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mDataRemote =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemote->Init(SB_DATAREMOTE_FACEPLATE_STATUS, EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundleService> stringBundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stringBundleService->CreateBundle(BUNDLE_URL, getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libManager->GetMainLibrary(getter_AddRefs(mMainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbRemoteNotificationManager::Cancel()
{
  if (!mTimer) {
    return NS_OK;
  }

  nsRefPtr<sbRemoteNotificationManager> kungFuDeathGrip(this);
  nsresult rv, rv2;
  // clear the status bar text
  mCurrentActionType = eNone;
  rv = UpdateStatus();
  
  // and clear the timer (regardless of if the status bar was cleared)
  // keep the return value so we always return the worst one
  rv2 = mTimer->Cancel();
  mTimer = nsnull;
  
  NS_ENSURE_SUCCESS(rv, rv);
  return rv2;
}

nsresult
sbRemoteNotificationManager::Action(ActionType aType, sbILibrary* aLibrary)
{
  TRACE(("sbRemoteNotificationManager[0x%.8x] - Action (%d, 0x%.8x)",
         this, aType, aLibrary));

  nsresult rv;

  if (aType < eDownload || aType > eEditedPlaylist) {
    return NS_ERROR_INVALID_ARG;
  }

  nsString libraryName;
  if (aLibrary) {
    // Only include library-specific notifications from the main library
    PRBool isMainLibrary;
    rv = mMainLibrary->Equals(aLibrary, &isMainLibrary);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isMainLibrary) {
      return NS_OK;
    }

    rv = aLibrary->GetName(libraryName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update this action
  mPriorityList[aType].mLibraryName = libraryName;
  mPriorityList[aType].mDisplayUntilTime = PR_Now() + MAX_NOTIFICATION_TIME;

  // If this action has a higher priorty than the currently displaying action,
  // make this the new current action and update the status immediately
  if (aType < mCurrentActionType) {
    mCurrentActionType = aType;
    rv = UpdateStatus();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Start up the timer that is used to cycle through the message list
  if (!mTimer) {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mTimer->InitWithCallback(this,
                                  TIMER_RESOLUTION,
                                  nsITimer::TYPE_REPEATING_SLACK);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbRemoteNotificationManager::Notify(nsITimer* aTimer)
{
  TRACE(("sbRemoteNotificationManager[0x%.8x] - Notify", this));

  NS_ENSURE_ARG_POINTER(aTimer);

  nsresult rv;
  PRTime now = PR_Now();

  // If we're currently showing a message, check to see if it has been up
  // long enough
  if (mCurrentActionType > eNone) {
    if (now > mPriorityList[mCurrentActionType].mDisplayUntilTime) {
      mPriorityList[mCurrentActionType].mDisplayUntilTime = 0;
    }
    else {
      // The current message still has time left, do nothing
      return NS_OK;
    }
  }

  // Is there another message waiting to be displayed?  If so, bump the
  // display until time, update status message and return
  for (ActionType i = eDownload; i <= eEditedPlaylist; i = ActionType(i + 1)) {
    if (mPriorityList[i].mDisplayUntilTime > 0) {
      mCurrentActionType = i;
      mPriorityList[i].mDisplayUntilTime = now + MAX_NOTIFICATION_TIME;
      rv = UpdateStatus();
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }
  }

  // If we get here, there are no messages to display, so cancel the timer.
  return Cancel();
}

nsresult
sbRemoteNotificationManager::UpdateStatus()
{
  TRACE(("sbRemoteNotificationManager[0x%.8x] - UpdateStatus", this));

  nsresult rv;
  nsString key;
  nsString message;

  switch(mCurrentActionType) {
    case eDownload:
      key.AssignLiteral("rapi.notification.download");
    break;
    case eUpdatedWithItems:
      key.AssignLiteral("rapi.notification.updateditems");
    break;
    case eUpdatedWithPlaylists:
      key.AssignLiteral("rapi.notification.updatedplaylists");
    break;
    case eEditedItems:
      key.AssignLiteral("rapi.notification.editeditems");
    break;
    case eEditedPlaylist:
      key.AssignLiteral("rapi.notification.editedplaylists");
    break;
    default:
      // Use a blank message
    break;
  }

  if (!key.IsEmpty()) {
    nsString libraryName = mPriorityList[mCurrentActionType].mLibraryName;
    const PRUnichar* strings[1] = { libraryName.get() };
    rv = mBundle->FormatStringFromName(key.BeginReading(),
                                       strings,
                                       1,
                                       getter_Copies(message));
    if (NS_FAILED(rv)) {
      message = key;
    }
  }

  rv = mDataRemote->SetStringValue(message);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

