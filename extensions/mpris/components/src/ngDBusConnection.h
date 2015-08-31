/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
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

/*
 * Written by Logan F. Smyth Š 2009
 * http://logansmyth.com
 * me@logansmyth.com
 */



#ifndef __DEFINE_NGMPRIS_H__
#define __DEFINE_NGMPRIS_H__

#include "ngIMpris.h"

#include <deque>

#include <dbus/dbus.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIObserver.h>

class sbDBusConnection;
class nsITimer;


union ngDBusBasicValue { char* string; unsigned char bytes[8]; };

/* Header file */
class ngDBusConnection : public nsIObserver, public ngIDBusConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NGIDBUSCONNECTION

  ngDBusConnection();
  virtual ~ngDBusConnection();

private:
  NS_IMETHOD Check();
  const char* ngTypeToDBusType(const int ngType) const;

protected:
  nsAutoPtr<sbDBusConnection> mConn;
  DBusMessage *signal_msg;
  ngIMethodHandler* mHandler;
  std::deque<DBusMessageIter*> outgoing_args;
  DBusMessageIter incoming_args;
  nsCOMPtr<nsITimer> mTimer;
  /* additional members */
};

#endif