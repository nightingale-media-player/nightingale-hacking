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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird D-Bus services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDBus.cpp
 * \brief Songbird D-Bus Services Source.
 */

//------------------------------------------------------------------------------
//
// Songbird D-Bus imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDBus.h"

// Mozilla imports.
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// Songbird D-Bus message services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// NewMethodCall
//

/* static */ nsresult
sbDBusMessage::NewMethodCall(sbDBusMessage** aMessage,
                             const char*     aDestination,
                             const char*     aPath,
                             const char*     aInterface,
                             const char*     aMethod)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMessage);

  // Create a Songbird D-Bus message.
  nsAutoPtr<sbDBusMessage> message(new sbDBusMessage());
  NS_ENSURE_TRUE(message, NS_ERROR_OUT_OF_MEMORY);

  // Create a D-Bus method call message.
  DBusMessage* baseMessage = dbus_message_new_method_call(aDestination,
                                                          aPath,
                                                          aInterface,
                                                          aMethod);
  NS_ENSURE_TRUE(baseMessage, NS_ERROR_OUT_OF_MEMORY);
  message->Assign(baseMessage);

  // Return results.
  *aMessage = message.forget();

  return NS_OK;
}


//-------------------------------------
//
// Assign
//

void
sbDBusMessage::Assign(DBusMessage* aMessage)
{
  // Unreference current base message.
  if (mBaseMessage)
    dbus_message_unref(mBaseMessage);

  // Assign new, already ref'ed base message.
  mBaseMessage = aMessage;
}


//-------------------------------------
//
// GetArgs
//

nsresult
sbDBusMessage::GetArgs(int           aFirstArgType,
                       ...)
{
  sbDBusError error;

  // Get the message arguments.
  va_list varArgs;
  va_start(varArgs, aFirstArgType);
  dbus_message_get_args_valist(mBaseMessage, &error, aFirstArgType, varArgs);
  va_end(varArgs);
  SB_DBUS_ENSURE_SUCCESS(error, NS_ERROR_FAILURE);

  return NS_OK;
}


//-------------------------------------
//
// ~sbDBusMessage
//

sbDBusMessage::~sbDBusMessage()
{
  // Unreference base message.
  if (mBaseMessage)
    dbus_message_unref(mBaseMessage);
}


//------------------------------------------------------------------------------
//
// Songbird D-Bus connection services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// New
//

/* static */ nsresult
sbDBusConnection::New(sbDBusConnection** aConnection,
                      DBusBusType        aBusType,
                      const char*        aDestination,
                      const char*        aPath,
                      const char*        aInterface)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aConnection);

  // Function variables.
  nsresult rv;

  // Create and initialize a new connection.
  nsAutoPtr<sbDBusConnection> connection(new sbDBusConnection());
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);
  rv = connection->Initialize(aBusType, aDestination, aPath, aInterface);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aConnection = connection.forget();

  return NS_OK;
}


//-------------------------------------
//
// Initialize
//

nsresult
sbDBusConnection::Initialize(DBusBusType aBusType,
                             const char* aDestination,
                             const char* aPath,
                             const char* aInterface)
{
  sbDBusError error;

  // Get a bus connection.
  mBaseConnection = dbus_bus_get(aBusType, &error);
  SB_DBUS_ENSURE_SUCCESS(error, NS_ERROR_FAILURE);

  // Set connection info.
  if (aDestination)
    mDestination.Assign(aDestination);
  if (aPath)
    mPath.Assign(aPath);
  if (aInterface)
    mInterface.Assign(aInterface);

  return NS_OK;
}


//-------------------------------------
//
// InvokeMethod
//

nsresult
sbDBusConnection::InvokeMethod(const char*     aMethod,
                               sbDBusMessage** aReply,
                               int             aFirstArgType,
                               ...)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMethod);

  // Function variables.
  dbus_bool_t success;
  sbDBusError error;
  nsresult    rv;

  // Create a new D-Bus method call message.
  nsAutoPtr<sbDBusMessage> dBusMessage;
  rv = sbDBusMessage::NewMethodCall(getter_Transfers(dBusMessage),
                                    mDestination.get(),
                                    mPath.get(),
                                    mInterface.get(),
                                    aMethod);
  NS_ENSURE_SUCCESS(rv, rv);

  // Automatically start message destination.
  dbus_message_set_auto_start(dBusMessage->get(), TRUE);

  // Add arguments to message.
  if (aFirstArgType != DBUS_TYPE_INVALID) {
    va_list varArgs;
    va_start(varArgs, aFirstArgType);
    success = dbus_message_append_args_valist(dBusMessage->get(),
                                              aFirstArgType,
                                              varArgs);
    va_end(varArgs);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  // Send message and get reply if requested.
  if (aReply) {
    // Create a Songbird D-Bus message for the reply.
    nsAutoPtr<sbDBusMessage> reply(new sbDBusMessage());
    NS_ENSURE_TRUE(reply, NS_ERROR_OUT_OF_MEMORY);

    // Send the message and wait for the reply.
    DBusMessage* baseReply;
    baseReply = dbus_connection_send_with_reply_and_block(mBaseConnection,
                                                          dBusMessage->get(),
                                                          sMethodCallTimeoutMS,
                                                          &error);
    SB_DBUS_ENSURE_SUCCESS(error, NS_ERROR_FAILURE);
    reply->Assign(baseReply);

    // Return results.
    *aReply = reply.forget();
  }
  else {
    // Send the message.
    success = dbus_connection_send(mBaseConnection, dBusMessage->get(), NULL);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}


//-------------------------------------
//
// sbDBusConnection
//

sbDBusConnection::sbDBusConnection() :
  mBaseConnection(NULL)
{
}


//-------------------------------------
//
// ~sbDBusConnection
//

sbDBusConnection::~sbDBusConnection()
{
  // Unreference the base D-Bus connection.
  if (mBaseConnection)
    dbus_connection_unref(mBaseConnection);
}


//------------------------------------------------------------------------------
//
// Songbird D-Bus error services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbDBusError
//

sbDBusError::sbDBusError()
{
  // Initialize the D-Bus error.
  dbus_error_init(this);
}


//-------------------------------------
//
// ~sbDBusError
//

sbDBusError::~sbDBusError()
{
  // Free the D-Bus error.
  dbus_error_free(this);
}

