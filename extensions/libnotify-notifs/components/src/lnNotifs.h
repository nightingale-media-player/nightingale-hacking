/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#ifndef _LN_NOTIFS_H_
#define _LN_NOTIFS_H_

#include "ILNNotifs.h"
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <nsCOMPtr.h>
#include <nsIObserverService.h>
#include <nsServiceManagerUtils.h>

#define LN_NOTIFS_CONTRACTID "@getnightingale.com/Nightingale/lnNotifs;1"
#define LN_NOTIFS_CLASSNAME  "lnNotifs"

/* 649f3733-12fd-485b-bd55-5106b6dafb80 */
#define LN_NOTIFS_CID \
{ 0x649f3733, 0x12fd, 0x485b, \
{ 0xbd, 0x55, 0x51, 0x06, 0xb6, 0xda, 0xfb, 0x80 } }

class lnNotifs : public ILNNotifs
{
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_ILNNOTIFS

        lnNotifs();

    private:
        virtual ~lnNotifs();

    protected:
        bool notifsEnabled;
        bool unitySoundMenuAction;
        NotifyNotification *notification;
        nsCOMPtr<nsIObserverService> observerService;
};

#endif // _LN_NOTIFS_H_
