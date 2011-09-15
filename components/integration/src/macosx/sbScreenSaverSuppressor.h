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

#ifndef _SB_SCREEN_SAVER_SUPPRESSOR_H_
#define _SB_SCREEN_SAVER_SUPPRESSOR_H_

#include "../sbBaseScreenSaverSuppressor.h"

#include <nsITimer.h>

class sbScreenSaverSuppressor : public sbBaseScreenSaverSuppressor
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  sbScreenSaverSuppressor();
  virtual ~sbScreenSaverSuppressor();

  virtual nsresult OnSuppress(PRBool aSuppress);

  static void UpdateSystemActivityCallback(nsITimer* aTimer,
                                           void*     aClosure);

  // 30 second period per
  // "http://developer.apple.com/mac/library/qa/qa2004/qa1160.html".
  static const PRUint32 UPDATE_SYSTEM_ACTIVITY_PERIOD = 30 * 1000;

  nsCOMPtr<nsITimer> mUpdateSystemActivityTimer;
};

#endif // _SB_SCREEN_SAVER_SUPPRESSOR_H_

