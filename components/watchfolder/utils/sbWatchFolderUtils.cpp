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

#include "sbWatchFolderUtils.h"

#include <sbIWatchFolderService.h>

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <sbDebugUtils.h>

//==============================================================================
// sbAutoIgnoreWatchFolderPath implementation
//==============================================================================

NS_IMPL_THREADSAFE_ISUPPORTS0(sbAutoIgnoreWatchFolderPath)

sbAutoIgnoreWatchFolderPath::sbAutoIgnoreWatchFolderPath()
  : mWFService(nsnull)
  , mIsIgnoring(PR_FALSE)
{
}

sbAutoIgnoreWatchFolderPath::~sbAutoIgnoreWatchFolderPath()
{
  // If the path was watched, it is now time to clean up and stop ignoring
  // the watch path.
  if (mIsIgnoring) {
    nsresult SB_UNUSED_IN_RELEASE(rv) = mWFService->RemoveIgnorePath(mWatchPath);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not remove a file path from the watchfolders ignore list!");
  }
}

nsresult
sbAutoIgnoreWatchFolderPath::Init(nsAString const & aWatchPath)
{
  mWatchPath = aWatchPath;

  nsresult rv;
  mWFService = do_GetService("@songbirdnest.com/watch-folder-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isRunning = PR_FALSE;
  rv = mWFService->GetIsRunning(&isRunning);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the watchfolder service is not running, don't bother appending to the
  // ignore path list.
  if (!isRunning) {
    return NS_OK;
  }

  rv = mWFService->AddIgnorePath(mWatchPath);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsIgnoring = PR_TRUE;
  return NS_OK;
}
