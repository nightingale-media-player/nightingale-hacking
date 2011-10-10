/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#include "sbScreenSaverSuppressor.h"

#include <CoreServices/CoreServices.h>

NS_IMPL_ISUPPORTS_INHERITED0(sbScreenSaverSuppressor,
                             sbBaseScreenSaverSuppressor)

sbScreenSaverSuppressor::sbScreenSaverSuppressor()
{
}

sbScreenSaverSuppressor::~sbScreenSaverSuppressor()
{
  if (mUpdateSystemActivityTimer) {
    mUpdateSystemActivityTimer->Cancel();
    mUpdateSystemActivityTimer = nsnull;
  }
}

/*virtual*/ nsresult
sbScreenSaverSuppressor::OnSuppress(PRBool aSuppress)
{
  nsresult rv;

  if (aSuppress) {
    if (!mUpdateSystemActivityTimer) {
      mUpdateSystemActivityTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mUpdateSystemActivityTimer->InitWithFuncCallback
                                         (UpdateSystemActivityCallback,
                                          this,
                                          UPDATE_SYSTEM_ACTIVITY_PERIOD,
                                          nsITimer::TYPE_REPEATING_SLACK);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    if (mUpdateSystemActivityTimer) {
      rv = mUpdateSystemActivityTimer->Cancel();
      NS_ENSURE_SUCCESS(rv, rv);
      mUpdateSystemActivityTimer = nsnull;
    }
  }

  return NS_OK;
}

/*static*/ void
sbScreenSaverSuppressor::UpdateSystemActivityCallback(nsITimer* aTimer,
                                                      void*     aClosure)
{
  // Update system activity to suppress the screen saver.
  UpdateSystemActivity(UsrActivity);
}

