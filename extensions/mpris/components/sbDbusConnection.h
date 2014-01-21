/*
 * Written by Logan F. Smyth © 2009
 * http://logansmyth.com
 * me@logansmyth.com
 * 
 * Feel free to use/modify this code, but if you do
 * then please at least tell me!
 *
 */



#ifndef __DEFINE_SBMPRIS_H__
#define __DEFINE_SBMPRIS_H__

#include <iostream>
//#include <string>
//#include <cstring>
#include <deque>

#include <cstdlib>

#include <dbus/dbus.h>

#include "sbIMpris.h"
#include "nsStringAPI.h"


/* Header file */
class sbDbusConnection : public sbIDbusConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDBUSCONNECTION

  sbDbusConnection();

private:
  ~sbDbusConnection();

protected:
  DBusConnection *conn;
  DBusMessage *signal_msg;
  sbIMethodHandler *handler;
  std::deque<DBusMessageIter*> outgoing_args;
  DBusMessageIter incoming_args;
  PRBool debug_mode;
  /* additional members */
};

#endif
