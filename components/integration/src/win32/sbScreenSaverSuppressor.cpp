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

// Include windows.h first because it's required for nsWindowsRegKey.h
#include <windows.h>

#include <nsWindowsRegKey.h>

#define SCREENSAVER_ENABLED   "1"
#define SCREENSAVER_DISABLED  "0"

NS_IMPL_ISUPPORTS_INHERITED0(sbScreenSaverSuppressor, 
                             sbBaseScreenSaverSuppressor)

sbScreenSaverSuppressor::sbScreenSaverSuppressor() 
{
}

sbScreenSaverSuppressor::~sbScreenSaverSuppressor() 
{
}

/*virtual*/ nsresult
sbScreenSaverSuppressor::OnSuppress(PRBool aSuppress)
{
  NS_NAMED_LITERAL_STRING(regKeyPath, "Control Panel\\Desktop");
  NS_NAMED_LITERAL_STRING(regKeyValueName, "ScreenSaveActive");

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance(NS_WINDOWSREGKEY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER, 
                    regKeyPath, 
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString currentUserValue;
  rv = regKey->ReadStringValue(regKeyValueName, currentUserValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = regKey->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  if(aSuppress) {
    // Only suppress if we need to.
    if(currentUserValue.EqualsLiteral(SCREENSAVER_ENABLED)) {
      BOOL success = SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, 
                                           FALSE, 
                                           NULL, 
                                           0);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    }

    // Push user setting so we can restore it for the login session.
    if(!mHasUserSetting) {
      mHasUserSetting = PR_TRUE;
      mUserSetting = currentUserValue.EqualsLiteral(SCREENSAVER_ENABLED);
    }
  }
  else {
    // No user setting, nothing to restore.
    if(!mHasUserSetting) {
      return NS_OK;
    }

    // Only unsuppress if we need to.
    if(mUserSetting) {
      BOOL success = SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE,
                                           TRUE,
                                           NULL,
                                           0);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

      mHasUserSetting = PR_FALSE;
      mUserSetting = PR_FALSE;
    }
  }

  return NS_OK;
}
