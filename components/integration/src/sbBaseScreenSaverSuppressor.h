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

#ifndef __SB_BASE_SCREEN_SAVER_SUPPRESSOR_H__
#define __SB_BASE_SCREEN_SAVER_SUPPRESSOR_H__

#include "sbIScreenSaverSuppressor.h"

#include <nsIObserver.h>
#include <nsAutoLock.h>

class nsIComponentManager;
class nsIFile;

struct nsModuleComponentInfo;

class sbBaseScreenSaverSuppressor : public sbIScreenSaverSuppressor,
                                    public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBISCREENSAVERSUPPRESSOR

  sbBaseScreenSaverSuppressor();
  virtual ~sbBaseScreenSaverSuppressor();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  nsresult Init();

  virtual nsresult OnSuppress(PRBool aSuppress) = 0;

protected:
  PRInt32       mSuppress;
};

#define SB_BASE_SCREEN_SAVER_SUPPRESSOR_DESC \
  "Songbird Screen Saver Suppressor Service"
#define SB_BASE_SCREEN_SAVER_SUPPRESSOR_CLASSNAME \
  "sbScreenSaverSuppressor"
#define SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID \
  "@songbirdnest.com/Songbird/ScreenSaverSuppressor;1"
#define SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID \
{ 0xe3247f9, 0xa111, 0x4d2a, { 0x86, 0x9a, 0x4, 0x39, 0x66, 0x10, 0x8d, 0x98 } }

#endif // __SB_BASE_SCREEN_SAVER_SUPPRESSOR_H__
