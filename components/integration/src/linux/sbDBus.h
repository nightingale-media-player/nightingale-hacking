/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef _SB_DBUS_H_
#define _SB_DBUS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird D-Bus service defs.
//
//   For D-Bus API documentation, see
// http://dbus.freedesktop.org/doc/api/html/index.html.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDBus.h
 * \brief Songbird D-Bus Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird D-Bus imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsStringGlue.h>

// D-Bus imports.
#include <dbus/dbus.h>


//------------------------------------------------------------------------------
//
// Songbird D-Bus service classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides support for D-Bus messages.
 */

class sbDBusMessage
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  /**
   * Create and return in aMessage a new Songbird D-Bus method call message
   * object with the destination, path, interface, and method specified by
   * aDestination, aPath, aInterface, and aMethod.
   *
   * \param aMessage            Returned, created message.
   * \param aDestination        Message destination.
   * \param aPath               Message path.
   * \param aInterface          Message interface.
   * \param aMethod             Message method.
   */
  static nsresult NewMethodCall(sbDBusMessage** aMessage,
                                const char*     aDestination,
                                const char*     aPath,
                                const char*     aInterface,
                                const char*     aMethod);

  /**
   * Get the variable message arguments specified by aFirstArgType and
   * subsequent arguments.  See documentation for dbus_message_get_args.
   *
   * \param aFirstArgType,...   List of arguments to get.
   */
  nsresult GetArgs(int aFirstArgType,
                   ...);

  /**
   * Assign the base D-Bus message specified by aMessage to the Songbird D-Bus
   * message object.  Unreference any base D-Bus message currently assigned.
   *
   * \param aMessage            Base D-Bus message to assign.
   */
  void Assign(DBusMessage* aMessage);

  /**
   * Return the base D-Bus message data record.
   *
   * \return                    Base D-Bus message data record.
   */
  DBusMessage* get() const
  {
    return mBaseMessage;
  }

  /**
   * Construct a Songbird D-Bus message object using the base D-Bus data record
   * specified by aMessage.
   */
  sbDBusMessage(DBusMessage* aMessage = NULL) : mBaseMessage(aMessage) {}

  /**
   * Destroy the Songbird D-Bus message object.
   */
  virtual ~sbDBusMessage();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  //   mBaseMessage             Base D-Bus message data record.
  //

  DBusMessage*                  mBaseMessage;
};


/**
 * This class provides support for D-Bus connections.
 */

class sbDBusConnection
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  /**
   * Create and return in aConnection a new Songbird D-Bus bus connection of
   * the type specified by aBusType.  For messages sent via this connection,
   * use the destination, path, and interface specified by aDestination, aPath,
   * and a Interface.
   *
   * \param aConnection         Returned, created connection.
   * \param aBusType            Type of bus to which to connect.
   * \param aDestination        Destination of messages.
   * \param aPath               Path of messages.
   * \param aInterface          Interface to use when invoking methods.
   */
  static nsresult New(sbDBusConnection** aConnection,
                      DBusBusType        aBusType,
                      const char*        aDestination,
                      const char*        aPath,
                      const char*        aInterface);

  /**
   *   Invoke the method specified by aMethod with the variable parameters
   * starting with aFirstArgType.  If aReply is not null, wait for a method
   * reply message and return it in aReply.
   *   Use the default message destination, path, and interface set for the
   * connection.
   *   See documentation for dbus_message_append_args.
   *
   * \param aMethod             Method to invoke.
   * \param aReply              Returned reply to method.
   * \param aFirstArgType,...   Variable method arguments.
   */
  nsresult InvokeMethod(const char*     aMethod,
                        sbDBusMessage** aReply,
                        int             aFirstArgType,
                        ...);

  /**
   * Return the base D-Bus connection data record.
   *
   * \return                    Base D-Bus connection data record.
   */
  DBusConnection* get() const
  {
    return mBaseConnection;
  }

  /**
   * Destroy the Songbird D-Bus connection object.
   */
  virtual ~sbDBusConnection();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Private constants.
  //
  //   sMethodCallTimeoutMS     Amount of time in milliseconds to wait for
  //                            D-Bus method calls to complete.
  //

  static const PRUint32 sMethodCallTimeoutMS = 1000;


  //
  // Private fields.
  //
  //   mBaseConnection          Base D-Bus connection data record.
  //   mDestination             Default message destination.
  //   mPath                    Default message path.
  //   mInterface               Default message interface.
  //

  DBusConnection*               mBaseConnection;
  nsCString                     mDestination;
  nsCString                     mPath;
  nsCString                     mInterface;

  /**
   * Initialize the Songbird D-Bus connection with the bus type specified by
   * aBusType and message destination, path, and interface specified by
   * aDestination, aPath, and aInterface.
   *
   * \param aBusType            Type of bus to which to connect.
   * \param aDestination        Destination of messages.
   * \param aPath               Path of messages.
   * \param aInterface          Interface to use when invoking methods.
   */
  nsresult Initialize(DBusBusType aBusType,
                      const char* aDestination,
                      const char* aPath,
                      const char* aInterface);

  /**
   * Construct a Songbird D-Bus connection object.
   */
  sbDBusConnection();
};


/**
 * This class provides support for D-Bus errors.
 */

class sbDBusError : public DBusError
{
public:

  sbDBusError();

  virtual ~sbDBusError();
};


//------------------------------------------------------------------------------
//
// Songbird D-Bus service macros.
//
//------------------------------------------------------------------------------

/**
 * Post warning on failed ensure success using the error specified by aError and
 * return value expression specified by aReturnValue.
 *
 * \param aError                D-Bus error.
 * \param aReturnValue          Return value expression.
 */

#if defined(DEBUG)

#define SB_DBUS_ENSURE_SUCCESS_BODY(aError, aReturnValue)                      \
  char* msg = PR_smprintf("SB_DBUS_ENSURE_SUCCESS(%s, %s) failed with "        \
                          "result %s",                                         \
                          #aError,                                             \
                          #aReturnValue,                                       \
                          aError.message);                                     \
  NS_WARNING(msg);                                                             \
  PR_smprintf_free(msg);

#else

#define SB_DBUS_ENSURE_SUCCESS_BODY(aError, aReturnValue)                      \
  NS_WARNING("SB_DBUS_ENSURE_SUCCESS(" #aError ", " #aReturnValue ") failed");

#endif


/**
 * Ensure that the D-Bus error specified by aError indicates success.  If it
 * doesn't post a warning with the D-Bus error message and return from the
 * current function with the return value specified by aReturnValue.
 *
 * \param aError                D-Bus error.
 * \param aReturnValue          Return value expression.
 */

#define SB_DBUS_ENSURE_SUCCESS(aError, aReturnValue)                           \
  PR_BEGIN_MACRO                                                               \
    if (dbus_error_is_set(&aError)) {                                          \
      SB_DBUS_ENSURE_SUCCESS_BODY(aError, aReturnValue)                        \
      return aReturnValue;                                                     \
    }                                                                          \
  PR_END_MACRO


#endif // _SB_DBUS_H_

