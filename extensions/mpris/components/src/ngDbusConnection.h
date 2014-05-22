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

//#include <cstring>
#include <deque>

#include <dbus/dbus.h>

#include "ngIMpris.h"

union DBusBasicValue { char* string; unsigned char bytes[8]; };

/* Header file */
class ngDbusConnection : public ngIDbusConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NGIDBUSCONNECTION

  ngDbusConnection();

private:
  ~ngDbusConnection();
  const char* ngTypeToDBusType(const int ngType) const;

protected:
  DBusConnection *conn;
  DBusMessage *signal_msg;
  ngIMethodHandler *handler;
  std::deque<DBusMessageIter*> outgoing_args;
  DBusMessageIter incoming_args;
  /* additional members */
};

#endif
